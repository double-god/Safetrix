#include "ui/ProgressBar.h"
#include <stdio.h>

void ProgressBar_Init(ProgressBar* bar, int id, int width)
{
    if (!bar) return;
    bar->id = id;
    bar->current_percent = 0.0f;
    bar->width = (width > 0) ? width : 50;
}

void ProgressBar_Update(ProgressBar* bar, float percent)
{
    if (!bar) return;
    if (percent < 0.0f) percent = 0.0f;
    if (percent > 100.0f) percent = 100.0f;
    bar->current_percent = percent;
}

void ProgressBar_Render(const ProgressBar* bar)
{
    if (!bar) return;

    // 模拟绘制一个文本进度条 [====>    ] 45%
    printf("\r[");
    int pos = (int)((bar->width * bar->current_percent) / 100.0f);
    for (int i = 0; i < bar->width; ++i)
    {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %.1f%%", bar->current_percent);
    fflush(stdout); // 刷新缓冲区以确保实时显示
}
