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

// ��¼��ǰϵͳ��¼���û���Ϣ
User g_currentUser;
// ��¼��ǰ��¼�û��ĺ����б���Ϣ
vector<User> g_currentUserFriendList;
// ��¼��ǰ��¼�û���Ⱥ���б���Ϣ
vector<Group> g_currentUserGroupList;
// ��¼��ǰ��¼�û���������Ϣ
vector<string> g_currentUserOffline;

// �������˵�Ӧ�ó���
bool isMainMenuRunning = false;

// �û���д�߳�֮���ͨ��
sem_t rwsem;
// ��¼��¼״̬
atomic_bool g_isLoginSuccess;

// �����߳�
void readTaskHandler(int clientfd);
// ��ȡϵͳʱ�䣨������Ϣ��Ҫ��ʾʱ�䣩
string getCurrentTime();
// ������ҳ�����
void mainMenu(int);
// ��ʾ��ǰ��¼�ɹ��û�����Ϣ
void showCurrentUserData();

int main(int argc, char* argv[])
{
    if(argc < 3)
    {
        cerr << "command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // ����ͨ�������д��ݵ�ip��port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // ����client�˵�socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd)
    {
        cerr << "socket create error!" << endl;
        exit(-1);
    }

    // ��дclient��Ҫ���ӵ�server��Ϣip��port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client��server��������
    if(-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // ��ʼ����д�߳��õ��ź���
    sem_init(&rwsem, 0, 0);

    // ���ӷ������ɹ��������������߳�
    thread readTask(readTaskHandler, clientfd);
    readTask.detach();

    // main�߳����ڽ����û����룬�շ�����
    while(1)
    {
        // ��ʾ��ҳ��˵� ��¼��ע�ᡢ�˳�
        cout << "==============================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "==============================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        cin.get();                  // ��ȡ�����������Ļس�

        switch (choice)
        {
            case 1:
            {
                // �û�����id������
                int id = 0;
                char pwd[50] = {0};
                cout << "userid: ";
                cin >> id;
                cin.get();              // ��ȡ�����������Ļس�
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

                sem_wait(&rwsem);               // �ȴ��ź����������̴߳������¼����Ӧ��Ϣ��֪ͨ����

                if(g_isLoginSuccess)
                {
                    // ��������������
                    isMainMenuRunning = true;
                    mainMenu(clientfd);
                }
            }
            break;
        case 2:     // registerҵ��
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

                sem_wait(&rwsem);           //�ȴ��ź��������̴߳�����ע����Ϣ��֪ͨ
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

// ����ע�����Ӧ�߼�
void doRegResponse(json &responsejs)
{
    if(0 != responsejs["errno"])            // ע��ʧ��
    {
        cerr << "name already exists, register error" <<endl;
    }
    else                                    // ע��ɹ�
    {
        cout << "name register success, userid is " << responsejs["id"]
                << ", don't forget it" << endl;
    }
}

// �����¼����Ӧ�߼�
void doLoginResponse(json &responsejs)
{
    if(0 != responsejs["errno"])            // ��¼ʧ��
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else                                    // ��¼�ɹ�
    {
        // ��¼��ǰ�û���id��name
        g_currentUser.setId(responsejs["id"]);
        g_currentUser.setName(responsejs["name"]);

        // ��¼��ǰ�û��ĺ�����Ϣ
        if(responsejs.contains("friends"))
        {
            // ��ʼ��
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

        // ��¼��ǰ�û���Ⱥ���б���Ϣ
        if(responsejs.contains("groups"))
        {
            // ��ʼ��
            g_currentUserGroupList.clear();

            // �ַ���=��json=��group=��groupList
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

        // ��ʾ��¼�û��Ļ�����Ϣ
        showCurrentUserData();

        // ��ʾ��ǰ�û���������Ϣ ����������Ϣ��Ⱥ����Ϣ
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

// ���߳� �����߳�
void readTaskHandler(int clientfd)
{
    while(1)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, sizeof(buffer), 0);    // ����
        if(-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // ����ChatServerת�������ݣ������л�����Json����
        json js = json::parse(buffer);
        int msgtype = js["msgid"];

        // һ��һ������Ϣ
        if(ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"] << "[" << js["id"] << "]" << js["name"] 
                    << "said: " << js["msg"] << endl;
            continue;
        }

        // Ⱥ������Ϣ
        if(GROUP_CHAT_MSG == msgtype)
        {
            cout << "group msg[" << js["groupid"] << "]: " << js["time"] << "[" << js["id"] << "]" << js["name"]
                <<"said: " << js["msg"] << endl;
            continue;
        }

        // ��¼Ӧ����Ϣ
        if(LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js);        // �����¼��Ӧ��ҵ���߼�
            sem_post(&rwsem);           // ֪ͨ���̣߳���¼����������
            continue;

        }

        // ע��Ӧ����Ϣ
        if(REGISTER_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem);           // ֪ͨ���̣߳�ע�����������
        }

    }
}

// ��ʾ��¼�û��Ļ�����Ϣ
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

// ϵͳ֧�ֵĿͻ��������б�
unordered_map<string, string> commandMap = 
{
    {"help", "��ʾ����֧�ֵ������ʽhelp"},
    {"chat", "һ��һ���죬��ʽchat:friendid:message"},
    {"addfriend", "��Ӻ��ѣ���ʽaddfriend:friendid"},
    {"creategroup", "����Ⱥ�飬��ʽcreategroup:groupname:groupdesc"},
    {"addgroup", "����Ⱥ�飬��ʽaddgroup:groupid"},
    {"groupchat", "Ⱥ�ģ���ʽgroupchat:groupid:message"},
    {"loginout", "ע������ʽloginout"}
};

// ע��ϵͳ֧�ֵĿͻ����������
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

// ������ҳ�����
void mainMenu(int clientfd)
{
    cout << "mainMenu" << endl;
    help();

    char buffer[1024] = {0};
    while(isMainMenuRunning)
    {
        cin.getline(buffer, sizeof(buffer));
        string commandbuf(buffer);
        string command;                         // �洢����

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
        // ����command��Ӧ�Ļص�����
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

// ��ȡϵͳʱ�䣨������Ϣ��Ҫ��ʾʱ�䣩
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