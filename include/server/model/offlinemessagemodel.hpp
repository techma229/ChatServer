#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <string>
#include <vector>

using namespace std;

// �ṩ������Ϣ��Ĳ����ӿڷ���
class OffLineMsgModel
{
public:
    // �洢�û���������Ϣ
    void insert(int userId, string msg);

    // ɾ���û���������Ϣ
    void remove(int userId);

    // ��ѯ�û���������Ϣ
    vector<string> query(int userId);
};

#endif