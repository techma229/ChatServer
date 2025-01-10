#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>

using namespace std;
using redis_handler = function<void(int,string)>;

class Redis
{
public:
    Redis();
    ~Redis();

    //����Redis������
    bool connect();

    //��redisָ����ͨ��channel������Ϣ
    bool publish(int channel, string message);

    //��redisָ����ͨ��subscribe������Ϣ
    bool subscribe(int channel);

    //ȡ������
    bool unsubscribe(int channel);

    //�����߳��н��ն���ͨ������Ϣ
    void observer_channel_message();

    //��ʼ��ҵ����ϱ�ͨ����Ϣ�Ļص�����
    void init_notify_handler(redis_handler handler);

private:
    //hiredisͬ�������Ķ��󣬸���publish��Ϣ
    redisContext *publish_context_;

    //����subscribe��Ϣ
    redisContext *subcribe_context_;

    //�ص��������յ���Ϣ���service�ϱ�
    redis_handler notify_message_handler_;
};

#endif