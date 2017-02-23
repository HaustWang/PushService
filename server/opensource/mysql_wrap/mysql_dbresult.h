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
    //结果集合
    MYSQL_RES*    m_result;
    //结果集合的当前行
    MYSQL_ROW    m_current_row;
    //结果集合的当前列
    uint32_t     m_current_field;
    //结果集内当前行的列的长度
    uint64_t*     m_fields_length;
    //结果集合的行数
    uint32_t     m_row_count;
    //结果集合的列数
    uint32_t     m_field_count;

public:

    MYSQLDBResult();
    ~MYSQLDBResult();

    //结果集合是否为空
    bool is_null();
    //设置结果集合
    void set_result(MYSQL_RES* result);
    //释放MYSQL_RES
    void free_result();
    //获取下一行数据
    bool fetch_next_row();


    //重载 >> 操作符号,用于将结果输出
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

	//调用以下三个函数前，得先保证val的长度，防止溢出
    MYSQLDBResult& operator >> (char* val);
	MYSQLDBResult& operator >> (unsigned char* val); 
    MYSQLDBResult& operator >> (BINARY* val);


	MYSQLDBResult& get_string (char* buff, size_t buff_len);
	MYSQLDBResult& get_string (unsigned char* buff, size_t buff_len);

	//使用时判断buff_len,如果buff_len 返回值为0,blob为空
	MYSQLDBResult& get_blob (char* buff, size_t &buff_len);
	MYSQLDBResult& get_blob (unsigned char* buff, size_t &buff_len);


private:
    //不让用
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


