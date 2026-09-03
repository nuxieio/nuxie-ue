# Nuxie for Unreal Engine

The Nuxie Unreal plugin exposes native Nuxie Journeys and Features through a
UGameInstanceSubsystem and Blueprint async actions. Version 0.2.0 is a pre-GA
hard cut to the current Journey contract and requires Nuxie iOS and Android
0.1.0.

Trigger records an event and returns immediately. The native SDK selects and
runs any matching Journey. Runtime feedback arrives through typed activity and
App Action events; there is no trigger result, handle, or cancellation API.

## Install

Copy this repository into your Unreal project's Plugins/Nuxie directory and
regenerate project files. Release packages include:

- ThirdParty/Android/lib/nuxie-unreal-bridge.aar
- ThirdParty/IOS/lib/NuxieUnrealBridge.embeddedframework.zip

The Android packaging rule resolves ai.nuxie:nuxie-android:0.1.0 exactly. The
iOS framework contains the Swift bridge, Nuxie SDK, and required resource
bundle.

## Configure

    #include "NuxieSubsystem.h"

    UNuxieSubsystem* Nuxie =
      GetGameInstance()->GetSubsystem<UNuxieSubsystem>();

    FNuxieConfigureOptions Options;
    Options.ApiKey = TEXT("NX_PUBLIC_API_KEY");
    Options.Environment = ENuxieEnvironment::Production;
    Options.PurchaseHandlingMode = ENuxiePurchaseHandlingMode::Full;

    FNuxieError Error;
    Nuxie->Configure(Options, Error);

## Start a Journey from an event

    FNuxieScalarValue Source;
    Source.Type = ENuxieScalarType::String;
    Source.StringValue = TEXT("inventory");

    TMap<FString, FNuxieScalarValue> Properties;
    Properties.Add(TEXT("source"), Source);
    Nuxie->Trigger(TEXT("premium_feature_tapped"), Properties);

The call is intentionally fire-and-forget. Subscribe to OnActivity for typed
runtime telemetry and OnAppAction for actions delegated to the game.

## Features

HasFeatureAsync accepts fractional balances and an explicit
ENuxieFeatureCheckPolicy. UseFeature reports usage without waiting.
UseFeatureAndWaitAsync returns the atomic usage result and its authoritative
access snapshot when one is available.

Blueprint users can call:

- Shutdown Nuxie
- Has Nuxie Feature
- Use Nuxie Feature And Wait
- Dismiss Nuxie
- Set Nuxie Locale

See [getting started](docs/getting-started.md) and the
[API reference](docs/api-reference.md).
