package io.nuxie.unreal;

import java.lang.reflect.Constructor;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.UUID;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;

/**
 * Unreal <-> native Android bridge entrypoint.
 *
 * This class intentionally avoids compile-time dependencies on Android or Nuxie SDK symbols;
 * integration happens via reflection so unit tests can run on plain JVM.
 */
public final class NuxieBridge {
  public interface Emitter {
    void onTriggerUpdate(String requestId, String payload, boolean terminal, long timestampMs);

    void onFeatureAccessChanged(String featureId, String fromPayload, String toPayload, long timestampMs);

    void onPurchaseRequest(String payload);

    void onRestoreRequest(String payload);

    void onFlowPresented(String flowId, long timestampMs);

    void onFlowDismissed(String payload, long timestampMs);
  }

  interface RuntimeCallbacks {
    void onTriggerUpdate(String requestId, TriggerUpdatePayload update);

    void onFeatureAccessChanged(String featureId, FeatureAccessPayload from, FeatureAccessPayload to);

    PurchaseResultPayload awaitPurchaseResult(PurchaseRequestPayload request, long timeoutMs);

    RestoreResultPayload awaitRestoreResult(RestoreRequestPayload request, long timeoutMs);

    void onFlowPresented(String flowId);

    void onFlowDismissed(FlowDismissedPayload payload);
  }

  interface Runtime {
    void configure(String apiKey, Map<String, String> options, boolean usePurchaseController, RuntimeCallbacks callbacks) throws Exception;

    void shutdown() throws Exception;

    void identify(String distinctId, Map<String, String> userProperties, Map<String, String> userPropertiesSetOnce) throws Exception;

    void reset(boolean keepAnonymousId) throws Exception;

    String getDistinctId() throws Exception;

    String getAnonymousId() throws Exception;

    boolean getIsIdentified() throws Exception;

    void startTrigger(String requestId, String eventName, TriggerOptions triggerOptions) throws Exception;

    void cancelTrigger(String requestId) throws Exception;

    void showFlow(String flowId) throws Exception;

    ProfilePayload refreshProfile() throws Exception;

    FeatureAccessPayload hasFeature(String featureId, Integer requiredBalance, String entityId) throws Exception;

    FeatureCheckPayload checkFeature(String featureId, Integer requiredBalance, String entityId, boolean forceRefresh) throws Exception;

    void useFeature(String featureId, double amount, String entityId, Map<String, String> metadata) throws Exception;

    FeatureUsagePayload useFeatureAndWait(String featureId, double amount, String entityId, boolean setUsage, Map<String, String> metadata) throws Exception;

    boolean flushEvents() throws Exception;

    int getQueuedEventCount() throws Exception;

    void pauseEventQueue() throws Exception;

    void resumeEventQueue() throws Exception;

    void completePurchase(String requestId, PurchaseResultPayload result) throws Exception;

    void completeRestore(String requestId, RestoreResultPayload result) throws Exception;
  }

  private static final class BridgeCore implements RuntimeCallbacks {
    private final Map<String, CompletableFuture<PurchaseResultPayload>> pendingPurchases = new ConcurrentHashMap<>();
    private final Map<String, CompletableFuture<RestoreResultPayload>> pendingRestores = new ConcurrentHashMap<>();
    private final Map<String, Object> startedTriggers = new ConcurrentHashMap<>();

    private volatile Runtime runtime;
    private volatile Emitter emitter;
    private volatile long nativeHandle;
    private volatile long requestTimeoutMillis = 60_000L;

    private BridgeCore() {
      this.runtime = new ReflectiveRuntime();
    }

    synchronized void setRuntimeForTesting(Runtime runtime) {
      this.runtime = runtime;
    }

    synchronized void setEmitterForTesting(Emitter emitter) {
      this.emitter = emitter;
    }

    synchronized void setNativeHandle(long nativeHandle) {
      this.nativeHandle = nativeHandle;
    }

    synchronized void setRequestTimeoutMillisForTesting(long timeoutMillis) {
      this.requestTimeoutMillis = timeoutMillis;
    }

    synchronized void configure(String apiKey, String optionsPayload, boolean usePurchaseController, String wrapperVersion)
      throws Exception {
      Map<String, String> options = KvCodec.decodeMap(optionsPayload);
      if (wrapperVersion != null && !wrapperVersion.isEmpty()) {
        options.put("wrapper_version", wrapperVersion);
      }

      runtime.configure(apiKey, options, usePurchaseController, this);
    }

    synchronized void shutdown() throws Exception {
      runtime.shutdown();
      startedTriggers.clear();
      pendingPurchases.clear();
      pendingRestores.clear();
    }

    synchronized void identify(String distinctId, String userPropertiesPayload, String userPropertiesSetOncePayload) throws Exception {
      runtime.identify(
        distinctId,
        KvCodec.decodeMap(userPropertiesPayload),
        KvCodec.decodeMap(userPropertiesSetOncePayload));
    }

    synchronized void reset(boolean keepAnonymousId) throws Exception {
      runtime.reset(keepAnonymousId);
    }

    synchronized String getDistinctId() throws Exception {
      return runtime.getDistinctId();
    }

    synchronized String getAnonymousId() throws Exception {
      return runtime.getAnonymousId();
    }

    synchronized boolean getIsIdentified() throws Exception {
      return runtime.getIsIdentified();
    }

    synchronized void startTrigger(String requestId, String eventName, String triggerOptionsPayload) throws Exception {
      TriggerOptions options = TriggerOptions.fromPayload(triggerOptionsPayload);
      startedTriggers.put(requestId, new Object());
      runtime.startTrigger(requestId, eventName, options);
    }

    synchronized void cancelTrigger(String requestId) throws Exception {
      startedTriggers.remove(requestId);
      runtime.cancelTrigger(requestId);
    }

    synchronized void showFlow(String flowId) throws Exception {
      runtime.showFlow(flowId);
    }

    synchronized String refreshProfile() throws Exception {
      return runtime.refreshProfile().toPayload();
    }

    synchronized String hasFeature(String featureId, Integer requiredBalance, String entityId) throws Exception {
      return runtime.hasFeature(featureId, requiredBalance, entityId).toPayload();
    }

    synchronized String checkFeature(String featureId, Integer requiredBalance, String entityId, boolean forceRefresh)
      throws Exception {
      return runtime.checkFeature(featureId, requiredBalance, entityId, forceRefresh).toPayload();
    }

    synchronized void useFeature(String featureId, double amount, String entityId, String metadataPayload) throws Exception {
      runtime.useFeature(featureId, amount, entityId, KvCodec.decodeMap(metadataPayload));
    }

    synchronized String useFeatureAndWait(
      String featureId,
      double amount,
      String entityId,
      boolean setUsage,
      String metadataPayload)
      throws Exception {
      return runtime.useFeatureAndWait(featureId, amount, entityId, setUsage, KvCodec.decodeMap(metadataPayload)).toPayload();
    }

    synchronized boolean flushEvents() throws Exception {
      return runtime.flushEvents();
    }

    synchronized int getQueuedEventCount() throws Exception {
      return runtime.getQueuedEventCount();
    }

    synchronized void pauseEventQueue() throws Exception {
      runtime.pauseEventQueue();
    }

    synchronized void resumeEventQueue() throws Exception {
      runtime.resumeEventQueue();
    }

    synchronized void completePurchase(String requestId, String purchaseResultPayload) throws Exception {
      PurchaseResultPayload result = PurchaseResultPayload.fromPayload(purchaseResultPayload);
      CompletableFuture<PurchaseResultPayload> future = pendingPurchases.remove(requestId);
      if (future != null) {
        future.complete(result);
      }
      runtime.completePurchase(requestId, result);
    }

    synchronized void completeRestore(String requestId, String restoreResultPayload) throws Exception {
      RestoreResultPayload result = RestoreResultPayload.fromPayload(restoreResultPayload);
      CompletableFuture<RestoreResultPayload> future = pendingRestores.remove(requestId);
      if (future != null) {
        future.complete(result);
      }
      runtime.completeRestore(requestId, result);
    }

    @Override
    public void onTriggerUpdate(String requestId, TriggerUpdatePayload update) {
      boolean terminal = update.isTerminal();
      if (terminal) {
        startedTriggers.remove(requestId);
      }
      emitTriggerUpdate(requestId, update.toPayload(), terminal, update.timestampMs);
    }

    @Override
    public void onFeatureAccessChanged(String featureId, FeatureAccessPayload from, FeatureAccessPayload to) {
      emitFeatureAccessChanged(featureId, from != null ? from.toPayload() : "", to != null ? to.toPayload() : "", nowMs());
    }

    @Override
    public PurchaseResultPayload awaitPurchaseResult(PurchaseRequestPayload request, long timeoutMs) {
      CompletableFuture<PurchaseResultPayload> future = new CompletableFuture<PurchaseResultPayload>();
      pendingPurchases.put(request.requestId, future);
      emitPurchaseRequest(request.toPayload());
      try {
        return future.get(timeoutMs, TimeUnit.MILLISECONDS);
      } catch (Exception timeout) {
        pendingPurchases.remove(request.requestId);
        return PurchaseResultPayload.failed("purchase_timeout");
      }
    }

    @Override
    public RestoreResultPayload awaitRestoreResult(RestoreRequestPayload request, long timeoutMs) {
      CompletableFuture<RestoreResultPayload> future = new CompletableFuture<RestoreResultPayload>();
      pendingRestores.put(request.requestId, future);
      emitRestoreRequest(request.toPayload());
      try {
        return future.get(timeoutMs, TimeUnit.MILLISECONDS);
      } catch (Exception timeout) {
        pendingRestores.remove(request.requestId);
        return RestoreResultPayload.failed("restore_timeout");
      }
    }

    @Override
    public void onFlowPresented(String flowId) {
      emitFlowPresented(flowId, nowMs());
    }

    @Override
    public void onFlowDismissed(FlowDismissedPayload payload) {
      emitFlowDismissed(payload.toPayload(), nowMs());
    }

    private void emitTriggerUpdate(String requestId, String payload, boolean terminal, long timestampMs) {
      Emitter localEmitter = emitter;
      if (localEmitter != null) {
        localEmitter.onTriggerUpdate(requestId, payload, terminal, timestampMs);
        return;
      }

      long handle = nativeHandle;
      if (handle != 0L) {
        nativeOnTriggerUpdate(handle, requestId, payload, terminal, timestampMs);
      }
    }

    private void emitFeatureAccessChanged(String featureId, String fromPayload, String toPayload, long timestampMs) {
      Emitter localEmitter = emitter;
      if (localEmitter != null) {
        localEmitter.onFeatureAccessChanged(featureId, fromPayload, toPayload, timestampMs);
        return;
      }

      long handle = nativeHandle;
      if (handle != 0L) {
        nativeOnFeatureAccessChanged(handle, featureId, fromPayload, toPayload, timestampMs);
      }
    }

    private void emitPurchaseRequest(String payload) {
      Emitter localEmitter = emitter;
      if (localEmitter != null) {
        localEmitter.onPurchaseRequest(payload);
        return;
      }

      long handle = nativeHandle;
      if (handle != 0L) {
        nativeOnPurchaseRequest(handle, payload);
      }
    }

    private void emitRestoreRequest(String payload) {
      Emitter localEmitter = emitter;
      if (localEmitter != null) {
        localEmitter.onRestoreRequest(payload);
        return;
      }

      long handle = nativeHandle;
      if (handle != 0L) {
        nativeOnRestoreRequest(handle, payload);
      }
    }

    private void emitFlowPresented(String flowId, long timestampMs) {
      Emitter localEmitter = emitter;
      if (localEmitter != null) {
        localEmitter.onFlowPresented(flowId, timestampMs);
        return;
      }

      long handle = nativeHandle;
      if (handle != 0L) {
        nativeOnFlowPresented(handle, flowId, timestampMs);
      }
    }

    private void emitFlowDismissed(String payload, long timestampMs) {
      Emitter localEmitter = emitter;
      if (localEmitter != null) {
        localEmitter.onFlowDismissed(payload, timestampMs);
        return;
      }

      long handle = nativeHandle;
      if (handle != 0L) {
        nativeOnFlowDismissed(handle, payload, timestampMs);
      }
    }
  }

  private static final class ReflectiveRuntime implements Runtime {
    private static final String SDK_CLASS = "io.nuxie.sdk.NuxieSDK";

    private Object sdk;
    private RuntimeCallbacks callbacks;
    private final Map<String, Object> triggerHandles = new ConcurrentHashMap<String, Object>();

    @Override
    public void configure(String apiKey, Map<String, String> options, boolean usePurchaseController, RuntimeCallbacks callbacks)
      throws Exception {
      this.callbacks = callbacks;

      Class<?> sdkClass = Class.forName(SDK_CLASS);
      this.sdk = sdkClass.getMethod("shared").invoke(null);

      Object configuration = buildConfiguration(apiKey, options, usePurchaseController, callbacks);
      Object context = resolveContext();
      Method setup = findMethod(sdkClass, "setup", 2);
      setup.invoke(sdk, context, configuration);

      attachDelegate(sdkClass, callbacks);
    }

    @Override
    public void shutdown() throws Exception {
      if (sdk == null) {
        return;
      }
      Method shutdown = findMethod(sdk.getClass(), "shutdown", 1);
      invokeSuspend(shutdown, sdk);
      triggerHandles.clear();
    }

    @Override
    public void identify(String distinctId, Map<String, String> userProperties, Map<String, String> userPropertiesSetOnce)
      throws Exception {
      Method identify = findMethod(sdk.getClass(), "identify", 3);
      identify.invoke(sdk, distinctId, userProperties, userPropertiesSetOnce);
    }

    @Override
    public void reset(boolean keepAnonymousId) throws Exception {
      Method reset = findMethod(sdk.getClass(), "reset", 1);
      reset.invoke(sdk, keepAnonymousId);
    }

    @Override
    public String getDistinctId() throws Exception {
      Method method = findMethod(sdk.getClass(), "getDistinctId", 0);
      return String.valueOf(method.invoke(sdk));
    }

    @Override
    public String getAnonymousId() throws Exception {
      Method method = findMethod(sdk.getClass(), "getAnonymousId", 0);
      return String.valueOf(method.invoke(sdk));
    }

    @Override
    public boolean getIsIdentified() throws Exception {
      Method method = findMethod(sdk.getClass(), "isIdentified", 0);
      return ((Boolean) method.invoke(sdk)).booleanValue();
    }

    @Override
    public void startTrigger(String requestId, String eventName, TriggerOptions triggerOptions) throws Exception {
      Class<?> function1Class = Class.forName("kotlin.jvm.functions.Function1");
      Object handler = Proxy.newProxyInstance(
        function1Class.getClassLoader(),
        new Class[] { function1Class },
        new InvocationHandler() {
          @Override
          public Object invoke(Object proxy, Method method, Object[] args) {
            if ("invoke".equals(method.getName()) && args != null && args.length == 1) {
              TriggerUpdatePayload update = mapTriggerUpdate(args[0]);
              callbacks.onTriggerUpdate(requestId, update);
              return kotlinUnit();
            }
            return null;
          }
        });

      Method trigger = findMethod(sdk.getClass(), "trigger", 5);
      Object handle = trigger.invoke(
        sdk,
        eventName,
        triggerOptions.properties,
        triggerOptions.userProperties,
        triggerOptions.userPropertiesSetOnce,
        handler);

      triggerHandles.put(requestId, handle);
    }

    @Override
    public void cancelTrigger(String requestId) throws Exception {
      Object handle = triggerHandles.remove(requestId);
      if (handle == null) {
        return;
      }
      Method cancel = findMethod(handle.getClass(), "cancel", 0);
      cancel.invoke(handle);
    }

    @Override
    public void showFlow(String flowId) throws Exception {
      Method showFlow = findMethod(sdk.getClass(), "showFlow", 1);
      showFlow.invoke(sdk, flowId);
      callbacks.onFlowPresented(flowId);
    }

    @Override
    public ProfilePayload refreshProfile() throws Exception {
      Method refresh = findMethod(sdk.getClass(), "refreshProfile", 1);
      Object profile = invokeSuspend(refresh, sdk);
      return ProfilePayload.fromProfile(profile);
    }

    @Override
    public FeatureAccessPayload hasFeature(String featureId, Integer requiredBalance, String entityId) throws Exception {
      if (requiredBalance != null) {
        Method hasFeature = findMethod(sdk.getClass(), "hasFeature", 4);
        Object access = invokeSuspend(hasFeature, sdk, featureId, Integer.valueOf(requiredBalance.intValue()), entityId);
        return FeatureAccessPayload.fromFeatureAccess(access);
      }

      Method hasFeature = findMethod(sdk.getClass(), "hasFeature", 2);
      Object access = invokeSuspend(hasFeature, sdk, featureId);
      return FeatureAccessPayload.fromFeatureAccess(access);
    }

    @Override
    public FeatureCheckPayload checkFeature(String featureId, Integer requiredBalance, String entityId, boolean forceRefresh)
      throws Exception {
      final String methodName = forceRefresh ? "refreshFeature" : "checkFeature";
      Method check = findMethod(sdk.getClass(), methodName, 4);
      Object result = invokeSuspend(check, sdk, featureId, requiredBalance, entityId);
      return FeatureCheckPayload.fromCheckResult(result);
    }

    @Override
    public void useFeature(String featureId, double amount, String entityId, Map<String, String> metadata) throws Exception {
      Method useFeature = findMethod(sdk.getClass(), "useFeature", 4);
      useFeature.invoke(sdk, featureId, Double.valueOf(amount), entityId, metadata);
    }

    @Override
    public FeatureUsagePayload useFeatureAndWait(
      String featureId,
      double amount,
      String entityId,
      boolean setUsage,
      Map<String, String> metadata)
      throws Exception {
      Method method = findMethod(sdk.getClass(), "useFeatureAndWait", 6);
      Object result = invokeSuspend(method, sdk, featureId, Double.valueOf(amount), entityId, Boolean.valueOf(setUsage), metadata);
      return FeatureUsagePayload.fromUsageResult(result);
    }

    @Override
    public boolean flushEvents() throws Exception {
      Method method = findMethod(sdk.getClass(), "flushEvents", 1);
      Object value = invokeSuspend(method, sdk);
      return asBoolean(value);
    }

    @Override
    public int getQueuedEventCount() throws Exception {
      Method method = findMethod(sdk.getClass(), "getQueuedEventCount", 1);
      Object value = invokeSuspend(method, sdk);
      return asInt(value);
    }

    @Override
    public void pauseEventQueue() throws Exception {
      Method method = findMethod(sdk.getClass(), "pauseEventQueue", 1);
      invokeSuspend(method, sdk);
    }

    @Override
    public void resumeEventQueue() throws Exception {
      Method method = findMethod(sdk.getClass(), "resumeEventQueue", 1);
      invokeSuspend(method, sdk);
    }

    @Override
    public void completePurchase(String requestId, PurchaseResultPayload result) {
      // No-op: purchase/restore completions are resolved in the bridge-level futures.
    }

    @Override
    public void completeRestore(String requestId, RestoreResultPayload result) {
      // No-op: purchase/restore completions are resolved in the bridge-level futures.
    }

    private Object buildConfiguration(String apiKey, Map<String, String> options, boolean usePurchaseController, RuntimeCallbacks callbacks)
      throws Exception {
      Class<?> configClass = Class.forName("io.nuxie.sdk.config.NuxieConfiguration");
      Constructor<?> ctor = configClass.getConstructor(String.class);
      Object config = ctor.newInstance(apiKey);

      setOptionalField(configClass, config, "localeIdentifier", options.get("locale"));
      setOptionalBooleanField(configClass, config, "isDebugMode", options.get("debug"));
      setOptionalBooleanField(configClass, config, "enableConsoleLogging", options.get("console_logging"));
      setOptionalBooleanField(configClass, config, "enableFileLogging", options.get("file_logging"));

      if (options.get("api_endpoint") != null && !options.get("api_endpoint").isEmpty()) {
        Method setApiEndpoint = findMethod(configClass, "setApiEndpoint", 1);
        setApiEndpoint.invoke(config, options.get("api_endpoint"));
      }

      if (usePurchaseController) {
        Class<?> delegateInterface = Class.forName("io.nuxie.sdk.purchases.NuxiePurchaseDelegate");
        Object delegateProxy = Proxy.newProxyInstance(
          delegateInterface.getClassLoader(),
          new Class[] { delegateInterface },
          new InvocationHandler() {
            @Override
            public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
              String name = method.getName();
              if ("purchase".equals(name)) {
                String productId = args != null && args.length > 0 && args[0] != null ? String.valueOf(args[0]) : "";
                PurchaseRequestPayload request = PurchaseRequestPayload.create(productId);
                PurchaseResultPayload result = callbacks.awaitPurchaseResult(request, 60_000L);
                return buildPurchaseResultObject(result);
              }
              if ("purchaseOutcome".equals(name)) {
                String productId = args != null && args.length > 0 && args[0] != null ? String.valueOf(args[0]) : "";
                PurchaseRequestPayload request = PurchaseRequestPayload.create(productId);
                PurchaseResultPayload result = callbacks.awaitPurchaseResult(request, 60_000L);
                return buildPurchaseOutcomeObject(result, productId);
              }
              if ("restore".equals(name)) {
                RestoreRequestPayload request = RestoreRequestPayload.create();
                RestoreResultPayload result = callbacks.awaitRestoreResult(request, 60_000L);
                return buildRestoreResultObject(result);
              }
              return null;
            }
          });

      Method setPurchaseDelegate = findMethod(configClass, "setPurchaseDelegate", 1);
      setPurchaseDelegate.invoke(config, delegateProxy);
      }

      return config;
    }

    private void attachDelegate(Class<?> sdkClass, RuntimeCallbacks callbacks) throws Exception {
      Class<?> delegateClass = Class.forName("io.nuxie.sdk.NuxieDelegate");
      Object proxy = Proxy.newProxyInstance(
        delegateClass.getClassLoader(),
        new Class[] { delegateClass },
        new InvocationHandler() {
          @Override
          public Object invoke(Object proxy, Method method, Object[] args) {
            String name = method.getName();
            try {
              if ("featureAccessDidChange".equals(name)) {
                String featureId = args != null && args.length > 0 && args[0] != null ? String.valueOf(args[0]) : "";
                FeatureAccessPayload from = args != null && args.length > 1 && args[1] != null
                  ? FeatureAccessPayload.fromFeatureAccess(args[1])
                  : null;
                FeatureAccessPayload to = args != null && args.length > 2 && args[2] != null
                  ? FeatureAccessPayload.fromFeatureAccess(args[2])
                  : null;
                callbacks.onFeatureAccessChanged(featureId, from, to);
              } else if ("flowDismissed".equals(name)) {
                FlowDismissedPayload payload = new FlowDismissedPayload();
                payload.journeyId = safeArg(args, 0);
                payload.campaignId = safeArg(args, 1);
                payload.screenId = safeArg(args, 2);
                payload.reason = safeArg(args, 3);
                payload.error = safeArg(args, 4);
                callbacks.onFlowDismissed(payload);
              }
            } catch (Exception ignored) {
            }
            return null;
          }
        });

      Method setDelegate = findMethod(sdkClass, "setDelegate", 1);
      setDelegate.invoke(sdk, proxy);
    }

    private Object resolveContext() throws Exception {
      Class<?> gameActivityClass = Class.forName("com.epicgames.unreal.GameActivity");
      Method getMethod = findMethod(gameActivityClass, "Get", 0);
      return getMethod.invoke(null);
    }

    private static Method findMethod(Class<?> clazz, String methodName, int parameterCount) {
      for (Method method : clazz.getMethods()) {
        if (method.getName().equals(methodName) && method.getParameterCount() == parameterCount) {
          method.setAccessible(true);
          return method;
        }
      }
      throw new IllegalStateException("Missing method: " + clazz.getName() + "." + methodName + "(" + parameterCount + ")");
    }

    private static void setOptionalField(Class<?> clazz, Object target, String propertyName, String value) {
      if (value == null || value.isEmpty()) {
        return;
      }
      try {
        Method setter = findMethod(clazz, "set" + capitalize(propertyName), 1);
        setter.invoke(target, value);
      } catch (Exception ignored) {
      }
    }

    private static void setOptionalBooleanField(Class<?> clazz, Object target, String propertyName, String value) {
      if (value == null || value.isEmpty()) {
        return;
      }

      try {
        Method setter = findMethod(clazz, "set" + capitalize(propertyName), 1);
        setter.invoke(target, Boolean.valueOf("1".equals(value) || "true".equalsIgnoreCase(value)));
      } catch (Exception ignored) {
      }
    }

    private static String capitalize(String value) {
      if (value == null || value.isEmpty()) {
        return value;
      }
      return Character.toUpperCase(value.charAt(0)) + value.substring(1);
    }

    private static Object invokeSuspend(Method suspendMethod, Object target, Object... argsWithoutContinuation) throws Exception {
      ClassLoader classLoader = suspendMethod.getDeclaringClass().getClassLoader();
      final Class<?> continuationClass = Class.forName("kotlin.coroutines.Continuation", true, classLoader);
      final Class<?> resultKtClass = Class.forName("kotlin.ResultKt", true, classLoader);
      final Method throwOnFailure = resultKtClass.getMethod("throwOnFailure", Object.class);
      final Method getSuspended = Class
        .forName("kotlin.coroutines.intrinsics.IntrinsicsKt", true, classLoader)
        .getMethod("getCOROUTINE_SUSPENDED");

      final AtomicReference<Object> resultValue = new AtomicReference<Object>();
      final AtomicReference<Throwable> errorValue = new AtomicReference<Throwable>();
      final CountDownLatch latch = new CountDownLatch(1);

      Object continuationProxy = Proxy.newProxyInstance(
        continuationClass.getClassLoader(),
        new Class[] { continuationClass },
        new InvocationHandler() {
          @Override
          public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
            if ("resumeWith".equals(method.getName())) {
              Object result = args != null && args.length > 0 ? args[0] : null;
              try {
                throwOnFailure.invoke(null, result);
                resultValue.set(result);
              } catch (InvocationTargetException invokeFailure) {
                errorValue.set(invokeFailure.getTargetException());
              } catch (Throwable throwable) {
                errorValue.set(throwable);
              }
              latch.countDown();
              return null;
            }

            if ("getContext".equals(method.getName())) {
              Class<?> emptyContext = Class.forName("kotlin.coroutines.EmptyCoroutineContext", true, classLoader);
              return emptyContext.getField("INSTANCE").get(null);
            }

            return null;
          }
        });

      Object[] params = Arrays.copyOf(argsWithoutContinuation, argsWithoutContinuation.length + 1);
      params[params.length - 1] = continuationProxy;

      Object immediate = suspendMethod.invoke(target, params);
      Object suspended = getSuspended.invoke(null);
      if (immediate != suspended) {
        return immediate;
      }

      if (!latch.await(65, TimeUnit.SECONDS)) {
        throw new TimeoutException("Coroutine timeout for " + suspendMethod.getName());
      }

      Throwable error = errorValue.get();
      if (error != null) {
        throw new RuntimeException(error);
      }

      return resultValue.get();
    }

    private static Object kotlinUnit() {
      try {
        Class<?> unitClass = Class.forName("kotlin.Unit");
        return unitClass.getField("INSTANCE").get(null);
      } catch (Exception ignored) {
        return null;
      }
    }

    private static String safeArg(Object[] args, int index) {
      if (args == null || index >= args.length || args[index] == null) {
        return "";
      }
      return String.valueOf(args[index]);
    }

    private static boolean asBoolean(Object value) {
      if (value instanceof Boolean) {
        return ((Boolean) value).booleanValue();
      }
      return "true".equalsIgnoreCase(String.valueOf(value));
    }

    private static int asInt(Object value) {
      if (value instanceof Number) {
        return ((Number) value).intValue();
      }
      try {
        return Integer.parseInt(String.valueOf(value));
      } catch (NumberFormatException ignored) {
        return 0;
      }
    }

    private static TriggerUpdatePayload mapTriggerUpdate(Object updateObj) {
      TriggerUpdatePayload payload = new TriggerUpdatePayload();
      payload.timestampMs = nowMs();
      if (updateObj == null) {
        payload.kind = "error";
        payload.errorCode = "trigger_null";
        payload.errorMessage = "Trigger update is null";
        return payload;
      }

      String wrapperName = updateObj.getClass().getSimpleName();
      if ("Decision".equals(wrapperName)) {
        payload.kind = "decision";
        Object decision = invokeGetter(updateObj, "getDecision");
        payload.decisionKind = mapDecision(decision, payload);
      } else if ("Entitlement".equals(wrapperName)) {
        payload.kind = "entitlement";
        Object entitlement = invokeGetter(updateObj, "getEntitlement");
        payload.entitlementKind = mapEntitlement(entitlement, payload);
      } else if ("Journey".equals(wrapperName)) {
        payload.kind = "journey";
        Object journey = invokeGetter(updateObj, "getJourney");
        payload.journey = JourneyPayload.fromJourney(journey);
      } else if ("Error".equals(wrapperName)) {
        payload.kind = "error";
        Object error = invokeGetter(updateObj, "getError");
        payload.errorCode = safeString(invokeGetter(error, "getCode"));
        payload.errorMessage = safeString(invokeGetter(error, "getMessage"));
      } else {
        payload.kind = "error";
        payload.errorCode = "trigger_unknown";
        payload.errorMessage = "Unknown trigger update wrapper: " + wrapperName;
      }

      return payload;
    }

    private static String mapDecision(Object decision, TriggerUpdatePayload payload) {
      if (decision == null) {
        return "unknown";
      }

      String name = decision.getClass().getSimpleName();
      if ("NoMatch".equals(name)) {
        return "no_match";
      }
      if ("Suppressed".equals(name)) {
        Object reason = invokeGetter(decision, "getReason");
        String mapped = mapSuppressReason(reason);
        payload.suppressReason = mapped;
        payload.rawSuppressReason = reason != null ? reason.toString() : "";
        return "suppressed";
      }
      if ("JourneyStarted".equals(name)) {
        payload.journeyRef = JourneyRefPayload.fromRef(invokeGetter(decision, "getRef"));
        return "journey_started";
      }
      if ("JourneyResumed".equals(name)) {
        payload.journeyRef = JourneyRefPayload.fromRef(invokeGetter(decision, "getRef"));
        return "journey_resumed";
      }
      if ("FlowShown".equals(name)) {
        payload.journeyRef = JourneyRefPayload.fromRef(invokeGetter(decision, "getRef"));
        return "flow_shown";
      }
      if ("AllowedImmediate".equals(name)) {
        return "allowed_immediate";
      }
      if ("DeniedImmediate".equals(name)) {
        return "denied_immediate";
      }

      return "unknown";
    }

    private static String mapSuppressReason(Object reason) {
      if (reason == null) {
        return "unknown";
      }

      String name = reason.getClass().getSimpleName();
      if ("AlreadyActive".equals(name)) {
        return "already_active";
      }
      if ("ReentryLimited".equals(name)) {
        return "reentry_limited";
      }
      if ("Holdout".equals(name)) {
        return "holdout";
      }
      if ("NoFlow".equals(name)) {
        return "no_flow";
      }
      return "unknown";
    }

    private static String mapEntitlement(Object entitlement, TriggerUpdatePayload payload) {
      if (entitlement == null) {
        return "pending";
      }

      String name = entitlement.getClass().getSimpleName();
      if ("Pending".equals(name)) {
        return "pending";
      }
      if ("Denied".equals(name)) {
        return "denied";
      }
      if ("Allowed".equals(name)) {
        Object source = invokeGetter(entitlement, "getSource");
        payload.gateSource = source != null ? source.toString().toLowerCase() : "cache";
        return "allowed";
      }

      return "pending";
    }

    private static Object invokeGetter(Object target, String getterName) {
      if (target == null) {
        return null;
      }

      try {
        Method getter = target.getClass().getMethod(getterName);
        getter.setAccessible(true);
        return getter.invoke(target);
      } catch (Exception ignored) {
        return null;
      }
    }

    private static String safeString(Object value) {
      return value == null ? "" : String.valueOf(value);
    }

    private static Object buildPurchaseResultObject(PurchaseResultPayload payload) throws Exception {
      if ("success".equals(payload.kind)) {
        Class<?> successClass = Class.forName("io.nuxie.sdk.purchases.PurchaseResult$Success");
        return successClass.getField("INSTANCE").get(null);
      }
      if ("cancelled".equals(payload.kind)) {
        Class<?> cancelledClass = Class.forName("io.nuxie.sdk.purchases.PurchaseResult$Cancelled");
        return cancelledClass.getField("INSTANCE").get(null);
      }
      if ("pending".equals(payload.kind)) {
        Class<?> pendingClass = Class.forName("io.nuxie.sdk.purchases.PurchaseResult$Pending");
        return pendingClass.getField("INSTANCE").get(null);
      }

      Class<?> failedClass = Class.forName("io.nuxie.sdk.purchases.PurchaseResult$Failed");
      return failedClass.getConstructor(String.class).newInstance(payload.message);
    }

    private static Object buildPurchaseOutcomeObject(PurchaseResultPayload payload, String productId) throws Exception {
      Object result = buildPurchaseResultObject(payload);
      Class<?> outcomeClass = Class.forName("io.nuxie.sdk.purchases.PurchaseOutcome");
      return outcomeClass
        .getConstructor(Class.forName("io.nuxie.sdk.purchases.PurchaseResult"), String.class, String.class, String.class)
        .newInstance(result, productId, payload.purchaseToken, payload.orderId);
    }

    private static Object buildRestoreResultObject(RestoreResultPayload payload) throws Exception {
      if ("success".equals(payload.kind)) {
        Class<?> successClass = Class.forName("io.nuxie.sdk.purchases.RestoreResult$Success");
        return successClass.getConstructor(int.class).newInstance(Integer.valueOf(payload.restoredCount));
      }

      if ("no_purchases".equals(payload.kind)) {
        Class<?> noPurchasesClass = Class.forName("io.nuxie.sdk.purchases.RestoreResult$NoPurchases");
        return noPurchasesClass.getField("INSTANCE").get(null);
      }

      Class<?> failedClass = Class.forName("io.nuxie.sdk.purchases.RestoreResult$Failed");
      return failedClass.getConstructor(String.class).newInstance(payload.message);
    }
  }

  private static final BridgeCore CORE = new BridgeCore();

  private NuxieBridge() {
  }

  public static void setNativeHandle(long nativeHandle) {
    CORE.setNativeHandle(nativeHandle);
  }

  public static void setEmitterForTesting(Emitter emitter) {
    CORE.setEmitterForTesting(emitter);
  }

  public static void setRuntimeForTesting(Runtime runtime) {
    CORE.setRuntimeForTesting(runtime);
  }

  public static void setRequestTimeoutMillisForTesting(long timeoutMillis) {
    CORE.setRequestTimeoutMillisForTesting(timeoutMillis);
  }

  public static void configure(String apiKey, String optionsPayload, boolean usePurchaseController, String wrapperVersion)
    throws Exception {
    CORE.configure(apiKey, optionsPayload, usePurchaseController, wrapperVersion);
  }

  public static void shutdown() throws Exception {
    CORE.shutdown();
  }

  public static void identify(String distinctId, String userPropertiesPayload, String userPropertiesSetOncePayload)
    throws Exception {
    CORE.identify(distinctId, userPropertiesPayload, userPropertiesSetOncePayload);
  }

  public static void reset(boolean keepAnonymousId) throws Exception {
    CORE.reset(keepAnonymousId);
  }

  public static String getDistinctId() throws Exception {
    return CORE.getDistinctId();
  }

  public static String getAnonymousId() throws Exception {
    return CORE.getAnonymousId();
  }

  public static boolean getIsIdentified() throws Exception {
    return CORE.getIsIdentified();
  }

  public static void startTrigger(String requestId, String eventName, String triggerOptionsPayload) throws Exception {
    CORE.startTrigger(requestId, eventName, triggerOptionsPayload);
  }

  public static void cancelTrigger(String requestId) throws Exception {
    CORE.cancelTrigger(requestId);
  }

  public static void showFlow(String flowId) throws Exception {
    CORE.showFlow(flowId);
  }

  public static String refreshProfile() throws Exception {
    return CORE.refreshProfile();
  }

  public static String hasFeature(String featureId, Integer requiredBalance, String entityId) throws Exception {
    return CORE.hasFeature(featureId, requiredBalance, entityId);
  }

  public static String checkFeature(String featureId, Integer requiredBalance, String entityId, boolean forceRefresh)
    throws Exception {
    return CORE.checkFeature(featureId, requiredBalance, entityId, forceRefresh);
  }

  public static void useFeature(String featureId, double amount, String entityId, String metadataPayload) throws Exception {
    CORE.useFeature(featureId, amount, entityId, metadataPayload);
  }

  public static String useFeatureAndWait(
    String featureId,
    double amount,
    String entityId,
    boolean setUsage,
    String metadataPayload)
    throws Exception {
    return CORE.useFeatureAndWait(featureId, amount, entityId, setUsage, metadataPayload);
  }

  public static boolean flushEvents() throws Exception {
    return CORE.flushEvents();
  }

  public static int getQueuedEventCount() throws Exception {
    return CORE.getQueuedEventCount();
  }

  public static void pauseEventQueue() throws Exception {
    CORE.pauseEventQueue();
  }

  public static void resumeEventQueue() throws Exception {
    CORE.resumeEventQueue();
  }

  public static void completePurchase(String requestId, String purchaseResultPayload) throws Exception {
    CORE.completePurchase(requestId, purchaseResultPayload);
  }

  public static void completeRestore(String requestId, String restoreResultPayload) throws Exception {
    CORE.completeRestore(requestId, restoreResultPayload);
  }

  private static long nowMs() {
    return System.currentTimeMillis();
  }

  private static native void nativeOnTriggerUpdate(long nativeHandle, String requestId, String payload, boolean terminal, long timestampMs);

  private static native void nativeOnFeatureAccessChanged(
    long nativeHandle,
    String featureId,
    String fromPayload,
    String toPayload,
    long timestampMs);

  private static native void nativeOnPurchaseRequest(long nativeHandle, String payload);

  private static native void nativeOnRestoreRequest(long nativeHandle, String payload);

  private static native void nativeOnFlowPresented(long nativeHandle, String flowId, long timestampMs);

  private static native void nativeOnFlowDismissed(long nativeHandle, String payload, long timestampMs);

  static final class TriggerOptions {
    final Map<String, String> properties;
    final Map<String, String> userProperties;
    final Map<String, String> userPropertiesSetOnce;

    private TriggerOptions(Map<String, String> properties, Map<String, String> userProperties, Map<String, String> userPropertiesSetOnce) {
      this.properties = properties;
      this.userProperties = userProperties;
      this.userPropertiesSetOnce = userPropertiesSetOnce;
    }

    static TriggerOptions fromPayload(String payload) {
      Map<String, String> sections = KvCodec.decodeMap(payload);
      return new TriggerOptions(
        KvCodec.decodeMap(sections.get("properties")),
        KvCodec.decodeMap(sections.get("user_properties")),
        KvCodec.decodeMap(sections.get("user_properties_set_once")));
    }
  }

  static final class TriggerUpdatePayload {
    String kind = "error";
    String decisionKind = "unknown";
    String suppressReason = "";
    String rawSuppressReason = "";
    String entitlementKind = "pending";
    String gateSource = "cache";
    JourneyRefPayload journeyRef;
    JourneyPayload journey;
    String errorCode = "";
    String errorMessage = "";
    long timestampMs = nowMs();

    boolean isTerminal() {
      if ("error".equals(kind) || "journey".equals(kind)) {
        return true;
      }
      if ("decision".equals(kind)) {
        return "no_match".equals(decisionKind)
          || "suppressed".equals(decisionKind)
          || "allowed_immediate".equals(decisionKind)
          || "denied_immediate".equals(decisionKind);
      }
      if ("entitlement".equals(kind)) {
        return "allowed".equals(entitlementKind) || "denied".equals(entitlementKind);
      }
      return false;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("kind", kind);
      out.put("decision_kind", decisionKind);
      out.put("suppress_reason", suppressReason);
      out.put("raw_suppress_reason", rawSuppressReason);
      out.put("entitlement_kind", entitlementKind);
      out.put("gate_source", gateSource);
      out.put("error_code", errorCode);
      out.put("error_message", errorMessage);
      out.put("timestamp_ms", String.valueOf(timestampMs));
      if (journeyRef != null) {
        out.put("journey_ref", journeyRef.toPayload());
      }
      if (journey != null) {
        out.put("journey", journey.toPayload());
      }
      out.put("is_terminal", isTerminal() ? "1" : "0");
      return KvCodec.encodeMap(out);
    }
  }

  static final class JourneyRefPayload {
    String journeyId = "";
    String campaignId = "";
    String flowId = "";

    static JourneyRefPayload fromRef(Object ref) {
      JourneyRefPayload payload = new JourneyRefPayload();
      if (ref == null) {
        return payload;
      }

      payload.journeyId = safeString(invokeGetter(ref, "getJourneyId"));
      payload.campaignId = safeString(invokeGetter(ref, "getCampaignId"));
      payload.flowId = safeString(invokeGetter(ref, "getFlowId"));
      return payload;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("journey_id", journeyId);
      out.put("campaign_id", campaignId);
      out.put("flow_id", flowId);
      return KvCodec.encodeMap(out);
    }
  }

  static final class JourneyPayload {
    String journeyId = "";
    String campaignId = "";
    String flowId = "";
    String exitReason = "completed";
    boolean goalMet;
    long goalMetAtEpochMillis;
    boolean hasDuration;
    double durationSeconds;
    String flowExitReason = "";

    static JourneyPayload fromJourney(Object journey) {
      JourneyPayload payload = new JourneyPayload();
      if (journey == null) {
        return payload;
      }

      payload.journeyId = safeString(invokeGetter(journey, "getJourneyId"));
      payload.campaignId = safeString(invokeGetter(journey, "getCampaignId"));
      payload.flowId = safeString(invokeGetter(journey, "getFlowId"));
      Object exit = invokeGetter(journey, "getExitReason");
      payload.exitReason = exit != null ? exit.toString().toLowerCase() : "completed";
      payload.goalMet = asBoolean(invokeGetter(journey, "getGoalMet"));
      Object goalMetAt = invokeGetter(journey, "getGoalMetAtEpochMillis");
      payload.goalMetAtEpochMillis = goalMetAt instanceof Number ? ((Number) goalMetAt).longValue() : 0L;
      Object duration = invokeGetter(journey, "getDurationSeconds");
      payload.hasDuration = duration instanceof Number;
      payload.durationSeconds = duration instanceof Number ? ((Number) duration).doubleValue() : 0.0;
      payload.flowExitReason = safeString(invokeGetter(journey, "getFlowExitReason"));
      return payload;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("journey_id", journeyId);
      out.put("campaign_id", campaignId);
      out.put("flow_id", flowId);
      out.put("exit_reason", exitReason);
      out.put("goal_met", goalMet ? "1" : "0");
      out.put("goal_met_at", String.valueOf(goalMetAtEpochMillis));
      out.put("has_duration", hasDuration ? "1" : "0");
      out.put("duration_seconds", String.valueOf(durationSeconds));
      out.put("flow_exit_reason", flowExitReason);
      return KvCodec.encodeMap(out);
    }
  }

  static final class FeatureAccessPayload {
    boolean allowed;
    boolean unlimited;
    boolean hasBalance;
    int balance;
    String type = "boolean";

    static FeatureAccessPayload fromFeatureAccess(Object access) {
      FeatureAccessPayload payload = new FeatureAccessPayload();
      if (access == null) {
        return payload;
      }

      payload.allowed = asBoolean(invokeGetter(access, "getAllowed"));
      payload.unlimited = asBoolean(invokeGetter(access, "getUnlimited"));
      Object balanceObj = invokeGetter(access, "getBalance");
      payload.hasBalance = balanceObj instanceof Number;
      payload.balance = balanceObj instanceof Number ? ((Number) balanceObj).intValue() : 0;
      Object typeObj = invokeGetter(access, "getType");
      payload.type = mapFeatureType(typeObj);
      return payload;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("allowed", allowed ? "1" : "0");
      out.put("unlimited", unlimited ? "1" : "0");
      out.put("has_balance", hasBalance ? "1" : "0");
      out.put("balance", String.valueOf(balance));
      out.put("type", type);
      return KvCodec.encodeMap(out);
    }

    private static String mapFeatureType(Object typeObj) {
      if (typeObj == null) {
        return "boolean";
      }
      String raw = typeObj.toString();
      if ("CREDIT_SYSTEM".equals(raw)) {
        return "creditSystem";
      }
      return raw.toLowerCase();
    }
  }

  static final class FeatureCheckPayload {
    String customerId = "";
    String featureId = "";
    int requiredBalance = 1;
    String code = "";
    FeatureAccessPayload access = new FeatureAccessPayload();
    String preview = "";

    static FeatureCheckPayload fromCheckResult(Object checkResult) {
      FeatureCheckPayload payload = new FeatureCheckPayload();
      if (checkResult == null) {
        return payload;
      }

      payload.customerId = safeString(invokeGetter(checkResult, "getCustomerId"));
      payload.featureId = safeString(invokeGetter(checkResult, "getFeatureId"));
      payload.requiredBalance = asInt(invokeGetter(checkResult, "getRequiredBalance"));
      payload.code = safeString(invokeGetter(checkResult, "getCode"));
      payload.access.allowed = asBoolean(invokeGetter(checkResult, "getAllowed"));
      payload.access.unlimited = asBoolean(invokeGetter(checkResult, "getUnlimited"));
      Object balanceObj = invokeGetter(checkResult, "getBalance");
      payload.access.hasBalance = balanceObj instanceof Number;
      payload.access.balance = balanceObj instanceof Number ? ((Number) balanceObj).intValue() : 0;
      payload.access.type = FeatureAccessPayload.mapFeatureType(invokeGetter(checkResult, "getType"));
      payload.preview = safeString(invokeGetter(checkResult, "getPreview"));
      return payload;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("customer_id", customerId);
      out.put("feature_id", featureId);
      out.put("required_balance", String.valueOf(requiredBalance));
      out.put("code", code);
      out.put("preview", preview);
      out.put("access", access.toPayload());
      return KvCodec.encodeMap(out);
    }
  }

  static final class FeatureUsagePayload {
    boolean success;
    String featureId = "";
    double amountUsed;
    String message = "";
    boolean hasUsage;
    double usageCurrent;
    boolean hasUsageLimit;
    double usageLimit;
    boolean hasUsageRemaining;
    double usageRemaining;

    static FeatureUsagePayload fromUsageResult(Object usageResult) {
      FeatureUsagePayload payload = new FeatureUsagePayload();
      if (usageResult == null) {
        return payload;
      }

      payload.success = asBoolean(invokeGetter(usageResult, "getSuccess"));
      payload.featureId = safeString(invokeGetter(usageResult, "getFeatureId"));
      Object amount = invokeGetter(usageResult, "getAmountUsed");
      payload.amountUsed = amount instanceof Number ? ((Number) amount).doubleValue() : 0.0;
      payload.message = safeString(invokeGetter(usageResult, "getMessage"));

      Object usage = invokeGetter(usageResult, "getUsage");
      if (usage != null) {
        payload.hasUsage = true;
        Object current = invokeGetter(usage, "getCurrent");
        Object limit = invokeGetter(usage, "getLimit");
        Object remaining = invokeGetter(usage, "getRemaining");
        payload.usageCurrent = current instanceof Number ? ((Number) current).doubleValue() : 0.0;
        payload.hasUsageLimit = limit instanceof Number;
        payload.usageLimit = limit instanceof Number ? ((Number) limit).doubleValue() : 0.0;
        payload.hasUsageRemaining = remaining instanceof Number;
        payload.usageRemaining = remaining instanceof Number ? ((Number) remaining).doubleValue() : 0.0;
      }

      return payload;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("success", success ? "1" : "0");
      out.put("feature_id", featureId);
      out.put("amount_used", String.valueOf(amountUsed));
      out.put("message", message);
      out.put("has_usage", hasUsage ? "1" : "0");
      out.put("usage_current", String.valueOf(usageCurrent));
      out.put("has_usage_limit", hasUsageLimit ? "1" : "0");
      out.put("usage_limit", String.valueOf(usageLimit));
      out.put("has_usage_remaining", hasUsageRemaining ? "1" : "0");
      out.put("usage_remaining", String.valueOf(usageRemaining));
      return KvCodec.encodeMap(out);
    }
  }

  static final class ProfilePayload {
    String customerId = "";
    String raw = "";

    static ProfilePayload fromProfile(Object profile) {
      ProfilePayload payload = new ProfilePayload();
      if (profile == null) {
        return payload;
      }
      payload.customerId = safeString(invokeGetter(profile, "getCustomerId"));
      payload.raw = String.valueOf(profile);
      return payload;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("customer_id", customerId);
      out.put("raw", raw);
      return KvCodec.encodeMap(out);
    }
  }

  static final class PurchaseRequestPayload {
    String requestId = "";
    String platform = "android";
    String productId = "";
    String basePlanId = "";
    String offerId = "";
    String displayName = "";
    String displayPrice = "";
    boolean hasPrice;
    double price;
    String currencyCode = "";
    long timestampMs;

    static PurchaseRequestPayload create(String productId) {
      PurchaseRequestPayload payload = new PurchaseRequestPayload();
      payload.requestId = UUID.randomUUID().toString();
      payload.productId = productId != null ? productId : "";
      payload.timestampMs = nowMs();
      return payload;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("request_id", requestId);
      out.put("platform", platform);
      out.put("product_id", productId);
      out.put("base_plan_id", basePlanId);
      out.put("offer_id", offerId);
      out.put("display_name", displayName);
      out.put("display_price", displayPrice);
      out.put("has_price", hasPrice ? "1" : "0");
      out.put("price", String.valueOf(price));
      out.put("currency_code", currencyCode);
      out.put("timestamp_ms", String.valueOf(timestampMs));
      return KvCodec.encodeMap(out);
    }
  }

  static final class RestoreRequestPayload {
    String requestId = "";
    String platform = "android";
    long timestampMs;

    static RestoreRequestPayload create() {
      RestoreRequestPayload payload = new RestoreRequestPayload();
      payload.requestId = UUID.randomUUID().toString();
      payload.timestampMs = nowMs();
      return payload;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("request_id", requestId);
      out.put("platform", platform);
      out.put("timestamp_ms", String.valueOf(timestampMs));
      return KvCodec.encodeMap(out);
    }
  }

  static final class PurchaseResultPayload {
    String kind = "failed";
    String productId = "";
    String purchaseToken = "";
    String orderId = "";
    String transactionId = "";
    String originalTransactionId = "";
    String transactionJws = "";
    String message = "";

    static PurchaseResultPayload failed(String message) {
      PurchaseResultPayload payload = new PurchaseResultPayload();
      payload.kind = "failed";
      payload.message = message;
      return payload;
    }

    static PurchaseResultPayload fromPayload(String payload) {
      Map<String, String> values = KvCodec.decodeMap(payload);
      PurchaseResultPayload result = new PurchaseResultPayload();
      result.kind = values.getOrDefault("kind", "failed");
      result.productId = values.getOrDefault("product_id", "");
      result.purchaseToken = values.getOrDefault("purchase_token", "");
      result.orderId = values.getOrDefault("order_id", "");
      result.transactionId = values.getOrDefault("transaction_id", "");
      result.originalTransactionId = values.getOrDefault("original_transaction_id", "");
      result.transactionJws = values.getOrDefault("transaction_jws", "");
      result.message = values.getOrDefault("message", "");
      return result;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("kind", kind);
      out.put("product_id", productId);
      out.put("purchase_token", purchaseToken);
      out.put("order_id", orderId);
      out.put("transaction_id", transactionId);
      out.put("original_transaction_id", originalTransactionId);
      out.put("transaction_jws", transactionJws);
      out.put("message", message);
      return KvCodec.encodeMap(out);
    }
  }

  static final class RestoreResultPayload {
    String kind = "failed";
    int restoredCount;
    String message = "";

    static RestoreResultPayload failed(String message) {
      RestoreResultPayload payload = new RestoreResultPayload();
      payload.kind = "failed";
      payload.message = message;
      return payload;
    }

    static RestoreResultPayload fromPayload(String payload) {
      Map<String, String> values = KvCodec.decodeMap(payload);
      RestoreResultPayload result = new RestoreResultPayload();
      result.kind = values.getOrDefault("kind", "failed");
      result.restoredCount = asInt(values.get("restored_count"));
      result.message = values.getOrDefault("message", "");
      return result;
    }

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("kind", kind);
      out.put("restored_count", String.valueOf(restoredCount));
      out.put("message", message);
      return KvCodec.encodeMap(out);
    }
  }

  static final class FlowDismissedPayload {
    String flowId = "";
    String reason = "";
    String journeyId = "";
    String campaignId = "";
    String screenId = "";
    String error = "";

    String toPayload() {
      Map<String, String> out = new LinkedHashMap<String, String>();
      out.put("flow_id", flowId);
      out.put("reason", reason);
      out.put("journey_id", journeyId);
      out.put("campaign_id", campaignId);
      out.put("screen_id", screenId);
      out.put("error", error);
      return KvCodec.encodeMap(out);
    }
  }

  static final class KvCodec {
    private KvCodec() {
    }

    static String encodeMap(Map<String, String> values) {
      if (values == null || values.isEmpty()) {
        return "";
      }

      StringBuilder builder = new StringBuilder();
      boolean first = true;
      for (Map.Entry<String, String> entry : values.entrySet()) {
        if (!first) {
          builder.append('&');
        }
        builder.append(encode(entry.getKey()));
        builder.append('=');
        builder.append(encode(entry.getValue() == null ? "" : entry.getValue()));
        first = false;
      }
      return builder.toString();
    }

    static Map<String, String> decodeMap(String encoded) {
      if (encoded == null || encoded.isEmpty()) {
        return new LinkedHashMap<String, String>();
      }

      Map<String, String> out = new LinkedHashMap<String, String>();
      String[] pairs = encoded.split("&");
      for (String pair : pairs) {
        if (pair == null || pair.isEmpty()) {
          continue;
        }
        int sep = pair.indexOf('=');
        if (sep < 0) {
          out.put(decode(pair), "");
          continue;
        }

        String key = decode(pair.substring(0, sep));
        String value = decode(pair.substring(sep + 1));
        out.put(key, value);
      }

      return out;
    }

    private static String encode(String value) {
      return URLEncoder.encode(value == null ? "" : value, StandardCharsets.UTF_8);
    }

    private static String decode(String value) {
      return URLDecoder.decode(value == null ? "" : value, StandardCharsets.UTF_8);
    }
  }

  private static Object invokeGetter(Object target, String getterName) {
    if (target == null) {
      return null;
    }

    try {
      Method getter = target.getClass().getMethod(getterName);
      getter.setAccessible(true);
      return getter.invoke(target);
    } catch (Exception ignored) {
      return null;
    }
  }

  private static boolean asBoolean(Object value) {
    if (value instanceof Boolean) {
      return ((Boolean) value).booleanValue();
    }
    if (value instanceof Number) {
      return ((Number) value).intValue() != 0;
    }
    return "1".equals(String.valueOf(value)) || "true".equalsIgnoreCase(String.valueOf(value));
  }

  private static int asInt(Object value) {
    if (value instanceof Number) {
      return ((Number) value).intValue();
    }
    try {
      return Integer.parseInt(String.valueOf(value));
    } catch (Exception ignored) {
      return 0;
    }
  }

  private static String safeString(Object value) {
    return value == null ? "" : String.valueOf(value);
  }
}
