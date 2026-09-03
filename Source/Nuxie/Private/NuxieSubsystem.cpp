#include "NuxieSubsystem.h"

#include "Async/Async.h"
#include "NuxiePlatformBridge.h"

class FNuxieBridgeListener final : public INuxiePlatformBridgeListener
{
public:
  explicit FNuxieBridgeListener(UNuxieSubsystem* InOwner)
    : Owner(InOwner)
  {
  }

  virtual void OnFeatureAccessChanged(const FNuxieFeatureAccessChanged& Event) override
  {
    Dispatch([Event](UNuxieSubsystem& Target)
    {
      Target.OnFeatureAccessChanged.Broadcast(Event);
    });
  }

  virtual void OnActivity(const FNuxieActivityInfo& Activity) override
  {
    Dispatch([Activity](UNuxieSubsystem& Target)
    {
      Target.OnActivity.Broadcast(Activity);
    });
  }

  virtual void OnAppAction(const FNuxieAppAction& Action) override
  {
    Dispatch([Action](UNuxieSubsystem& Target)
    {
      Target.OnAppAction.Broadcast(Action);
    });
  }

  virtual void OnPurchaseRequest(const FNuxiePurchaseRequest& Request) override
  {
    Dispatch([Request](UNuxieSubsystem& Target)
    {
      Target.OnPurchaseRequest.Broadcast(Request);
      if (Target.PurchaseController.GetObject() == nullptr)
      {
        return;
      }

      const FNuxiePurchaseResult Result =
        INuxiePurchaseController::Execute_OnPurchaseRequested(
          Target.PurchaseController.GetObject(),
          Request);
      FNuxieError IgnoreError;
      Target.CompletePurchase(Request.RequestId, Result, IgnoreError);
    });
  }

  virtual void OnRestoreRequest(const FNuxieRestoreRequest& Request) override
  {
    Dispatch([Request](UNuxieSubsystem& Target)
    {
      Target.OnRestoreRequest.Broadcast(Request);
      if (Target.PurchaseController.GetObject() == nullptr)
      {
        return;
      }

      const FNuxieRestoreResult Result =
        INuxiePurchaseController::Execute_OnRestoreRequested(
          Target.PurchaseController.GetObject(),
          Request);
      FNuxieError IgnoreError;
      Target.CompleteRestore(Request.RequestId, Result, IgnoreError);
    });
  }

private:
  void Dispatch(TFunction<void(UNuxieSubsystem&)> Work)
  {
    if (!Owner.IsValid())
    {
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [Owner = Owner, Work = MoveTemp(Work)]() mutable
    {
      if (Owner.IsValid())
      {
        Work(*Owner.Get());
      }
    });
  }

  TWeakObjectPtr<UNuxieSubsystem> Owner;
};

UNuxieSubsystem::~UNuxieSubsystem() = default;

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
    Bridge->SetListener(nullptr);
    if (bIsConfigured)
    {
      Bridge->ShutdownAsync(
        FSimpleDelegate(),
        [](const FNuxieError& Error)
        {
          UE_LOG(
            LogTemp,
            Warning,
            TEXT("Nuxie shutdown failed during subsystem teardown: %s"),
            *Error.Message);
        });
    }
    Bridge.Reset();
  }

  delete BridgeListener;
  BridgeListener = nullptr;
  bIsConfigured = false;
  PurchaseController = nullptr;
  Super::Deinitialize();
}

bool UNuxieSubsystem::EnsureBridge(FNuxieError& OutError) const
{
  if (Bridge != nullptr)
  {
    return true;
  }

  OutError = FNuxieError::Make(
    TEXT("NATIVE_UNAVAILABLE"),
    TEXT("Nuxie platform bridge is unavailable."));
  return false;
}

bool UNuxieSubsystem::Configure(
  const FNuxieConfigureOptions& Options,
  FNuxieError& OutError)
{
  if (!EnsureBridge(OutError))
  {
    return false;
  }

  bIsConfigured = Bridge->Configure(Options, OutError);
  return bIsConfigured;
}

void UNuxieSubsystem::ShutdownAsync(
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  bIsConfigured = false;
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(
      TEXT("NATIVE_UNAVAILABLE"),
      TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }

  Bridge->ShutdownAsync(MoveTemp(OnSuccess), MoveTemp(OnError));
}

bool UNuxieSubsystem::Identify(
  const FString& DistinctId,
  const TMap<FString, FNuxieScalarValue>& UserProperties,
  const TMap<FString, FNuxieScalarValue>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
  return EnsureBridge(OutError)
    && Bridge->Identify(
      DistinctId,
      UserProperties,
      UserPropertiesSetOnce,
      OutError);
}

bool UNuxieSubsystem::Reset(bool bKeepAnonymousId, FNuxieError& OutError)
{
  return EnsureBridge(OutError)
    && Bridge->Reset(bKeepAnonymousId, OutError);
}

FString UNuxieSubsystem::GetDistinctId() const
{
  return Bridge != nullptr ? Bridge->GetDistinctId() : FString();
}

FString UNuxieSubsystem::GetAnonymousId() const
{
  return Bridge != nullptr ? Bridge->GetAnonymousId() : FString();
}

bool UNuxieSubsystem::IsIdentified() const
{
  return Bridge != nullptr && Bridge->IsIdentified();
}

void UNuxieSubsystem::Trigger(
  const FString& EventName,
  const TMap<FString, FNuxieScalarValue>& Properties)
{
  if (Bridge != nullptr)
  {
    Bridge->Trigger(EventName, Properties);
  }
}

void UNuxieSubsystem::UseFeature(
  const FString& FeatureId,
  double Amount,
  const FString& EntityId,
  const TMap<FString, FNuxieScalarValue>& Metadata)
{
  if (Bridge != nullptr)
  {
    Bridge->UseFeature(FeatureId, Amount, EntityId, Metadata);
  }
}

bool UNuxieSubsystem::CompletePurchase(
  const FString& RequestId,
  const FNuxiePurchaseResult& Result,
  FNuxieError& OutError)
{
  return EnsureBridge(OutError)
    && Bridge->CompletePurchase(RequestId, Result, OutError);
}

bool UNuxieSubsystem::CompleteRestore(
  const FString& RequestId,
  const FNuxieRestoreResult& Result,
  FNuxieError& OutError)
{
  return EnsureBridge(OutError)
    && Bridge->CompleteRestore(RequestId, Result, OutError);
}

void UNuxieSubsystem::SetPurchaseController(
  const TScriptInterface<INuxiePurchaseController>& Controller)
{
  PurchaseController = Controller;
}

bool UNuxieSubsystem::GetIsConfigured() const
{
  return bIsConfigured;
}

void UNuxieSubsystem::DismissAsync(
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(
      TEXT("NATIVE_UNAVAILABLE"),
      TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }
  Bridge->DismissAsync(MoveTemp(OnSuccess), MoveTemp(OnError));
}

void UNuxieSubsystem::SetLocaleIdentifierAsync(
  const FString& LocaleIdentifier,
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(
      TEXT("NATIVE_UNAVAILABLE"),
      TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }
  Bridge->SetLocaleIdentifierAsync(
    LocaleIdentifier,
    MoveTemp(OnSuccess),
    MoveTemp(OnError));
}

void UNuxieSubsystem::HasFeatureAsync(
  const FString& FeatureId,
  double RequiredBalance,
  const FString& EntityId,
  ENuxieFeatureCheckPolicy Policy,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(
      TEXT("NATIVE_UNAVAILABLE"),
      TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }
  Bridge->HasFeatureAsync(
    FeatureId,
    RequiredBalance,
    EntityId,
    Policy,
    MoveTemp(OnSuccess),
    MoveTemp(OnError));
}

void UNuxieSubsystem::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  double Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FNuxieScalarValue>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  if (Bridge == nullptr)
  {
    OnError(FNuxieError::Make(
      TEXT("NATIVE_UNAVAILABLE"),
      TEXT("Nuxie platform bridge is unavailable.")));
    return;
  }
  Bridge->UseFeatureAndWaitAsync(
    FeatureId,
    Amount,
    EntityId,
    bSetUsage,
    Metadata,
    MoveTemp(OnSuccess),
    MoveTemp(OnError));
}
