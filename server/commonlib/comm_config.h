#ifndef __COMM_CONFIGFILE_H__
#define __COMM_CONFIGFILE_H__

#include <stdint.h>
#include <stdio.h>
class CConfigFile
{
public:

    CConfigFile();
    ~CConfigFile();

	int32_t open_file(const char* pszFileName);
	void close_file();
    bool opened();

	//get value by key with specified section name
    unsigned int get_value_of( const char *pszSectionName, const char* pszKeyName, char* pszResult, unsigned int iSize);
    unsigned int get_value_of( const char *pszSectionName, const char* pszKeyName, int32_t& lKeyValue);
    unsigned int get_value_of( const char *pszSectionName, const char* pszKeyName, int32_t& lKeyValue, int32_t lDefaultKeyValue);
	unsigned int get_value_of( const char *pszSectionName, const char* pszKeyName, int64_t& lKeyValue);
	unsigned int get_value_of( const char *pszSectionName, const char* pszKeyName, int64_t& lKeyValue, int64_t lDefaultKeyValue);
    unsigned int get_value_of( const char *pszSectionName, const char* pszKeyName, char* pszResult, unsigned int iSize,const char *pszDefaultValue);

    unsigned int get_value_of( const char *pszSectionName, const char* pszKeyName, uint16_t& nKeyValue);
    unsigned int get_value_of( const char *pszSectionName, const char* pszKeyName, uint16_t& nKeyValue, uint16_t nDefaultKeyValue);


protected:
	char* get_sub_string(const char* s1, const char* s2);
	char* find_string(const char* pszSubString,const char* pszStart, const char *pszEnd);

	char* mirror2src(const char* mirror);
	char* src2mirror(const char* src);
	void lower_string(char*  pszSrcString, size_t size);

	//get the start & end of section of pszSectionName in config file buffer
	//pszSectionName: the name of the specified section: format: [SectionName]
    bool get_section_of(const char* pszSectionName, char* &pszSectionBegin, char* &pszSectionEnd);

	//get the range of key()
    bool get_key_zone_of(const char* pszKeyName, const char* pszSectionStart, const char* pszSectionEnd,
                        char* &pszKeyStart, char* &pszKeyEnd);

	//get the value by key (pszKeyName)
    bool get_value_zone_by_key(const char* pszKeyName, const char* pszSectionStart, const char* pszSectionEnd,
                        char* &pszValueStart, char* &pszValueEnd);

protected :
	char* m_pszFileName;		 //the base name of config file, without directory
	size_t  m_unFileSize;        //the size of config file
	char* m_pszFileContent;      //config file content
	char* m_pszFileMirror;		 //the mirror of config file

	bool   m_blOpened;			 //whether was opened ?


};

#endif

