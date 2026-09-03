# Testing

Run the portable and packaged-artifact checks from the repository root:

    node scripts/test-contract.mjs
    ./scripts/test-android-bridge.sh

To compile the Kotlin adapter against a local exact 0.1.0 Maven fixture:

    NUXIE_ANDROID_MAVEN_REPO=/path/to/maven-repo \
      ./scripts/test-android-bridge.sh

That runs Android unit tests, lint, a release AAR build, and verifies the
exported JVM signatures.

To compile and test the Swift bridge:

    cd ThirdParty/IOS
    xcodebuild test \
      -scheme NuxieUnrealBridge \
      -destination 'platform=iOS Simulator,name=iPhone 17 Pro' \
      SWIFT_STRICT_CONCURRENCY=complete
    ./scripts/build-framework.sh

The framework build uses library evolution and complete concurrency checking,
copies Nuxie_Nuxie.bundle, and creates the archive consumed by Unreal.

An Unreal Engine installation is still required for a full packaged game
smoke test. The native compile gates catch SDK API drift without relying on
runtime symbol lookup.
