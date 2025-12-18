#include "data/Logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

static FILE* g_logFile = NULL;

void Logger_Init(const char* logFilePath)
{
    if (g_logFile) fclose(g_logFile);
    g_logFile = fopen(logFilePath, "a"); // 追加模式
}

void Logger_Log(LogLevel level, const char* format, ...)
{
    if (!g_logFile) return;

    // 获取时间
    time_t now;
    time(&now);
    struct tm* t = localtime(&now);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", t);

    // 等级标签
    const char* levelStr = "[INFO]";
    if (level == LOG_WARNING) levelStr = "[WARN]";
    else if (level == LOG_ERROR) levelStr = "[ERR ]";

    // 写入头部
    fprintf(g_logFile, "%s %s ", timeStr, levelStr);

    // 写入内容
    va_list args;
    va_start(args, format);
    vfprintf(g_logFile, format, args);
    va_end(args);

    fprintf(g_logFile, "\n");
    fflush(g_logFile); // 确保立即写入磁盘
}

void Logger_Close(void)
{
    if (g_logFile)
    {
        fclose(g_logFile);
        g_logFile = NULL;
    }
}
