#include "AsyncActions/NuxieDismissAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "NuxieSubsystem.h"

UNuxieDismissAsyncAction* UNuxieDismissAsyncAction::DismissNuxie(
  UObject* WorldContextObjectIn)
{
  UNuxieDismissAsyncAction* Action =
    NewObject<UNuxieDismissAsyncAction>();
  Action->WorldContextObject = WorldContextObjectIn;
  Action->RegisterWithGameInstance(WorldContextObjectIn);
  return Action;
}

void UNuxieDismissAsyncAction::Activate()
{
  UWorld* World = WorldContextObject != nullptr
    ? GEngine->GetWorldFromContextObject(
      WorldContextObject,
      EGetWorldErrorMode::ReturnNull)
    : nullptr;
  UNuxieSubsystem* Subsystem =
    World != nullptr && World->GetGameInstance() != nullptr
      ? World->GetGameInstance()->GetSubsystem<UNuxieSubsystem>()
      : nullptr;
  if (Subsystem == nullptr)
  {
    OnFailed.Broadcast(FNuxieError::Make(
      TEXT("NO_SUBSYSTEM"),
      TEXT("Nuxie subsystem is unavailable.")));
    SetReadyToDestroy();
    return;
  }

  TWeakObjectPtr<UNuxieDismissAsyncAction> WeakThis(this);
  Subsystem->DismissAsync(
    FSimpleDelegate::CreateLambda([WeakThis]()
    {
      if (WeakThis.IsValid())
      {
        WeakThis->OnSuccess.Broadcast();
        WeakThis->SetReadyToDestroy();
      }
    }),
    [WeakThis](const FNuxieError& Error)
    {
      if (WeakThis.IsValid())
      {
        WeakThis->OnFailed.Broadcast(Error);
        WeakThis->SetReadyToDestroy();
      }
    });
}
