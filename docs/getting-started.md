# Getting started

## 1. Add the plugin

Place this repository at Plugins/Nuxie in the game project. Enable Nuxie and
NuxieBlueprint, then regenerate project files.

Android packaging requires Maven Central access for the exact
ai.nuxie:nuxie-android:0.1.0 dependency. iOS packaging uses the prepared
embedded framework in ThirdParty/IOS/lib.

## 2. Configure once

Resolve UNuxieSubsystem from the game instance and call Configure. Use
production or development, choose a log level, and optionally provide a locale
and purchase controller.

Identity calls are cache-first:

    FNuxieError Error;
    Nuxie->Identify(TEXT("player_123"), {}, {}, Error);
    Nuxie->Reset(false, Error);

Profile synchronization happens at native lifecycle sync points.

## 3. Record events

    Nuxie->Trigger(TEXT("level_completed"), {});

Trigger returns no result and exposes no operation handle. A matching Journey
continues in the native SDK, including experiment selection and presentation.

## 4. Receive runtime output

Bind to:

- OnFeatureAccessChanged
- OnActivity
- OnAppAction
- OnPurchaseRequest
- OnRestoreRequest

Activity and App Action properties use FNuxieScalarValue, preserving strings,
integers, numbers, and booleans.

## 5. Check and consume Features

Use Has Nuxie Feature with either cache-first or remote policy. Use UseFeature
for fire-and-forget usage, or Use Nuxie Feature And Wait when the authoritative
post-usage access snapshot is required.

## 6. Commerce

Set bUsePurchaseController during configuration and provide an
INuxiePurchaseController. The plugin forwards canonical purchase and restore
requests, waits for the controller result, and completes the native request.

## 7. Shut down

Use ShutdownAsync from C++, or Shutdown Nuxie in Blueprint, and wait for its
completion before reconfiguring the SDK.
