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
#include <direct.h>

// Convert UTF-8 string to wide string (UTF-16)
static wchar_t* utf8_to_wide_alloc(const char* s)
{
    if (!s) return NULL;
    int needed = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    if (needed <= 0) return NULL;
    wchar_t* w = (wchar_t*)malloc(sizeof(wchar_t) * needed);
    if (!w) return NULL;
    MultiByteToWideChar(CP_UTF8, 0, s, -1, w, needed);
    return w;
}
#endif

FILE* FileUtils_OpenFileUTF8(const char* path, const char* mode)
{
#ifdef _WIN32
    wchar_t* wPath = utf8_to_wide_alloc(path);
    wchar_t* wMode = utf8_to_wide_alloc(mode);
    FILE* f = NULL;
    if (wPath && wMode)
    {
        f = _wfopen(wPath, wMode);
    }
    if (wPath) free(wPath);
    if (wMode) free(wMode);
    return f;
#else
    return fopen(path, mode);
#endif
}

int FileUtils_Mkdir(const char* path)
{
#ifdef _WIN32
    wchar_t* wPath = utf8_to_wide_alloc(path);
    int res = -1;
    if (wPath)
    {
        res = _wmkdir(wPath);
        free(wPath);
    }
    return res;
#else
    return mkdir(path, 0755);
#endif
}

bool FileUtils_Exists(const char* filepath)
{
    if (!filepath) return false;
#ifdef _WIN32
    wchar_t* wpath = utf8_to_wide_alloc(filepath);
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
    wchar_t* wpath = utf8_to_wide_alloc(filepath);
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
