#ifndef CORE_TASK_MANAGER_H
#define CORE_TASK_MANAGER_H

#include "common/app_types.h"

void InitTaskManager(void);
int AddTask(const char* src, const char* dest, int priority);
TransferTask* GetTaskById(int id);
TransferTask* GetTaskList(int* count);
void SetTaskCallbacks(int taskId, OnProgressCallback onProgress, OnErrorCallback onError);

#endif // CORE_TASK_MANAGER_H
