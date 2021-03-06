#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <event2/listener.h>
#include "log4cpp_log.h"


struct ServerAddr
{
    ServerAddr()
    {
        server_port = 0;
    }

	std::string server_name;
	std::string server_ip;
	unsigned short server_port;
};

struct Config
{
    std::string center_ip;
    unsigned short center_port;
    int svr_type;
    int svr_id;
    int listen_port;
    bool need_listen;
};

unsigned long strhash(const char *str, unsigned int length);
unsigned long get_curr_time(void);
int split_str(const char* ps_str , char* ps_sp , std::vector<std::string> &v_ret);
std::string ToHexString(const char *c, int l);
std::string GenerateClientId(std::string const& imsi, time_t t);
std::string GenerateSecretKey(std::string const& last_key, std::string const& client_id);

int ParseArg(int argc, char **argv, Config& config);

bool IsAddressListening(const char *ip, unsigned short port);
std::string GetLocalListenIp(unsigned short port, bool islan = false);
std::string GetWlanIp();

class MessageProcessor;

int InitLibevent(struct event_base* &base);
int InitListener(const char *ip, unsigned short port, struct event_base* &base, MessageProcessor* processor);
int ClientConnect(const char *ip, unsigned short port, struct event_base* &base, struct bufferevent* &bevt, int &fd, void *ctx);


