#include "db.h"
#include "muduo/base/Logging.h"

// ���ݿ�������Ϣ
static const string server = "127.0.0.1";
static const string user = "root";
static const string password = "123456";
static const string dbname = "chat";

// ��ʼ�����ݿ�����
MySQL::MySQL()
{
    _conn = mysql_init(nullptr);
}
// �ͷ����ݿ�������Դ
MySQL::~MySQL()
{
    if(_conn != nullptr)
    {
        mysql_close(_conn);
    }
}
// �������ݿ�
bool MySQL::connect()
{
    MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),password.c_str(),
                                    dbname.c_str(), 3306, nullptr, 0);
    if(p != nullptr)
    {
        mysql_query(_conn, "set names gbk");
    }
    else
    {
        LOG_INFO << "connect mysql failed";
    }
    return p;
}
// ���²���
bool MySQL::update(string sql)
{
    if(mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                    << sql << "����ʧ��";
        return false;
    }
    return true;
}
// ��ѯ����
MYSQL_RES *MySQL::query(string sql)
{
    if(mysql_query(_conn, sql.c_str()))
    {
        LOG_INFO << __FILE__ << ":" << __LINE__ << ":"
                    << sql << "��ѯʧ��";
        return nullptr;
    }
    return mysql_use_result(_conn);
}
// ��ȡ����
MYSQL *MySQL::getConnection()
{
    return _conn;
}