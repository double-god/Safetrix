//
// Created by HaoTang on 2025/12/17.
//
#include "core/security.h"
#include <string.h>

// Deprecated: implementation moved to src/core/security.c

void InitSecurity(CryptoContext* ctx, const char* password)
{
    // 简单的密钥生成策略：取密码第一个字符
    // 实际项目中应使用 Hash 算法
    if (password && strlen(password) > 0)
    {
        ctx->key = (uint8_t)password[0];
    }
    else
    {
        ctx->key = 0xAB; // 默认密钥
    }
}

void EncryptBuffer(uint8_t* buffer, size_t len, CryptoContext* ctx)
{
    for (size_t i = 0; i < len; ++i)
    {
        buffer[i] ^= ctx->key; // 异或运算
    }
}
