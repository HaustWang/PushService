#include "push_common.h"
#include <openssl/md5.h>
#include <sys/time.h>
#include <sstream>

#include <event2/util.h>

#include "comm_datetime.h"
#include "log4cpp_log.h"
#include "push_common.h"
#include "comm_client.h"
#include "message_processor.h"


unsigned long strhash(const char *str, unsigned int length)
{
    if (str == NULL)
        return 0;

    unsigned long ret=0;
    if(sizeof(unsigned long) >= length)
    {
        memcpy((char *)&ret, str, length);
        return ret;
    }

    int i = 0,l = 0;
    unsigned short *s = NULL;
    l=(length+1)/2;
    s=(unsigned short *)str;
    for (i=0; i < l; ++i)
        ret^=(s[i]<<(i&0x0f));

    return ret;
}

unsigned long get_curr_time(void)
{
    struct timeval tv;
    unsigned long curr_time = 0;

    gettimeofday(&tv, NULL);

    curr_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

    return curr_time;
}

std::string ToHexString(const char *c, int l)
{
    static std::ostringstream oss;
    oss.clear();

    for(int i = 0; i < l; ++i)
        oss << std::hex << c;

    return oss.str();
}

std::string GenerateClientId(std::string const& imsi, time_t t)
{
    static unsigned char info[60];
    memset(info, 0, sizeof(info));
    memcpy(info+sizeof(info)-sizeof(time_t), (unsigned char *)&t, sizeof(time_t));
    if(imsi.length() > sizeof(info) - sizeof(time_t))
        memcpy(info, imsi.data(), sizeof(info) - sizeof(time_t));
    else
        memcpy(info, imsi.data(), imsi.length());

    static unsigned char md5_value[16];
    memset(md5_value, 0, sizeof(md5_value));

    static MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, info, sizeof(info));
    MD5_Final(md5_value, &ctx);

    static char out[37];
    int len = 0;
    for(int i = 0; i < 16; ++i)
    {
        snprintf(out+len, 37-len, "%02x", md5_value[i]);
        len += 2;
        if(i == 3 || i == 5 || i == 7 || i == 9)
        {
            strcat(out, "-");
            ++len;
        }
    }
    out[len] = 0;

    return out;
}

const char key_group[] = "~!@#$%^&*()_+`1234567890-=QWERTYUIOP{}|qwertyuiop[]\\ASDFGHJKL:\"asdfghjkl;\'ZXCVBNM<>?zxcvbnm,./";

int GetIntByte(int in, int seq)
{
    return (char)(((in >> (8*seq)) & 0xff) + (int)rand()) % (sizeof(key_group)-1);
}

std::string GenerateSecretKey(std::string const& last_key, std::string const& client_id)
{
    int key_hash = (int)strhash(last_key.c_str(), last_key.length());
    int id_hash = (int)strhash(client_id.c_str(), client_id.length());
    int rand_value = (int)rand();
    int nowt = time(NULL);

    static std::ostringstream oss;
    oss.seekp(0);
    for(int i = 0; i < 4; ++i)
    {
       oss << key_group[GetIntByte(key_hash, i)] << key_group[GetIntByte(id_hash, (i+1)%4)] << key_group[GetIntByte(rand_value, (i+2)%4)] << key_group[GetIntByte(nowt, (i+3)%4)];
    }

    return oss.str();
}

int split_str(const char* ps_str , char* ps_sp , std::vector<std::string> &v_ret)
{
    char* ps_temp;
    char* p;
    int i_len = (int)strlen(ps_str);
    std::string st_str;
    ps_temp = new char[i_len+2];
    snprintf(ps_temp , i_len+1 , "%s" , ps_str);
    char *last = NULL;
    p = strtok_r(ps_temp , ps_sp , &last);
    if(NULL == p)
    {
        delete []ps_temp;
        return 0;
    }
    st_str = (std::string)p;
    v_ret.push_back(st_str);
    while( NULL != ( p=strtok_r(NULL , ps_sp , &last) ) )
    {
        st_str = (std::string)p;
        v_ret.push_back(st_str);
    }
    delete []ps_temp;
    return 0;
}

namespace server {

const int kMaxProcessMessagePerSecond = 100000;
inline bool IsSocketOverload()
{
    static int recv_message_num = 0;
    static struct timeval last_check_message_num_time = {0, 0};
    ++recv_message_num;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t interval = timersub_ms(&tv, &last_check_message_num_time);
    if (interval < 1000)
    {
        if ((int)recv_message_num > kMaxProcessMessagePerSecond)
            return true;
        else
            return false;
    }
    last_check_message_num_time = tv;
    recv_message_num = 1;
    return false;
}

void BuffereventEventCallback(struct bufferevent* bev, short what, void* ctx)
{
    ClientInfo  *client_info = (ClientInfo *)ctx;

    if(client_info->is_register)
	    log_info("connect to server, server_ip:%s, server_port:%hd, %d, %d", client_info->remote_ip.c_str(), client_info->remote_port, what, BEV_EVENT_CONNECTED);
    else
	    printf("[%s:%d] connect to server, server_ip:%s, server_port:%hd, %d, %d\n", __FILE__, __LINE__, client_info->remote_ip.c_str(), client_info->remote_port, what, BEV_EVENT_CONNECTED);


    if (client_info->fd != bufferevent_getfd(bev) && client_info->fd != -1)
    {
        if(client_info->is_register)
            log_error("client info fd:%d is not equal bufferevent fd:%d", client_info->fd, bufferevent_getfd(bev));
        else
            printf("[%s:%d] client info fd:%d is not equal bufferevent fd:%d\n", __FILE__, __LINE__, client_info->fd, bufferevent_getfd(bev));

        return;
    }

	if(what & BEV_EVENT_CONNECTED)
	{
        if(client_info->is_register)
	    	log_info("sucess Connect to server, begin to register! Server_ip:%s, server_port:%hd", client_info->remote_ip.c_str(), client_info->remote_port );
        else
	    	printf("[%s:%d] sucess Connect to server, begin to register! Server_ip:%s, server_port:%hd\n", __FILE__, __LINE__, client_info->remote_ip.c_str(), client_info->remote_port );

		client_info->processor->RegisterToServer(client_info);
		return;
	}

	if ((what & BEV_EVENT_EOF) || (what & BEV_EVENT_ERROR))
   	{
        if(client_info->is_register)
		    log_warning("lost connection to server, Server_ip:%s, server_port:%hd", client_info->remote_ip.c_str(), client_info->remote_port);
        else
		    printf("[%s:%d] lost connection to server, Server_ip:%s, server_port:%hd\n", __FILE__, __LINE__, client_info->remote_ip.c_str(), client_info->remote_port);

        if(client_info->bevt)
        {
            bufferevent_free(client_info->bevt);
            client_info->bevt = NULL;
        }

        if(client_info->fd >0)
        {
            close(client_info->fd);
            client_info->fd= -1;
        }
   	}

	return;
}

void BuffereventReadCallback(struct bufferevent *bev, void *ctx)
{
    ClientInfo  *client_info = (ClientInfo *)ctx;
    if (client_info->fd != bufferevent_getfd(bev) )
    {
        if(client_info->is_register)
            log_error("client info fd:%d is not equal bufferevent fd:%d", client_info->fd, bufferevent_getfd(bev));
        else
            printf("[%s:%d] client info fd:%d is not equal bufferevent fd:%d\n", __FILE__, __LINE__, client_info->fd, bufferevent_getfd(bev));
        return;
    }

    while(1)
    {
        static char buf[1024 * 1024];
        char *pkg_start = buf;
        int pkg_len = sizeof(buf);
        //小于0,表示流的数据已经错乱了，只能断开连接
        int ret = client_info->processor->GetCompletePackage(bev, pkg_start, &pkg_len);
        if(ret < 0)
        {
            if(client_info->is_register)
                log_warning("GetCompletePackage err  ret %d", ret);
            else
                printf("[%s:%d] GetCompletePackage err  ret %d\n", __FILE__, __LINE__, ret);

            break;
        }
        //大于0,表示一个包还不完整, 继续收包
        else if(ret > 0)
        {
            return;
        }

        if(IsSocketOverload())
        {
            if(client_info->is_register)
                log_error("Svr process message overload! drop msg");
            else
                printf("[%s:%d] Svr process message overload! drop msg\n", __FILE__, __LINE__);
            break;
        }

        static SvrMsg svr_msg;
        svr_msg.Clear();
        if (!svr_msg.ParseFromArray(pkg_start + sizeof(int), pkg_len - sizeof(int)))
        {
            break;
        }
        //required废弃了， 逻辑层自己检查
        if (!svr_msg.has_head() || !svr_msg.head().has_type() )
        {
            // ClientManage::Instance()->DeleteConnectToCenter(client_info);
            break;
        }

        if(client_info->is_register)
            log_debug("msg:%s", svr_msg.ShortDebugString().c_str());
        else
            printf("[%s:%d] msg:%s\n", __FILE__, __LINE__, svr_msg.ShortDebugString().c_str());
        client_info->processor->ProcessMessage(client_info, &svr_msg.head(), svr_msg.body());
    }
}


void SvrEventCallback(struct bufferevent *bev, short what , void *ctx)
{
    UNUSED(bev);
    if(what & BEV_EVENT_CONNECTED || NULL == ctx)
    {
        return;
    }

    ClientInfo *client_info = (ClientInfo *)ctx;

    log_warning("client logout, what: %hd, client_info:%s", what, client_info->ShortDebugString().c_str());
    client_info->processor->ProcessClose(client_info);
}


void SvrReadCallback(struct bufferevent *bev, void *ctx)
{
    ClientInfo *client_info = (ClientInfo *)ctx;
    if (client_info->fd != bufferevent_getfd(bev))
    {
        log_error("client info fd:%d is not equal bufferevent fd:%d",
                client_info->fd, bufferevent_getfd(bev));
        return;
    }

    while(1)
    {
        static char buf[1024 * 1024];
        char *pkg_start = buf;
        int pkg_len = sizeof(buf);
        //小于0,表示流的数据已经错乱了，只能断开连接
        int ret = client_info->processor->GetCompletePackage(bev, pkg_start, &pkg_len);
        if(ret < 0)
        {
            log_warning("server closed socket, because client:%s packet is wrong, ret:%d\n", GetPeerAddrStr(client_info->address), ret);
            ClientManage::Instance()->DeleteClientInfo(client_info);
            break;
        }
        //大于0,表示一个包还不完整, 继续收包
        else if(ret > 0)
        {
            return;
        }

        if(IsSocketOverload())
        {
            log_error("Svr process message overload! drop msg");
            break;
        }

        static  SvrMsg  svr_msg;
        if (!svr_msg.ParseFromArray(pkg_start + sizeof(int), pkg_len - sizeof(int)))
        {
            log_warning("client:%s parse message failed, pkg len:%d", GetPeerAddrStr(client_info->address), pkg_len);
            ClientManage::Instance()->DeleteClientInfo(client_info);
            break;
        }
        //required废弃了， 逻辑层自己检查
        if (!svr_msg.has_head() ||  !svr_msg.head().has_type())
	     {
            log_warning("client:%s parse message failed, pkg len:%d", GetPeerAddrStr(client_info->address), pkg_len);
            ClientManage::Instance()->DeleteClientInfo(client_info);
            break;
        }

        client_info->processor->ProcessMessage(client_info, &svr_msg.head(), svr_msg.body());
    }
}

void SvrListenerCallback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx)
{
    UNUSED(socklen);
    UNUSED(ctx);

    char addr_str[128]={0};
    switch(address->sa_family)
    {
        case AF_INET:
            {
                struct sockaddr_in *psa = (struct sockaddr_in *)address;
                log_debug("new connect from %s:%d, fd is %d\n",
                        inet_ntop(address->sa_family, &psa->sin_addr, addr_str, sizeof(addr_str)),
                        ntohs(psa->sin_port), fd);
                break;
            }
        case AF_INET6:
            {
                struct sockaddr_in6 *psa = (struct sockaddr_in6 *)address;
                log_debug("new connect from %s, fd is %d\n",
                        inet_ntop(address->sa_family, &psa->sin6_addr, addr_str, sizeof(addr_str)), fd);
                break;
            }
        default:
            {
                log_error("invalid family type:%d", (int)address->sa_family);
                return;
            }
    }

    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    if(!bev)
    {
        log_error("fd %d, bufferevent_socket_new err\n", fd);
        close(fd);
        return;
    }

    ClientInfo* client_info = ClientManage::Instance()->CreateClientInfo(fd, bev, address, (MessageProcessor*)ctx);
    if (client_info == NULL)
    {
        log_error("fd %d, create client info err\n", fd);
        close(fd);
        return;
    }

    bufferevent_setcb(bev, SvrReadCallback, NULL, SvrEventCallback, client_info);

    if(0 != bufferevent_enable(bev, EV_READ|EV_WRITE))
    {
        log_error("fd %d, bufferevent_enable err\n", fd);
        bufferevent_free(bev);
        return;
    }

    int tcp_nodelay = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &tcp_nodelay, sizeof(tcp_nodelay)) < 0)
    {
        log_error("fd %d, set IPPROTO_TCP:TCP_NODELAY err\n", fd);
        bufferevent_free(bev);
        return;
    }
}

void SvrListenerErrCallback(struct evconnlistener *listener, void *ctx)
{
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    log_error("Got an error %d (%s) on the listener. ""Shutting down.", err, evutil_socket_error_to_string(err));

    event_base_loopbreak(base);
}

};

int ClientConnect(const char *ip, unsigned short port, struct event_base* &base, struct bufferevent* &bevt, int &fd, void *ctx)
{
    int addrlen = 0;
	struct sockaddr * paddr = make_sock_addr(ip, port, addrlen);
	fd = socket( AF_INET, SOCK_STREAM, 0);
	bevt = bufferevent_socket_new(base, fd, 0);
	bufferevent_setcb(bevt, server::BuffereventReadCallback, NULL, server::BuffereventEventCallback, ctx);
	bufferevent_enable(bevt, EV_READ);
	bufferevent_socket_connect(bevt, paddr, addrlen);
    return 0;

}



int InitLibevent(struct event_base* &base)
{
    if(NULL == base)
    {
        base = event_base_new();
        if(NULL == base)
        {
            log_warning("event_base_new error. \n");
            return -1;
        }
    }

    return 0;
}

int InitListener(const char *ip, unsigned short port, struct event_base* &base, MessageProcessor* processor)
{
    struct sockaddr_in listen_addr;
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(port);
    listen_addr.sin_addr.s_addr = inet_addr(ip);

    if(-1 == InitLibevent(base))
        return -1;

    //监听
    struct evconnlistener *listener = evconnlistener_new_bind(base, server::SvrListenerCallback, processor,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, 128,
            (struct sockaddr *)(&listen_addr), sizeof(listen_addr));
    if(NULL == listener)
    {
        log_warning("evconnlistener_new_bind err:%s \n", strerror(errno));
        exit(1);
        return -10;
    }
    evconnlistener_set_error_cb(listener, server::SvrListenerErrCallback);

    return 0;
}


int ParseArg (int argc, char **argv, Config& config)
{
    const char* option = "p:m:s:hd";
    int result;
    while((result = getopt(argc, argv, option)) != -1)
    {
        if(result == 'm')
        {
		    char server_ip[64] = {0};
	        sscanf(optarg, "%[^:]:%hd", server_ip, &config.center_port);
            config.center_ip = server_ip;
        }
        else if(result == 'p')
        {
            config.listen_port = (unsigned short)atoi(optarg);
        }
        else if (result == 's')
        {
            config.svr_id = atoi(optarg);
        }
        else if (result == 'h')
        {
	        printf("usage %s -s 1 -m 127.0.0.1:19988 %s\n", argv[0], config.need_listen?"-p 3333":"");
	        exit(1);
        }
    }

    if (config.svr_id < 0)
    {
        printf("usage %s -s 1 -m 127.0.0.1:19988 %s\n", argv[0], config.need_listen?"-p 3333":"");
        exit(1);
    }
    return 0;
}

bool IsAddressListening(const char *ip, unsigned short port)
{
    char cmd[64] = {0};
    snprintf(cmd, sizeof(cmd), "nc -w 1 %s %hd || echo false", ip, port);
    FILE *f = popen(cmd, "r");
    if(NULL == f)   return false;

    bool ret = false;
    if(EOF == fgetc(f))
        ret = true;

    pclose(f);
    return ret;
}

std::string GetLocalListenIp(unsigned short port, bool islan)
{
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "netstat -nl |grep %hd |awk '{print $4}' | awk -F: '{print $1}'", port);

    FILE *f = popen(cmd, "r");
    if(NULL == f)   return "";

    char loc_ip[20] = {0};
    char *pret = fgets(loc_ip, sizeof(loc_ip), f);
    pclose(f);

    if(islan)
    {
        if(strcmp("0.0.0.0", loc_ip) == 0)
        {
            memset(cmd, 0, sizeof(cmd));
            snprintf(cmd, sizeof(cmd), "ifconfig|grep 192.168|xargs echo|awk -F'[: ]' '{print $3}'");

            f = popen(cmd, "r");
            if(NULL == f)   return loc_ip;

            pret = fgets(loc_ip, sizeof(loc_ip), f);
            pclose(f);

            if(NULL == pret)    return "0.0.0.0";
            return loc_ip;
        }
    }

    if(NULL == pret)    return "";
    return loc_ip;
}

std::string GetWlanIp()
{
    char cmd[128] = {0};
    snprintf(cmd, sizeof(cmd), "curl myip.dnsomatic.com");

    FILE *f = popen(cmd, "r");
    if(NULL == f)   return "";

    char ip[20] = {0};
    char *pret = fgets(ip, sizeof(ip), f);
    pclose(f);

    if(NULL == pret)    return "";
    return ip;
}
