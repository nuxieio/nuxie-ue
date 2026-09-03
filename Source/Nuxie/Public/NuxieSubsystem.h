#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "NuxiePlatformBridge.h"
#include "NuxiePurchaseController.h"
#include "NuxieTypes.h"
#include "NuxieSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieFeatureAccessChangedEvent,
  const FNuxieFeatureAccessChanged&,
  Event);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieActivityEvent,
  const FNuxieActivityInfo&,
  Activity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieAppActionEvent,
  const FNuxieAppAction&,
  Action);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxiePurchaseRequestEvent,
  const FNuxiePurchaseRequest&,
  Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
  FNuxieRestoreRequestEvent,
  const FNuxieRestoreRequest&,
  Request);

UCLASS()
class NUXIE_API UNuxieSubsystem : public UGameInstanceSubsystem
{
  GENERATED_BODY()

public:
  virtual ~UNuxieSubsystem() override;
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool Identify(
    const FString& DistinctId,
    const TMap<FString, FNuxieScalarValue>& UserProperties,
    const TMap<FString, FNuxieScalarValue>& UserPropertiesSetOnce,
    FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool Reset(bool bKeepAnonymousId, FNuxieError& OutError);

  UFUNCTION(BlueprintPure, Category = "Nuxie")
  FString GetDistinctId() const;

  UFUNCTION(BlueprintPure, Category = "Nuxie")
  FString GetAnonymousId() const;

  UFUNCTION(BlueprintPure, Category = "Nuxie")
  bool IsIdentified() const;

  /** Records one event and returns immediately. Any matching Journey runs in the native SDK. */
  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  void Trigger(
    const FString& EventName,
    const TMap<FString, FNuxieScalarValue>& Properties);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  void UseFeature(
    const FString& FeatureId,
    double Amount,
    const FString& EntityId,
    const TMap<FString, FNuxieScalarValue>& Metadata);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool CompletePurchase(
    const FString& RequestId,
    const FNuxiePurchaseResult& Result,
    FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool CompleteRestore(
    const FString& RequestId,
    const FNuxieRestoreResult& Result,
    FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  void SetPurchaseController(const TScriptInterface<INuxiePurchaseController>& Controller);

  UFUNCTION(BlueprintPure, Category = "Nuxie")
  bool GetIsConfigured() const;

  void ShutdownAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError);
  void DismissAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError);
  void SetLocaleIdentifierAsync(
    const FString& LocaleIdentifier,
    FSimpleDelegate OnSuccess,
    FNuxieErrorCallback OnError);
  void HasFeatureAsync(
    const FString& FeatureId,
    double RequiredBalance,
    const FString& EntityId,
    ENuxieFeatureCheckPolicy Policy,
    FNuxieFeatureAccessSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError);
  void UseFeatureAndWaitAsync(
    const FString& FeatureId,
    double Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FNuxieScalarValue>& Metadata,
    FNuxieFeatureUsageSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError);

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieFeatureAccessChangedEvent OnFeatureAccessChanged;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieActivityEvent OnActivity;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieAppActionEvent OnAppAction;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxiePurchaseRequestEvent OnPurchaseRequest;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieRestoreRequestEvent OnRestoreRequest;

private:
  friend class FNuxieBridgeListener;

  bool EnsureBridge(FNuxieError& OutError) const;

  TUniquePtr<INuxiePlatformBridge> Bridge;
  bool bIsConfigured = false;
  TScriptInterface<INuxiePurchaseController> PurchaseController;
  class FNuxieBridgeListener* BridgeListener = nullptr;
};
