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

- Android: verify flow presentation and callbacks on a physical/emulator device.
- iOS: verify linked `Nuxie` SDK symbols resolve and setup succeeds.
