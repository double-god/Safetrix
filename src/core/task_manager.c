#include "core/TaskManager.h"
#include "common/AppTypes.h"
#include "common/ErrorCode.h"
#include "data/Logger.h"
#include "data/Persistence.h"
#include "utils/FileUtils.h"

#include <string.h>
#include <stdio.h>

#define MAX_TASKS 128
#define DB_PATH "data/safetrix.db"

static TransferTask g_tasks[MAX_TASKS];
static int g_task_count = 0;
static int g_next_task_id = 1;
static unsigned char g_dirty[MAX_TASKS];

// --- 内部辅助函数：重新计算下一个任务 ID ---
// 遍历当前任务列表，找到最大 ID，然后设置 g_next_task_id 为 maxId + 1
static void RecalculateNextId(void)
{
    int maxId = 0;
    for (int i = 0; i < g_task_count; ++i)
    {
        if (g_tasks[i].id > maxId)
        {
            maxId = g_tasks[i].id;
        }
    }
    // 下一个新任务的 ID 应该是当前最大 ID + 1
    g_next_task_id = maxId + 1;
}

// 将内存中的任务列表同步到磁盘
void TaskManager_Sync(void)
{
    if (Persistence_SaveTasks(DB_PATH, g_tasks, g_task_count) != 0)
    {
        Logger_Log(LOG_ERROR, "保存任务列表失败 -> %s", DB_PATH);
    }
}

// 初始化任务管理器：清理内存并从磁盘加载上次保存的任务列表
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

// 添加新任务并立即持久化
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
    // 复制路径到任务结构体中（安全复制）
    strncpy(task->srcPath, src, sizeof(task->srcPath) - 1);
    strncpy(task->destPath, dest, sizeof(task->destPath) - 1);

    task->priority = priority;
    task->status = TASK_WAITING; // 初始状态为等待中
    task->currentOffset = 0;

    // 尝试获取源文件大小以便显示进度（失败时保留为 0）
    task->totalSize = FileUtils_GetFileSize(src);

    g_task_count++;
    Persistence_SaveTasks(DB_PATH, g_tasks, g_task_count); // 任务变更立即持久化
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

// 为指定任务设置回调（不会持久化回调指针）
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

// 标记任务为已修改（用于延迟或按需持久化）
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
