#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC_DIR="$ROOT/ThirdParty/Android/src"
TEST_DIR="$ROOT/ThirdParty/Android/test"
OUT_DIR="$ROOT/ThirdParty/Android/build/jvm-tests"

mkdir -p "$OUT_DIR"

javac -source 8 -target 8 -d "$OUT_DIR" \
  "$SRC_DIR/io/nuxie/unreal/NuxieBridge.java" \
  "$TEST_DIR/io/nuxie/unreal/NuxieBridgeContractTest.java"

java -cp "$OUT_DIR" io.nuxie.unreal.NuxieBridgeContractTest
