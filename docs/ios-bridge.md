# iOS Bridge

## Components

- Objective-C++ bridge: `Source/Nuxie/Private/Platform/IOS/NuxieIOSBridge.mm`

## Design

The iOS bridge uses dynamic Objective-C runtime lookup (`NSClassFromString`, selector dispatch) so it can adapt to different linked SDK symbol layouts without hard compile-time binding to generated Swift headers.

Implemented runtime operations:

- setup/configure
- identify/reset
- distinct/anonymous ID access
- trigger start/cancel
- flow show
- profile refresh (async completion bridging)

Currently guarded with explicit `NATIVE_UNAVAILABLE` for operations where selectors are unavailable at runtime.

## Notes

Because the bridge is dynamic, final runtime behavior depends on linked `Nuxie` iOS SDK symbol availability and selector naming in the consuming app build.
