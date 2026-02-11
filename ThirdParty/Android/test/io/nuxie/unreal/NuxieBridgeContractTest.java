package io.nuxie.unreal;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;

public final class NuxieBridgeContractTest {
  private static final class RecordingEmitter implements NuxieBridge.Emitter {
    int triggerCount;
    int terminalCount;
    int purchaseRequests;
    int restoreRequests;
    final List<String> triggerPayloads = new ArrayList<String>();
    final List<String> purchasePayloads = new ArrayList<String>();
    final List<String> restorePayloads = new ArrayList<String>();

    @Override
    public void onTriggerUpdate(String requestId, String payload, boolean terminal, long timestampMs) {
      triggerCount += 1;
      if (terminal) {
        terminalCount += 1;
      }
      triggerPayloads.add(payload);
    }

    @Override
    public void onFeatureAccessChanged(String featureId, String fromPayload, String toPayload, long timestampMs) {
    }

    @Override
    public void onPurchaseRequest(String payload) {
      purchaseRequests += 1;
      purchasePayloads.add(payload);
    }

    @Override
    public void onRestoreRequest(String payload) {
      restoreRequests += 1;
      restorePayloads.add(payload);
    }

    @Override
    public void onFlowPresented(String flowId, long timestampMs) {
    }

    @Override
    public void onFlowDismissed(String payload, long timestampMs) {
    }
  }

  private static final class FakeRuntime implements NuxieBridge.Runtime {
    NuxieBridge.RuntimeCallbacks callbacks;
    String distinctId = "anon_test";

    @Override
    public void configure(String apiKey, Map<String, String> options, boolean usePurchaseController, NuxieBridge.RuntimeCallbacks callbacks) {
      this.callbacks = callbacks;
    }

    @Override
    public void shutdown() {
    }

    @Override
    public void identify(String distinctId, Map<String, String> userProperties, Map<String, String> userPropertiesSetOnce) {
      this.distinctId = distinctId;
    }

    @Override
    public void reset(boolean keepAnonymousId) {
      this.distinctId = "anon_test";
    }

    @Override
    public String getDistinctId() {
      return distinctId;
    }

    @Override
    public String getAnonymousId() {
      return "anon_test";
    }

    @Override
    public boolean getIsIdentified() {
      return !"anon_test".equals(distinctId);
    }

    @Override
    public void startTrigger(String requestId, String eventName, NuxieBridge.TriggerOptions triggerOptions) {
      NuxieBridge.TriggerUpdatePayload nonTerminal = new NuxieBridge.TriggerUpdatePayload();
      nonTerminal.kind = "decision";
      nonTerminal.decisionKind = "flow_shown";
      callbacks.onTriggerUpdate(requestId, nonTerminal);

      NuxieBridge.TriggerUpdatePayload terminal = new NuxieBridge.TriggerUpdatePayload();
      terminal.kind = "entitlement";
      terminal.entitlementKind = "allowed";
      callbacks.onTriggerUpdate(requestId, terminal);
    }

    @Override
    public void cancelTrigger(String requestId) {
    }

    @Override
    public void showFlow(String flowId) {
      callbacks.onFlowPresented(flowId);
    }

    @Override
    public NuxieBridge.ProfilePayload refreshProfile() {
      NuxieBridge.ProfilePayload payload = new NuxieBridge.ProfilePayload();
      payload.customerId = "customer_1";
      payload.raw = "{}";
      return payload;
    }

    @Override
    public NuxieBridge.FeatureAccessPayload hasFeature(String featureId, Integer requiredBalance, String entityId) {
      NuxieBridge.FeatureAccessPayload payload = new NuxieBridge.FeatureAccessPayload();
      payload.allowed = true;
      payload.unlimited = false;
      payload.hasBalance = true;
      payload.balance = 7;
      payload.type = "metered";
      return payload;
    }

    @Override
    public NuxieBridge.FeatureCheckPayload checkFeature(String featureId, Integer requiredBalance, String entityId, boolean forceRefresh) {
      NuxieBridge.FeatureCheckPayload payload = new NuxieBridge.FeatureCheckPayload();
      payload.customerId = "customer_1";
      payload.featureId = featureId;
      payload.requiredBalance = requiredBalance == null ? 1 : requiredBalance.intValue();
      payload.code = "ok";
      payload.access = hasFeature(featureId, requiredBalance, entityId);
      return payload;
    }

    @Override
    public void useFeature(String featureId, double amount, String entityId, Map<String, String> metadata) {
    }

    @Override
    public NuxieBridge.FeatureUsagePayload useFeatureAndWait(
      String featureId,
      double amount,
      String entityId,
      boolean setUsage,
      Map<String, String> metadata) {
      NuxieBridge.FeatureUsagePayload payload = new NuxieBridge.FeatureUsagePayload();
      payload.success = true;
      payload.featureId = featureId;
      payload.amountUsed = amount;
      return payload;
    }

    @Override
    public boolean flushEvents() {
      return true;
    }

    @Override
    public int getQueuedEventCount() {
      return 0;
    }

    @Override
    public void pauseEventQueue() {
    }

    @Override
    public void resumeEventQueue() {
    }

    @Override
    public void completePurchase(String requestId, NuxieBridge.PurchaseResultPayload result) {
    }

    @Override
    public void completeRestore(String requestId, NuxieBridge.RestoreResultPayload result) {
    }

    CompletableFuture<NuxieBridge.PurchaseResultPayload> awaitPurchase(long timeoutMs) {
      final CompletableFuture<NuxieBridge.PurchaseResultPayload> future = new CompletableFuture<NuxieBridge.PurchaseResultPayload>();
      final NuxieBridge.PurchaseRequestPayload req = NuxieBridge.PurchaseRequestPayload.create("pro_monthly");
      new Thread(new Runnable() {
        @Override
        public void run() {
          NuxieBridge.PurchaseResultPayload result = callbacks.awaitPurchaseResult(req, timeoutMs);
          future.complete(result);
        }
      }).start();
      future.whenComplete((r, e) -> {
      });
      return future;
    }

    CompletableFuture<NuxieBridge.RestoreResultPayload> awaitRestore(long timeoutMs) {
      final CompletableFuture<NuxieBridge.RestoreResultPayload> future = new CompletableFuture<NuxieBridge.RestoreResultPayload>();
      final NuxieBridge.RestoreRequestPayload req = NuxieBridge.RestoreRequestPayload.create();
      new Thread(new Runnable() {
        @Override
        public void run() {
          NuxieBridge.RestoreResultPayload result = callbacks.awaitRestoreResult(req, timeoutMs);
          future.complete(result);
        }
      }).start();
      return future;
    }
  }

  public static void main(String[] args) throws Exception {
    testTerminalContract();
    testTriggerEmission();
    testPurchaseAndRestoreCompletion();
    testPurchaseAndRestoreTimeout();
    System.out.println("NuxieBridgeContractTest: all tests passed");
  }

  private static void testTerminalContract() {
    NuxieBridge.TriggerUpdatePayload update = new NuxieBridge.TriggerUpdatePayload();
    update.kind = "decision";
    update.decisionKind = "flow_shown";
    assertFalse(update.isTerminal(), "flow_shown should be non-terminal");

    update.decisionKind = "no_match";
    assertTrue(update.isTerminal(), "no_match should be terminal");

    update.kind = "entitlement";
    update.entitlementKind = "pending";
    assertFalse(update.isTerminal(), "entitlement.pending should be non-terminal");

    update.entitlementKind = "denied";
    assertTrue(update.isTerminal(), "entitlement.denied should be terminal");

    update.kind = "journey";
    assertTrue(update.isTerminal(), "journey should be terminal");

    update.kind = "error";
    assertTrue(update.isTerminal(), "error should be terminal");
  }

  private static void testTriggerEmission() throws Exception {
    FakeRuntime runtime = new FakeRuntime();
    RecordingEmitter emitter = new RecordingEmitter();
    NuxieBridge.setRuntimeForTesting(runtime);
    NuxieBridge.setEmitterForTesting(emitter);

    NuxieBridge.configure("NX_TEST", "", false, "0.1.0-test");
    NuxieBridge.startTrigger("req-1", "event_a", "");

    assertEquals(2, emitter.triggerCount, "expected two trigger updates");
    assertEquals(1, emitter.terminalCount, "expected one terminal update");
  }

  private static void testPurchaseAndRestoreCompletion() throws Exception {
    FakeRuntime runtime = new FakeRuntime();
    RecordingEmitter emitter = new RecordingEmitter();
    NuxieBridge.setRuntimeForTesting(runtime);
    NuxieBridge.setEmitterForTesting(emitter);
    NuxieBridge.setRequestTimeoutMillisForTesting(2000);
    NuxieBridge.configure("NX_TEST", "", true, "0.1.0-test");

    CompletableFuture<NuxieBridge.PurchaseResultPayload> purchaseFuture = runtime.awaitPurchase(2000);
    Thread.sleep(30L);
    assertEquals(1, emitter.purchaseRequests, "purchase request should emit");

    NuxieBridge.PurchaseResultPayload purchaseResult = new NuxieBridge.PurchaseResultPayload();
    purchaseResult.kind = "success";
    String purchaseRequestPayload = emitter.purchasePayloads.get(emitter.purchasePayloads.size() - 1);
    String purchaseRequestId = NuxieBridge.KvCodec.decodeMap(purchaseRequestPayload).get("request_id");
    NuxieBridge.completePurchase(purchaseRequestId, purchaseResult.toPayload());

    // restore completion
    CompletableFuture<NuxieBridge.RestoreResultPayload> restoreFuture = runtime.awaitRestore(2000);
    Thread.sleep(30L);
    assertEquals(1, emitter.restoreRequests, "restore request should emit");

    NuxieBridge.RestoreResultPayload restoreResult = new NuxieBridge.RestoreResultPayload();
    restoreResult.kind = "success";
    restoreResult.restoredCount = 1;
    String restoreRequestPayload = emitter.restorePayloads.get(emitter.restorePayloads.size() - 1);
    String restoreRequestId = NuxieBridge.KvCodec.decodeMap(restoreRequestPayload).get("request_id");
    NuxieBridge.completeRestore(restoreRequestId, restoreResult.toPayload());

    NuxieBridge.PurchaseResultPayload purchaseCompleted = purchaseFuture.get(3, TimeUnit.SECONDS);
    assertEquals("success", purchaseCompleted.kind, "purchase completion should resolve success");

    NuxieBridge.RestoreResultPayload restoreCompleted = restoreFuture.get(3, TimeUnit.SECONDS);
    assertEquals("success", restoreCompleted.kind, "restore completion should resolve success");
  }

  private static void testPurchaseAndRestoreTimeout() throws Exception {
    FakeRuntime runtime = new FakeRuntime();
    RecordingEmitter emitter = new RecordingEmitter();
    NuxieBridge.setRuntimeForTesting(runtime);
    NuxieBridge.setEmitterForTesting(emitter);
    NuxieBridge.configure("NX_TEST", "", true, "0.1.0-test");

    NuxieBridge.PurchaseResultPayload purchaseResult = runtime.awaitPurchase(60).get(2, TimeUnit.SECONDS);
    assertEquals("failed", purchaseResult.kind, "purchase timeout should fail");

    NuxieBridge.RestoreResultPayload restoreResult = runtime.awaitRestore(60).get(2, TimeUnit.SECONDS);
    assertEquals("failed", restoreResult.kind, "restore timeout should fail");
  }

  private static void assertTrue(boolean condition, String message) {
    if (!condition) {
      throw new AssertionError(message);
    }
  }

  private static void assertFalse(boolean condition, String message) {
    if (condition) {
      throw new AssertionError(message);
    }
  }

  private static void assertEquals(Object expected, Object actual, String message) {
    if (expected == null && actual == null) {
      return;
    }
    if (expected != null && expected.equals(actual)) {
      return;
    }
    throw new AssertionError(message + " expected=" + expected + " actual=" + actual);
  }
}
