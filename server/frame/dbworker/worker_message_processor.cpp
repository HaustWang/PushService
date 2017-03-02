#include <set>
#include "log4cpp_log.h"

#include "common_singleton.h"
#include "base.h"
#include "worker_message_processor.h"
#include "dbworker_manager.h"

WorkerMessageProcessor::WorkerMessageProcessor()
    : ProxyMessageProcessor()
{
    InitMessageIdMap();
}

WorkerMessageProcessor::~WorkerMessageProcessor()
{
}

void WorkerMessageProcessor::InitMessageIdMap()
{
    ProxyMessageProcessor::InitMessageIdMap();
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_UPDATE_USER, new UserUpdateStatusHandler, "SvrUpdateUser");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_INSERT_MSG, new SvrInsertMsgHandler, "SvrInsertMsg");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_USER_MSG_ACK, new SvrUserMsgAckHandler, "SvrUserMsgAck");
}

int WorkerMessageProcessor::SendTransferMsg(std::string const& client_id, uint32_t msgid, int32_t connector_id, std::string const& msg)
{
    SvrMsgHead mh;
    FillMsgHead(&mh, SMT_TRANSFER_MSG, SERVER_TYPE_CONNECTOR);
    mh.set_dst_svr_id(connector_id);

    SvrTransferMsg trans_msg;
    trans_msg.set_msgid(msgid);
    trans_msg.set_content(msg);
    trans_msg.add_client_ids(client_id);

    return ProxyMgr.SendMessageToServer(&mh, &trans_msg);
}

int UserUpdateStatusHandler::ProcessMessage(ClientInfo*, const google::protobuf::Message* phead, const google::protobuf::Message* pmsg)
{
    if(NULL == phead || NULL == pmsg)
        return -1;

    const SvrUpdateUser* msg = dynamic_cast<const SvrUpdateUser*>(pmsg);

    DbWorkerMgrInst.UpdateUserStatus(msg->client_id(), msg->is_online(), msg->connector_id());

    if(msg->is_online())
        DbWorkerMgrInst.QueryMsgFromRedis(msg->client_id()); 

    return 0;
}

int SvrInsertMsgHandler::ProcessMessage(ClientInfo*, const google::protobuf::Message* phead, const google::protobuf::Message* pmsg)
{
    if(NULL == phead || NULL == pmsg)
        return -1;

    const SvrInsertMsg* msg = dynamic_cast<const SvrInsertMsg*>(pmsg);

    std::map<int, SvrMsgHead> head_map;
    std::map<int, SvrTransferMsg> msg_map;
    for(int i = 0; i < msg->client_ids_size(); ++i)
    {
        std::string const& cli_id = msg->client_ids(i);
        if(!DbWorkerMgrInst.IsClientOnline(cli_id)) continue;

        int conn_id = DbWorkerMgrInst.GetClientConnectorId(cli_id);
        if(head_map.end() != head_map.find(conn_id))
        {
            msg_map[conn_id].add_client_ids(cli_id);
            continue;
        }

        SvrMsgHead& mh = head_map[conn_id];
        MessageProcessorInst.FillMsgHead(&mh, SMT_TRANSFER_MSG, SERVER_TYPE_CONNECTOR);
        mh.set_dst_svr_id(conn_id);

        SvrTransferMsg& trans_msg = msg_map[conn_id];
        trans_msg.set_msgid(msg->msgid());
        trans_msg.set_content(msg->msg());
        trans_msg.add_client_ids(cli_id);
    }

    for(std::map<int, SvrMsgHead>::iterator it = head_map.begin(); it != head_map.end(); ++it)
        ProxyMgr.SendMessageToServer(&it->second, &msg_map[it->first]);

    if(0 <= msg->expire_time())
        DbWorkerMgrInst.InsertMsgToRedis(msg->msgid(), msg->client_ids().begin(), msg->client_ids().end(), msg->msg(), msg->expire_time());

    return 0;
}

int SvrUserMsgAckHandler::ProcessMessage(ClientInfo* , const google::protobuf::Message* , const google::protobuf::Message* pmsg)
{
    if(NULL == pmsg)
        return -1;

    SvrMsgHead mh;
    MessageProcessorInst.FillMsgHead(&mh, SMT_USER_MSG_ACK, SERVER_TYPE_PHP_PROXY);

    ProxyMgr.SendMessageToServer(&mh, (google::protobuf::Message*)pmsg);

    const SvrUserMsgAck* msg = dynamic_cast<const SvrUserMsgAck*>(pmsg);
    return  DbWorkerMgrInst.UserReceivedMsg(msg->client_id(), msg->msgid());
}



