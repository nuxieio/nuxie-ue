#pragma once

#include "CoreMinimal.h"

#include "NuxieTypes.h"

using FNuxieErrorCallback = TFunction<void(const FNuxieError&)>;
using FNuxieProfileSuccessCallback = TFunction<void(const FNuxieProfileResponse&)>;
using FNuxieFeatureCheckSuccessCallback = TFunction<void(const FNuxieFeatureCheckResult&)>;
using FNuxieFeatureAccessSuccessCallback = TFunction<void(const FNuxieFeatureAccess&)>;
using FNuxieFeatureUsageSuccessCallback = TFunction<void(const FNuxieFeatureUsageResult&)>;
using FNuxieBoolSuccessCallback = TFunction<void(bool)>;
using FNuxieIntSuccessCallback = TFunction<void(int32)>;

class INuxiePlatformBridgeListener
{
public:
  virtual ~INuxiePlatformBridgeListener() = default;

  virtual void OnTriggerUpdate(const FString& RequestId, const FNuxieTriggerUpdate& Update) = 0;
  virtual void OnFeatureAccessChanged(const FString& FeatureId, const FNuxieFeatureAccess& Previous, const FNuxieFeatureAccess& Current) = 0;
  virtual void OnPurchaseRequest(const FNuxiePurchaseRequest& Request) = 0;
  virtual void OnRestoreRequest(const FNuxieRestoreRequest& Request) = 0;
  virtual void OnFlowPresented(const FString& FlowId) = 0;
  virtual void OnFlowDismissed(const FString& FlowId) = 0;
};

class INuxiePlatformBridge
{
public:
  virtual ~INuxiePlatformBridge() = default;

  virtual void SetListener(INuxiePlatformBridgeListener* InListener) = 0;

  virtual bool Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError) = 0;
  virtual bool Shutdown(FNuxieError& OutError) = 0;

  virtual bool Identify(
    const FString& DistinctId,
    const TMap<FString, FString>& UserProperties,
    const TMap<FString, FString>& UserPropertiesSetOnce,
    FNuxieError& OutError) = 0;

  virtual bool Reset(bool bKeepAnonymousId, FNuxieError& OutError) = 0;

  virtual FString GetDistinctId() const = 0;
  virtual FString GetAnonymousId() const = 0;
  virtual bool IsIdentified() const = 0;

  virtual bool StartTrigger(
    const FString& RequestId,
    const FString& EventName,
    const FNuxieTriggerOptions& Options,
    FNuxieError& OutError) = 0;

  virtual bool CancelTrigger(const FString& RequestId, FNuxieError& OutError) = 0;

  virtual bool ShowFlow(const FString& FlowId, FNuxieError& OutError) = 0;

  virtual void RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError) = 0;

  virtual void HasFeatureAsync(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    FNuxieFeatureAccessSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) = 0;

  virtual void CheckFeatureAsync(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    bool bForceRefresh,
    FNuxieFeatureCheckSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) = 0;

  virtual bool UseFeature(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    const TMap<FString, FString>& Metadata,
    FNuxieError& OutError) = 0;

  virtual void UseFeatureAndWaitAsync(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FString>& Metadata,
    FNuxieFeatureUsageSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) = 0;

  virtual void FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError) = 0;
  virtual void GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError) = 0;
  virtual void PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) = 0;
  virtual void ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) = 0;

  virtual bool CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError) = 0;
  virtual bool CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError) = 0;
};

TUniquePtr<INuxiePlatformBridge> CreateNuxiePlatformBridge();
