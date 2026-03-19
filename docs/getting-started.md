# Getting Started

## 1. Install plugin

Place the plugin at:

```text
<YourProject>/Plugins/Nuxie
```

## 2. Enable plugin

In Unreal Editor:

1. Open `Edit > Plugins`.
2. Search for `Nuxie`.
3. Enable the plugin.
4. Restart editor.

## 3. Configure SDK early

Use `UGameInstance` startup path or an early gameplay subsystem to call `UNuxieSubsystem::Configure`.

## 4. Set identity

Call `Identify` when you know the authenticated player ID.

## 5. Trigger events

Use `StartTrigger` for progressive updates, then observe `OnTriggerUpdate` or Blueprint async action nodes.

## 6. Handle purchase/restore requests

Attach `INuxiePurchaseController` to `UNuxieSubsystem` using `SetPurchaseController`.

When purchase/restore requests arrive, return `FNuxiePurchaseResult` / `FNuxieRestoreResult` for native runtime continuation.

## 7. Validate on device

- Ensure UE has Android/iOS platform components installed (Epic Games Launcher -> Unreal Engine -> `...` -> `Options`).
- Android: verify flow presentation and callbacks on a physical/emulator device.
- iOS: verify linked `Nuxie` SDK symbols resolve and setup succeeds.

## 8. Native permission action setup

`showFlow(...)` picks up native permission actions automatically from the linked
SDKs. No new Unreal API is required, but your generated mobile projects still
need native declarations when those actions are authored in flows.

iOS keys:

- `NSUserTrackingUsageDescription` for `request_tracking`
- `NSCameraUsageDescription` for `request_permission("camera")`
- `NSMicrophoneUsageDescription` for `request_permission("microphone")`
- `NSPhotoLibraryUsageDescription` for `request_permission("photos")`
- `NSLocationWhenInUseUsageDescription` for
  `request_permission("location")`

Android manifest permissions:

- `android.permission.POST_NOTIFICATIONS`
- `android.permission.CAMERA`
- `android.permission.RECORD_AUDIO`
- `android.permission.READ_MEDIA_IMAGES` on Android 13+ and
  `android.permission.READ_EXTERNAL_STORAGE` on Android 12 and below
- `android.permission.ACCESS_COARSE_LOCATION` and/or
  `android.permission.ACCESS_FINE_LOCATION`

`request_tracking` is iOS-only. `request_notifications` uses the native Android
notification permission path provided by `nuxie-android`, but Android 13+ apps
still need `POST_NOTIFICATIONS` in the host manifest.
