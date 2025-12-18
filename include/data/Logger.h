#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <stdarg.h>

// 日志级别
typedef enum
{
    LOG_INFO = 0,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// 初始化日志系统
void Logger_Init(const char* logFilePath);

// 记录一条日志
void Logger_Log(LogLevel level, const char* format, ...);

// 关闭日志文件
void Logger_Close(void);

#endif // DATA_LOGGER_H

