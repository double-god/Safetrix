//
// Created by HaoTang on 2025/12/17.
//
#include "core/transfer_engine.h"
#include "core/security.h"
#include "common/error_code.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 4KB 分片大小 (F-01)
#define CHUNK_SIZE 4096

int InitTransferEngine(void)
{
    return ERR_SUCCESS;
}

int RunTask(TransferTask* task)
{
    if (!task) return ERR_MEMORY;

    FILE* fpSrc = fopen(task->srcPath, "rb");
    if (!fpSrc)
    {
        if (task->onError) task->onError(task->id, ERR_FILE_OPEN, "无法打开源文件");
        return ERR_FILE_OPEN;
    }

    FILE* fpDest = fopen(task->destPath, "ab"); // "ab" 支持追加 (断点续传基础)
    if (!fpDest)
    {
        fclose(fpSrc);
        if (task->onError) task->onError(task->id, ERR_FILE_OPEN, "无法创建目标文件");
        return ERR_FILE_WRITE;
    }

    // F-02: 断点续传 - 定位到上次结束的位置
    fseek(fpSrc, (long)task->currentOffset, SEEK_SET);
    // 目标文件如果是追加模式，指针天然在末尾，但为了安全可以再次检查

    // 初始化加密上下文
    CryptoContext ctx;
    InitSecurity(&ctx, "SecretKey123"); // 模拟从配置读取密钥

    uint8_t buffer[CHUNK_SIZE];
    size_t bytesRead;
    clock_t lastTime = clock();
    task->status = TASK_RUNNING;

    // F-01: 分片循环
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, fpSrc)) > 0)
    {
        // F-04: 加密
        EncryptBuffer(buffer, bytesRead, &ctx);

        // 写入
        size_t bytesWritten = fwrite(buffer, 1, bytesRead, fpDest);
        if (bytesWritten < bytesRead)
        {
            task->status = TASK_ERROR;
            break;
        }

        task->currentOffset += bytesRead;

        // 计算并回调进度 (降低频率，避免刷屏)
        if (task->onProgress && (task->currentOffset % (CHUNK_SIZE * 10) == 0))
        {
            double percent = 0;
            if (task->totalSize > 0)
            {
                percent = (double)task->currentOffset / (double)task->totalSize * 100.0;
            }
            // 模拟速度计算
            task->onProgress(task->id, percent, 12.5);
        }

        // 模拟一点延迟，让你能看到进度条
        // _sleep(10); // Windows特有，跨平台需封装，这里为了演示省略
    }

    task->status = TASK_COMPLETED;
    if (task->onProgress) task->onProgress(task->id, 100.0, 0.0);

    fclose(fpSrc);
    fclose(fpDest);
    return ERR_SUCCESS;
}
