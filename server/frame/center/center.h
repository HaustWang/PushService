#pragma once
#include <string>
#include <vector>
#include <netinet/in.h>
#include <string>
#include <set>
#include <map>
#include "event2/listener.h"
#include "json.h"

#include "comm_client.h"
#include "common_singleton.h"
#include "message_processor.h"

class CenterInfo : public ClientInfo
{
public:
    void Reconnect();

    CenterInfo();
private:
    time_t m_last_connect_time;
};

class Center : public MessageProcessor
{
public:
    int Run(int argc, char **argv);
    int ParseArgs(int argc, char **argv);
    void Usage(const char *name);

    int LoadConfig();
    int ReloadConfig();

    int InitService();

    virtual void InitMessageIdMap();
	virtual int ProcessClose(ClientInfo*);

    int SyncAddrToOther();
    int SyncConnectorAddr();

    int ProcessConfigReq(ClientInfo*, const SvrMsgHead*, const google::protobuf::Message*);
    int ProcessSyncAddr(ClientInfo*, const SvrMsgHead*, const google::protobuf::Message*);

    struct event_base*& GetEventBase() { return m_event_base; }
    Center();
    ~Center();

    void Reconnect()
    {
        for(size_t i=0; i<m_ociv.size(); i++)
        {
           m_ociv[i]->Reconnect();
        }
    }
private:

    void FillConfigResp(int svr_type, SvrConfigResp& resp);
private:
    Json::Value m_json_value;
    std::string m_log_config;

    std::map<int, std::map<int, SvrAddress> > m_server_addr; 

    unsigned short m_listen_port;
    std::string m_cfg_file;

    struct event_base *m_event_base;

	std::vector<CenterInfo*> m_ociv;

};

#define CenterInst Singleton<Center>::Instance()

class ConfigReqHandler : public MessageHandler 
{
public:
    virtual int ProcessMessage(ClientInfo*,  const google::protobuf::Message*, const google::protobuf::Message*);
};

class SyncAddrHandler : public MessageHandler 
{
public:
    virtual int ProcessMessage(ClientInfo*,  const google::protobuf::Message*, const google::protobuf::Message*);
};
