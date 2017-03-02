
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "comm_config.h"
#include "log4cpp_log.h"

#include "dbworker_manager.h"
#include "json_protobuf.h"
#include "base.h"
#include "push_common.h"
#include "connect_center.h"
#include "worker_message_processor.h"

#define FIELD_MSG_NAME "msg"
#define MSG_SET_KEY_PREFIX "set_"
#define CLI_CONN_PAIR_KEY "cli_conn_pair"

#define CHECK_INTER 10*60

DbWorkerMgr::DbWorkerMgr()
{
    m_last_check_time_ = time(NULL);
}

DbWorkerMgr::~DbWorkerMgr()
{
}

int DbWorkerMgr::Init()
{
    int iRet = m_RedisManagerMsg.InitClient(ConnectToCenter::Instance()->GetConfig().redis_ip(), ConnectToCenter::Instance()->GetConfig().redis_port());
    if (iRet != 0)
    {
        log_warning("Init redis message failed, ret :%d\n", iRet);
        exit(1);
    }
    log_info("Init redis message sucess");

    return LoadDataFromRedis();
}

int DbWorkerMgr::LoadDataFromRedis()
{
    std::vector<FieldInfo> infos;
    int ret = m_RedisManagerMsg.GetHashAll(CLI_CONN_PAIR_KEY, infos);
    if(RedisSuccess == ret)
    {
        for(int i = 0; i < infos.size(); ++i)
            m_client_connetor_map[infos[i].name] = atoi(infos[i].value.c_str());
    }

    std::vector<std::string> keys_list;
    ret = m_RedisManagerMsg.GetKeys(MSG_SET_KEY_PREFIX "*", keys_list);
    if(RedisSuccess != ret)
        return -1;

    for(std::vector<std::string>::iterator it = keys_list.begin(); keys_list.end() != it; ++it)
    {
        static std::vector<int64_t> msgids;
        msgids.clear();
        ret = m_RedisManagerMsg.GetSetMembers(*it, msgids);
        if(RedisSuccess == ret)
        {
            for(int i = 0; i < msgids.size(); ++i)
                m_MsgMap[*it].insert(msgids[i]);
        }
    }
    return 0;
}

int DbWorkerMgr::UpdateUserStatus(std::string const& client_id, bool is_online, int32_t connector_id)
{
    if(is_online)
    {
        m_client_connetor_map[client_id] = connector_id;
        static char connector_id_str[10];
        snprintf(connector_id_str, sizeof(connector_id_str), "%d", connector_id);
        return m_RedisManagerMsg.UpdateOneHashField(CLI_CONN_PAIR_KEY, FieldInfo(client_id, connector_id_str));
    }
    else
    {
        m_client_connetor_map.erase(client_id);
        return m_RedisManagerMsg.DelOneHashField(CLI_CONN_PAIR_KEY, client_id);
    }
}

void DbWorkerMgr::CheckMsg()
{
    int64_t now = time(NULL);
    if(now - CHECK_INTER < m_last_check_time_)  return;

    m_last_check_time_ = now;

    for(std::map<std::string, int32_t>::iterator it = m_client_connetor_map.begin(); it != m_client_connetor_map.end(); ++it)
    {
        QueryMsgFromRedis(it->first);
    }
}

int DbWorkerMgr::InsertMsgToRedis(uint32_t msgid, Iter begin, Iter end, std::string const& msg, int expire_time)
{
    if(msg.empty() || begin == end || expire_time <= 0)
        return 0;

    if(RedisSuccess != m_RedisManagerMsg.UpdateOneHashField(msgid, FieldInfo(FIELD_MSG_NAME, msg), expire_time))
    {
        log_error("insert msg error|msgid=%u|msg=%s|expire_time=%d", msgid, msg.c_str(), expire_time);
        return -1;
    }

    static std::string set_key_prefix(MSG_SET_KEY_PREFIX);
    for(Iter it = begin; end != it; ++it)
    {
        if(it->empty()) continue;
        m_MsgMap[*it].insert(msgid);
        m_RedisManagerMsg.AddSet(set_key_prefix + *it, msgid);
    }

    log_info("insert msg success|msgid=%u|msg=%s|expire_time=%d", msgid, msg.c_str(), expire_time);
    return 0;
}

void DbWorkerMgr::QueryMsgFromRedis(std::string const& client_id)
{
    if(m_MsgMap.end() == m_MsgMap.find(client_id))  return;

    FieldInfo info;
    info.name = FIELD_MSG_NAME;

    int connector_id = m_client_connetor_map[client_id];

    static std::string set_key_prefix(MSG_SET_KEY_PREFIX);
    std::set<int64_t>& msgids = m_MsgMap[client_id];
    for(std::set<int64_t>::iterator it = msgids.begin(); it != msgids.end();)
    {
        int ret = m_RedisManagerMsg.GetOneHashField(*it, info);
        if(RedisKeyNotExist == ret)
        {
            msgids.erase(it++);
            m_RedisManagerMsg.RemSet(set_key_prefix + client_id, *it);
            continue;
        }

        if(RedisSuccess == ret)
            MessageProcessorInst.SendTransferMsg(client_id, *it, connector_id, info.value);

        ++it;
    }
}

int DbWorkerMgr::UserReceivedMsg(std::string const& client_id, int64_t msgid)
{
    m_MsgMap[client_id].erase(msgid);
    if(m_MsgMap[client_id].empty())
        m_MsgMap.erase(client_id);

    return m_RedisManagerMsg.RemSet(std::string(MSG_SET_KEY_PREFIX) + client_id, msgid);
}
