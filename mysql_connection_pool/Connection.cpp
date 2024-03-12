#include "Connection.h"
#include "public.h"
#include <iostream>

Connection::Connection()
{
    _conn = mysql_init(nullptr);
}

Connection::~Connection()
{
    if (_conn != nullptr)
    {
        mysql_close(_conn);
    }
}

bool Connection::connect(
    string ip,
    unsigned short port,
    string user,
    string passworld,
    string dbname)
{
    MYSQL *p = mysql_real_connect(_conn, ip.c_str(), user.c_str(), passworld.c_str(), dbname.c_str(), port, nullptr, 0);
    return p != nullptr;
}

bool Connection::update(string sql)
{
    //更新操作 insert delect update
    if(mysql_query(_conn,sql.c_str()))
    {
        LOG("更新失败：" + sql);
        return false;
    }
    return true;
}

MYSQL_RES* Connection::query(string sql)
{
    //查询操作select
    if(mysql_query(_conn,sql.c_str()))
    {
        LOG("更新失败" + sql);
        return nullptr;
    }
    return mysql_use_result(_conn);
}

