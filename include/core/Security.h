#ifndef CORE_SECURITY_H
#define CORE_SECURITY_H

#include <stdint.h>
#include <stddef.h>

// 上下文结构，用于流式加密
typedef struct
{
    uint8_t key[32]; // 扩展密钥长度
    size_t keyLen;
    size_t keyIndex; // 当前密钥流位置
} CryptoContext;

void InitSecurity(CryptoContext* ctx, const char* password);

// 流式加密缓冲区
void EncryptBuffer(uint8_t* buffer, size_t len, CryptoContext* ctx);

#endif // CORE_SECURITY_H
