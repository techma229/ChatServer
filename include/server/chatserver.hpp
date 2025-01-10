#ifndef CHARSERVER_H
#define CHARSERVER_H

#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpServer.h>
using namespace muduo;
using namespace muduo::net;

// 服务器类，基于muduo库开发
class ChatServer
{
public:
    // 初始化聊天服务器对象
    ChatServer(EventLoop *loop,
		const InetAddress &listenAddr,
		const std::string &nameArg);

    // 开启事件循环
    void start();

private:
    // 连接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &conn);

    // 读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &conn,
                   Buffer *buffer,
                   Timestamp);

    TcpServer _server;
    EventLoop* _loop;
};

#endif