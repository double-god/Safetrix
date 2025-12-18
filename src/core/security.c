#include "core/security.h"
#include <string.h>

void InitSecurity(CryptoContext* ctx, const char* password)
{
    if (!ctx) return;

    memset(ctx, 0, sizeof(CryptoContext));

    if (password && strlen(password) > 0)
    {
        // 简单的密钥扩展：将密码填充满 key 数组
        size_t pwdLen = strlen(password);
        for (int i = 0; i < 32; ++i)
        {
            ctx->key[i] = password[i % pwdLen];
            // 简单的混淆：每个字节加上位置索引
            ctx->key[i] ^= (uint8_t)i;
        }
        ctx->keyLen = 32;
    }
    else
    {
        // 默认密钥
        for (int i = 0; i < 32; ++i) ctx->key[i] = (uint8_t)(0xAB + i);
        ctx->keyLen = 32;
    }
    ctx->keyIndex = 0;
}

void EncryptBuffer(uint8_t* buffer, size_t len, CryptoContext* ctx)
{
    if (!ctx || ctx->keyLen == 0) return;

    for (size_t i = 0; i < len; ++i)
    {
        buffer[i] ^= ctx->key[ctx->keyIndex];
        ctx->keyIndex = (ctx->keyIndex + 1) % ctx->keyLen;
    }
}
