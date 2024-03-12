#include "Connection.h"
#include"CommonConnectionpool.h"
#include <iostream>
#include <thread>
#include<vector>
using namespace std;

int main()
{
    vector <thread>t;
    Connection conn;
    //conn.connect("127.0.0.1", 3306, "root", "123456", "chat");
    clock_t begin = clock();
    for (int i = 0; i < 4; ++i)
    {
        thread tt([]()
                  { 
                ConnectionPool *cp = ConnectionPool::getConnectionPool();
            for (int i = 0;i<2500;++i)
            {
                char sql[1024] = {0};
                sprintf(sql, "insert into user(name , age , sex) value ('%s','%d','%s')", "zhangsan", 20, "male");
                shared_ptr<Connection> sp = cp->getConnection();
                sp->update(sql);
                /*
                Connection conn;
                char sql[1024] = {0};
                sprintf(sql, "insert into user(name , age , sex) value ('%s','%d','%s')", "zhangsan", 20, "male");
                conn.connect("127.0.0.1", 3306, "root", "123456", "chat"); // 连接
                conn.update(sql); //存数据
                */
            } });
        t.push_back(move(tt));
    }
    t[1].join();
    t[2].join();
    t[3].join();
    t[0].join();
    clock_t end = clock();
    cout << (end - begin) << endl;
    
}