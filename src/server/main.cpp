#include "chatserver.hpp"
#include "chatservice.hpp"
#include <signal.h>
#include <iostream>

using namespace std;

// ������Ctl+C �˳��Ĵ�����
void resetHandler(int)
{
    ChatService::instance()->reset();
    exit(0);
}

int main(int argc, char *argv[])
{
    // ע��SIGINT�Ĵ�����
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