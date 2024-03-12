#ifndef _CONNECITON_H_
#define _CONNECITON_H_
#include <string>
#include <mysql/mysql.h>
#include <ctime>
using namespace std;

class Connection
{
public:
    Connection();
    ~Connection();
    // 连接数据库
    bool connect(
        string ip,
        unsigned short port,
        string user,
        string passworld,
        string dbname);
    // 更新操作：insert、delect、update
    bool update(string sql);
    // 查询操作 select
    MYSQL_RES *query(string sql);
    // 刷新一下连接起始的空闲时间点
    void refreshAliveTime() { _alivetime = clock(); }
    // 返回存活时间
    clock_t getAliveTime() const { return clock() - _alivetime; }

private:
    MYSQL *_conn;       // 表示和MySQL Server的一条连接
    clock_t _alivetime; // 记录进入空闲状态后的起始存活时间
};

#endif