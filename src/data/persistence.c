#include "data/Persistence.h"
#include "data/Logger.h"
#include "utils/FileUtils.h"
#include <stdio.h>
#include <string.h>

// 文件头标识，用于校验文件格式是否合法
static const uint32_t DB_MAGIC = 0x53465458; // ASCII "SFTX"

int Persistence_SaveTasks(const char* dbPath, TransferTask* tasks, int count)
{
    if (!dbPath)
    {
        Logger_Log(LOG_ERROR, "Persistence_SaveTasks: dbPath 为空");
        return -1;
    }
    //覆盖写入
    FILE* fp = FileUtils_OpenFileUTF8(dbPath, "wb");
    if (!fp)
    {
        Logger_Log(LOG_ERROR, "无法打开任务数据库: %s", dbPath);
        return -1;
    }

    // 1. 写入魔数
    fwrite(&DB_MAGIC, sizeof(uint32_t), 1, fp);

    // 2. 写入任务数量
    fwrite(&count, sizeof(int), 1, fp);

    // 3. 写入任务数据（回调指针不参与持久化）
    if (count > 0 && tasks)
    {
        fwrite(tasks, sizeof(TransferTask), count, fp);
    }

    fclose(fp);
    return 0;
}

int Persistence_LoadTasks(const char* dbPath, TransferTask* outTasks, int maxCount)
{
    if (!dbPath)
    {
        Logger_Log(LOG_ERROR, "Persistence_LoadTasks: dbPath 为空");
        return -1;
    }

    FILE* fp = FileUtils_OpenFileUTF8(dbPath, "rb");
    if (!fp) return 0; // 文件不存在视为无任务

    // 1. 校验魔数
    uint32_t magic;
    if (fread(&magic, sizeof(uint32_t), 1, fp) != 1 || magic != DB_MAGIC)
    {
        fclose(fp);
        Logger_Log(LOG_WARNING, "任务数据库格式不正确，已忽略");
        return 0;
    }

    // 2. 读取数量
    int count = 0;
    fread(&count, sizeof(int), 1, fp);
    if (count > maxCount) count = maxCount;

    // 3. 读取任务数据
    int loaded = 0;
    if (count > 0 && outTasks)
    {
        loaded = (int)fread(outTasks, sizeof(TransferTask), count, fp);
    }

    // 4. 清理回调指针
    // 结构体中的函数指针 (onProgress, onError) 保存的是上次运行时的内存地址。
    for (int i = 0; i < loaded; ++i)
    {
        outTasks[i].onProgress = NULL;
        outTasks[i].onError = NULL;
    }

    fclose(fp);
    return loaded;
}
