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
#include "client.h"
#include "json.h"
#include "player.h"
#include "message_processor.h"



class ConnectorMessageProcessor;

class ConnectorMessageProcessor : public MessageProcessor
{
public:
    static ConnectorMessageProcessor* Instance()
    {
        static ConnectorMessageProcessor *_instance = NULL;
        if(!_instance)
        {
            _instance = new ConnectorMessageProcessor;
        }
        return _instance;
    }
    virtual ~ConnectorMessageProcessor();
	 ConnectorMessageProcessor();

public:
	void InitMessageIdMap();
public:
	virtual int ProcessClose(ClientInfo* );
    int SendMessageToClient(std::string const& client_id, std::string const& appid, std::string const& content);
    int SendMessageToAllClient(std::string const& appid, std::string const& content);
	int SendMessageToClient(PushClientInfo* client_info,   const ClientMsgHead*, const google::protobuf::Message*);
	int ProcessLoginReq( PushClientInfo* client_info,   const ClientMsgHead*, const google::protobuf::Message*);
	int ProcessHeartbeat( PushClientInfo* client_info,   const ClientMsgHead*, const google::protobuf::Message*);
	int ResponseClientLogin(PushClientInfo* client_info, const ResultCode process_result, std::string& last_key, bool bNew = false);
};

class LoginHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo* , const google::protobuf::Message*, const google::protobuf::Message*);
};

class PushAckHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo* , const google::protobuf::Message*, const google::protobuf::Message*);
};

class UserReadHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo* , const google::protobuf::Message*, const google::protobuf::Message*);
};

class HeartbeatHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo* , const google::protobuf::Message*, const google::protobuf::Message*);
};




