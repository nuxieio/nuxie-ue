#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "NuxieTypes.h"
#include "NuxieUseFeatureAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieUseFeatureSuccessEvent,
  const FNuxieFeatureUsageResult&,
  Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieUseFeatureFailureEvent,
  const FNuxieError&,
  Error);

UCLASS()
class NUXIEBLUEPRINT_API UNuxieUseFeatureAsyncAction
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
  static UNuxieUseFeatureAsyncAction* UseNuxieFeatureAndWait(
    UObject* WorldContextObject,
    const FString& FeatureId,
    double Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FNuxieScalarValue>& Metadata);

  virtual void Activate() override;

  UPROPERTY(BlueprintAssignable)
  FNuxieUseFeatureSuccessEvent OnSuccess;

  UPROPERTY(BlueprintAssignable)
  FNuxieUseFeatureFailureEvent OnFailed;

private:
  UPROPERTY()
  TObjectPtr<UObject> WorldContextObject;

  FString FeatureId;
  double Amount = 1.0;
  FString EntityId;
  bool bSetUsage = false;
  TMap<FString, FNuxieScalarValue> Metadata;
};
