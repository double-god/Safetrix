#include "core/task_manager.h"
#include "common/error_code.h"
#include <string.h>

#define MAX_TASKS 100

static TransferTask g_tasks[MAX_TASKS];
static int g_taskCount = 0;

void InitTaskManager(void)
{
    g_taskCount = 0;
    // F-10: 这里应该加载 safetrix.db，为简化演示省略
}

int AddTask(const char* src, const char* dest, int priority)
{
    if (g_taskCount >= MAX_TASKS) return ERR_TASK_FULL;

    TransferTask* t = &g_tasks[g_taskCount];
    t->id = g_taskCount + 1;
    strncpy(t->srcPath, src, 255);
    strncpy(t->destPath, dest, 255);
    t->priority = priority;
    t->status = TASK_WAITING;
    t->currentOffset = 0;
    t->totalSize = 1024 * 1024 * 10; // 模拟 10MB 文件大小，实际应调用 stat 获取
    t->onProgress = NULL;
    t->onError = NULL;

    g_taskCount++;
    return t->id;
}

TransferTask* GetTaskById(int id)
{
    for (int i = 0; i < g_taskCount; ++i)
    {
        if (g_tasks[i].id == id) return &g_tasks[i];
    }
    return NULL;
}

TransferTask* GetTaskList(int* count)
{
    if (count) *count = g_taskCount;
    return g_tasks;
}

void SetTaskCallbacks(int taskId, OnProgressCallback onProgress, OnErrorCallback onError)
{
    TransferTask* t = GetTaskById(taskId);
    if (t)
    {
        t->onProgress = onProgress;
        t->onError = onError;
    }
} //
// Created by HaoTang on 2025/12/17.
//
