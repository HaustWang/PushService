#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <evhttp.h>
#include <vector>

#include "https_server.h"
#include "log4cpp_log.h"
#include "push_common.h"
#include "push_proto_common.h"
#include "push_proto_server.h"
#include "comm_server.h"
#include "connect_center.h"
#include "center_message_processor.h"


#define LOG_NAME "loader"

extern volatile int stop;

struct event_base *g_event_base = NULL;

static void send_document_cb (struct evhttp_request *req, void *arg)
{
    const char *uri = evhttp_request_get_uri (req);

    if (evhttp_request_get_command (req) != EVHTTP_REQ_GET)
    {
        evhttp_send_reply (req, 200, "OK", NULL);
        return;
    }

    log_debug ("get request for <%s>\n", uri);

    struct evkeyvalq all_param;
    struct evkeyval *cur_param;
    int iret = evhttp_parse_query_str(evhttp_uri_get_query(req->uri_elems), &all_param);

    std::map<std::string,std::string> request_parm ;
    if(0 == iret)
    {
        for (cur_param = all_param.tqh_first; cur_param != NULL; cur_param = cur_param->next.tqe_next)
        {
            request_parm[cur_param->key] = cur_param->value;
            log_debug("request_parameter:key:%s, value=%s", cur_param->key, cur_param->value);
        }
        evhttp_clear_headers(&all_param);
    }

    std::string resp;
    CENTERMSGPROINST.GetAddress(request_parm["client_id"], resp);

    if(resp.empty())
        resp.assign("doesnt have any connector running");

    struct evbuffer *evb = evbuffer_new ();

    evhttp_add_header (evhttp_request_get_output_headers (req),
                       "Content-Type", "application/x-yaml");
    evbuffer_add (evb, resp.c_str(), resp.length());

    evhttp_send_reply (req, 200, "OK", evb);

    if (NULL != evb)
        evbuffer_free (evb);
}


int InitNet()
{
    InitLibevent(g_event_base);
    return 0;
}

bool ReloadConfig()
{
    if(ConnectToCenter::Instance()->IsNewConfig())
    {
        const SvrConfig& config = ConnectToCenter::Instance()->GetConfig();
        set_log_type(config.log_type());

        init_log(LOG_NAME, config.log_dir().c_str(),config.log_config().c_str());
        set_log_level(config.log_level());
        ConnectToCenter::Instance()->SetNewConfig(false);
        return true;
    }

    return false;
}

int main(int argc, char **argv)
{
    SingleInstance(argv[0]);

    Config config;
    config.svr_type = SERVER_TYPE_LOADER;
    config.need_listen = true;

    ParseArg(argc, argv, config);
    if(!IsAddressListening(config.center_ip.c_str(), config.center_port))
    {
        printf("center server not startted! address:%s:%hd\n", config.center_ip.c_str(), config.center_port);
        return -1;
    }

    InitSignal();

    int ret = InitNet();
    if (ret != 0)
    {
        printf("InitNet failed, ret:%d\n", ret);
        return ret;
    }

    CENTERMSGPROINST.Init(SERVER_TYPE_LOADER, config.svr_id);
    ConnectToCenter::Instance()->Init(config.center_ip, config.center_port, config.svr_type, config.svr_id);
    ConnectToCenter::Instance()->SetMessageProcessor(&CENTERMSGPROINST);

    while(true)
    {
        event_base_loop(g_event_base, EVLOOP_ONCE);
        if(ReloadConfig())
            break;
    }

    HttpsServerInst.InitEnv(config.listen_port, send_document_cb);
    if(0 != HttpsServerInst.StartHttpsService())
    {
        log_error("StartHttpsService failed!");
        stop = 1;
    }

    event_set_log_callback(CommonEventLog);
    struct timeval tm;
    tm.tv_sec = 1;
    tm.tv_usec = 0;

    while (1 != stop)
    {
        event_base_loopexit(g_event_base, &tm);
        event_base_loop(g_event_base, EVLOOP_NONBLOCK);

        ConnectToCenter::Instance()->Reconnect();

        //检查是否有新配置
        ReloadConfig();

        usleep(1000);
    }

    event_base_free(g_event_base);
    return 0;
}

