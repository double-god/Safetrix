#ifndef CORE_SECURITY_H
#define CORE_SECURITY_H

#include <stdint.h>
#include <stddef.h>

// 上下文结构，用于流式加密
typedef struct
{
    uint8_t key; // 简化版：使用单字节异或
} CryptoContext;

void InitSecurity(CryptoContext* ctx, const char* password);

// 流式加密缓冲区
void EncryptBuffer(uint8_t* buffer, size_t len, CryptoContext* ctx);

#endif // CORE_SECURITY_H
