//
// Created by HaoTang on 2025/12/17.
//

#ifndef UTILS_FILE_UTILS_H
#define UTILS_FILE_UTILS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {



#endif

// Check if file exists
bool FileUtils_Exists(const char* filepath);

// Get file size (used for progress calculation)
uint64_t FileUtils_GetFileSize(const char* filepath);

#ifdef __cplusplus
}
#endif

#endif // UTILS_FILE_UTILS_H
