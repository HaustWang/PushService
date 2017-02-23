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
#include "comm_config.h"
#include "comm_datetime.h"
#include "log4cpp_log.h"
#include "comm_server.h"

#include "push_proto_common.h"
#include "work_manage.h"
#include "push_common.h"
#include "connect_center.h"

const int kMaxDBWorkNum = 64;
struct event_base *g_event_base = NULL;

#define LOG_NAME "dbproxy"

int CheckFlag()
{
    if (stop)
    {
        log_warning("dbproxy recv quit signal, quit......\n");
        exit(0);
    }
    return 0;
}

 int InitProxySvr(Config const& config)
{
    event_set_log_callback(CommonEventLog);

    int ret = InitListener("0.0.0.0", config.listen_port, g_event_base, DBWorkManage::Instance());
    if (ret < 0)
    {
        printf("InitNet failed, ret:%d\n", ret);
        log_warning("InitNet failed, ret:%d\n", ret);
        return ret;
    }
    InitSignal();

    return 0;
}

bool ReloadConfig()
{
    if(ConnectToCenter::Instance()->IsNewConfig())
    {
        const SvrConfig& config = ConnectToCenter::Instance()->GetConfig();
        set_log_type(config.log_type());

        char log_name[64] = {0};
        snprintf(log_name, sizeof(log_name), LOG_NAME "_%d", getpid());
        init_log(log_name, config.log_dir().c_str(),config.log_config().c_str());
        set_log_level(config.log_level());
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
        printf("center server not startted! address:%s:%hd\n", config.center_ip.c_str(), config.center_port);
        return -1;
    }

    InitLibevent(g_event_base);
    ConnectToCenter::Instance()->Init(config.center_ip, config.center_port, config.svr_type, config.svr_id);
    while(true)
    {
        if(ReloadConfig())
            break;
    }

    int ret = InitProxySvr(config);
    if (ret != 0)
    {
        printf("InitProxySvr failed ret:%d\n", ret);
        log_warning("InitProxySvr failed ret:%d\n", ret);
        return ret;
    }

	DBWorkManage::Instance();
	log_warning("dbproxy start runing\n");
	daemon(1, 1);

    while (true)
    {
        event_base_loop(g_event_base, EVLOOP_NONBLOCK);

        CheckFlag();

        //静默加载配置
        ReloadConfig();
        usleep(1000); //提高了延迟
    }

    return 0;
}

