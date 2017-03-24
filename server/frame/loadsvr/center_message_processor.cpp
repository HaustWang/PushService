
#include "center_message_processor.h"
#include "push_common.h"
#include <string>

void CenterMsgProcess::InitMessageIdMap()
{
    ConnectCenterMsgProcessor::InitMessageIdMap();
	REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_SYNC_ADDRESS, new SyncAddressHandler, "SvrSyncAddress" );
}

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

void CenterMsgProcess::GetAddress(std::string const& id, std::string& addr)
{
    if(m_connector_addr.empty())    return;
    auto hash_value = strhash(id.c_str(), id.length());
    auto map_idx = hash_value % m_connector_addr.size();
    auto it = m_connector_addr.begin();
    for(auto i = 0; i < map_idx; ++i, ++it);

    auto& address = it->second[hash_value % it->second.size()];
    addr = address.ip() + std::to_string(address.port());
}

int SyncAddressHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message*, const google::protobuf::Message* message)
{
    if(NULL == message || NULL == client_info)
        return -1;

    const SvrSyncAddress* resp = dynamic_cast<const SvrSyncAddress*>(message);
    for(int i = 0; i < resp->peer_addresses_size(); ++i)
    {
        CENTERMSGPROINST.AddConnectorAddr(client_info->fd, resp->peer_addresses(i));
    }
    return 0;
}
