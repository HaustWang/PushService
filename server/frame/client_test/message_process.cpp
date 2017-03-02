#include "base.h"
#include "message_process.h"
#include "log4cpp_log.h"
#include "crypto_aes.h"

#define REGIST_MESSAGE_PROCESS(msg_id, message_name, process_func) \
        message_id_map_[msg_id] = std::pair<std::string, CallFunc>(message_name, &MessageProcess::process_func);

extern SvrConfigure svr_cfg;

MessageProcess::MessageProcess()
{
}

MessageProcess::~MessageProcess()
{
}

inline google::protobuf::Message* MessageProcess::CreateMessage(const std::string& type_name, const std::string& message_body)
{
        google::protobuf::Message* message = NULL;
        const google::protobuf::Descriptor* descriptor =
                google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
        if (descriptor)
        {
                const google::protobuf::Message* prototype =
                        google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
                if (prototype)
                {
                        message = prototype->New();
                        message->ParseFromString(message_body);
                }
        }

        return message;
}

int MessageProcess::InitMessageIdMap()
{
        //注册消息处理函数
        REGIST_MESSAGE_PROCESS(CMT_LOGIN_RESP, "LoginResponse", ProcessLoginResponse);
        REGIST_MESSAGE_PROCESS(CMT_PUSH_MSG, "SvrPushMessage", ProcessSvrPushMessage);
        REGIST_MESSAGE_PROCESS(CMT_HEATBEAT, "HeartbeatMsg", ProcessHeartbeat);
        return 0;
}



int MessageProcess::GetCompletePackage(struct bufferevent* bev, char* pkg, int* len)
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

int MessageProcess::ProcessMessage(const char *data, const int len)
{
    if(NULL == data || 0 > len) return -1;

    std::string msg(data, len);

    AES aes(true_key, AES_ECB, true);
    if(true_key.empty())
        aes.SetCryptKey("]u%t&1v=as^f!/i*");

    std::string out;
    if(0 > aes.Decrypt(msg, out))
    {
        log_error("Decrypt msg error, key:%s", aes.GetCryptKey().c_str());
        return -1;
    }

    static  PushMessage  push_msg;
    if (!push_msg.ParseFromString(out))
    {
        log_error("ParseFromString error!");
        return -1;;
    }
    //required废弃了， 逻辑层自己检查
    if (!push_msg.has_message_head() || !push_msg.message_head().has_type())
    {
        return -1;;
    }

    return ProcessMessage(&push_msg.message_head(), push_msg.message_body());
}

int MessageProcess::ProcessMessage( const ClientMsgHead*  head, const std::string& message_body)
{
        std::map<ClientMsgType, std::pair<std::string, CallFunc> >::iterator it = message_id_map_.find(head->type());
        if(it == message_id_map_.end())
        {
                log_warning("message id %d not register!", head->type());
                return -1;
        }

        svr_cfg.ShortDebugString();
        google::protobuf::Message* message = CreateMessage(it->second.first,message_body);
        if(!message)
        {
                log_error("can not create message body , message head: %s", head->ShortDebugString().c_str());
                return -1;
        }
        log_debug("message head:%s, message body:%s",head->ShortDebugString().c_str(), message->ShortDebugString().c_str());
        int ret = (this->*(it->second.second))( head, message);
        if (ret < 0)
        {
                log_warning("process :%d failed,client_id:%s", head->type(), head->client_id().c_str()  );
        }

        delete message;

        return 0;

}

int MessageProcess::SendMessageToServer( ClientMsgHead* head, google::protobuf::Message* message)
{
        svr_cfg.ShortDebugString();

        PushMessage imsg;
        ClientMsgHead* ph = imsg.mutable_message_head();
        ph->CopyFrom(*head);
        std::string* message_body_str = imsg.mutable_message_body();
        message->SerializeToString(message_body_str);

        std::string msg_buf;
        log_debug("message head:%s, message body:%s", ph->ShortDebugString().c_str(), message->ShortDebugString().c_str());
        if (!imsg.SerializeToString(&msg_buf))
        {
                log_error("uid:%s SerializeToArray failed", head->client_id().c_str());
                return -10;
        }

        log_info("send msg to server: msg len:%ld", msg_buf.length());

        std::string out;
        AES aes(true_key, AES_ECB, true);
        if(true_key.empty() || CMT_LOGIN_REQ == head->type())
        {
            aes.SetCryptKey("]u%t&1v=as^f!/i*");
            true_key.clear();
        }

        int len = aes.Encrypt(msg_buf, out) + sizeof(int);
        len = htonl(len);

        bufferevent_write(svr_cfg.bufevt, &len, sizeof(int));
        bufferevent_write(svr_cfg.bufevt, out.data(), out.length());
        return 0;
}

int MessageProcess::ProcessLoginResponse(const ClientMsgHead* head, google::protobuf::Message *message)
{
    LoginResponse* resp = dynamic_cast<LoginResponse*>(message);

    if(resp->result() != RESULT_SUCCESS)    return -1;

    AES aes(resp->key(), AES_ECB, true);

    std::string out_str;
    if(0 > aes.Decrypt(resp->cipher(), out_str))    return -1;

    CipherContent* cipher_content = dynamic_cast<CipherContent*>(CreateMessage("CipherContent", out_str));
    if(NULL == cipher_content)  return -1;

    true_key = cipher_content->true_key();

    log_debug("client_id:%s, true_key:%s, head_client_id:%s", cipher_content->client_id().c_str(), cipher_content->true_key().c_str(), head->client_id().c_str());

    return 0;
}

int MessageProcess::ProcessClientRegResponse(const ClientMsgHead*, google::protobuf::Message*)
{
    return 0;
}

int MessageProcess::ProcessSvrPushMessage(const  ClientMsgHead* head, google::protobuf::Message* message)
{
    SvrPushMessage* msg = dynamic_cast<SvrPushMessage*>(message);
    log_debug("client_id:%s, msg:%s", head->client_id().c_str(),msg->msg().c_str());

    ClientMsgHead mh;
    fill_message_head(&mh, head->client_id(), CMT_PUSH_ACK);

    ClientPushAck ack;
    ack.set_result(RESULT_SUCCESS);
    ack.set_msgid(msg->msgid());

    return SendMessageToServer(&mh, &ack);
}

int MessageProcess::ProcessHeartbeat(const ClientMsgHead*, google::protobuf::Message*)
{
    return 0;
}
