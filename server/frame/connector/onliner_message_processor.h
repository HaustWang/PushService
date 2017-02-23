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
#include "push_proto_server.h"
#include "client.h"
#include "connect_proxy.h"
#include "base.h"
#include "player.h"

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
    void init(Config const&config);
    static ConnectToOnlinerMgr* Instance()
    {
        static ConnectToOnlinerMgr *_instance = NULL;
        if(!_instance)
        {
            _instance = new ConnectToOnlinerMgr;
        }
        return _instance;
	}

    void UpdateUserStatus(std::string const& client_id, bool is_online); 
    void UpdateUserPushAck(std::string const& client_id, int64_t msg_id);
    void UpdateUserRead(std::string const& client_id, int64_t msg_id);
};

class TransMsgHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo* , const google::protobuf::Message*, const google::protobuf::Message*);
};

class KickUserHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo* , const google::protobuf::Message*, const google::protobuf::Message*);
};



