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
    // log_debug("pkg len : %d", pkg_len);

    if(pkg_len > *len || pkg_len <= (int)sizeof(int))
    {
        // log_warning("pkg len :%d is invalid", pkg_len);
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

int MessageProcessor::FillMsgHead(SvrMsgHead* head, const SvrMsgType type, const int dst_svr_type, const SvrMsgHead* src_head)
{
    head->set_type(type);
    head->set_src_svr_type(m_SvrType);
    head->set_src_svr_id(m_SvrId);

    if(NULL == src_head)
    {
        head->set_dst_svr_type(dst_svr_type);
        head->set_dst_svr_id(0);
    }
    else
    {
        head->set_dst_svr_type(src_head->src_svr_type());
        head->set_dst_svr_id(src_head->src_svr_id());
        head->set_proxy_svr_id(src_head->proxy_svr_id());
    }

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
        return -1;
    }

    if(client_info->is_register)
	    log_debug("client_info:%s, message head:%s, message body:%s", client_info->ShortDebugString().c_str(), head->ShortDebugString().c_str(), message->ShortDebugString().c_str());
    else
	    printf("[%s:%d] client_info:%s, message head:%s, message body:%s\n", __FILE__, __LINE__, client_info->ShortDebugString().c_str(), head->ShortDebugString().c_str(), message->ShortDebugString().c_str());

    std::string message_body_str;
	message->SerializeToString(&message_body_str);

    return SendMessageToServer(client_info, head, message_body_str);
}

int MessageProcessor::SendMessageToServer(const ClientInfo* client_info,const SvrMsgHead* head, const std::string& message)
{
    if(!client_info)
    {
        return -1;
    }

    if(client_info->is_register)
	    log_debug("client_info:%s, message head:%s", client_info->ShortDebugString().c_str(), head->ShortDebugString().c_str());
    else
	    printf("[%s:%d] client_info:%s, message head:%s\n", __FILE__, __LINE__, client_info->ShortDebugString().c_str(), head->ShortDebugString().c_str());

    if( client_info->fd <=0 || client_info->bevt==NULL)
    {
        if(client_info->is_register)
            log_error(" lost connection to server , Reconnect it!  ");
        else
            printf("[%s:%d] lost connection to server , Reconnect it!\n", __FILE__, __LINE__);

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
    if(client_info->is_register)
        log_debug("client_info:%s", client_info->ShortDebugString().c_str());
    else
        printf("[%s:%d] client_info:%s\n", __FILE__, __LINE__, client_info->ShortDebugString().c_str());


    const SvrMsgHead* head = dynamic_cast<const SvrMsgHead*>(phead);

    HANDLER_TYPE_IT it;
    it = msg_handler_map_.find(head->type());
    if(it == msg_handler_map_.end())
    {
        if(client_info->is_register)
            log_warning("message id %d not register!", head->type() );
        else
            printf("[%s:%d] message id %d not register!\n", __FILE__, __LINE__, head->type() );

        return -1;
    }

    if(body.empty())
    {
        if(0 < it->second.first->ProcessMessage(client_info, head, NULL))
        {
            if(client_info->is_register)
                log_warning("process :%d failed", head->type());
            else
                printf("[%s:%d] process :%d failed\n", __FILE__, __LINE__, head->type());
        }
    }
    else
    {
        google::protobuf::Message* message = CreateMessage(it->second.second,body);
        if(!message)
        {
            if(client_info->is_register)
                log_error("can not create message body , message head: %s, msg_name:%s", head->ShortDebugString().c_str(), it->second.second.c_str());
            else
                printf("[%s:%d] can not create message body , message head: %s, msg_name:%s\n", __FILE__, __LINE__, head->ShortDebugString().c_str(), it->second.second.c_str());

            return -1;
        }


        if(client_info->is_register)
            log_info("message head:%s, message body:%s",head->ShortDebugString().c_str(), message->ShortDebugString().c_str());
        else
            printf("[%s:%d] message head:%s, message body:%s\n", __FILE__, __LINE__, head->ShortDebugString().c_str(), message->ShortDebugString().c_str());

        if(0 < it->second.first->ProcessMessage(client_info, head, message))
        {
            if(client_info->is_register)
                log_warning("process :%d failed", head->type());
            else
                printf("[%s:%d] process :%d failed\n", __FILE__, __LINE__, head->type());
        }

        DestroyMessage(message);
    }

    return 0;

}


