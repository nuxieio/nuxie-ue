#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ANDROID_DIR="$ROOT_DIR/ThirdParty/Android"
AAR="$ANDROID_DIR/lib/nuxie-unreal-bridge.aar"

if [[ ! -f "$AAR" ]]; then
  echo "Missing prepared Android bridge AAR: $AAR" >&2
  exit 1
fi

if [[ -n "${NUXIE_ANDROID_MAVEN_REPO:-}" ]]; then
  (
    cd "$ANDROID_DIR"
    ./gradlew :bridge:testDebugUnitTest :bridge:lint :bridge:prepareBridgeAar
  )
fi

python3 - "$AAR" <<'PY'
import pathlib
import subprocess
import sys
import tempfile
import zipfile

aar = pathlib.Path(sys.argv[1])
with tempfile.TemporaryDirectory(prefix="nuxie-unreal-aar-") as directory:
    classes = pathlib.Path(directory) / "classes.jar"
    with zipfile.ZipFile(aar) as archive:
        classes.write_bytes(archive.read("classes.jar"))
    output = subprocess.check_output(
        [
            "javap",
            "-classpath",
            str(classes),
            "ai.nuxie.unreal.NuxieUnrealBridge",
        ],
        text=True,
    )

required = (
    "invoke(android.app.Activity, java.lang.String, java.lang.String)",
    "popPendingEvent()",
)
for signature in required:
    if signature not in output:
        raise SystemExit(f"missing Android bridge signature: {signature}")

for forbidden in ("java.lang.reflect", "startTrigger", "cancelTrigger", "showFlow"):
    if forbidden in output:
        raise SystemExit(f"retired Android bridge symbol remains: {forbidden}")

print("Android bridge artifact contract passed")
PY
