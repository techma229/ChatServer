#ifndef USERMODEL_H
#define USERmODEL_H

#include <user.hpp>

class UserModel
{
public:
    // User表的插入方法
    bool insert(User &user);

    // 根据用户的号码查询用户的信息
    User query(int id);

    // 更新用户的状态信息
    bool updateState(User user);

    // 重置用户的状态信息
    void resetState();

};

#endif