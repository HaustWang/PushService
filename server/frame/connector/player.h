#ifndef __PLAYER_H__
#define __PLAYER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <vector>
#include <openssl/md5.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "push_proto_common.h"
#include "push_proto_client.h"
#include "push_proto_server.h"

class CPlayer
{
public:
    enum 
    {
        WAIT_REGIST = 0,
        REGISTED = 1,
        WAIT_UNREGIST = 2,
    };

    struct AppInfo
    {
        AppInfo()
        {
        }

        AppInfo(std::string const& appid, std::string const& appname, int32_t mid)
        {
            this->appid.assign(appid);
            this->appname.assign(appname);
            this->mid = mid;
            status = WAIT_REGIST;
        }

        bool operator==(const AppInfo& rh) const
        {
            return this->appid == rh.appid;
        }

        bool operator==(const std::string& rh) const
        {
            return this->appid == rh;
        }

        std::string appid;
        std::string appname;
        int32_t mid;
        int status;
    };

    CPlayer()
    {
        m_client_fd = -1;
        m_last_heartbeat_time = 0;
    }

    void SetFd(int fd)
    {
        m_client_fd = fd;
    }

    int GetFd()const
    {
        return m_client_fd;
    }

    void SetLastHeatbeatTime()
    {
        m_last_heartbeat_time = time(NULL);
    }

    time_t GetLastHeatbeatTime() const
    {
        return m_last_heartbeat_time;
    }

    void SetClientId(std::string const& client_id)
    {
        this->m_client_id = client_id;
    }

    const std::string & GetClientId() const
    {
        return m_client_id;
    }

    int AddAppInfo(std::string const& appid, std::string const& appname, int32_t mid)
    {
        std::vector<AppInfo>::iterator it = std::find(m_apps.begin(), m_apps.end(), appid);
        if(m_apps.end() != it)
        {
            if(appname != it->appname)
                return -1;

            it->mid = mid;
            it->status = WAIT_REGIST;
            return 0;
        }

        m_apps.push_back(AppInfo(appid, appname, mid));
        return 0;
    }

    void SetAppInfoStatus(std::string const& appid, int status)
    {
        std::vector<AppInfo>::iterator it = std::find(m_apps.begin(), m_apps.end(), appid);
        if(m_apps.end() == it)  return;

        it->status = status;
    }

    void UpdateAppInfo()
    {
        for(std::vector<AppInfo>::iterator it = m_apps.begin(); m_apps.end() != it; )
        {
            if(it->status == WAIT_UNREGIST)
            {
                it = m_apps.erase(it);
                continue;
            }

            if(it->status == WAIT_REGIST)
                it->status = REGISTED;

            ++it;
        }
    }

    std::string GetAppName(std::string const appid)
    {
        std::vector<AppInfo>::iterator it = std::find(m_apps.begin(), m_apps.end(), appid);
        if(m_apps.end() != it)
            return it->appname;

        return "";
    }

    std::vector<AppInfo> const& GetAppList() const
    {
        return m_apps;
    }

private:
    int m_client_fd;
    time_t m_last_heartbeat_time; //上一次收到心跳包的时间
    std::string m_client_id;   //client_id;
    std::vector<AppInfo> m_apps;
};

//客户端管理类
class CPlayerFrame
{
public:
    static CPlayerFrame* Instance()
    {
        static CPlayerFrame*_instance = NULL;
        if(!_instance)
        {
            _instance = new CPlayerFrame;
        }
        return _instance;
    }

    ~CPlayerFrame()
    {
    }

    CPlayer* CreatePlayer(std::string const& client_id, int client_fd);
    int DeletePlayer(std::string const& client_id);

    int GetPlayerCnt(void)
    {
        return map_player_info_.size();
    }

    CPlayer* GetPlayer(std::string const& client_id);

    int UpdateHeartBeatTime(std::string const& client_id);
    int CloseOutoftimeClient();

    std::map<std::string, CPlayer> const& GetPlayerList() const
    {
        return map_player_info_;
    }
private:
    CPlayerFrame()
    {
        map_player_info_.clear();
    }
    std::map<std::string, CPlayer> map_player_info_;
};

#endif
