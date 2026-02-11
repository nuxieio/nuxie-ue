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
