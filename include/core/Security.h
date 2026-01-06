#ifndef CORE_SECURITY_H
#define CORE_SECURITY_H

#include <stdint.h>
#include <stddef.h>

// 上下文结构的前置声明
struct CryptoContext;

// 定义加密策略接口 (Strategy Interface)
// 这里的 ctx 使用 struct CryptoContext* 类型，实现了对具体上下文的引用
typedef void (*CipherFunc)(uint8_t* data, size_t len, struct CryptoContext* ctx);

// 上下文结构，用于流式加密
typedef struct CryptoContext
{
    uint8_t key[32]; // 扩展密钥长度
    size_t keyLen;
    size_t keyIndex; // 当前密钥流位置

    // 加密策略接口
    // 允许在运行时动态挂载不同的加密算法 (XOR, AES, etc.)
    CipherFunc algorithm;
} CryptoContext;

void InitSecurity(CryptoContext* ctx, const char* password);

// 流式加密缓冲区
void EncryptBuffer(uint8_t* buffer, size_t len, CryptoContext* ctx);

#endif // CORE_SECURITY_H
