#include "AsyncActions/NuxieHasFeatureAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "NuxieSubsystem.h"

namespace
{
  UNuxieSubsystem* ResolveSubsystem(
    UObject* WorldContextObject,
    FNuxieError& OutError)
  {
    if (WorldContextObject == nullptr)
    {
      OutError = FNuxieError::Make(
        TEXT("NO_WORLD_CONTEXT"),
        TEXT("World context object is required."));
      return nullptr;
    }

    UWorld* World = GEngine->GetWorldFromContextObject(
      WorldContextObject,
      EGetWorldErrorMode::ReturnNull);
    if (World == nullptr || World->GetGameInstance() == nullptr)
    {
      OutError = FNuxieError::Make(
        TEXT("NO_GAME_INSTANCE"),
        TEXT("Unable to resolve game instance."));
      return nullptr;
    }

    UNuxieSubsystem* Subsystem =
      World->GetGameInstance()->GetSubsystem<UNuxieSubsystem>();
    if (Subsystem == nullptr)
    {
      OutError = FNuxieError::Make(
        TEXT("NO_SUBSYSTEM"),
        TEXT("Nuxie subsystem is unavailable."));
    }
    return Subsystem;
  }
}

UNuxieHasFeatureAsyncAction*
UNuxieHasFeatureAsyncAction::HasNuxieFeature(
  UObject* WorldContextObjectIn,
  const FString& FeatureIdIn,
  double RequiredBalanceIn,
  const FString& EntityIdIn,
  ENuxieFeatureCheckPolicy PolicyIn)
{
  UNuxieHasFeatureAsyncAction* Action =
    NewObject<UNuxieHasFeatureAsyncAction>();
  Action->WorldContextObject = WorldContextObjectIn;
  Action->FeatureId = FeatureIdIn;
  Action->RequiredBalance = RequiredBalanceIn;
  Action->EntityId = EntityIdIn;
  Action->Policy = PolicyIn;
  Action->RegisterWithGameInstance(WorldContextObjectIn);
  return Action;
}

void UNuxieHasFeatureAsyncAction::Activate()
{
  FNuxieError ResolveError;
  UNuxieSubsystem* Subsystem =
    ResolveSubsystem(WorldContextObject, ResolveError);
  if (Subsystem == nullptr)
  {
    OnFailed.Broadcast(ResolveError);
    SetReadyToDestroy();
    return;
  }

  TWeakObjectPtr<UNuxieHasFeatureAsyncAction> WeakThis(this);
  Subsystem->HasFeatureAsync(
    FeatureId,
    RequiredBalance,
    EntityId,
    Policy,
    [WeakThis](const FNuxieFeatureAccess& Access)
    {
      if (WeakThis.IsValid())
      {
        WeakThis->OnSuccess.Broadcast(Access);
        WeakThis->SetReadyToDestroy();
      }
    },
    [WeakThis](const FNuxieError& Error)
    {
      if (WeakThis.IsValid())
      {
        WeakThis->OnFailed.Broadcast(Error);
        WeakThis->SetReadyToDestroy();
      }
    });
}
