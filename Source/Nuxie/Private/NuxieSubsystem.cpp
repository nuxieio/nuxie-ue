#include "NuxieSubsystem.h"

#include "Async/Async.h"
#include "Engine/Engine.h"
#include "Misc/Guid.h"
#include "Platform/NuxiePlatformBridge.h"

class FNuxieBridgeListener final : public INuxiePlatformBridgeListener
{
public:
  explicit FNuxieBridgeListener(UNuxieSubsystem* InOwner)
    : Owner(InOwner)
  {
  }

  virtual void OnTriggerUpdate(const FString& RequestId, const FNuxieTriggerUpdate& Update) override
  {
    if (!Owner.IsValid())
    {
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [Owner = Owner, RequestId, Update]()
    {
      if (!Owner.IsValid())
      {
        return;
      }
      Owner->OnTriggerUpdateNative.Broadcast(RequestId, Update);
      Owner->OnTriggerUpdate.Broadcast(RequestId, Update);
    });
  }

  virtual void OnFeatureAccessChanged(const FString& FeatureId, const FNuxieFeatureAccess& Previous, const FNuxieFeatureAccess& Current) override
  {
    if (!Owner.IsValid())
    {
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [Owner = Owner, FeatureId, Previous, Current]()
    {
      if (!Owner.IsValid())
      {
        return;
      }
      Owner->OnFeatureAccessChanged.Broadcast(FeatureId, Previous, Current);
    });
  }

  virtual void OnPurchaseRequest(const FNuxiePurchaseRequest& Request) override
  {
    if (!Owner.IsValid())
    {
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [Owner = Owner, Request]()
    {
      if (!Owner.IsValid())
      {
        return;
      }

      Owner->OnPurchaseRequest.Broadcast(Request);

      if (Owner->PurchaseController.GetObject() != nullptr)
      {
        FNuxiePurchaseResult Result = INuxiePurchaseController::Execute_OnPurchaseRequested(Owner->PurchaseController.GetObject(), Request);
        FNuxieError CompletionError;
        Owner->CompletePurchase(Request.RequestId, Result, CompletionError);
      }
    });
  }

  virtual void OnRestoreRequest(const FNuxieRestoreRequest& Request) override
  {
    if (!Owner.IsValid())
    {
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [Owner = Owner, Request]()
    {
      if (!Owner.IsValid())
      {
        return;
      }

      Owner->OnRestoreRequest.Broadcast(Request);

      if (Owner->PurchaseController.GetObject() != nullptr)
      {
        FNuxieRestoreResult Result = INuxiePurchaseController::Execute_OnRestoreRequested(Owner->PurchaseController.GetObject(), Request);
        FNuxieError CompletionError;
        Owner->CompleteRestore(Request.RequestId, Result, CompletionError);
      }
    });
  }

  virtual void OnFlowPresented(const FString& FlowId) override
  {
    if (!Owner.IsValid())
    {
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [Owner = Owner, FlowId]()
    {
      if (!Owner.IsValid())
      {
        return;
      }
      Owner->OnFlowPresented.Broadcast(FlowId);
    });
  }

  virtual void OnFlowDismissed(const FString& FlowId) override
  {
    if (!Owner.IsValid())
    {
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [Owner = Owner, FlowId]()
    {
      if (!Owner.IsValid())
      {
        return;
      }
      Owner->OnFlowDismissed.Broadcast(FlowId);
    });
  }

private:
  TWeakObjectPtr<UNuxieSubsystem> Owner;
};

void UNuxieSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
  Super::Initialize(Collection);

  Bridge = CreateNuxiePlatformBridge();
  BridgeListener = new FNuxieBridgeListener(this);
  Bridge->SetListener(BridgeListener);
}

void UNuxieSubsystem::Deinitialize()
{
  if (Bridge != nullptr)
  {
    FNuxieError IgnoreError;
    Bridge->Shutdown(IgnoreError);
    Bridge->SetListener(nullptr);
    Bridge.Reset();
  }

  if (BridgeListener != nullptr)
  {
    delete BridgeListener;
    BridgeListener = nullptr;
  }

  bIsConfigured = false;
  PurchaseController = nullptr;

  Super::Deinitialize();
}

bool UNuxieSubsystem::EnsureBridge(FNuxieError& OutError) const
{
  if (Bridge == nullptr)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable."));
    return false;
  }
  return true;
}

bool UNuxieSubsystem::Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  const bool bSuccess = Bridge->Configure(Options, OutError);
  bIsConfigured = bSuccess;
  return bSuccess;
}

bool UNuxieSubsystem::Shutdown(FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  const bool bSuccess = Bridge->Shutdown(OutError);
  bIsConfigured = false;
  return bSuccess;
}

bool UNuxieSubsystem::Identify(
  const FString& DistinctId,
  const TMap<FString, FString>& UserProperties,
  const TMap<FString, FString>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  return Bridge->Identify(DistinctId, UserProperties, UserPropertiesSetOnce, OutError);
}

bool UNuxieSubsystem::Reset(bool bKeepAnonymousId, FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  return Bridge->Reset(bKeepAnonymousId, OutError);
}

FString UNuxieSubsystem::GetDistinctId() const
{
  if (Bridge == nullptr)
  {
    return FString();
  }
  return Bridge->GetDistinctId();
}

FString UNuxieSubsystem::GetAnonymousId() const
{
  if (Bridge == nullptr)
  {
    return FString();
  }
  return Bridge->GetAnonymousId();
}

bool UNuxieSubsystem::IsIdentified() const
{
  if (Bridge == nullptr)
  {
    return false;
  }
  return Bridge->IsIdentified();
}

bool UNuxieSubsystem::StartTrigger(
  const FString& EventName,
  const FNuxieTriggerOptions& Options,
  FString& OutRequestId,
  FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  OutRequestId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
  return Bridge->StartTrigger(OutRequestId, EventName, Options, OutError);
}

bool UNuxieSubsystem::CancelTrigger(const FString& RequestId, FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  return Bridge->CancelTrigger(RequestId, OutError);
}

bool UNuxieSubsystem::ShowFlow(const FString& FlowId, FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  return Bridge->ShowFlow(FlowId, OutError);
}

bool UNuxieSubsystem::UseFeature(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  const TMap<FString, FString>& Metadata,
  FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  return Bridge->UseFeature(FeatureId, Amount, EntityId, Metadata, OutError);
}

bool UNuxieSubsystem::CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  return Bridge->CompletePurchase(RequestId, Result, OutError);
}

bool UNuxieSubsystem::CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  return Bridge->CompleteRestore(RequestId, Result, OutError);
}

void UNuxieSubsystem::SetPurchaseController(const TScriptInterface<INuxiePurchaseController>& Controller)
{
  PurchaseController = Controller;
}

bool UNuxieSubsystem::GetIsConfigured() const
{
  return bIsConfigured;
}

void UNuxieSubsystem::RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->RefreshProfileAsync(MoveTemp(OnSuccess), MoveTemp(OnError));
}

void UNuxieSubsystem::HasFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->HasFeatureAsync(FeatureId, RequiredBalance, EntityId, MoveTemp(OnSuccess), MoveTemp(OnError));
}

void UNuxieSubsystem::CheckFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  bool bForceRefresh,
  FNuxieFeatureCheckSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->CheckFeatureAsync(FeatureId, RequiredBalance, EntityId, bForceRefresh, MoveTemp(OnSuccess), MoveTemp(OnError));
}

void UNuxieSubsystem::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FString>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->UseFeatureAndWaitAsync(FeatureId, Amount, EntityId, bSetUsage, Metadata, MoveTemp(OnSuccess), MoveTemp(OnError));
}

void UNuxieSubsystem::FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->FlushEventsAsync(MoveTemp(OnSuccess), MoveTemp(OnError));
}

void UNuxieSubsystem::GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->GetQueuedEventCountAsync(MoveTemp(OnSuccess), MoveTemp(OnError));
}

void UNuxieSubsystem::PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->PauseEventQueueAsync(MoveTemp(OnSuccess), MoveTemp(OnError));
}

void UNuxieSubsystem::ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->ResumeEventQueueAsync(MoveTemp(OnSuccess), MoveTemp(OnError));
}
