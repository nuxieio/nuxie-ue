#include "AsyncActions/NuxieTriggerAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "NuxieSubsystem.h"

UNuxieTriggerAsyncAction* UNuxieTriggerAsyncAction::StartNuxieTrigger(
  UObject* WorldContextObjectIn,
  const FString& EventNameIn,
  const FNuxieTriggerOptions& OptionsIn)
{
  UNuxieTriggerAsyncAction* Action = NewObject<UNuxieTriggerAsyncAction>();
  Action->WorldContextObject = WorldContextObjectIn;
  Action->EventName = EventNameIn;
  Action->Options = OptionsIn;
  return Action;
}

void UNuxieTriggerAsyncAction::Activate()
{
  if (WorldContextObject == nullptr)
  {
    OnFailed.Broadcast(FNuxieError::Make(TEXT("NO_WORLD_CONTEXT"), TEXT("World context object is required.")));
    SetReadyToDestroy();
    return;
  }

  UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
  if (World == nullptr || World->GetGameInstance() == nullptr)
  {
    OnFailed.Broadcast(FNuxieError::Make(TEXT("NO_GAME_INSTANCE"), TEXT("Unable to resolve game instance.")));
    SetReadyToDestroy();
    return;
  }

  Subsystem = World->GetGameInstance()->GetSubsystem<UNuxieSubsystem>();
  if (Subsystem == nullptr)
  {
    OnFailed.Broadcast(FNuxieError::Make(TEXT("NO_SUBSYSTEM"), TEXT("Nuxie subsystem is unavailable.")));
    SetReadyToDestroy();
    return;
  }

  TriggerUpdateHandle = Subsystem->OnTriggerUpdateNative.AddUObject(this, &UNuxieTriggerAsyncAction::HandleSubsystemTriggerUpdate);

  FNuxieError Error;
  if (!Subsystem->StartTrigger(EventName, Options, RequestId, Error))
  {
    CleanupBinding();
    OnFailed.Broadcast(Error);
    SetReadyToDestroy();
    return;
  }
}

void UNuxieTriggerAsyncAction::Cancel()
{
  if (Subsystem != nullptr && !RequestId.IsEmpty())
  {
    FNuxieError IgnoreError;
    Subsystem->CancelTrigger(RequestId, IgnoreError);
  }

  CleanupBinding();
  Super::Cancel();
  SetReadyToDestroy();
}

void UNuxieTriggerAsyncAction::HandleSubsystemTriggerUpdate(const FString& InRequestId, const FNuxieTriggerUpdate& Update)
{
  if (InRequestId != RequestId)
  {
    return;
  }

  OnUpdate.Broadcast(Update);

  if (Update.bIsTerminal || Nuxie::FTriggerContract::IsTerminal(Update))
  {
    CleanupBinding();
    OnCompleted.Broadcast(Update);
    SetReadyToDestroy();
  }
}

void UNuxieTriggerAsyncAction::CleanupBinding()
{
  if (Subsystem != nullptr && TriggerUpdateHandle.IsValid())
  {
    Subsystem->OnTriggerUpdateNative.Remove(TriggerUpdateHandle);
    TriggerUpdateHandle.Reset();
  }
}
