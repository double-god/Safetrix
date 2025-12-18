#ifndef CORE_TRANSFER_ENGINE_H
#define CORE_TRANSFER_ENGINE_H

#include "common/AppTypes.h"

void InitTransferEngine(void);
void StartTransfer(TransferTask* task);
void StopTransfer(int taskId);

#endif // CORE_TRANSFER_ENGINE_H
