/
Deprecated: implementation
 moved to src
/
core
/
task_manager
.
c

// 头部引入头文件
#include "utils/FileUtils.h"

// ... (其他代码保持不变)

int AddTask(const char* src, const char* dest, int priority)
{
    if (g_taskCount >= MAX_TASKS) return ERR_TASK_FULL;

    // 检查源文件是否存在
    if (!FileUtils_Exists(src))
    {
        return ERR_FILE_OPEN; // 或者定义一个 ERR_FILE_NOT_FOUND
    }

    TransferTask* t = &g_tasks[g_taskCount];
    t->id = g_taskCount + 1;
    strncpy(t->srcPath, src, 255);
    strncpy(t->destPath, dest, 255);
    t->priority = priority;
    t->status = TASK_WAITING;
    t->currentOffset = 0;

    // 【修改点】获取真实文件大小
    t->totalSize = FileUtils_GetFileSize(src);

    t->onProgress = NULL;
    t->onError = NULL;

    g_taskCount++;
    return t->id;
}

// ... (其他代码保持不变)
