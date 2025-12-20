#include "core/TaskManager.h"
#include "core/security.h"
#include "common/ErrorCode.h"
#include "data/Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define CHUNK_SIZE 4096

int InitTransferEngine(void)
{
    return 0;
}

int RunTask(TransferTask* task)
{
    if (!task) return -1;

    FILE* fpSrc = fopen(task->srcPath, "rb");
    if (!fpSrc)
    {
        if (task->onError) task->onError(task->id, -1, "Cannot open source file");
        return -1;
    }

    // 打开目标文件为可读写（以便支持断点续传），若不存在则创建
    FILE* fpDest = fopen(task->destPath, "r+b");
    if (!fpDest)
    {
        // 目标文件不存在，创建新文件
        fpDest = fopen(task->destPath, "wb");
        if (!fpDest)
        {
            fclose(fpSrc);
            if (task->onError) task->onError(task->id, -1, "Cannot create dest file");
            return -1;
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
