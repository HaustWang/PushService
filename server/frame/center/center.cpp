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

const static char server_name[][20] = {"center", "proxy", "connector", "phpproxy", "dbworker" };

int ConfigReqHandler::ProcessMessage(ClientInfo* client_info, const google::protobuf::Message* phead, const google::protobuf::Message* message)
{
    return CenterInst.ProcessConfigReq(client_info, (const SvrMsgHead*)phead, message);
}

Center::Center()
    : MessageProcessor()
    , m_listen_port(0)
    , m_event_base(NULL)
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
}

int Center::ProcessClose(ClientInfo* client_info)
{
    log_info("close connection:%s", client_info->ShortDebugString().c_str());

    m_server_addr.erase(client_info->fd);
    return ClientManage::Instance()->DeleteClientInfo((ClientInfo*)client_info);
}

int Center::ProcessConfigReq(ClientInfo* client_info, const SvrMsgHead* head, const google::protobuf::Message* message)
{
    if(NULL == client_info || NULL == head)
        return -1;

    client_info->server_type = head->src_svr_type();
    client_info->server_id = head->src_svr_id();
    client_info->is_register = true;

    //response get config
    SvrConfigResp resp;
    FillConfigResp(head->src_svr_type(), resp);

    SvrMsgHead msg_head;
    FillMsgHead(&msg_head, SMT_CONFIG_RESP, head->src_svr_type(), head);
    SendMessageToServer(client_info, &msg_head, &resp);

    if(head->src_svr_type() != SERVER_TYPE_PROXY || NULL == message)
      return 0;

    const SvrConfigReq* req = dynamic_cast<const SvrConfigReq*>(message);
    m_server_addr[client_info->fd] = req->address();

    SvrBroadcastAddress broad_msg;
    broad_msg.add_peer_addresses()->CopyFrom(req->address());

    const std::map<int, ClientInfo*>& client_map = ClientManage::Instance()->GetClientMap();
    for(std::map<int, ClientInfo*>::const_iterator client_it = client_map.begin(); client_it != client_map.end(); ++client_it)
    {
        if(SERVER_TYPE_PROXY == client_it->second->server_type)
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

    std::vector<ClientInfo*> client_vec;
    ClientManage::Instance()->GetClientInfoList(SERVER_TYPE_PROXY, client_vec);

    log_debug("get proxy list size:%ld, client_size:%ld\n", client_vec.size(), ClientManage::Instance()->GetClientCnt());
    for(std::vector<ClientInfo*>::iterator client_it = client_vec.begin(); client_it != client_vec.end(); ++client_it)
        resp.add_peer_addresses()->CopyFrom(m_server_addr[(*client_it)->fd]);
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
        printf("[%s:%d] parse args error!", __FILE__, __LINE__);
        Usage(argv[0]);
        return -1;
    }

    if(0 != InitService())
    {
        log_error("Init service failed!");
        return 0;
    }

    while (1 != stop)
    {
        event_base_loop(m_event_base, EVLOOP_NONBLOCK);

        //静默加载配置
        ReloadConfig();
        usleep(1000);
    }

    return 0;
}

int Center::ParseArgs(int argc, char **argv)
{
    const char* option = "c:p:h";
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
