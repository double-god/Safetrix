#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "ui/MainWindow.h"
#include "data/Logger.h"      // 新增
#include "utils/Algorithm.h"  // 新增

int main(int argc, char* argv[])
{
#ifdef _WIN32
    // 恢复使用系统默认控制台编码（注释掉强制 UTF-8），避免与文件系统 API 的编码不匹配
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
#endif

    // 1. 初始化基础服务
    Logger_Init("data/app.log");
    Algorithm_InitCRC32();

    Logger_Log(LOG_INFO, "System Booting...");

    // ... (中间的 TaskManager 和 Engine 初始化) ...
    // InitTaskManager(); // 确保 TaskManager 内部会调用 Persistence_LoadTasks

    // 2. 初始化 UI
    MainWindow appWindow;
    MainWindow_Init(&appWindow, "Safetrix v1.0");

    // 3. 运行
    MainWindow_Show(&appWindow);
    MainWindow_RunLoop(&appWindow);

    // 4. 退出清理
    MainWindow_Destroy(&appWindow);
    Logger_Log(LOG_INFO, "System Shutdown.");
    Logger_Close();

    return 0;
}
