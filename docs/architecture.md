# Architecture

The plugin has one public contract and two compiled native transports.

UNuxieSubsystem owns the public C++ and Blueprint surface. It forwards calls
to FNuxieJsonBridge, which serializes portable scalar values and parses typed
Feature, activity, App Action, and commerce payloads. A game-thread ticker
drains native events, so both platforms deliver the same event shape.
Operations backed by native async APIs, including shutdown, run away from the
game thread and complete on it.

Android packages a Kotlin adapter that imports the Nuxie Android SDK at compile
time. C++ calls two static methods through JNI:

- invoke(Activity, method, argumentsJson)
- popPendingEvent()

iOS packages a Swift dynamic framework that imports the Nuxie iOS SDK at
compile time. Objective-C runtime lookup is not used. C++ calls the versioned C
ABI described in [ios-bridge.md](ios-bridge.md).

The JSON boundary is private implementation detail. It exists to keep the
Unreal parser and platform adapters on one contract; game code uses Unreal
types only. Scalar values carry an explicit type tag. Integers cross as decimal
strings and numbers cross as their exact IEEE 754 bit pattern, so int64 values,
floating-point values, and their distinct types survive the boundary exactly.
