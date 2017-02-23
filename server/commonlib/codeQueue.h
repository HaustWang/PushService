#ifndef _CODEQUEUE_HPP_
#define _CODEQUEUE_HPP_

#include "base.h"
#define ID_PIPE_C2S 0
#define ID_PIPE_S2C 1

struct _tagLock
{
	int m_aiLockFDs[32];
	short SetLock(short nLockID, const char *szFileName);
	short DelLock(short nLockID);
	int   WLockWait( short nLockID );
	int   RLockWait( short nLockID );
	int   UnLock( short nLockID );
};


class CCodeQueue
{
public:
	CCodeQueue();
	CCodeQueue( int nTotalSize );
	CCodeQueue( int nTotalSize, const char *szLockFile, short nLockID );
	~CCodeQueue();

	int Initialize( int nTotalSize );
	int Resume( int nTotalSize );
	int AppendOneCode(const BYTE *pInCode, short sInLength);
	int GetHeadCode(BYTE *pOutCode, short *psOutLength);
	int PeekHeadCode(BYTE *pOutCode, short *psOutLength);
	int DeleteHeadCode();
	int GetOneCode(int iCodeOffset, short nCodeLength, BYTE *pOutCode, short *psOutLength);
	int IsQueueEmpty();

	static struct _tagLock stFileLocks;

protected:

private:

	int SetFullFlag( int iFullFlag );

	int GetCriticalData(int *piBeginIdx, int *piEndIdx, short *pnFullFlag);
	int SetCriticalData(int iBeginIdx, int iEndIdx, short nFullFlag);

	BYTE *m_pCurrentShm;
	struct _tagHead
	{
		int m_nSize;
		long m_iCodeBufOffSet;
		int m_nBegin;
		int m_nEnd;
		short m_nFullFlag;
		short m_nLockIdx;
	} m_stQueueHead;
};

#endif
