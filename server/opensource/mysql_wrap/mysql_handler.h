#ifndef _MYSQL_DB_HANDLER_H_
#define _MYSQL_DB_HANDLER_H_

#include <mysql.h>
#include <stdint.h>

class MYSQLDBResult;

class MYSQLHandler
{
public:
    enum
    {
		mysql_success		 = 0,
		mysql_default_port	 = 3306,
		mysql_string_length	 = 128,
		mysql_dbname_length	 = 256,
    };

protected:
    //数据库的IP地址
    char m_host_ip[mysql_string_length+1];
    //数据库的用户名
    char m_user_name[mysql_string_length+1];
    //数据库的密码
    char m_password[mysql_string_length+1];
    //数据库的端口号
    uint16_t m_port;
	//默认数据库
	char m_db_name[mysql_dbname_length+1];

    //是否连接MYSQL数据库
    bool m_is_connected;

public:
	//MYSQL的句柄
	MYSQL m_handler;

public:
    MYSQLHandler(void);
    ~MYSQLHandler(void);

    //初始化DB处理器，会尝试连接一次数据库，即调用connect函数
    int32_t init(const char* host_ip, const char* user="root", const char* pwd="", uint16_t port=mysql_default_port);

	int32_t init(const char* host_ip, const char* user="root", const char* pwd="", const char* db_name="", uint16_t port=mysql_default_port);
    
    //结束DB处理器，断开与数据库的连接
    int32_t fini();

    //创建可在SQL语句中使用的合法SQL字符串
    int32_t make_real_escape_string(char* to, const char* from, uint32_t length);

	//用于非SELECT,INSERT,UPDATE 语句
	int32_t execute(const char* sql);

    //用于非SELECT语句(INSERT,UPDATE)
    int32_t execute(const char* sql, int32_t& numaffect, uint64_t& lastid);

    //用于SELECT语句,直接转储结果集合的方法
    int32_t execute(const char* sql, int32_t& numaffect, MYSQLDBResult& dbresult);

    //用于SELECT语句,用于use_result得到结果集合的方法
    int32_t execute(const char* sql, MYSQLDBResult& dbresult);

	//选择指定的数据库成为由mysql指定的连接上的默认数据库（当前数据库）
	int32_t select_db(const char* db_name);

	//返回最近调用MySQL的API函数的错误提示
	const char* get_last_error(void); 

	//返回最近调用MySQL的API函数的错误代码。“0”返回值表示未出现错误，在MySQL errmsg.h头文件中，列出了客户端错误消息编号。
	uint32_t get_last_errno();
    
    //连接数据服务器
    int32_t connect(uint32_t timeout);
    
    //断开数据服务器
    int32_t disconnect();

protected:

    //excute实现函数，设指针参数的目的在于检查某个参数是否为NULL表示它是否起作用
    int32_t execute(const char* sql, int32_t* numaffect, uint64_t* lastid, MYSQLDBResult* dbresult, bool bstore);

    MYSQLHandler& operator= (const MYSQLHandler& handler);
    MYSQLHandler(const MYSQLHandler& handler);

};

#define MYSQL_HANDLER Singleton<MYSQLHandler>::Instance()


#endif //_MYSQL_DB_HANDLER_H_


