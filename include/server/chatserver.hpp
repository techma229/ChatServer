#ifndef CHARSERVER_H
#define CHARSERVER_H

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
using namespace muduo;
using namespace muduo::net;

// �������࣬����muduo�⿪��
class ChatServer
{
public:
    // ��ʼ���������������
    ChatServer(EventLoop *loop,
		const InetAddress &listenAddr,
		const std::string &nameArg);

    // �����¼�ѭ��
    void start();

private:
    // ���������Ϣ�Ļص�����
    void onConnection(const TcpConnectionPtr &conn);

    // ��д�¼������Ϣ�Ļص�����
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buffer,
                   Timestamp);

    TcpServer _server;
    EventLoop* _loop;
};

#endif