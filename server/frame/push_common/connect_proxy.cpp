#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include "base.h"
#include "connect_proxy.h"

extern struct event_base * g_event_base;
extern struct SvrConfigure svr_cfg;

void ConnectProxyMgr::init(int server_id, int server_type, ProxyMessageProcessor* processor)
{
    m_processor = processor;
    m_processor->InitMessageIdMap();
    m_server_id = server_id;
    m_server_type = server_type;

    m_processor->SetSvrInfo(m_server_type, m_server_id);
}

void ConnectProxyMgr::AddProxy(ServerAddr const& server_addr)
{
    m_ociv.push_back(new ProxyInfo());

    ProxyInfo& info = *m_ociv.back();
    info.remote_ip = server_addr.server_ip;
    info.remote_port = server_addr.server_port;
    info.server_id = m_server_id;
    info.server_type = m_server_type;
    info.processor = m_processor;
    info.Reconnect();
}

void ConnectProxyMgr::AddProxy(std::string const& ip, unsigned short port)
{
    m_ociv.push_back(new ProxyInfo());

    ProxyInfo& info = *m_ociv.back();
    info.remote_ip = ip;
    info.remote_port = port;
    info.server_id = m_server_id;
    info.server_type = m_server_type;
    info.processor = m_processor;
    info.Reconnect();
}

void ProxyInfo::Reconnect()
{
    time_t now_time = time(NULL);
    //超过2s重连一次
    if( now_time-last_connect_time>2)
       last_connect_time = now_time;
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

//给服务器发送注册消息
int ProxyMessageProcessor::RegisterToServer(const ClientInfo* client_info)
{
	SvrMsgHead mh;
	SvrRegRequest req;

	FillMsgHead(&mh, SMT_REG_REQ, SERVER_TYPE_PROXY);
	req.set_svr_id(client_info->server_id);
	req.set_svr_type(client_info->server_type);
    log_debug("register to server, client_info:%s", client_info->ShortDebugString().c_str());
	return SendMessageToServer(client_info, &mh, &req);
}

void ProxyMessageProcessor::InitMessageIdMap()
{
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_REG_RESP, new RegRespHandler, "SvrRegResponse");
}

int ProxyMessageProcessor::ProcessClose(ClientInfo* client_info)
{
    if(NULL == client_info || NULL == m_proxymgr)
        return -1;

    return m_proxymgr->RemoveProxy(client_info->server_id)?0:-1;
}

int  ConnectProxyMgr::SendMessageToServer(const SvrMsgHead* head, google::protobuf::Message* message)
{
    if(head->has_proxy_svr_id())
    {
        ProxyInfo* info = GetProxy(head->proxy_svr_id());
        if(NULL != info)
    	    return m_processor->SendMessageToServer(info, head, message);
    }

    if(head->is_broadcast())
    {
        for(int i = 0; i < m_ociv.size(); ++i)
        {
            ((SvrMsgHead*)head)->set_proxy_svr_id(m_ociv[i]->server_id);
	        m_processor->SendMessageToServer(m_ociv[i], head, message);
        }

        return 0;
    }

    size_t select_proxy_idx = 0;

    if(m_ociv.size()>0)
    {
        select_proxy_idx = head->dst_svr_id() % m_ociv.size() ;
    }

    ((SvrMsgHead*)head)->set_proxy_svr_id(m_ociv[select_proxy_idx]->server_id);

	return m_processor->SendMessageToServer(m_ociv[select_proxy_idx], head, message);
}

ProxyInfo* ConnectProxyMgr::GetProxy(int proxy_svr_id)
{
    std::vector<ProxyInfo*>::iterator it = m_ociv.begin(); 
    for(; it != m_ociv.end(); ++it)
    {
        if((*it)->server_id == proxy_svr_id)
            return *it;
    }

    return NULL;
}

bool ConnectProxyMgr::RemoveProxy(int proxy_svr_id)
{
    std::vector<ProxyInfo*>::iterator it = m_ociv.begin(); 
    for(; it != m_ociv.end(); ++it)
    {
        if((*it)->server_id == proxy_svr_id)
        {
            delete *it;
            m_ociv.erase(it);
            return true;
        }
    }

    return false;
}

int RegRespHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    const SvrRegResponse* res = dynamic_cast<const SvrRegResponse*> (message);

	if(res->result()!=RESULT_SUCCESS)
	{
		log_error("register server error!");
		return -1;
	}

	client_info->is_register = true;

    const SvrMsgHead* head = dynamic_cast<const SvrMsgHead*>(phead);
    client_info->server_id = head->src_svr_id();
	log_debug("register sucess, %d,client_info:%s", head->type(),  client_info->ShortDebugString().c_str());

    return 0;
}
