#include <stddef.h>

// 提供给 MainWindow 使用的资源函数
// 注意：由于没有 include/ui/ui_resources.h，我们在 MainWindow.h 中声明了这些原型

const char* UI_GetThemeColor()
{
    return "#1A1A1D"; // 假设的十六进制颜色
}

const char* UI_GetWelcomeText()
{
    return "Welcome to Safetrix Secure Transfer System";
}
