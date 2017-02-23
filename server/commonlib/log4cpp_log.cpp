#include <string.h>
#include <errno.h>
#include "log4cpp_log.h"

using namespace std;
using namespace log4cplus;
using namespace log4cplus::helpers;
log4cplus::Logger g_logger;

#define DEFAULT_FILE_NUM    3
#define DEFAULT_FILE_SIZE   50*1024*1024

static char log_dir[128] = "../log";

unsigned short log_type = 2;

int init_log(const char *app, const char *dir, const char *config_content, int max_num, int max_size)
{
    char szData[512] = { 0 };
    char appname[128] = { 0 };
    char str_pid[128]={0};

    //将PID,Process name 放到环境变量中，在日志命名中用到
    snprintf(str_pid,sizeof(str_pid)-1,"%d",(unsigned)getpid());
    setenv("PID",str_pid,1);
    setenv("PROCNAME",program_invocation_short_name,1);

    //设置APP环境变量
    strncpy(appname, app, sizeof(appname)-1);
    setenv("APP",appname,1);
    if(dir)
    {
        strncpy (log_dir, dir, sizeof (log_dir) - 1);
    }

    mkdir(log_dir, 0777);
    if(access(log_dir, W_OK|X_OK) < 0)
    {
        printf("logdir(%s): Not writable:%s\n", log_dir, strerror(errno));
        return -1;
    }

    //设置DIR环境变量
    setenv("DIR",log_dir,1);

    if(max_num > 0)
    {
        snprintf(szData,sizeof(szData),"%d",max_num);
    }
    else
    {
        snprintf(szData,sizeof(szData),"%d",DEFAULT_FILE_NUM);
    }
    setenv("MAXNUMBER",szData,1);
    if(max_size > 0)
    {
        snprintf(szData,sizeof(szData),"%d",max_size);
    }
    else
    {
        snprintf(szData,sizeof(szData),"%d",DEFAULT_FILE_SIZE);
    }
    setenv("MAXSIZE", szData,1);
    if( Logger::exists(app) )
    {
        printf( "Logger::exists %s\n", app );
        return 0;
    }

    char log_name[256] = { 0 };
    switch( log_type )
    {
        case 1:
            {
                if(config_content == NULL || 0 == strlen(config_content))
                {
                    snprintf( log_name, sizeof(log_name),"../conf/net_logger.properties");
                    //printf( "log file name:%s\n", log_name );
                    log4cplus::PropertyConfigurator::doConfigure(log_name);
                }
                else
                {
                    log4cplus::tistringstream t_stream(config_content);
                    PropertyConfigurator config(t_stream); 
                    config.configure();
                }

                SharedAppenderPtr sock = Logger::getRoot().getAppender("RemoteServer");
                Logger::getRoot().removeAllAppenders();

                g_logger = Logger::getInstance(app);
                if(sock)
                {
                    g_logger.addAppender(sock);
                    //printf("start write network log\n");
                }
            }
            break;
        case 2:
            {
                if(config_content == NULL || 0 == strlen(config_content))
                {
                    snprintf( log_name, sizeof(log_name),"../conf/local_logger.properties");
                    //printf( "log file name:%s\n", log_name );
                    log4cplus::PropertyConfigurator::doConfigure(log_name);
                }
                else
                {
                    log4cplus::tistringstream t_stream(config_content);
                    PropertyConfigurator config(t_stream); 
                    config.configure();
                }

                SharedAppenderPtr fileAppender = Logger::getRoot().getAppender("GAME_LOG");
                Logger::getRoot().removeAllAppenders();
                g_logger = Logger::getInstance(app);
                if( fileAppender )
                {
                    g_logger.addAppender(fileAppender);

                }
            }
            break;
        default:
            printf("Error log type:%d\n", log_type);
            break;
    }

    return 0;
}

void set_log_level(unsigned short l)
{
    int level = l % 7;
    g_logger.setLogLevel(level*10000);
}

void set_log_type(unsigned short type)
{
    log_type = type;
}

extern "C"
{
    void CommonEventLog(int severity, const char *msg)
    {
        switch(severity)
        {
            //case EVENT_LOG_DEBUG:
            default:
            case 0:
                {
                    log_debug("libevent|%s", msg);
                    break;
                }
                //case EVENT_LOG_MSG:
            case 1:
                {
                    log_info("libevent|%s", msg);
                    break;
                }
                //case EVENT_LOG_WARN:
            case 2:
                {
                    log_warning("libevent|%s", msg);
                    break;
                }
                //case EVENT_LOG_ERR:
            case 3:
                {
                    log_error("libevent|%s", msg);
                    break;
                }
        }
    }
}
