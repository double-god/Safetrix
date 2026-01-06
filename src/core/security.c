#include "core/security.h"
#include <string.h>

// 具体策略实现：XOR 算法 (隐藏在模块内部)
static void XOR_Algorithm(uint8_t* buffer, size_t len, CryptoContext* ctx)
{
    if (!ctx || ctx->keyLen == 0) return;

    for (size_t i = 0; i < len; ++i)
    {
        buffer[i] ^= ctx->key[ctx->keyIndex];
        ctx->keyIndex = (ctx->keyIndex + 1) % ctx->keyLen;
    }
}

void InitSecurity(CryptoContext* ctx, const char* password)
{
    if (!ctx) return;

    memset(ctx, 0, sizeof(CryptoContext));

    // 【架构亮点】挂载具体的加密策略
    // 此处体现了多态性：ctx 并不关心使用的是什么算法，只管调用 interface
    ctx->algorithm = XOR_Algorithm;

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

// 对外统一接口：将请求委托给当前挂载的策略
void EncryptBuffer(uint8_t* buffer, size_t len, CryptoContext* ctx)
{
    if (ctx && ctx->algorithm)
    {
        // 多态调用 (Polymorphic Call)
        ctx->algorithm(buffer, len, ctx);
    }
}
