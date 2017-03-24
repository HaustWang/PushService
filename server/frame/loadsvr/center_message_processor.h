#pragma once

#include "connect_center.h"
#include "common_singleton.h"
#include <vector>
#include <map>

class CenterMsgProcess : public ConnectCenterMsgProcessor
{
public:
    virtual void InitMessageIdMap();

    virtual int ProcessClose(ClientInfo*);

    virtual void AddConnectorAddr(int , const SvrAddress& );

    void Init(int svr_type, int svr_id) { SetSvrInfo(svr_type, svr_id); }
    void GetAddress(std::string const& id, std::string& addr);
private:
    std::map<int, std::vector<SvrAddress> > m_connector_addr;
};

#define CENTERMSGPROINST Singleton<CenterMsgProcess>::Instance()

class SyncAddressHandler : public MessageHandler
{
public:
    virtual int ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message*);
};


