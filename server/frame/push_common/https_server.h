#pragma once

#include <openssl/ssl.h>

#include "common_singleton.h"

#define COMMON_PASSCODE "R23"

typedef void (*handle)(struct evhttp_request *req, void *arg);

class HttpsServer
{
public:
    void InitEnv(unsigned short port, handle http_handle); 
    int StartHttpsService();

private:
    int LoadCertFile(SSL_CTX* ctx);

private:
    unsigned short m_listen_port;

    handle m_http_handle;
};

#define HttpsServerInst Singleton<HttpsServer>::Instance()
