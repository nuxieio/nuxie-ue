#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "NuxieTypes.h"
#include "NuxieSetLocaleAsyncAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNuxieSetLocaleSuccessEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieSetLocaleFailureEvent,
  const FNuxieError&,
  Error);

UCLASS()
class NUXIEBLUEPRINT_API UNuxieSetLocaleAsyncAction
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
  static UNuxieSetLocaleAsyncAction* SetNuxieLocale(
    UObject* WorldContextObject,
    const FString& LocaleIdentifier);

  virtual void Activate() override;

  UPROPERTY(BlueprintAssignable)
  FNuxieSetLocaleSuccessEvent OnSuccess;

  UPROPERTY(BlueprintAssignable)
  FNuxieSetLocaleFailureEvent OnFailed;

private:
  UPROPERTY()
  TObjectPtr<UObject> WorldContextObject;

  FString LocaleIdentifier;
};
