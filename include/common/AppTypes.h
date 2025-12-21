#ifndef COMMON_APP_TYPES_H
#define COMMON_APP_TYPES_H

#include <stdint.h>
#include <stddef.h>

// 状态枚举
typedef enum
{
    TASK_WAITING = 0,
    TASK_RUNNING,
    TASK_PAUSED,
    TASK_COMPLETED,
    TASK_ERROR
} TaskStatus;

// 回调函数原型 (UI与逻辑解耦的关键)
typedef void (*OnProgressCallback)(int taskId, double percentage, double speedMbS);
typedef void (*OnErrorCallback)(int taskId, int errorCode, const char* errorMsg);

// 核心任务结构体
typedef struct
{
    int id;
    char srcPath[256];
    char destPath[256];
    uint64_t totalSize;
    uint64_t currentOffset; // 断点续传游标
    int priority; // 优先级
    TaskStatus status;
    uint32_t crc32; // 完整性校验值

    // 运行时回调 (不持久化到磁盘)
    OnProgressCallback onProgress;
    OnErrorCallback onError;
} TransferTask;

#endif // COMMON_APP_TYPES_H
