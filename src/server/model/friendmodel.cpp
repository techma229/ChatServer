#include "friendmodel.hpp"
#include "db.h"
#include <vector>

using namespace std;

// ��Ӻ��ѹ�ϵ
void FriendModel::insert(int userid, int friendid)
{
    // 1.��֯sql���
    char sql[1024] = {0};
    snprintf(sql, sizeof(sql), "insert into friend values(%d, %d)", userid, friendid);

    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}

// �����û������б�
vector<User> FriendModel::query(int userId)
{
    // 1.��֯sql���
    char sql[1024] = {0};

    // ���ϲ�ѯ
    sprintf(sql, "select a.id, a.name, a.state from user a inner join friend b on b.friendid = a.id where b.userid = %d", userId);

    vector<User> vec;
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            // ��userid�û��ĺ��ѵ���Ϣ����vec�з���
            MYSQL_ROW row;
            while((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(stoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                vec.push_back(user);
            }
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}