#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>

#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>

#include <assert.h>
#include <map>

#include "event2/event.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/bufferevent_struct.h"

#include "comm_client.h"
#include "crypto_aes.h"
#include "log4cpp_log.h"

static const char *first_key = "]u%t&1v=as^f!/i*";
class PushClientInfo : public ClientInfo
{
public:

public:
	PushClientInfo(): ClientInfo(), need_delete(false), aes(first_key, AES_ECB, true)
	{
	}

    ~PushClientInfo()
    {
    }

public:
    std::string client_id;
    bool need_delete;//需要销毁标识
    AES aes;
};


class PushClientManage
{
public:
    static PushClientManage* Instance()
    {
        static PushClientManage*_instance = NULL;
        if(!_instance)
        {
            _instance = new PushClientManage;
        }
        return _instance;
    }
    virtual ~PushClientManage()
    {
    }

    PushClientInfo* CreatePushClientInfo(int fd, struct bufferevent* bev, struct sockaddr* addr);
    int DeletePushClientInfo(PushClientInfo* client_info, bool delay_delete=false);
    PushClientInfo* GetPushClientInfo(int fd);

    int GetClientCnt(void)
    {
        return map_client_info_.size();
    }

    PushClientInfo * GetPushClientInfoById(std::string const & client_id)
    {
        if (map_client_info_.size() <= 0)
        {
                return NULL;
        }

        std::map<int, PushClientInfo *>::iterator it = map_client_info_.begin();
        for (; it!=map_client_info_.end(); it++)
        {
            if (client_id == it->second->client_id)
            {
                return it->second;
            }
        }

        return NULL;
    }

    const std::map<int,PushClientInfo*>& GetMap()
    {
        return map_client_info_;
    }

private:
    PushClientManage()
    {
        map_client_info_.clear();
    }

    std::map<int, PushClientInfo*> map_client_info_;
};

#endif
