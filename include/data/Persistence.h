#ifndef DATA_PERSISTENCE_H
#define DATA_PERSISTENCE_H

#include "common/AppTypes.h"

// 保存任务列表到数据库文件
int Persistence_SaveTasks(const char* dbPath, TransferTask* tasks, int count);

// 从数据库文件加载任务列表，返回实际加载的任务数量
int Persistence_LoadTasks(const char* dbPath, TransferTask* outTasks, int maxCount);

#endif // DATA_PERSISTENCE_H

