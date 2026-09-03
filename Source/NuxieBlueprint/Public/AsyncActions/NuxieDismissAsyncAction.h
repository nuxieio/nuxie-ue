#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "NuxieTypes.h"
#include "NuxieDismissAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNuxieDismissSuccessEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieDismissFailureEvent,
  const FNuxieError&,
  Error);

UCLASS()
class NUXIEBLUEPRINT_API UNuxieDismissAsyncAction
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
  static UNuxieDismissAsyncAction* DismissNuxie(
    UObject* WorldContextObject);

  virtual void Activate() override;

  UPROPERTY(BlueprintAssignable)
  FNuxieDismissSuccessEvent OnSuccess;

  UPROPERTY(BlueprintAssignable)
  FNuxieDismissFailureEvent OnFailed;

private:
  UPROPERTY()
  TObjectPtr<UObject> WorldContextObject;
};
