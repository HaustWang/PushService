#include "https_server.h"
#include "log4cpp_log.h"
#include "push_common.h"
#include "push_proto_common.h"
#include "push_proto_server.h"
#include "comm_server.h"
#include "connect_center.h"
#include "https_server.h"
#include "center_message_processor.h"

#include <event2/bufferevent.h>
#include <event2/bufferevent_ssl.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/keyvalq_struct.h>
#include <vector>


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

    struct evhttp_uri *decoded = evhttp_uri_parse (uri);
    if (NULL == decoded)
    {
        printf ("It's not a good URI. Sending BADREQUEST\n");
        evhttp_send_error (req, HTTP_BADREQUEST, 0);
        return;
    }

    struct evkeyvalq kv;
    memset (&kv, 0, sizeof (kv));
    struct evbuffer *buf = evhttp_request_get_input_buffer (req);
    evbuffer_add (buf, "", 1);    /* NUL-terminate the buffer */
    char *payload = (char *) evbuffer_pullup (buf, -1);
    if (0 != evhttp_parse_query_str (payload, &kv))
    { printf ("Malformed payload. Sending BADREQUEST\n");
      evhttp_send_error (req, HTTP_BADREQUEST, 0);
      return;
    }

    char *peer_addr;
    ev_uint16_t peer_port;
    struct evhttp_connection *con = evhttp_request_get_connection (req);
    evhttp_connection_get_peer (con, &peer_addr, &peer_port);

    const char *passcode = evhttp_find_header (&kv, "passcode");
    char response[256];
    evutil_snprintf (response, sizeof (response),
                   "Hi %s!  I %s your passcode.\n", peer_addr,
                   (0 == strcmp (passcode, COMMON_PASSCODE)
                    ?  "liked"
                    :  "didn't like"));
    evhttp_clear_headers (&kv);   /* to free memory held by kv */

    struct evbuffer *evb = evbuffer_new ();

    evhttp_add_header (evhttp_request_get_output_headers (req),
                       "Content-Type", "application/x-yaml");
    evbuffer_add (evb, response, strlen (response));

    evhttp_send_reply (req, 200, "OK", evb);

    if (NULL != decoded)
        evhttp_uri_free (decoded);
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

void NewConnect()
{
    if(ConnectToCenter::Instance()->HasNewAddress(SERVER_TYPE_CONNECTOR))
    {
        const std::vector<SvrAddress>& addrs = ConnectToCenter::Instance()->GetNewAddress(SERVER_TYPE_CONNECTOR);
        for(std::vector<SvrAddress>::const_iterator it = addrs.begin(); it != addrs.end(); ++it)
            CENTERMSGPROINST.AddConnectorAddr(ConnectToCenter::Instance()->fd, *it);

        ConnectToCenter::Instance()->EraseNewAddress(SERVER_TYPE_CONNECTOR);
    }
}

int main(int argc, char **argv)
{
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

    ConnectToCenter::Instance()->Init(config.center_ip, config.center_port, config.svr_type, config.svr_id);

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

        NewConnect();

        usleep(1000);
    }

    event_base_free(g_event_base);
    return 0;
}

