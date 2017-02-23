#ifndef __MULTI_HASH_H__
#define __MULTI_HASH_H__

#include <string>
#include "base.h"
#include "comm_shm.h"
#include "log4cpp_log.h"

const int kMaxRow = 100;
enum MultiHashErrNo
{
    MULTI_HASH_SUCCESS = 0,
    MULTI_HASH_FULL = 1,
    MULTI_HASH_EXSIST = 2,
    MULTI_HASH_NOT_EXSIST = 3,
};

struct HashStatistic
{
    //最大已经使用行数
    int total_count;
    int max_use_row_num;
    int total_used_count;
    int use_count[kMaxRow];
    int collict_num;
    int fail_num;
    void Print()
    {
        log_info("max_use_row_num %d", max_use_row_num);
        log_info("total_count %d", total_count);
        log_info("total_used_count %d", total_used_count);
        log_info("collict_num %d", collict_num);
        log_info("fail_num %d", fail_num);
    }

};
template <class T>
class MultiHash
{
public:
    MultiHash();
    int Init(int shm_id, int row_num, int col_num);

    T* Get(int key);
    int Insert(const T& element);
    int Del(const T& element);
    int Del(int key);

    void traversal(std::vector<T*>& all_item);
    void Print()
    {
        hash_statistic_.Print();
        for (int i = 0; i < row_num_; ++i)
        {
            log_info("row:%d total %d has used %d percent:%d",
                    i, mods_[i], hash_statistic_.use_count[i], 100 * hash_statistic_.use_count[i] / mods_[i]);
        }
    }

    int Size()const
    {
        return hash_statistic_.total_used_count;
    }

private:
    T* hash_table_;
    int row_num_;
    int col_num_;
    int shm_id_;
    int mods_[kMaxRow];
    HashStatistic hash_statistic_;

    int find_row_idx_;
};


template <class T>
MultiHash<T>::MultiHash()
{
    //typedef is_pod_type<T>::CHECK c;
    hash_table_ = NULL;
    row_num_ = 0;
    col_num_ = 0;
    shm_id_ = 0;
    memset(mods_, 0, sizeof(mods_));
    memset(&hash_statistic_, 0, sizeof(hash_statistic_));
}

template <class T>
int MultiHash<T>::Init(int shm_id, int row_num, int col_num)
{
    if (row_num >= kMaxRow)
    {
        log_error("row_num:%d is too big max row:%d", row_num, kMaxRow);
        return -1;
    }

    row_num_ = row_num;
    col_num_ = col_num;
    shm_id_ = shm_id;
    int mod_idx = 0;
    for (int i = col_num_; i > 0; i--)
    {
        if (IsPrime(i))
        {
            mods_[mod_idx] = i;
            mod_idx++;
            if (mod_idx == row_num_)
            {
                break;
            }
        }
    }

    if (mod_idx < row_num_)
    {
        log_error("row_num:%d col_num:%d is invalid", row_num, col_num);
        return -10;
    }

    log_info("row num:%d col num:%d", row_num_, col_num_);
    for (int i = 0; i < row_num_; ++i)
    {
        log_info("row:%d mod:%d", i, mods_[i]);
    }

    std::string err;
    int size = sizeof(T) * row_num_ * col_num_;
    if (GetShmAttachFirst((void **)&hash_table_, shm_id_, size, 0666|IPC_CREAT, err) != 0)
    {
        log_error("get shm id:%d failed, size:%d err:%s", shm_id, size, err.c_str());
        return -20;
    }

    memset(&hash_statistic_, 0, sizeof(hash_statistic_));
    for (int i = 0; i < row_num_; ++i)
    {
        for (int j = 0; j < mods_[i]; ++j)
        {
            T* node = hash_table_ + i * col_num_  + j;
            if (node->GetKey() != 0)
            {
                hash_statistic_.use_count[i]++;
            }
        }
        hash_statistic_.total_count += mods_[i];
    }

    for (int i = 0; i < row_num_; ++i)
    {
        if (hash_statistic_.use_count[i] != 0)
        {
            hash_statistic_.total_used_count += hash_statistic_.use_count[i];
            hash_statistic_.max_use_row_num = i;
        }
    }

    Print();
    return 0;
}

template <class T>
void MultiHash<T>::traversal(std::vector<T*>& all_item)
{
    for (int i = 0; i < row_num_; ++i)
    {
        for (int j = 0; j < mods_[i]; ++j)
        {
            T* node = hash_table_ + i * col_num_  + j;
            if (node->GetKey() != 0)
            {
                all_item.push_back(node);
            }
        }
    }
}

template <class T>
T* MultiHash<T>::Get(int key)
{
    find_row_idx_ = -1;
    for (int i = 0; i < row_num_; ++i)
    {
        T* node = hash_table_ + i * col_num_  + key % mods_[i];
        if (node->GetKey() == key)
        {
            find_row_idx_ = i;
            return node;
        }
    }
    return NULL;
}

template <class T>
int MultiHash<T>::Insert(const T& element)
{
    for (int i = 0; i < row_num_; ++i)
    {
        T* node = hash_table_ + i * col_num_ + element.GetKey() % mods_[i];
        if (node->GetKey() == element.GetKey())
        {
            log_warning("hash table has exsisted element:%d", element.GetKey());
            hash_statistic_.collict_num++;
            return MULTI_HASH_EXSIST;
        }
        //找到一个空槽， 插入
        if (node->GetKey() == 0)
        {
            memcpy(node, &element, sizeof(T));
            hash_statistic_.use_count[i]++;
            hash_statistic_.total_used_count++;
            if (hash_statistic_.max_use_row_num < i)
            {
                hash_statistic_.max_use_row_num = i;
            }
            return MULTI_HASH_SUCCESS;
        }
    }

    log_error("element key:%d insert failed, hash full", element.GetKey());
    hash_statistic_.fail_num++;
    return MULTI_HASH_FULL;
}

template <class T>
int MultiHash<T>::Del(const T& element)
{
    T* node = Get(element.GetKey());
    if (node == NULL)
    {
        return MULTI_HASH_NOT_EXSIST;
    }
    memset(node, 0, sizeof(T));
    hash_statistic_.total_used_count--;
    if (find_row_idx_ >= 0 && find_row_idx_ < row_num_)
    {
        hash_statistic_.use_count[find_row_idx_]--;
    }
    return 0;
}

template <class T>
int MultiHash<T>::Del(int key)
{
    T* node = Get(key);
    if (node == NULL)
    {
        return MULTI_HASH_NOT_EXSIST;
    }
    memset(node, 0, sizeof(T));
    hash_statistic_.total_used_count--;
    if (find_row_idx_ >= 0 && find_row_idx_ < row_num_)
    {
        hash_statistic_.use_count[find_row_idx_]--;
    }
    return 0;
}

#endif
