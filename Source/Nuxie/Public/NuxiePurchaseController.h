#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "NuxieTypes.h"
#include "NuxiePurchaseController.generated.h"

UINTERFACE(BlueprintType)
class NUXIE_API UNuxiePurchaseController : public UInterface
{
  GENERATED_BODY()
};

class NUXIE_API INuxiePurchaseController
{
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Nuxie|Purchases")
  FNuxiePurchaseResult OnPurchaseRequested(const FNuxiePurchaseRequest& Request);

  UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Nuxie|Purchases")
  FNuxieRestoreResult OnRestoreRequested(const FNuxieRestoreRequest& Request);
};
