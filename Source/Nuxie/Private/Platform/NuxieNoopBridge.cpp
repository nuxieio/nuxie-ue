#include "Platform/NuxieNoopBridge.h"

#include "Async/Async.h"

namespace
{
  void FailAsync(FNuxieErrorCallback OnError)
  {
    AsyncTask(ENamedThreads::GameThread, [
      OnError = MoveTemp(OnError)]() mutable
    {
      OnError(FNuxieError::Make(
        TEXT("NATIVE_UNAVAILABLE"),
        TEXT("Nuxie is available only on iOS and Android.")));
    });
  }
}

FNuxieError FNuxieNoopBridge::UnsupportedError()
{
  return FNuxieError::Make(
    TEXT("NATIVE_UNAVAILABLE"),
    TEXT("Nuxie is available only on iOS and Android."));
}

void FNuxieNoopBridge::SetListener(
  INuxiePlatformBridgeListener* InListener)
{
}

bool FNuxieNoopBridge::Configure(
  const FNuxieConfigureOptions& Options,
  FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

void FNuxieNoopBridge::ShutdownAsync(
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  FailAsync(MoveTemp(OnError));
}

bool FNuxieNoopBridge::Identify(
  const FString& DistinctId,
  const TMap<FString, FNuxieScalarValue>& UserProperties,
  const TMap<FString, FNuxieScalarValue>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

bool FNuxieNoopBridge::Reset(
  bool bKeepAnonymousId,
  FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

FString FNuxieNoopBridge::GetDistinctId() const
{
  return FString();
}

FString FNuxieNoopBridge::GetAnonymousId() const
{
  return FString();
}

bool FNuxieNoopBridge::IsIdentified() const
{
  return false;
}

void FNuxieNoopBridge::Trigger(
  const FString& EventName,
  const TMap<FString, FNuxieScalarValue>& Properties)
{
}

void FNuxieNoopBridge::DismissAsync(
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  FailAsync(MoveTemp(OnError));
}

void FNuxieNoopBridge::SetLocaleIdentifierAsync(
  const FString& LocaleIdentifier,
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  FailAsync(MoveTemp(OnError));
}

void FNuxieNoopBridge::HasFeatureAsync(
  const FString& FeatureId,
  double RequiredBalance,
  const FString& EntityId,
  ENuxieFeatureCheckPolicy Policy,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  FailAsync(MoveTemp(OnError));
}

void FNuxieNoopBridge::UseFeature(
  const FString& FeatureId,
  double Amount,
  const FString& EntityId,
  const TMap<FString, FNuxieScalarValue>& Metadata)
{
}

void FNuxieNoopBridge::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  double Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FNuxieScalarValue>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  FailAsync(MoveTemp(OnError));
}

bool FNuxieNoopBridge::CompletePurchase(
  const FString& RequestId,
  const FNuxiePurchaseResult& Result,
  FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

bool FNuxieNoopBridge::CompleteRestore(
  const FString& RequestId,
  const FNuxieRestoreResult& Result,
  FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}
