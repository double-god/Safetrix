#include "ui/MainWindow.h"
#include "core/TaskManager.h"     // 引入任务管理器
#include "core/TransferEngine.h"  // 引入传输引擎
#include "utils/FileUtils.h"      // 引入工具
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 定义一个回调函数，Core 层进度更新时会调用这个
void OnUIProgressUpdate(int taskId, double percentage, double speedMbS)
{
    // 这里我们很难直接获取 MainWindow 指针，
    // 在简单的 C 程序中，我们可以通过全局变量或传参 context 解决。
    // 为了保持你目前的结构简单，我们在主循环里手动渲染，或者在这里打印日志。

    // 实际的 UI 更新逻辑由 RunLoop 中的渲染负责，
    // 但我们需要一个地方把百分比存下来。
    // 简化起见，我们直接打印（控制台应用）：
    // printf("\rProgress: %.1f%% Speed: %.2f MB/s", percentage, speedMbS);
}

void MainWindow_Init(MainWindow* win, const char* title)
{
    if (!win) return;
    strncpy(win->title, title, sizeof(win->title) - 1);
    ProgressBar_Init(&win->main_progress_bar, 1, 40);
    win->is_running = 0;
}

void MainWindow_Show(MainWindow* win)
{
    if (!win) return;
    printf("------------------------------------------------\n");
    printf("应用: %s\n", win->title);
    printf("公告: %s\n", UI_GetWelcomeText());
    printf("主题: %s\n", UI_GetThemeColor());
    printf("------------------------------------------------\n");
}

// --- 专门为 UI 定义的适配回调 ---
// 为了让回调能更新 MainWindow 的进度条，我们需要一个静态指针或者传递 context
static MainWindow* g_currentWindow = NULL;

void _ui_callback(int taskId, double percentage, double speed)
{
    if (g_currentWindow)
    {
        ProgressBar_Update(&g_currentWindow->main_progress_bar, (float)percentage);
        ProgressBar_Render(&g_currentWindow->main_progress_bar);
    }
}

// 辅助函数：生成一个测试文件
static void CreateDummyFile(const char* filename, size_t sizeMB)
{
    FILE* f = fopen(filename, "wb");
    if (!f)
    {
        printf("[错误] 无法创建文件: %s\n", filename);
        return;
    }
    // 写入一些随机数据
    char buffer[1024];
    for (int i = 0; i < 1024; ++i) buffer[i] = (char)(i % 256);

    size_t total = sizeMB * 1024 * 1024;
    size_t written = 0;
    while (written < total)
    {
        size_t to_write = (total - written > sizeof(buffer)) ? sizeof(buffer) : (total - written);
        fwrite(buffer, 1, to_write, f);
        written += to_write;
    }
    fclose(f);
    printf("[系统] 已创建测试文件 '%s' (%zu MB)\n", filename, sizeMB);
}

// UI回调：进度更新
static void _ui_progress_callback(int taskId, double percentage, double speed)
{
    if (g_currentWindow)
    {
        ProgressBar_Update(&g_currentWindow->main_progress_bar, (float)percentage);
        ProgressBar_Render(&g_currentWindow->main_progress_bar);
    }
}

// UI回调：错误处理
static void _ui_error_callback(int taskId, int errorCode, const char* msg)
{
    printf("\n[任务 %d 错误] 代码: %d, 信息: %s\n", taskId, errorCode, msg);
}

void MainWindow_RunLoop(MainWindow* win)
{
    if (!win) return;
    g_currentWindow = win; // 绑定当前窗口以便回调使用

    win->is_running = 1;

    // 初始化 Core
    InitTaskManager();
    InitTransferEngine();

    char inputBuffer[256];

    while (win->is_running)
    {
        printf("\n========================================\n");
        printf("       SafeTrix 任务管理控制台          \n");
        printf("========================================\n");
        printf(" 1. 创建测试文件 (10MB)                 \n");
        printf(" 2. 添加新任务                          \n");
        printf(" 3. 查看任务列表                        \n");
        printf(" 4. 运行任务 (阻塞执行)                 \n");
        printf(" 0. 退出                                \n");
        printf("========================================\n");
        printf("请输入编号: ");

        if (scanf("%s", inputBuffer) != 1) break;
        int choice = atoi(inputBuffer);

        switch (choice)
        {
        case 1:
            {
                CreateDummyFile("test_source.dat", 10);
                break;
            }
        case 2:
            {
                char src[128], dest[128];
                printf("请输入源文件路径: ");
                scanf("%s", src);
                printf("请输入目标文件路径: ");
                scanf("%s", dest);

                int id = AddTask(src, dest, 1);
                if (id > 0)
                {
                    printf("[成功] 任务已创建，编号: %d\n", id);
                    // 默认绑定回调
                    SetTaskCallbacks(id, _ui_progress_callback, _ui_error_callback);
                }
                else
                {
                    printf("[错误] 任务创建失败。\n");
                }
                break;
            }
        case 3:
            {
                int count = 0;
                TransferTask* list = GetTaskList(&count);
                printf("\n--- 当前任务 (%d) ---\n", count);
                for (int i = 0; i < count; i++)
                {
                    const char* statusStr = "未知";
                    switch (list[i].status)
                    {
                    case TASK_WAITING: statusStr = "等待中";
                        break;
                    case TASK_RUNNING: statusStr = "运行中";
                        break;
                    case TASK_PAUSED: statusStr = "已暂停";
                        break;
                    case TASK_COMPLETED: statusStr = "已完成";
                        break;
                    case TASK_ERROR: statusStr = "异常";
                        break;
                    }
                    printf("ID:%d 状态:%-8s 进度:%llu/%llu 源:%s -> 目标:%s\n",
                           list[i].id, statusStr, list[i].currentOffset, list[i].totalSize,
                           list[i].srcPath, list[i].destPath);
                }
                break;
            }
        case 4:
            {
                printf("请输入需要启动的任务编号: ");
                int runId;
                if (scanf("%d", &runId) == 1)
                {
                    TransferTask* task = GetTaskById(runId);
                    if (task)
                    {
                        printf("[系统] 正在启动任务 %d ...\n", runId);
                        RunTask(task);
                        printf("\n[系统] 任务 %d 已结束。\n", runId);
                    }
                    else
                    {
                        printf("[警告] 未找到该任务。\n");
                    }
                }
                break;
            }
        case 0:
            win->is_running = 0;
            printf("正在退出程序...\n");
            break;
        default:
            printf("无效的输入，请重新选择。\n");
            break;
        }
    }
    g_currentWindow = NULL;
}

void MainWindow_Destroy(MainWindow* win)
{
    // 清理逻辑
}
