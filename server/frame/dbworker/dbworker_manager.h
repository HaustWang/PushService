#pragma once

#include "redismanager.h"

#include "push_proto_common.h"
#include "push_proto_server.h"
#include "json/json.h"
#include "common_singleton.h"

#include <map>
#include <set>

typedef google::protobuf::internal::RepeatedPtrIterator<std::string const> Iter;

class DbWorkerMgr
{
public:
    DbWorkerMgr();
    virtual ~DbWorkerMgr();
    int Init();
    int LoadDataFromRedis();

    int UpdateUserStatus(std::string const& client_id, bool is_online, int32_t connector_id);

    int InsertMsgToRedis(uint32_t msgid, Iter begin, Iter end, std::string const& msg, int expire_time);
    void QueryMsgFromRedis(std::string const& client_id);
    int UserReceivedMsg(std::string const& client_id, int64_t msgid);

    bool IsClientOnline(std::string const& client_id) { return m_client_connetor_map.end() != m_client_connetor_map.find(client_id); }
    int32_t GetClientConnectorId(std::string const& client_id) { return m_client_connetor_map[client_id]; }

    void CheckMsg();
private:
    RedisManager m_RedisManagerMsg;

    std::map<std::string, int32_t> m_client_connetor_map;
    std::map<std::string, std::set<int64_t> > m_MsgMap;

    int64_t m_last_check_time_;
};

#define DbWorkerMgrInst Singleton<DbWorkerMgr>::Instance()
