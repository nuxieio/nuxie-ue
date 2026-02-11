# Android Bridge

## Components

- C++ JNI bridge: `Source/Nuxie/Private/Platform/Android/NuxieAndroidBridge.cpp`
- Java bridge core: `ThirdParty/Android/src/io/nuxie/unreal/NuxieBridge.java`
- APL integration: `ThirdParty/Android/Nuxie_APL.xml`

## Design

The bridge uses:

1. JNI method calls from C++ into static Java methods.
2. A reflective Java runtime adapter (`ReflectiveRuntime`) to call Nuxie Android SDK APIs.
3. Native callback methods from Java to C++ for trigger updates and lifecycle events.

## Payload encoding

C++ <-> Java payloads use URL-encoded key-value maps (`KvCodec`) for deterministic, dependency-free serialization.

## Purchase/restore continuation

Java bridge holds pending completion futures keyed by `request_id` and resolves them via:

- `completePurchase(requestId, payload)`
- `completeRestore(requestId, payload)`

Default timeout: 60 seconds.

## APL behavior

`Nuxie_APL.xml` performs:

- Android manifest metadata insertion (`NUXIE_API_KEY`)
- Java source copy into build directory
- Gradle dependency insertion for `io.nuxie:nuxie-android`
- proguard keep rules for Nuxie namespaces
