#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <map>
#include <vector>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include "log4cpp_log.h"
#include "push_proto_common.h"
#include "push_common.h"
#include "base.h"
#include "message_processor.h"
#include "comm_client.h"


class ConnectCenterMsgProcessor;

class ConnectToCenter : public ClientInfo
{
public:
    static ConnectToCenter* Instance()
    {
        static ConnectToCenter* _instance = NULL;
        if(NULL == _instance)
        {
            _instance = new ConnectToCenter();
        }

        return _instance;
    }

    ~ConnectToCenter()
	{
        if(processor)
            delete processor;
	}

    void Init(std::string const& ip, unsigned short port, int server_type, int server_id, std::string const& local_listen_ip = "", int local_listen_port = 0);

    void ShortDebugString() const
    {
        log_debug("Show ConnectToCenter,fd:%d, server_ip:%s, server_port:%d,server_id:%d, server_type:%d,is_register:%s", fd, remote_ip.c_str(), remote_port, server_id, server_type, is_register==true?"TRUE":"FALSE");
    }

    void Reconnect();//连接服务器
	int SendMessageToServer(const SvrMsgHead*, google::protobuf::Message*);
    int SendConfigReq();

    ConnectCenterMsgProcessor* GetProcessor() { return (ConnectCenterMsgProcessor*)processor; } 

    const SvrConfig& GetConfig() const { return m_svr_config; }
    SvrConfig& Config() { return m_svr_config; }

    bool HasNewAddress(int svr_type) { return !m_svr_addr[svr_type].empty(); }
    const std::vector<SvrAddress>& GetNewAddress(int svr_type) { return m_svr_addr[svr_type]; }
    void EraseNewAddress(int svr_type) { m_svr_addr.erase(svr_type); }
    void SetNewConfig(bool v) { m_new_config = v; }
    bool IsNewConfig() { return m_new_config; }

    void AddServerAddress(google::protobuf::RepeatedPtrField<SvrAddress> const& field);

private:
    ConnectToCenter():ClientInfo()
	{
        bevt = NULL;
        m_last_connect_time = 0;
        m_new_config = false;
	}

private:
    time_t m_last_connect_time; //最近一次连接服务器时间
    SvrConfig m_svr_config;
    bool m_new_config;
    std::string m_listen_ip;
    unsigned short m_listen_port;

    std::map<int, std::vector<SvrAddress> > m_svr_addr;
};

class ConnectCenterMsgProcessor : public MessageProcessor
{
public:
    ConnectCenterMsgProcessor()
        : MessageProcessor()
    {
    }

    virtual ~ConnectCenterMsgProcessor()
    {
    }

    virtual void InitMessageIdMap();
	virtual int RegisterToServer(const ClientInfo* );

    int ProcessConfigResponse(ConnectToCenter*, const SvrMsgHead*, const google::protobuf::Message*);
    int ProcessBroadcastAddr(ConnectToCenter*, const SvrMsgHead*, const google::protobuf::Message*);
    int ProcessBroadcastConfig(ConnectToCenter*, const SvrMsgHead*, const google::protobuf::Message*);
    int ProcessClose(ClientInfo*) {return 0;}
};

class ConfigResponseHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};

class BroadcastAddrHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};

class BroadcastConfigHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};
