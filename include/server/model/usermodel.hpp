#ifndef USERMODEL_H
#define USERmODEL_H

#include <user.hpp>

class UserModel
{
public:
    // User��Ĳ��뷽��
    bool insert(User &user);

    // �����û��ĺ����ѯ�û�����Ϣ
    User query(int id);

    // �����û���״̬��Ϣ
    bool updateState(User user);

    // �����û���״̬��Ϣ
    void resetState();

};

#endif