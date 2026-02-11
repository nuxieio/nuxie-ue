# Testing

## Local tests

### Trigger contract fixtures

```bash
node ./scripts/test-trigger-contract.mjs
```

This validates terminal-state semantics against fixture cases in `tests/fixtures/trigger_terminal_cases.json`.

### Android bridge JVM tests

```bash
./scripts/test-android-bridge.sh
```

This compiles and runs:

- `ThirdParty/Android/src/io/nuxie/unreal/NuxieBridge.java`
- `ThirdParty/Android/test/io/nuxie/unreal/NuxieBridgeContractTest.java`

Covers:

- trigger terminal rules
- trigger event emission behavior
- purchase/restore completion and timeout behavior

## CI

GitHub Actions workflow: `.github/workflows/ci.yml`

Runs:

1. plugin descriptor sanity check
2. trigger contract fixture tests
3. Android bridge JVM tests

## Unreal compile/package validation

Use Unreal Automation Tool to build and package the plugin from source.

```bash
UE_ROOT="/Users/Shared/Epic Games/UE_5.7"
"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildPlugin \
  -Plugin="$(pwd)/Nuxie.uplugin" \
  -Package="/tmp/nuxie-ue-package" \
  -TargetPlatforms=Android+IOS \
  -Rocket -StrictIncludes
```

Expected logs include both:

- `Building plugin for host platforms: Mac`
- `Building plugin for target platforms: Android, IOS`

### If Android/iOS are skipped

`BuildPlugin` skips targets when UE marks them invalid for code projects. Force direct diagnostics with UBT:

```bash
UE_ROOT="/Users/Shared/Epic Games/UE_5.7"
DOTNET="$UE_ROOT/Engine/Binaries/ThirdParty/DotNet/8.0.412/mac-arm64/dotnet"
UBT="$UE_ROOT/Engine/Binaries/DotNET/UnrealBuildTool/UnrealBuildTool.dll"
HOST="/tmp/nuxie-ue-package/HostProject/HostProject.uproject"
PLUGIN="/tmp/nuxie-ue-package/HostProject/Plugins/Nuxie/Nuxie.uplugin"

"$DOTNET" "$UBT" UnrealGame Android Development -Project="$HOST" -plugin="$PLUGIN" -noubtmakefiles -manifest="/tmp/Manifest-UnrealGame-Android-Development.xml" -nohotreload
"$DOTNET" "$UBT" UnrealGame IOS Development -Project="$HOST" -plugin="$PLUGIN" -noubtmakefiles -manifest="/tmp/Manifest-UnrealGame-IOS-Development.xml" -nohotreload
```

Common failure output:

- `Missing files required to build Android targets. Enable Android as an optional download component in the Epic Games Launcher.`
- `Missing files required to build IOS targets. Enable IOS as an optional download component in the Epic Games Launcher.`

Fix by installing Android/iOS UE platform components in Epic Games Launcher for your engine version, then rerun the commands above.
