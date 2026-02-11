#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "NuxieDelegates.h"
#include "NuxiePurchaseController.h"
#include "NuxieTypes.h"
#include "NuxieSubsystem.generated.h"

class INuxiePlatformBridge;
class INuxiePlatformBridgeListener;

using FNuxieErrorCallback = TFunction<void(const FNuxieError&)>;
using FNuxieProfileSuccessCallback = TFunction<void(const FNuxieProfileResponse&)>;
using FNuxieFeatureCheckSuccessCallback = TFunction<void(const FNuxieFeatureCheckResult&)>;
using FNuxieFeatureAccessSuccessCallback = TFunction<void(const FNuxieFeatureAccess&)>;
using FNuxieFeatureUsageSuccessCallback = TFunction<void(const FNuxieFeatureUsageResult&)>;
using FNuxieBoolSuccessCallback = TFunction<void(bool)>;
using FNuxieIntSuccessCallback = TFunction<void(int32)>;

UCLASS()
class NUXIE_API UNuxieSubsystem : public UGameInstanceSubsystem
{
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase& Collection) override;
  virtual void Deinitialize() override;

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool Shutdown(FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool Identify(
    const FString& DistinctId,
    const TMap<FString, FString>& UserProperties,
    const TMap<FString, FString>& UserPropertiesSetOnce,
    FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool Reset(bool bKeepAnonymousId, FNuxieError& OutError);

  UFUNCTION(BlueprintPure, Category = "Nuxie")
  FString GetDistinctId() const;

  UFUNCTION(BlueprintPure, Category = "Nuxie")
  FString GetAnonymousId() const;

  UFUNCTION(BlueprintPure, Category = "Nuxie")
  bool IsIdentified() const;

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool StartTrigger(
    const FString& EventName,
    const FNuxieTriggerOptions& Options,
    FString& OutRequestId,
    FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool CancelTrigger(const FString& RequestId, FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool ShowFlow(const FString& FlowId, FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool UseFeature(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    const TMap<FString, FString>& Metadata,
    FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  bool CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError);

  UFUNCTION(BlueprintCallable, Category = "Nuxie")
  void SetPurchaseController(const TScriptInterface<INuxiePurchaseController>& Controller);

  UFUNCTION(BlueprintPure, Category = "Nuxie")
  bool GetIsConfigured() const;

  void RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError);
  void HasFeatureAsync(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    FNuxieFeatureAccessSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError);
  void CheckFeatureAsync(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    bool bForceRefresh,
    FNuxieFeatureCheckSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError);
  void UseFeatureAndWaitAsync(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FString>& Metadata,
    FNuxieFeatureUsageSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError);
  void FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError);
  void GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError);
  void PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError);
  void ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError);

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieTriggerUpdateEvent OnTriggerUpdate;

  FNuxieTriggerUpdateNativeEvent OnTriggerUpdateNative;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieFeatureAccessChangedEvent OnFeatureAccessChanged;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxiePurchaseRequestEvent OnPurchaseRequest;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieRestoreRequestEvent OnRestoreRequest;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieFlowPresentedEvent OnFlowPresented;

  UPROPERTY(BlueprintAssignable, Category = "Nuxie|Events")
  FNuxieFlowDismissedEvent OnFlowDismissed;

private:
  friend class FNuxieBridgeListener;

  bool EnsureBridge(FNuxieError& OutError) const;

  TUniquePtr<INuxiePlatformBridge> Bridge;
  bool bIsConfigured = false;
  TScriptInterface<INuxiePurchaseController> PurchaseController;

  class FNuxieBridgeListener* BridgeListener = nullptr;
};
