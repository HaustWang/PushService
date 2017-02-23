#include <string.h>
#include <stdlib.h>
#include "comm_config.h"


CConfigFile::CConfigFile()
{
    m_pszFileName = NULL;
	m_unFileSize = 0;

    m_pszFileContent = NULL;
    m_pszFileMirror = NULL;
    
    m_blOpened = false;
}

CConfigFile::~CConfigFile()
{
	close_file();
}

bool CConfigFile::opened()
{
	return m_blOpened;
}

int CConfigFile::open_file(const char *pszFileName)
{   
	//first, close the config file   
    close_file();

	//
    if (NULL == pszFileName)
    {
        return -1;
    }

    m_pszFileName = strdup(pszFileName);

	FILE* fp = NULL;
    fp = fopen(m_pszFileName, "rb");
    if (NULL == fp)
    {
        return -1;
    }

	int  ret = -1;
    ret = fseek(fp, 0L, SEEK_END);
    if (0 != ret)
    {
        fclose(fp);
        return -1;
    }

	long size = 0;
    size = ftell(fp);
    m_pszFileContent = (char *)new char [size+1];
    m_pszFileMirror = (char *)new char [size+1];

    if ((NULL == m_pszFileContent) || (NULL == m_pszFileMirror))
    {
        fclose(fp);
        return -1;
    }

    ret = fseek(fp, 0L, SEEK_SET);
    if (0 != ret)
    {
        fclose(fp);
        return -1;
    }

    m_unFileSize = fread(m_pszFileContent, 1, size, fp);
    m_pszFileContent[m_unFileSize] = '\0';

    //
    memcpy(m_pszFileMirror, m_pszFileContent, m_unFileSize + 1);
    lower_string(m_pszFileMirror, m_unFileSize + 1);

    fclose(fp);
    m_blOpened = true;

    return 0;
}

void CConfigFile::close_file()
{
	m_unFileSize = 0;
	m_blOpened = false;

    if (m_pszFileName)
    {
        free(m_pszFileName);
        m_pszFileName = NULL;
    }

    if (m_pszFileContent)
    {
        delete [] m_pszFileContent;
        m_pszFileContent = NULL;
    }

    if (m_pszFileMirror)
    {
        delete [] m_pszFileMirror;
        m_pszFileMirror = NULL;
    }
   
}

//
unsigned int CConfigFile::get_value_of(const char *pszSectionName, const char* pszKeyName,
									   char* pszResult, unsigned int iSize)
{
	if (NULL == pszSectionName || NULL == pszKeyName || NULL == pszResult)
	{
		return 0;
	}

	if (!opened() || 0 == iSize)
	{
		return 0;
	}

	char* pszSectionStart = NULL; 
	char* pszSectionEnd = NULL;

	//get start & end of section(pszSectionName)
	if (!get_section_of(pszSectionName, pszSectionStart, pszSectionEnd))
	{
		return 0;
	}

	char* pszValueStart = NULL;
	char* pszValueEnd = NULL;

	//
	if (!get_value_zone_by_key(pszKeyName, pszSectionStart, pszSectionEnd, pszValueStart, pszValueEnd))
	{
		return 0;
	}


	size_t tmpSize = 0;;
	for (/**/; pszValueStart < pszValueEnd && tmpSize < (iSize -1); pszValueStart++, tmpSize++)
	{
		pszResult[tmpSize] = (*pszValueStart);
	}

	//terminated by null
	pszResult[tmpSize] = '\0';

	if ((unsigned char) pszResult[tmpSize-1] > 0x7f && (iSize-1) == tmpSize)
	{
		pszResult[tmpSize-1] = '\0';
		--tmpSize;
	}

	return (unsigned int)tmpSize;
}

//
unsigned int CConfigFile::get_value_of( const char *pszSectionName, const char *pszKeyName, int32_t& lKeyValue)
{
	char szValue[256] = {'\0'};
	if (0 == get_value_of(pszSectionName, pszKeyName, szValue, sizeof(szValue)))
	{
		return 0;
	}

	lKeyValue = (int32_t)atol(szValue);
	return 1;
}

//
unsigned int CConfigFile::get_value_of(const char *pszSectionName, const char* pszKeyName, int32_t& lKeyValue, int32_t lDefaultKeyValue)
{
    if (0 == get_value_of(pszSectionName, pszKeyName, lKeyValue))
    {
        lKeyValue = lDefaultKeyValue;
        return 0;
    }

    return 1;
}

unsigned int CConfigFile::get_value_of( const char *pszSectionName, const char* pszKeyName, uint16_t& nKeyValue)
{
	char szValue[256] = {'\0'};
	if (0 == get_value_of(pszSectionName, pszKeyName, szValue, sizeof(szValue)))
	{
		return 0;
	}

	nKeyValue = (uint16_t)atoi(szValue);
	return 1;
}

unsigned int CConfigFile::get_value_of( const char *pszSectionName, const char* pszKeyName, uint16_t& nKeyValue, uint16_t nDefaultKeyValue)
{
    if (0 == get_value_of(pszSectionName, pszKeyName, nKeyValue))
    {
        nKeyValue = nDefaultKeyValue;
        return 0;
    }
    return 1;
}

unsigned int CConfigFile::get_value_of( const char *pszSectionName, const char *pszKeyName, int64_t& lKeyValue )
{
	char szValue[256] = {'\0'};
	if (0 == get_value_of(pszSectionName, pszKeyName, szValue, sizeof(szValue)))
	{
		return 0;
	}

	lKeyValue = atoll(szValue);
	return 1;
}


unsigned int CConfigFile::get_value_of( const char *pszSectionName, const char *pszKeyName, int64_t& lKeyValue, int64_t lDefaultKeyValue)
{
    if (0 == get_value_of(pszSectionName, pszKeyName, lKeyValue))
    {
        lKeyValue = lDefaultKeyValue;
        return 0;
    }
    return 1;
}

//
unsigned int CConfigFile::get_value_of(const char *pszSectionName, const char* pszKeyName, char* pszResult, unsigned int iSize,const char *pszDefaultValue)
{
  	if (0 >= iSize)
	{
		return 0;
	}

	unsigned int tmpSize = 0;
    tmpSize = get_value_of(pszSectionName, pszKeyName, pszResult, iSize);
    if (0 == tmpSize)
    {
        strncpy(pszResult, pszDefaultValue, iSize-1);
        pszResult[iSize-1] = '\0';
		tmpSize = strlen(pszResult);
    }

    return tmpSize;
}


//
bool CConfigFile::get_section_of( const char *pszSectionName, char* &pszSectionBegin,
                                 char* &pszSectionEnd )
{
	if (NULL == pszSectionName)
	{
		return false;
	}

	if ((NULL == m_pszFileContent) || (NULL == m_pszFileMirror))
	{
		return false;
	}
    
	char* pszLowerSection = NULL;
    size_t nameSize = strlen(pszSectionName) +3;
    pszLowerSection = new char [nameSize];
	if (NULL == pszLowerSection)
	{
		return false;
	}

	snprintf(pszLowerSection, nameSize, "[%s]", pszSectionName);
  
    lower_string(pszLowerSection, strlen(pszLowerSection));

    //find the section by section name 
	char* pszSectionStartInMirror = NULL;
    pszSectionStartInMirror = find_string(pszLowerSection, m_pszFileMirror, (m_pszFileMirror + m_unFileSize ));
    if (NULL == pszSectionStartInMirror)//not find the section
    {
        delete [] pszLowerSection;
        return false;
    }

    pszSectionBegin = mirror2src(pszSectionStartInMirror) + strlen(pszLowerSection);
    for ( /* nothing*/; pszSectionBegin < (m_pszFileContent + m_unFileSize); pszSectionBegin++)
    {
        if (( '\r' == *pszSectionBegin) || ('\n' == *pszSectionBegin))
        {
            break;
        }
    }

    
    for (/**/; pszSectionBegin < (m_pszFileContent + m_unFileSize); pszSectionBegin++)
    {
        if (('\r' != *pszSectionBegin) && ('\n' != *pszSectionBegin))
        {
            break;
        }
    }

	//free the allocated memory
    delete [] pszLowerSection;

	bool bValidHeadCharater = true;
	char* pszTmp = NULL;
    pszTmp = pszSectionBegin;
    for (/**/; pszTmp < (m_pszFileContent + m_unFileSize + 1); pszTmp++)
    {
        if (bValidHeadCharater && '[' == *pszTmp)
        {
            break;
        }

        if ('\0' == *pszTmp)
        {
            break;
        }

        if ('\r' == *pszTmp ||'\n' == *pszTmp)
        {
            bValidHeadCharater = true;
        }
        else if ((' ' != *pszTmp) && ('\t' != *pszTmp))
        {
            bValidHeadCharater = false;
        }
    }

    pszSectionEnd = pszTmp;

    return true;
}


bool CConfigFile::get_key_zone_of( const char* pszKeyName, const char* pszSectionStart, const char* pszSectionEnd,
										  char* &pszKeyStart, char* &pszKeyEnd)
{
	if (NULL == pszKeyName || NULL == pszSectionStart || NULL == pszSectionEnd)
	{
		return false;
	}
 
	if (pszSectionEnd <= pszKeyStart)
	{
		return false;
	}

    char* pszSmallKey = NULL;
    pszSmallKey = strdup(pszKeyName);
    lower_string(pszSmallKey, strlen(pszSmallKey));

	char* pszKeyMirrorStart = NULL;
	char* pszSectionMirrorStart = src2mirror(pszSectionStart);
	char* pszSectionMirrorEnd = src2mirror(pszSectionEnd);
    pszKeyMirrorStart = find_string(pszSmallKey, pszSectionMirrorStart, pszSectionMirrorEnd);
    if (NULL == pszKeyMirrorStart)
    {
        free(pszSmallKey);
        return false;
    }

	//free memory
    free(pszSmallKey);

	//set key range
    pszKeyStart = mirror2src(pszKeyMirrorStart);
    pszKeyEnd = pszKeyStart + strlen(pszKeyName);

   
    for (/**/; pszKeyEnd < pszSectionEnd; pszKeyEnd++)
    {
        if ((' ' != *pszKeyEnd) && ('\t' != *pszKeyEnd))
        {
            break;
        }
    }

	//unfortunately, find the invalid range, so just iterator to do 
    if ('=' != *pszKeyEnd)
    {
        char* pszTmpStart = pszKeyEnd;                                
	    return get_key_zone_of( pszKeyName, pszTmpStart, pszSectionEnd, pszKeyStart, pszKeyEnd );
    }
   
    for (pszKeyEnd++; pszKeyEnd < pszSectionEnd; pszKeyEnd++)
    {
		if ('\r' == *pszKeyEnd || '\n' == *pszKeyEnd)
		{
			break;
		}
    }

	for (/**/; pszKeyEnd < pszSectionEnd; pszKeyEnd++)
    {
     
		if ('\r' != *pszKeyEnd && '\n' != *pszKeyEnd)
		{
			break;
		}
    }

    if (pszKeyEnd > pszKeyStart)
    {
        return true;
    }
    else
    {
        return false;
    }

	return true;
}

//the config file entry format: key = value 
bool CConfigFile::get_value_zone_by_key(const char* pszKeyName, const char* pszSectionStart, const char* pszSectionEnd,
												char* &pszValueStart, char* &pszValueEnd)
{
	if (NULL == pszKeyName || NULL == pszSectionStart || NULL == pszSectionEnd)
	{
		return false;
	}

    if (pszSectionEnd <= pszSectionStart)
    {
		return false;
    }
    
	//
	char* pszSmallKeyChar = NULL;
    pszSmallKeyChar = strdup(pszKeyName); //attention: would malloc some memory
    lower_string(pszSmallKeyChar, strlen(pszSmallKeyChar));

    char* pszKeyMirrorStart = NULL;
	char* pszSectionMirrorStart = src2mirror(pszSectionStart);
	char* pszSectionMirrorEnd = src2mirror(pszSectionEnd);

    //
    pszKeyMirrorStart = find_string(pszSmallKeyChar, pszSectionMirrorStart, pszSectionMirrorEnd);
    if (NULL == pszKeyMirrorStart)
    {
        free(pszSmallKeyChar);
        return false;
    }

	//free memory 
    free(pszSmallKeyChar);

    pszValueStart = mirror2src(pszKeyMirrorStart) + strlen(pszKeyName);
    
    for (/**/; pszValueStart < pszSectionEnd; pszValueStart++)
    {
		if (' ' != *pszValueStart && '\t' != *pszValueStart)
		{
			break;
		}
    }

    if ('=' != *pszValueStart)
    {
        char* pszTmpStart = pszValueStart;      
        return get_value_zone_by_key( pszKeyName, pszTmpStart, pszSectionEnd, pszValueStart, pszValueEnd );
    }
   
    for (pszValueStart++; pszValueStart < pszSectionEnd; pszValueStart++)
    {
		if ( '\n' == *pszValueStart || '\r' == *pszValueStart || ( '\t' != *pszValueStart && ' ' != *pszValueStart))
		{
			break;
		}
    }

    pszValueEnd = pszValueStart;

    for (/**/; pszValueEnd < pszSectionEnd; pszValueEnd++)
    {
  		if ('\n' == *pszValueEnd || '\r' == *pszValueEnd)
		{
			break;
		}
    }

    if (pszValueEnd > pszValueStart)
    {
        return true;
    }
    else
    {
        return false;
    }

	return true;
}


char* CConfigFile::find_string(const char* pszSubString,const char* pszStart, const char *pszEnd)
{
	if (NULL == pszSubString || NULL == pszStart || NULL == pszEnd)
	{
		return NULL;
	}

    if (pszStart >= pszEnd)
    {
        return NULL;
    }

    //find substring
	char* pszResult = NULL;
    pszResult = get_sub_string(pszStart, pszSubString);
	if (NULL != pszResult && (pszResult + strlen(pszSubString)) > pszEnd)
	{
		pszResult = NULL;
	}

	return pszResult;
}

char *CConfigFile::get_sub_string(const char* s1, const char* s2)
{
	if (NULL == s1 || NULL == s2)
	{
		return NULL;
	}

    char* pszTmp = NULL;
    char* pszTmp2 = NULL;

    const char *pszStart = s1;

    while(true)
    {
        pszTmp = (char*)strstr(pszStart, s2);
        if (NULL == pszTmp)
        {
            return NULL;
        }

        if (pszStart < pszTmp)
        {
            pszTmp2 = (pszTmp - 1);
            if ('\r' != *pszTmp2 && '\n' != *pszTmp2 &&' ' != *pszTmp2 && '\t' != *pszTmp2)
            {
                pszStart = pszTmp + strlen(s2);
                continue;
            }
        }

        pszTmp2 = pszTmp + strlen(s2);
        if ('\r' == *pszTmp2|| '\n' == *pszTmp2 || ' ' == *pszTmp2 || '=' == *pszTmp2 || '\t' == *pszTmp2)
        {
            return pszTmp;
        }

        pszStart = pszTmp + strlen(s2);
    }

    return NULL;
}

char *CConfigFile::src2mirror(const char* src)
{
    return (m_pszFileMirror + (src - m_pszFileContent));
}


void CConfigFile::lower_string(char* pszSrcString, size_t size)
{
	if (NULL == pszSrcString)
    {
        return;
    }

	unsigned char character = 0;
    for (size_t i = 0; i < size; ++i)
    {
        character = *(unsigned char *)(pszSrcString + i);
        if (character >= 'A' && character <= 'Z')
        {
            *(unsigned char *)(pszSrcString + i) = (unsigned char)(character + 0x20);
        }
    }//
	
	return;
}

char* CConfigFile::mirror2src( const char* mirror )
{
	return (m_pszFileContent + (mirror - m_pszFileMirror));
}

