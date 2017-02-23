#include "mysql_handler.h"
#include "mysql_dbresult.h"

MYSQLHandler::MYSQLHandler(void):
    m_port(mysql_default_port),   
    m_is_connected(false)
{
    mysql_init(&m_handler);
    
    m_host_ip[0] = '\0';
    m_user_name[0] = '\0';
    m_password[0] = '\0';
}

MYSQLHandler::~MYSQLHandler(void)
{
    disconnect();
}

int32_t MYSQLHandler::init(const char* host_ip, const char* user, const char* pwd, uint16_t port)
{
    memcpy(m_host_ip, host_ip, mysql_string_length);
    memcpy(m_user_name, user, mysql_string_length);
    memcpy(m_password, pwd, mysql_string_length);
    m_port = port;

    //尝试着连接数据库
    uint32_t timeout = 15;
    return connect(timeout);
}

int32_t MYSQLHandler::init(const char* host_ip, const char* user, const char* pwd, const char* db_name, uint16_t port)
{
	int32_t ret = init(host_ip, user, pwd, port);
	if (0 == ret)
	{
		return select_db(db_name);
	}

	return -1;
}

int32_t MYSQLHandler::fini()
{
    if (m_is_connected)
    {
        disconnect();
    }

    return 0;
}

//用于非SELECT,INSERT,UPDATE 语句
int32_t MYSQLHandler::execute(const char* sql)
{
	return execute(sql, NULL, NULL, NULL, false);
}

//用于非SELECT语句(INSERT,UPDATE)
int32_t MYSQLHandler::execute(const char* sql, int32_t& numaffect, uint64_t& lastid)
{
    return execute(sql, &numaffect, &lastid, NULL, false);
}

//用于SELECT语句,直接转储结果集合的方法
int32_t MYSQLHandler::execute(const char* sql, int32_t& numaffect, MYSQLDBResult& dbresult)
{
    return execute(sql, &numaffect, NULL, &dbresult, true);
}

//用于SELECT语句,用于use_result得到结果集合的方法
int32_t MYSQLHandler::execute(const char* sql, MYSQLDBResult& dbresult)
{
    return execute(sql, NULL, NULL, &dbresult, false);
}

int32_t MYSQLHandler::select_db(const char* db_name)
{
	if (NULL == db_name)
	{
		return -1;
	}
	else
	{
		if (mysql_success == mysql_select_db(&m_handler, db_name))
		{
			strncpy(m_db_name, db_name, mysql_dbname_length);
			return 0;
		}
		else
		{
			return -1;
		}
	}
}

const char* MYSQLHandler::get_last_error(void)
{
	return mysql_error(&m_handler);
}

//返回最近调用MySQL的API函数的错误代码。“0”返回值表示未出现错误，在MySQL errmsg.h头文件中，列出了客户端错误消息编号。
uint32_t MYSQLHandler::get_last_errno()
{
    return mysql_errno(&m_handler);
}

//excute实现函数，设指针参数的目的在于检查某个参数是否为NULL表示它是否起作用
int32_t MYSQLHandler::execute(const char* sql, int32_t* numaffect, uint64_t* lastid, MYSQLDBResult* dbresult, bool bstore)
{

    if((!m_is_connected) || (NULL == sql))
    {
        return -1;
    }
    
    int result = -1;
    //如果数据库未连接上，则尝试着连接一次
    if(!m_is_connected)
    {
        uint32_t timeout = 1;
        result = connect(timeout);
        if (0 != result)
        {
            return -1;
        }

    }
    else
    {
        result = mysql_ping(&m_handler);
        if (0 != result)
        {
            return -1;
        }
    }

    //执行SQL命令
    result = mysql_real_query(&m_handler, sql, (unsigned long)strlen(sql));
    if (0 != result)
    {
        return -1;
    }
    
    if(dbresult)
    {
        MYSQL_RES* mysql_result = NULL;
        if(bstore)
        {
            mysql_result = mysql_store_result(&m_handler);
        }
        else
        {
            mysql_result = mysql_use_result(&m_handler);
        }

        if((NULL == mysql_result) && (mysql_field_count(&m_handler) > 0))
        {
            return -1;
        }

        dbresult->set_result(mysql_result);
    }

    if(numaffect)
    {
		my_ulonglong ulTmp = mysql_affected_rows(&m_handler);
		if ((my_ulonglong)-1 == ulTmp)
		{
			*numaffect = -1;
		}
		else
		{
			*numaffect = static_cast<int32_t>(ulTmp) ;
		}
    }

    if(lastid)
    {
        *lastid = static_cast<uint64_t>(mysql_insert_id(&m_handler)) ;
    }

    return 0;
}

int32_t MYSQLHandler::make_real_escape_string(char* to, const char* from, uint32_t length)
{
    return mysql_real_escape_string(&m_handler, to, from, length);
}

int32_t MYSQLHandler::connect(uint32_t timeout)
{
    //如果连接已经建立,则关闭原来的连接
    if(m_is_connected)
    {
        disconnect();
    }

    //初始化MYSQL句柄
    mysql_init(&m_handler);

    //设置连接的超时
    if(0 != timeout)
    {
        mysql_options(&m_handler, MYSQL_OPT_CONNECT_TIMEOUT, (char*)(&timeout));
    }

    mysql_options(&m_handler, MYSQL_SET_CHARSET_NAME, "utf8");

    uint32_t client_flag =0;

    //连接数据库
    MYSQL* result = NULL;
    result = mysql_real_connect(&m_handler, m_host_ip, m_user_name, m_password, NULL, m_port, NULL, client_flag);
    if (NULL == result)
    {
        return -1;
    }

	//设置重连
	my_bool reconnect = 1; 
	mysql_options(&m_handler, MYSQL_OPT_RECONNECT, &reconnect);

    m_is_connected = true;
    return 0;
}

int32_t MYSQLHandler::disconnect()
{
    if (!m_is_connected)
    {
        return 0;
    }

    mysql_close(&m_handler);
    m_is_connected = false;
    return 0;
}

