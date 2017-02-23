#include "mysql_dbresult.h"

#define FIELD_CONVERT_CHECK(source,format,dest)   if(NULL != source) sscanf(source,format,dest) 

MYSQLDBResult::MYSQLDBResult():
    m_result(NULL),
    m_current_row(NULL),
    m_current_field(0),
    m_fields_length(NULL),
    m_row_count(0),
    m_field_count(0)
{
}

MYSQLDBResult::~MYSQLDBResult()
{
    if (NULL != m_result)			
    {
        mysql_free_result(m_result);
    }
}

void MYSQLDBResult::set_result(MYSQL_RES* result)
{
    if (NULL != m_result)
    {
        //释放原有的结果集
        free_result();
        m_result = NULL;
    }

    //重新初始化成员变量
    m_result = result;

    if (NULL != m_result)
    {
        //得到行数,列数
        m_row_count   = static_cast<uint32_t>(mysql_num_rows(m_result));
        m_field_count = static_cast<uint32_t>(mysql_num_fields(m_result));
    }

    return;
}

void MYSQLDBResult::free_result()
{
    if (NULL != m_result)			
    {
        mysql_free_result(m_result);
        //重新初始化成员变量
        m_result = NULL;
        m_current_row   = NULL;
        m_current_field = 0;
        m_fields_length = NULL;
        m_row_count     = 0;
        m_field_count   = 0;
    }

    return;
}

bool MYSQLDBResult::fetch_next_row()
{
    if (NULL == m_result)
    {
        return false;
    }
    //检索结果集的下一行
    m_current_row = mysql_fetch_row(m_result);

    //如果NEXT行为空,结束访问
    if(NULL ==  m_current_row)
    {
        return false;
    }

    //得到结果集内当前行的列的长度
    m_fields_length = mysql_fetch_lengths(m_result);
    m_current_field =0;
    return true;
}

MYSQLDBResult& MYSQLDBResult::operator >> (char& val)
{
    val =0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%c",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (int8_t& val)
{
	val =0;
    int32_t iTmp = 0;
	FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%d",&iTmp);
    val = (int8_t)iTmp;
	m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
	return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (int16_t& val) 
{
    val =0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%hd",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (int32_t& val) 
{
    val =0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%d",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}


MYSQLDBResult& MYSQLDBResult::operator >> (int64_t& val) 
{
    val =0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%ld",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (unsigned char& val)
{
    val =0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%c",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (uint16_t& val) 
{
    val=0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%hu",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (uint32_t& val) 
{
    val=0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%u",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}


MYSQLDBResult& MYSQLDBResult::operator >> (uint64_t& val) 
{
    val = 0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%lu",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (float& val) 
{
    val = 0.0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%f",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}
MYSQLDBResult& MYSQLDBResult::operator >> (double& val) 
{
    val = 0.0;
    FIELD_CONVERT_CHECK(m_current_row[m_current_field],"%lf",&val);
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (char* val) 
{
    if((NULL != val) && (m_current_row[m_current_field]))
    {
        memcpy(val, m_current_row[m_current_field], m_fields_length[m_current_field]+1);
    }
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (unsigned char* val) 
{
    if((NULL != val) && (m_current_row[m_current_field]))
    {
        memcpy(val, m_current_row[m_current_field], m_fields_length[m_current_field]+1);
    }
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}

MYSQLDBResult& MYSQLDBResult::operator >> (MYSQLDBResult::BINARY* val)
{
    if((NULL != val) && (m_current_row[m_current_field]))
    {
        memcpy(val, m_current_row[m_current_field], m_fields_length[m_current_field]);
    }
    m_current_field  = (m_current_field < m_field_count -1)? m_current_field+1 : m_current_field;
    return *this;
}


MYSQLDBResult& MYSQLDBResult::get_string (char* buff, size_t buff_len)
{
	if ((NULL != buff) && (NULL != m_current_row[m_current_field]) && (buff_len > m_fields_length[m_current_field]))
	{
		memcpy(buff, m_current_row[m_current_field], m_fields_length[m_current_field]+1);
	}
	
	m_current_field = (m_current_field < m_field_count-1) ? (m_current_field+1) : m_current_field;

	return *this;
}

MYSQLDBResult& MYSQLDBResult::get_string (unsigned char* buff, size_t buff_len)
{
	if ((NULL != buff) && (NULL != m_current_row[m_current_field]) && (buff_len > m_fields_length[m_current_field]))
	{
		memcpy(buff, m_current_row[m_current_field], m_fields_length[m_current_field]+1);
	}

	m_current_field = (m_current_field < m_field_count-1) ? (m_current_field+1) : m_current_field;

	return *this;
}

MYSQLDBResult& MYSQLDBResult::get_blob (char* buff, size_t &buff_len)
{
	if ((NULL != buff) && (NULL != m_current_row[m_current_field]) && (buff_len >= m_fields_length[m_current_field]))
	{
		memcpy(buff, m_current_row[m_current_field], m_fields_length[m_current_field]);
        buff_len = m_fields_length[m_current_field];
	}else
	{
		buff_len = 0;
	}
	
	m_current_field = (m_current_field < m_field_count-1) ? (m_current_field+1) : m_current_field;

	return *this;
}

MYSQLDBResult& MYSQLDBResult::get_blob (unsigned char* buff, size_t &buff_len)
{
	if ((NULL != buff) && (NULL != m_current_row[m_current_field]) && (buff_len >= m_fields_length[m_current_field]))
	{
		memcpy(buff, m_current_row[m_current_field], m_fields_length[m_current_field]);
        buff_len = m_fields_length[m_current_field];
	}else
	{
		buff_len = 0;
	}
	
	m_current_field = (m_current_field < m_field_count-1) ? (m_current_field+1) : m_current_field;

	return *this;
}


