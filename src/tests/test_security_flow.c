#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "common/AppTypes.h"
#include "core/TaskManager.h"
#include "core/TransferEngine.h"
#include "utils/FileUtils.h"

// TransferEngine.c 实现了 RunTask，但没有在头文件暴露（项目中直接调用）。
// 在测试中我们声明一下以便链接。
extern int RunTask(TransferTask* task);

// 简单的回调实现，用于展示进度与错误
void test_on_progress(int taskId, double percentage, double speedMbS)
{
    printf("[回调] 任务 %d 进度: %.2f%%, 速率: %.2f MB/s\n", taskId, percentage, speedMbS);
}

void test_on_error(int taskId, int errorCode, const char* msg)
{
    printf("[回调] 任务 %d 错误: %d, %s\n", taskId, errorCode, msg ? msg : "(null)");
}

// 生成一个大小为 sizeMB 的测试文件（覆盖）
static int create_dummy_file(const char* path, size_t sizeMB)
{
    FILE* f = FileUtils_OpenFileUTF8(path, "wb");
    if (!f) return -1;
    const size_t block = 1024;
    char buf[1024];
    for (size_t i = 0; i < 1024; ++i) buf[i] = (char)(i % 256);
    size_t total = sizeMB * 1024 * 1024;
    size_t written = 0;
    while (written < total)
    {
        size_t to_write = (total - written > block) ? block : (total - written);
        if (fwrite(buf, 1, to_write, f) != to_write)
        {
            fclose(f);
            return -2;
        }
        written += to_write;
    }
    fclose(f);
    return 0;
}

int main(void)
{
#ifdef _WIN32
    // 设置控制台输出编码为 UTF-8，防止中文乱码
    SetConsoleOutputCP(65001);
#endif

    printf("=== SafeTrix 测试流程演示 ===\n");

    // 初始化 TaskManager（会尝试从磁盘加载历史任务）
    InitTaskManager();

    // 1) 创建测试文件（10MB）
    const char* src = "test_source.dat";
    const char* dest = "test_dest.dat";
    printf("1) 生成测试文件 %s (1MB) ...\n", src);
    if (create_dummy_file(src, 1) != 0)
    {
        printf("无法创建测试文件。\n");
        return 1;
    }
    printf("测试文件已创建。\n");

    // 2) 添加新任务
    printf("2) 添加新任务: %s -> %s\n", src, dest);
    int id = AddTask(src, dest, 1);
    if (id <= 0)
    {
        printf("添加任务失败，返回: %d\n", id);
        return 1;
    }
    printf("任务已创建，ID = %d\n", id);

    // 绑定回调
    SetTaskCallbacks(id, test_on_progress, test_on_error);

    // 3) 查看任务列表
    int count = 0;
    TransferTask* list = GetTaskList(&count);
    printf("3) 当前任务数: %d\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("  ID=%d 状态=%d 进度=%llu/%llu 源=%s 目标=%s\n",
               list[i].id, (int)list[i].status, list[i].currentOffset, list[i].totalSize,
               list[i].srcPath, list[i].destPath);
    }

    // 4) 运行任务（阻塞调用 RunTask）
    TransferTask* task = GetTaskById(id);
    if (!task)
    {
        printf("未找到任务 %d\n", id);
        return 1;
    }

    printf("4) 启动任务 %d（阻塞运行，直到完成）...\n", id);
    // InitTransferEngine 若需要在 RunTask 前被调用，确保初始化
    InitTransferEngine();
    int r = RunTask(task);
    if (r != 0)
    {
        printf("RunTask 返回错误: %d\n", r);
    }
    else
    {
        printf("任务 %d 运行完成。\n", id);
    }

    // 显示最终状态
    list = GetTaskList(&count);
    printf("最终任务列表 (%d):\n", count);
    for (int i = 0; i < count; ++i)
    {
        printf("  ID=%d 状态=%d 进度=%llu/%llu 源=%s 目标=%s\n",
               list[i].id, (int)list[i].status, list[i].currentOffset, list[i].totalSize,
               list[i].srcPath, list[i].destPath);
    }

    // 5) 解密（直接对密文做一次传输，使用相同的密钥 -> 应还原原始内容）
    const char* recovered = "test_recovered.dat";
    printf("\n5) 解密文件: %s -> %s\n", dest, recovered);
    int id2 = AddTask(dest, recovered, 1);
    if (id2 <= 0)
    {
        printf("无法添加解密任务，错误: %d\n", id2);
    }
    else
    {
        SetTaskCallbacks(id2, test_on_progress, test_on_error);
        TransferTask* task2 = GetTaskById(id2);
        if (!task2)
        {
            printf("未找到解密任务 %d\n", id2);
        }
        else
        {
            InitTransferEngine();
            int r2 = RunTask(task2);
            if (r2 != 0)
            {
                printf("解密任务返回错误: %d\n", r2);
            }
            else
            {
                printf("解密完成，开始校验输出是否与原文件一致...\n");

                // 比较文件内容
                FILE* f1 = FileUtils_OpenFileUTF8(src, "rb");
                FILE* f2 = FileUtils_OpenFileUTF8(recovered, "rb");
                if (!f1 || !f2)
                {
                    printf("无法打开文件进行比较。\n");
                }
                else
                {
                    // 获取大小
                    fseek(f1, 0, SEEK_END);
                    long s1 = ftell(f1);
                    fseek(f1, 0, SEEK_SET);
                    fseek(f2, 0, SEEK_END);
                    long s2 = ftell(f2);
                    fseek(f2, 0, SEEK_SET);
                    if (s1 != s2)
                    {
                        printf("校验失败：大小不匹配，src=%ld, recovered=%ld\n", s1, s2);
                    }
                    else
                    {
                        int same = 1;
                        const size_t bufSz = 4096;
                        unsigned char* b1 = (unsigned char*)malloc(bufSz);
                        unsigned char* b2 = (unsigned char*)malloc(bufSz);
                        if (!b1 || !b2)
                        {
                            printf("内存分配失败，无法完成比较。\n");
                            same = 0;
                        }
                        else
                        {
                            size_t rcount;
                            while ((rcount = fread(b1, 1, bufSz, f1)) > 0)
                            {
                                size_t r2c = fread(b2, 1, bufSz, f2);
                                if (r2c != rcount || memcmp(b1, b2, rcount) != 0)
                                {
                                    same = 0;
                                    break;
                                }
                            }
                        }
                        if (b1) free(b1);
                        if (b2) free(b2);
                        if (same) printf("校验通过：解密后文件与原始文件一致。\n");
                        else printf("校验失败：文件内容不同。\n");
                    }
                    fclose(f1);
                    fclose(f2);
                }
            }
        }
    }

    printf("测试结束。\n");
    return 0;
}

