//
// Created by HaoTang on 2025/12/17.
//

#ifndef SAFETRIX_LOGGER_C_H
#define SAFETRIX_LOGGER_C_H

#endif //SAFETRIX_LOGGER_C_H
#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

typedef enum
{
    LOG_INFO = 0,
    LOG_WARNING,
    LOG_ERROR
} LogLevel;

// 初始化日志系统
void Logger_Init(const char* logFilePath);

// 写入日志
void Logger_Log(LogLevel level, const char* format, ...);

// 关闭日志
void Logger_Close(void);

#endif // DATA_LOGGER_H
