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
    ProxyMessageProcessor::InitMessageIdMap();
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_TRANSFER_MSG,  new TransMsgHandler, "SvrTransferMsg");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_KICK_USER, new KickUserHandler, "SvrKickUser");
}

int TransMsgHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* , const google::protobuf::Message* message)
{
    if(NULL == client_info || NULL == message)
        return -1;

    const SvrTransferMsg* msg = dynamic_cast<const SvrTransferMsg*>(message);
    log_info("Transfer msg:svr_id:%d, content:%s", client_info->server_id, msg->content().c_str());
    if(0 == msg->client_ids_size()) return 0;

    for(int i = 0; i < msg->client_ids_size(); ++i)
    {
        ConnectorMessageProcessor::Instance()->SendMessageToClient(msg->client_ids(i), msg->msgid(), msg->content());
    }

    return 0;
}

int KickUserHandler::ProcessMessage(ClientInfo*, const google::protobuf::Message* , const google::protobuf::Message* message)
{
    const SvrKickUser* kick = dynamic_cast<const SvrKickUser*>(message);
    ConnectorMessageProcessor::Instance()->ProcessClose(PushClientManage::Instance()->GetPushClientInfoById(kick->client_id()));
    return 0;
}

void ConnectToOnlinerMgr::init(Config const& config)
{
    ConnectProxyMgr::init(config.svr_id, config.svr_type, OnlinerMessageProcessor::Instance());
}

int ConnectToOnlinerMgr::UpdateUserStatus(std::string const& client_id, bool is_online)
{
    if(client_id.empty())   return -1;

    SvrMsgHead head;
    OnlinerMessageProcessor::Instance()->FillMsgHead(&head, SMT_UPDATE_USER, SERVER_TYPE_DB_WORKER);

    SvrUpdateUser msg;
    msg.set_connector_id(ServerId());
    msg.set_client_id(client_id);
    msg.set_is_online(is_online);

    return SendMessageToServer(&head, (google::protobuf::Message*)&msg);
}

int ConnectToOnlinerMgr::UpdateUserPushAck(std::string const& client_id, int64_t msg_id, ResultCode code)
{
    if(client_id.empty())   return -1;

    SvrMsgHead head;
    OnlinerMessageProcessor::Instance()->FillMsgHead(&head, SMT_USER_MSG_ACK, SERVER_TYPE_DB_WORKER);

    SvrUserMsgAck msg;
    msg.set_msgid(msg_id);
    msg.set_client_id(client_id);
    msg.set_code(code);

    return SendMessageToServer(&head, (google::protobuf::Message*)&msg);
}


int ConnectToOnlinerMgr::UpdateUserRead(std::string const& client_id, int64_t msg_id)
{
    if(client_id.empty())   return -1;

    SvrMsgHead head;
    OnlinerMessageProcessor::Instance()->FillMsgHead(&head, SMT_USER_READ_MSG, SERVER_TYPE_PHP_PROXY);

    SvrUserReadMsg msg;
    msg.set_msgid(msg_id);
    msg.set_client_id(client_id);

    return SendMessageToServer(&head, (google::protobuf::Message*)&msg);
}


