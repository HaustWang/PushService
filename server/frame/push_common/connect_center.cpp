#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include "base.h"
#include "connect_center.h"

extern struct event_base * g_event_base;

void ConnectToCenter::Init(std::string const& ip, unsigned short port, int server_type, int server_id, std::string const& local_listen_ip, int local_listen_port)
{
    this->processor = new ConnectCenterMsgProcessor();
    this->processor->InitMessageIdMap();

    remote_ip = ip;
    remote_port = port;
    m_listen_ip = local_listen_ip;
    m_listen_port = local_listen_port;

    this->server_type = SERVER_TYPE_CENTER;
    this->server_id = 0;

    this->processor->SetSvrInfo(server_type, server_id);

    Reconnect();
}

void ConnectToCenter::SetMessageProcessor(ConnectCenterMsgProcessor* processor)
{
    if(NULL == processor)
        return ;

    if(NULL != this->processor)
        delete this->processor;

    this->processor = processor;
    this->processor->InitMessageIdMap();
}

void ConnectToCenter::AddServerAddress(google::protobuf::RepeatedPtrField<SvrAddress> const& field)
{
    for(google::protobuf::RepeatedPtrField<SvrAddress>::const_iterator it = field.begin(); it != field.end(); ++it)
    {
        m_svr_addr[it->svr_type()].push_back(*it);
    }
}

void ConnectToCenter::Reconnect()
{
    time_t now_time = time(NULL);
    //超过2s重连一次
    if( now_time-m_last_connect_time>5)
        m_last_connect_time = now_time;
    else
        return ;

    if(bevt && fd>0)
        return;

	if(bevt)
	{
		bufferevent_free(bevt);
		bevt = NULL;
	}

    if(fd>0)
    {
        close(fd);
        fd = -1;
    }

    ClientConnect(remote_ip.c_str(), remote_port, g_event_base, bevt, fd, this);
}

int ConnectToCenter::SendConfigReq()
{
    SvrMsgHead mh;
	SvrConfigReq req;

	processor->FillMsgHead(&mh, SMT_CONFIG_REQ, SERVER_TYPE_CENTER);

    if(0 != m_listen_port)
    {
        SvrAddress* address = req.mutable_address();
        address->set_svr_type(processor->GetServerType());
        address->set_svr_id(processor->GetServerId());
        address->set_ip(m_listen_ip);
        address->set_port(m_listen_port);
    }

    printf("[%s:%d] register to center:head:%s, msg:%s\n", __FILE__, __LINE__, mh.ShortDebugString().c_str(), req.ShortDebugString().c_str());
    return SendMessageToServer(&mh, &req);
}

int  ConnectToCenter::SendMessageToServer(const SvrMsgHead* head, google::protobuf::Message* message)
{
	return processor->SendMessageToServer(this, head, message);
}

//给服务器发送注册消息
int ConnectCenterMsgProcessor::RegisterToServer(const ClientInfo* client_info)
{
    return ((ConnectToCenter*)client_info)->SendConfigReq();
}

void ConnectCenterMsgProcessor::InitMessageIdMap()
{
	REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_CONFIG_RESP, new ConfigResponseHandler, "SvrConfigResp" );
	REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_BROADCAST_ADDR, new BroadcastAddrHandler, "SvrBroadcastAddress" );
	REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_BROADCAST_CONFIG, new BroadcastConfigHandler, "SvrConfig" );
}

int ConfigResponseHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message*, const google::protobuf::Message* message)
{
    if(NULL == message)
        return -1;

    const SvrConfigResp* resp = dynamic_cast<const SvrConfigResp*>(message);
    ConnectToCenter::Instance()->Config().CopyFrom(resp->config());
    ConnectToCenter::Instance()->SetNewConfig(true);

    client_info->is_register = true;
    if(0 == resp->peer_addresses_size())
        return 0;

    ConnectToCenter::Instance()->AddServerAddress(resp->peer_addresses());
    return 0;
}

int BroadcastAddrHandler::ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message* message)
{
    const SvrBroadcastAddress* resp = dynamic_cast<const SvrBroadcastAddress*>(message);
    if(0 == resp->peer_addresses_size())
        return 0;

    ConnectToCenter::Instance()->AddServerAddress(resp->peer_addresses());
    return 0;
}

int BroadcastConfigHandler::ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message* message)
{
    ConnectToCenter::Instance()->Config().CopyFrom(*message);
    ConnectToCenter::Instance()->SetNewConfig(true);

    return 0;
}
