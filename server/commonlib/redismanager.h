#ifndef __REDIS_MANAGER_H__
#define __REDIS_MANAGER_H__

#include <vector>
#include <string>
#include <map>
#include "../opensource/redis-2.8.23/deps/hiredis/hiredis.h"

enum RedisErrorCode
{
    RedisSuccess = 0,
    RedisServerInvaild = 1,
    RedisBuffNotEnough = 2, //get buff 不够
    RedisKeyNotExist = 3,
    RedisSystemError = 4,
    RedisNotAllCache =5,
    RedisParaInvalid = 6,
};

//const int kDefaultMaxListNum = 10;  // 2^16 -2  = 65534
const int kMaxCommandLen = 100000;
const int kKeyLen = 64;
//key在redis里最多存活7天

struct FieldInfo
{
    std::string name;
    std::string value;

    FieldInfo(){}
    FieldInfo(std::string const& name, std::string const& value) 
    {
        this->name.assign(name);
        this->value.assign(value);
    }
};

//redis manage redis 封装
class RedisManager
{
public:
    static RedisManager& Instance()
    {
        static RedisManager instance_;
        return instance_;
    }
    RedisManager();
    ~RedisManager();
    int InitClient(std::string const& ip, unsigned short port);

    //get hash里多个field
    int GetMultiHashField(int key, std::vector<std::string>& field_name, std::vector<FieldInfo>& field_list);
    int GetMultiHashField(std::string const& key_name, std::vector<std::string>& field_name, std::vector<FieldInfo>& field_list);
    int GetValueByKey(const std::string& key_name,std::string& value );
    //get hash里一个field
    int GetOneHashField(int key, FieldInfo& field);
    int GetOneHashField(std::string const& key_name, FieldInfo& field);

    //hget all
    int GetHashAll(int key, std::vector<FieldInfo>& field_list);
    int GetHashAll(std::string const& key_name, std::vector<FieldInfo>& field_list);

    int GetFieldsFromReply(redisReply* reply,
            const std::string& key,
            const std::vector<std::string>& field_name,
            std::vector<FieldInfo>& field_list);

    //批量获取一批key 的hash里多个field
    int BatchGetHashFields(const std::vector<int>& key_list,
            const std::vector<std::string>& field_name,
            std::map<std::string, std::vector<FieldInfo> >& batch_field_info);

    int BatchGetHashFields(const std::vector<std::string>& key_list,
            const std::vector<std::string>& field_name,
            std::map<std::string, std::vector<FieldInfo> >& batch_field_info);

    //update hash多个field
    //expire time 为0表示不设置超时
    int UpdateMultiHashField(int key, std::vector<FieldInfo> const& field_list, int expire_time = 0);
    int UpdateMultiHashField(std::string const& key_name, std::vector<FieldInfo> const& field_list, int expire_time = 0);

    //update hash 一个field
    int UpdateOneHashField(std::string const& key_name, FieldInfo const& field, int expire_time = 0);
    int UpdateOneHashField(int key, FieldInfo const& field, int expire_time = 0);

    int DelKey(int key);
    int DelKey(std::string const& key_name);
    int DelMultiHashField(int key, std::vector<std::string> const& field_list);
    int DelMultiHashField(std::string const& key_name, std::vector<std::string> const& field_list);
    int DelOneHashField(std::string const& key_name, std::string const& field);
    int DelOneHashField(int key, std::string const& field);

    int GetKeys(std::string const& key_pattern, std::vector<std::string>& key_list);
    int ExpireKey(std::string const& key_name,int expire_time);

	int PushList(const std::string& list_key, const std::string& node_value, int expire_time=0,int kDefaultMaxListNum=10);
	int GetList(const std::string& list_key, std::vector<std::string>& list_values,int kDefaultMaxListNum=10);
	int TrimList(const std::string& list_key, int list_start, int list_end);

    int AddSet(const std::string& set_key, const std::string& set_value, int expire_time = 0);
    int AddSet(const std::string& set_key, const std::vector<std::string>& set_value, int expire_time = 0);
    int AddSet(const std::string& set_key, const int64_t set_value, int expire_time = 0);
    int AddSet(const std::string& set_key, const std::vector<int64_t>& set_value, int expire_time = 0);
    int RemSet(const std::string& set_key, const std::string& set_value);
    int RemSet(const std::string& set_key, const std::vector<std::string>& set_value);
    int RemSet(const std::string& set_key, const int64_t set_value);
    int RemSet(const std::string& set_key, const std::vector<int64_t>& set_value);
    int GetSetMembers(const std::string& set_key, std::vector<std::string>& members);
    int GetSetMembers(const std::string& set_key, std::vector<int64_t>& members);

private:
    int Connect();
    int ReConnect();
private:
    std::string redis_server_ip_;
    unsigned short port_;
    char command_[kMaxCommandLen];

    redisContext* connect_;
};

#endif
