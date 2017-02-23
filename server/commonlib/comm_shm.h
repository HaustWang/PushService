#ifndef __COMM_SHM_H__
#define __COMM_SHM_H__

#include <sys/ipc.h>
#include <sys/shm.h>
#include <string>

char* GetReadOnlyShm(int iKey, int iSize, std::string& err);
char* GetShm(int iKey, int iSize, int iFlag, std::string& err);
//先尝试直接attach，没有的话，如果IPC_CREAT, 就create
//否则fail
int GetShmAttachFirst(void **pstShm, int iShmID, int iSize, int iFlag, std::string& err);

#endif
