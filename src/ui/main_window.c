#include "ui/MainWindow.h"
#include "core/TaskManager.h"     // 引入任务管理器
#include "core/TransferEngine.h"  // 引入传输引擎
#include "utils/FileUtils.h"      // 引入工具
#include <stdio.h>
#include <string.h>

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
    printf("APP: %s\n", win->title);
    printf("MSG: %s\n", UI_GetWelcomeText());
    printf("THEME: %s\n", UI_GetThemeColor());
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

void MainWindow_RunLoop(MainWindow* win)
{
    if (!win) return;
    g_currentWindow = win; // 绑定当前窗口以便回调使用

    win->is_running = 1;
    printf("System Ready. Preparing tasks...\n");

    // 1. 初始化 Core (如果 main.c 没做，这里做也可以，但最好在 main 做)
    InitTaskManager();
    InitTransferEngine();

    // 2. 创建一个测试文件 (为了让你能看到效果)
    const char* testSrc = "test_source.dat";
    const char* testDest = "test_dest.dat.enc"; // 加密后缀

    // 临时生成一个大文件用于测试 (10MB)
    FILE* fp = fopen(testSrc, "wb");
    if (fp)
    {
        uint8_t dummy[1024]; // 1KB
        memset(dummy, 0xAA, 1024);
        for (int i = 0; i < 1024 * 10; i++) fwrite(dummy, 1, 1024, fp);
        fclose(fp);
        printf("[Test] Created dummy file: %s (10MB)\n", testSrc);
    }

    // 3. 添加任务
    int taskId = AddTask(testSrc, testDest, 1);
    if (taskId < 0)
    {
        printf("[Error] Failed to add task. Error: %d\n", taskId);
        return;
    }
    printf("[Task] Task Added ID: %d\n", taskId);

    // 4. 设置回调 -> 连接 UI 进度条
    SetTaskCallbacks(taskId, _ui_callback, NULL);

    // 5. 执行任务 (注意：目前的 RunTask 是阻塞的，会卡住主线程直到完成)
    // 在 GUI 程序中这应该放在线程里，但控制台程序可以直接跑
    printf("Starting Transfer...\n");
    TransferTask* task = GetTaskById(taskId);
    if (task)
    {
        RunTask(task); // 这会触发 _ui_callback 并更新进度条
    }

    printf("\n[System] All tasks finished.\n");
    win->is_running = 0;
    g_currentWindow = NULL;
}

void MainWindow_Destroy(MainWindow* win)
{
    // 清理逻辑
}
