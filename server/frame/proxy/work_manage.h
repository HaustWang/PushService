#pragma once

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <netinet/in.h>
#include "log4cpp_log.h"
#include "message_processor.h"


class DBWorkManage : public MessageProcessor
{
public:
    static DBWorkManage *Instance()
    {
        static DBWorkManage *instance_;
        if(NULL == instance_)
            instance_ = new DBWorkManage;

        return instance_;
    }

    virtual ~DBWorkManage() {}

    virtual void InitMessageIdMap();
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const std::string& message_body);
    virtual int ProcessClose(ClientInfo* callback_info);

private:
	DBWorkManage();
};

class RegReqHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};

