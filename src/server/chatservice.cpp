#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include "group.hpp"
#include "groupuser.hpp"

using namespace std;
using namespace muduo;

// ChatService 单例模式
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 获取对应消息的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    // 找不到对应处理器的情况
    auto it = _msgHandlerMap.find(msgid);

    if(it == _msgHandlerMap.end())
    {
        // 返回一个默认的处理器
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    }
    return _msgHandlerMap[msgid];
}

// 注销业务
void ChatService::loginOut(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);

    // 在redis中取消订阅
    _redis.unsubscribe(userid);
}

// 处理客户端异常退出
void ChatService::clientCloseExceptionHandler(const TcpConnectionPtr& conn)
{
    User user;
    // 互斥锁保护
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if(conn == it->second)
            {
                // 从map表中删除用户连接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 在redis中取消订阅
    _redis.unsubscribe(user.getId());

    // 用户存在，则更新状态
    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 登录业务
void ChatService::loginHandler(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    LOG_INFO << "do login service!";

    int id = js["id"];
    string password = js["password"];

    User user = _userModel.query(id);
    if(user.getId() == id && user.getPassword() == password)
    {
        //id 和 密码输入正确
        if(user.getState() == "online")
        {
            // 用户已经登录，不能重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account has logged in, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，更新用户的状态信息
            user.setState("online");
            _userModel.updateState(user);

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            // 记录用户的连接信息
            // 需考虑线程安全问题，
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询用户是否有离线消息
            vector<string> vec = _offLineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlineMsg"] = vec;
                // 读取该用户的离线消息后，将该用户的离线消息删除掉
                _offLineMsgModel.remove(id);
            }
            else
            {
                LOG_INFO << "无离线消息";
            }

            // 显示用户的好友信息
            vector<User> vecFriend = _friendModel.query(id);
            if(!vecFriend.empty())
            {
                vector<string> vec;
                for(auto& user : vecFriend)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec.push_back(js.dump());
                }
                response["friends"] = vec;
            }

            // 显示用户的群组信息
            vector<Group> vecGroup = _groupModel.queryGroups(id);
            if(!vecGroup.empty())
            {
                vector<string> vec;
                for(auto& group : vecGroup)
                {
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();
                    vector<GroupUser> vecUsers = group.getUsers();
                    vector<string> vecUserStr;
                    for(auto &user : vecUsers)
                    {
                        json userJs;
                        userJs["id"] = user.getId();
                        userJs["name"] = user.getName();
                        userJs["state"] = user.getState();
                        userJs["role"] = user.getRole();
                        vecUserStr.push_back(userJs.dump());
                    }
                    js["users"] = vecUserStr;
                    vec.push_back(js.dump());
                }
                response["groups"] = vec;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = -1;
        response["errmsg"] = "id or password wrong!";

        conn->send(response.dump());
    }
}
// 注册业务
void ChatService::registerHandler(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    LOG_INFO << "do register service!";

    string name = js["name"];
    string password = js["password"];

    User user;
    user.setName(name);
    user.setPassword(password);
    bool state = _userModel.insert(user);
    if(state)
    {
        // 注册成功
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 一对一聊天业务
void ChatService::oneChatHandler(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    // 需要接收信息的用户id
    int toId = js["toid"];

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toId);
        // 确认是在线状态
        if(it != _userConnMap.end())
        {
            it->second->send(js.dump());
            return;
        }
    }

    // 查询toid是否在线
    User user = _userModel.query(toId);
    if(user.getState() == "online")
    {
        _redis.publish(toId, js.dump());
        return;
    }

    // 用户不在线，则存储离线消息
    _offLineMsgModel.insert(toId, js.dump());
}

// 添加好友业务
void ChatService::addFriendHandler(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userId = js["id"];
    int friendId = js["friendid"];

    // 存储好友信息
    _friendModel.insert(userId, friendId);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userId = js["id"];
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新建的群组消息
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group))
    {
        // 存储群组创建人的消息
        _groupModel.addGroup(userId, group.getId(), "creator");
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userId = js["id"];
    int groupId = js["groupid"];
    _groupModel.addGroup(userId, groupId, "normal");
}
// 群组聊天业务
void ChatService::chatGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userId = js["id"];
    int groupId = js["groupid"];
    vector<int> userIdVec = _groupModel.queryGroupUsers(userId, groupId);

    lock_guard<mutex> lock(_connMutex);
    for(auto &id : userIdVec)
    {
        auto it = _userConnMap.find(id);

        if(it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
           // 存储离线消息
            _offLineMsgModel.insert(id, js.dump());
        }
    }
}

ChatService::ChatService()
{
    // 对各类消息处理方法的注册
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::loginHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginOut, this, _1, _2, _3)});
    _msgHandlerMap.insert({REGISTER_MSG, std::bind(&ChatService::registerHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChatHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriendHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::chatGroup, this, _1, _2, _3)});

    // 连接redis服务器
    if(_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::redis_subscribe_message_handler, this, _1, _2));
    }
}

// 服务器 异常终止后的操作 所有用户的状态重置
void ChatService::reset()
{
    _userModel.resetState();
}

// redis订阅消息触发的回调函数
void ChatService::redis_subscribe_message_handler(int channel, string message)
{
    // 用户在线
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(channel);
    if(it != _userConnMap.end())
    {
        it->second->send(message);
        return;
    }
    // 转储离线
    _offLineMsgModel.insert(channel, message);
}