#ifndef __HTTP_MANAGER_H__
#define __HTTP_MANAGER_H__

#include <vector>
#include "comm_http_client.h"
#include "http_process.h"
#include "push_proto_server.h"

enum
{
    REPORT_TYPE_RECEIVED = 1,
    REPORT_TYPE_READ = 2,
    REPORT_TYPE_UNLOAD = 3,
};

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

    int ReportData(int64_t msgid, std::string const client_id, int action);
private:
    HttpManager();
private:
    HttpClient send_msgproxy_client_;
    HttpProcess get_proxymsg_server_; 
};

#endif
