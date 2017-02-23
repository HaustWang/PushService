#include "base.h"

#include "push_proto_common.h"
#include "push_proto_server.h"
#include "work_manage.h"
#include "push_common.h"

DBWorkManage::DBWorkManage()
    : MessageProcessor()
{
	InitMessageIdMap();
}

void DBWorkManage::InitMessageIdMap()
{
}

int DBWorkManage::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, std::string const& body)
{
    client_info->ShortDebugString();

    const SvrMsgHead* head = dynamic_cast<const SvrMsgHead*>(phead);

    HANDLER_TYPE_IT it;
    it = msg_handler_map_.find(head->type());
    if(it == msg_handler_map_.end())
    {
        log_info("proxy %d transfer msg: src_svr_type:%d, src_svr_id:%d, dst_svr_type:%d, dst_svr_id:%d", GetServerId(), head->src_svr_type(), head->src_svr_id(), head->dst_svr_type(), head->dst_svr_id());
        if(!head->has_dst_svr_type() || SERVER_TYPE_MIN > head->dst_svr_type() || SERVER_TYPE_MAX <= head->dst_svr_type())
        {
            log_error("proxy %d transfer msg error cause dst_svr_type error! dst_svr_type:%d", GetServerId(), head->dst_svr_type());
            return -1;
        }

        SvrMsgHead* new_head = (SvrMsgHead*)head;
        new_head->set_proxy_svr_id(GetServerId());
        ClientInfo* client_info = NULL;
        if(!head->has_dst_svr_id() || 0 == head->dst_svr_id())
        {
            if(head->is_broadcast())
            {
                std::vector<ClientInfo*> client_vec;
                ClientManage::Instance()->GetClientInfoList(head->dst_svr_type(), client_vec);
                if(client_vec.empty())
                {
                    log_error("proxy %d the msg dst svr list is empty, head:%s", GetServerId(), head->ShortDebugString().c_str());
                    return -1;
                }

                for(int i = 0; i < client_vec.size(); ++i)
                {
                    SendMessageToServer(client_vec[i], new_head, body);
                }

                return 0;
            }
            else
                client_info = ClientManage::Instance()->GetClientInfoRandom(head->dst_svr_type());
        }
        else
        {
            client_info = ClientManage::Instance()->GetClientInfoById(head->dst_svr_id(), head->dst_svr_type());
        }

        if(NULL == client_info)
        {
            log_error("proxy %d transfer msg error cause dst_svr_id error! dst_svr_id:%d", GetServerId(), head->dst_svr_id());
            return -1;
        }

        SendMessageToServer(client_info, new_head, body);
    }
    else
    {
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
    }
    return 0;
}

int DBWorkManage::ProcessClose(ClientInfo* client_info)
{
    if(NULL == client_info)
    {
        log_error("client_info is null");
		return -1;
    }

    log_info("client close, fd:%d, svr_type:%d, svr_id:%d", client_info->fd, client_info->server_type, client_info->server_id);

    ClientManage::Instance()->DeleteClientInfo(client_info);
    return 0;
}

