#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType
{
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK,
    LOGINOUT_MSG,               // ע����Ϣ
    REGISTER_MSG,
    REGISTER_MSG_ACK,
    ONE_CHAT_MSG,               // һ��һ������Ϣ
    ADD_FRIEND_MSG,             // ��Ӻ�����Ϣ
    CREATE_GROUP_MSG,           // ����Ⱥ��
    ADD_GROUP_MSG,              // ����Ⱥ��
    GROUP_CHAT_MSG              // Ⱥ����
};

#endif