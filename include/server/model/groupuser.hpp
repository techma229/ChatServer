#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"
#include <string>

using namespace std;

class GroupUser : public User
{
public:
    GroupUser() = default;
    void setRole(string role) {_role = role;}
    string getRole() {return _role;}
private:
    string _role;
};

#endif