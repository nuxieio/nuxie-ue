#include "AsyncActions/NuxieSetLocaleAsyncAction.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "NuxieSubsystem.h"

UNuxieSetLocaleAsyncAction*
UNuxieSetLocaleAsyncAction::SetNuxieLocale(
  UObject* WorldContextObjectIn,
  const FString& LocaleIdentifierIn)
{
  UNuxieSetLocaleAsyncAction* Action =
    NewObject<UNuxieSetLocaleAsyncAction>();
  Action->WorldContextObject = WorldContextObjectIn;
  Action->LocaleIdentifier = LocaleIdentifierIn;
  Action->RegisterWithGameInstance(WorldContextObjectIn);
  return Action;
}

void UNuxieSetLocaleAsyncAction::Activate()
{
  UWorld* World = WorldContextObject != nullptr
    ? GEngine->GetWorldFromContextObject(
      WorldContextObject,
      EGetWorldErrorMode::ReturnNull)
    : nullptr;
  UNuxieSubsystem* Subsystem =
    World != nullptr && World->GetGameInstance() != nullptr
      ? World->GetGameInstance()->GetSubsystem<UNuxieSubsystem>()
      : nullptr;
  if (Subsystem == nullptr)
  {
    OnFailed.Broadcast(FNuxieError::Make(
      TEXT("NO_SUBSYSTEM"),
      TEXT("Nuxie subsystem is unavailable.")));
    SetReadyToDestroy();
    return;
  }

  TWeakObjectPtr<UNuxieSetLocaleAsyncAction> WeakThis(this);
  Subsystem->SetLocaleIdentifierAsync(
    LocaleIdentifier,
    FSimpleDelegate::CreateLambda([WeakThis]()
    {
      if (WeakThis.IsValid())
      {
        WeakThis->OnSuccess.Broadcast();
        WeakThis->SetReadyToDestroy();
      }
    }),
    [WeakThis](const FNuxieError& Error)
    {
      if (WeakThis.IsValid())
      {
        WeakThis->OnFailed.Broadcast(Error);
        WeakThis->SetReadyToDestroy();
      }
    });
}
