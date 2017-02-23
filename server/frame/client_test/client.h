#ifndef __CONNECTR_CLIENT_H_
#define __CONNECTR_CLIENT_H_

#include <string>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <stdio.h>
#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/bufferevent_struct.h"
#include "log4cpp_log.h"

class SvrConfigure
{
 public:
    std::string server_ip;
    unsigned short server_port;
    struct bufferevent* bufevt;
    int fd;
    std::string log_dir ;
    int log_level ;
    int log_type;
public:
void ShortDebugString()
{
    log_debug("client info, server_ip:%s, server_port:%d",server_ip.c_str(), server_port);
}

SvrConfigure():server_port(0), bufevt(NULL),fd(-1)
{
    log_dir = "./";
    log_level = 7;
    log_type = 2;
}

~SvrConfigure()
{
    if( bufevt )
    {
        bufferevent_free(bufevt);
        bufevt = NULL;
    }
}
};

void Reconnect( );
void BuffereventEventCallback(struct bufferevent *bev,short what ,void *ctx);
void BuffereventReadCallback(struct bufferevent *bev, void *ctx);

void login();

#endif
