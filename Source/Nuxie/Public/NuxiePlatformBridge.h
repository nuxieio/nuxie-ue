#pragma once

#include "CoreMinimal.h"

#include "NuxieTypes.h"

using FNuxieErrorCallback = TFunction<void(const FNuxieError&)>;
using FNuxieFeatureAccessSuccessCallback = TFunction<void(const FNuxieFeatureAccess&)>;
using FNuxieFeatureUsageSuccessCallback = TFunction<void(const FNuxieFeatureUsageResult&)>;

class INuxiePlatformBridgeListener
{
public:
  virtual ~INuxiePlatformBridgeListener() = default;

  virtual void OnFeatureAccessChanged(const FNuxieFeatureAccessChanged& Event) = 0;
  virtual void OnActivity(const FNuxieActivityInfo& Activity) = 0;
  virtual void OnAppAction(const FNuxieAppAction& Action) = 0;
  virtual void OnPurchaseRequest(const FNuxiePurchaseRequest& Request) = 0;
  virtual void OnRestoreRequest(const FNuxieRestoreRequest& Request) = 0;
};

class INuxiePlatformBridge
{
public:
  virtual ~INuxiePlatformBridge() = default;

  virtual void SetListener(INuxiePlatformBridgeListener* InListener) = 0;

  virtual bool Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError) = 0;
  virtual void ShutdownAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) = 0;
  virtual bool Identify(
    const FString& DistinctId,
    const TMap<FString, FNuxieScalarValue>& UserProperties,
    const TMap<FString, FNuxieScalarValue>& UserPropertiesSetOnce,
    FNuxieError& OutError) = 0;
  virtual bool Reset(bool bKeepAnonymousId, FNuxieError& OutError) = 0;
  virtual FString GetDistinctId() const = 0;
  virtual FString GetAnonymousId() const = 0;
  virtual bool IsIdentified() const = 0;

  /** Records one event. A matching Journey continues asynchronously in the native SDK. */
  virtual void Trigger(
    const FString& EventName,
    const TMap<FString, FNuxieScalarValue>& Properties) = 0;

  virtual void DismissAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) = 0;
  virtual void SetLocaleIdentifierAsync(
    const FString& LocaleIdentifier,
    FSimpleDelegate OnSuccess,
    FNuxieErrorCallback OnError) = 0;
  virtual void HasFeatureAsync(
    const FString& FeatureId,
    double RequiredBalance,
    const FString& EntityId,
    ENuxieFeatureCheckPolicy Policy,
    FNuxieFeatureAccessSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) = 0;
  virtual void UseFeature(
    const FString& FeatureId,
    double Amount,
    const FString& EntityId,
    const TMap<FString, FNuxieScalarValue>& Metadata) = 0;
  virtual void UseFeatureAndWaitAsync(
    const FString& FeatureId,
    double Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FNuxieScalarValue>& Metadata,
    FNuxieFeatureUsageSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) = 0;
  virtual bool CompletePurchase(
    const FString& RequestId,
    const FNuxiePurchaseResult& Result,
    FNuxieError& OutError) = 0;
  virtual bool CompleteRestore(
    const FString& RequestId,
    const FNuxieRestoreResult& Result,
    FNuxieError& OutError) = 0;
};

NUXIE_API TUniquePtr<INuxiePlatformBridge> CreateNuxiePlatformBridge();
