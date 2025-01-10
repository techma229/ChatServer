#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>

using namespace std;

class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group& group);
    // 加入群组
    void addGroup(int userid, int groupid, string role);
    // 查询用户所在组的信息
    vector<Group> queryGroups(int userid);
    // 根据指定的groupid和userid查询群组用户id列表，除userid自己，用于用户群聊业务向组内其他用户发消息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif