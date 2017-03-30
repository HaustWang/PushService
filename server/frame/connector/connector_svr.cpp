#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <endian.h>
#include <errno.h>
#include <string>
#include <vector>
#include <map>

#include "event2/listener.h"
#include "log4cpp_log.h"
#include "comm_datetime.h"
#include "connector_message_processor.h"
#include "onliner_message_processor.h"
#include "base.h"
#include "comm_server.h"
#include "connect_center.h"
#include "push_common.h"


struct event_base *g_event_base = NULL;
extern volatile int stop;


#define LOG_NAME "connector"

void ServerWriteCallback(struct bufferevent* bev, void* ctx)
{
    PushClientInfo *client_info = (PushClientInfo *)ctx;
    if (client_info->fd != bufferevent_getfd(bev))
    {
        log_error("client info fd:%d is not equal bufferevent fd:%d",
                        client_info->fd, bufferevent_getfd(bev));
        return;
    }

    if(client_info->need_delete == true)
    {
        log_info("client_info [fd:%d,client_id:%s] will be delete!!!", client_info->fd,client_info->client_id.c_str());
        delete client_info;
        client_info = NULL;
    }

    return ;
}


void ServerReadCallback(struct bufferevent *bev, void *ctx)
{
    PushClientInfo *client_info = (PushClientInfo *)ctx;
    log_debug("client_info:%d, %p", client_info->fd, client_info->bevt);
    if (client_info->fd != bufferevent_getfd(bev))
    {
        log_error("client info fd:%d is not equal bufferevent fd:%d",
                        client_info->fd, bufferevent_getfd(bev));
        return;
    }

    while(1)
    {
        static char buf[1024 * 1024];
        char *pkg_start = buf;
        int pkg_len = sizeof(buf);
        //小于0,表示流的数据已经错乱了，只能断开连接
        int ret = ConnectorMessageProcessor::Instance()->GetCompletePackage(bev, pkg_start, &pkg_len);
        if(ret < 0)
        {
            log_warning("GetCompletePackage err  ret %d", ret);
            log_warning("server closed socket, because client:%s packet is wrong\n", GetPeerAddrStr(client_info->address));
            ConnectorMessageProcessor::Instance()->ProcessClose(client_info);
            break;
        }
        //大于0,表示一个包还不完整, 继续收包
        else if(ret > 0)
        {
            return;
        }

        static std::string out;
        out.clear();

        if(-1 == client_info->aes.Decrypt(std::string(pkg_start+sizeof(int), pkg_len-sizeof(int)), out))
        {
            log_error("Decrypt data error. client_id:%s, data len:%d, key:%s", client_info->client_id.c_str(), pkg_len, client_info->aes.GetCryptKey().c_str());
            return;
        }

        static PushMessage  push_msg;
        if (!push_msg.ParseFromString(out))
        {
            log_warning("client:%s parse message failed, pkg len:%ld, aes key:%s", GetPeerAddrStr(client_info->address), out.length(), client_info->aes.GetCryptKey().c_str());
            ConnectorMessageProcessor::Instance()->ProcessClose(client_info);
            break;
        }
        //required废弃了， 逻辑层自己检查
        if (!push_msg.has_message_head() ||
                !push_msg.message_head().has_type())
        {
            log_warning("client:%s parse message failed, pkg len:%ld", GetPeerAddrStr(client_info->address), out.length());
            ConnectorMessageProcessor::Instance()->ProcessClose(client_info);
            break;
        }
        ConnectorMessageProcessor::Instance()->ProcessMessage(client_info, &push_msg.message_head(), push_msg.message_body());
    }
}

void SvrEventCallback(struct bufferevent *bev, short what, void *ctx)
{
    UNUSED(bev);
    PushClientInfo *client_info = (PushClientInfo *)ctx;
    if(what & BEV_EVENT_CONNECTED)
    {
        return;
    }

    log_warning("client:%s closed fd:%d  client_id:%s\n",
                GetPeerAddrStr(client_info->address), client_info->fd, client_info->client_id.c_str());

    if(PushClientManage::Instance()->GetPushClientInfo(client_info->fd) == NULL)
    {
        log_warning("can not find client_info:%d", client_info->fd);
        return;
    }

    ConnectorMessageProcessor::Instance()->ProcessClose(client_info);
    return;
}


void ServerListenerCallback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx)
{
    UNUSED(socklen);
    UNUSED(ctx);

    char addr_str[128]={0};
    switch(address->sa_family)
    {
        case AF_INET:
        {
            struct sockaddr_in *psa = (struct sockaddr_in *)address;
            log_info("new connect from %s:%d, fd is %d\n",
                            inet_ntop(address->sa_family, &psa->sin_addr, addr_str, sizeof(addr_str)),
                            ntohs(psa->sin_port), fd);
            break;
        }
        case AF_INET6:
        {
            struct sockaddr_in6 *psa = (struct sockaddr_in6 *)address;
            log_info("new connect from %s, fd is %d\n",
                            inet_ntop(address->sa_family, &psa->sin6_addr, addr_str, sizeof(addr_str)), fd);
            break;
        }
        default:
        {
            log_error("invalid family type:%d", (int)address->sa_family);
            return;
        }
    }

    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if(!bev)
    {
        log_error("fd %d, bufferevent_socket_new err\n", fd);
        close(fd);
        return;
    }

    PushClientInfo* client_info = PushClientManage::Instance()->CreatePushClientInfo(fd, bev, address);
    if (client_info == NULL)
    {
        log_error("fd %d, create client info err\n", fd);
        close(fd);
        return;
    }

    log_debug("client_info:%d, %p", client_info->fd, client_info->bevt);
    bufferevent_setcb(bev, ServerReadCallback , ServerWriteCallback, SvrEventCallback, client_info);

    if(0 != bufferevent_enable(bev, EV_READ|EV_WRITE))
    {
        log_error("fd %d, bufferevent_enable err\n", fd);
        PushClientManage::Instance()->DeletePushClientInfo(client_info);
        return;
    }

    int tcp_nodelay = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcp_nodelay, sizeof(tcp_nodelay)) < 0)
    {
        log_error("fd %d, set IPPROTO_TCP:TCP_NODELAY err\n", fd);
        PushClientManage::Instance()->DeletePushClientInfo(client_info);
        return;
    }
}

void SvrListenerErrCallback(struct evconnlistener *listener, void *ctx)
{
    UNUSED(ctx);

    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. ""Shutting down.\n", err, evutil_socket_error_to_string(err));

    event_base_loopexit(base, NULL);

}

int init_net(Config& config)
{
    struct sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(config.listen_port);
    listen_addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    g_event_base = event_base_new();
    if(!g_event_base)
    {
        log_warning("event_base_new error. \n");
        return -1;
    }

    //监听
    struct evconnlistener *listener = evconnlistener_new_bind(g_event_base, ServerListenerCallback , NULL,
                    LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, 128,
                    (struct sockaddr *)(&listen_addr), sizeof(listen_addr));
    if(NULL == listener)
    {
        log_warning("evconnlistener_new_bind err:%s \n", strerror(errno));
        exit(1);
        return -10;
    }
    evconnlistener_set_error_cb(listener, SvrListenerErrCallback);

    return 0;
}

int InitConnectorService(Config& config)
{
    event_set_log_callback(CommonEventLog);
    int ret = init_net(config);
    if (ret < 0)
    {
        printf("[%s:%d] InitNet failed, ret:%d\n", __FILE__, __LINE__, ret);
        log_warning("InitNet failed, ret:%d\n", ret);
        return ret;
    }

    InitSignal();

    ConnectorMessageProcessor::Instance()->InitMessageIdMap();
    return 0;
}

bool ReloadConfig(int svr_id)
{
    if(ConnectToCenter::Instance()->IsNewConfig())
    {
        const SvrConfig& config = ConnectToCenter::Instance()->GetConfig();
        set_log_type(config.log_type());

        char log_name[64] = {0};
        snprintf(log_name, sizeof(log_name), LOG_NAME "_%d", svr_id);

        init_log(log_name, config.log_dir().c_str(),config.log_config().c_str());
        set_log_level(config.log_level());
        ConnectToCenter::Instance()->SetNewConfig(false);
        return true;
    }

    return false;
}

void NewConnect()
{
    if(ConnectToCenter::Instance()->HasNewAddress(SERVER_TYPE_PROXY))
    {
        const std::vector<SvrAddress>& addrs = ConnectToCenter::Instance()->GetNewAddress(SERVER_TYPE_PROXY);
        for(std::vector<SvrAddress>::const_iterator it = addrs.begin(); it != addrs.end(); ++it)
            ConnectToOnlinerMgr::Instance()->AddProxy(*it);

        ConnectToCenter::Instance()->EraseNewAddress(SERVER_TYPE_PROXY);
    }
}

int main(int argc, char** argv)
{
    Config config;
    config.svr_type = SERVER_TYPE_CONNECTOR;
    config.need_listen = true;
    ParseArg(argc, argv, config);

    if(!IsAddressListening(config.center_ip.c_str(), config.center_port))
    {
        printf("[%s:%d] center server not startted! address:%s:%hd\n", __FILE__, __LINE__, config.center_ip.c_str(), config.center_port);
        return -1;
    }

    InitLibevent(g_event_base);
    ConnectToCenter::Instance()->Init(config.center_ip, config.center_port, config.svr_type, config.svr_id, GetWlanIp(), config.listen_port);
    while(true)
    {
        event_base_loop(g_event_base, EVLOOP_ONCE);
        if(ReloadConfig(config.svr_id))
            break;
    }

    //开启服务
    int ret = InitConnectorService(config);

    if(ret!=0)
     {
        log_warning("InitSvr failed ret:%d\n", ret);
    }
    //作为客户端,连接onliner
    ConnectToOnlinerMgr::Instance()->init(config);

    log_warning("connector start running \n" );

    struct timeval tm;
    tm.tv_sec = 1;
    tm.tv_usec = 0;

    while (1 != stop)
    {
        event_base_loopexit(g_event_base, &tm);
        event_base_loop(g_event_base, EVLOOP_NONBLOCK);
        //尝试重连
        ConnectToCenter::Instance()->Reconnect();
        ConnectToOnlinerMgr::Instance()->Reconnect();
        //删除超时的客户端
        CPlayerFrame::Instance()->CloseOutoftimeClient();
        //静默加载配置
        ReloadConfig(config.svr_id);

        NewConnect();
        usleep(1000);
    }

    event_base_free(g_event_base);

    return 0;
}
