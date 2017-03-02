#include <stdio.h>
#include <stdlib.h>
#include "message_process.h"
#include "log4cpp_log.h"
#include "base.h"
#include "client.h"
#include "crypto_aes.h"
#include "base64.h"

struct event_base* g_event_base = NULL;
SvrConfigure svr_cfg;
int g_error_times = 0;


void BuffereventReadCallback(struct bufferevent *bev, void *ctx)
{
    UNUSED(ctx);
    g_error_times = 0;
     while(1)
    {
        static char buf[1024 * 1024];
        char *pkg_start = buf;
        int pkg_len = sizeof(buf);
        //小于0,表示流的数据已经错乱了，只能断开连接
        int ret = MessageProcess::Instance()->GetCompletePackage(bev, pkg_start, &pkg_len);
        if(ret < 0)
        {
            log_warning("GetCompletePackage err  ret %d", ret);
            break;
        }
        //大于0,表示一个包还不完整, 继续收包
        else if(ret > 0)
        {
            return;
        }

        MessageProcess::Instance()->ProcessMessage(pkg_start + sizeof(int), pkg_len - sizeof(int));
    }
}

void Reconnect( )
{
    if(svr_cfg.bufevt)
    {
        bufferevent_free(svr_cfg.bufevt);
        svr_cfg.bufevt = NULL;
    }

    if(svr_cfg.fd>0)
    {
        close(svr_cfg.fd);
        svr_cfg.fd = -1;
    }

    int addrlen=0;
	struct sockaddr * paddr = make_sock_addr(svr_cfg.server_ip.c_str(),svr_cfg.server_port, addrlen);
	int fd = socket( AF_INET, SOCK_STREAM, 0);
	struct bufferevent * bufevt = bufferevent_socket_new(g_event_base, fd, 0);

    svr_cfg.fd = fd;
    svr_cfg.bufevt = bufevt;

    bufferevent_setcb(bufevt, BuffereventReadCallback, NULL, BuffereventEventCallback, NULL);
	bufferevent_enable(bufevt, EV_READ);
	bufferevent_socket_connect(bufevt, paddr, addrlen);
}

void BuffereventEventCallback(struct bufferevent *bev,short what ,void *ctx)
{
    log_error("server event, %d", what);
    UNUSED(bev);
    UNUSED(ctx);

    if(what & BEV_EVENT_CONNECTED)
	{
		log_warning("sucess Connect to connector, begin to login!,server_ip:%s, server_port:%hd", svr_cfg.server_ip.c_str(), svr_cfg.server_port );
        login();
		return;
	}

	if ((what & BEV_EVENT_EOF) || (what & BEV_EVENT_ERROR))
   	 {

		    log_warning("lost connection to connector , reconnect it ....,server_ip: %s, server_port: %hd", svr_cfg.server_ip.c_str(),svr_cfg.server_port );
            sleep(1);
       		Reconnect();
   	 }

	return;
}

void login()
{
    ClientMsgHead mh;
    LoginRequest req;

    MessageProcess::Instance()->fill_message_head(&mh, "", CMT_LOGIN_REQ);
    req.set_imsi("123huoiadhaijga12316145aaj");
    req.set_time(time(NULL));

    MessageProcess::Instance()->SendMessageToServer(&mh, &req) ;
}

void SendHeart()
{
    static time_t last_heart_time = 0;

    if(g_error_times>=3 && svr_cfg.bufevt)
    {
        log_warning("error times reached maxtimes, so close socket");
        close(bufferevent_getfd(svr_cfg.bufevt));
        g_error_times = 0;
        Reconnect();
    }

    if(time(NULL) - last_heart_time < 10)
        return;

    last_heart_time = time(NULL);

    ClientMsgHead mh;
    HeartbeatMsg req;
    req.set_time(time(NULL));
    MessageProcess::Instance()->fill_message_head(&mh, "", CMT_HEATBEAT);

    MessageProcess::Instance()->SendMessageToServer(&mh, &req) ;
    // g_error_times++;
}



void InitArgv(int argc, char** argv)
{
    const char* option = "s:p:";
    int result = 0;

    while( (result=getopt(argc, argv,option))!=-1 )
	{
		if(result == 's') //服务器ip
		{
            svr_cfg.server_ip = optarg;
		}
		else if(result == 'p') //服务器端口
		{
            svr_cfg.server_port = atoi(optarg);
		}
    }

    if(svr_cfg.server_ip=="" || svr_cfg.server_port<=0)
    {
        fprintf(stderr, "useage: ./imclient -s 127.0.0.1 -p 1116 \n");
        exit(1);
    }

    set_log_type(svr_cfg.log_type);
    init_log("pushclient",svr_cfg.log_dir.c_str());
    set_log_level( svr_cfg.log_level);
}


void check_login()
{
    static time_t last_chk_time = 0;

    if(time(NULL) - last_chk_time < 15 )
    {
        return;
    }

    last_chk_time = time(NULL);
}


int main(int argc, char** argv)
{
   InitArgv(argc, argv);
    g_event_base = event_base_new();
    MessageProcess::Instance()->InitMessageIdMap();
    Reconnect();

    while (1)
    {
        event_base_loop(g_event_base, EVLOOP_NONBLOCK);
        usleep(1000); //提高了延迟
   //     check_login();
        SendHeart();

    }

	return 0;

}


