#include "ui/MainWindow.h"
#include <stdio.h>
#include <string.h>

// 模拟休眠，实际项目中可能在 utils 或 core 中
#include <time.h>

void _ui_sleep_ms(int ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

void MainWindow_Init(MainWindow* win, const char* title)
{
    if (!win) return;

    // 1. 设置标题
    strncpy(win->title, title, sizeof(win->title) - 1);

    // 2. 初始化子控件 (进度条)
    ProgressBar_Init(&win->main_progress_bar, 1, 40);

    win->is_running = 0;
}

void MainWindow_Show(MainWindow* win)
{
    if (!win) return;

    // 使用 ui_resources.c 中的资源
    printf("------------------------------------------------\n");
    printf("APP: %s\n", win->title);
    printf("MSG: %s\n", UI_GetWelcomeText());
    printf("THEME: %s\n", UI_GetThemeColor());
    printf("------------------------------------------------\n");
}

void MainWindow_RunLoop(MainWindow* win)
{
    if (!win) return;

    win->is_running = 1;
    printf("GUI Loop Started. Simulating user activity...\n");

    // 模拟一个任务进度，实际中这里会接收 Core 层的事件
    float progress = 0.0f;
    while (win->is_running)
    {
        progress += 5.0f;

        // 更新进度条数据
        ProgressBar_Update(&win->main_progress_bar, progress);
 // 渲染进度条
        ProgressBar_Render(&win->main_progress_bar);

        if (progress >= 100.0f)
        {
            printf("\nTask Complete.\n");
            win->is_running = 0; // 退出循环
        }

        _ui_sleep_ms(200); // 模拟帧率控制
    }
}

void MainWindow_Destroy(MainWindow* win)
{
    if (!win) return;
    printf("Cleaning up MainWindow resources...\n");
    // 这里执行具体的清理工作
}