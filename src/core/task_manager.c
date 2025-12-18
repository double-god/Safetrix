#include "core/TaskManager.h"
#include "common/AppTypes.h"
#include "common/ErrorCode.h"
#include "data/Logger.h"
#include "data/Persistence.h"

#include <string.h>
#include <stdio.h>

#define MAX_TASKS 128
#define DB_PATH "data/safetrix.db"

static TransferTask g_tasks[MAX_TASKS];
static int g_task_count = 0;
static int g_next_task_id = 1;
static unsigned char g_dirty[MAX_TASKS];

void TaskManager_Sync(void)
{
    if (Persistence_SaveTasks(DB_PATH, g_tasks, g_task_count) != 0)
    {
        Logger_Log(LOG_ERROR, "保存任务列表失败 -> %s", DB_PATH);
    }
}

void InitTaskManager(void)
{
    memset(g_tasks, 0, sizeof(g_tasks));
    memset(g_dirty, 0, sizeof(g_dirty));

    int loaded = Persistence_LoadTasks(DB_PATH, g_tasks, MAX_TASKS);
    if (loaded < 0)
    {
        Logger_Log(LOG_WARNING, "任务列表加载异常，启用空任务列表");
        g_task_count = 0;
    }
    else
    {
        g_task_count = loaded;
    }

    RecalculateNextId();
}

int AddTask(const char* src, const char* dest, int priority)
{
    if (!src || !dest)
    {
        return ERR_MEMORY; // 使用已有的错误码，避免未定义符号
    }

    if (g_task_count >= MAX_TASKS)
    {
        return ERR_TASK_FULL;
    }

    TransferTask* task = &g_tasks[g_task_count];
    memset(task, 0, sizeof(TransferTask));

    task->id = g_next_task_id++;
    // 复制路径到任务结构体中
    strncpy(task->srcPath, src, sizeof(task->srcPath) - 1);
    strncpy(task->destPath, dest, sizeof(task->destPath) - 1);

    task->priority = priority;
    task->status = TASK_WAITING; // 使用已定义的状态枚举
    task->currentOffset = 0;

    // 如果能够获取文件大小，则填充 totalSize；无法获取时保持 0，由 TransferEngine 内部容错
    FILE* fp = fopen(src, "rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        long sz = ftell(fp);
        if (sz > 0)
        {
            task->totalSize = (unsigned long long)sz;
        }
        fclose(fp);
    }

    g_task_count++;
    TaskManager_Sync();
    return task->id;
}

TransferTask* GetTaskById(int id)
{
    for (int i = 0; i < g_task_count; ++i)
    {
        if (g_tasks[i].id == id)
        {
            return &g_tasks[i];
        }
    }
    return NULL;
}

TransferTask* GetTaskList(int* count)
{
    if (count)
    {
        *count = g_task_count;
    }
    return g_tasks;
}

void SetTaskCallbacks(int taskId, OnProgressCallback onProgress, OnErrorCallback onError)
{
    TransferTask* task = GetTaskById(taskId);
    if (!task)
    {
        return;
    }

    task->onProgress = onProgress;
    task->onError = onError;
}

void TaskManager_UpdateTask(TransferTask* task)
{
    for (int i = 0; i < g_task_count; ++i)
    {
        if (&g_tasks[i] == task)
        {
            g_dirty[i] = 1;
            break;
        }
    }
}
