#include "Platform/NuxieNoopBridge.h"

#include "Async/Async.h"
#include "Misc/Guid.h"

namespace
{
  FNuxieError UnsupportedError()
  {
    return FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie is only supported on iOS/Android targets in this build."));
  }
}

FNuxieError FNuxieNoopBridge::UnsupportedError()
{
  return ::UnsupportedError();
}

void FNuxieNoopBridge::SetListener(INuxiePlatformBridgeListener* InListener)
{
  Listener = InListener;
}

bool FNuxieNoopBridge::Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError)
{
  DistinctId = FString::Printf(TEXT("anon_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
  AnonymousId = DistinctId;
  OutError = UnsupportedError();
  return false;
}

bool FNuxieNoopBridge::Shutdown(FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

bool FNuxieNoopBridge::Identify(
  const FString& DistinctIdIn,
  const TMap<FString, FString>& UserProperties,
  const TMap<FString, FString>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
  DistinctId = DistinctIdIn;
  OutError = UnsupportedError();
  return false;
}

bool FNuxieNoopBridge::Reset(bool bKeepAnonymousId, FNuxieError& OutError)
{
  if (!bKeepAnonymousId)
  {
    AnonymousId = FString::Printf(TEXT("anon_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
  }
  DistinctId = AnonymousId;
  OutError = UnsupportedError();
  return false;
}

FString FNuxieNoopBridge::GetDistinctId() const
{
  return DistinctId;
}

FString FNuxieNoopBridge::GetAnonymousId() const
{
  return AnonymousId;
}

bool FNuxieNoopBridge::IsIdentified() const
{
  return !DistinctId.IsEmpty() && DistinctId != AnonymousId;
}

bool FNuxieNoopBridge::StartTrigger(
  const FString& RequestId,
  const FString& EventName,
  const FNuxieTriggerOptions& Options,
  FNuxieError& OutError)
{
  if (Listener != nullptr)
  {
    FNuxieTriggerUpdate Update;
    Update.Kind = ENuxieTriggerUpdateKind::Error;
    Update.Error = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("No mobile bridge is available."));
    Update.TimestampMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
    Update.bIsTerminal = true;
    AsyncTask(ENamedThreads::GameThread, [Listener = Listener, RequestId, Update]()
    {
      Listener->OnTriggerUpdate(RequestId, Update);
    });
  }

  OutError = UnsupportedError();
  return false;
}

bool FNuxieNoopBridge::CancelTrigger(const FString& RequestId, FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

bool FNuxieNoopBridge::ShowFlow(const FString& FlowId, FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

void FNuxieNoopBridge::RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError]()
  {
    OnError(UnsupportedError());
  });
}

void FNuxieNoopBridge::HasFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError]()
  {
    OnError(UnsupportedError());
  });
}

void FNuxieNoopBridge::CheckFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  bool bForceRefresh,
  FNuxieFeatureCheckSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError]()
  {
    OnError(UnsupportedError());
  });
}

bool FNuxieNoopBridge::UseFeature(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  const TMap<FString, FString>& Metadata,
  FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

void FNuxieNoopBridge::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FString>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError]()
  {
    OnError(UnsupportedError());
  });
}

void FNuxieNoopBridge::FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError]()
  {
    OnError(UnsupportedError());
  });
}

void FNuxieNoopBridge::GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError]()
  {
    OnError(UnsupportedError());
  });
}

void FNuxieNoopBridge::PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError]()
  {
    OnError(UnsupportedError());
  });
}

void FNuxieNoopBridge::ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError]()
  {
    OnError(UnsupportedError());
  });
}

bool FNuxieNoopBridge::CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}

bool FNuxieNoopBridge::CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError)
{
  OutError = UnsupportedError();
  return false;
}
