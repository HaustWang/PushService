#include "message_processor.h"

MessageProcessor::MessageProcessor()
    : m_SvrType(0)
      , m_SvrId(0)
{
}

MessageProcessor::~MessageProcessor()
{
}

void MessageProcessor::SetSvrInfo(int svr_type, int svr_id)
{
    m_SvrType = svr_type;
    m_SvrId = svr_id;
}

int MessageProcessor::GetCompletePackage(struct bufferevent *bev, char* pkg, int* len)
{
    int32_t pkg_len = 0;

    if(evbuffer_copyout(bev->input, &pkg_len, sizeof(pkg_len)) != sizeof(pkg_len))
    {
        //不足四个字节，需要继续收包
        return 1;
    }

    //转换字节序
    pkg_len = htonl(pkg_len);
    log_debug("pkg len : %d", pkg_len);

    if(pkg_len > *len || pkg_len <= (int)sizeof(int))
    {
        log_warning("pkg len :%d is invalid", pkg_len);
        evbuffer_drain(bev->input, pkg_len);
        return -1;
    }

    if(evbuffer_get_length(bev->input) < (size_t)pkg_len)
    {
        //不足一个完整的包，需要继续收包
        return 1;
    }

    *len = pkg_len;

    //拷贝一个完整的数据包
    evbuffer_remove(bev->input, pkg, pkg_len);
    return 0;
}

google::protobuf::Message* MessageProcessor::CreateMessage(const std::string& type_name, const std::string& message_body)
{
    if(type_name.empty() || message_body.empty())   return NULL;

    google::protobuf::Message* pmsg = NULL;
    const google::protobuf::Descriptor* descriptor =
        google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
    if (descriptor)
    {
        const google::protobuf::Message* prototype =
            google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
        if (prototype)
        {
            pmsg = prototype->New();
            pmsg->ParseFromString(message_body);
        }
    }

	return pmsg;
}

void MessageProcessor::DestroyMessage(google::protobuf::Message* pmsg)
{
    if(NULL != pmsg)
        delete pmsg;
}

int MessageProcessor::FillMsgHead(SvrMsgHead* head, const SvrMsgType type, const int dst_svr_type, const SvrMsgHead* src_head, const std::string& client_id, const std::string& appid)
{
    head->set_type(type);
    head->set_src_svr_type(m_SvrType);
    head->set_src_svr_id(m_SvrId);

    if(NULL == src_head)
    {
        head->set_dst_svr_type(dst_svr_type);
    }
    else
    {
        head->set_dst_svr_type(src_head->src_svr_type());
        head->set_dst_svr_id(src_head->src_svr_id());
        head->set_proxy_svr_id(src_head->proxy_svr_id());
    }

    if(!client_id.empty())
        head->set_client_id(client_id);

    if(!appid.empty())
        head->set_appid(appid);

    return 0;
}

int MessageProcessor::FillMsgHead(ClientMsgHead* head, const std::string& deviceId, const ClientMsgType type)
{
    head->set_client_id(deviceId);
    head->set_type(type);

    return 0;
}

int MessageProcessor::SendMessageToServer(const ClientInfo* client_info,const SvrMsgHead* head, const google::protobuf::Message* message)
{
    if(!client_info)
    {
        log_error("client_info is null");
        return -1;
    }

    client_info->ShortDebugString();
	log_debug("message head:%s, message body:%s",head->ShortDebugString().c_str(), message->ShortDebugString().c_str());

    std::string message_body_str;
	message->SerializeToString(&message_body_str);

    return SendMessageToServer(client_info, head, message_body_str);
}

int MessageProcessor::SendMessageToServer(const ClientInfo* client_info,const SvrMsgHead* head, const std::string& message)
{
    if(!client_info)
    {
        log_error("client_info is null");
        return -1;
    }

    client_info->ShortDebugString();
	log_debug("message head:%s",head->ShortDebugString().c_str());

    if( client_info->fd <=0 || client_info->bevt==NULL)
    {
        log_error(" lost connection to server , Reconnect it!  ");
        ((ClientInfo*)client_info)->Reconnect();
    }

	static char buff[1024*1024];
	SvrMsg imsg;
	SvrMsgHead* ph = imsg.mutable_head();
	*ph = *head;
    imsg.set_body(message);

	if (!imsg.SerializeToArray(buff + sizeof(int), sizeof(buff) - sizeof(int)))
    {
        return -10;
    }

    int package_len = imsg.GetCachedSize() + sizeof(int);
    *(int *)buff = htonl(package_len);

    ASSET_AND_RETURN_INT(client_info->bevt);
	bufferevent_write(client_info->bevt, buff, package_len);
	return 0;
}

int MessageProcessor::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, std::string const& body)
{
    client_info->ShortDebugString();

    const SvrMsgHead* head = dynamic_cast<const SvrMsgHead*>(phead);

    HANDLER_TYPE_IT it;
    it = msg_handler_map_.find(head->type());
    if(it == msg_handler_map_.end())
    {
        log_warning("message id %d not register!", head->type() );
        return -1;
    }

    google::protobuf::Message* message = CreateMessage(it->second.second,body);
    if(!message)
    {
        log_error("can not create message body , message head: %s", head->ShortDebugString().c_str());
        return -1;
    }

    log_debug("message head:%s, message body:%s",head->ShortDebugString().c_str(), message->ShortDebugString().c_str());
    int ret = it->second.first->ProcessMessage(client_info, head, message);
    if (ret < 0)
    {
        log_warning("process :%d failed,client_id:%s, appid:%s", head->type(), head->client_id().c_str(), head->appid().c_str() );
    }

    DestroyMessage(message);
    return 0;

}


