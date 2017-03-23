#include "http_manager.h"
#include "log4cpp_log.h"
#include "connect_center.h"

#include <sstream>

HttpManager::HttpManager()
{
}


HttpManager::~HttpManager()
{
}

int HttpManager::Init()
{
    const SvrConfig& config = ConnectToCenter::Instance()->GetConfig();
    int ret = send_msgproxy_client_.Init(config.php_host().c_str(), config.php_port(), HttpCallBackRsp);
    if (ret < 0)
    {
        log_error("http client init failed, iRet:%d", ret);
        return ret;
    }

    ret = get_proxymsg_server_.Init("0.0.0.0", config.http_listen());
    if(ret < 0)
    {
        log_error("http msgproxy server init failed, ret:%d",ret);
        return ret;
    }

    return 0;
}

int HttpManager::ReportData(int64_t msgid, std::string const client_id, int action)
{
    if(client_id.empty() || msgid < 0 || action < 0)    return -1;

    std::ostringstream oss;
    oss << "/push/report?client_id=" << client_id << "&msgid=" << msgid << "&action=" << action;

    return send_msgproxy_client_.Query(oss.str().c_str());
}



