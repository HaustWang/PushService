#ifndef _MYSQL_DB_RESULT_H_
#define _MYSQL_DB_RESULT_H_

#include <mysql.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>


class MYSQLDBResult
{
public:
    //
    struct BINARY
    {
    };
private:
    //�������
    MYSQL_RES*    m_result;
    //������ϵĵ�ǰ��
    MYSQL_ROW    m_current_row;
    //������ϵĵ�ǰ��
    uint32_t     m_current_field;
    //������ڵ�ǰ�е��еĳ���
    uint64_t*     m_fields_length;
    //������ϵ�����
    uint32_t     m_row_count;
    //������ϵ�����
    uint32_t     m_field_count;

public:

    MYSQLDBResult();
    ~MYSQLDBResult();

    //��������Ƿ�Ϊ��
    bool is_null();
    //���ý������
    void set_result(MYSQL_RES* result);
    //�ͷ�MYSQL_RES
    void free_result();
    //��ȡ��һ������
    bool fetch_next_row();


    //���� >> ��������,���ڽ�������
    MYSQLDBResult& operator >> (char& val);
	MYSQLDBResult& operator >> (int8_t& val);
    MYSQLDBResult& operator >> (int16_t& val);
    MYSQLDBResult& operator >> (int32_t& val);
    MYSQLDBResult& operator >> (int64_t& val);
    
    MYSQLDBResult& operator >> (unsigned char& val);
    MYSQLDBResult& operator >> (uint16_t& val);
    MYSQLDBResult& operator >> (uint32_t& val);
    MYSQLDBResult& operator >> (uint64_t& val);

    MYSQLDBResult& operator >> (float& val);
    MYSQLDBResult& operator >> (double& val);

	//����������������ǰ�����ȱ�֤val�ĳ��ȣ���ֹ���
    MYSQLDBResult& operator >> (char* val);
	MYSQLDBResult& operator >> (unsigned char* val); 
    MYSQLDBResult& operator >> (BINARY* val);


	MYSQLDBResult& get_string (char* buff, size_t buff_len);
	MYSQLDBResult& get_string (unsigned char* buff, size_t buff_len);

	//ʹ��ʱ�ж�buff_len,���buff_len ����ֵΪ0,blobΪ��
	MYSQLDBResult& get_blob (char* buff, size_t &buff_len);
	MYSQLDBResult& get_blob (unsigned char* buff, size_t &buff_len);


private:
    //������
    MYSQLDBResult& operator= (const MYSQLDBResult&);

};

inline bool MYSQLDBResult::is_null()
{
    if(NULL == m_result)
    {
        return true;
    }

    return false;
}

#endif //_MYSQL_DB_RESULT_H_


