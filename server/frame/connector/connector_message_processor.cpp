#include "player.h"
#include "base.h"
#include "log4cpp_log.h"
#include "onliner_message_processor.h"
#include "connector_message_processor.h"
#include "base64.h"
#include "connect_proxy.h"
#include "crypto_aes.h"


class SendHeartbeatRequest;

ConnectorMessageProcessor::ConnectorMessageProcessor()
    : MessageProcessor()
{
}

ConnectorMessageProcessor::~ConnectorMessageProcessor()
{
}

void ConnectorMessageProcessor::InitMessageIdMap()
{
    //注册消息处理函数
    REGIST_MESSAGE_PROCESS(msg_handler_map_, CMT_LOGIN_REQ, new LoginHandler, "LoginRequest");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, CMT_PUSH_ACK, new PushAckHandler, "ClientPushAck");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, CMT_HEATBEAT, new HeartbeatHandler, "");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, CMT_UPDATE_READ, new UserReadHandler, "ClientUpdateRead")
}

int ConnectorMessageProcessor::ProcessClose(ClientInfo* pclient_info)
{
	if(!pclient_info)
	{
		log_error("client_info is null");
		return -1;
	}

    PushClientInfo* client_info = dynamic_cast<PushClientInfo*>(pclient_info);
	log_debug("user: %s logout, PushClientInfo fd: %d", client_info->client_id.c_str(),client_info->fd);


    CPlayer* player = CPlayerFrame::Instance()->GetPlayer(client_info->client_id);
    if(NULL == player)
    {
        log_warning("can not find the userid: %s", client_info->client_id.c_str());
	    return PushClientManage::Instance()->DeletePushClientInfo(client_info);
    }

    //通知onliner下线状态
    ConnectToOnlinerMgr::Instance()->UpdateUserStatus(client_info->client_id, false);

    if(player->GetFd() != client_info->fd)
    {
        log_warning("userid in client_info is not same with player, delete client_info");
    	return PushClientManage::Instance()->DeletePushClientInfo(client_info);
    }

	CPlayerFrame::Instance()->DeletePlayer(client_info->client_id);

	return PushClientManage::Instance()->DeletePushClientInfo(client_info);
}

int ConnectorMessageProcessor::SendMessageToClient(std::string const& client_id, int64_t msgid, std::string const& content)
{
    if(client_id.empty())   return 0;

    CPlayer* player = CPlayerFrame::Instance()->GetPlayer(client_id);
    if(NULL == player)
    {
        log_info("client %s is not online", client_id.c_str());
        return 0;
    }

    PushClientInfo* client_info = PushClientManage::Instance()->GetPushClientInfo(player->GetFd());
    if(NULL == client_info)
    {
        log_error("client %s exist but client info not exist, client_fd:%d", client_id.c_str(), player->GetFd());
        return -1;
    }

    ClientMsgHead mh;
    FillMsgHead(&mh, client_id, CMT_PUSH_MSG);

    SvrPushMessage msg;
    msg.set_msgid(msgid);
    msg.set_msg(content);

    return SendMessageToClient(client_info, &mh, &msg);
}

int ConnectorMessageProcessor::SendMessageToAllClient(int64_t msgid, std::string const& content)
{
    ClientMsgHead mh;
    SvrPushMessage msg;
    msg.set_msgid(msgid);
    msg.set_msg(content);

    const std::map<std::string, CPlayer> &clients = CPlayerFrame::Instance()->GetPlayerList();
    std::map<std::string, CPlayer>::const_iterator it = clients.begin();
    for(; clients.end() != it; ++it)
    {
        const CPlayer& player = it->second;

        PushClientInfo* client_info = PushClientManage::Instance()->GetPushClientInfo(player.GetFd());
        if(NULL == client_info)
        {
            log_error("client %s exist but client info not exist, client_fd:%d", player.GetClientId().c_str(), player.GetFd());
            continue;
        }

        FillMsgHead(&mh, player.GetClientId(), CMT_PUSH_MSG);

        SendMessageToClient(client_info, &mh, &msg);
    }

    return 0;
}

int ConnectorMessageProcessor::SendMessageToClient(PushClientInfo* client_info, const ClientMsgHead* head, const google::protobuf::Message* message)
{
	if(!client_info)
	{
		log_error("client_info is null, maybe can not found the userid in connector, client_id:%s", head->client_id().c_str());
		return -1;
	}

   	PushMessage imsg;
	ClientMsgHead* ph = imsg.mutable_message_head();
    ph->CopyFrom(*head);
	std::string* message_body_str = imsg.mutable_message_body();
	message->SerializeToString(message_body_str);

    if(ph->type()!=CMT_HEATBEAT)
    	log_debug("message head:%s, message body:%s",ph->ShortDebugString().c_str(), message->ShortDebugString().c_str());

	static std::string buff;
    buff.clear();
	if (!imsg.SerializeToString(&buff))
    {
        log_error("client_id:%s SerializeToString failed", client_info->client_id.c_str());
        return -10;
    }

    static std::string out_buff;
    out_buff.clear();
    if(-1 == client_info->aes.Encrypt(buff, out_buff))  return -1;

    int package_len = out_buff.length() + sizeof(int);
    package_len = htonl(package_len);

	int ret = bufferevent_write(client_info->bevt, &package_len, sizeof(int));
    if(0 != ret)
    {
        log_error("bufferevent_write length error!!! ret = %d", ret);
        return -1;
    }

    ret = bufferevent_write(client_info->bevt, out_buff.data(), out_buff.length());
    if(0 != ret)
    {
        log_error("bufferevent_write content error!!! ret = %d", ret);
        return -1;
    }

	return 0;

}

//处理登录请求
int ConnectorMessageProcessor::ProcessLoginReq( PushClientInfo* client_info, const ClientMsgHead* head, const google::protobuf::Message* message)
{
	const LoginRequest* request = dynamic_cast<const LoginRequest*>(message);

    //没必要重复登陆
    if(!client_info->client_id.empty() && head->client_id() == client_info->client_id)
    {
        log_warning("not allow login mutible times!, client_id[%s] ", client_info->client_id.c_str());
        return 0;
    }

    //客户端连接上来的client_id与原来的client_id不符，则直接断掉连接
    if(!client_info->client_id.empty()
            && (head->client_id().empty() || client_info->client_id != head->client_id()))
    {
        log_info("User change account, old client_id:%s, new client_id:%s", client_info->client_id.c_str(), head->client_id().c_str());
        ProcessClose(client_info);
        return 0;
    }

    std::string client_id(head->client_id());
    if(client_id.empty())
    {
        client_id = GenerateClientId(request->imsi(), request->time());
    }

    client_info->client_id = client_id;

	CPlayer* player = CPlayerFrame::Instance()->GetPlayer(client_id);
	if(player == NULL)
	{
        player = CPlayerFrame::Instance()->CreatePlayer(client_id, client_info->fd);
        if (player == NULL)
        {
            log_error("create player fail, user id:%s fd:%d", head->client_id().c_str(), client_info->fd);
            ProcessClose(client_info);
            PushClientManage::Instance()->DeletePushClientInfo((PushClientInfo*)client_info);
            return 0;
        }
	}

    log_info("client_id login:%s, is_new:%s", client_id.c_str(), head->client_id().empty()?"true":"false");
    static std::string last_key;
    last_key.clear();

    if (0 != ResponseClientLogin(client_info, RESULT_SUCCESS, last_key, head->client_id().empty()))
    {
        log_error("send login response failed! client_id:%s", client_id.c_str());
        ProcessClose(client_info);
        PushClientManage::Instance()->DeletePushClientInfo((PushClientInfo*)client_info);
        return -1;
    }

    client_info->aes.SetCryptKey(last_key);

    ConnectToOnlinerMgr::Instance()->UpdateUserStatus(client_info->client_id, true);
	return 0;
}

int ConnectorMessageProcessor::ResponseClientLogin(PushClientInfo* client_info, const ResultCode process_result, std::string& last_key, bool bNew)
{
	if(!client_info)
	{
		log_error("client_info is null");
		return -1;
	}

    //登陆失败， 关闭连接
    if(process_result != RESULT_SUCCESS)
    {
        PushClientManage::Instance()->DeletePushClientInfo(client_info,true);
        return -1;
    }

	LoginResponse res;
	ClientMsgHead mh;

    std::string second_key = GenerateSecretKey(first_key, client_info->client_id);
	res.set_result(process_result);
    res.set_key(second_key);

    AES tmp_aes(second_key, AES_ECB, true);
	FillMsgHead(&mh, client_info->client_id,  CMT_LOGIN_RESP);

    last_key = GenerateSecretKey(second_key, client_info->client_id);
    CipherContent content;
    content.set_true_key(last_key);
    if(bNew)
        content.set_client_id(client_info->client_id);

    static std::string tmp_string;
    tmp_string.clear();
    content.SerializeToString(&tmp_string);

    static std::string tmp_cipher;
    tmp_cipher.clear();
    tmp_aes.Encrypt(tmp_string, tmp_cipher);

    res.set_cipher(tmp_cipher);

    return SendMessageToClient(client_info, &mh,&res);
}

int ConnectorMessageProcessor::ProcessHeartbeat(PushClientInfo* client_info, const ClientMsgHead* head, const google::protobuf::Message* message)
{
    if(!client_info)
	{
		log_error("client_info is null");
		return -1;
	}

    UNUSED(head);
    UNUSED(message);
    if(!client_info->client_id.empty())
    {
        //更新心跳时间
        if( CPlayerFrame::Instance()->UpdateHeartBeatTime(client_info->client_id) <0)
        {
           log_warning("UpdateHeartBeatTime error, userid:%s, userid in head:%s, client_info:%s",
                   client_info->client_id.c_str(),head->client_id().c_str(),GetPeerAddrStr(client_info->address));
        }

        //返回客户端
        HeartbeatMsg res ;
        ClientMsgHead mh;
        FillMsgHead(&mh, client_info->client_id, CMT_HEATBEAT);

        return SendMessageToClient(client_info, &mh, &res);
    }

    return -1;
}

int LoginHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    return ConnectorMessageProcessor::Instance()->ProcessLoginReq((PushClientInfo*)client_info, (const ClientMsgHead*)phead, message);
}

int PushAckHandler::ProcessMessage(ClientInfo* pclient_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    if(!pclient_info)
	{
		log_error("client_info is null");
		return -1;
	}

    PushClientInfo* client_info = dynamic_cast<PushClientInfo*>(pclient_info);
    CPlayer* player = CPlayerFrame::Instance()->GetPlayer(client_info->client_id);
    if(NULL == player)
    {
        log_error("cant find player in player frame, client_id:%s", client_info->client_id.c_str());
        return -1;
    }

    const ClientPushAck *msg = dynamic_cast<const ClientPushAck*>(message);
    ConnectToOnlinerMgr::Instance()->UpdateUserPushAck(client_info->client_id, msg->msgid());
    return 0;
}

int UserReadHandler::ProcessMessage(ClientInfo* pclient_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    if(!pclient_info)
	{
		log_error("client_info is null");
		return -1;
	}

    PushClientInfo* client_info = dynamic_cast<PushClientInfo*>(pclient_info);
    const ClientUpdateRead *msg = dynamic_cast<const ClientUpdateRead*>(message);
    ConnectToOnlinerMgr::Instance()->UpdateUserRead(client_info->client_id, msg->msgid());
    return 0;
}

int HeartbeatHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    return ConnectorMessageProcessor::Instance()->ProcessHeartbeat((PushClientInfo*)client_info, (const ClientMsgHead*)phead, message);
}
