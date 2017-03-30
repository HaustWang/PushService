#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string>
#include <vector>
#include <map>

#include <event2/listener.h>

#include "base.h"
#include "comm_datetime.h"
#include "comm_server.h"
#include "log4cpp_log.h"

#include "push_proto_common.h"
#include "common_singleton.h"
#include "worker_message_processor.h"
#include "push_common.h"
#include "connect_center.h"

extern std::map<std::string, std::string> g_reason_to_query;
extern volatile int stop; 

struct event_base *g_event_base = NULL;
std::map<int, ClientInfo *> g_map_worker_info;

#define LOG_NAME "dbworker"

int InitNet()
{
    InitLibevent(g_event_base);
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
            ProxyMgr.AddProxy(*it);

        ConnectToCenter::Instance()->EraseNewAddress(SERVER_TYPE_PROXY);
    }
}


int main(int argc, char **argv)
{
    Config config;
    config.svr_type = SERVER_TYPE_DB_WORKER;
    config.need_listen = false;

    ParseArg(argc, argv, config);
    if(!IsAddressListening(config.center_ip.c_str(), config.center_port))
    {
        printf("[%s:%d] center server not startted! address:%s:%hd\n", __FILE__, __LINE__, config.center_ip.c_str(), config.center_port);
        return -1;
    }

    InitSignal();

    int ret = InitNet();
    if (ret != 0)
    {
        printf("[%s:%d] InitNet failed, ret:%d\n", __FILE__, __LINE__, ret);
        return ret;
    }

    ConnectToCenter::Instance()->Init(config.center_ip, config.center_port, config.svr_type, config.svr_id);

    while(true)
    {
        event_base_loop(g_event_base, EVLOOP_ONCE);
        if(ReloadConfig(config.svr_id))
            break;
    }

    event_set_log_callback(CommonEventLog);
    ret = DbWorkerMgrInst.Init();
    if (ret != 0)
    {
        log_warning("DbWorkerMgr Init failed, ret:%d\n", ret);
        return ret;
    }

    log_warning("worker proc init, pid:%d\n", getpid());

    ProxyMgr.init(config.svr_id,config.svr_type, &MessageProcessorInst);

    struct timeval tm;
    tm.tv_sec = 1;
    tm.tv_usec = 0;

    while (1 != stop)
    {
        event_base_loopexit(g_event_base, &tm);
        event_base_loop(g_event_base, EVLOOP_NONBLOCK);

        ConnectToCenter::Instance()->Reconnect();
        ProxyMgr.Reconnect();

        DbWorkerMgrInst.CheckMsg();

        //检查是否有新配置
        ReloadConfig(config.svr_id);

        NewConnect();

        usleep(1000);
    }

    event_base_free(g_event_base);
    return 0;
}

