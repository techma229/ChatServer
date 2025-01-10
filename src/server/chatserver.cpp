#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <functional>
#include "json.hpp"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// ��ʼ���������������
ChatServer::ChatServer(EventLoop *loop,
    const InetAddress &listenAddr,
    const std::string &nameArg)
    : _server(loop, listenAddr, nameArg),
    _loop(loop)
{
    // ע�������¼��Ļص�����
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // ע����Ϣ�¼��Ļص�����
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // �����߳�����
    _server.setThreadNum(4);
}

// �����¼�ѭ��
void ChatServer::start()
{
    _server.start();
}

// ���������Ϣ�Ļص�����
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // �ͻ��˶Ͽ�����
    if(!conn->connected())
    {
        // ����ͻ����쳣�˳��¼�
        ChatService::instance()->clientCloseExceptionHandler(conn);
        conn->shutdown();
    }
}

// ��д�¼������Ϣ�Ļص�����
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                Buffer *buffer,
                Timestamp time)
{
    // ��json����ת��Ϊstring
    string buf = buffer->retrieveAllAsString();
    // ���ݵķ����л�
    json js = json::parse(buf);

    // ��ȫ��������ģ���ҵ��ģ�飬��Ҫ������ģ���е���ҵ��ģ��ķ���
    // ͨ��js["msgid"]����ȡ��ͬ��ҵ�����������Ȱ󶨵Ļص�������
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // �ص���Ϣ�󶨺õ��¼�����������ִ����Ӧ��ҵ����
    msgHandler(conn, js, time);
}
