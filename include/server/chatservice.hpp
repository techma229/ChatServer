#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"

using json = nlohmann::json;
using namespace muduo;
using namespace muduo::net;
using namespace std;

// �ص���������
using MsgHandler = std::function<void(const TcpConnectionPtr&, json&, Timestamp)>;

// ���������ҵ����
class ChatService
{
public:
    // ChatService ����ģʽ
    static ChatService* instance();

    // ��ȡ��Ӧ��Ϣ�Ĵ�����
    MsgHandler getHandler(int msgid);

    // ����ͻ����쳣�˳�
    void clientCloseExceptionHandler(const TcpConnectionPtr& conn);

    // ��¼ҵ��
    void loginHandler(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // ע��ҵ��
    void registerHandler(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // һ��һ����ҵ��
    void oneChatHandler(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // ��Ӻ���ҵ��
    void addFriendHandler(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // ����Ⱥ��ҵ��
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // ����Ⱥ��ҵ��
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // Ⱥ������ҵ��
    void chatGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // ע��ҵ��
    void loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // ������ �쳣��ֹ��Ĳ���
    void reset();
    // redis������Ϣ�����Ļص�����
    void redis_subscribe_message_handler(int channel, string message);
private:
    ChatService();
    ChatService(const ChatService&) = delete;
    ChatService& operator=(const ChatService&) = delete;

    // �洢��Ϣid�����Ӧ��ҵ������
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // �洢�����û���ͨ������
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // ���廥����
    mutex _connMutex;

    // redis��������
    Redis _redis;

    // ���ݲ��������
    UserModel _userModel;
    OffLineMsgModel _offLineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
};

#endif