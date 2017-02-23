#ifndef _LOG_4_CPP_LOG_H_
#define _LOG_4_CPP_LOG_H_


#include "log4cplus/logger.h"
#include "log4cplus/configurator.h"
#include "log4cplus/fileappender.h"
#include "log4cplus/consoleappender.h"
#include "log4cplus/layout.h"
#include "log4cplus/loggingmacros.h"

#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

extern log4cplus::Logger g_logger;
#define log_fatal(fmt,args...)		LOG4CPLUS_FATAL_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)
#define log_notice(fmt, args...)	LOG4CPLUS_ERROR_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)
#define log_boot(fmt, args...)		LOG4CPLUS_ERROR_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)
#define log_error(fmt, args...)		LOG4CPLUS_ERROR_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)
#define log_warning(fmt, args...)	LOG4CPLUS_WARN_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)
#define log_info(fmt, args...)		LOG4CPLUS_INFO_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)
#define log_debug(fmt, args...)		LOG4CPLUS_DEBUG_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)
#define log_trace(fmt, args...)		LOG4CPLUS_TRACE_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)

#define error_log(fmt, args...)		LOG4CPLUS_ERROR_FMT(g_logger, "[%s:%s:%d] " fmt, basename(__FILE__), __FUNCTION__, __LINE__, ##args)

#define __ASSERT_AND_LOG(X) \
    if (!(X)) { log_error("%s is invalid\n", #X); }

#define ASSET_AND_RETURN_INT(X) \
    __ASSERT_AND_LOG(X); if (!(X)) return -1

#define ASSET_AND_RETURN_INT_FORMAT(X, format, ...) \
    if (!(X)) { log_error("%s is invalid," format, #X, ##__VA_ARGS__); }; if (!(X)) return -1

#define ASSET_AND_RETURN_VOID(X) \
    __ASSERT_AND_LOG(X); if (!(X)) return

#define ASSET_AND_RETURN_PTR(X) \
    __ASSERT_AND_LOG(X); if (!(X)) return NULL

#define ASSET_AND_RETURN_EX(X, ret) \
    __ASSERT_AND_LOG(X); if (!(X)) return ret

#define ASSET_AND_RETURN_BOOL(X) \
    __ASSERT_AND_LOG(X); if (!(X)) return false


int init_log(const char *app, const char *dir=NULL, const char *config_content = NULL, int max_num=-1, int max_size=-1);
void set_log_level(unsigned short l);
void set_log_type(unsigned short  t);


extern "C"
{
    void CommonEventLog(int severity, const char *msg);
}
#endif

