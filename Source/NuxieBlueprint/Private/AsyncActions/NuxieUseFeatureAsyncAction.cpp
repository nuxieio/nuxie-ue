#include "AsyncActions/NuxieUseFeatureAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "NuxieSubsystem.h"

UNuxieUseFeatureAsyncAction*
UNuxieUseFeatureAsyncAction::UseNuxieFeatureAndWait(
  UObject* WorldContextObjectIn,
  const FString& FeatureIdIn,
  double AmountIn,
  const FString& EntityIdIn,
  bool bSetUsageIn,
  const TMap<FString, FNuxieScalarValue>& MetadataIn)
{
  UNuxieUseFeatureAsyncAction* Action =
    NewObject<UNuxieUseFeatureAsyncAction>();
  Action->WorldContextObject = WorldContextObjectIn;
  Action->FeatureId = FeatureIdIn;
  Action->Amount = AmountIn;
  Action->EntityId = EntityIdIn;
  Action->bSetUsage = bSetUsageIn;
  Action->Metadata = MetadataIn;
  Action->RegisterWithGameInstance(WorldContextObjectIn);
  return Action;
}

void UNuxieUseFeatureAsyncAction::Activate()
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

  TWeakObjectPtr<UNuxieUseFeatureAsyncAction> WeakThis(this);
  Subsystem->UseFeatureAndWaitAsync(
    FeatureId,
    Amount,
    EntityId,
    bSetUsage,
    Metadata,
    [WeakThis](const FNuxieFeatureUsageResult& Result)
    {
      if (WeakThis.IsValid())
      {
        WeakThis->OnSuccess.Broadcast(Result);
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
