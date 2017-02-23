#include "client.h"
#include "base.h"


int PushClientManage::DeletePushClientInfo(PushClientInfo* client_info,bool delay_delete)
{
    ASSET_AND_RETURN_INT(client_info);
    map_client_info_.erase(client_info->fd);
    if(delay_delete)
    {   client_info->need_delete = true;
    }
    else
    {
        SAFE_DELETE(client_info);
    }

    return 0;
}

PushClientInfo* PushClientManage::CreatePushClientInfo(int fd, struct bufferevent* bev, struct sockaddr* addr)
{
    PushClientInfo *client_info = new PushClientInfo;
    ASSET_AND_RETURN_PTR(client_info);
    log_debug("fd:%d new client info:%p", fd, client_info);
    memcpy(&client_info->address, addr, sizeof(client_info->address));
    client_info->bevt = bev;
    client_info->fd = fd;
    map_client_info_[fd] = client_info;

    return client_info;
}

PushClientInfo* PushClientManage::GetPushClientInfo(int fd)
{
    std::map<int, PushClientInfo*>::iterator it = map_client_info_.find(fd);
    if (it == map_client_info_.end())
    {
        return NULL;
    }

    return it->second;
}
