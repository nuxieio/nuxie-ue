# nuxie-ue

Native-first Unreal Engine plugin for Nuxie mobile SDKs.

`nuxie-ue` provides a UE runtime + Blueprint surface over Nuxie iOS/Android SDKs so gameplay code can use a single API for:

- SDK setup and identity
- trigger streaming (with terminal semantics parity)
- flow presentation
- feature checks and usage
- event queue controls
- purchase/restore request bridging

## Repository layout

- `Nuxie.uplugin`: plugin descriptor.
- `Source/Nuxie`: core runtime module.
- `Source/NuxieBlueprint`: Blueprint async action module.
- `ThirdParty/Android`: Android bridge Java source + APL + JVM tests.
- `docs/`: integration and API docs.
- `scripts/`: local test runners.

## Status

- Android bridge: implemented with JNI + reflective runtime adapter and contract tests.
- iOS bridge: dynamic runtime integration for setup/identity/trigger/showFlow/profile; advanced feature/queue/purchase async methods are currently surfaced with explicit `NATIVE_UNAVAILABLE` errors where selectors are unavailable.

## Requirements

- Unreal Engine `5.4+`
- Android target for full native bridge behavior
- iOS target with `Nuxie` iOS SDK linked in app build
- Java 8+ for local JVM bridge tests

## Installation

### As monorepo submodule

```bash
git submodule add git@github.com:nuxieio/nuxie-ue.git packages/nuxie-ue
```

### In Unreal project

1. Copy/link plugin into your UE project's `Plugins/` directory.
2. Enable `Nuxie` plugin in UE Plugins panel.
3. Regenerate project files.
4. Build your game target.

## Quick start (C++)

```cpp
#include "NuxieSubsystem.h"

UNuxieSubsystem* Nuxie = GetGameInstance()->GetSubsystem<UNuxieSubsystem>();

FNuxieConfigureOptions Config;
Config.ApiKey = TEXT("NX_PROD_...");
Config.bUsePurchaseController = true;

FNuxieError SetupError;
if (!Nuxie->Configure(Config, SetupError)) {
  UE_LOG(LogTemp, Error, TEXT("Nuxie setup failed: %s"), *SetupError.Message);
}

FNuxieError IdentifyError;
Nuxie->Identify(TEXT("user_123"), {}, {}, IdentifyError);

FNuxieTriggerOptions TriggerOptions;
TriggerOptions.Properties.Add(TEXT("source"), TEXT("gameplay"));

FString RequestId;
FNuxieError TriggerError;
Nuxie->StartTrigger(TEXT("premium_feature_tapped"), TriggerOptions, RequestId, TriggerError);
```

## Trigger terminal semantics

`nuxie-ue` uses the same terminal rules as Nuxie mobile wrappers:

Terminal:

- `error`
- `journey`
- `decision.no_match`
- `decision.suppressed`
- `decision.allowed_immediate`
- `decision.denied_immediate`
- `entitlement.allowed`
- `entitlement.denied`

Non-terminal:

- `decision.journey_started`
- `decision.journey_resumed`
- `decision.flow_shown`
- `entitlement.pending`

## Testing

Run local contract and Android bridge tests:

```bash
./scripts/test-trigger-contract.mjs
./scripts/test-android-bridge.sh
```

Or both via CI-equivalent sequence:

```bash
node ./scripts/test-trigger-contract.mjs
./scripts/test-android-bridge.sh
```

## Documentation

- `docs/getting-started.md`
- `docs/api-reference.md`
- `docs/android-bridge.md`
- `docs/ios-bridge.md`
- `docs/testing.md`
- `docs/architecture.md`

## Contributing

1. Keep runtime semantics aligned with `nuxie-ios` / `nuxie-android`.
2. Add or update contract tests when changing trigger/purchase behavior.
3. Keep JNI/ObjC++ bridge changes explicit and accompanied by docs.
