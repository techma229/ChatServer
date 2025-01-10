#include "chatserver.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <functional>
#include "json.hpp"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

// 初始化聊天服务器对象
ChatServer::ChatServer(EventLoop *loop,
    const InetAddress &listenAddr,
    const std::string &nameArg)
    : _server(loop, listenAddr, nameArg),
    _loop(loop)
{
    // 注册连接事件的回调函数
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息事件的回调函数
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 开启事件循环
void ChatServer::start()
{
    _server.start();
}

// 连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if(!conn->connected())
    {
        // 处理客户端异常退出事件
        ChatService::instance()->clientCloseExceptionHandler(conn);
        conn->shutdown();
    }
}

// 读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                Buffer *buffer,
                Timestamp time)
{
    // 将json数据转化为string
    string buf = buffer->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(buf);

    // 完全解耦网络模块和业务模块，不要在网络模块中调用业务模块的方法
    // 通过js["msgid"]来获取不同的业务处理器（事先绑定的回调方法）
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHandler(conn, js, time);
}
