#ifndef CORE_TRANSFER_ENGINE_H
#define CORE_TRANSFER_ENGINE_H

#include "common/app_types.h"

// 初始化引擎
int InitTransferEngine(void);

// 执行任务 (阻塞式，用于演示)
int RunTask(TransferTask* task);

#endif // CORE_TRANSFER_ENGINE_H
