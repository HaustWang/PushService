#pragma once

#include <map>
#include <string>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "push_proto_common.h"
#include "push_proto_client.h"
#include "push_proto_server.h"

#include "comm_client.h"

#define HANDLER_TYPE std::map<int, std::pair<MessageHandler*, std::string> >
#define HANDLER_TYPE_IT HANDLER_TYPE::iterator

#define REGIST_MESSAGE_PROCESS(message_map, message_id, handler, message_name) \
    message_map[message_id] = std::pair<MessageHandler*, std::string>(handler, message_name);

class MessageHandler
{
public:
    virtual ~MessageHandler(){};
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*) = 0;
};

class MessageProcessor
{
public:
    MessageProcessor();
    virtual ~MessageProcessor();

    virtual void SetSvrInfo(int svr_type, int svr_id);
    virtual void InitMessageIdMap() = 0;

    virtual int GetCompletePackage(struct bufferevent* bev, char* pkg, int* len);
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const std::string& message_body);
    virtual int ProcessClose(ClientInfo*) = 0;
    virtual int SendMessageToServer(const ClientInfo*,const SvrMsgHead*, const google::protobuf::Message*);
    virtual int SendMessageToServer(const ClientInfo*,const SvrMsgHead*, const std::string& message);
    virtual int RegisterToServer(const ClientInfo*){return 0;}

    virtual google::protobuf::Message* CreateMessage(const std::string& type_name, const std::string& message_body);
    virtual void DestroyMessage(google::protobuf::Message* pmsg);

    int FillMsgHead(SvrMsgHead* head, const SvrMsgType type, const int dst_svr_type, const SvrMsgHead* src_head = NULL);
    int FillMsgHead(ClientMsgHead *head, const std::string& deviceId, const ClientMsgType type);

    int GetServerType() { return m_SvrType; }
    int GetServerId() { return m_SvrId; }

    HANDLER_TYPE msg_handler_map_; 
private:
    int32_t m_SvrType;
    int32_t m_SvrId;
};
