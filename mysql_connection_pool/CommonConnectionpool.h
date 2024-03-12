#ifndef _COMMONCONNECTIONPOOL_H_
#define _COMMONCONNECTIONPOOL_H_

#include <string>
#include <queue>
#include <mutex>
#include <iostream>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <memory>
#include <functional>
using namespace std;
#include "Connection.h"

/*
    实现连接池功能模块
*/
class ConnectionPool
{
public:
    // 获取连接池实例对象
    static ConnectionPool *getConnectionPool();
    // 给外部提供接口，从连接池中获取一个可用的空闲连接
    shared_ptr<Connection> getConnection();

private:
    // 单例模式#1 构造私有化
    ConnectionPool();
    ~ConnectionPool();

    // 从配置文件中加载配置项
    bool loadConfigFile();

    // 运行在独立的线程中，专门负责生产连接
    void produceConnnectionTask();

    // 扫描超过maxIdleTime 时间的空闲连接负责连接回收
    void scanerConnectionTask();

    string _ip;             // mysql的ip地址
    unsigned short _port;   // mysql 的端口号 3306
    string _username;       // mysql登录用户名
    string _password;       // mysql登陆密码
    string _dbname;         // 连接的数据库名称
    int _initSize;          // 连接池初始化连接数
    int _maxSize;           // 连接池最大连接量
    int _maxIdleTime;       // 连接池最大空闲时间
    int _connectionTimeout; // 连接池获取连接的超时时间
    bool flag;              // 标志程序是否结束，如果结束释放cv

    queue<Connection *> _connectionQue; // 储存mysql的连接队列
    mutex _queueMutex;                  // 维护连接队列线程互斥锁
    atomic_int _connectionCnt;          // 记录连接所创建的connection连接的总数
    condition_variable cv;              // 设置条件变量，用于生产线程和连接消费现场的通信
};

#endif