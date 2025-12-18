#ifndef UTILS_ALGORITHM_H
#define UTILS_ALGORITHM_H

#include <stdint.h>
#include <stddef.h>

// 初始化 CRC 表 (程序启动时调用一次)
void Algorithm_InitCRC32(void);

// 计算数据的 CRC32
uint32_t Algorithm_CalculateCRC32(const uint8_t* data, size_t length);

// 累加计算 CRC32 (用于流式处理大文件)
uint32_t Algorithm_UpdateCRC32(uint32_t currentCrc, const uint8_t* data, size_t length);

#endif // UTILS_ALGORITHM_H

