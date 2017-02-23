#ifndef _CRYPTOTEST_H_
#define _CRYPTOTEST_H_

#include <string>

using namespace std;

typedef enum {
	GENERAL = 0,
	ECB = 0x1,
	CBC = 0x2,
	CFB = 0x3,
	TRIPLE_ECB = 0x4,
	TRIPLE_CBC = 0x5
}CRYPTO_MODE;

string DES_Encrypt(const string cleartext, const string key, CRYPTO_MODE mode);
string DES_Decrypt(const string ciphertext, const string key, CRYPTO_MODE mode);
#endif 
