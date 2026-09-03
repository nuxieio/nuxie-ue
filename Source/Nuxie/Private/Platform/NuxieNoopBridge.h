#pragma once

#include "NuxiePlatformBridge.h"

class FNuxieNoopBridge final : public INuxiePlatformBridge
{
public:
  virtual void SetListener(INuxiePlatformBridgeListener* InListener) override;
  virtual bool Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError) override;
  virtual void ShutdownAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) override;
  virtual bool Identify(
    const FString& DistinctId,
    const TMap<FString, FNuxieScalarValue>& UserProperties,
    const TMap<FString, FNuxieScalarValue>& UserPropertiesSetOnce,
    FNuxieError& OutError) override;
  virtual bool Reset(bool bKeepAnonymousId, FNuxieError& OutError) override;
  virtual FString GetDistinctId() const override;
  virtual FString GetAnonymousId() const override;
  virtual bool IsIdentified() const override;
  virtual void Trigger(
    const FString& EventName,
    const TMap<FString, FNuxieScalarValue>& Properties) override;
  virtual void DismissAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void SetLocaleIdentifierAsync(
    const FString& LocaleIdentifier,
    FSimpleDelegate OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual void HasFeatureAsync(
    const FString& FeatureId,
    double RequiredBalance,
    const FString& EntityId,
    ENuxieFeatureCheckPolicy Policy,
    FNuxieFeatureAccessSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual void UseFeature(
    const FString& FeatureId,
    double Amount,
    const FString& EntityId,
    const TMap<FString, FNuxieScalarValue>& Metadata) override;
  virtual void UseFeatureAndWaitAsync(
    const FString& FeatureId,
    double Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FNuxieScalarValue>& Metadata,
    FNuxieFeatureUsageSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual bool CompletePurchase(
    const FString& RequestId,
    const FNuxiePurchaseResult& Result,
    FNuxieError& OutError) override;
  virtual bool CompleteRestore(
    const FString& RequestId,
    const FNuxieRestoreResult& Result,
    FNuxieError& OutError) override;

private:
  static FNuxieError UnsupportedError();
};
