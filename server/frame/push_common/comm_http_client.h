#ifndef __COMMHTTP_CLIENT_H__
#define __COMMHTTP_CLIENT_H__

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/bufferevent.h>
#include <string>


typedef void (*HttpCb)(struct evhttp_request *req, void *arg);
void HttpCallBackRsp(struct evhttp_request *req, void *arg);
void CloseCB(struct evhttp_connection *con, void *arg);

int ProcessHttpResponse(const char *http_response);

const int kHttpPort = 80;
const int kConnectionTimeOut = 120;

enum CON_STATUS
{
    STATUS_CONNECTED = 0,
    STATUS_CONNECTING = 1,
};

class HttpClient
{
public:
    HttpClient():conn(NULL), status(STATUS_CONNECTING), last_reconnector_time(0){}
    ~HttpClient();
    int Init(const char* http_host,unsigned short http_port ,HttpCb http_callback);
    int Query(const char* path_query);

    int GetHttpResponse(struct evhttp_request* req);

    int ProcessDataCallBack(struct evhttp_request* req);
public:
    struct evhttp_connection *conn;
    CON_STATUS status;
    time_t last_reconnector_time;
    std::string host;
    unsigned short port;
    HttpCb callback;
    static char http_response[100 * 1024];
};


#endif


