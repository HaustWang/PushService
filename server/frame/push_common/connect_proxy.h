#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "log4cpp_log.h"
#include "push_proto_common.h"
#include "push_proto_server.h"
#include "push_common.h"
#include "base.h"
#include "message_processor.h"
#include "comm_client.h"

class ProxyInfo : public ClientInfo
{
public:
	ProxyInfo():ClientInfo()
	{
        bevt = NULL;
        processor = NULL;
        last_connect_time = 0;
	}

    ~ProxyInfo()
	{
	}

    void ShortDebugString() const
    {
        log_debug("Show ProxyInfo,fd:%d, server_ip:%s, server_port:%d,server_id:%d, server_type:%d,is_register:%s", fd, remote_ip.c_str(), remote_port, server_id, server_type, is_register==true?"TRUE":"FALSE");
    }

    void Reconnect();//连接服务器

public:
    time_t last_connect_time; //最近一次连接服务器时间
};

class ConnectProxyMgr;
class ProxyMessageProcessor : public MessageProcessor
{
public:
    ProxyMessageProcessor()
        : MessageProcessor()
    {
    }

    virtual ~ProxyMessageProcessor()
    {
    }

    virtual void InitMessageIdMap();
	virtual int RegisterToServer(const ClientInfo* );
    virtual int ProcessClose(ClientInfo* client_info);

    void SetProxyMgr(ConnectProxyMgr* mgr) { m_proxymgr = mgr; }
private:
    ConnectProxyMgr* m_proxymgr;
};

class ConnectProxyMgr
{
public:
    ConnectProxyMgr()
    {
        m_processor = NULL;
    }

    void init(int server_id, int server_type, ProxyMessageProcessor* processor);
	int SendMessageToServer(const SvrMsgHead*, google::protobuf::Message*);
	void Reconnect()
    {
        for(size_t i=0; i<m_ociv.size(); i++)
        {
           m_ociv[i]->Reconnect();
        }
    }

    void AddProxy(SvrAddress const& svr_addr);

    int ServerId() { return m_server_id; }
    int ServerType() { return m_server_type; }

    ProxyInfo* GetProxy(int proxy_svr_id);
    bool RemoveProxy(int proxy_svr_id);
private:
	std::vector<ProxyInfo*> m_ociv;
    ProxyMessageProcessor* m_processor;

    int m_server_id;
    int m_server_type;
};

class RegRespHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};

