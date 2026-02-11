#include "Platform/IOS/NuxieIOSBridge.h"

#include "Platform/NuxieNoopBridge.h"

void FNuxieIOSBridge::SetListener(INuxiePlatformBridgeListener* InListener)
{
  Listener = InListener;
}

bool FNuxieIOSBridge::Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

bool FNuxieIOSBridge::Shutdown(FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

bool FNuxieIOSBridge::Identify(
  const FString& DistinctId,
  const TMap<FString, FString>& UserProperties,
  const TMap<FString, FString>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

bool FNuxieIOSBridge::Reset(bool bKeepAnonymousId, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

FString FNuxieIOSBridge::GetDistinctId() const
{
  return FString();
}

FString FNuxieIOSBridge::GetAnonymousId() const
{
  return FString();
}

bool FNuxieIOSBridge::IsIdentified() const
{
  return false;
}

bool FNuxieIOSBridge::StartTrigger(
  const FString& RequestId,
  const FString& EventName,
  const FNuxieTriggerOptions& Options,
  FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

bool FNuxieIOSBridge::CancelTrigger(const FString& RequestId, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

bool FNuxieIOSBridge::ShowFlow(const FString& FlowId, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

void FNuxieIOSBridge::RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build.")));
}

void FNuxieIOSBridge::HasFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build.")));
}

void FNuxieIOSBridge::CheckFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  bool bForceRefresh,
  FNuxieFeatureCheckSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build.")));
}

bool FNuxieIOSBridge::UseFeature(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  const TMap<FString, FString>& Metadata,
  FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

void FNuxieIOSBridge::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FString>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build.")));
}

void FNuxieIOSBridge::FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build.")));
}

void FNuxieIOSBridge::GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build.")));
}

void FNuxieIOSBridge::PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build.")));
}

void FNuxieIOSBridge::ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build.")));
}

bool FNuxieIOSBridge::CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}

bool FNuxieIOSBridge::CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge is not linked in this build."));
  return false;
}
