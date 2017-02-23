#ifndef _THREAD_HPP_
#define _THREAD_HPP_

#include <pthread.h>

enum eRunStatus
{
	rt_init = 0,
	rt_blocked = 1,
	rt_running = 2,
	rt_stopped = 3
};

typedef struct
{
	char sLogBaseName[200];
	long lMaxLogSize;
	int iMaxLogNum;
	int iLogInitialized;
	int iIsShow;
} TLogCfg;

void* ThreadProc( void *pvArgs );

class CThread
{
public:
	CThread();
	virtual ~CThread();

	virtual int PrepareToRun() = 0;
	virtual int Run() = 0;
	virtual int IsToBeBlocked() = 0;

	int CreateThread();
	int WakeUp();
	int StopThread();

	int GetThreadStatus();

protected:
	int CondBlock();
	void ThreadLog(const char *sFormat, ...);

	pthread_t m_hTrd;
	pthread_attr_t m_stAttr;
	pthread_mutex_t m_stMutex;
	pthread_cond_t m_stCond;
	int m_iRunStatus;
    char m_abyRetVal[64];

	TLogCfg m_stLogCfg;

private:
};

#endif
