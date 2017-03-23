#pragma once

#include "connect_center.h"
#include "common_singleton.h"
#include <vector>
#include <map>

class CenterMsgProcess : public ConnectCenterMsgProcessor
{
public:
    virtual int ProcessClose(ClientInfo*);

    virtual void AddConnectorAddr(int , const SvrAddress& );

private:
    std::map<int, std::vector<SvrAddress> > m_connector_addr;
};

#define CENTERMSGPROINST Singleton<CenterMsgProcess>::Instance()
