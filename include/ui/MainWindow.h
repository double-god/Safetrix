#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "ProgressBar.h"

// 主窗口结构体，包含子控件
typedef struct
{
    char title[64];
    int is_running;
    ProgressBar main_progress_bar;
} MainWindow;

// 初始化主窗口
void MainWindow_Init(MainWindow* win, const char* title);

// 显示主窗口
void MainWindow_Show(MainWindow* win);

// 主窗口事件循环
void MainWindow_RunLoop(MainWindow* win);

// 销毁/清理主窗口
void MainWindow_Destroy(MainWindow* win);

// 声明 ui_resources.c 中可能提供的辅助函数 (为了不创建新头文件)
const char* UI_GetThemeColor();
const char* UI_GetWelcomeText();

#endif // MAIN_WINDOW_H
