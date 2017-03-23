
#include "center_message_processor.h"

int CenterMsgProcess::ProcessClose(ClientInfo* client_info)
{
    if(NULL == client_info) return -1;

    m_connector_addr.erase(client_info->fd);
    return 0;
}

void CenterMsgProcess::AddConnectorAddr(int client_fd, const SvrAddress& addr)
{
    bool exist = false;
    std::vector<SvrAddress> addrs = m_connector_addr[client_fd];
    std::vector<SvrAddress>::iterator it = addrs.begin();
    while(addrs.end() != it)
    {
        if(addr.ip() == it->ip() && addr.port() == it->port())
        {
            exist = true;
            break;
        }

        ++it;
    }

    if(exist)   return;
    addrs.push_back(addr);
}
