#ifndef __HTTP_MANAGER_H__
#define __HTTP_MANAGER_H__

#include <vector>
#include "comm_http_client.h"
#include "http_process.h"
#include "push_proto_server.h"

class HttpManager
{
public:
    virtual ~HttpManager();
    static HttpManager& Instance()
    {
        static HttpManager instance_;
        return instance_;
    }
    int Init();
private:
    HttpManager();
private:
    HttpClient send_msgproxy_client_;
    HttpProcess get_proxymsg_server_; 
};

#endif
