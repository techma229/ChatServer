#include "offlinemessagemodel.hpp"
#include "db.h"

// �洢�û���������Ϣ
void OffLineMsgModel::insert(int userId, string msg)
{
    // ��֯sql���
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "insert into offlinemessage values(%d, '%s')", userId, msg.c_str());

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// ɾ���û���������Ϣ
void OffLineMsgModel::remove(int userId)
{
    // ��֯sql���
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "delete from offlinemessage where userid = %d", userId);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// ��ѯ�û���������Ϣ
vector<string> OffLineMsgModel::query(int userId)
{
    // ��֯sql���
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "select message from offlinemessage where userid = %d", userId);

    vector<string> vec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            // ��userid�û���������Ϣ����vec�з���
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                vec.push_back(row[0]);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}