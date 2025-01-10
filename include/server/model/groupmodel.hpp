#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>

using namespace std;

class GroupModel
{
public:
    // ����Ⱥ��
    bool createGroup(Group& group);
    // ����Ⱥ��
    void addGroup(int userid, int groupid, string role);
    // ��ѯ�û����������Ϣ
    vector<Group> queryGroups(int userid);
    // ����ָ����groupid��userid��ѯȺ���û�id�б���userid�Լ��������û�Ⱥ��ҵ�������������û�����Ϣ
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif