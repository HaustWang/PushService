#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <vector>
#include <map>
#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include <event2/listener.h>

#include "base.h"
#include "comm_datetime.h"
#include "log4cpp_log.h"
#include "comm_server.h"

#include "push_proto_common.h"
#include "work_manage.h"
#include "push_common.h"
#include "connect_center.h"

const int kMaxDBWorkNum = 64;
struct event_base *g_event_base = NULL;
extern volatile int stop;

#define LOG_NAME "proxy"

 int InitProxySvr(Config const& config)
{
    event_set_log_callback(CommonEventLog);

    int ret = InitListener("0.0.0.0", config.listen_port, g_event_base, DBWorkManage::Instance());
    if (ret < 0)
    {
        printf("[%s:%d] InitNet failed, ret:%d\n", __FILE__, __LINE__, ret);
        return ret;
    }
    InitSignal();

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


int main(int argc, char **argv)
{
    Config config;
    config.need_listen = true;
    config.svr_type = SERVER_TYPE_PROXY;
    ParseArg(argc, argv, config);

    if(!IsAddressListening(config.center_ip.c_str(), config.center_port))
    {
        printf("[%s:%d] center server not started! address:%s:%hd\n", __FILE__, __LINE__, config.center_ip.c_str(), config.center_port);
        return -1;
    }

    int ret = InitProxySvr(config);
    if (ret != 0)
    {
        printf("[%s:%d] InitProxySvr failed ret:%d\n", __FILE__, __LINE__,  ret);
        return ret;
    }

    ConnectToCenter::Instance()->Init(config.center_ip, config.center_port, config.svr_type, config.svr_id,
        GetLocalListenIp(config.listen_port, true), config.listen_port);

    while(true)
    {
        event_base_loop(g_event_base, EVLOOP_ONCE);
        if(ReloadConfig(config.svr_id))
            break;
    }

	DBWorkManage::Instance()->Init(config);
	log_debug("proxy start runing\n");

    struct timeval tm;
    tm.tv_sec = 1;
    tm.tv_usec = 0;

    while(1 != stop)
    {
        event_base_loopexit(g_event_base, &tm);
        event_base_loop(g_event_base, EVLOOP_NONBLOCK);

        ConnectToCenter::Instance()->Reconnect();

        //静默加载配置
        ReloadConfig(config.svr_id);
        usleep(1000); //提高了延迟
    }

    event_base_free(g_event_base);
    return 0;
}

