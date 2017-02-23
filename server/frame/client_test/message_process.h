#ifndef __MESSAGE_PROCESS_H__
#define __MESSAGE_PROCESS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "push_proto_common.h"
#include "push_proto_client.h"
#include "client.h"

class MessageProcess;
typedef int(MessageProcess::*CallFunc) (const ClientMsgHead*, google::protobuf::Message*);

class MessageProcess
{
public:
    static MessageProcess* Instance()
    {
        static MessageProcess *_instance = NULL;
        if(!_instance)
        {
            _instance = new MessageProcess;
        }
        return _instance;
    }
    virtual ~MessageProcess();
	 MessageProcess();

    unsigned long get_curr_time(void)
    {
        struct timeval tv;
        unsigned long curr_time = 0;

        gettimeofday(&tv, NULL);

        curr_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        return curr_time;
    }

	std::map<ClientMsgType, std::pair<std::string, CallFunc> >  message_id_map_;
public:
	int GetCompletePackage(struct bufferevent* bev, char* pkg, int* len);
	int InitMessageIdMap();
	google::protobuf::Message* CreateMessage(const std::string& type_name, const std::string& message_body);
    int ProcessMessage(const char *buf, const int len);
	int ProcessMessage( const ClientMsgHead*, const std::string& message_body);
	int SendMessageToServer( ClientMsgHead*, google::protobuf::Message*);
	int fill_message_head(ClientMsgHead* head, const std::string& client_id,const ClientMsgType type)
	{
		head->set_client_id(client_id);
        head->set_type(type);

		return 0;
	}

public:
	int ProcessLoginResponse(   const ClientMsgHead*, google::protobuf::Message*);
	int ProcessClientRegResponse(   const ClientMsgHead*, google::protobuf::Message*);
	int ProcessSvrPushMessage(   const ClientMsgHead*, google::protobuf::Message*);
	int ProcessHeartbeat(   const ClientMsgHead*, google::protobuf::Message*);

    std::string true_key;
};

#endif
