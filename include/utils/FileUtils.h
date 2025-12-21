//
// Created by HaoTang on 2025/12/17.
//

#ifndef UTILS_FILE_UTILS_H
#define UTILS_FILE_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {



#endif

// Check if file exists
bool FileUtils_Exists(const char* filepath);

// Open file with UTF-8 path support on Windows
FILE* FileUtils_OpenFileUTF8(const char* path, const char* mode);

// Create directory with UTF-8 path support on Windows
int FileUtils_Mkdir(const char* path);

// Get file size (used for progress calculation)
uint64_t FileUtils_GetFileSize(const char* filepath);

#ifdef __cplusplus
}
#endif

#endif // UTILS_FILE_UTILS_H
