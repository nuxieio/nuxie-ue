#pragma once

#include "CoreMinimal.h"
#include "Kismet/CancellableAsyncAction.h"

#include "NuxieTypes.h"
#include "NuxieTriggerAsyncAction.generated.h"

class UNuxieSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieTriggerAsyncUpdateEvent, const FNuxieTriggerUpdate&, Update);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieTriggerAsyncCompletedEvent, const FNuxieTriggerUpdate&, TerminalUpdate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieTriggerAsyncFailedEvent, const FNuxieError&, Error);

UCLASS()
class NUXIEBLUEPRINT_API UNuxieTriggerAsyncAction : public UCancellableAsyncAction
{
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Nuxie|Async")
  static UNuxieTriggerAsyncAction* StartNuxieTrigger(
    UObject* WorldContextObject,
    const FString& EventName,
    const FNuxieTriggerOptions& Options);

  virtual void Activate() override;
  virtual void Cancel() override;

  UPROPERTY(BlueprintAssignable)
  FNuxieTriggerAsyncUpdateEvent OnUpdate;

  UPROPERTY(BlueprintAssignable)
  FNuxieTriggerAsyncCompletedEvent OnCompleted;

  UPROPERTY(BlueprintAssignable)
  FNuxieTriggerAsyncFailedEvent OnFailed;

private:
  UFUNCTION()
  void HandleSubsystemTriggerUpdate(const FString& RequestId, const FNuxieTriggerUpdate& Update);

  void CleanupBinding();

  UPROPERTY()
  TObjectPtr<UObject> WorldContextObject;

  UPROPERTY()
  TObjectPtr<UNuxieSubsystem> Subsystem;

  FString EventName;
  FNuxieTriggerOptions Options;
  FString RequestId;
  FDelegateHandle TriggerUpdateHandle;
};
