#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "comm_shm.h"

char* GetReadOnlyShm(int iKey, int iSize, std::string& err)
{
    int iShmID;
    char* sShm;
    char sErrMsg[200];

    if ((iShmID = shmget(iKey, iSize, 0)) < 0) 
    {
        snprintf(sErrMsg, sizeof(sErrMsg), "shmget %d %d %d %s", iKey, iSize, errno, strerror(errno));
        err = sErrMsg;
        return NULL;
    }
    if ((sShm = (char *)shmat(iShmID, NULL, SHM_RDONLY)) == (char *) -1) 
    {
        snprintf(sErrMsg, sizeof(sErrMsg), "shmat %d %d %d %s", iKey, iSize, errno, strerror(errno));
        err = sErrMsg;
        return NULL;
    }
    return sShm;
}

char* GetShm(int iKey, int iSize, int iFlag, std::string& err)
{
    int iShmID;
    char* sShm;
    char sErrMsg[200];

    if ((iShmID = shmget(iKey, iSize, iFlag)) < 0) 
    {
        snprintf(sErrMsg, sizeof(sErrMsg), "shmget %d %d %d %s", iKey, iSize, errno, strerror(errno));
        err = sErrMsg;
        return NULL;
    }
    if ((sShm = (char *)shmat(iShmID, NULL ,0)) == (char *) -1) 
    {
        snprintf(sErrMsg, sizeof(sErrMsg), "shmat %d %d %d %s", iKey, iSize, errno, strerror(errno));
        err = sErrMsg;
        return NULL;
    }
    return sShm;
}

//先尝试直接attach，没有的话，如果IPC_CREAT, 就create
//否则fail
int GetShmAttachFirst(void **pstShm, int iShmID, int iSize, int iFlag, std::string& err)
{
    char* sShm;
    if (!(sShm = GetShm(iShmID, iSize, iFlag & (~IPC_CREAT), err))) 
    {
        if (!(iFlag & IPC_CREAT)) 
        {
            return -1;
        }
        if (!(sShm = GetShm(iShmID, iSize, iFlag, err))) 
        {
            return -1;
        }
        memset(sShm, 0, iSize);
    }
    *pstShm = sShm;
    return 0;
}

