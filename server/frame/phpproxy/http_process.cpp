#include <stdlib.h>
#include <map>
#include <string>
#include <evhttp.h>
#include "http_process.h"
#include "log4cpp_log.h"
#include "push_proto_common.h"
#include "push_proto_client.h"
#include "push_proto_server.h"
#include "onliner_message_processor.h"
#include "json.h"

extern struct event_base* g_event_base;


static void api_request_cb(struct evhttp_request *req, void *arg)
{
    HttpProcess::instance().HttpRequest(req, arg);
}

HttpProcess::HttpProcess()
{
    srand(time(NULL));
    seq = rand();
}

HttpProcess::~HttpProcess()
{
}

int HttpProcess::Init(const char *addr, unsigned int port)
{
    struct evhttp *http = evhttp_new(g_event_base);
    if(!http)
    {
        log_error("evhttp_new err!");
        return -1;
    }
    //绑定http的回调函数
    int iret = evhttp_set_cb(http, "/push",api_request_cb, NULL);
    if(0 != iret)
    {
        log_error("evhttp_set_cb err!");
        return -1;
    }

    iret = evhttp_bind_socket(http, addr, port);
    if(0 != iret)
    {
        log_error("evhttp_bind_socket err!");
        return -1;
    }

    return 0;
}

void HttpProcess::HttpRequest(struct evhttp_request *req, void *arg)
{
    UNUSED(arg);
    //只处理Get请求
    if(EVHTTP_REQ_POST != evhttp_request_get_command(req))
    {
        evhttp_send_error(req, HTTP_NOTFOUND, "err");
        return;
    }

    std::map<std::string,std::string> request_parm ;
    GetRequestParam(req,&request_parm);

    static unsigned char data[1024*1024];
    memset(data, 0, sizeof(data));

    struct evbuffer* evinput = evhttp_request_get_input_buffer(req);
    int length = (int)evbuffer_get_length(evinput);
    evbuffer_remove(evinput, data, length);

    Json::Reader reader;
    Json::Value value;
    if(!reader.parse((const char*)data, (const char*)data+length, value))
    {
        SendResponse(req, -1, "format error!");
        return;
    }

    Json::FastWriter writer;
    std::string msg = writer.write(value["msg"]);
    if(request_parm["platform"] == "0")
        ConnectToOnlinerMgr::Instance()->SendPushMsgToDBWorker(atoi(request_parm["msgid"].c_str()),
                value["client_ids"].begin(), value["client_ids"].end(), atoi(request_parm["expire_time"].c_str()), (unsigned char*)msg.data(), msg.length());

    SendResponse(req, 0, "success");
    return;
}

void HttpProcess::GetRequestParam(struct evhttp_request *req, std::map<std::string, std::string> *request_parm)
{
    //获取GET参数
    struct evkeyvalq all_param;
    struct evkeyval *cur_param;
    int iret = evhttp_parse_query_str(evhttp_uri_get_query(req->uri_elems), &all_param);

    if(0 == iret)
    {
        for (cur_param = all_param.tqh_first; cur_param != NULL; cur_param = cur_param->next.tqe_next)
        {
            (*request_parm)[cur_param->key] = cur_param->value;
        }
        evhttp_clear_headers(&all_param);
    }

}

void HttpProcess::SendResponse(struct evhttp_request *req, int ret, const std:: string& msg)
{
    evhttp_add_header(req->output_headers, "Content-Type", "text/html;charset=utf-8");
    evhttp_add_header(req->output_headers, "Connection", "keep-alive");
    evhttp_add_header(req->output_headers, "Cache-Control", "no-cache");

    struct evbuffer *databuf = evbuffer_new();
    evbuffer_add_printf(databuf, "{\"status\":%d,\"msg\":\"%s\"}", ret, msg.c_str());
    evhttp_send_reply(req, HTTP_OK, "", databuf);
    evbuffer_free(databuf);
}


