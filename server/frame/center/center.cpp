#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <fstream>
#include <iostream>
#include <signal.h>

#include "event2/listener.h"
#include "json_protobuf.h"
#include "log4cpp_log.h"
#include "center.h"
#include "comm_server.h"
#include "push_common.h"


#define CONFIG_PATH "../conf/config.json"
#define DEFAULT_PORT 19988
#define DEFAULT_LOG_DIR "../log/"
#define LOG_NAME "center"

extern volatile int loadcmd;
extern volatile int stop;

const static char server_name[][20] = {"center", "proxy", "connector", "phpproxy", "dbworker", "loader"};

const int SYNC_INTER_TIME = 5;

CenterInfo::CenterInfo() : ClientInfo()
{
    m_last_connect_time = 0;
}

void CenterInfo::Reconnect()
{
    time_t now_time = time(NULL);
    //超过2s重连一次
    if( now_time-m_last_connect_time>5)
        m_last_connect_time = now_time;
    else
        return ;

    if(bevt && fd>0)
        return;

	if(bevt)
	{
		bufferevent_free(bevt);
		bevt = NULL;
	}

    if(fd>0)
    {
        close(fd);
        fd = -1;
    }

    ClientConnect(remote_ip.c_str(), remote_port, CenterInst.GetEventBase(), bevt, fd, this);
}

int ConfigReqHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    return CenterInst.ProcessConfigReq(client_info, (const SvrMsgHead*)phead, message);
}

int SyncAddrHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    return CenterInst.ProcessSyncAddr(client_info, (const SvrMsgHead*)phead, message);
}

Center::Center()
    : MessageProcessor()
    , m_listen_port(0)
{
}

Center::~Center()
{
}

int Center::InitService()
{
    InitMessageIdMap();
    return InitListener("0.0.0.0", m_listen_port, m_event_base, this);
}

void Center::InitMessageIdMap()
{
    //注册消息处理函数
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_CONFIG_REQ, new ConfigReqHandler, "SvrConfigReq");
    REGIST_MESSAGE_PROCESS(msg_handler_map_, SMT_SYNC_ADDRESS, new SyncAddrHandler, "SvrSyncAddress");
}

int Center::ProcessClose(ClientInfo* client_info)
{
    log_info("close connection:%s", client_info->ShortDebugString().c_str());

    m_server_addr[GetServerId()].erase(client_info->fd);
    return ClientManage::Instance()->DeleteClientInfo((ClientInfo*)client_info);
}

int Center::ProcessConfigReq(ClientInfo* client_info, const SvrMsgHead* head, const google::protobuf::Message* message)
{
    if(NULL == client_info || NULL == head)
        return -1;

    client_info->server_type = head->src_svr_type();
    client_info->server_id = head->src_svr_id();
    client_info->is_register = true;

    if(SERVER_TYPE_CENTER == head->src_svr_type())
        return 0;

    //response get config
    SvrConfigResp resp;
    FillConfigResp(head->src_svr_type(), resp);

    SvrMsgHead msg_head;
    FillMsgHead(&msg_head, SMT_CONFIG_RESP, head->src_svr_type(), head);
    SendMessageToServer(client_info, &msg_head, &resp);

    if(head->src_svr_type() != SERVER_TYPE_PROXY || NULL == message)
      return 0;

    const SvrConfigReq* req = dynamic_cast<const SvrConfigReq*>(message);
    m_server_addr[GetServerId()][client_info->fd] = req->address();

    SvrBroadcastAddress broad_msg;
    broad_msg.add_peer_addresses()->CopyFrom(req->address());

    const std::map<int, ClientInfo*>& client_map = ClientManage::Instance()->GetClientMap();
    for(std::map<int, ClientInfo*>::const_iterator client_it = client_map.begin(); client_it != client_map.end(); ++client_it)
    {
        if(SERVER_TYPE_PROXY == client_it->second->server_type
            || SERVER_TYPE_CENTER == client_it->second->server_type)
            continue;

        FillMsgHead(&msg_head, SMT_BROADCAST_ADDR, client_it->second->server_type);
        SendMessageToServer(client_it->second, &msg_head, &broad_msg);
    }

    return 0;
}

void Center::FillConfigResp(int svr_type, SvrConfigResp& resp)
{
    resp.Clear();
    json_protobuf::update_from_json(m_json_value[server_name[svr_type]], *resp.mutable_config());
    resp.mutable_config()->set_log_config(m_log_config);

    if(SERVER_TYPE_PROXY == svr_type)
        return;

    std::map<int, std::map<int, SvrAddress> >::iterator it = m_server_addr.begin();
    for(; m_server_addr.end() != it; ++it)
    {
        std::map<int, SvrAddress>::iterator addr_it = it->second.begin();
        for(; it->second.end() != addr_it; ++addr_it)
        {
            if(SERVER_TYPE_PROXY == addr_it->second.svr_type())
                resp.add_peer_addresses()->CopyFrom(addr_it->second);
        }
    }
}

int Center::ProcessSyncAddr(ClientInfo* client_info, const SvrMsgHead* head, const google::protobuf::Message* message)
{
    if(NULL == client_info || NULL == head || NULL == message)
        return -1;

    SvrMsgHead msg_head;
    SvrBroadcastAddress broad_msg;

    std::map<int, SvrAddress>& addr_map = m_server_addr[head->src_svr_type()];
    const SvrSyncAddress* sync_addr = dynamic_cast<const SvrSyncAddress*>(message);
    for(int i = 0; i < sync_addr->peer_addresses_size(); ++i)
    {
        bool exist = false;
        const SvrAddress& addr = sync_addr->peer_addresses(i);
        for(std::map<int, SvrAddress>::iterator it = addr_map.begin(); addr_map.end() != it; ++it)
        {
            if(it->second.ip() == addr.ip() && it->second.port() == addr.port())
            {
                exist = true;
                break;
            }
        }

        if(!exist)
        {
            addr_map[addr.svr_type() << 16 | addr.svr_id()].CopyFrom(addr);
            if(SERVER_TYPE_PROXY == addr.svr_type())
                broad_msg.add_peer_addresses()->CopyFrom(addr);
        }
    }

    const std::map<int, ClientInfo*>& client_map = ClientManage::Instance()->GetClientMap();
    for(std::map<int, ClientInfo*>::const_iterator client_it = client_map.begin(); client_it != client_map.end(); ++client_it)
    {
        if(SERVER_TYPE_PROXY == client_it->second->server_type
            || SERVER_TYPE_CENTER == client_it->second->server_type
            || SERVER_TYPE_LOADER == client_it->second->server_type)
            continue;

        FillMsgHead(&msg_head, SMT_BROADCAST_ADDR, client_it->second->server_type);
        SendMessageToServer(client_it->second, &msg_head, &broad_msg);
    }

    return 0;
}

int Center::SyncAddrToOther()
{
    if(m_server_addr.empty())   return 0;

    SvrMsgHead msg_head;
    SvrSyncAddress sync_addr;

    std::map<int, std::map<int, SvrAddress> >::iterator it = m_server_addr.begin();
    for(; m_server_addr.end() != it; ++it)
    {
        std::map<int, SvrAddress>::iterator addr_it = it->second.begin();
        for(; it->second.end() != addr_it; ++addr_it)
        {
            sync_addr.add_peer_addresses()->CopyFrom(addr_it->second);
        }
    }

    if(0 == sync_addr.peer_addresses_size())    return 0;

    for(int i = 0; i < m_ociv.size(); ++i)
    {
        FillMsgHead(&msg_head, SMT_SYNC_ADDRESS, m_ociv[i]->server_type);
        SendMessageToServer(m_ociv[i], &msg_head, &sync_addr);
    }

    return 0;
}

int Center::SyncConnectorAddr()
{
    if(m_server_addr.empty())   return 0;

    SvrMsgHead msg_head;
    FillMsgHead(&msg_head, SMT_SYNC_ADDRESS, SERVER_TYPE_LOADER);

    SvrSyncAddress sync_addr;

    std::map<int, std::map<int, SvrAddress> >::iterator it = m_server_addr.begin();
    for(; m_server_addr.end() != it; ++it)
    {
        std::map<int, SvrAddress>::iterator addr_it = it->second.begin();
        for(; it->second.end() != addr_it; ++addr_it)
        {
            if(SERVER_TYPE_CONNECTOR != addr_it->second.svr_type()) continue;

            sync_addr.add_peer_addresses()->CopyFrom(addr_it->second);
        }
    }

    if(0 == sync_addr.peer_addresses_size())    return 0;
    const std::map<int, ClientInfo*>& client_map = ClientManage::Instance()->GetClientMap();
    for(std::map<int, ClientInfo*>::const_iterator client_it = client_map.begin(); client_it != client_map.end(); ++client_it)
    {
        if(SERVER_TYPE_LOADER != client_it->second->server_type)       continue;

        SendMessageToServer(client_it->second, &msg_head, &sync_addr);
    }

    return 0;
}



int Center::LoadConfig()
{
    try {
        std::ifstream iconfig(m_cfg_file.c_str());
        if(!iconfig.is_open())
        {
            std::cout<< "open config file "<< m_cfg_file << " failed" << std::endl;
            return -1;
        }

        iconfig >> m_json_value;
        iconfig.close();
    }
    catch( std::exception& e )
    {
        std::cout << "parse config file json error:" << e.what() << std::endl;
        return -1;
    }

    if(!m_json_value.isObject() || !m_json_value.isMember("center") || !m_json_value["center"].isObject())
    {
        std::cout << "config format error! content:" << m_json_value << std::endl;
        return -1;
    }

    const Json::Value &center_value = m_json_value["center"];

    //init log
    std::string log_path;
    if(center_value.isMember("log_dir") && center_value["log_dir"].isString())
    {
        log_path = center_value["log_dir"].asString();
    }
    else
    {
        log_path = DEFAULT_LOG_DIR;
    }

    int log_level = 7;
    if(center_value.isMember("log_level") && center_value["log_level"].isInt())
    {
        log_level = center_value["log_level"].asInt();
    }

    if(log_level > 7 || log_level < 1)
        log_level = 7;

    init_log(LOG_NAME, log_path.c_str());
    set_log_level(log_level);

    if(center_value.isMember("center_list") && center_value["center_list"].isArray())
    {
        for(Json::Value::iterator it = center_value["center_list"].begin(); it != center_value["center_list"].end(); ++it)
        {
            if(it->isString())
            {
                char ip[20] = {0};
                unsigned short port = 0;
                sscanf(it->asString().c_str(), "%[^:]:%hd", ip, &port);
                if(m_listen_port == port && GetLocalListenIp(m_listen_port, true) == ip)    continue;

                m_ociv.push_back(new CenterInfo());

                CenterInfo& info = *m_ociv.back();

                info.remote_ip = ip;
                info.remote_port = port;
                info.server_id = GetServerId();
                info.server_type = SERVER_TYPE_CENTER;
                info.processor = this;
            }
        }
    }

    //read log config, and log config is applied for all
    static char log_configcontent[4096];
    memset(log_configcontent, 0, sizeof(log_configcontent));

    std::ifstream ilogconfig("../conf/local_logger.properties");
    if(ilogconfig.is_open())
    {
        ilogconfig.read(log_configcontent, sizeof(log_configcontent));
        ilogconfig.close();
    }

    m_log_config.assign(log_configcontent);

    //read listen port
    if(0 == m_listen_port && center_value.isMember("port") && center_value["port"].isInt())
    {
        m_listen_port = (unsigned short)center_value["port"].asInt();
    }
    else
    {
        m_listen_port = DEFAULT_PORT;
    }

    return 0;
}

int Center::ReloadConfig()
{
    if(1 != loadcmd)
        return 0;

    LoadConfig();
    loadcmd = 0;

    SvrMsgHead msg_head;

    SvrConfig config;
    config.set_log_config(m_log_config);

    std::vector<ClientInfo*> client_vec;
    for(int i = ServerType_MIN; i <= ServerType_MAX; ++i)
    {
        ClientManage::Instance()->GetClientInfoList(i, client_vec);
        if(client_vec.empty())  continue;

        FillMsgHead(&msg_head, SMT_BROADCAST_CONFIG, i);
        json_protobuf::update_from_json(m_json_value[server_name[i]], config);
        for(std::vector<ClientInfo*>::iterator client_it = client_vec.begin(); client_it != client_vec.end(); ++client_it)
            SendMessageToServer(*client_it, &msg_head, &config);
    }

    return 0;
}

int Center::Run(int argc, char **argv)
{
    int ret = ParseArgs(argc, argv);
    if(0 != ret)
    {
        printf("parse args error!");
        Usage(argv[0]);
        return -1;
    }

    if(0 != InitService())
    {
        log_error("Init service failed!");
        return 0;
    }

    time_t start = time(NULL);
    time_t now = start;
    while (1 != stop)
    {
        event_base_loop(m_event_base, EVLOOP_NONBLOCK);

        //静默加载配置
        ReloadConfig();

        now = time(NULL);
        if(now - start > SYNC_INTER_TIME)
        {
            SyncAddrToOther();
            SyncConnectorAddr();
            start = now;
        }

        usleep(1000);
    }

    return 0;
}

int Center::ParseArgs(int argc, char **argv)
{
    const char* option = "c:p:s:h";
    int result = 0;
    while((result = getopt(argc, argv, option)) != -1)
    {
        if (result == 'c')
        {
            m_cfg_file = optarg;
        }
        else if (result == 'p')
        {
            m_listen_port = (unsigned short)atoi(optarg);
        }
        else if(result == 's')
        {
            SetSvrInfo(SERVER_TYPE_CENTER, atoi(optarg));
        }
        else if (result == 'h')
        {
            Usage(argv[0]);
            return -2;
        }
    }

    if(m_cfg_file.empty())
        m_cfg_file = CONFIG_PATH;

    LoadConfig();

    return 0;
}



void Center::Usage(const char *name)
{
    std::cout << "Usage:" << std::endl;;
    std::cout << "\t" << name << " -p 1234" << std::endl;
    std::cout << "\t" << name << " -c ../conf/config.json" << std::endl;
    std::cout << "\t" << name << " -p 1234 -c ../conf/config.json" << std::endl;
}

int main(int argc, char **argv)
{
    SingleInstance(argv[0]);

    daemon (1, 1);
    InitSignal();
    CenterInst.Run(argc, argv);
    return 0;
}
