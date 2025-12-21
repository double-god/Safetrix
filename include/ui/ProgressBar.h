#ifndef PROGRESS_BAR_H
#define PROGRESS_BAR_H

#include <stdint.h>

// 进度条结构体
typedef struct
{
    int id;
    float current_percent; // 0.0 到 100.0
    int width; // 显示宽度
} ProgressBar;

// 初始化进度条
void ProgressBar_Init(ProgressBar* bar, int id, int width);

// 更新进度
void ProgressBar_Update(ProgressBar* bar, float percent);

// 渲染进度条
void ProgressBar_Render(const ProgressBar* bar);

#endif // PROGRESS_BAR_H
