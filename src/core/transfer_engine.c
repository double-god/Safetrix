#include "core/TaskManager.h"
#include "core/security.h"
#include "common/ErrorCode.h"
#include "data/Logger.h"
#include "utils/FileUtils.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#include <wchar.h>
#include <conio.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
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
        FileUtils_Mkdir(accum);

        if (!*sep) break;
        p = sep + 1;
    }

    return 0;
}

int InitTransferEngine(void)
{
    return 0;
}

int RunTask(TransferTask* task)
{
    if (!task) return -1;

    FILE* fpSrc = FileUtils_OpenFileUTF8(task->srcPath, "rb");
    if (!fpSrc)
    {
        if (task->onError) task->onError(task->id, -1, "Cannot open source file");
        return -1;
    }

    // 打开目标文件为可读写（以便支持断点续传），若不存在则创建
    FILE* fpDest = FileUtils_OpenFileUTF8(task->destPath, "r+b");
    if (!fpDest)
    {
        // 尝试创建父目录后再创建目标文件
        ensure_parent_dir_exists(task->destPath);
        fpDest = FileUtils_OpenFileUTF8(task->destPath, "r+b");
        if (!fpDest)
        {
            fpDest = FileUtils_OpenFileUTF8(task->destPath, "wb");
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

    // 修复：根据当前文件偏移量，调整密钥流的索引
    // 否则断点续传时，密钥会从头开始算，导致解密失败
    if (task->currentOffset > 0 && ctx.keyLen > 0)
    {
        ctx.keyIndex = (int)(task->currentOffset % ctx.keyLen);
    }

    uint8_t buffer[CHUNK_SIZE];
    size_t bytesRead;
    task->status = TASK_RUNNING;

    size_t bytesSinceLastSync = 0;
    const size_t SYNC_THRESHOLD = 64 * 1024; // 64KB 更频繁的同步，以便快速恢复

    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE, fpSrc)) > 0)
    {
#ifdef _WIN32
        // 非阻塞交互检测
        if (_kbhit()) // 检查是否有键盘敲击（不阻塞）
        {
            int ch = _getch(); // 获取字符
            if (ch == 'p' || ch == 'P') // 设定 'p' 为暂停 (Pause)
            {
                // A. 修改状态
                task->status = TASK_PAUSED;

                // B. 立即保存进度 (可以演示断点续传)
                TaskManager_UpdateTask(task);
                TaskManager_Sync();

                // C. 给出提示
                printf("\n\n[交互] 检测到暂停指令！\n");
                printf("[系统] 进度已保存 (Offset: %llu)。\n", task->currentOffset);
                printf("[系统] 文件句柄已释放，您现在可以检查文件内容。\n");

                // D. 必须关闭文件！否则文件被锁死，无法用编辑器查看
                fclose(fpSrc);
                fclose(fpDest);

                return 0; // 退出 RunTask，回到主菜单
            }
        }
#endif

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
        //计算current-percent
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
