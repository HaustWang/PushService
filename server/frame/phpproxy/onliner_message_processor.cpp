#include <event2/event.h>
#include <event2/util.h>
#include <event2/bufferevent.h>
#include "onliner_message_processor.h"
#include "base.h"
#include "connect_proxy.h"
#include "http_manager.h"


extern struct event_base * g_event_base;

OnlinerMessageProcessor::OnlinerMessageProcessor()
    : ProxyMessageProcessor()
{}

void OnlinerMessageProcessor::InitMessageIdMap()
{
    ProxyMessageProcessor::InitMessageIdMap();
	REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_USER_MSG_ACK, new UserMsgAckHandler, "SvrUserMsgAck");
	REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_USER_READ_MSG, new UserReadHandler, "SvrUserReadMsg");
}



void ConnectToOnlinerMgr::init(Config const& config)
{
    ConnectProxyMgr::init(config.svr_id,config.svr_type,OnlinerMessageProcessor::Instance());
}

int ConnectToOnlinerMgr::SendPushMsgToDBWorker(int32_t msg_id, std::string const& client_id, int32_t expire_time, unsigned char *data, size_t length)
{
    SvrMsgHead mh;
    OnlinerMessageProcessor::Instance()->FillMsgHead(&mh, SMT_INSERT_MSG, SERVER_TYPE_DB_WORKER);

    SvrInsertMsg insert_msg;
    insert_msg.set_msgid(msg_id);
    insert_msg.add_client_ids(client_id);
    insert_msg.set_expire_time(expire_time);
    insert_msg.set_msg((const char *)data, length);

    return SendMessageToServer(&mh, &insert_msg);
}

int ConnectToOnlinerMgr::SendPushMsgToDBWorker(int32_t msg_id, Iter begin, Iter end, int32_t expire_time, unsigned char *data, size_t length)
{
    SvrMsgHead mh;
    OnlinerMessageProcessor::Instance()->FillMsgHead(&mh, SMT_INSERT_MSG, SERVER_TYPE_DB_WORKER);

    SvrInsertMsg insert_msg;
    insert_msg.set_msgid(msg_id);
    for(Iter it = begin; end != it; ++it)
        insert_msg.add_client_ids(it->asString());
    insert_msg.set_expire_time(expire_time);
    insert_msg.set_msg((const char *)data, length);

    return SendMessageToServer(&mh, &insert_msg);
}

int UserMsgAckHandler::ProcessMessage(ClientInfo*, const google::protobuf::Message*, const google::protobuf::Message* pmsg)
{
    if(NULL == pmsg)
    {
        log_error("info empty: pmsg:%p", pmsg);
        return -1;
    }

    const SvrUserMsgAck *msg = dynamic_cast<const SvrUserMsgAck*>(pmsg);
    if(!msg->has_client_id() || !msg->has_msgid())
    {
        log_error("User msg ack but info incomplete, client_id:%s, msg_id:%ld", msg->client_id().c_str(), msg->msgid());
        return -1;
    }

    if(msg->code() == RESULT_UNLOAD)
        return HttpManager::Instance().ReportData(msg->msgid(), msg->client_id(), REPORT_TYPE_UNLOAD);
    else
        return HttpManager::Instance().ReportData(msg->msgid(), msg->client_id(), REPORT_TYPE_RECEIVED);
}

int UserReadHandler::ProcessMessage(ClientInfo* pclient_info, const google::protobuf::Message*, const google::protobuf::Message* pmsg)
{
    if(NULL == pmsg)
    {
        log_error("info empty: pmsg:%p", pmsg);
        return -1;
    }

    const SvrUserReadMsg *msg = dynamic_cast<const SvrUserReadMsg*>(pmsg);
    if(!msg->has_client_id() || !msg->has_msgid())
    {
        log_error("User read msg but info incomplete, client_id:%s, msg_id:%ld", msg->client_id().c_str(), msg->msgid());
        return -1;
    }

    return HttpManager::Instance().ReportData(msg->msgid(), msg->client_id(), REPORT_TYPE_READ);
}
