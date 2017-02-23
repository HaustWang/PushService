#ifndef _BASE_CPP_
#define _BASE_CPP_

#include <math.h>
#include "base.h"

int TimeValPlus(timeval& tvA, timeval& tvB, timeval& tvResult)
{
	timeval tvTemp;
	tvTemp.tv_sec = tvA.tv_sec + tvB.tv_sec;
	tvTemp.tv_sec += ((tvA.tv_usec+tvB.tv_usec)/1000000);
	tvTemp.tv_usec = ((tvA.tv_usec+tvB.tv_usec)%1000000);

	tvResult.tv_sec = tvTemp.tv_sec;
	tvResult.tv_usec = tvTemp.tv_usec;

	return 0;
}

int TimeValMinus(timeval& tvA, timeval& tvB, timeval& tvResult)
{
	timeval tvTemp;

	if( tvA.tv_usec < tvB.tv_usec )
	{
		tvTemp.tv_usec = (1000000 + tvA.tv_usec) - tvB.tv_usec;
		tvTemp.tv_sec = tvA.tv_sec - tvB.tv_sec - 1;
	}
	else
	{
		tvTemp.tv_usec = tvA.tv_usec - tvB.tv_usec;
		tvTemp.tv_sec  = tvA.tv_sec - tvB.tv_sec;
	}

	tvResult.tv_sec = tvTemp.tv_sec;
	tvResult.tv_usec = tvTemp.tv_usec;

	return 0;
}

int SockAddrToString(sockaddr_in *pstSockAddr, char *szResult)
{
	char *pcTempIP = NULL;
	unsigned short nTempPort = 0;

	if( !pstSockAddr || !szResult )
	{
		return -1;
	}

	pcTempIP = inet_ntoa(pstSockAddr->sin_addr);

	if( !pcTempIP )
	{
		return -1;
	}

	nTempPort = ntohs(pstSockAddr->sin_port);

	sprintf(szResult, "%s:%d", pcTempIP, nTempPort);

	return 0;
}

void TrimStr( char *strInput )
{
	char *pb;
	char *pe;
	int iTempLength;

	if( strInput == NULL )
	{
		return;
	}

	iTempLength = strlen(strInput);
	if( iTempLength == 0 )
	{
		return;
	}

	pb = strInput;

	while (((*pb == ' ') || (*pb == '\t') || (*pb == '\n') || (*pb == '\r')) && (*pb != 0))
	{
		pb ++;
	}

	pe = &strInput[iTempLength-1];
	while ((pe >= pb) && ((*pe == ' ') || (*pe == '\t') || (*pe == '\n') || (*pe == '\r')))
	{
		pe --;
	}

	*(pe+1) = '\0';

	strcpy( strInput, pb );

	return;
}

CFlag::CFlag()
{
	m_byFlag = (BYTE)0;
}
CFlag::~CFlag()
{
}
int CFlag::Initial( BYTE byExFlag )
{
	m_byFlag = byExFlag;
	return 0;
}
unsigned char CFlag::GetAllFlag()
{
	return m_byFlag;
}
int CFlag::ClearFlag(BYTE byExFlag)
{
	BYTE byTemp;

	byTemp = byExFlag ^ 0xff;
	m_byFlag = m_byFlag & byTemp;

	return 0;
}
int CFlag::SetFlag( BYTE byExFlag )
{
	m_byFlag = m_byFlag | byExFlag;
	return 0;
}
int CFlag::IsFlagSet(BYTE byExFlag)
{
	if( byExFlag & m_byFlag )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

CFileLock::CFileLock()
{
	int i;

	for( i = 0; i < MAX_FILE_LOCK; i++ )
	{
		m_aiLockFDs[i] = -1;
	}
}
CFileLock::~CFileLock()
{
}
int CFileLock::SetLock(int iLockIdx, const char *szFileName)
{
	FILE *fp;
	int  iTempFD;

	if( !szFileName || iLockIdx < 0 || iLockIdx >= MAX_FILE_LOCK )
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

	m_aiLockFDs[iLockIdx] = iTempFD;

	return 0;
}
int CFileLock::RLockWait(int iLockIdx)
{
	int iTempRet;

	if( iLockIdx < 0 || iLockIdx >= MAX_FILE_LOCK || m_aiLockFDs[iLockIdx] < 0 )
	{
		return 0;
	}

	iTempRet = flock(m_aiLockFDs[iLockIdx], LOCK_SH);

	return iTempRet;
}
int CFileLock::WLockWait(int iLockIdx)
{
	int iTempRet;

	if( iLockIdx < 0 || iLockIdx >= MAX_FILE_LOCK || m_aiLockFDs[iLockIdx] < 0 )
	{
		return 0;
	}

	iTempRet = flock(m_aiLockFDs[iLockIdx], LOCK_EX);

	return iTempRet;
}
int CFileLock::UnLock(int iLockIdx)
{
	int iTempRet;

	if( iLockIdx < 0 || iLockIdx >= MAX_FILE_LOCK || m_aiLockFDs[iLockIdx] < 0 )
	{
		return 0;
	}

	iTempRet = flock(m_aiLockFDs[iLockIdx], LOCK_UN);

	return iTempRet;
}

void ConvertHashToStr(BYTE *pbyHash, char *pszHash)
{
	int i;
	char szTemp[4];

	for (i = 0; i < HASH_LEN; i++)
	{
		memset(szTemp, 0, sizeof(szTemp));
		sprintf(szTemp, "%x", pbyHash[i]);
		if (1 == strlen(szTemp))
		{
			memcpy(pszHash + 2 * i, "0", 1);
			memcpy(pszHash + 2 * i + 1, szTemp, 1);
		}
		else
		{
			memcpy(pszHash + 2 * i, szTemp, 2);

		}
	}
	pszHash[32] = 0;
}

int ConvertStrToHash(char *pszHash, BYTE *pbyHash)
{
	if (NULL == pszHash || NULL == pbyHash)
	{
		//CWriteRunInfo::WriteErrLog("ConvertStrToHash error input");
		return -1;
	}
	if (2 * HASH_LEN != strlen(pszHash))
	{
		//CWriteRunInfo::WriteErrLog("ConvertStrToHash error pszHash [%s] len = [%d]", pszHash, strlen(pszHash));
		return -1;
	}
	int i;
	int j;
	char szNum[2];
	char  nNum[2];

	for (i = 0; i < HASH_LEN; i++)
	{
		memcpy(szNum, pszHash + 2 * i, 2);

		for (j = 0; j < 2; j++)
		{
			if ('0' <= szNum[j] && '9' >=szNum[j])
			{
				nNum[j] = szNum[j] - '0';
				continue;
			}
			if ('A' <= szNum[j] && 'F' >=szNum[j])
			{
				nNum[j] = szNum[j] - 'A' + 10;
				continue;
			}
			if ('a' <= szNum[j] && 'f' >=szNum[j])
			{
				nNum[j] = szNum[j] - 'a' + 10;
				continue;
			}
			//CWriteRunInfo::WriteErrLog("ConvertStrToHash invalid pszHash");
			return -1;
		}
		pbyHash[i] = (BYTE )(16 * nNum[0] + nNum[1]);
	}

	return 0;
}

int GetStructTime(time_t tTime, TStruTime *pstTime)
{
	struct tm *pTempTm = NULL;
	struct tm stTempTm;
	time_t timer = tTime;

	if( !pstTime )
	{
		return -1;
	}

	pTempTm = localtime_r( &timer, &stTempTm );

	if( !pTempTm )
	{
		return -1;
	}

	pstTime->m_iYear = stTempTm.tm_year + 1900;
	pstTime->m_iMon = stTempTm.tm_mon + 1;
	pstTime->m_iDay = stTempTm.tm_mday;
	pstTime->m_iHour = stTempTm.tm_hour;
	pstTime->m_iMin = stTempTm.tm_min;
	pstTime->m_iSec = stTempTm.tm_sec;
	pstTime->m_iMSec = 0;

	return 0;
}

int GetCurStructTime(TStruTime *pstTime)
{
	time_t tTempNow;

	time( &tTempNow );

	return GetStructTime(tTempNow, pstTime);
}

char *GetCurVersion(unsigned int unVer)
{
	static char szVersion[64];
	char szVer[32];
	sprintf(szVer,"%d",unVer);

	sprintf(szVersion," %c.%c ",szVer[0],szVer[1]);
	if (szVer[2] == '0')
	{
		strcat(szVersion,"Alpha");
	}
	else if (szVer[2] == '1')
	{
		strcat(szVersion,"Beta");
	}
	else
	{
		strcat(szVersion,"Release");
	}
	sprintf(szVersion+strlen(szVersion),"%c%c Build%c%c%c",szVer[3],szVer[4],szVer[5],szVer[6],szVer[7]);
	sprintf(szVersion+strlen(szVersion)," (%d)",unVer);

	return &szVersion[0];
}

int SetSockBufLen(int iSock, int iLength)
{
	int iRcvLen, iSndLen, iErr, iOptLen = sizeof(int);
	if( iSock < 0 || iLength < 0 )
	{
		return -1;
	}

	iRcvLen = iSndLen = iLength;
	iErr = setsockopt(iSock, SOL_SOCKET, SO_RCVBUF, &iRcvLen, iOptLen);
	if( iErr )
	{
		return -2;
	}
	iErr = setsockopt(iSock, SOL_SOCKET, SO_SNDBUF, &iSndLen, iOptLen);
	if( iErr )
	{
		return -3;
	}

	return 0;
}

int GetNowTime(time_t timer, int &nHour, int &nMinute)
{
	struct tm *pTempTm = NULL;

	pTempTm = localtime( &timer );

	if( !pTempTm )
	{
		return -1;
	}

	nHour   = pTempTm->tm_hour;
	nMinute = pTempTm->tm_min;
	return 0;
}

int GetTimeString(time_t timer, char *strTime)
{
	struct tm *pTempTm = NULL;

	if( !strTime )
	{
		return -1;
	}

	strTime[0] = '\0';

	pTempTm = localtime( &timer );

	if( !pTempTm )
	{
		return -1;
	}

	sprintf(strTime, "%04d-%02d-%02d %02d:%02d:%02d", pTempTm->tm_year + 1900, pTempTm->tm_mon + 1,
		pTempTm->tm_mday, pTempTm->tm_hour, pTempTm->tm_min, pTempTm->tm_sec);

	return 0;
}

int GetDateString(char *strTime)
{
	struct tm *pTempTm = NULL;
	struct tm stTempTm;
	time_t timer;

	if( !strTime )
	{
		return -1;
	}

	time( &timer );

	strTime[0] = '\0';

	pTempTm = localtime_r( &timer, &stTempTm );

	if( !pTempTm )
	{
		return -1;
	}

	sprintf(strTime, "%04d-%02d-%02d", stTempTm.tm_year + 1900, stTempTm.tm_mon + 1, stTempTm.tm_mday);

	return 0;
}

int SetNBlock(int iSock)
{
	int iFlags;
	iFlags = fcntl(iSock, F_GETFL, 0);
	iFlags |= O_NONBLOCK;
	iFlags |= O_NDELAY;
	fcntl(iSock, F_SETFL, iFlags);

	return 0;
}
const char * GetPeerAddrStr(sockaddr_in addr)
{
    static char saddr[32];
    memset(saddr, 0, sizeof(saddr));
    sprintf(saddr, "%s:%hu", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
    return saddr;
}

std::string GetStringFromInt(int64_t value)
{
    char tmp[50];
    snprintf(tmp, sizeof(tmp), "%ld", value);
    return std::string(tmp);
}

struct sockaddr * make_sock_addr(const char *ip, int port, int& addrlen)
{
	if(!ip)
		return NULL;

	static struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if(inet_pton(AF_INET,ip,&addr.sin_addr)<=0)
	{
		return NULL;
    }

	addrlen = sizeof(addr);
	return (sockaddr*)&addr;
}

typedef unsigned char BYTE;

inline BYTE toHex(const BYTE &x)
{
    return x > 9 ? x -10 + 'A': x + '0';
}

inline BYTE fromHex(const BYTE &x)
{
    return isdigit(x) ? x-'0' : x-'A'+10;
}

std::string URLEncode(const std::string &sIn)
{
    std::string sOut;
    for( size_t ix = 0; ix < sIn.size(); ix++ )
    {
        BYTE buf[4];
        memset( buf, 0, 4 );
        if( isalnum( (BYTE)sIn[ix] ) )
        {
            buf[0] = sIn[ix];
        }
        //else if ( isspace( (BYTE)sIn[ix] ) ) //貌似把空格编码成%20或者+都可以
        //{
        //    buf[0] = '+';
        //}
        else
        {
            buf[0] = '%';
            buf[1] = toHex( (BYTE)sIn[ix] >> 4 );
            buf[2] = toHex( (BYTE)sIn[ix] % 16);
        }
        sOut += (char *)buf;
    }
    return sOut;
};

std::string URLDecode(const std::string &sIn)
{
    std::string sOut;
    for( size_t ix = 0; ix < sIn.size(); ix++ )
    {
        BYTE ch = 0;
        if(sIn[ix]=='%')
        {
            ch = (fromHex(sIn[ix+1])<<4);
            ch |= fromHex(sIn[ix+2]);
            ix += 2;
        }
        else if(sIn[ix] == '+')
        {
            ch = ' ';
        }
        else
        {
            ch = sIn[ix];
        }
        sOut += (char)ch;
    }
    return sOut;
}
bool IsPrime(int n)
{
    if (n <= 1)
    {
        return false;
    }

    int sqrtn = (int)sqrt(n);
    for (int i = 2; i <= sqrtn; i++)
    {
        // n is divisible by an integer other than 1 and itself.
        if ((n % i) == 0)
            return false;
    }

    return true;
}

void SplitString(const std::string& src, const std::string& separator, std::vector<std::string>& dest)
{
    std::string str = src;
    std::string substring;
    std::string::size_type start = 0, index;

    do
    {
        index = str.find_first_of(separator,start);
        if (index != std::string::npos)
        {
            substring = str.substr(start,index-start);
            dest.push_back(substring);
            start = str.find_first_not_of(separator,index);
            if (start == std::string::npos) return;
        }
    }while(index != std::string::npos);

    //the last token
    substring = str.substr(start);
    dest.push_back(substring);
}

 void ReplaceStr(std::string& name,char e)
{
        std::string::size_type startpos = 0;
        while (startpos!= std::string::npos)
        {
                startpos = name.find(e);   //找到'.'的位置
                if( startpos != std::string::npos ) //std::string::npos表示没有找到该字符
                {
                        name.replace(startpos,1,""); //实施替换，注意后面一定要用""引起来，表示字符串
                }
        }
}


#endif
