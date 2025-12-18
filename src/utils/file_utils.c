#include "utils/FileUtils.h"
#include <stdio.h>
#include <sys/stat.h>

bool FileUtils_Exists(const char* filepath)
{
    if (!filepath) return false;
    struct stat buffer;
    return (stat(filepath, &buffer) == 0);
}

uint64_t FileUtils_GetFileSize(const char* filepath)
{
    if (!filepath) return 0;
    struct stat buffer;
    if (stat(filepath, &buffer) == 0)
    {
        return (uint64_t)buffer.st_size;
    }
    return 0;
}
