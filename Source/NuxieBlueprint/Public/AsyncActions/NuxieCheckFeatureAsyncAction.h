#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "NuxieTypes.h"
#include "NuxieCheckFeatureAsyncAction.generated.h"

class UNuxieSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieCheckFeatureSuccessEvent, const FNuxieFeatureCheckResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieCheckFeatureFailureEvent, const FNuxieError&, Error);

UCLASS()
class NUXIEBLUEPRINT_API UNuxieCheckFeatureAsyncAction : public UBlueprintAsyncActionBase
{
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"), Category = "Nuxie|Async")
  static UNuxieCheckFeatureAsyncAction* CheckNuxieFeature(
    UObject* WorldContextObject,
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    bool bForceRefresh);

  virtual void Activate() override;

  UPROPERTY(BlueprintAssignable)
  FNuxieCheckFeatureSuccessEvent OnSuccess;

  UPROPERTY(BlueprintAssignable)
  FNuxieCheckFeatureFailureEvent OnFailed;

private:
  UPROPERTY()
  TObjectPtr<UObject> WorldContextObject;

  FString FeatureId;
  int32 RequiredBalance = 1;
  FString EntityId;
  bool bForceRefresh = false;
};
