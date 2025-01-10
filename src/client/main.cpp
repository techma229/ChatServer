#include <iostream>
#include <thread>
#include <unistd.h>
#include <semaphore.h>
#include <atomic>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unordered_map>
#include "json.hpp"
#include "user.hpp"
#include "group.hpp"
#include "public.hpp"

using namespace std;
using json = nlohmann::json;

// 记录当前系统登录的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;
// 记录当前登录用户的离线消息
vector<string> g_currentUserOffline;

// 控制主菜单应用程序
bool isMainMenuRunning = false;

// 用户读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool g_isLoginSuccess;

// 接收线程
void readTaskHandler(int clientfd);
// 获取系统时间（聊天信息需要显示时间）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int);
// 显示当前登录成功用户的信息
void showCurrentUserData();

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd)
    {
        cerr << "socket create error!" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息ip和port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client与server进行连接
    if(-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程用的信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功，启动接收子线程
    thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    // main线程用于接收用户输入，收发数据
    while(1)
    {
        // 显示首页面菜单 登录、注册、退出
        cout << "==============================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "==============================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        cin.get();                  // 读取缓冲区残留的回车

        switch (choice)
        {
            case 1:
            {
                // 用户输入id和密码
                int id = 0;
                char pwd[50] = {0};
                cout << "userid: ";
                cin >> id;
                cin.get();              // 读取缓冲区残留的回车
                cout << "userpassword: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = LOGIN_MSG;
                js["id"] = id;
                js["password"] = pwd;
                string request = js.dump();

                g_isLoginSuccess = false;

                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
                if(-1 == len)
                {
                    cerr << "send login msg error" << request << endl;
                }

                sem_wait(&rwsem);               // 等待信号量，由子线程处理完登录的响应消息后，通知这里

                if(g_isLoginSuccess)
                {
                    // 进入聊天主界面
                    isMainMenuRunning = true;
                    mainMenu(clientfd);
                }
            }
            break;
        case 2:     // register业务
            {
                char name[50] = {0};
                char pwd[50] = {0};
                cout << "username: ";
                cin.getline(name, 50);
                cout << "password: ";
                cin.getline(pwd, 50);

                json js;
                js["msgid"] = REGISTER_MSG;
                js["name"] = name;
                js["password"] = pwd;
                string request = js.dump();

                int len = send(clientfd, request.c_str(), strlen(request.c_str())+1, 0);
                if(len == -1)
                {
                    cerr << "send reg msg error: " << request << endl;
                }

                sem_wait(&rwsem);           //等待信号量，子线程处理完注册消息会通知
            }
            break;
            case 3:
                close(clientfd);
                sem_destroy(&rwsem);
                exit(0);
            break;
        
        default:
            cerr << "invalid input!" << endl;
            break;
        }
    }

    return 0;
}

// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{
    if(0 != responsejs["errno"])            // 注册失败
    {
        cerr << "name already exists, register error" <<endl;
    }
    else                                    // 注册成功
    {
        cout << "name register success, userid is " << responsejs["id"]
                << ", don't forget it" << endl;
    }
}

// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    if(0 != responsejs["errno"])            // 登录失败
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else                                    // 登录成功
    {
        // 记录当前用户的id和name
        g_currentUser.setId(responsejs["id"]);
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友信息
        if(responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for(string& str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setId(js["id"]);
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user);
            }
        }

        // 记录当前用户的群组列表信息
        if(responsejs.contains("groups"))
        {
            // 初始化
            g_currentUserGroupList.clear();

            // 字符串=》json=》group=》groupList
            vector<string> vecGroup = responsejs["groups"];
            for(string &groupStr : vecGroup)
            {
                json groupJs = json::parse(groupStr);
                Group group;
                group.setId(groupJs["id"]);
                group.setName(groupJs["groupname"]);
                group.setDesc(groupJs["groupdesc"]);

                vector<string> vec2 = groupJs["users"];
                for(string &userStr : vec2)
                {
                    json userJs = json::parse(userStr);
                    GroupUser user;
                    user.setId(userJs["id"]);
                    user.setName(userJs["name"]);
                    user.setState(userJs["state"]);
                    user.setRole(userJs["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }
        }

        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息 个人聊天消息或群组消息
        if(responsejs.contains("offlineMsg"))
        {
            vector<string> vec = responsejs["offlineMsg"];
            for(string &str : vec)
            {
                json js = json::parse(str);
                // time + [id] + name + " said: " + msg
                if(ONE_CHAT_MSG == js["msgid"])
                {
                    cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                        << " said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "group msg:[" << js["groupid"] << "]:" << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                        << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}

// 子线程 接收线程
void readTaskHandler(int clientfd)
{
    while(1)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, sizeof(buffer), 0);    // 阻塞
        if(-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成Json对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"];

        // 一对一聊天消息
        if(ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"] << "[" << js["id"] << "]" << js["name"] 
                    << "said: " << js["msg"] << endl;
            continue;
        }

        // 群聊天消息
        if(GROUP_CHAT_MSG == msgtype)
        {
            cout << "group msg[" << js["groupid"] << "]: " << js["time"] << "[" << js["id"] << "]" << js["name"]
                <<"said: " << js["msg"] << endl;
            continue;
        }

        // 登录应答消息
        if(LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js);        // 处理登录响应的业务逻辑
            sem_post(&rwsem);           // 通知主线程，登录结果处理完成
            continue;

        }

        // 注册应答消息
        if(REGISTER_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);           // 通知主线程，注册结果处理完成
        }

    }
}

// 显示登录用户的基本信息
void showCurrentUserData()
{
    cout << "===============login user===============" << endl;
    cout << "current login user -> id: " << g_currentUser.getId() << " name: " << g_currentUser.getName() << endl;

    cout << "---------------friend list--------------" << endl;
    if(!g_currentUserFriendList.empty())
    {
        for(User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }

    cout << "---------------group list--------------" << endl;
    if(!g_currentUserGroupList.empty())
    {
        for(Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for(GroupUser &user : group.getUsers())
            {
                cout << "\t" << user.getId() << " " << user.getName() << " " << user.getName() 
                        << " " <<user.getRole() << " " << user.getState() << endl;
            }
        }
    }
    cout << "===================================" << endl;

}

// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = 
{
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"loginout", "注销，格式loginout"}
};

// 注册系统支持的客户端命令处理器
unordered_map<string, function<void(int, string)>> commandHandlerMap = 
{
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}
};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    cout << "mainMenu" << endl;
    help();

    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer, sizeof(buffer));
        string commandbuf(buffer);
        string command;                         // 存储命令

        int idx = commandbuf.find(":");
        if(-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if(it == commandHandlerMap.end())
        {
            cerr << "invalid input!" << endl;
            continue;
        }
        // 调用command对应的回调函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

// "help" command handler
void help(int, string)
{
    cout << "show command list >>> " << endl;
    for(auto &p : commandMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

// "chat" command handler   chat:friendid:message
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if(-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx+1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        cerr << "send chat msg error -> " << buffer << endl; 
    }
}
// "addfriend" command handler  addfriend:friendid
void addfriend(int clientfd, string str) 
{
    int friendid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str())+1, 0);
    if(-1 == len)
    {
        cerr << "send addfriend msg error -> " << buffer << endl;
    }
}
// "creategroup" command handler creategroup:groupname:groupdesc
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if(-1 == idx)
    {
        cerr << "creategroup command invalid!" << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        cerr << "send creategroup msg error -> " << buffer << endl;
    }
}
// "addgroup" command handler addgroup:groupid
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());

    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        cerr << "send addgroup msg error -> " << buffer << endl;
    }
}
// "groupchat" command handler groupchat:groupid:message
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["name"] = g_currentUser.getName();
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        cerr << "send groupchat msg error -> " << buffer << endl;
    }
}
// "loginout" command handler   loginout
void loginout(int client, string str)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();
    int len = send(client, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if(-1 == len)
    {
        cerr << "send loginmsg error!" << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

// 获取系统时间（聊天信息需要显示时间）
string getCurrentTime()
{
    auto tt = chrono::system_clock::to_time_t(chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d", 
                (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
                (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
    return string(date);
}