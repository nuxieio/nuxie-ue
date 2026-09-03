# API reference

## Configuration and identity

- bool Configure(const FNuxieConfigureOptions&, FNuxieError&)
- void ShutdownAsync(success callback, error callback)
- bool Identify(const FString&, scalar user properties, scalar set-once properties, FNuxieError&)
- bool Reset(bool bKeepAnonymousId, FNuxieError&)
- FString GetDistinctId()
- FString GetAnonymousId()
- bool IsIdentified()

FNuxieConfigureOptions contains the public API key, environment, log level,
iOS console and redaction controls, locale, purchase handling mode, iOS Test
Store switch, and purchase-controller switch.

## Journeys

- void Trigger(const FString& EventName, scalar properties)
- void DismissAsync(...)
- void SetLocaleIdentifierAsync(...)

Trigger records an event and returns immediately. It has no result, handle,
cancellation, or identity mutation.

## Features

- void HasFeatureAsync(feature, double required balance, entity, policy, callbacks)
- void UseFeature(feature, double amount, entity, scalar metadata)
- void UseFeatureAndWaitAsync(feature, double amount, entity, set usage, metadata, callbacks)

FNuxieFeatureUsageResult contains optional usage details and optional
AuthoritativeAccess.

## Events

- OnFeatureAccessChanged
- OnActivity
- OnAppAction
- OnPurchaseRequest
- OnRestoreRequest

## Commerce

- void SetPurchaseController(...)
- bool CompletePurchase(...)
- bool CompleteRestore(...)

Purchase result values are Purchased, Cancelled, Pending, and Failed. Restore
values are Restored, NoPurchases, and Failed.

## Blueprint async actions

- UNuxieShutdownAsyncAction::ShutdownNuxie
- UNuxieHasFeatureAsyncAction::HasNuxieFeature
- UNuxieUseFeatureAsyncAction::UseNuxieFeatureAndWait
- UNuxieDismissAsyncAction::DismissNuxie
- UNuxieSetLocaleAsyncAction::SetNuxieLocale
