#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include <vector>
#include "user.hpp"

using namespace std;

// ά��������Ϣ�Ĳ����ӿڷ���
class FriendModel
{
public:
    // ��Ӻ��ѹ�ϵ
    void insert(int userid, int friendid);

    // �����û������б�
    vector<User> query(int userId);
};

#endif