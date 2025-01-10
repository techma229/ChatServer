#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include "group.hpp"
#include "groupuser.hpp"

using namespace std;
using namespace muduo;

// ChatService ����ģʽ
ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}

// ��ȡ��Ӧ��Ϣ�Ĵ�����
MsgHandler ChatService::getHandler(int msgid)
{
    // �Ҳ�����Ӧ�����������
    auto it = _msgHandlerMap.find(msgid);

    if(it == _msgHandlerMap.end())
    {
        // ����һ��Ĭ�ϵĴ�����
        return [=](const TcpConnectionPtr& conn, json& js, Timestamp time)
        {
            LOG_ERROR << "msgid: " << msgid << " can not find handler!";
        };
    }
    return _msgHandlerMap[msgid];
}

// ע��ҵ��
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

    // �����û���״̬��Ϣ
    User user(userid, "", "", "offline");
    _userModel.updateState(user);

    // ��redis��ȡ������
    _redis.unsubscribe(userid);
}

// ����ͻ����쳣�˳�
void ChatService::clientCloseExceptionHandler(const TcpConnectionPtr& conn)
{
    User user;
    // ����������
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if(conn == it->second)
            {
                // ��map����ɾ���û�������Ϣ
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // ��redis��ȡ������
    _redis.unsubscribe(user.getId());

    // �û����ڣ������״̬
    if(user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// ��¼ҵ��
void ChatService::loginHandler(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    LOG_INFO << "do login service!";

    int id = js["id"];
    string password = js["password"];

    User user = _userModel.query(id);
    if(user.getId() == id && user.getPassword() == password)
    {
        //id �� ����������ȷ
        if(user.getState() == "online")
        {
            // �û��Ѿ���¼�������ظ���¼
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account has logged in, input another!";
            conn->send(response.dump());
        }
        else
        {
            // ��¼�ɹ��������û���״̬��Ϣ
            user.setState("online");
            _userModel.updateState(user);

            // id�û���¼�ɹ�����redis����channel(id)
            _redis.subscribe(id);

            // ��¼�û���������Ϣ
            // �迼���̰߳�ȫ���⣬
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // ��ѯ�û��Ƿ���������Ϣ
            vector<string> vec = _offLineMsgModel.query(id);
            if(!vec.empty())
            {
                response["offlineMsg"] = vec;
                // ��ȡ���û���������Ϣ�󣬽����û���������Ϣɾ����
                _offLineMsgModel.remove(id);
            }
            else
            {
                LOG_INFO << "��������Ϣ";
            }

            // ��ʾ�û��ĺ�����Ϣ
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

            // ��ʾ�û���Ⱥ����Ϣ
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
// ע��ҵ��
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
        // ע��ɹ�
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // ע��ʧ��
        json response;
        response["msgid"] = REGISTER_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// һ��һ����ҵ��
void ChatService::oneChatHandler(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    // ��Ҫ������Ϣ���û�id
    int toId = js["toid"];

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toId);
        // ȷ��������״̬
        if(it != _userConnMap.end())
        {
            it->second->send(js.dump());
            return;
        }
    }

    // ��ѯtoid�Ƿ�����
    User user = _userModel.query(toId);
    if(user.getState() == "online")
    {
        _redis.publish(toId, js.dump());
        return;
    }

    // �û������ߣ���洢������Ϣ
    _offLineMsgModel.insert(toId, js.dump());
}

// ��Ӻ���ҵ��
void ChatService::addFriendHandler(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userId = js["id"];
    int friendId = js["friendid"];

    // �洢������Ϣ
    _friendModel.insert(userId, friendId);
}

// ����Ⱥ��ҵ��
void ChatService::createGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userId = js["id"];
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // �洢�½���Ⱥ����Ϣ
    Group group(-1, name, desc);
    if(_groupModel.createGroup(group))
    {
        // �洢Ⱥ�鴴���˵���Ϣ
        _groupModel.addGroup(userId, group.getId(), "creator");
    }
}
// ����Ⱥ��ҵ��
void ChatService::addGroup(const TcpConnectionPtr& conn, json& js, Timestamp time)
{
    int userId = js["id"];
    int groupId = js["groupid"];
    _groupModel.addGroup(userId, groupId, "normal");
}
// Ⱥ������ҵ��
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
            // ת��Ⱥ��Ϣ
            it->second->send(js.dump());
        }
        else
        {
            User user = _userModel.query(id);
            if(user.getState() == "online")
            {
                _redis.publish(id, js.dump());
            }
           // �洢������Ϣ
            _offLineMsgModel.insert(id, js.dump());
        }
    }
}

ChatService::ChatService()
{
    // �Ը�����Ϣ��������ע��
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&ChatService::loginHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginOut, this, _1, _2, _3)});
    _msgHandlerMap.insert({REGISTER_MSG, std::bind(&ChatService::registerHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&ChatService::oneChatHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&ChatService::addFriendHandler, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::chatGroup, this, _1, _2, _3)});

    // ����redis������
    if(_redis.connect())
    {
        // �����ϱ���Ϣ�Ļص�
        _redis.init_notify_handler(std::bind(&ChatService::redis_subscribe_message_handler, this, _1, _2));
    }
}

// ������ �쳣��ֹ��Ĳ��� �����û���״̬����
void ChatService::reset()
{
    _userModel.resetState();
}

// redis������Ϣ�����Ļص�����
void ChatService::redis_subscribe_message_handler(int channel, string message)
{
    // �û�����
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(channel);
    if(it != _userConnMap.end())
    {
        it->second->send(message);
        return;
    }
    // ת������
    _offLineMsgModel.insert(channel, message);
}