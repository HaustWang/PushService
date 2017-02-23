#include "comm_http_client.h"
#include "log4cpp_log.h"
#include "json/json.h"

extern struct event_base *g_event_base;
char HttpClient::http_response[100 * 1024];

static inline void PrintRequestHeadInfo(struct evkeyvalq *header)
{
    struct evkeyval *first_node = header->tqh_first;
    while (first_node)
	{
        log_debug("key:%s  value:%s", first_node->key, first_node->value);
        first_node = first_node->next.tqe_next;
    }
}

void HttpCallBackRsp(struct evhttp_request *req, void *arg)
{
    struct HttpClient *http_client = (struct HttpClient *)arg;
    ASSET_AND_RETURN_VOID(arg);
    ASSET_AND_RETURN_VOID(req);
    http_client->ProcessDataCallBack(req);
}

void CloseCB(struct evhttp_connection *con, void *arg)
{
    struct HttpClient *http_client = (struct HttpClient *)arg;
    if (http_client->conn != con)
    {
        log_error("con:%p is not equal to arg:%p", con, http_client);
        return;
    }

    log_info("host:%s close connect", http_client->host.c_str());
    http_client->status = STATUS_CONNECTING;
}

HttpClient::~HttpClient()
{
    if (conn)
    {
        evhttp_connection_free(conn);
        conn = NULL;
    }
}

int HttpClient::Init(const char* http_host,unsigned short http_port ,HttpCb http_callback)
{
    log_info("http host:%s", http_host);
    conn = evhttp_connection_base_new(g_event_base, NULL, http_host, http_port);
    if (conn == NULL)
    {
        log_error("evhttp_connection_base_new failed");
        return -1;
    }
    evhttp_connection_set_closecb(conn, CloseCB, this);
    host = http_host;
    port = http_port;
    callback = http_callback;
    status = STATUS_CONNECTED;
    return 0;
}

int HttpClient::Query(const char* path_query)
{
    if (status != STATUS_CONNECTED)
    {
        if (conn)
        {
            evhttp_connection_free(conn);
            conn = NULL;
        }

        Init(host.c_str(),port ,callback);
    }

    //这里不要手动释放， 在evhttp_connection_done
    struct evhttp_request *req = evhttp_request_new(callback, this);
    if (req == NULL)
    {
        log_error("evhttp_request_new failed");
        return -5;
    }
    evhttp_add_header(req->output_headers, "Host", host.c_str());
    evhttp_add_header(req->output_headers,"Connection","Keep-Alive");
    evhttp_add_header(req->output_headers, "Content-Length", "0");
    return evhttp_make_request(conn, req, EVHTTP_REQ_GET, path_query);
}

int HttpClient::GetHttpResponse(struct evhttp_request* req)
{
    if (req->response_code != HTTP_OK)
    {
        log_error("the uri invalid ret code:%d", (int)(req->response_code));
        return -1;
    }

    struct evbuffer* buf = evhttp_request_get_input_buffer(req);
    size_t len = evbuffer_get_length(buf);
    if (len >= sizeof(http_response) || len <= 0)
    {
        log_error("http response len:%lu is invalid", len);
        return -10;
    }

    PrintRequestHeadInfo(req->output_headers);
    evbuffer_remove(buf, http_response, len);
    http_response[len] = '\0';

    log_debug("len:%zu  body size:%zu content:%s",
            len, req->body_size, http_response);

    return 0;
}


int HttpClient::ProcessDataCallBack(struct evhttp_request* req)
{
    int ret = GetHttpResponse(req);
    if (ret < 0)
    {
        log_error("GetHttpResponse failed, ret:%d", ret);
        return ret;
    }
   return 0;
}


static inline void print_uri_parts_info(const struct evhttp_uri * http_uri)
{
    log_debug("scheme:%s", evhttp_uri_get_scheme(http_uri));
    log_debug("host:%s", evhttp_uri_get_host(http_uri));
    log_debug("path:%s", evhttp_uri_get_path(http_uri));
    log_debug("port:%d", evhttp_uri_get_port(http_uri));
    log_debug("query:%s", evhttp_uri_get_query(http_uri));
    log_debug("userinfo:%s", evhttp_uri_get_userinfo(http_uri));
    log_debug("fragment:%s", evhttp_uri_get_fragment(http_uri));
}

