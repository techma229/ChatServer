#include "usermodel.hpp"
#include "db.h"

// User��Ĳ��뷽��
bool UserModel::insert(User &user)
{
    // ��װsql���
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name, password, state) values('%s', '%s', '%s')", 
                    user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// �����û��ĺ����ѯ�û�����Ϣ
User UserModel::query(int id)
{
    // ��װsql���
    char sql[1024] = {0};
    sprintf(sql, "select * from user where id = %d", id);
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr)
            {
                // ����һ��User����������Ϣ
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);

                return user;
            }
        }
    }
    return User();
}

// �����û���״̬��Ϣ
bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "update user set state = '%s' where id = '%d'", user.getState().c_str(), user.getId());

    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

// �����û���״̬��Ϣ
void UserModel::resetState()
{
    // 1.��װsql���
    char sql[1024] = "update user set state = 'offline' where state = 'online'";

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }

}