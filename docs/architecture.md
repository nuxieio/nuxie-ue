# Architecture

## Runtime modules

- `Nuxie`: core runtime API, type system, subsystem, and platform bridges.
- `NuxieBlueprint`: async Blueprint wrappers over `UNuxieSubsystem`.

## Core flow

1. `UNuxieSubsystem` initializes a platform bridge via `CreateNuxiePlatformBridge()`.
2. Calls from game code and blueprints route to the bridge.
3. Native callbacks route back through `INuxiePlatformBridgeListener`.
4. Subsystem re-emits events as Blueprint multicast delegates.

## Contract alignment

Trigger terminal-state semantics are centralized in `Nuxie::FTriggerContract::IsTerminal` and match mobile wrapper contracts.

For implementation details, see:

- `Source/Nuxie/Public/NuxieTypes.h`
- `Source/Nuxie/Public/NuxieSubsystem.h`
- `Source/Nuxie/Private/Platform/NuxiePlatformBridge.h`
