#pragma once

#include "Platform/NuxiePlatformBridge.h"

class FNuxieAndroidBridge final : public INuxiePlatformBridge
{
public:
  virtual void SetListener(INuxiePlatformBridgeListener* InListener) override;

  virtual bool Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError) override;
  virtual bool Shutdown(FNuxieError& OutError) override;
  virtual bool Identify(
    const FString& DistinctId,
    const TMap<FString, FString>& UserProperties,
    const TMap<FString, FString>& UserPropertiesSetOnce,
    FNuxieError& OutError) override;
  virtual bool Reset(bool bKeepAnonymousId, FNuxieError& OutError) override;
  virtual FString GetDistinctId() const override;
  virtual FString GetAnonymousId() const override;
  virtual bool IsIdentified() const override;
  virtual bool StartTrigger(
    const FString& RequestId,
    const FString& EventName,
    const FNuxieTriggerOptions& Options,
    FNuxieError& OutError) override;
  virtual bool CancelTrigger(const FString& RequestId, FNuxieError& OutError) override;
  virtual bool ShowFlow(const FString& FlowId, FNuxieError& OutError) override;
  virtual void RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void HasFeatureAsync(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    FNuxieFeatureAccessSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual void CheckFeatureAsync(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    bool bForceRefresh,
    FNuxieFeatureCheckSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual bool UseFeature(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    const TMap<FString, FString>& Metadata,
    FNuxieError& OutError) override;
  virtual void UseFeatureAndWaitAsync(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FString>& Metadata,
    FNuxieFeatureUsageSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual void FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) override;
  virtual bool CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError) override;
  virtual bool CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError) override;

private:
  INuxiePlatformBridgeListener* Listener = nullptr;
};
