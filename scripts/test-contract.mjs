import assert from "node:assert/strict";
import { execFileSync } from "node:child_process";
import { readFileSync, readdirSync, statSync } from "node:fs";
import { join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const root = resolve(fileURLToPath(new URL("..", import.meta.url)));

function read(path) {
  return readFileSync(join(root, path), "utf8");
}

function filesUnder(path) {
  const absolute = join(root, path);
  return readdirSync(absolute, { withFileTypes: true }).flatMap((entry) => {
    const child = join(path, entry.name);
    return entry.isDirectory() ? filesUnder(child) : [child];
  });
}

const types = read("Source/Nuxie/Public/NuxieTypes.h");
const subsystem = read("Source/Nuxie/Public/NuxieSubsystem.h");
const bridge = read("Source/Nuxie/Public/NuxiePlatformBridge.h");
const jsonBridge = read("Source/Nuxie/Private/NuxieJsonBridge.cpp");
const androidBuild = read("ThirdParty/Android/bridge/build.gradle.kts");
const androidApl = read("ThirdParty/Android/Nuxie_APL.xml");
const androidBridge = read(
  "ThirdParty/Android/bridge/src/main/kotlin/ai/nuxie/unreal/NuxieUnrealBridge.kt",
);
const iosPackage = read("ThirdParty/IOS/Package.swift");
const iosBridge = read(
  "ThirdParty/IOS/Sources/NuxieUnrealBridge/NuxieUnrealBridge.swift",
);
const descriptor = JSON.parse(read("Nuxie.uplugin"));

assert.equal(descriptor.VersionName, "0.2.0");
assert.match(subsystem, /void Trigger\s*\(/);
assert.match(bridge, /virtual void Trigger\s*\(/);
assert.match(subsystem, /void ShutdownAsync\s*\(/);
assert.match(bridge, /virtual void ShutdownAsync\s*\(/);
assert.doesNotMatch(subsystem, /bool Shutdown\s*\(/);
assert.doesNotMatch(subsystem, /StartTrigger|CancelTrigger|OnTriggerUpdate/);
assert.match(types, /double Balance/);
assert.match(types, /bHasAuthoritativeAccess/);
assert.match(types, /FNuxieActivityInfo/);
assert.match(types, /FNuxieAppAction/);
assert.match(jsonBridge, /LexToString\(Value\.IntegerValue\)/);
assert.match(jsonBridge, /TEXT\("integer"\)/);
assert.match(jsonBridge, /EncodeNumber\(Value\.NumberValue\)/);
assert.match(androidBridge, /toTaggedScalar/);
assert.match(androidBridge, /toULongOrNull/);
assert.match(iosBridge, /decodeScalarMap/);
assert.match(iosBridge, /Int64\(text\)/);
assert.match(iosBridge, /Double\(bitPattern: bits\)/);

for (const manifest of [androidBuild, androidApl]) {
  assert.match(manifest, /ai\.nuxie:nuxie-android:0\.1\.0/);
}
assert.match(iosPackage, /exact: "0\.1\.0"/);
for (const symbol of [
  "NuxieUnreal_Invoke",
  "NuxieUnreal_PopPendingEvent",
  "NuxieUnreal_FreeCString",
]) {
  assert.match(iosBridge, new RegExp(symbol));
}

const auditedFiles = [
  ...filesUnder("Source"),
  ...filesUnder("ThirdParty/Android/bridge/src"),
  ...filesUnder("ThirdParty/IOS/Sources"),
  "README.md",
  ...filesUnder("docs"),
];
const blueprintActions = filesUnder("Source/NuxieBlueprint/Private/AsyncActions")
  .filter((path) => path.endsWith(".cpp"));
for (const path of blueprintActions) {
  assert.match(
    read(path),
    /RegisterWithGameInstance/,
    `${path} must register until its async callback completes`,
  );
}
const forbidden = [
  /StartTrigger/,
  /CancelTrigger/,
  /ShowFlow/,
  /RefreshProfile/,
  /FlushEvents/,
  /QueuedEvent/,
  /PauseEventQueue/,
  /ResumeEventQueue/,
  /TriggerUpdate/,
  /FeatureCheckResult/,
  /OnFlowPresented/,
  /OnFlowDismissed/,
  /java\.lang\.reflect/,
  /objc_msgSend/,
];

for (const path of auditedFiles) {
  const contents = read(path);
  for (const pattern of forbidden) {
    assert.doesNotMatch(contents, pattern, `${path} contains ${pattern}`);
  }
}

for (const artifact of [
  "ThirdParty/Android/lib/nuxie-unreal-bridge.aar",
  "ThirdParty/IOS/lib/NuxieUnrealBridge.embeddedframework.zip",
]) {
  assert.ok(statSync(join(root, artifact)).size > 0, `${artifact} is empty`);
}

const iosEntries = execFileSync(
  "unzip",
  ["-l", join(root, "ThirdParty/IOS/lib/NuxieUnrealBridge.embeddedframework.zip")],
  { encoding: "utf8" },
);
assert.match(iosEntries, /NuxieUnrealBridge\.framework\/NuxieUnrealBridge/);
assert.match(iosEntries, /Nuxie_Nuxie\.bundle\/PrivacyInfo\.xcprivacy/);
assert.match(iosEntries, /Nuxie_Nuxie\.bundle\/timezone-bundle\.json/);

console.log("Unreal Journey contract passed");
