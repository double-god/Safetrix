#include "ui/MainWindow.h"
#include "core/TaskManager.h"     // 引入任务管理器
#include "core/TransferEngine.h"  // 引入传输引擎
#include "utils/FileUtils.h"      // 引入工具
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 安全的读取一行输入，去掉末尾换行符，支持路径里有空格
static void SafeGetLine(char* buffer, int size)
{
    if (!buffer || size <= 0) return;
    if (fgets(buffer, size, stdin))
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }
    }
    else
    {
        // 如果读取失败，确保字符串为空
        if (size > 0) buffer[0] = '\0';
    }
}

// 定义一个回调函数，Core 层进度更新时会调用这个
void OnUIProgressUpdate(int taskId, double percentage, double speedMbS)
{
    // 在纯 C 程序里很难随时拿到 MainWindow 指针，通常需要全局变量或传递上下文参数。
    // 为了保持结构简单，我们让主循环负责渲染，这里可以视情况打印日志。
    // 实际 UI 更新由 RunLoop 完成，但我们仍需要保存最新进度，必要时可直接 printf。
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

// UI 回调：进度更新
static void _ui_progress_callback(int taskId, double percentage, double speed)
{
    if (g_currentWindow)
    {
        ProgressBar_Update(&g_currentWindow->main_progress_bar, (float)percentage);
        ProgressBar_Render(&g_currentWindow->main_progress_bar);
    }
}

// UI 回调：错误处理
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

        // 使用 SafeGetLine 读取整行，支持空格，并避免 scanf 的不安全问题
        SafeGetLine(inputBuffer, (int)sizeof(inputBuffer));
        if (strlen(inputBuffer) == 0) continue; // 空输入忽略
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
                char src[512], dest[512];

                printf("\n--- 添加任务 (输入空行取消) ---\n");
                printf("提示：支持绝对路径 (如 C:\\Data\\file.txt) 或相对路径，路径可包含空格。\n");

                printf("源文件路径: ");
                SafeGetLine(src, (int)sizeof(src));
                if (strlen(src) == 0)
                {
                    printf("已取消。\n");
                    break;
                }

                if (!FileUtils_Exists(src))
                {
                    printf("[错误] 找不到文件: %s\n", src);
                    break;
                }

                printf("目标文件路径: ");
                SafeGetLine(dest, (int)sizeof(dest));
                if (strlen(dest) == 0)
                {
                    printf("已取消。\n");
                    break;
                }

                int id = AddTask(src, dest, 1);
                if (id > 0)
                {
                    printf("[成功] 任务已加入队列 (ID: %d)。\n", id);
                    printf("提示：请选择菜单 '4' 开始传输。\n");
                    // 默认绑定回调
                    SetTaskCallbacks(id, _ui_progress_callback, _ui_error_callback);
                }
                else
                {
                    printf("[错误] 任务创建失败 (错误码: %d)\n", id);
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
                char idBuf[64];
                printf("请输入需要启动的任务编号: ");
                SafeGetLine(idBuf, (int)sizeof(idBuf));
                if (strlen(idBuf) == 0)
                {
                    printf("已取消。\n");
                    break;
                }
                int runId = atoi(idBuf);
                TransferTask* task = GetTaskById(runId);
                if (task)
                {
                    printf("[系统] 正在启动任务 %d ... (该操作为阻塞式执行，按 Ctrl+C 可中断)\n", runId);
                    // 如果需要非阻塞执行，应将 RunTask 放到线程中或改为状态机
                    RunTask(task);
                    printf("\n[系统] 任务 %d 已结束。\n", runId);
                }
                else
                {
                    printf("[警告] 未找到该任务。\n");
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
    // 预留的清理逻辑
}
