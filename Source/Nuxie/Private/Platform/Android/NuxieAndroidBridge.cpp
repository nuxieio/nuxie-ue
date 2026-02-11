#include "Platform/Android/NuxieAndroidBridge.h"

void FNuxieAndroidBridge::SetListener(INuxiePlatformBridgeListener* InListener)
{
  Listener = InListener;
}

bool FNuxieAndroidBridge::Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

bool FNuxieAndroidBridge::Shutdown(FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

bool FNuxieAndroidBridge::Identify(
  const FString& DistinctId,
  const TMap<FString, FString>& UserProperties,
  const TMap<FString, FString>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

bool FNuxieAndroidBridge::Reset(bool bKeepAnonymousId, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

FString FNuxieAndroidBridge::GetDistinctId() const
{
  return FString();
}

FString FNuxieAndroidBridge::GetAnonymousId() const
{
  return FString();
}

bool FNuxieAndroidBridge::IsIdentified() const
{
  return false;
}

bool FNuxieAndroidBridge::StartTrigger(
  const FString& RequestId,
  const FString& EventName,
  const FNuxieTriggerOptions& Options,
  FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

bool FNuxieAndroidBridge::CancelTrigger(const FString& RequestId, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

bool FNuxieAndroidBridge::ShowFlow(const FString& FlowId, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

void FNuxieAndroidBridge::RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
}

void FNuxieAndroidBridge::HasFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
}

void FNuxieAndroidBridge::CheckFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  bool bForceRefresh,
  FNuxieFeatureCheckSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
}

bool FNuxieAndroidBridge::UseFeature(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  const TMap<FString, FString>& Metadata,
  FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

void FNuxieAndroidBridge::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FString>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
}

void FNuxieAndroidBridge::FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
}

void FNuxieAndroidBridge::GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
}

void FNuxieAndroidBridge::PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
}

void FNuxieAndroidBridge::ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
}

bool FNuxieAndroidBridge::CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}

bool FNuxieAndroidBridge::CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
}
