#include "comm_client.h"


int ClientManage::DeleteClientInfo(ClientInfo* client_info)
{
    ASSET_AND_RETURN_INT(client_info);
    map_client_info_.erase(client_info->fd);
    SAFE_DELETE(client_info);

    return 0;
}

ClientInfo* ClientManage::CreateClientInfo(int fd, struct bufferevent* bev, struct sockaddr* addr, MessageProcessor *processor)
{
    ClientInfo *client_info = new ClientInfo;
    ASSET_AND_RETURN_PTR(client_info);
    log_debug("fd:%d new client info:%p", fd, client_info);
    memcpy(&client_info->address, addr, sizeof(client_info->address));
    client_info->bevt = bev;
    client_info->fd = fd;
    client_info->processor = processor;
    map_client_info_[fd] = client_info;

    return client_info;
}

ClientInfo* ClientManage::GetClientInfo(int fd)
{
    std::map<int, ClientInfo*>::iterator it = map_client_info_.find(fd);
    if (it == map_client_info_.end())
    {
        return NULL;
    }

    return it->second;
}

ClientInfo* ClientManage::GetClientInfoRandom(int svr_type)
{
    std::vector<ClientInfo*> client_vec;
    GetClientInfoList(svr_type, client_vec);

    if(client_vec.empty())  return NULL;

    int idx = rand() % client_vec.size();
    return client_vec[idx];
}

ClientInfo* ClientManage::GetClientInfoById(int server_id, int server_type)
{
    if (map_client_info_.size() <= 0)
    {
        return NULL;
    }

    std::map<int, ClientInfo *>::iterator it = map_client_info_.begin();
    for (; it!=map_client_info_.end(); ++it)
    {
        if (server_id == it->second->server_id && server_type == it->second->server_type)
        {
            return it->second;
        }
    }

    return NULL;
}

void ClientManage::GetClientInfoList(int server_type, std::vector<ClientInfo*>& vec)
{
    vec.clear();
    std::map<int, ClientInfo *>::iterator it = map_client_info_.begin();
    for (; it!=map_client_info_.end(); it++)
    {
        if (server_type == it->second->server_type)
        {
            vec.push_back(it->second);
        }
    }
}


