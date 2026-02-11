#include "NuxieAsyncQueue.h"

#include "Async/Async.h"

void NuxieRunOnGameThread(TFunction<void()> Work)
{
  if (IsInGameThread())
  {
    Work();
    return;
  }

  AsyncTask(ENamedThreads::GameThread, [Work = MoveTemp(Work)]() mutable
  {
    Work();
  });
}
