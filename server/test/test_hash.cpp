#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include "../commonlib/multi_hash.h"

#pragma    pack(1)
class CPlayer
{

public:
	CPlayer()
	{}
	~CPlayer()
	{}

    int GetKey()const
    {
        return cid;
    }
public:
	int cid;//大厅id
	int mid;//具体游戏id
	int regionid;//地区id
	int client_version;//客户端版本号
	int connector_id;//在哪台服务器登录的
	unsigned long time; //登录时间
	char ipaddr[32];//登录的ip地址
	int status; // 0 离线 1 在线
};
#pragma    pack()

const int test_count = 15000;
int main()
{
    MultiHash<CPlayer> multi_hash;
    int ret = multi_hash.Init(0x6666, 20, 1000);
    if (ret != 0)
    {
        printf("Init error ret:%d\n", ret);
        return -1;
    }

    srand(time(NULL));

    std::vector<int> cid_list;
    cid_list.clear();
    for (int i = 0; i < test_count; ++i)
    {
        CPlayer player;
        player.cid = rand();
        player.mid = player.cid;
        player.time = player.cid;
        ret = multi_hash.Insert(player);
        cid_list.push_back(player.cid);
        if (ret != 0)
        {
            printf("insert failed cid %d ret %d\n", player.cid,ret);
            continue;
        }
        printf("%d\n", player.cid);
    }

    for (int i = 0; i < test_count; ++i)
    {
        CPlayer* player = multi_hash.Get(cid_list[i]);
        if (player == NULL)
        {
            printf("cid %d not find\n", cid_list[i]);
            continue;
        }
        if (player->cid != player->mid || player->cid != (int)player->time)
        {
            printf("cid: %d data is wrong\n", player->cid);
        }
    }

    multi_hash.Print();
    return 0;
}



