#include "AsyncActions/NuxieCheckFeatureAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "NuxieSubsystem.h"

UNuxieCheckFeatureAsyncAction* UNuxieCheckFeatureAsyncAction::CheckNuxieFeature(
  UObject* WorldContextObjectIn,
  const FString& FeatureIdIn,
  int32 RequiredBalanceIn,
  const FString& EntityIdIn,
  bool bForceRefreshIn)
{
  UNuxieCheckFeatureAsyncAction* Action = NewObject<UNuxieCheckFeatureAsyncAction>();
  Action->WorldContextObject = WorldContextObjectIn;
  Action->FeatureId = FeatureIdIn;
  Action->RequiredBalance = RequiredBalanceIn;
  Action->EntityId = EntityIdIn;
  Action->bForceRefresh = bForceRefreshIn;
  return Action;
}

void UNuxieCheckFeatureAsyncAction::Activate()
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

  UNuxieSubsystem* Subsystem = World->GetGameInstance()->GetSubsystem<UNuxieSubsystem>();
  if (Subsystem == nullptr)
  {
    OnFailed.Broadcast(FNuxieError::Make(TEXT("NO_SUBSYSTEM"), TEXT("Nuxie subsystem is unavailable.")));
    SetReadyToDestroy();
    return;
  }

  TWeakObjectPtr<UNuxieCheckFeatureAsyncAction> WeakThis(this);
  Subsystem->CheckFeatureAsync(
    FeatureId,
    RequiredBalance,
    EntityId,
    bForceRefresh,
    [WeakThis](const FNuxieFeatureCheckResult& Result)
    {
      if (!WeakThis.IsValid())
      {
        return;
      }

      WeakThis->OnSuccess.Broadcast(Result);
      WeakThis->SetReadyToDestroy();
    },
    [WeakThis](const FNuxieError& Error)
    {
      if (!WeakThis.IsValid())
      {
        return;
      }

      WeakThis->OnFailed.Broadcast(Error);
      WeakThis->SetReadyToDestroy();
    });
}
