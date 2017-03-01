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
#include "comm_config.h"
#include "comm_datetime.h"
#include "onliner_message_processor.h"
#include "base.h"
#include "comm_server.h"
#include "http_manager.h"
#include "connect_center.h"

struct event_base *g_event_base = NULL;
extern volatile int stop;

#define LOG_NAME "phpproxy"

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
            ConnectToOnlinerMgr::Instance()->AddServer(it->ip(), it->port());

        ConnectToCenter::Instance()->EraseNewAddress(SERVER_TYPE_PROXY);
    }
}


int main(int argc, char** argv)
{
    SingleInstance(argv[0]);

    Config config;
    config.svr_type = SERVER_TYPE_PHP_PROXY;
    config.need_listen = false;
    ParseArg(argc, argv, config);

    if(!IsAddressListening(config.center_ip.c_str(), config.center_port))
    {
        printf("center server not startted! address:%s:%hd\n", config.center_ip.c_str(), config.center_port);
        return -1;
    }

    event_set_log_callback(CommonEventLog);
    InitSignal();

    InitLibevent(g_event_base);
    ConnectToCenter::Instance()->Init(config.center_ip, config.center_port, config.svr_type, config.svr_id);
    while(true)
    {
        event_base_loop(g_event_base, EVLOOP_ONCE);
        if(ReloadConfig())
            break;
    }

    //开启http服务， 接收php发过来的消息
    int ret = HttpManager::Instance().Init();
    if(ret!=0)
    {
        log_warning("Init http server error, ret:%d\n",ret);
    }

    //作为客户端,连接onliner
    ConnectToOnlinerMgr::Instance()->init(config);

    log_warning("Phpproxy start running \n" );

    struct timeval tm;
    tm.tv_sec = 1;
    tm.tv_usec = 0;

    while(1 != stop)
    {
        event_base_loopexit(g_event_base, &tm);
        event_base_loop(g_event_base, EVLOOP_NONBLOCK);

        ConnectToCenter::Instance()->Reconnect();
        ConnectToOnlinerMgr::Instance()->Reconnect();

        //静默加载配置
        ReloadConfig();
        NewConnect();

        usleep(1000); //提高了延迟
    }

    return 0;
}
