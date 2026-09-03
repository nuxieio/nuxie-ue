# iOS bridge

The iOS binding is a Swift dynamic framework compiled against Nuxie iOS 0.1.0.
It exports a small C ABI:

    char *NuxieUnreal_Invoke(const char *method, const char *arguments_json);
    char *NuxieUnreal_PopPendingEvent(void);
    void NuxieUnreal_FreeCString(char *value);

The Swift implementation calls the typed NuxieSDK.shared surface directly. It
uses the same private response and event envelope as Android. The framework
contains Nuxie_Nuxie.bundle, including the privacy manifest and timezone data
required by the SDK.

Build the embedded framework archive with:

    cd ThirdParty/IOS
    ./scripts/build-framework.sh

The build runs with library evolution and complete concurrency checking.
Nuxie.Build.cs embeds the prepared framework on iOS.
