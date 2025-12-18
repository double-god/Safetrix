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

    FILE* fpDest = fopen(task->destPath, "ab");
    if (!fpDest)
    {
        fclose(fpSrc);
        if (task->onError) task->onError(task->id, -1, "Cannot create dest file");
        return -1;
    }

    fseek(fpSrc, (long)task->currentOffset, SEEK_SET);

    CryptoContext ctx;
    InitSecurity(&ctx, "SecretKey123");

    uint8_t buffer[CHUNK_SIZE];
    size_t bytesRead;
    task->status = TASK_RUNNING;

    size_t bytesSinceLastSync = 0;
    const size_t SYNC_THRESHOLD = 1024 * 1024; // 1MB

    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, fpSrc)) > 0)
    {
        EncryptBuffer(buffer, (size_t)bytesRead, &ctx);

        size_t bytesWritten = fwrite(buffer, 1, bytesRead, fpDest);
        if (bytesWritten < bytesRead)
        {
            task->status = TASK_ERROR;
            break;
        }

        task->currentOffset += bytesRead;
        bytesSinceLastSync += bytesRead;

        // Optimize: Sync to disk only when threshold reached
        if (bytesSinceLastSync >= SYNC_THRESHOLD)
        {
            TaskManager_UpdateTask(task);
            TaskManager_Sync();
            bytesSinceLastSync = 0;
        }

        if (task->onProgress && (task->currentOffset % (CHUNK_SIZE * 10) == 0))
        {
            double percent = 0;
            if (task->totalSize > 0)
            {
                percent = (double)task->currentOffset / (double)task->totalSize * 100.0;
            }
            task->onProgress(task->id, percent, 12.5);
        }
    }

    task->status = TASK_COMPLETED;
    TaskManager_UpdateTask(task);
    TaskManager_Sync();
    if (task->onProgress) task->onProgress(task->id, 100.0, 0.0);

    fclose(fpSrc);
    fclose(fpDest);
    return 0;
}
