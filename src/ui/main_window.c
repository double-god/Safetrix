#include "ui/MainWindow.h"
#include "core/TaskManager.h"     // 引入任务管理器
#include "core/TransferEngine.h"  // 引入传输引擎
#include "utils/FileUtils.h"      // 引入工具
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h> // 用于检测目标路径是否为目录
#include <stdarg.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

// 辅助：跨平台的 UI 输出，保证 Windows 控制台能正确显示 UTF-8 字符串
static void UI_Print(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
#ifdef _WIN32
    char utf8buf[8192];
    int n = vsnprintf(utf8buf, sizeof(utf8buf), fmt, ap);
    if (n < 0)
    {
        va_end(ap);
        return;
    }
    // 将 UTF-8 转为 UTF-16
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8buf, -1, NULL, 0);
    if (wlen > 0)
    {
        wchar_t* wbuf = (wchar_t*)malloc(sizeof(wchar_t) * (wlen));
        if (wbuf)
        {
            MultiByteToWideChar(CP_UTF8, 0, utf8buf, -1, wbuf, wlen);
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            if (h && h != INVALID_HANDLE_VALUE)
            {
                DWORD written = 0;
                // 写入不包含末尾 NUL 的字符数
                WriteConsoleW(h, wbuf, wlen - 1, &written, NULL);
            }
            free(wbuf);
        }
    }
#else
    vprintf(fmt, ap);
#endif
    va_end(ap);
}

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
    UI_Print("------------------------------------------------\n");
    UI_Print("应用: %s\n", win->title);
    UI_Print("公告: %s\n", UI_GetWelcomeText());
    UI_Print("主题: %s\n", UI_GetThemeColor());
    UI_Print("------------------------------------------------\n");
}

// --- 专门为 UI 定义的适配回调 ---
// 为了让回调能更新 MainWindow 的进度条，我们需要一个静态指针或者传递 context
static MainWindow* g_currentWindow = NULL;

// 辅助函数：生成一个测试文件
static void CreateDummyFile(const char* filename, size_t sizeMB)
{
    FILE* f = FileUtils_OpenFileUTF8(filename, "wb");
    if (!f)
    {
        UI_Print("[错误] 无法创建文件: %s\n", filename);
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
    UI_Print("[系统] 已创建测试文件 '%s' (%zu MB)\n", filename, sizeMB);
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
    UI_Print("\n[任务 %d 错误] 代码: %d, 信息: %s\n", taskId, errorCode, msg);
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
        UI_Print("\n========================================\n");
        UI_Print("       SafeTrix 安全传输控制台          \n");
        UI_Print("========================================\n");
        UI_Print(" 1. 创建测试文件 (10MB)                 \n");
        UI_Print(" 2. 添加传输任务 (加密/解密)            \n");
        UI_Print(" 3. 查看任务列表                        \n");
        UI_Print(" 4. 运行任务 (阻塞执行)                 \n");
        UI_Print(" 0. 退出                                \n");
        UI_Print("========================================\n");
        UI_Print(" [提示] 本工具采用对称加密。\n");
        UI_Print("       - 加密：选择普通文件 -> 输出乱码文件\n");
        UI_Print("       - 解密：选择乱码文件 -> 输出普通文件\n");
        UI_Print("========================================\n");
        UI_Print("请输入编号: ");

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

                UI_Print("\n--- 添加任务 (输入空行取消) ---\n");
                UI_Print("提示：支持绝对路径 (如 C:\\Data\\file.txt) 或相对路径，路径可包含空格。\n");

                UI_Print("源文件路径: ");
                SafeGetLine(src, (int)sizeof(src));
                if (strlen(src) == 0)
                {
                    UI_Print("已取消。\n");
                    break;
                }

                if (!FileUtils_Exists(src))
                {
                    UI_Print("[错误] 找不到文件: %s\n", src);
                    break;
                }

                UI_Print("目标文件路径: ");
                SafeGetLine(dest, (int)sizeof(dest));
                if (strlen(dest) == 0)
                {
                    UI_Print("已取消。\n");
                    break;
                }

                // --- 启发式规则: 如果 src 有扩展名但 dest 没有扩展名，则把 dest 当作目录处理 (追加分隔符) ---
                {
                    const char* src_fname = strrchr(src, '\\');
                    const char* src_fname2 = strrchr(src, '/');
                    const char* src_name = src;
                    if (src_fname && src_fname > src_name) src_name = src_fname + 1;
                    if (src_fname2 && src_fname2 > src_name) src_name = src_fname2 + 1;
                    const char* src_dot = strrchr(src_name, '.');

                    const char* dest_fname = strrchr(dest, '\\');
                    const char* dest_fname2 = strrchr(dest, '/');
                    const char* dest_name = dest;
                    if (dest_fname && dest_fname > dest_name) dest_name = dest_fname + 1;
                    if (dest_fname2 && dest_fname2 > dest_name) dest_name = dest_fname2 + 1;
                    const char* dest_dot = strrchr(dest_name, '.');

                    if (src_dot != NULL && dest_dot == NULL)
                    {
                        // src 有扩展名，dest 没有；将 dest 末尾追加分隔符以触发目录拼接逻辑
                        size_t dlen_now = strlen(dest);
                        if (dlen_now + 1 < sizeof(dest))
                        {
                            // 如果末尾没有分隔符就添加
                            if (dlen_now == 0 || (dest[dlen_now - 1] != '\\' && dest[dlen_now - 1] != '/'))
                            {
                                dest[dlen_now] = '\\';
                                dest[dlen_now + 1] = '\0';
                                UI_Print("[提示] 目标看起来像目录，已自动追加分隔符以便拼接文件名。\n");
                            }
                        }
                    }
                }

                // --- 新增: 如果 dest 是目录，或用户输入以分隔符结尾（尽管路径可能不存在），则自动拼接源文件名 ---
                struct stat statbuf;
                int stat_ok = (stat(dest, &statbuf) == 0);
                int is_dir = 0;
                if (stat_ok)
                {
                    is_dir = (statbuf.st_mode & S_IFDIR) != 0;
                }
                else
                {
                    // stat 失败（路径不存在）——如果用户输入以分隔符结尾，我们也把它当作目录处理
                    size_t dlen_tmp = strlen(dest);
                    if (dlen_tmp > 0 && (dest[dlen_tmp - 1] == '\\' || dest[dlen_tmp - 1] == '/'))
                    {
                        is_dir = 1;
                    }
                }

                if (is_dir)
                {
                    // 提取 src 的文件名部分
                    const char* filename = NULL;
                    const char* p = strrchr(src, '\\');
                    const char* p2 = strrchr(src, '/');
                    if (p && p2) filename = (p > p2) ? p : p2;
                    else if (p) filename = p;
                    else if (p2) filename = p2;
                    else filename = src;
                    if (filename != src && (*filename == '\\' || *filename == '/')) filename++;

                    // 处理 dest 末尾是否已有分隔符
                    size_t dlen = strlen(dest);
                    char newDest[1024];
                    if (dlen > 0 && (dest[dlen - 1] == '\\' || dest[dlen - 1] == '/'))
                    {
                        snprintf(newDest, sizeof(newDest), "%s%s", dest, filename);
                    }
                    else
                    {
                        snprintf(newDest, sizeof(newDest), "%s\\%s", dest, filename);
                    }

                    // 更新 dest
                    strncpy(dest, newDest, sizeof(dest) - 1);
                    dest[sizeof(dest) - 1] = '\0';
                    UI_Print("[智能修正] 检测到目标是目录，已自动修改为: %s\n", dest);
                }

                int id = AddTask(src, dest, 1);
                if (id > 0)
                {
                    UI_Print("[成功] 任务已加入队列 (ID: %d)。\n", id);
                    UI_Print("提示：请选择菜单 '4' 开始传输。\n");
                    // 默认绑定回调
                    SetTaskCallbacks(id, _ui_progress_callback, _ui_error_callback);
                }
                else
                {
                    UI_Print("[错误] 任务创建失败 (错误码: %d)\n", id);
                }
                break;
            }
        case 3:
            {
                int count = 0;
                TransferTask* list = GetTaskList(&count);
                UI_Print("\n--- 当前任务 (%d) ---\n", count);
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
                    UI_Print("ID:%d 状态:%-8s 进度:%llu/%llu 源:%s -> 目标:%s\n",
                             list[i].id, statusStr, list[i].currentOffset, list[i].totalSize,
                             list[i].srcPath, list[i].destPath);
                }
                break;
            }
        case 4:
            {
                char idBuf[64];
                UI_Print("请输入需要启动的任务编号: ");
                SafeGetLine(idBuf, (int)sizeof(idBuf));
                if (strlen(idBuf) == 0)
                {
                    UI_Print("已取消。\n");
                    break;
                }
                int runId = atoi(idBuf);
                TransferTask* task = GetTaskById(runId);
                if (task)
                {
                    UI_Print("[系统] 正在启动任务 %d ... (该操作为阻塞式执行，按 Ctrl+C 可中断)\n", runId);
                    // 如果需要非阻塞执行，应将 RunTask 放到线程中或改为状态机
                    RunTask(task);
                    UI_Print("\n[系统] 任务 %d 已结束。\n", runId);
                }
                else
                {
                    UI_Print("[警告] 未找到该任务。\n");
                }
                break;
            }
        case 0:
            win->is_running = 0;
            UI_Print("正在退出程序...\n");
            break;
        default:
            UI_Print("无效的输入，请重新选择。\n");
            break;
        }
    }
    g_currentWindow = NULL;
}

void MainWindow_Destroy(MainWindow* win)
{
    // 预留的清理逻辑
}
