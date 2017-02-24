#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>

#include "connector_message_processor.h"
#include "onliner_message_processor.h"
#include "player.h"
#include "base.h"

extern struct event_base * g_event_base;

OnlinerMessageProcessor::OnlinerMessageProcessor()
    : ProxyMessageProcessor()
{
}

void OnlinerMessageProcessor::InitMessageIdMap()
{
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_TRANSFER_MSG,  new TransMsgHandler, "SvrTransferMsg");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_KICK_USER, new KickUserHandler, "SvrKickUser");
}

int TransMsgHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    if(NULL == client_info || NULL == phead || NULL == message)
        return -1;

    const SvrMsgHead* head = dynamic_cast<const SvrMsgHead*>(phead);
    if(head->client_id().empty())
    {
        log_error("transfer message but client_id empty");
        return -1;
    }

    const SvrTransferMsg* msg = dynamic_cast<const SvrTransferMsg*>(message);
    log_info("Transfer msg:svr_id:%d, client_id:%s, content:%s", client_info->server_id, head->client_id().c_str(), msg->content().c_str());
    if(!head->client_id().empty())
    {
        ConnectorMessageProcessor::Instance()->SendMessageToClient(head->client_id(), msg->msgid(), msg->content());
    }
    else
    {
        ConnectorMessageProcessor::Instance()->SendMessageToAllClient(msg->msgid(), msg->content());
    }

    return 0;
}

int KickUserHandler::ProcessMessage(ClientInfo*, const google::protobuf::Message* phead, const google::protobuf::Message*)
{
    if(NULL == phead)
        return -1;

    const SvrMsgHead* head = dynamic_cast<const SvrMsgHead*>(phead);
    if(head->client_id().empty())   return 0;

    ConnectorMessageProcessor::Instance()->ProcessClose(PushClientManage::Instance()->GetPushClientInfoById(head->client_id()));
    return 0;
}

void ConnectToOnlinerMgr::init(Config const& config)
{
    ConnectProxyMgr::init(config.svr_id,config.svr_type,OnlinerMessageProcessor::Instance());
}

void ConnectToOnlinerMgr::UpdateUserStatus(std::string const& client_id, bool is_online)
{
    SvrMsgHead head;
    ConnectorMessageProcessor::Instance()->FillMsgHead(&head, SMT_UPDATE_USER, SERVER_TYPE_DB_WORKER, NULL, client_id);

    SvrUpdateUser msg;
    msg.set_connector_id(ServerId());
    msg.set_is_online(is_online);

    SendMessageToServer(&head, (google::protobuf::Message*)&msg);
}

void ConnectToOnlinerMgr::UpdateUserPushAck(std::string const& client_id, int64_t msg_id)
{
    SvrMsgHead head;
    ConnectorMessageProcessor::Instance()->FillMsgHead(&head, SMT_USER_MSG_ACK, SERVER_TYPE_DB_WORKER, NULL, client_id);

    SvrUserMsgAck msg;
    msg.set_msgid(msg_id);

    SendMessageToServer(&head, (google::protobuf::Message*)&msg);
}


void ConnectToOnlinerMgr::UpdateUserRead(std::string const& client_id, int64_t msg_id)
{
    SvrMsgHead head;
    ConnectorMessageProcessor::Instance()->FillMsgHead(&head, SMT_USER_READ_MSG, SERVER_TYPE_PHP_PROXY, NULL, client_id);

    SvrUserReadMsg msg;
    msg.set_msgid(msg_id);

    SendMessageToServer(&head, (google::protobuf::Message*)&msg);
}


