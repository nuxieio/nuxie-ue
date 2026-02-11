# API Reference

## Core subsystem

`UNuxieSubsystem` (`UGameInstanceSubsystem`) is the runtime entrypoint.

### Configuration and identity

- `bool Configure(const FNuxieConfigureOptions&, FNuxieError&)`
- `bool Shutdown(FNuxieError&)`
- `bool Identify(const FString&, const TMap<FString, FString>&, const TMap<FString, FString>&, FNuxieError&)`
- `bool Reset(bool bKeepAnonymousId, FNuxieError&)`
- `FString GetDistinctId() const`
- `FString GetAnonymousId() const`
- `bool IsIdentified() const`

### Trigger and flow

- `bool StartTrigger(const FString& EventName, const FNuxieTriggerOptions&, FString& OutRequestId, FNuxieError&)`
- `bool CancelTrigger(const FString& RequestId, FNuxieError&)`
- `bool ShowFlow(const FString& FlowId, FNuxieError&)`

### Features and usage

- `bool UseFeature(const FString& FeatureId, float Amount, const FString& EntityId, const TMap<FString, FString>& Metadata, FNuxieError&)`
- `void HasFeatureAsync(...)`
- `void CheckFeatureAsync(...)`
- `void UseFeatureAndWaitAsync(...)`

### Profile and queue

- `void RefreshProfileAsync(...)`
- `void FlushEventsAsync(...)`
- `void GetQueuedEventCountAsync(...)`
- `void PauseEventQueueAsync(...)`
- `void ResumeEventQueueAsync(...)`

### Purchase completion

- `bool CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult&, FNuxieError&)`
- `bool CompleteRestore(const FString& RequestId, const FNuxieRestoreResult&, FNuxieError&)`

## Delegates/events

Blueprint-assignable events:

- `OnTriggerUpdate`
- `OnFeatureAccessChanged`
- `OnPurchaseRequest`
- `OnRestoreRequest`
- `OnFlowPresented`
- `OnFlowDismissed`

Native multicast event:

- `OnTriggerUpdateNative`

## Blueprint async actions

- `UNuxieTriggerAsyncAction::StartNuxieTrigger(...)`
- `UNuxieCheckFeatureAsyncAction::CheckNuxieFeature(...)`

## Core types

See `Source/Nuxie/Public/NuxieTypes.h` for full structs/enums.
