# Android bridge

The Android binding is a compiled Kotlin AAR in the package ai.nuxie.unreal.
It depends exactly on:

    ai.nuxie:nuxie-android:0.1.0

Unreal C++ passes GameActivity directly to
NuxieUnrealBridge.invoke(Activity, method, argumentsJson). The adapter calls
the typed Kotlin SDK surface and returns one JSON response. Public native events
are written to a concurrent queue and consumed through popPendingEvent().

The adapter directly compiles setup, identity, event recording, dismiss,
locale, policy-aware Feature access, Feature usage, activity, App Actions, and
commerce. SDK method lookup by string is not used.

Build the prepared AAR with:

    cd ThirdParty/Android
    NUXIE_ANDROID_MAVEN_REPO=/path/to/maven-repo \
      ./gradlew :bridge:testDebugUnitTest :bridge:lint :bridge:prepareBridgeAar

The APL copies the resulting AAR into the Unreal Android build and resolves the
native SDK from Maven.
