#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType
{
    LOGIN_MSG = 1,
    LOGIN_MSG_ACK,
    LOGINOUT_MSG,               // 注销消息
    REGISTER_MSG,
    REGISTER_MSG_ACK,
    ONE_CHAT_MSG,               // 一对一聊天消息
    ADD_FRIEND_MSG,             // 添加好友消息
    CREATE_GROUP_MSG,           // 创建群组
    ADD_GROUP_MSG,              // 加入群组
    GROUP_CHAT_MSG              // 群聊天
};

#endif