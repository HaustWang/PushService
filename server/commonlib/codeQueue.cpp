#ifndef _CODEQUEUE_CPP_
#define _CODEQUEUE_CPP_

#include "codeQueue.h"

#ifdef WIN32
#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_UN 4
#define LOCK_NB 8

int flock( int iFD, int op )
{
	return 0;
}
#endif

struct _tagLock CCodeQueue::stFileLocks;

short _tagLock::SetLock(short nLockID, const char *szFileName)
{
	FILE *fp;
	int  iTempFD;

	if( !szFileName || nLockID < 0 || nLockID >=32 )
	{
		return -1;
	}

	fp = fopen(szFileName, "r+");

	if( !fp )
	{
		return -2;
	}

	iTempFD = fileno( fp );

	if( iTempFD < 0)
	{
		return -3;
	}

	m_aiLockFDs[nLockID] = iTempFD;

	return 0;

}

int _tagLock::RLockWait(short nLockID)
{
	if( nLockID < 0 || nLockID >= 32 || m_aiLockFDs[nLockID] < 0 )
	{
		return 0;
	}

	return flock(m_aiLockFDs[nLockID], LOCK_SH);
}

int _tagLock::WLockWait(short nLockID)
{
	if( nLockID < 0 || nLockID >= 32 || m_aiLockFDs[nLockID] < 0 )
	{
		return 0;
	}

	return flock(m_aiLockFDs[nLockID], LOCK_EX);
}

int _tagLock::UnLock(short nLockID)
{
	if( nLockID < 0 || nLockID >= 32 || m_aiLockFDs[nLockID] < 0 )
	{
		return 0;
	}

	return flock(m_aiLockFDs[nLockID], LOCK_UN);
}



CCodeQueue::CCodeQueue()
{
	m_stQueueHead.m_nBegin = 0;
	m_stQueueHead.m_nEnd = 0;
	m_stQueueHead.m_nSize = 0;
	m_stQueueHead.m_iCodeBufOffSet = -1;
}
CCodeQueue::CCodeQueue( int nTotalSize )
{
	m_stQueueHead.m_nBegin = 0;
	m_stQueueHead.m_nEnd = 0;
	m_stQueueHead.m_nSize = 0;
	m_stQueueHead.m_nFullFlag = 0;
	m_stQueueHead.m_iCodeBufOffSet = -1;
	m_stQueueHead.m_nLockIdx = -1;

	Initialize( nTotalSize );
}
CCodeQueue::CCodeQueue(int nTotalSize, const char *szLockFile, short nLockID)
{
	
	m_stQueueHead.m_nBegin = 0;
	m_stQueueHead.m_nEnd = 0;
	m_stQueueHead.m_nSize = 0;
	m_stQueueHead.m_nFullFlag = 0;
	m_stQueueHead.m_iCodeBufOffSet = -1;
	m_stQueueHead.m_nLockIdx = -1;

	Initialize( nTotalSize );
	

	if( !szLockFile || nLockID < 0 || nLockID >= 32 )
	{
		return;
	}

	
	if( !stFileLocks.SetLock(nLockID, szLockFile) )
	{
		m_stQueueHead.m_nLockIdx = nLockID;
	}
}

CCodeQueue::~CCodeQueue()
{
	free(m_pCurrentShm);
}

int CCodeQueue::Initialize( int nTotalSize )
{	
	m_stQueueHead.m_nSize = nTotalSize;
	m_stQueueHead.m_nBegin = 0;
	m_stQueueHead.m_nEnd = 0;
	m_stQueueHead.m_nFullFlag = 0;
	m_stQueueHead.m_nLockIdx = -1;	

	m_pCurrentShm = (BYTE *)(malloc((size_t)nTotalSize));
	m_stQueueHead.m_iCodeBufOffSet = (long)((BYTE *)m_pCurrentShm - (BYTE *)this); 

	return 0;
}

int CCodeQueue::Resume(int nTotalSize)
{
	m_pCurrentShm = (BYTE*)realloc(m_pCurrentShm,(size_t)nTotalSize);
	return 0;
}

int CCodeQueue::GetCriticalData(int *piBeginIdx, int *piEndIdx, short *pnFullFlag)
{
	if( stFileLocks.RLockWait(m_stQueueHead.m_nLockIdx) )
	{
		return -1;
	}
	
	if( piBeginIdx )
	{
		*piBeginIdx = m_stQueueHead.m_nBegin;
	}
	if( piEndIdx )
	{
		*piEndIdx = m_stQueueHead.m_nEnd;
	}
	if( pnFullFlag )
	{
		*pnFullFlag = m_stQueueHead.m_nFullFlag;
	}

	return stFileLocks.UnLock(m_stQueueHead.m_nLockIdx);
}

int CCodeQueue::SetCriticalData(int iBeginIdx, int iEndIdx, short nFullFlag)
{
	if( stFileLocks.WLockWait(m_stQueueHead.m_nLockIdx) )
	{
		return -1;
	}

	if( iBeginIdx >= 0 && iBeginIdx < m_stQueueHead.m_nSize )
	{
		m_stQueueHead.m_nBegin = iBeginIdx;
	}
	if( iEndIdx >= 0 && iEndIdx < m_stQueueHead.m_nSize )
	{
		m_stQueueHead.m_nEnd = iEndIdx;
	}
	if( nFullFlag == 0 )
	{
		m_stQueueHead.m_nFullFlag = nFullFlag;
	}
	else if( nFullFlag == 1 )
	{
		if( m_stQueueHead.m_nBegin == m_stQueueHead.m_nEnd )
		{
			m_stQueueHead.m_nFullFlag = nFullFlag;
		}
	}

	return stFileLocks.UnLock(m_stQueueHead.m_nLockIdx);
	
}

//Error code: -1 invalid para; -2 not enough; -3 data crashed
int CCodeQueue::AppendOneCode(const BYTE *pInCode, short sInLength)
{
	int nTempMaxLength = 0;
	int nTempRt = -1;
	int nTempBegin = -1;
	int nTempEnd = -1;
	short nTempFullFlag = -1;
	//char pcTempBuf[8192];
	BYTE *pbyCodeBuf;
	
	BYTE *pTempSrc = NULL;
	BYTE *pTempDst = NULL;
	int i;

	if( !pInCode || sInLength <= 0 )
	{
		return -1;
	}
	if(m_stQueueHead.m_nSize <= 0 )
	{
		printf("Error line:%d\n",__LINE__);
		return -1;
	}
	pbyCodeBuf = (BYTE *)((BYTE *)this + m_stQueueHead.m_iCodeBufOffSet);


	if( GetCriticalData(&nTempBegin, &nTempEnd, &nTempFullFlag) )
	{
		printf("Error line:%d\n",__LINE__);
		return -1;
	}

	printf("begin:%d\tend:%d\tflag:%d\n",nTempBegin,nTempEnd,nTempFullFlag);

	if( nTempFullFlag )			//if( m_stQueueHead.m_nFullFlag )
	{
		printf("Error line:%d\n",__LINE__);
		return -2;
	}

	if( nTempBegin < 0 || nTempBegin >= m_stQueueHead.m_nSize
		|| nTempEnd < 0 || nTempEnd >= m_stQueueHead.m_nSize )
	{
		//TLib_Log_LogMsg("In CCodeQueue::AppendOneCode, data crashed: begin = %d, end = %d",
		//				nTempBegin, nTempEnd);
		
		printf("In CCodeQueue::AppendOneCode, data crashed: begin = %d, end = %d\n",
						nTempBegin, nTempEnd);
		nTempBegin = nTempEnd = 0;
		nTempFullFlag = 0;
		SetCriticalData(nTempBegin, nTempEnd, 0);

		printf("Error line:%d\n",__LINE__);
		return -3;
	}

	
	if( nTempBegin == nTempEnd )
	{
		nTempBegin = nTempEnd = 0;
		nTempFullFlag = 0;
		SetCriticalData(nTempBegin, nTempEnd, 0);
		nTempMaxLength = m_stQueueHead.m_nSize;
	}
	else if( nTempBegin > nTempEnd )
	{
		nTempMaxLength = nTempBegin - nTempEnd;		
	}
	else
	{
		nTempMaxLength = (m_stQueueHead.m_nSize - nTempEnd) + nTempBegin;
	}

	nTempRt = nTempEnd;
	
	if( (int)(sInLength + sizeof(short)) > nTempMaxLength )
	{
		printf("Error line:%d\n",__LINE__);
		return -2;
	}

	pTempDst = &pbyCodeBuf[0];
	pTempSrc = (BYTE *)&sInLength;

	for( i = 0; i < (int)sizeof(sInLength); i++ )
	{
		pTempDst[nTempEnd] = pTempSrc[i];
		nTempEnd = (nTempEnd+1) % m_stQueueHead.m_nSize;
	}

	if( nTempBegin > nTempEnd )
	{
		memcpy((void *)&pbyCodeBuf[nTempEnd], (const void *)pInCode, (size_t)sInLength );
	}
	else
	{
		if( (int)sInLength > (m_stQueueHead.m_nSize - nTempEnd) )
		{
			memcpy((void *)&pbyCodeBuf[nTempEnd], (const void *)&pInCode[0], (size_t)(m_stQueueHead.m_nSize - nTempEnd) );
			memcpy((void *)&pbyCodeBuf[0],(const void *)&pInCode[(m_stQueueHead.m_nSize - nTempEnd)], (size_t)(sInLength + nTempEnd - m_stQueueHead.m_nSize) );
		}
		else
		{
			memcpy((void *)&pbyCodeBuf[nTempEnd], (const void *)&pInCode[0], (size_t)sInLength);
		}
	}
	nTempEnd = (nTempEnd + sInLength) % m_stQueueHead.m_nSize;


	if( nTempEnd == nTempBegin )
	{
		nTempFullFlag = 1;
	}

	SetCriticalData(-1, nTempEnd, nTempFullFlag);

	return nTempRt;
}

int CCodeQueue::PeekHeadCode(BYTE *pOutCode, short *psOutLength)
{
	int nTempMaxLength = 0;
	int nTempRet = -1;
	int nTempBegin = -1;
	int nTempEnd = -1;
	short nTempFullFlag = -1;
	BYTE *pTempSrc;
	BYTE *pTempDst;
	int i;
	BYTE *pbyCodeBuf;

	if( !pOutCode || !psOutLength )
	{
		return -1;
	}
	if( m_stQueueHead.m_nSize <= 0 )
	{
		return -1;
	}
	
	pbyCodeBuf = (BYTE *)((BYTE *)this + m_stQueueHead.m_iCodeBufOffSet);
	
	if( GetCriticalData(&nTempBegin, &nTempEnd, &nTempFullFlag) )
	{
		return -1;
	}
	nTempRet = nTempBegin;

	if( nTempBegin == nTempEnd && !nTempFullFlag )
	{
		*psOutLength = 0;
		return nTempRet;
	}


	if( nTempBegin < nTempEnd )
	{
		nTempMaxLength = nTempEnd - nTempBegin;
	}
	else
	{
		nTempMaxLength = m_stQueueHead.m_nSize - nTempBegin + nTempEnd;
	}

	if( nTempMaxLength < (int)sizeof(short) )
	{
		*psOutLength = 0;
		return -3;
	}

	pTempDst = (BYTE *)psOutLength;
	pTempSrc = (BYTE *)&pbyCodeBuf[0];
	for( i = 0; i < (int)sizeof(short); i++ )
	{
		pTempDst[i] = pTempSrc[nTempBegin];
		nTempBegin = (nTempBegin+1) % m_stQueueHead.m_nSize; 
	}

	if( (*psOutLength) > (int)(nTempMaxLength - sizeof(short)) )
	{
		*psOutLength = 0;
		return -3;
	}

	pTempDst = (BYTE *)&pOutCode[0];

	if( nTempBegin < nTempEnd )
	{
		memcpy((void *)pTempDst, (const void *)&pTempSrc[nTempBegin], (size_t)(*psOutLength));
	}
	else
	{
		//如果出现分片，则分段拷贝
		if( m_stQueueHead.m_nSize - nTempBegin < *psOutLength)
		{
			memcpy((void *)pTempDst, (const void *)&pTempSrc[nTempBegin], (size_t)(m_stQueueHead.m_nSize - nTempBegin));
			pTempDst += (m_stQueueHead.m_nSize - nTempBegin);
			memcpy((void *)pTempDst, (const void *)&pTempSrc[0], (size_t)(*psOutLength+nTempBegin-m_stQueueHead.m_nSize));
		}
		else	//否则，直接拷贝
		{
			memcpy((void *)pTempDst, (const void *)&pTempSrc[nTempBegin], (size_t)(*psOutLength));
		}
	}
	nTempBegin = (nTempBegin + (*psOutLength)) % m_stQueueHead.m_nSize;

	return nTempRet;
}

int CCodeQueue::GetHeadCode(BYTE *pOutCode, short *psOutLength)
{
	int nTempMaxLength = 0;
	int nTempRet = -1;
	int nTempBegin = -1;
	int nTempEnd = -1;
	short nTempFullFlag = -1;
	BYTE *pTempSrc;
	BYTE *pTempDst;
	int i;
	BYTE *pbyCodeBuf;

	if( !pOutCode || !psOutLength )
	{
		return -1;
	}
	if( m_stQueueHead.m_nSize <= 0 )
	{
		return -1;
	}

	pbyCodeBuf = (BYTE *)((BYTE *)this + m_stQueueHead.m_iCodeBufOffSet);
	
	if( GetCriticalData(&nTempBegin, &nTempEnd, &nTempFullFlag) )
	{
		return -1;
	}
	nTempRet = nTempBegin;

	if( nTempBegin == nTempEnd && !nTempFullFlag )
	{
		*psOutLength = 0;
		return nTempRet;
	}


	if( nTempBegin < nTempEnd )
	{
		nTempMaxLength = nTempEnd - nTempBegin;
	}
	else
	{
		nTempMaxLength = m_stQueueHead.m_nSize - nTempBegin + nTempEnd;
	}

	if( nTempMaxLength < (int)sizeof(short) )
	{
		*psOutLength = 0;
		nTempBegin = nTempEnd;
		SetCriticalData(nTempBegin, -1, 0);
		return -3;
	}

	pTempDst = (BYTE *)psOutLength;
	pTempSrc = (BYTE *)&pbyCodeBuf[0];
	for( i = 0; i < (int)sizeof(short); i++ )
	{
		pTempDst[i] = pTempSrc[nTempBegin];
		nTempBegin = (nTempBegin+1) % m_stQueueHead.m_nSize; 
	}

	if( (*psOutLength) > (int)(nTempMaxLength - sizeof(short)) || *psOutLength < 0 )
	{
		*psOutLength = 0;
		nTempBegin = nTempEnd;
		SetCriticalData(nTempBegin, -1, 0);
		return -3;
	}

	pTempDst = (BYTE *)&pOutCode[0];


	if( nTempBegin < nTempEnd )
	{
		memcpy((void *)pTempDst, (const void *)&pTempSrc[nTempBegin], (size_t)(*psOutLength));
	}
	else
	{
		//如果出现分片，则分段拷贝
		if( m_stQueueHead.m_nSize - nTempBegin < *psOutLength)
		{
			memcpy((void *)pTempDst, (const void *)&pTempSrc[nTempBegin], (size_t)(m_stQueueHead.m_nSize - nTempBegin));
			pTempDst += (m_stQueueHead.m_nSize - nTempBegin);
			memcpy((void *)pTempDst, (const void *)&pTempSrc[0], (size_t)(*psOutLength+nTempBegin-m_stQueueHead.m_nSize));
		}
		else	//否则，直接拷贝
		{
			memcpy((void *)pTempDst, (const void *)&pTempSrc[nTempBegin], (size_t)(*psOutLength));
		}
	}
	nTempBegin = (nTempBegin + (*psOutLength)) % m_stQueueHead.m_nSize;

	SetCriticalData(nTempBegin, -1, 0);

	return nTempRet;
}

int CCodeQueue::DeleteHeadCode()
{
	short sTempShort = 0;
	int nTempMaxLength = 0;
	int nTempRet = -1;
	int nTempBegin = -1;
	int nTempEnd = -1;
	short nTempFullFlag = -1;

	BYTE *pTempSrc;
	BYTE *pTempDst;
	BYTE *pbyCodeBuf;

	if( m_stQueueHead.m_nSize <= 0 )
	{
		return -1;
	}

	pbyCodeBuf = (BYTE *)((BYTE *)this + m_stQueueHead.m_iCodeBufOffSet);
	
	if( GetCriticalData(&nTempBegin, &nTempEnd, &nTempFullFlag) )
	{
		return -1;
	}
	nTempRet = nTempBegin;

	if( nTempBegin == nTempEnd && !nTempFullFlag )
	{
		return nTempRet;
	}


	if( nTempBegin < nTempEnd )
	{
		nTempMaxLength = nTempEnd - nTempBegin;
	}
	else
	{
		nTempMaxLength = m_stQueueHead.m_nSize - nTempBegin + nTempEnd;
	}
	
	if( nTempMaxLength < (int)sizeof(short) )
	{
		nTempBegin = nTempEnd;
		SetCriticalData(nTempBegin, -1, 0);
		return -3;
	}

	pTempDst = (BYTE *)&sTempShort;
	pTempSrc = (BYTE *)&pbyCodeBuf[0];
	for( int i = 0; i < (int)sizeof(short); i++ )
	{
		pTempDst[i] = pTempSrc[nTempBegin];
		nTempBegin = (nTempBegin+1) % m_stQueueHead.m_nSize; 
	}

	if( sTempShort > (int)(nTempMaxLength - sizeof(short)) || sTempShort < 0 )
	{
		nTempBegin = nTempEnd;
		SetCriticalData(nTempBegin, -1, 0);
		return -3;
	}

	nTempBegin = (nTempBegin + sTempShort) % m_stQueueHead.m_nSize;

	SetCriticalData(nTempBegin, -1, 0);
	
	return nTempBegin;
}

int CCodeQueue::GetOneCode(int iCodeOffset, short nCodeLength, BYTE *pOutCode, short *psOutLength)
{
	short nTempShort = 0;
	int iTempMaxLength = 0;
	int iTempBegin;
	int iTempEnd;
	short nTempFullFlag = -1;
	int iTempIdx;
	BYTE *pTempSrc;
	BYTE *pTempDst;
	BYTE *pbyCodeBuf;

	if( m_stQueueHead.m_nSize <= 0 )
	{
		return -1;
	}

	pbyCodeBuf = (BYTE *)((BYTE *)this + m_stQueueHead.m_iCodeBufOffSet);

	if( !pOutCode || !psOutLength )
	{
		//TLib_Log_LogMsg("In GetOneCode, invalid input paraments.\n");
		return -1;
	}

	if( iCodeOffset < 0 || iCodeOffset >= m_stQueueHead.m_nSize)
	{
		//TLib_Log_LogMsg("In GetOneCode, invalid code offset %d.\n", iCodeOffset);
		return -1;
	}
	if( nCodeLength < 0 || nCodeLength >= m_stQueueHead.m_nSize )
	{
		//TLib_Log_LogMsg("In GetOneCode, invalid code length %d.\n", nCodeLength);
		return -1;
	}

	if( GetCriticalData(&iTempBegin, &iTempEnd, &nTempFullFlag) )
	{
		return -1;
	}
	
	if( iTempBegin == iTempEnd && !nTempFullFlag )
	{
		*psOutLength = 0;
		return 0;
	}

	if( iTempBegin == iCodeOffset )
	{
		return GetHeadCode(pOutCode, psOutLength);
	}

	//TLib_Log_LogMsg("Warning: Get code is not the first one, there might be sth wrong.\n");

	if( iCodeOffset < iTempBegin || iCodeOffset >= iTempEnd )
	{
		//TLib_Log_LogMsg("In GetOneCode, code offset out of range.\n");
		*psOutLength = 0;
		return -1;
	}
	
	if( iTempBegin < iTempEnd )
	{
		iTempMaxLength = iTempEnd - iTempBegin;		
	}
	else
	{
		iTempMaxLength = m_stQueueHead.m_nSize - iTempBegin + iTempEnd;
	}

	if( iTempMaxLength < (int)sizeof(short) )
	{
		*psOutLength = 0;
		iTempBegin = iTempEnd;
		SetCriticalData(iTempBegin, -1, 0);
		return -3;
	}

	pTempDst = (BYTE *)&nTempShort;
	pTempSrc = (BYTE *)&pbyCodeBuf[0];
	iTempIdx = iCodeOffset;
	for( int i = 0; i < (int)sizeof(short); i++ )
	{
		pTempDst[i] = pTempSrc[iTempIdx];
		iTempIdx = (iTempIdx+1) % m_stQueueHead.m_nSize; 
	}

	if( nTempShort > (int)(iTempMaxLength - sizeof(short)) || nTempShort < 0 || nTempShort != nCodeLength )
	{
		//TLib_Log_LogMsg("Can't get code, code length not matched.\n");
		*psOutLength = 0;
		return -2;
	}

	SetCriticalData(iCodeOffset, -1, 0);
	
	return GetHeadCode( pOutCode, psOutLength );
}

int CCodeQueue::IsQueueEmpty()
{
	int iTempBegin;
	int iTempEnd;
	short nTempFullFlag = -1;

	GetCriticalData(&iTempBegin, &iTempEnd, &nTempFullFlag);

	if( iTempBegin == iTempEnd && !nTempFullFlag )
	{
		return True;
	}

	return False;
}

#endif
