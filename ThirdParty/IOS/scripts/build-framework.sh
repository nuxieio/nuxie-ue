#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SCHEME="NuxieUnrealBridge"
DERIVED_DATA="$ROOT_DIR/.build/DerivedData"
ARCHIVE_DIR="$ROOT_DIR/.build/ios.xcarchive"
OUTPUT_DIR="$ROOT_DIR/lib"
FRAMEWORK_ZIP="$OUTPUT_DIR/NuxieUnrealBridge.embeddedframework.zip"

resolve_framework_path() {
  local candidate_usr_local_lib="$ARCHIVE_DIR/Products/usr/local/lib/$SCHEME.framework"
  local candidate_frameworks="$ARCHIVE_DIR/Products/Library/Frameworks/$SCHEME.framework"
  if [[ -d "$candidate_usr_local_lib" ]]; then
    printf '%s\n' "$candidate_usr_local_lib"
    return
  fi
  if [[ -d "$candidate_frameworks" ]]; then
    printf '%s\n' "$candidate_frameworks"
    return
  fi
  echo "Unable to find $SCHEME.framework in $ARCHIVE_DIR" >&2
  exit 1
}

rm -rf "$DERIVED_DATA" "$ARCHIVE_DIR" "$FRAMEWORK_ZIP"
mkdir -p "$OUTPUT_DIR"

xcodebuild archive -quiet \
  -scheme "$SCHEME" \
  -destination "generic/platform=iOS" \
  -archivePath "$ARCHIVE_DIR" \
  -derivedDataPath "$DERIVED_DATA" \
  SKIP_INSTALL=NO \
  BUILD_LIBRARY_FOR_DISTRIBUTION=YES \
  SWIFT_STRICT_CONCURRENCY=complete

FRAMEWORK_PATH="$(resolve_framework_path)"
RESOURCE_BUNDLE="$DERIVED_DATA/Build/Intermediates.noindex/ArchiveIntermediates/$SCHEME/IntermediateBuildFilesPath/UninstalledProducts/iphoneos/Nuxie_Nuxie.bundle"
if [[ ! -d "$RESOURCE_BUNDLE" ]]; then
  echo "Unable to find the Nuxie SDK resource bundle." >&2
  exit 1
fi
ditto "$RESOURCE_BUNDLE" "$FRAMEWORK_PATH/Nuxie_Nuxie.bundle"

(
  cd "$(dirname "$FRAMEWORK_PATH")"
  /usr/bin/zip -qry -X \
    "$FRAMEWORK_ZIP" \
    "$(basename "$FRAMEWORK_PATH")"
)

echo "Created $FRAMEWORK_ZIP"
