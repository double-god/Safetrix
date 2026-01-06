#ifndef STARTUP_H
#define STARTUP_H

#ifdef __cplusplus
extern "C" {

#endif

/* 返回版权字符串（UTF-8 编码） */
const char* Startup_GetCopyright(void);

/* 在启动时显示版权：控制台输出；Windows 下弹出 MessageBox（自动将 UTF-8 转为宽字符） */
void Startup_ShowCopyright(void);

#ifdef __cplusplus
}
#endif

#endif // STARTUP_H

