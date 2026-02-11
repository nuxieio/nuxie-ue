#!/usr/bin/env node
import fs from 'node:fs';
import path from 'node:path';
import process from 'node:process';

function isTerminal(updateKind, decisionKind, entitlementKind) {
  if (updateKind === 'error' || updateKind === 'journey') {
    return true;
  }

  if (updateKind === 'decision') {
    return (
      decisionKind === 'allowed_immediate' ||
      decisionKind === 'denied_immediate' ||
      decisionKind === 'no_match' ||
      decisionKind === 'suppressed'
    );
  }

  if (updateKind === 'entitlement') {
    return entitlementKind === 'allowed' || entitlementKind === 'denied';
  }

  return false;
}

const fixturePath = path.resolve(process.cwd(), 'tests/fixtures/trigger_terminal_cases.json');
const cases = JSON.parse(fs.readFileSync(fixturePath, 'utf8'));

let failures = 0;
for (const testCase of cases) {
  const actual = isTerminal(testCase.updateKind, testCase.decisionKind, testCase.entitlementKind);
  if (actual !== testCase.expectedTerminal) {
    failures += 1;
    console.error(`[FAIL] ${testCase.name}: expected ${testCase.expectedTerminal}, got ${actual}`);
  }
}

if (failures > 0) {
  process.exit(1);
}

console.log(`Trigger contract tests passed (${cases.length} cases)`);
