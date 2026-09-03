package ai.nuxie.unreal

import ai.nuxie.sdk.AppAction
import ai.nuxie.sdk.AppActionValue
import ai.nuxie.sdk.LogLevel
import ai.nuxie.sdk.Nuxie
import ai.nuxie.sdk.NuxieActivityInfo
import ai.nuxie.sdk.NuxieActivityValue
import ai.nuxie.sdk.NuxieConfiguration
import ai.nuxie.sdk.NuxieEnvironment
import ai.nuxie.sdk.NuxieListener
import ai.nuxie.sdk.billing.NuxiePurchaseDelegate
import ai.nuxie.sdk.billing.PurchaseHandlingMode
import ai.nuxie.sdk.billing.PurchaseResult
import ai.nuxie.sdk.billing.RestoreResult
import ai.nuxie.sdk.billing.StoreProduct
import ai.nuxie.sdk.features.FeatureAccess
import ai.nuxie.sdk.features.FeatureCheckPolicy
import ai.nuxie.sdk.features.FeatureType
import ai.nuxie.sdk.features.FeatureUsageResult
import android.app.Activity
import java.util.UUID
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentLinkedQueue
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeoutOrNull
import org.json.JSONArray
import org.json.JSONObject

object NuxieUnrealBridge {
  private val pendingEvents = ConcurrentLinkedQueue<String>()
  private val purchaseDelegate = UnrealPurchaseDelegate(::emit)
  private val listener = object : NuxieListener {
    override fun featureAccessDidChange(
      featureId: String,
      oldAccess: FeatureAccess?,
      newAccess: FeatureAccess,
    ) {
      emit(
        "feature_access_changed",
        mapOf(
          "featureId" to featureId,
          "from" to oldAccess?.toBridgeMap(),
          "to" to newAccess.toBridgeMap(),
        ),
      )
    }

    override fun onActivityEmitted(sdk: Nuxie, info: NuxieActivityInfo) {
      emit("activity", info.toMap())
    }

    override fun onAppActionRequested(sdk: Nuxie, action: AppAction) {
      emit("app_action", action.toMap())
    }
  }

  @JvmStatic
  fun invoke(
    activity: Activity?,
    method: String,
    argumentsJson: String,
  ): String {
    return runCatching {
      val arguments = jsonObjectToMap(JSONObject(argumentsJson))
      when (method) {
        "configure" -> configure(activity, arguments)
        "shutdown" -> {
          purchaseDelegate.cancelPending("sdk_shutdown")
          runBlocking { Nuxie.shutdown() }
          if (Nuxie.listener === listener) {
            Nuxie.listener = null
          }
          pendingEvents.clear()
          okResponse(null)
        }

        "identify" -> {
          val distinctId = arguments["distinctId"] as? String
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "identify requires distinctId.",
            )
          Nuxie.identify(
            distinctId = distinctId,
            userProperties = decodeScalarMap(arguments["userProperties"]),
            userPropertiesSetOnce = decodeScalarMap(arguments["userPropertiesSetOnce"]),
          )
          okResponse(null)
        }

        "reset" -> {
          Nuxie.reset(keepAnonymousId = arguments["keepAnonymousId"] as? Boolean ?: false)
          okResponse(null)
        }

        "getDistinctId" -> okResponse(Nuxie.distinctId)
        "getAnonymousId" -> okResponse(Nuxie.anonymousId)
        "getIsIdentified" -> okResponse(Nuxie.isIdentified)

        "trigger" -> {
          val eventName = arguments["eventName"] as? String
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "trigger requires eventName.",
            )
          Nuxie.trigger(eventName, decodeScalarMap(arguments["properties"]))
          okResponse(null)
        }

        "dismiss" -> {
          runBlocking { Nuxie.dismiss() }
          okResponse(null)
        }

        "setLocaleIdentifier" -> {
          runBlocking {
            Nuxie.setLocaleIdentifier(
              (arguments["localeIdentifier"] as? String)?.ifBlank { null },
            )
          }
          okResponse(null)
        }

        "hasFeature" -> {
          val featureId = arguments["featureId"] as? String
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "hasFeature requires featureId.",
            )
          val access = runBlocking {
            Nuxie.hasFeature(
              featureId = featureId,
              requiredBalance = (arguments["requiredBalance"] as? Number)?.toDouble() ?: 1.0,
              entityId = (arguments["entityId"] as? String)?.ifBlank { null },
              policy = if (arguments["policy"] == "remote") {
                FeatureCheckPolicy.REMOTE
              } else {
                FeatureCheckPolicy.CACHE_FIRST
              },
            )
          }
          okResponse(access.toBridgeMap())
        }

        "useFeature" -> {
          val featureId = arguments["featureId"] as? String
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "useFeature requires featureId.",
            )
          Nuxie.useFeature(
            featureId = featureId,
            amount = (arguments["amount"] as? Number)?.toDouble() ?: 1.0,
            entityId = (arguments["entityId"] as? String)?.ifBlank { null },
            metadata = decodeScalarMap(arguments["metadata"]),
          )
          okResponse(null)
        }

        "useFeatureAndWait" -> {
          val featureId = arguments["featureId"] as? String
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "useFeatureAndWait requires featureId.",
            )
          val result = runBlocking {
            Nuxie.useFeatureAndWait(
              featureId = featureId,
              amount = (arguments["amount"] as? Number)?.toDouble() ?: 1.0,
              entityId = (arguments["entityId"] as? String)?.ifBlank { null },
              setUsage = arguments["setUsage"] as? Boolean ?: false,
              metadata = decodeScalarMap(arguments["metadata"]),
            )
          }
          okResponse(result.toBridgeMap())
        }

        "completePurchase" -> {
          val requestId = arguments["requestId"] as? String
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "completePurchase requires requestId.",
            )
          val result = arguments["result"].asStringAnyMap()
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "completePurchase requires result.",
            )
          purchaseDelegate.completePurchase(requestId, result)
          okResponse(null)
        }

        "completeRestore" -> {
          val requestId = arguments["requestId"] as? String
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "completeRestore requires requestId.",
            )
          val result = arguments["result"].asStringAnyMap()
            ?: return@runCatching errorResponse(
              "INVALID_ARGUMENTS",
              "completeRestore requires result.",
            )
          purchaseDelegate.completeRestore(requestId, result)
          okResponse(null)
        }

        else -> errorResponse("NATIVE_ERROR", "Unsupported method '$method'.")
      }
    }.getOrElse { error ->
      errorResponse(
        "NATIVE_ERROR",
        error.message ?: "Native bridge failure.",
        error.stackTraceToString(),
      )
    }
  }

  @JvmStatic
  fun popPendingEvent(): String? = pendingEvents.poll()

  private fun configure(
    activity: Activity?,
    arguments: Map<String, Any?>,
  ): String {
    val apiKey = arguments["apiKey"] as? String
      ?: return errorResponse("MISSING_API_KEY", "Nuxie API key is required.")
    if (apiKey.isBlank()) {
      return errorResponse("MISSING_API_KEY", "Nuxie API key is required.")
    }

    val context = activity?.applicationContext
      ?: return errorResponse("NO_CONTEXT", "Unreal activity is unavailable.")
    val options = arguments["options"].asStringAnyMap()
    val usePurchaseController = arguments["usingPurchaseController"] as? Boolean ?: false

    pendingEvents.clear()
    Nuxie.listener = listener
    try {
      Nuxie.setup(
        context,
        NuxieConfiguration(apiKey).apply {
        environment = if (options?.get("environment") == "development") {
          NuxieEnvironment.DEVELOPMENT
        } else {
          NuxieEnvironment.PRODUCTION
        }
        logLevel = when (options?.get("logLevel") as? String) {
          "verbose", "debug" -> LogLevel.DEBUG
          "info" -> LogLevel.INFO
          "error" -> LogLevel.ERROR
          "none" -> LogLevel.NONE
          else -> LogLevel.WARN
        }
        if (options?.containsKey("localeIdentifier") == true) {
          localeIdentifier = options["localeIdentifier"] as? String
        }
        purchaseHandlingMode = if (options?.get("purchaseHandlingMode") == "observer") {
          PurchaseHandlingMode.APP_MANAGED
        } else {
          PurchaseHandlingMode.NUXIE_MANAGED
        }
        if (usePurchaseController) {
          purchaseDelegate = this@NuxieUnrealBridge.purchaseDelegate
        }
        },
      )
    } catch (error: Throwable) {
      if (!Nuxie.isSetup && Nuxie.listener === listener) {
        Nuxie.listener = null
      }
      throw error
    }
    return okResponse(null)
  }

  private fun emit(type: String, payload: Map<String, Any?>) {
    val message = JSONObject(
      mapOf(
        "type" to type,
        "timestampMs" to System.currentTimeMillis(),
        "payload" to payload,
      ),
    ).toString()

    pendingEvents.add(message)
  }

  private fun okResponse(value: Any?): String = JSONObject()
    .put("ok", true)
    .put("value", value ?: JSONObject.NULL)
    .toString()

  private fun errorResponse(
    code: String,
    message: String,
    nativeStack: String? = null,
  ): String {
    val error = JSONObject()
      .put("code", code)
      .put("message", message)
    if (nativeStack != null) {
      error.put("nativeStack", nativeStack)
    }
    return JSONObject()
      .put("ok", false)
      .put("error", error)
      .toString()
  }
}

internal class UnrealPurchaseDelegate(
  private val emit: (String, Map<String, Any?>) -> Unit,
  private val timeoutMs: Long = 60_000,
) : NuxiePurchaseDelegate {
  private val purchases = ConcurrentHashMap<String, CompletableDeferred<PurchaseResult>>()
  private val restores = ConcurrentHashMap<String, CompletableDeferred<RestoreResult>>()

  override suspend fun purchase(product: StoreProduct): PurchaseResult {
    val requestId = UUID.randomUUID().toString()
    val deferred = CompletableDeferred<PurchaseResult>()
    purchases[requestId] = deferred

    emit(
      "purchase_request",
      mapOf(
        "request_id" to requestId,
        "platform" to "android",
        "product_id" to product.productId,
        "store_product_id" to product.storeProductId,
        "base_plan_id" to product.basePlanId,
        "purchase_option_id" to product.purchaseOptionId,
        "offer_id" to product.offerId,
        "placement_id" to product.placementId,
        "display_name" to product.rawProduct?.name,
        "display_price" to null,
        "timestamp_ms" to System.currentTimeMillis(),
      ),
    )

    return try {
      withTimeoutOrNull(timeoutMs) { deferred.await() }
        ?: PurchaseResult.Failed(bridgeError("purchase_timeout"))
    } finally {
      purchases.remove(requestId)
    }
  }

  override suspend fun restorePurchases(): RestoreResult {
    val requestId = UUID.randomUUID().toString()
    val deferred = CompletableDeferred<RestoreResult>()
    restores[requestId] = deferred

    emit(
      "restore_request",
      mapOf(
        "request_id" to requestId,
        "platform" to "android",
        "timestamp_ms" to System.currentTimeMillis(),
      ),
    )

    return try {
      withTimeoutOrNull(timeoutMs) { deferred.await() }
        ?: RestoreResult.Failed(bridgeError("restore_timeout"))
    } finally {
      restores.remove(requestId)
    }
  }

  fun completePurchase(requestId: String, payload: Map<String, Any?>) {
    purchases.remove(requestId)?.complete(
      when ((payload["type"] as? String)?.lowercase()) {
        "purchased" -> PurchaseResult.Purchased
        "cancelled" -> PurchaseResult.Cancelled
        "pending" -> PurchaseResult.Pending
        else -> PurchaseResult.Failed(
          bridgeError((payload["message"] as? String) ?: "purchase_failed"),
        )
      },
    )
  }

  fun completeRestore(requestId: String, payload: Map<String, Any?>) {
    restores.remove(requestId)?.complete(
      when ((payload["type"] as? String)?.lowercase()) {
        "restored" -> RestoreResult.Restored
        "no_purchases" -> RestoreResult.NoPurchases
        else -> RestoreResult.Failed(
          bridgeError((payload["message"] as? String) ?: "restore_failed"),
        )
      },
    )
  }

  fun cancelPending(reason: String) {
    purchases.values.forEach { it.complete(PurchaseResult.Failed(bridgeError(reason))) }
    restores.values.forEach { it.complete(RestoreResult.Failed(bridgeError(reason))) }
    purchases.clear()
    restores.clear()
  }

  private fun bridgeError(message: String): Throwable = IllegalStateException(message)
}

private fun FeatureType.toBridgeValue(): String = when (this) {
  FeatureType.BOOLEAN -> "boolean"
  FeatureType.METERED -> "metered"
  FeatureType.CREDIT_SYSTEM -> "creditSystem"
}

internal fun FeatureAccess.toBridgeMap(): Map<String, Any?> = mapOf(
  "allowed" to allowed,
  "unlimited" to unlimited,
  "balance" to balance,
  "type" to type.toBridgeValue(),
)

internal fun FeatureUsageResult.toBridgeMap(): Map<String, Any?> = mapOf(
  "success" to success,
  "featureId" to featureId,
  "amountUsed" to amountUsed,
  "message" to message,
  "usage" to usage?.let { value ->
    mapOf(
      "current" to value.current,
      "limit" to value.limit,
      "remaining" to value.remaining,
    )
  },
  "authoritativeAccess" to authoritativeAccess?.toBridgeMap(),
)

private fun NuxieActivityInfo.toMap(): Map<String, Any?> = mapOf(
  "schemaVersion" to NuxieActivityInfo.SCHEMA_VERSION,
  "id" to id,
  "timestampMs" to timestampMillis,
  "receivedAtMs" to receivedAtMillis,
  "name" to name,
  "properties" to properties.mapValues { (_, value) -> value.toTaggedScalar() },
)

internal fun NuxieActivityValue.toTaggedScalar(): Map<String, Any> = when (this) {
  is NuxieActivityValue.String -> mapOf("type" to "string", "value" to value)
  is NuxieActivityValue.Int -> mapOf("type" to "integer", "value" to value.toString())
  is NuxieActivityValue.Double -> mapOf("type" to "number", "value" to value.toWireBits())
  is NuxieActivityValue.Bool -> mapOf("type" to "boolean", "value" to value)
}

private fun AppAction.toMap(): Map<String, Any?> = mapOf(
  "name" to name,
  "payload" to payload?.mapValues { (_, value) -> value.toTaggedScalar() },
  "experience" to mapOf(
    "experienceId" to experience.experienceId,
    "experienceVersion" to experience.experienceVersion,
    "journeyId" to experience.journeyId,
  ),
)

private fun AppActionValue.toTaggedScalar(): Map<String, Any> = when (this) {
  is AppActionValue.String -> mapOf("type" to "string", "value" to value)
  is AppActionValue.Int -> mapOf("type" to "integer", "value" to value.toString())
  is AppActionValue.Double -> mapOf("type" to "number", "value" to value.toWireBits())
  is AppActionValue.Bool -> mapOf("type" to "boolean", "value" to value)
}

private fun Double.toWireBits(): String =
  toRawBits().toULong().toString(16).padStart(16, '0')

private fun jsonObjectToMap(value: JSONObject): Map<String, Any?> {
  val result = mutableMapOf<String, Any?>()
  val keys = value.keys()
  while (keys.hasNext()) {
    val key = keys.next()
    result[key] = jsonValue(value.get(key))
  }
  return result
}

private fun jsonArrayToList(value: JSONArray): List<Any?> =
  (0 until value.length()).map { index -> jsonValue(value.get(index)) }

private fun jsonValue(value: Any?): Any? = when (value) {
  null, JSONObject.NULL -> null
  is JSONObject -> jsonObjectToMap(value)
  is JSONArray -> jsonArrayToList(value)
  else -> value
}

@Suppress("UNCHECKED_CAST")
private fun Any?.asStringAnyMap(): Map<String, Any?>? = this as? Map<String, Any?>

internal fun decodeScalarMap(raw: Any?): Map<String, Any?> {
  val encoded = raw.asStringAnyMap()
    ?: throw IllegalArgumentException("Scalar map must be an object.")
  return encoded.mapValues { (key, rawValue) ->
    val value = rawValue.asStringAnyMap()
      ?: throw IllegalArgumentException("Scalar '$key' must be a tagged object.")
    when (value["type"] as? String) {
      "string" -> value["value"] as? String
        ?: throw IllegalArgumentException("Scalar '$key' must contain a string value.")
      "integer" -> (value["value"] as? String)?.toLongOrNull()
        ?: throw IllegalArgumentException("Scalar '$key' must contain an int64 string.")
      "number" -> (value["value"] as? String)
        ?.takeIf { it.length == 16 }
        ?.toULongOrNull(16)
        ?.toLong()
        ?.let(Double::fromBits)
        ?: throw IllegalArgumentException("Scalar '$key' must contain 64-bit number bits.")
      "boolean" -> value["value"] as? Boolean
        ?: throw IllegalArgumentException("Scalar '$key' must contain a Boolean value.")
      else -> throw IllegalArgumentException("Scalar '$key' has an unsupported type.")
    }
  }
}
