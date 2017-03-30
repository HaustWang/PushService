#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "log4cpp_log.h"
#include "redismanager.h"

RedisManager::RedisManager()
{
}
RedisManager::~RedisManager()
{
    if (connect_)
    {
        redisFree(connect_);
    }
}

int RedisManager::InitClient(std::string const& ip, unsigned short port, std::string const& env, std::string const& passwd)
{
    redis_server_ip_ = ip;
    port_ = port;
    env_ = env;
    passwd_ = passwd;
    return Connect();
}

int RedisManager::Connect()
{
    struct timeval time_out = {1, 500000};
    connect_ = redisConnectWithTimeout(redis_server_ip_.c_str(), port_, time_out);
    if (connect_ == NULL)
    {
        log_error("redisConnect failed, ip:%s, port:%d\n", redis_server_ip_.c_str(), port_);
        return RedisServerInvaild;
    }
    if (connect_->err)
    {
        log_error("redisConnect failed, ip:%s, port:%d, err:%d:%s\n", redis_server_ip_.c_str(), port_, connect_->err, connect_->errstr);
        redisFree(connect_);
        connect_ = NULL;
        return RedisServerInvaild;
    }

    return Authorize();
}

int RedisManager::ReConnect()
{
    if(connect_)
    {
        redisFree(connect_);
        connect_ = NULL;
    }
    return Connect();
}

int RedisManager::Authorize()
{
    if("test" == env_)   return RedisSuccess;

    if("AliCloud" == env_)
    {
        redisReply* reply = (redisReply *)redisCommand(connect_, "auth %s", passwd_.c_str());
        if (NULL == reply)
        {
            log_error("auth failed:%s", passwd_.c_str()); 
            return RedisSystemError;
        }

        if (reply->type != REDIS_REPLY_STATUS || 0 != strncasecmp(reply->str, "OK", 2))
        {
            log_error("auth failed, type%d err:%s, passwd_:%s\n", reply->type, reply->str, passwd_.c_str());
            freeReplyObject(reply);
            return RedisSystemError;
        }

        return RedisSuccess; 
    }

    return RedisServerInvaild;
}

int RedisManager::UpdateOneHashField(std::string const& key_name, FieldInfo const& field, int expire_time)
{
    redisAppendCommand(connect_, "hset %s %s %b", key_name.c_str(), field.name.c_str(), field.value.c_str(), field.value.size());
    if (expire_time > 0)
    {
        redisAppendCommand(connect_, "expire %s %d", key_name.c_str(), expire_time);
    }
    redisReply* reply = NULL;
    redisGetReply(connect_, (void **)&reply);
    if (reply == NULL)
    {
        log_error("set key:%s field name:%s, error:%d:%s\n", key_name.c_str(), field.name.c_str(), connect_->err, connect_->errstr);
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("connect redis failed err:%d \n", error);
            if (reply != NULL)
            {
                freeReplyObject(reply);
            }
            return error;
        }
        return RedisSystemError;
    }
    if (reply->type != REDIS_REPLY_INTEGER)
    {
        log_error("set Key:%s Field:%s failed, type%d err:%s\n", key_name.c_str(), field.name.c_str(), reply->type, reply->str);
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    freeReplyObject(reply);

    if (expire_time > 0)
    {
        redisGetReply(connect_, (void **)&reply);
        if (reply != NULL)
        {
            log_debug("expire ret type:%d value:%lld", reply->type, reply->integer);
            freeReplyObject(reply);
        }
        else
        {
            log_error("expire set failed ret type:%d value:%lld", reply->type, reply->integer);
        }
    }
    return RedisSuccess;
}

int RedisManager::UpdateOneHashField(int key, FieldInfo const& field, int expire_time)
{
    char _key[kKeyLen] = {0};
    snprintf(_key, sizeof(_key), "%d", key);
    return UpdateOneHashField(_key, field, expire_time);
}

int RedisManager::GetValueByKey(const std::string& key_name,std::string& value )
{
    log_debug("get Key:%s", key_name.c_str());
    redisReply* reply = (redisReply *)redisCommand(connect_, "get %s ", key_name.c_str());
    if (reply == NULL)
    {
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("get key_name:%s", key_name.c_str());
            return error;
        }
        reply = (redisReply *)redisCommand(connect_, "get %s", key_name.c_str());
        if (reply == NULL)
        {
            log_error("get key_name.c_str():%s", key_name.c_str());
            return RedisSystemError;
        }
    }

    if (reply->type == REDIS_REPLY_NIL)
    {
        log_error("get key_name:%s  not get data\n", key_name.c_str());
        freeReplyObject(reply);
        return RedisKeyNotExist;
    }
    else if (reply->type != REDIS_REPLY_STRING)
    {
        log_error("key len:%d is too long\n", reply->len);
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    value.clear();
    value.assign(reply->str, reply->len);

    freeReplyObject(reply);
    return RedisSuccess;
}

int RedisManager::GetOneHashField(std::string const& key_name, FieldInfo& field)
{
    log_debug("get Key:%s, field:%s\n", key_name.c_str(), field.name.c_str());
    redisReply* reply = (redisReply *)redisCommand(connect_, "hget %s %s", key_name.c_str(), field.name.c_str());
    if (reply == NULL)
    {
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("get key_name:%s Field:%s\n", key_name.c_str(), field.name.c_str());
            return error;
        }
        reply = (redisReply *)redisCommand(connect_, "hget %s %s", key_name.c_str(), field.name.c_str());
        if (reply == NULL)
        {
            log_error("get key_name.c_str():%s, field:%s\n", key_name.c_str(), field.name.c_str());
            return RedisSystemError;
        }
    }

    if (reply->type == REDIS_REPLY_NIL)
    {
        log_error("get key_name:%s field:%s not get data\n", key_name.c_str(), field.name.c_str());
        freeReplyObject(reply);
        return RedisKeyNotExist;
    }
    else if (reply->type != REDIS_REPLY_STRING)
    {
        log_error("key len:%d is too long\n", reply->len);
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    field.value.clear();
    field.value.assign(reply->str, reply->len);

    freeReplyObject(reply);
    return RedisSuccess;
}

int RedisManager::GetOneHashField(int key, FieldInfo& field)
{
    char _key[kKeyLen] = {0};
    snprintf(_key, sizeof(_key), "%d", key);
    return GetOneHashField(_key, field);
}

int RedisManager::ExpireKey(std::string const& key_name,int expire_time)
{
    log_debug("expire %s %d",key_name.c_str(), expire_time);
    redisReply* reply = (redisReply *)redisCommand(connect_, "expire %s %d", key_name.c_str(),expire_time);
    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER)
    {
        log_error("expire key:%s ", key_name.c_str());
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    freeReplyObject(reply);
    return RedisSuccess;
}

int RedisManager::DelKey(std::string const& key_name)
{
    log_debug("delete key:%s\n", key_name.c_str());
    redisReply* reply = (redisReply *)redisCommand(connect_, "del %s", key_name.c_str());
    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER)
    {
        log_error("delete key:%s \n", key_name.c_str());
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    freeReplyObject(reply);
    return RedisSuccess;
}


int RedisManager::DelKey(int key)
{
    char _key[kKeyLen] = {0};
    snprintf(_key, sizeof(_key), "%d", key);
    return DelKey(_key);
}

int RedisManager::DelMultiHashField(int key, std::vector<std::string> const& field_list)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }


    static char tmp_key[12];
    snprintf(tmp_key, sizeof(tmp_key), "%d", key);

    return DelMultiHashField(tmp_key, field_list);
}

int RedisManager::DelMultiHashField(std::string const& key, std::vector<std::string> const& field_list)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    int field_num = field_list.size();
    std::vector<const char *> argv(field_num + 2);
    std::vector<size_t> argv_len( field_num + 2);

    char cmd[] = "hdel";
    argv[0] = cmd;
    argv_len[0] = strlen(cmd);

    argv[1] = key.c_str();
    argv_len[1] = strlen(key.c_str());

    for (int i = 0; i < field_num; ++i)
    {
        log_debug("key %s, field %s, len %ld", key.c_str(), field_list[i].c_str(), field_list[i].length());

        argv[i + 2] = field_list[i].c_str();
        argv_len[i + 2] = field_list[i].length();
    }

    redisAppendCommandArgv(connect_, argv.size(), &(argv[0]), &(argv_len[0]));
    redisReply* reply = NULL;
    redisGetReply(connect_, (void **)&reply);
    if (reply == NULL)
    {
        log_error("set key:%s field num:%d, error:%d:%s\n", key.c_str(), field_num, connect_->err, connect_->errstr);
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("connect failed, error:%d\n", error);
            return error;
        }
        return RedisSystemError;
    }

    if (reply->type != REDIS_REPLY_INTEGER || reply->integer != field_num)
    {
        log_error("szkey:%s delete failed, type:%d field_num:%d error:%s\n", key.c_str(), reply->type, field_num, reply->str);
        ReConnect();
        freeReplyObject(reply);
        return RedisSystemError;
    }

    freeReplyObject(reply);
    return RedisSuccess;
}

int RedisManager::DelOneHashField(int key, std::string const& field_name)
{
    redisReply* reply = (redisReply *)redisCommand(connect_, "hdel %d %s", key, field_name.c_str());
    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER)
    {
        log_error("hdel key:%d,field:%s  \n", key, field_name.c_str());
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    freeReplyObject(reply);
    return RedisSuccess;
}

int RedisManager::DelOneHashField(std::string const& key, std::string const& field_name)
{
    redisReply* reply = (redisReply *)redisCommand(connect_, "hdel %s %s", key.c_str(), field_name.c_str());
    if (reply == NULL || reply->type != REDIS_REPLY_INTEGER)
    {
        log_error("hdel key:%s, field:%s \n", key.c_str() ,field_name.c_str());
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    freeReplyObject(reply);
    return RedisSuccess;
}

int RedisManager::GetMultiHashField(int key, std::vector<std::string>& field_name, std::vector<FieldInfo>& field_list)
{
    char _key[kKeyLen] = {0};
    snprintf(_key, sizeof(_key), "%d", key);
    return GetMultiHashField(_key, field_name, field_list);
}

int RedisManager::GetMultiHashField(std::string const& key_name, std::vector<std::string>& field_name, std::vector<FieldInfo>& field_list)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error(" key name:%s connect_ is null", key_name.c_str());
            return -1;
        }
    }

    snprintf(command_, sizeof(command_), "hmget %s ", key_name.c_str());
    for (int i = 0; i < (int)field_name.size(); ++i)
    {
        strncat(command_, field_name[i].c_str(), sizeof(command_) - 1 - strlen(command_));
        strcat(command_, " ");
    }

    log_debug("get key:%s field num:%lu command:%s\n", key_name.c_str(), field_name.size(), command_);

    redisReply* reply = (redisReply *)redisCommand(connect_, command_);
    if (reply == NULL)
    {
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("Connect failed, error:%d, key:%s \n", error, key_name.c_str());
            return error;
        }
        reply = (redisReply *)redisCommand(connect_, command_);
        if (reply == NULL)
        {
            log_error("batch get failed, key:%s error:%d-%s \n", key_name.c_str(), connect_->err, connect_->errstr);
            return RedisSystemError;
        }
    }
    if (reply->type == REDIS_REPLY_NIL)
    {
        log_error("key:%s not get data\n", key_name.c_str());
        freeReplyObject(reply);
        return RedisKeyNotExist;
    }
    if (reply->type != REDIS_REPLY_ARRAY)     // 返回数组
    {
        log_error("key:%s batch get failed, type:%d error:%d-%s\n", key_name.c_str(), reply->type, connect_->err, connect_->errstr);
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    if (reply->elements != field_name.size())
    {
        log_error("key:%s Reply num:%lu is wrong\n", key_name.c_str(), reply->elements);
        freeReplyObject(reply);
        return RedisNotAllCache;
    }

    field_list.resize(field_name.size());
    for (size_t i = 0; i < reply->elements; ++i)
    {
        field_list[i].name = field_name[i];
        field_list[i].value.clear();

        if (reply->element[i] == NULL)
        {
            log_error("key :%s field name:%s get not value", key_name.c_str(), field_name[i].c_str());
            freeReplyObject(reply);
            return RedisSystemError;
        }
        if (reply->element[i]->type == REDIS_REPLY_NIL)
        {
            log_error("key:%s field:%s has no data\n", key_name.c_str(), field_name[i].c_str());
            continue;
        }
        field_list[i].value.assign(reply->element[i]->str, reply->element[i]->len);
    }
    freeReplyObject(reply);
    return RedisSuccess;
}

int RedisManager::GetHashAll(int key, std::vector<FieldInfo>& field_list)
{
    char _key[kKeyLen] = {0};
    snprintf(_key, sizeof(_key), "%d", key);
    return GetHashAll(_key, field_list);
}

int RedisManager::GetHashAll(std::string const& key_name, std::vector<FieldInfo>& field_list)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    snprintf(command_, sizeof(command_), "hgetall %s ", key_name.c_str());
    field_list.clear();

    redisReply* reply = (redisReply *)redisCommand(connect_, command_);
    if (reply == NULL)
    {
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("Connect failed, error:%d, key:%s \n", error, key_name.c_str());
            return error;
        }
        reply = (redisReply *)redisCommand(connect_, command_);
        if (reply == NULL)
        {
            log_error("batch get failed, key:%s error:%d-%s \n", key_name.c_str(), connect_->err, connect_->errstr);
            return RedisSystemError;
        }
    }
    if (reply->type == REDIS_REPLY_NIL)
    {
        log_debug("key:%s not get data\n", key_name.c_str());
        freeReplyObject(reply);
        return RedisKeyNotExist;
    }
    if (reply->type != REDIS_REPLY_ARRAY)     // 返回数组
    {
        log_error("key:%s batch get failed, type:%d error:%d-%s\n", key_name.c_str(), reply->type, connect_->err, connect_->errstr);
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    if (reply->elements % 2 != 0)
    {
        log_error("key:%s Reply num:%lu is wrong\n", key_name.c_str(), reply->elements);
        freeReplyObject(reply);
        return RedisSystemError;
    }

    field_list.resize(reply->elements / 2);
    for (int i = 0; i < (int)reply->elements / 2; ++i)
    {
        if (reply->element[2 * i] == NULL || reply->element[2 * i + 1] == NULL)
        {
            log_error("key :%s element %d invalid\n", key_name.c_str(), 2 * i);
            freeReplyObject(reply);
            return RedisSystemError;
        }
        if (reply->element[2 * i]->type == REDIS_REPLY_NIL ||
                reply->element[2 * i + 1]->type == REDIS_REPLY_NIL)
        {
            log_error("key:%s element %d invalid", key_name.c_str(), 2 * i);
            freeReplyObject(reply);
            return RedisSystemError;
        }

        field_list[i].name.assign(reply->element[2 * i]->str, reply->element[2 * i]->len);
        field_list[i].value.assign(reply->element[2 * i + 1]->str, reply->element[2 * i + 1]->len);
    }

    freeReplyObject(reply);
    return RedisSuccess;
}

int RedisManager::UpdateMultiHashField(int key, std::vector<FieldInfo> const& field_list, int expire_time)
{
    char _key[kKeyLen] = {0};
    snprintf(_key, sizeof(_key), "%d", key);
    return UpdateMultiHashField(_key, field_list, expire_time);
}

int RedisManager::UpdateMultiHashField(std::string const& key_name, std::vector<FieldInfo> const& field_list, int expire_time)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    int field_num = field_list.size();
    std::vector<const char *> argv(field_num * 2 + 2);
    std::vector<size_t> argv_len( field_num * 2 + 2);

    char cmd[] = "hmset";
    argv[0] = cmd;
    argv_len[0] = strlen(cmd);

    argv[1] = key_name.c_str();
    argv_len[1] = strlen(key_name.c_str());

    for (int i = 0; i < field_num; ++i)
    {
        log_debug("key %s, field %s, len %ld", key_name.c_str(), field_list[i].name.c_str(), field_list[i].value.size());

        argv[(i + 1) * 2] = field_list[i].name.c_str();
        argv_len[(i + 1) * 2] = field_list[i].name.size();

        argv[(i + 1) * 2 + 1] = field_list[i].value.c_str();
        argv_len[(i + 1) * 2 + 1] = field_list[i].value.size();
    }
    redisAppendCommandArgv(connect_, argv.size(), &(argv[0]), &(argv_len[0]));
    if (expire_time > 0)
    {
        redisAppendCommand(connect_, "expire %s %d", key_name.c_str(), expire_time);
    }
    redisReply* reply = NULL;
    redisGetReply(connect_, (void **)&reply);
    if (reply == NULL)
    {
        log_error("set key:%s field num:%d, error:%d:%s\n", key_name.c_str(), field_num, connect_->err, connect_->errstr);
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("connect failed, error:%d\n", error);
            return error;
        }
        return RedisSystemError;
    }

    if (reply->type != REDIS_REPLY_STATUS || strcasecmp(reply->str, "OK") != 0)
    {
        log_error("szkey:%s update failed, type:%d field_num:%d error:%s\n", key_name.c_str(), reply->type, field_num, reply->str);
        ReConnect();
        freeReplyObject(reply);
        return RedisSystemError;
    }

    freeReplyObject(reply);

    if (expire_time > 0)
    {
        redisGetReply(connect_, (void **)&reply);
        if (reply != NULL)
        {
            log_debug("expire ret type:%d value:%lld", reply->type, reply->integer);
            freeReplyObject(reply);
        }
        else
        {
            log_error("expire set failed ret type:%d value:%lld", reply->type, reply->integer);
        }
    }
    return RedisSuccess;
}

int RedisManager::GetFieldsFromReply(redisReply* reply,
        const std::string& key,
        const std::vector<std::string>& field_name,
        std::vector<FieldInfo>& field_list)
{
    field_list.resize(field_name.size());
    for (size_t i = 0; i < reply->elements; ++i)
    {
        field_list[i].name = field_name[i];
        field_list[i].value.clear();

        if (reply->element[i] == NULL || reply->element[i]->type == REDIS_REPLY_NIL)
        {
            log_debug("key :%s field name:%s get not value", key.c_str(), field_name[i].c_str());
            continue;
        }

        field_list[i].value.assign(reply->element[i]->str, reply->element[i]->len);
    }
    return 0;
}
int RedisManager::BatchGetHashFields(const std::vector<int>& key_list,
        const std::vector<std::string>& field_name,
        std::map<std::string, std::vector<FieldInfo> >& batch_field_info)
{
    if (key_list.empty())
    {
        log_error("key list empty");
        return -1;
    }
    std::vector<std::string> str_key_list;
    for (size_t i = 0; i < key_list.size(); ++i)
    {
        char _key[kKeyLen] = {0};
        snprintf(_key, sizeof(_key), "%d", key_list[i]);
        str_key_list.push_back(_key);
    }

    return BatchGetHashFields(str_key_list, field_name, batch_field_info);
}

int RedisManager::BatchGetHashFields(const std::vector<std::string>& key_list,
            const std::vector<std::string>& field_name,
            std::map<std::string, std::vector<FieldInfo> >& batch_field_info)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error(" key name: connect_ is null");
            return -1;
        }
    }

    char tmp[1024] = "";
    for (int i = 0; i < (int)field_name.size(); ++i)
    {
        strncat(tmp, field_name[i].c_str(), sizeof(tmp) - 1 - strlen(tmp));
        strcat(tmp, " ");
    }
    log_debug("field name list:%s", tmp);
    //pipe line
    char redis_cmd[1024];
    for (int i = 0; i < (int)key_list.size(); ++i)
    {
        snprintf(redis_cmd, sizeof(redis_cmd) - 1, "hmget %s %s", key_list[i].c_str(), tmp);
        redisAppendCommand(connect_, redis_cmd);
    }

    redisReply* reply = NULL;
    for (int i = 0; i < (int)key_list.size(); ++i)
    {
        redisGetReply(connect_, (void **)&reply);
        if (reply == NULL)
        {
            log_error("get key i:%d %s  failed, error:%d:%s", i, key_list[i].c_str(), connect_->err, connect_->errstr);
            //出现重连直接返回, 后续的数据已经乱了
            int error = ReConnect();
            if (error != RedisSuccess)
            {
                log_error("connect redis failed err:%d", error);
                return error;
            }
            return RedisSystemError;
        }
        if (reply->type != REDIS_REPLY_ARRAY)
        {
            log_error("get key i:%d :%s get failed, type:%d error:%d-%s\n", i, key_list[i].c_str(), reply->type, connect_->err, connect_->errstr);
            freeReplyObject(reply);
            ReConnect();
            return RedisSystemError;
        }

        std::vector<FieldInfo> field_list;
        //从reply中获取hash field info
        GetFieldsFromReply(reply, key_list[i], field_name, field_list);
        batch_field_info[key_list[i]] = field_list;

        freeReplyObject(reply);
    }

    return RedisSuccess;
}

int RedisManager::TrimList(const std::string& list_key, int list_start, int list_end)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    char szkey[128] = {0};
    snprintf(szkey, sizeof(szkey) - 1, "%s", list_key.c_str());
    redisReply* p_reply = (redisReply *)redisCommand(connect_, "ltrim %s %d %d", szkey, list_start, list_end);
    if (NULL == p_reply)
    {
        log_error("ltrim %s failed\n, error:%d-%s\n", szkey, connect_->err, connect_->errstr);
        redisFree(connect_);
        connect_ = NULL;
        return RedisSystemError;
    }
    if (p_reply->type != REDIS_REPLY_STATUS || strcmp(p_reply->str, "OK"))
    {
        log_error("ltrim %s, type:%d, failed\n, error:%d-%s\n", szkey, p_reply->type, connect_->err, connect_->errstr);
        freeReplyObject(p_reply);
        ReConnect();
        return RedisSystemError;
    }

    freeReplyObject(p_reply);
    return RedisSuccess;
}

int RedisManager::PushList(const std::string& list_key, const std::string& node_value, int expire_time,int kDefaultMaxListNum)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    char szkey[128] = {0};
    snprintf(szkey, sizeof(szkey) - 1, "%s", list_key.c_str());
	if (node_value.empty())
    {
        log_error("key:%s push record empty\n", szkey);
        return RedisSystemError;
    }
    redisReply* p_reply = (redisReply *)redisCommand(connect_, "lpush %s %s", szkey, node_value.c_str());
    if (NULL == p_reply)
    {
        log_error("key:%s push one record listname, failed\n, error:%d-%s\n", szkey, connect_->err, connect_->errstr);
        redisFree(connect_);
        connect_ = NULL;
        return RedisSystemError;
    }
    if (p_reply->type != REDIS_REPLY_INTEGER)
    {
        log_error("key:%s, type:%d, push one record listname, failed\n, error:%d-%s\n", szkey, p_reply->type, connect_->err, connect_->errstr);
        freeReplyObject(p_reply);
        ReConnect();
        return RedisSystemError;
    }
    freeReplyObject(p_reply);

   	if (p_reply->integer > kDefaultMaxListNum)
	{
		TrimList(list_key, 0, kDefaultMaxListNum-1);
	}

     if (expire_time > 0)
    {
        log_debug("expire %s %d",szkey, expire_time);
        p_reply = (redisReply*)redisCommand(connect_, "expire %s %d", szkey, expire_time);
        if(p_reply)
        {
            freeReplyObject(p_reply);
        }
    }

    return RedisSuccess;
}

int RedisManager::GetList(const std::string& list_key, std::vector<std::string>& list_values, int kDefaultMaxListNum)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

	int list_num = 0;
    char szkey[1024] = {0};
    snprintf(szkey, sizeof(szkey) - 1, "%s", list_key.c_str());
    log_info("get list key %s\n", szkey);
    redisReply* p_reply = (redisReply *)redisCommand(connect_, "lrange %s 0 -1", szkey);
    if (NULL == p_reply)
    {
        log_error("get listname:%s failed\n, error:%d-%s\n", szkey, connect_->err, connect_->errstr);
        ReConnect();
        return RedisSystemError;
    }
    if (p_reply->type == REDIS_REPLY_NIL)
    {
        log_warning("key:%s has no data\n", szkey);
        freeReplyObject(p_reply);
        return RedisSuccess;
    }
    else if (p_reply->type != REDIS_REPLY_ARRAY)
    {
        log_warning("list key:%s type:%d, get failed, error:%d-%s\n", szkey, p_reply->type, connect_->err, connect_->errstr);
        freeReplyObject(p_reply);
        ReConnect();
        return RedisSystemError;
    }

	char node_value[1024];
	int value_len = 0;
    list_num = p_reply->elements;
    if (list_num > kDefaultMaxListNum)
    {
        list_num = kDefaultMaxListNum;
    }
    for (int i = 0; i < list_num; ++i)
    {
        ASSET_AND_RETURN_EX(p_reply->element[i], RedisSystemError);
        if (p_reply->element[i]->type == REDIS_REPLY_NIL)
        {
            log_warning("key :%s has no data\n", szkey);
            freeReplyObject(p_reply);
            return RedisKeyNotExist;
        }

        if (p_reply->element[i]->len >= (int)sizeof(node_value))
        {
            log_error("key:%s push record len:%d too long\n", szkey, p_reply->element[i]->len);
            freeReplyObject(p_reply);
            return RedisSystemError;
        }
		value_len = p_reply->element[i]->len;
		memcpy(node_value, p_reply->element[i]->str, value_len);
		list_values.push_back(std::string(node_value));
    }
    freeReplyObject(p_reply);

	if ((int)p_reply->elements > kDefaultMaxListNum)
	{
		//
	}

	TrimList(list_key, kDefaultMaxListNum, 0);
    return RedisSuccess;
}

int RedisManager::GetKeys(std::string const& key_pattern, std::vector<std::string>& key_list)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return RedisSystemError; 
        }
    }

    std::string redis_command("keys ");

    if(std::string::npos == key_pattern.find("*"))
        redis_command.append(key_pattern).append("*");
    else
        redis_command.append(key_pattern);

    redisReply* p_reply = (redisReply *)redisCommand(connect_, redis_command.c_str());
    if (NULL == p_reply)
    {
        log_error("get keys:%s failed\n, error:%d-%s\n", key_pattern.c_str(), connect_->err, connect_->errstr);
        ReConnect();
        return RedisSystemError;
    }

    if (p_reply->type == REDIS_REPLY_NIL)
    {
        log_warning("get keys:%s has no data\n", key_pattern.c_str());
        freeReplyObject(p_reply);
        return RedisSuccess;
    }

    if (p_reply->type != REDIS_REPLY_ARRAY)
    {
        log_warning("get keys:%s type:%d get failed, error:%d-%s\n", key_pattern.c_str(), p_reply->type, connect_->err, connect_->errstr);
        freeReplyObject(p_reply);
        ReConnect();
        return RedisSystemError;
    }

    key_list.resize(p_reply->elements);
    for (int i = 0; i < (int)p_reply->elements; ++i)
    {
        if (p_reply->element[i] == NULL || p_reply->element[i]->type == REDIS_REPLY_NIL)
        {
            log_error("get keys:%s element %d invalid\n", key_pattern.c_str(), i);
            freeReplyObject(p_reply);
            return RedisSystemError;
        }

        key_list[i].assign(p_reply->element[i]->str, p_reply->element[i]->len);
    }

    freeReplyObject(p_reply);

    return RedisSuccess;
}

int RedisManager::AddSet(std::string const& set_key, const std::string& set_value, int expire_time)
{
    redisAppendCommand(connect_, "sadd %s %s", set_key.c_str(), set_value.c_str());
    if (expire_time > 0)
    {
        redisAppendCommand(connect_, "expire %s %d", set_key.c_str(), expire_time);
    }
    redisReply* reply = NULL;
    redisGetReply(connect_, (void **)&reply);
    if (reply == NULL)
    {
        log_error("set key:%s set_value:%s, error:%d:%s\n", set_key.c_str(), set_value.c_str(), connect_->err, connect_->errstr);
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("connect redis failed err:%d \n", error);
            if (reply != NULL)
            {
                freeReplyObject(reply);
            }
            return error;
        }
        return RedisSystemError;
    }

    if (reply->type != REDIS_REPLY_INTEGER)
    {
        log_error("set Key:%s set_value:%s failed, type%d err:%s\n", set_key.c_str(), set_value.c_str(), reply->type, reply->str);
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    freeReplyObject(reply);

    if (expire_time > 0)
    {
        redisGetReply(connect_, (void **)&reply);
        if (reply != NULL)
        {
            log_debug("expire ret type:%d value:%lld", reply->type, reply->integer);
            freeReplyObject(reply);
        }
        else
        {
            log_error("expire set failed ret type:%d value:%lld", reply->type, reply->integer);
        }
    }
    return RedisSuccess;
}

int RedisManager::AddSet(std::string const& set_key, const std::vector<std::string>& set_value, int expire_time)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    int field_num = set_value.size();
    std::vector<const char *> argv(field_num + 2);
    std::vector<size_t> argv_len( field_num  + 2);

    char cmd[] = "sadd";
    argv[0] = cmd;
    argv_len[0] = strlen(cmd);

    argv[1] = set_key.c_str();
    argv_len[1] = strlen(set_key.c_str());

    for (int i = 0; i < field_num; ++i)
    {
        log_debug("key %s, field %s, len %ld", set_key.c_str(), set_value[i].c_str(),set_value[i].length()); 

        argv[i + 2] = set_value[i].c_str();
        argv_len[i + 2] = set_value[i].length();
    }

    redisAppendCommandArgv(connect_, argv.size(), &(argv[0]), &(argv_len[0]));
    if (expire_time > 0)
    {
        redisAppendCommand(connect_, "expire %s %d", set_key.c_str(), expire_time);
    }
    redisReply* reply = NULL;
    redisGetReply(connect_, (void **)&reply);
    if (reply == NULL)
    {
        log_error("set key:%s field num:%d, error:%d:%s\n", set_key.c_str(), field_num, connect_->err, connect_->errstr);
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("connect failed, error:%d\n", error);
            return error;
        }
        return RedisSystemError;
    }

    if (reply->type != REDIS_REPLY_INTEGER || reply->integer != field_num)
    {
        log_error("szkey:%s update failed, type:%d field_num:%d error:%s\n", set_key.c_str(), reply->type, field_num, reply->str);
        ReConnect();
        freeReplyObject(reply);
        return RedisSystemError;
    }

    freeReplyObject(reply);

    if (expire_time > 0)
    {
        redisGetReply(connect_, (void **)&reply);
        if (reply != NULL)
        {
            log_debug("expire ret type:%d value:%lld", reply->type, reply->integer);
            freeReplyObject(reply);
        }
        else
        {
            log_error("expire set failed ret type:%d value:%lld", reply->type, reply->integer);
        }
    }
    return RedisSuccess;
}

int RedisManager::AddSet(std::string const& set_key, const int64_t set_value, int expire_time)
{
    static char tmp_value[25];
    memset(tmp_value, 0, sizeof(tmp_value));
    snprintf(tmp_value, sizeof(tmp_value), "%ld", set_value);

    return AddSet(set_key, tmp_value, expire_time);
}

int RedisManager::AddSet(std::string const& set_key, const std::vector<int64_t>& set_value, int expire_time)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    std::vector<std::string> tmp_set_value(set_value.size());
    for(int i = 0; i < set_value.size(); ++i)
    {
        static char tmp_value[25];
        memset(tmp_value, 0, sizeof(tmp_value));
        snprintf(tmp_value, sizeof(tmp_value), "%ld", set_value[i]);
        tmp_set_value.push_back(tmp_value);
    }

    return AddSet(set_key, tmp_set_value, expire_time);
}


int RedisManager::RemSet(std::string const& set_key, const std::string& set_value)
{
    redisReply* reply = (redisReply *)redisCommand(connect_, "srem %s %s", set_key.c_str(), set_value.c_str());
    if (reply == NULL)
    {
        log_error("set key:%s set_value:%s, error:%d:%s\n", set_key.c_str(), set_value.c_str(), connect_->err, connect_->errstr);
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("connect redis failed err:%d \n", error);
            if (reply != NULL)
            {
                freeReplyObject(reply);
            }
            return error;
        }
        return RedisSystemError;
    }

    if (reply->type != REDIS_REPLY_INTEGER)
    {
        log_error("set Key:%s set_value:%s failed, type%d err:%s\n", set_key.c_str(), set_value.c_str(), reply->type, reply->str);
        freeReplyObject(reply);
        ReConnect();
        return RedisSystemError;
    }

    freeReplyObject(reply);

    return RedisSuccess;
}

int RedisManager::RemSet(std::string const& set_key, const std::vector<std::string>& set_value)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    int field_num = set_value.size();
    std::vector<const char *> argv(field_num + 2);
    std::vector<size_t> argv_len( field_num  + 2);

    char cmd[] = "srem";
    argv[0] = cmd;
    argv_len[0] = strlen(cmd);

    argv[1] = set_key.c_str();
    argv_len[1] = strlen(set_key.c_str());

    for (int i = 0; i < field_num; ++i)
    {
        log_debug("key %s, field %s, len %ld", set_key.c_str(), set_value[i].c_str(),set_value[i].length()); 

        argv[i + 2] = set_value[i].c_str();
        argv_len[i + 2] = set_value[i].length();
    }

    redisAppendCommandArgv(connect_, argv.size(), &(argv[0]), &(argv_len[0]));
    redisReply* reply = NULL;
    redisGetReply(connect_, (void **)&reply);
    if (reply == NULL)
    {
        log_error("set key:%s field num:%d, error:%d:%s\n", set_key.c_str(), field_num, connect_->err, connect_->errstr);
        //重连一次
        int error = ReConnect();
        if (error != RedisSuccess)
        {
            log_error("connect failed, error:%d\n", error);
            return error;
        }
        return RedisSystemError;
    }

    if (reply->type != REDIS_REPLY_INTEGER || reply->integer != field_num)
    {
        log_error("szkey:%s update failed, type:%d field_num:%d error:%s\n", set_key.c_str(), reply->type, field_num, reply->str);
        ReConnect();
        freeReplyObject(reply);
        return RedisSystemError;
    }

    freeReplyObject(reply);

    return RedisSuccess;
}

int RedisManager::RemSet(std::string const& set_key, const int64_t set_value)
{
    static char tmp_value[24];
    memset(tmp_value, 0, sizeof(tmp_value));
    snprintf(tmp_value, sizeof(tmp_value), "%ld", set_value);

    return RemSet(set_key, tmp_value);
}

int RedisManager::RemSet(std::string const& set_key, const std::vector<int64_t>& set_value)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return -1;
        }
    }

    std::vector<std::string> tmp_set_value(set_value.size());
    for(int i = 0; i < set_value.size(); ++i)
    {
        static char tmp_value[24];
        memset(tmp_value, 0, sizeof(tmp_value));
        snprintf(tmp_value, sizeof(tmp_value), "%ld", set_value[i]);
        tmp_set_value.push_back(tmp_value);
    }

    return RemSet(set_key, tmp_set_value);
}

int RedisManager::GetSetMembers(const std::string& set_key, std::vector<std::string>& members)
{
    if (connect_ == NULL)
    {
        //尝试重连
        Connect();
        if (connect_ == NULL)
        {
            log_error("connect_ is null");
            return RedisSystemError; 
        }
    }

    redisReply* p_reply = (redisReply *)redisCommand(connect_, "smembers %s", set_key.c_str());
    if (NULL == p_reply)
    {
        log_error("get members:%s failed\n, error:%d-%s\n", set_key.c_str(), connect_->err, connect_->errstr);
        ReConnect();
        return RedisSystemError;
    }

    if (p_reply->type == REDIS_REPLY_NIL)
    {
        log_warning("get members:%s has no data\n", set_key.c_str());
        freeReplyObject(p_reply);
        return RedisSuccess;
    }

    if (p_reply->type != REDIS_REPLY_ARRAY)
    {
        log_warning("get members:%s type:%d get failed, error:%d-%s\n", set_key.c_str(), p_reply->type, connect_->err, connect_->errstr);
        freeReplyObject(p_reply);
        ReConnect();
        return RedisSystemError;
    }

    members.resize(p_reply->elements);
    for (int i = 0; i < (int)p_reply->elements; ++i)
    {
        if (p_reply->element[i] == NULL || p_reply->element[i]->type == REDIS_REPLY_NIL)
        {
            log_error("get members:%s element %d invalid\n", set_key.c_str(), i);
            freeReplyObject(p_reply);
            return RedisSystemError;
        }

        members[i].assign(p_reply->element[i]->str, p_reply->element[i]->len);
    }

    freeReplyObject(p_reply);

    return RedisSuccess;
}

int RedisManager::GetSetMembers(const std::string& set_key, std::vector<int64_t>& members)
{
    std::vector<std::string> str_members;
    int ret = GetSetMembers(set_key, str_members);
    if(RedisSuccess != ret) return ret;

    members.resize(str_members.size());
    for(int i = 0; i < str_members.size(); ++i)
        members.push_back(atol(str_members[i].c_str()));

    return RedisSuccess;
}











