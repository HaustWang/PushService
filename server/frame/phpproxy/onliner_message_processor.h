#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "push_proto_common.h"
#include "push_proto_client.h"
#include "push_proto_server.h"
#include "log4cpp_log.h"
#include "connect_proxy.h"
#include "redismanager.h"
#include "json.h"

typedef Json::ValueIterator Iter;

class OnlinerMessageProcessor : public ProxyMessageProcessor 
{
public:
	OnlinerMessageProcessor();

	~OnlinerMessageProcessor()
	{
	}

 	static OnlinerMessageProcessor* Instance()
    {
        static OnlinerMessageProcessor *_instance = NULL;
        if(!_instance)
        {
            _instance = new OnlinerMessageProcessor;
        }
        return _instance;
	}

	virtual void InitMessageIdMap();
};


class ConnectToOnlinerMgr : public ConnectProxyMgr 
{
public:
	void init(Config const& config);

	static ConnectToOnlinerMgr* Instance()
    {
        static ConnectToOnlinerMgr *_instance = NULL;
        if(!_instance)
        {
            _instance = new ConnectToOnlinerMgr;
        }
        return _instance;
	}

    int SendPushMsgToDBWorker(int32_t msg_id, std::string const& client_id, int32_t expire_time, unsigned char *data, size_t length);

    int SendPushMsgToDBWorker(int32_t msg_id, Iter begin, Iter end, int32_t expire_time, unsigned char *data, size_t length);

};

class UserMsgAckHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};

class UserReadHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};

