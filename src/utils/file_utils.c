#include "utils/FileUtils.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#include <tchar.h>
#include <stdlib.h>
#include <sys/stat.h>

// Convert multibyte (ANSI) string to wide string (UTF-16) using system code page (CP_ACP)
static wchar_t* mbcs_to_wide_alloc(const char* s)
{
    if (!s) return NULL;
    int needed = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    if (needed <= 0) return NULL;
    wchar_t* w = (wchar_t*)malloc(sizeof(wchar_t) * needed);
    if (!w) return NULL;
    MultiByteToWideChar(CP_ACP, 0, s, -1, w, needed);
    return w;
}
#endif

bool FileUtils_Exists(const char* filepath)
{
    if (!filepath) return false;
#ifdef _WIN32
    wchar_t* wpath = mbcs_to_wide_alloc(filepath);
    if (!wpath) return false;
    struct _stat64 st;
    int r = _wstat64(wpath, &st);
    free(wpath);
    return (r == 0);
#else
    struct stat buffer;
    return (stat(filepath, &buffer) == 0);
#endif
}

uint64_t FileUtils_GetFileSize(const char* filepath)
{
    if (!filepath) return 0;
#ifdef _WIN32
    wchar_t* wpath = mbcs_to_wide_alloc(filepath);
    if (!wpath) return 0;
    struct _stat64 st;
    if (_wstat64(wpath, &st) == 0)
    {
        free(wpath);
        return (uint64_t)st.st_size;
    }
    free(wpath);
    return 0;
#else
    struct stat buffer;
    if (stat(filepath, &buffer) == 0)
    {
        return (uint64_t)buffer.st_size;
    }
    return 0;
#endif
}
