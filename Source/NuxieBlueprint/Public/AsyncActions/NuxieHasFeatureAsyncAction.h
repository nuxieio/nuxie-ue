#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "NuxieTypes.h"
#include "NuxieHasFeatureAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieHasFeatureSuccessEvent,
  const FNuxieFeatureAccess&,
  Access);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieHasFeatureFailureEvent,
  const FNuxieError&,
  Error);

UCLASS()
class NUXIEBLUEPRINT_API UNuxieHasFeatureAsyncAction
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
  static UNuxieHasFeatureAsyncAction* HasNuxieFeature(
    UObject* WorldContextObject,
    const FString& FeatureId,
    double RequiredBalance,
    const FString& EntityId,
    ENuxieFeatureCheckPolicy Policy);

  virtual void Activate() override;

  UPROPERTY(BlueprintAssignable)
  FNuxieHasFeatureSuccessEvent OnSuccess;

  UPROPERTY(BlueprintAssignable)
  FNuxieHasFeatureFailureEvent OnFailed;

private:
  UPROPERTY()
  TObjectPtr<UObject> WorldContextObject;

  FString FeatureId;
  double RequiredBalance = 1.0;
  FString EntityId;
  ENuxieFeatureCheckPolicy Policy = ENuxieFeatureCheckPolicy::CacheFirst;
};
