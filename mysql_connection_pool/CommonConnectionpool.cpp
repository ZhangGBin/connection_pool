#include "CommonConnectionpool.h"
#include "Connection.h"
#include "public.h"

ConnectionPool::~ConnectionPool()
{
    while (!_connectionQue.empty())
    {
        delete _connectionQue.front();
        _connectionQue.pop();
    }
    flag = false;
    cv.notify_all(); // 给个信号让生产者线程不再阻塞
}

// 线程安全的懒汉单例函数接口
ConnectionPool *ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool; // lock和unlock
    return &pool;
}

// 从配置文件中加载配置项
bool ConnectionPool::loadConfigFile()
{
    FILE *pf = fopen("mysql.ini", "r");
    if (pf == nullptr)
    {
        LOG("mysql.ini file is not exist!");
        return false;
    }

    while (!feof(pf))
    {
        char line[1024] = {0};
        fgets(line, 1024, pf);
        string str = line;
        int idx = str.find('=', 0);
        if (idx == -1) // 无效的配置项
        {
            continue;
        }

        // password=123456\n
        int endidx = str.find('\n', idx);
        string key = str.substr(0, idx);
        string value = str.substr(idx + 1, endidx - idx - 1);

        if (key == "ip")
        {
            _ip = value;
        }
        else if (key == "port")
        {
            _port = atoi(value.c_str());
        }
        else if (key == "username")
        {
            _username = value;
        }
        else if (key == "password")
        {
            _password = value;
        }
        else if (key == "dbname")
        {
            _dbname = value;
        }
        else if (key == "initSize")
        {
            _initSize = atoi(value.c_str());
        }
        else if (key == "maxSize")
        {
            _maxSize = atoi(value.c_str());
        }
        else if (key == "maxIdleTime")
        {
            _maxIdleTime = atoi(value.c_str());
        }
        else if (key == "connectionTimeOut")
        {
            _connectionTimeout = atoi(value.c_str());
        }
    }
    return true;
}
// // 从配置文件中加载配置项
// bool ConnectionPool::loadConfigFile()
// {
//     FILE *pf = fopen("mysql.ini", "r");
//     if (pf == nullptr)
//     {  //这里
//         LOG("mysql.ini file is not exist!");
//         while (!feof(pf))
//         {
//             char line[1024] = {0};
//             fgets(line, 1024, pf);
//             string str = line;
//             int idx = str.find('=', 0);
//             if (idx == -1)
//             {
//                 continue;
//                 return false;
//             }
//             // password = '123456';
//             int endidx = str.find('\n', idx);
//             string key = str.substr(0, idx);
//             string value = str.substr(idx + 1, endidx - idx - 1);
//             if (key == "ip")
//             {
//                 _ip = value;
//             }
//             else if (key == "port")
//             {
//                 _port = atoi(value.c_str());
//             }
//             else if (key == "username")
//             {
//                 _username = value;
//             }
//             else if (key == "password")
//             {
//                 _password = value;
//             }
//             else if (key == "dbname")
//             {
//                 _dbname = value;
//             }
//             else if (key == "maXSize")
//             {
//                 _maxSize = atoi(value.c_str());
//             }
//             else if (key == "maxIdleTime")
//             {
//                 _maxIdleTime = atoi(value.c_str());
//             }
//             else if (key == "connectionTimeout")
//             {
//                 _connectionTimeout = atoi(value.c_str());
//             }
//         }
//     }
//     return true;
// }

// 连接池的构造
ConnectionPool::ConnectionPool()
{
    // 加载配置项
    if (!loadConfigFile())
    {
        return;
    }
    for (int i = 0; i < _initSize; ++i)
    {
        Connection *p = new Connection();
        p->connect(_ip, _port, _username, _password, _dbname);
        p->refreshAliveTime();  // 刷新开始空闲的起始时间
        _connectionQue.push(p); // 把创建好的数据库连接传入数据库连接队列中
        _connectionCnt++;       // 队列的大小++

        // 启用一个新的线程，作为连接的生产者 Linux thread => pthread_creatr;
        thread produce(std::bind(&ConnectionPool::produceConnnectionTask, this));
        produce.detach(); // 线程分离

        // 启用一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行连接回收
        thread scanner(std::bind(&ConnectionPool::scanerConnectionTask, this));
        scanner.detach();
    }
}

// 运行在独立的线程中，负责产生新的连接
void ConnectionPool::produceConnnectionTask()
{
    for (;;)
    {
        unique_lock<mutex> lock(_queueMutex);
        while (!_connectionQue.empty())
        {
            cv.wait(lock); // 队列不空，此处生产线程进入阻塞状态
        }
        if (!flag)
        {
            break;
        }

        // 连接数量没达到上限继续创建新的连接
        if (_connectionCnt < _maxSize)
        {
            Connection *p = new Connection();
            p->connect(_ip, _port, _username, _password, _dbname);
            p->refreshAliveTime();
            _connectionQue.push(p);
            _connectionCnt++;
        }
        // 通知消费者线程，可以消费连接了
        cv.notify_all();
    }
}

// 给外部提供接口，从线程池中获取一个可用的空闲连接
shared_ptr<Connection> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(_queueMutex); // 上锁
    while (_connectionQue.empty())
    {
        if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeout)))
            if (_connectionQue.empty())
            {
                LOG("获取空闲连接超时...获取连接失败！");
                return nullptr;
            }
    }

    /*
    shared_ptr智能指针析构时，会把connection资源delect掉，相当于
    调用connection的析构函数，connection就被close掉了。
    这里需要自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
    */

    shared_ptr<Connection> sp(_connectionQue.front(),
                              [&](Connection *pcon)
                              {
                                  // 这里是在服务器应用程序中调用的，所以一定要考虑队列的线程安全操作
                                  unique_lock<mutex> lock(_queueMutex);
                                  pcon->refreshAliveTime(); // 刷新一下开始空闲的的时间
                                  _connectionQue.push(pcon);
                              });
    _connectionQue.pop();
    cv.notify_all(); // 消费完连接后，通知生产者线程检查一下，如果队列为空，赶紧生产连接
    return sp;
}

// 扫描超过maxIdleTime时间的空闲连接，进行对应连接回收
void ConnectionPool::scanerConnectionTask()
{
    for (;;)
    {
        // 通过sleep模拟定时效果
        this_thread::sleep_for(chrono::seconds(_maxIdleTime));

        // 扫描整个队列，释放多余的连接
        unique_lock<mutex> lock(_queueMutex);
        while (_connectionCnt > _maxSize)
        {
            Connection *p = _connectionQue.front();
            if (p->getAliveTime() >= (_maxIdleTime * 1000))
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p; // 调用~Connection()释放内存
            }
            else
            {
                break; // 对头的连接没有超过_maxIdleTime,其他连接肯定没有
            }
        }
    }
}