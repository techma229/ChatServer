#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <iostream>

using namespace std;

// 服务器Ctl+C 退出的处理器
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char *argv[])
{
    // 注册SIGINT的处理函数
    signal(SIGINT, resetHandler);

    EventLoop loop;

    if(argc < 3)
    {
        cout << "input error!" << endl;
        exit(-1);
    }

    InetAddress addr(argv[1], atoi(argv[2]));
    ChatServer server(&loop, addr, "ChatServer");

    server.start();
    loop.loop();

    return 0;
}