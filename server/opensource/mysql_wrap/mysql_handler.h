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
    //���ݿ��IP��ַ
    char m_host_ip[mysql_string_length+1];
    //���ݿ���û���
    char m_user_name[mysql_string_length+1];
    //���ݿ������
    char m_password[mysql_string_length+1];
    //���ݿ�Ķ˿ں�
    uint16_t m_port;
	//Ĭ�����ݿ�
	char m_db_name[mysql_dbname_length+1];

    //�Ƿ�����MYSQL���ݿ�
    bool m_is_connected;

public:
	//MYSQL�ľ��
	MYSQL m_handler;

public:
    MYSQLHandler(void);
    ~MYSQLHandler(void);

    //��ʼ��DB���������᳢������һ�����ݿ⣬������connect����
    int32_t init(const char* host_ip, const char* user="root", const char* pwd="", uint16_t port=mysql_default_port);

	int32_t init(const char* host_ip, const char* user="root", const char* pwd="", const char* db_name="", uint16_t port=mysql_default_port);
    
    //����DB���������Ͽ������ݿ������
    int32_t fini();

    //��������SQL�����ʹ�õĺϷ�SQL�ַ���
    int32_t make_real_escape_string(char* to, const char* from, uint32_t length);

	//���ڷ�SELECT,INSERT,UPDATE ���
	int32_t execute(const char* sql);

    //���ڷ�SELECT���(INSERT,UPDATE)
    int32_t execute(const char* sql, int32_t& numaffect, uint64_t& lastid);

    //����SELECT���,ֱ��ת��������ϵķ���
    int32_t execute(const char* sql, int32_t& numaffect, MYSQLDBResult& dbresult);

    //����SELECT���,����use_result�õ�������ϵķ���
    int32_t execute(const char* sql, MYSQLDBResult& dbresult);

	//ѡ��ָ�������ݿ��Ϊ��mysqlָ���������ϵ�Ĭ�����ݿ⣨��ǰ���ݿ⣩
	int32_t select_db(const char* db_name);

	//�����������MySQL��API�����Ĵ�����ʾ
	const char* get_last_error(void); 

	//�����������MySQL��API�����Ĵ�����롣��0������ֵ��ʾδ���ִ�����MySQL errmsg.hͷ�ļ��У��г��˿ͻ��˴�����Ϣ��š�
	uint32_t get_last_errno();
    
    //�������ݷ�����
    int32_t connect(uint32_t timeout);
    
    //�Ͽ����ݷ�����
    int32_t disconnect();

protected:

    //excuteʵ�ֺ�������ָ�������Ŀ�����ڼ��ĳ�������Ƿ�ΪNULL��ʾ���Ƿ�������
    int32_t execute(const char* sql, int32_t* numaffect, uint64_t* lastid, MYSQLDBResult* dbresult, bool bstore);

    MYSQLHandler& operator= (const MYSQLHandler& handler);
    MYSQLHandler(const MYSQLHandler& handler);

};

#define MYSQL_HANDLER Singleton<MYSQLHandler>::Instance()


#endif //_MYSQL_DB_HANDLER_H_


