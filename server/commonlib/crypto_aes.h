#pragma once
#include <openssl/aes.h>
#include <openssl/evp.h>
#include <string>

enum CryptMode
{
    AES_ECB,
    AES_CBC,
    AES_CFB128,
    AES_OFB,
};

class AES
{
public:
    AES();
    AES(std::string const& key, CryptMode mode = AES_ECB, bool bPadding = false);
    ~AES();

    void SetCryptKey(std::string const& key);
    void SetCryptMode(CryptMode mode);
    void SetPadding(bool bPadding);
    int Encrypt(std::string const& in, std::string& out);
    int Decrypt(std::string const& in, std::string& out);

    std::string GetCryptKey();
private:
    const EVP_CIPHER* GetCipherType(CryptMode mode);

private:
    EVP_CIPHER_CTX aes_enc_ctx;
    EVP_CIPHER_CTX aes_dec_ctx;
    CryptMode mode;
    unsigned char iv[EVP_MAX_IV_LENGTH];
    unsigned char ckey[EVP_MAX_KEY_LENGTH];
    size_t key_len;
};
