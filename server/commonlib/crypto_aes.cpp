#include "crypto_aes.h"

AES::AES()
{
    EVP_CIPHER_CTX_init(&aes_enc_ctx);
    EVP_CIPHER_CTX_init(&aes_dec_ctx);

    this->mode = AES_ECB;

    key_len = 16;
    SetPadding(false);
}

AES::AES(std::string const& key, CryptMode mode, bool bPadding)
{
    this->mode = mode;

    EVP_CIPHER_CTX_init(&aes_enc_ctx);
    EVP_CIPHER_CTX_init(&aes_dec_ctx);

    SetCryptKey(key);
    SetPadding(false);
}

AES::~AES()
{
    EVP_CIPHER_CTX_cleanup(&aes_enc_ctx);
    EVP_CIPHER_CTX_cleanup(&aes_dec_ctx);
}

const EVP_CIPHER* AES::GetCipherType(CryptMode mode)
{
    const EVP_CIPHER *cipher_type;
    switch(mode)
    {
        case AES_ECB:
            cipher_type = EVP_aes_128_ecb();
            break;
        case AES_CBC:
            cipher_type = EVP_aes_128_cbc();
            break;
        case AES_CFB128:
            cipher_type = EVP_aes_128_cfb();
            break;
        case AES_OFB:
            cipher_type = EVP_aes_128_ofb();
            break;
        default:
            cipher_type = EVP_aes_128_ecb();
            break;
    }

    return cipher_type;
}

void AES::SetCryptKey(std::string const& key)
{
    memset(ckey, 0, EVP_MAX_KEY_LENGTH);

    key_len = key.length()>EVP_MAX_KEY_LENGTH?EVP_MAX_KEY_LENGTH:key.length(); 
    memcpy(ckey, key.data(), key_len);
}

void AES::SetCryptMode(CryptMode mode)
{
    this->mode = mode;
}

void AES::SetPadding(bool bPadding)
{
    EVP_CIPHER_CTX_set_padding(&aes_enc_ctx, bPadding?1:0);
    EVP_CIPHER_CTX_set_padding(&aes_dec_ctx, bPadding?1:0);
}

int AES::Encrypt(std::string const& in, std::string& out)
{
    out.clear();

    const EVP_CIPHER* cipher_type = GetCipherType(mode);

    EVP_EncryptInit(&aes_enc_ctx, cipher_type, ckey, iv);

    unsigned char *c_out = new unsigned char[in.length() + 16];
    int cipher_len = 0, final_len = 0;
    bool success = true;
    int ret = EVP_EncryptUpdate(&aes_enc_ctx, c_out, &cipher_len, (const unsigned char*)in.data(), in.length());
    if(1 != ret)    success = false;

    if(success)
    {
        ret = EVP_EncryptFinal(&aes_enc_ctx, c_out+cipher_len, &final_len);
        if(1 != ret)    success = false;
    }

    cipher_len += final_len;

    if(success)
        out.assign((const char *)c_out, cipher_len);

    delete []c_out;

    return success?cipher_len:-1;
}

int AES::Decrypt(std::string const& in, std::string& out)
{
    out.clear();

    const EVP_CIPHER* cipher_type = GetCipherType(mode);
    EVP_DecryptInit(&aes_dec_ctx, cipher_type, ckey, iv);
    unsigned char *c_out = new unsigned char[in.length()];
    int cipher_len = 0, final_len = 0;
    bool success = true;
    int ret = EVP_DecryptUpdate(&aes_dec_ctx, c_out, &cipher_len, (const unsigned char*)in.data(), in.length());
    if(1 != ret)    success = false;

    if(success)
    {
        ret = EVP_DecryptFinal(&aes_dec_ctx, c_out+cipher_len, &final_len);
        if(1 != ret)    success = false;
    }

    cipher_len += final_len;

    if(success)
        out.assign((const char *)c_out, cipher_len);

    delete []c_out;

    return success?cipher_len:-1;
}

std::string AES::GetCryptKey()
{
    return std::string((char *)ckey, key_len);
}
