#include <stdio.h>
#include "ui/MainWindow.h"

// 假设这些头文件存在于 include/core/ 中
// #include "core/TaskManager.h"
// #include "core/Security.h"

int main(int argc, char* argv[]) {
    printf("[System] Booting Safetrix...\n");

    // 1. 初始化 Core 层 (模拟)
    // Security_Init();
    // TaskManager_Init();
    printf("[Core] Services initialized.\n");

    // 2. 初始化 UI 层
    MainWindow appWindow;
    MainWindow_Init(&appWindow, "Safetrix v1.0");

    // 3. 显示 UI 并进入主循环
    MainWindow_Show(&appWindow);
    MainWindow_RunLoop(&appWindow);

    // 4. 清理资源并退出
    MainWindow_Destroy(&appWindow);

    // TaskManager_Shutdown();
    printf("[System] Shutdown complete.\n");

    return 0;
}