#include <set>
#include <vector>
#include "event2/listener.h"
#include "log4cpp_log.h"
#include "comm_config.h"
#include "comm_datetime.h"
#include "base.h"
#include "comm_server.h"
#include "client.h"
#include "player.h"
#include "connector_message_processor.h"
#include "connect_center.h"

CPlayer* CPlayerFrame::CreatePlayer(std::string const& client_id, int client_fd)
{
    if(client_id.empty() || client_fd <=0)
    {
        log_warning("client_id or client_fd can not less than 0! %s, %d", client_id.c_str(), client_fd);
        return NULL;
    }

    std::map<std::string, CPlayer>::iterator it = map_player_info_.find(client_id);
	if(it != map_player_info_.end() )
        return &it->second;

	CPlayer one_player ;
	one_player.SetClientId(client_id);
    one_player.SetFd(client_fd);
    map_player_info_[client_id] = one_player;

    return &map_player_info_[client_id];
}

int CPlayerFrame::DeletePlayer(std::string const& client_id)
{
    if(client_id.empty())
    {
        log_warning("client_id empty");
        return -1;
    }

    std::map<std::string, CPlayer>::iterator it = map_player_info_.find(client_id);
    if (it == map_player_info_.end())
    {
        log_warning("client_id:%s cant find in map_player_info_", client_id.c_str());
        return -1;
    }

    log_warning("client id:%s delete player", client_id.c_str());
	map_player_info_.erase(it);
	return 0;
}

CPlayer* CPlayerFrame::GetPlayer(std::string const& client_id)
{
    if(client_id.empty())
    {
        log_warning("client_id empty");
        return NULL;
    }

    std::map<std::string, CPlayer>::iterator it = map_player_info_.find(client_id);
    if (it == map_player_info_.end())
    {
        log_debug("client id:%s find no player", client_id.c_str());
        return NULL;
    }

    return &it->second;
}

int CPlayerFrame::UpdateHeartBeatTime(std::string const& client_id)
{
    CPlayer* pl = GetPlayer(client_id);
    ASSET_AND_RETURN_INT(pl);

    pl->SetLastHeatbeatTime();

    return 0;
}

//删除超时的客户端
int CPlayerFrame::CloseOutoftimeClient()
{
    static time_t last_check_time = 0;
    if(time(NULL) - last_check_time < ConnectToCenter::Instance()->GetConfig().client_outoftime() )
    {
        return 0;
    }

    last_check_time = time(NULL) ;
    std::set<PushClientInfo*> s_client_del;

    std::map<std::string, CPlayer>::iterator it ;
    for(it = map_player_info_.begin();it!=map_player_info_.end();++it)
    {
        time_t last_heartbeat_time =  it->second.GetLastHeatbeatTime();

        if(last_check_time - last_heartbeat_time < ConnectToCenter::Instance()->GetConfig().client_outoftime() || 0 == last_heartbeat_time)    continue;

        int fd = it->second.GetFd();
        PushClientInfo* client_info = PushClientManage::Instance()->GetPushClientInfo(fd);
        if(!client_info)
        {
            log_warning("can not find the PushClientInfo,fd %d",fd);
            continue;
        }

        log_warning("user: %s is out of time, so delete it![last_heartbeat_time:%ld,nowtime:%ld,outtime:%d,fd:%d,client_info:%s]",
                it->first.c_str(),last_heartbeat_time,last_check_time,ConnectToCenter::Instance()->GetConfig().client_outoftime(),fd,GetPeerAddrStr(client_info->address));
        if(it->first != client_info->client_id)
        {
           log_warning("userid[%s] in player_info is not same with in client_info[%s]",it->first.c_str(),client_info->client_id.c_str());
        }
        else
        {
            s_client_del.insert(client_info);
        }
    }

    std::set<PushClientInfo*>::iterator its ;
    for(its=s_client_del.begin(); its!=s_client_del.end(); ++its)
    {
        ConnectorMessageProcessor::Instance()->ProcessClose(*its);
    }
    return 0;
}
