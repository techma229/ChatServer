#include "redis.hpp"
#include <iostream>

Redis::Redis() : publish_context_(nullptr), subcribe_context_(nullptr)
{
}

Redis::~Redis()
{
    if (publish_context_ != nullptr)
    {
        redisFree(publish_context_);
    }

    if (subcribe_context_ != nullptr)
    {
        redisFree(subcribe_context_);
    }
}

//����Redis������
bool Redis::connect()
{
    publish_context_ = redisConnect("127.0.0.1", 6379);
    if (publish_context_ == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    subcribe_context_ = redisConnect("127.0.0.1", 6379);
    if (subcribe_context_ == nullptr)
    {
        cerr << "connect redis failed!" << endl;
        return false;
    }

    // �����߳��н��ն���ͨ������Ϣ
    thread t([&]() {
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

//��redisָ����ͨ��channel������Ϣ
bool Redis::publish(int channel, string message)
{
    // �൱��publish �� ֵ
    // redis 127.0.0.1:6379> PUBLISH runoobChat "Redis PUBLISH test"
    redisReply *reply = (redisReply *)redisCommand(publish_context_, "PUBLISH %d %s", channel, message.c_str());
    if (reply == nullptr)
    {
        cerr << "publish command failed!" << endl;
        return false;
    }

    // �ͷ���Դ
    freeReplyObject(reply);
    return true;
}

// ��redisָ����ͨ��subscribe������Ϣ
bool Redis::subscribe(int channel)
{
    // redisCommand ���Ȱ�����浽context�У�Ȼ�����RedisAppendCommand���͸�redis
    // redisִ��subscribe��������������Ӧ�����������һ��reply
    // redis 127.0.0.1:6379> SUBSCRIBE runoobChat
    if (REDIS_ERR == redisAppendCommand(subcribe_context_, "SUBSCRIBE %d", channel))
    {
        cerr << "subscibe command failed" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subcribe_context_, &done))
        {
            cerr << "subscribe command failed" << endl;
            return false;
        }
    }

    return true;
}

//ȡ������
bool Redis::unsubscribe(int channel)
{
    //redisCommand ���Ȱ�����浽context�У�Ȼ�����RedisAppendCommand���͸�redis
    //redisִ��subscribe��������������Ӧ�����������һ��eply
    if (REDIS_ERR == redisAppendCommand(subcribe_context_, "UNSUBSCRIBE %d", channel))
    {
        cerr << "subscibe command failed" << endl;
        return false;
    }

    int done = 0;
    while (!done)
    {
        if (REDIS_ERR == redisBufferWrite(subcribe_context_, &done))
        {
            cerr << "subscribe command failed" << endl;
            return false;
        }
    }

    return true;
}

//�����߳��н��ն���ͨ������Ϣ
void Redis::observer_channel_message()
{
    redisReply *reply = nullptr;
    while (REDIS_OK == redisGetReply(subcribe_context_, (void **)&reply))
    {
        //reply�����Ƿ��ص�����������0. message , 1.ͨ��2.�ź�
        if (reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            //��ҵ����ϱ���Ϣ
            notify_message_handler_(atoi(reply->element[1]->str), reply->element[2]->str);
        }

        freeReplyObject(reply);
    }

    cerr << "----------------------- oberver_channel_message quit--------------------------" << endl;
}

//��ʼ��ҵ����ϱ�ͨ����Ϣ�Ļص�����
void Redis::init_notify_handler(redis_handler handler)
{
    notify_message_handler_ = handler;
}
