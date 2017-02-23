#pragma once

#include "redismanager.h"
#include "dbworker_manager.h"
#include "message_processor.h"
#include "connect_proxy.h"

class WorkerMessageProcessor : public ProxyMessageProcessor 
{
public:
    WorkerMessageProcessor();
    virtual ~WorkerMessageProcessor();

    virtual void InitMessageIdMap();

    int SendTransferMsg(std::string const& client_id, uint32_t msgid, int32_t connector_id, std::string const& msg);
};

#define ProxyMgr Singleton<ConnectProxyMgr>::Instance()
#define MessageProcessorInst Singleton<WorkerMessageProcessor>::Instance()

class UserUpdateStatusHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};

class SvrInsertMsgHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};

class SvrUserMsgAckHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};
