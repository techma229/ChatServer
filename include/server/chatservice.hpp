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

// 回调函数类型
using MsgHandler = std::function<void(const TcpConnectionPtr&, json&, Timestamp)>;

// 聊天服务器业务类
class ChatService
{
public:
    // ChatService 单例模式
    static ChatService* instance();

    // 获取对应消息的处理器
    MsgHandler getHandler(int msgid);

    // 处理客户端异常退出
    void clientCloseExceptionHandler(const TcpConnectionPtr& conn);

    // 登录业务
    void loginHandler(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 注册业务
    void registerHandler(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 一对一聊天业务
    void oneChatHandler(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 添加好友业务
    void addFriendHandler(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 群组聊天业务
    void chatGroup(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 注销业务
    void loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time);
    // 服务器 异常终止后的操作
    void reset();
    // redis订阅消息触发的回调函数
    void redis_subscribe_message_handler(int channel, string message);
private:
    ChatService();
    ChatService(const ChatService&) = delete;
    ChatService& operator=(const ChatService&) = delete;

    // 存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁
    mutex _connMutex;

    // redis操作对象
    Redis _redis;

    // 数据操作类对象
    UserModel _userModel;
    OffLineMsgModel _offLineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;
};

#endif