#include "core/TaskManager.h"
#include "core/security.h"
#include "common/ErrorCode.h"
#include "data/Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <wchar.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

#define CHUNK_SIZE 4096

// Helper: create parent directories recursively for a given path
static int ensure_parent_dir_exists(const char* path)
{
    if (!path) return -1;

    char tmp[1024];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    // Strip trailing separators
    size_t tlen = strlen(tmp);
    while (tlen > 0 && (tmp[tlen - 1] == '\\' || tmp[tlen - 1] == '/'))
    {
        tmp[tlen - 1] = '\0';
        tlen--;
    }
    if (tlen == 0) return -1;

    // Find last separator to get parent directory
    char* last_sep1 = strrchr(tmp, '\\');
    char* last_sep2 = strrchr(tmp, '/');
    char* last_sep = last_sep1 > last_sep2 ? last_sep1 : last_sep2;
    if (!last_sep) return 0; // no parent directory

    // Temporarily terminate string at parent dir
    *last_sep = '\0';

    // Build and create each component
    char accum[1024] = "";
    char* p = tmp;

    // Handle Windows drive letter like "C:\" -> start accum with "C:\"
    if (strlen(tmp) >= 2 && tmp[1] == ':')
    {
        accum[0] = tmp[0];
        accum[1] = ':';
        accum[2] = '\\';
        accum[3] = '\0';
        p = tmp + 3; // skip "C:\"
    }

    while (p && *p)
    {
        // find next separator or end
        char* sep = p;
        while (*sep && *sep != '\\' && *sep != '/') sep++;
        size_t seglen = sep - p;

        // append separator if needed
        if (accum[0] != '\0' && accum[strlen(accum) - 1] != '\\')
        {
            strncat(accum, "\\", sizeof(accum) - strlen(accum) - 1);
        }

        // append segment
        strncat(accum, p, (sizeof(accum) - strlen(accum) - 1) < seglen ? (sizeof(accum) - strlen(accum) - 1) : seglen);

        // try to create
        MKDIR(accum);

        if (!*sep) break;
        p = sep + 1;
    }

    return 0;
}

// Cross-platform fopen wrappers that accept multibyte char* paths and use wide APIs on Windows
#ifdef _WIN32
static wchar_t* mbcs_to_wide_alloc_local(const char* s)
{
    if (!s) return NULL;
    int needed = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    if (needed <= 0) return NULL;
    wchar_t* w = (wchar_t*)malloc(sizeof(wchar_t) * needed);
    if (!w) return NULL;
    MultiByteToWideChar(CP_ACP, 0, s, -1, w, needed);
    return w;
}

static FILE* fopen_rbin(const char* path)
{
    wchar_t* w = mbcs_to_wide_alloc_local(path);
    if (!w) return NULL;
    FILE* f = _wfopen(w, L"rb");
    free(w);
    return f;
}

static FILE* fopen_rwb(const char* path)
{
    wchar_t* w = mbcs_to_wide_alloc_local(path);
    if (!w) return NULL;
    FILE* f = _wfopen(w, L"r+b");
    free(w);
    return f;
}

static FILE* fopen_wb(const char* path)
{
    wchar_t* w = mbcs_to_wide_alloc_local(path);
    if (!w) return NULL;
    FILE* f = _wfopen(w, L"wb");
    free(w);
    return f;
}
#else
static FILE* fopen_rbin(const char* path) { return fopen(path, "rb"); }
static FILE* fopen_rwb(const char* path) { return fopen(path, "r+b"); }
static FILE* fopen_wb(const char* path) { return fopen(path, "wb"); }
#endif

int InitTransferEngine(void)
{
    return 0;
}

int RunTask(TransferTask* task)
{
    if (!task) return -1;

    FILE* fpSrc = fopen_rbin(task->srcPath);
    if (!fpSrc)
    {
        if (task->onError) task->onError(task->id, -1, "Cannot open source file");
        return -1;
    }

    // 打开目标文件为可读写（以便支持断点续传），若不存在则创建
    FILE* fpDest = fopen_rwb(task->destPath);
    if (!fpDest)
    {
        // 尝试创建父目录后再创建目标文件
        ensure_parent_dir_exists(task->destPath);
        fpDest = fopen_rwb(task->destPath);
        if (!fpDest)
        {
            fpDest = fopen_wb(task->destPath);
            if (!fpDest)
            {
                fclose(fpSrc);
                if (task->onError) task->onError(task->id, -1, "Cannot create dest file");
                return -1;
            }
        }
    }

    // 确保源文件从 task->currentOffset 开始读取
    if (fseek(fpSrc, (long)task->currentOffset, SEEK_SET) != 0)
    {
        fclose(fpSrc);
        fclose(fpDest);
        if (task->onError) task->onError(task->id, -1, "Failed to seek source file");
        return -1;
    }

    // 将目标文件位置移动到 currentOffset（resume）
    if (fseek(fpDest, (long)task->currentOffset, SEEK_SET) != 0)
    {
        fclose(fpSrc);
        fclose(fpDest);
        if (task->onError) task->onError(task->id, -1, "Failed to seek dest file");
        return -1;
    }

    CryptoContext ctx;
    InitSecurity(&ctx, "SecretKey123");

    uint8_t buffer[CHUNK_SIZE];
    size_t bytesRead;
    task->status = TASK_RUNNING;

    size_t bytesSinceLastSync = 0;
    const size_t SYNC_THRESHOLD = 64 * 1024; // 64KB 更频繁的同步，以便快速恢复

    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, fpSrc)) > 0)
    {
        EncryptBuffer(buffer, (size_t)bytesRead, &ctx);

        size_t bytesWritten = fwrite(buffer, 1, bytesRead, fpDest);
        if (bytesWritten < bytesRead)
        {
            task->status = TASK_ERROR;
            // 立即持久化状态并返回错误
            TaskManager_UpdateTask(task);
            TaskManager_Sync();
            if (task->onError) task->onError(task->id, -1, "Failed to write dest file");
            fclose(fpSrc);
            fclose(fpDest);
            return -1;
        }

        // 更新内存中的偏移量
        task->currentOffset += bytesWritten;
        bytesSinceLastSync += bytesWritten;

        // 每次写入后标记为已修改，这样 TaskManager 可以按需持久化
        TaskManager_UpdateTask(task);

        // 根据阈值进行持久化（避免过于频繁的磁盘写入，但仍足够频繁用于断点恢复）
        if (bytesSinceLastSync >= SYNC_THRESHOLD)
        {
            TaskManager_Sync();
            bytesSinceLastSync = 0;
        }

        // 更频繁地更新 UI 回调（每块都回调），便于实时显示
        if (task->onProgress)
        {
            double percent = 0.0;
            if (task->totalSize > 0)
            {
                percent = (double)task->currentOffset / (double)task->totalSize * 100.0;
            }
            task->onProgress(task->id, percent, 0.0);
        }
    }

    // 循环结束后检查是否为正常完成
    if (!feof(fpSrc))
    {
        // 读取出错
        task->status = TASK_ERROR;
        TaskManager_UpdateTask(task);
        TaskManager_Sync();
        if (task->onError) task->onError(task->id, -1, "Read error on source file");
        fclose(fpSrc);
        fclose(fpDest);
        return -1;
    }

    // 标记完成、持久化并触发最终进度回调
    task->status = TASK_COMPLETED;
    TaskManager_UpdateTask(task);
    TaskManager_Sync();
    if (task->onProgress) task->onProgress(task->id, 100.0, 0.0);

    fclose(fpSrc);
    fclose(fpDest);
    return 0;
}
