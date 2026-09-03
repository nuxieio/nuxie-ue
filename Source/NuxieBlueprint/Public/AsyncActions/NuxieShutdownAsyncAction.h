#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "NuxieTypes.h"
#include "NuxieShutdownAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNuxieShutdownSuccessEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieShutdownFailureEvent,
  const FNuxieError&,
  Error);

UCLASS()
class NUXIEBLUEPRINT_API UNuxieShutdownAsyncAction
  : public UBlueprintAsyncActionBase
{
  GENERATED_BODY()

public:
  UFUNCTION(
    BlueprintCallable,
    meta = (
      BlueprintInternalUseOnly = "true",
      WorldContext = "WorldContextObject"),
    Category = "Nuxie|Async")
  static UNuxieShutdownAsyncAction* ShutdownNuxie(
    UObject* WorldContextObject);

  virtual void Activate() override;

  UPROPERTY(BlueprintAssignable)
  FNuxieShutdownSuccessEvent OnSuccess;

  UPROPERTY(BlueprintAssignable)
  FNuxieShutdownFailureEvent OnFailed;

private:
  UPROPERTY()
  TObjectPtr<UObject> WorldContextObject;
};
