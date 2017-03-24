#ifndef HTTP_PROCESS_H_
#define HTTP_PROCESS_H_

#include <string>
#include <map>
#include "event2/event.h"
#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/bufferevent_struct.h"
#include "event2/util.h"
#include "event2/keyvalq_struct.h"

class HttpProcess{

public:
    static HttpProcess &instance()
    {
        static HttpProcess _instance;
        return _instance;
    }
    HttpProcess();
    virtual ~HttpProcess();
    int Init(const char *addr, unsigned int port);
    void HttpRequest(struct evhttp_request *req, void *arg);

private:
    void GetRequestParam(struct evhttp_request *req, std::map<std::string, std::string> *request_parm);
    void SendResponse(struct evhttp_request *req, int ret, const std::string& msg);
private:
};

#endif /* HTTP_PROCESS_H_ */
