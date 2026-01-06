#include "ui/startup.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 版权文本（UTF-8） */
static const char COPYRIGHT_TEXT[] = "版权说明©2025 南昌大学软件学院";

const char* Startup_GetCopyright(void)
{
    return COPYRIGHT_TEXT;
}

void Startup_ShowCopyright(void)
{
    /* 控制台输出：UTF-8 文本直接打印（主程序已设置控制台为 UTF-8） */
    printf("%s\n", COPYRIGHT_TEXT);
}
