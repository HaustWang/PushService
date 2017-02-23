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
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_UPDATE_USER, new UserUpdateStatusHandler, "SvrUpdateUser");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_INSERT_MSG, new SvrInsertMsgHandler, "SvrInsertMsg");
}

int WorkerMessageProcessor::SendTransferMsg(std::string const& client_id, uint32_t msgid, int32_t connector_id, std::string const& msg)
{
    SvrMsgHead mh;
    FillMsgHead(&mh, SMT_TRANSFER_MSG, SERVER_TYPE_CONNECTOR, NULL, client_id);
    mh.set_dst_svr_id(connector_id);

    SvrTransferMsg trans_msg;
    trans_msg.set_msgid(msgid);
    trans_msg.set_content(msg);

    return ProxyMgr.SendMessageToServer(&mh, &trans_msg);
}

int UserUpdateStatusHandler::ProcessMessage(ClientInfo* pclient_info, const google::protobuf::Message* phead, const google::protobuf::Message* pmsg)
{
    if(NULL == pclient_info || NULL == phead || NULL == pmsg)
        return -1;

    const SvrMsgHead* head = dynamic_cast<const SvrMsgHead*>(phead);
    const SvrUpdateUser* msg = dynamic_cast<const SvrUpdateUser*>(pmsg);

    DbWorkerMgrInst.UpdateUserStatus(head->client_id(), msg->is_online(), msg->connector_id());

    if(msg->is_online())
        DbWorkerMgrInst.QueryMsgFromRedis(head->client_id()); 

    return 0;
}

int SvrInsertMsgHandler::ProcessMessage(ClientInfo* pclient_info, const google::protobuf::Message* phead, const google::protobuf::Message* pmsg)
{
    if(NULL == pclient_info || NULL == phead || NULL == pmsg)
        return -1;

    const SvrInsertMsg* msg = dynamic_cast<const SvrInsertMsg*>(pmsg);

    std::vector<std::string> cli_ids_vec;
    split_str(msg->client_ids().c_str(), ",", cli_ids_vec);

    for(std::vector<std::string>::iterator it = cli_ids_vec.begin(); it != cli_ids_vec.end(); ++it)
    {
        if(DbWorkerMgrInst.IsClientOnline(*it))
            MessageProcessorInst.SendTransferMsg(*it, msg->msgid(), DbWorkerMgrInst.GetClientConnectorId(*it), msg->msg());
    }

    if(0 <= msg->expire_time())
        DbWorkerMgrInst.InsertMsgToRedis(msg->msgid(), cli_ids_vec, msg->msg(), msg->expire_time());

    return 0;
}

int SvrUserMsgAckHandler::ProcessMessage(ClientInfo* pclient_info, const google::protobuf::Message* phead, const google::protobuf::Message* pmsg)
{
    if(NULL == pclient_info || NULL == phead || NULL == pmsg)
        return -1;

    const SvrMsgHead* head = dynamic_cast<const SvrMsgHead*>(phead);

    SvrMsgHead mh;
    MessageProcessorInst.FillMsgHead(&mh, SMT_USER_MSG_ACK, SERVER_TYPE_PHP_PROXY, NULL, head->client_id());

    ProxyMgr.SendMessageToServer(&mh, (google::protobuf::Message*)pmsg);

    const SvrUserMsgAck* msg = dynamic_cast<const SvrUserMsgAck*>(pmsg);
    return  DbWorkerMgrInst.UserReceivedMsg(head->client_id(), msg->msgid());
}



