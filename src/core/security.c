#include "core/security.h"
#include <string.h>

void InitSecurity(CryptoContext* ctx, const char* password)
{
    if (password && strlen(password) > 0)
    {
        ctx->key = (uint8_t)password[0];
    }
    else
    {
        ctx->key = 0xAB;
    }
}

void EncryptBuffer(uint8_t* buffer, size_t len, CryptoContext* ctx)
{
    for (size_t i = 0; i < len; ++i)
    {
        buffer[i] ^= ctx->key;
    }
}
