#ifndef CORE_TRANSFER_ENGINE_H
#define CORE_TRANSFER_ENGINE_H

#include "common/AppTypes.h"

void InitTransferEngine(void);
int RunTask(TransferTask* task);
void StopTransfer(int taskId);

#endif // CORE_TRANSFER_ENGINE_H
