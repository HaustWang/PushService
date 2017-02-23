#ifndef __ONLINER_CLIENT_H__
#define __ONLINER_CLIENT_H__

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
#include "log4cpp_log.h"
#include "base.h"

class MessageProcessor;

class ClientInfo
{
public:
	ClientInfo(): fd(-1), bevt(NULL), server_id(-1),server_type(-1),is_register(false), processor(NULL)
	{
		memset(&address, 0, sizeof(address));
	}

    virtual ~ClientInfo()
    {
        if(bevt)
        {
            bufferevent_free(bevt);
            bevt = NULL;
        }
    }

    void ShortDebugString() const
    {
        log_debug("Show ClientInfo,fd:%d, server_addr:%s  ,server_id:%d, server_type:%d,is_register:%s", fd,GetPeerAddrStr(address) ,server_id, server_type, is_register==true?"TRUE":"FALSE");
    }

    virtual void Reconnect() {}
public:
	int fd;
	struct sockaddr_in address;
	struct bufferevent* bevt;
	int server_id; //标识客户端的编号
	int server_type; //标识客户端的类型
	bool is_register ;
    std::string remote_ip;
    unsigned short remote_port;
    MessageProcessor *processor;
};

class ClientManage
{
public:
    static ClientManage* Instance()
    {
        static ClientManage*_instance = NULL;
        if(!_instance)
        {
            _instance = new ClientManage;
        }
        return _instance;
    }
    virtual ~ClientManage()
    {
    }

    ClientInfo* CreateClientInfo(int fd, struct bufferevent* bev, struct sockaddr* addr, MessageProcessor *processor);
    int DeleteClientInfo(ClientInfo* client_info);
    ClientInfo* GetClientInfo(int fd);
    ClientInfo* GetClientInfoRandom(int svr_type);

    int GetClientCnt(void)
    {
        return map_client_info_.size();
    }

    ClientInfo * GetClientInfoById(int server_id, int server_type);
    void GetClientInfoList(int server_type, std::vector<ClientInfo*>& vec);

    const std::map<int, ClientInfo*>& GetClientMap() const { return map_client_info_; }

private:
    ClientManage()
    {
        map_client_info_.clear();
    }
    std::map<int, ClientInfo*> map_client_info_;
};

#endif
