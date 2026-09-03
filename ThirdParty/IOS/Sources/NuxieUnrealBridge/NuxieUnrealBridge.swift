import Foundation

#if canImport(Nuxie)
import Nuxie
#endif

@_cdecl("NuxieUnreal_Invoke")
public func NuxieUnreal_Invoke(
  _ methodPointer: UnsafePointer<CChar>?,
  _ argumentsPointer: UnsafePointer<CChar>?
) -> UnsafeMutablePointer<CChar>? {
  let method = methodPointer.map(String.init(cString:)) ?? ""
  let arguments = argumentsPointer.map(String.init(cString:)) ?? "{}"
  return strdup(NuxieUnrealBridge.shared.invoke(
    method: method,
    argumentsJSON: arguments
  ))
}

@_cdecl("NuxieUnreal_PopPendingEvent")
public func NuxieUnreal_PopPendingEvent() -> UnsafeMutablePointer<CChar>? {
  guard let event = NuxieUnrealBridge.shared.popPendingEvent() else {
    return nil
  }
  return strdup(event)
}

@_cdecl("NuxieUnreal_FreeCString")
public func NuxieUnreal_FreeCString(_ pointer: UnsafeMutablePointer<CChar>?) {
  free(pointer)
}

private final class NuxieUnrealBridge: @unchecked Sendable {
  static let shared = NuxieUnrealBridge()

  private let eventLock = NSLock()
  private var pendingEvents: [String] = []

  #if canImport(Nuxie)
  private lazy var purchaseDelegate = NuxieUnrealPurchaseDelegateBridge(emit: emit)
  @MainActor private lazy var delegate = NuxieUnrealDelegateBridge(emit: emit)
  #endif

  func invoke(
    method: String,
    argumentsJSON: String
  ) -> String {
    guard let arguments = parseObject(argumentsJSON) else {
      return errorResponse(code: "INVALID_ARGUMENTS", message: "Arguments must be a JSON object.")
    }

    #if canImport(Nuxie)
    switch method {
    case "configure":
      guard let apiKey = arguments["apiKey"] as? String,
            !apiKey.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty
      else {
        return errorResponse(code: "MISSING_API_KEY", message: "Nuxie API key is required.")
      }
      let options = UnsafeSendable(arguments["options"] as? [String: Any])
      let usePurchaseController = arguments["usingPurchaseController"] as? Bool ?? false
      eventLock.withLock {
        pendingEvents.removeAll()
      }
      return mapResult(runOnMainActorBlocking {
          let configuration = self.makeConfiguration(
            apiKey: apiKey,
            options: options.value,
            usePurchaseController: usePurchaseController
          )
          NuxieSDK.shared.delegate = self.delegate
          do {
            try NuxieSDK.shared.setup(with: configuration)
            return true
          } catch {
            NuxieSDK.shared.delegate = nil
            throw error
          }
      }) { _ in nil }

    case "shutdown":
      purchaseDelegate.cancelPending(reason: "sdk_shutdown")
      let response = mapResult(blockOnAsyncOperation {
        await NuxieSDK.shared.shutdown()
        NuxieSDK.shared.delegate = nil
        return true
      }) { _ in nil }
      eventLock.withLock {
        pendingEvents.removeAll()
      }
      return response

    case "identify":
      guard let distinctId = arguments["distinctId"] as? String else {
        return errorResponse(code: "INVALID_ARGUMENTS", message: "identify requires distinctId.")
      }
      guard let userProperties = decodeScalarMap(arguments["userProperties"]),
            let userPropertiesSetOnce = decodeScalarMap(arguments["userPropertiesSetOnce"])
      else {
        return invalidScalarMapResponse()
      }
      NuxieSDK.shared.identify(
        distinctId,
        userProperties: userProperties,
        userPropertiesSetOnce: userPropertiesSetOnce
      )
      return okResponse(nil)

    case "reset":
      NuxieSDK.shared.reset(keepAnonymousId: arguments["keepAnonymousId"] as? Bool ?? false)
      return okResponse(nil)

    case "getDistinctId":
      return okResponse(NuxieSDK.shared.getDistinctId())

    case "getAnonymousId":
      return okResponse(NuxieSDK.shared.getAnonymousId())

    case "getIsIdentified":
      return okResponse(NuxieSDK.shared.isIdentified)

    case "trigger":
      guard let eventName = arguments["eventName"] as? String else {
        return errorResponse(code: "INVALID_ARGUMENTS", message: "trigger requires eventName.")
      }
      guard let properties = decodeScalarMap(arguments["properties"]) else {
        return invalidScalarMapResponse()
      }
      NuxieSDK.shared.trigger(
        eventName,
        properties: properties
      )
      return okResponse(nil)

    case "dismiss":
      return mapResult(blockOnAsyncOperation {
        await NuxieSDK.shared.dismiss()
        return true
      }) { _ in nil }

    case "setLocaleIdentifier":
      let localeIdentifier = normalizedOptionalString(
        arguments["localeIdentifier"] as? String
      )
      return mapResult(blockOnAsyncOperation {
        try await NuxieSDK.shared.setLocaleIdentifier(localeIdentifier)
        return true
      }) { _ in nil }

    case "hasFeature":
      guard let featureId = arguments["featureId"] as? String else {
        return errorResponse(code: "INVALID_ARGUMENTS", message: "hasFeature requires featureId.")
      }
      let requiredBalance = number(arguments["requiredBalance"]) ?? 1
      let entityId = normalizedOptionalString(
        arguments["entityId"] as? String
      )
      let useRemotePolicy = arguments["policy"] as? String == "remote"
      return mapResult(blockOnAsyncOperation {
        let access = try await NuxieSDK.shared.hasFeature(
          featureId,
          requiredBalance: requiredBalance,
          entityId: entityId,
          policy: useRemotePolicy ? .remote : .cacheFirst
        )
        return self.featureAccessDictionary(access)
      })

    case "useFeature":
      guard let featureId = arguments["featureId"] as? String else {
        return errorResponse(code: "INVALID_ARGUMENTS", message: "useFeature requires featureId.")
      }
      guard let metadata = decodeScalarMap(arguments["metadata"]) else {
        return invalidScalarMapResponse()
      }
      NuxieSDK.shared.useFeature(
        featureId,
        amount: number(arguments["amount"]) ?? 1,
        entityId: normalizedOptionalString(
          arguments["entityId"] as? String
        ),
        metadata: metadata
      )
      return okResponse(nil)

    case "useFeatureAndWait":
      guard let featureId = arguments["featureId"] as? String else {
        return errorResponse(code: "INVALID_ARGUMENTS", message: "useFeatureAndWait requires featureId.")
      }
      let amount = number(arguments["amount"]) ?? 1
      let entityId = normalizedOptionalString(
        arguments["entityId"] as? String
      )
      let setUsage = arguments["setUsage"] as? Bool ?? false
      guard let decodedMetadata = decodeScalarMap(arguments["metadata"]) else {
        return invalidScalarMapResponse()
      }
      let metadata = UnsafeSendable(decodedMetadata)
      return mapResult(blockOnAsyncOperation {
        let result = try await NuxieSDK.shared.useFeatureAndWait(
          featureId,
          amount: amount,
          entityId: entityId,
          setUsage: setUsage,
          metadata: metadata.value
        )
        return self.featureUsageResultDictionary(result)
      })

    case "completePurchase":
      guard let requestId = arguments["requestId"] as? String,
            let payload = arguments["result"] as? [String: Any]
      else {
        return errorResponse(
          code: "INVALID_ARGUMENTS",
          message: "completePurchase requires requestId and result."
        )
      }
      purchaseDelegate.completePurchase(requestId: requestId, payload: payload)
      return okResponse(nil)

    case "completeRestore":
      guard let requestId = arguments["requestId"] as? String,
            let payload = arguments["result"] as? [String: Any]
      else {
        return errorResponse(
          code: "INVALID_ARGUMENTS",
          message: "completeRestore requires requestId and result."
        )
      }
      purchaseDelegate.completeRestore(requestId: requestId, payload: payload)
      return okResponse(nil)

    default:
      return errorResponse(code: "NATIVE_ERROR", message: "Unsupported method '\(method)'.")
    }
    #else
    return errorResponse(code: "NATIVE_UNAVAILABLE", message: "The Nuxie iOS SDK is not linked.")
    #endif
  }

  func popPendingEvent() -> String? {
    eventLock.withLock {
      guard !pendingEvents.isEmpty else {
        return nil
      }
      return pendingEvents.removeFirst()
    }
  }

  private func parseObject(_ raw: String) -> [String: Any]? {
    guard let data = raw.data(using: .utf8),
          let value = try? JSONSerialization.jsonObject(with: data)
    else {
      return nil
    }
    return value as? [String: Any]
  }

  private func number(_ value: Any?) -> Double? {
    (value as? NSNumber)?.doubleValue
  }

  private func normalizedOptionalString(_ value: String?) -> String? {
    guard let value = value?.trimmingCharacters(in: .whitespacesAndNewlines),
          !value.isEmpty
    else {
      return nil
    }
    return value
  }

  private func invalidScalarMapResponse() -> String {
    errorResponse(
      code: "INVALID_ARGUMENTS",
      message: "Scalar maps must use the tagged Nuxie Unreal wire format."
    )
  }

  #if canImport(Nuxie)
  @MainActor
  private func makeConfiguration(
    apiKey: String,
    options: [String: Any]?,
    usePurchaseController: Bool
  ) -> NuxieConfiguration {
    let configuration = NuxieConfiguration(apiKey: apiKey)

    if options?["environment"] as? String == "development" {
      configuration.environment = .development
    } else if options?["environment"] != nil {
      configuration.environment = .production
    }

    if let logLevel = options?["logLevel"] as? String {
      switch logLevel {
      case "verbose": configuration.logLevel = .verbose
      case "debug": configuration.logLevel = .debug
      case "info": configuration.logLevel = .info
      case "error": configuration.logLevel = .error
      case "none": configuration.logLevel = .none
      default: configuration.logLevel = .warning
      }
    }

    if let value = options?["enableConsoleLogging"] as? Bool {
      configuration.enableConsoleLogging = value
    }
    if let value = options?["redactSensitiveData"] as? Bool {
      configuration.redactSensitiveData = value
    }
    if options?.keys.contains("localeIdentifier") == true {
      configuration.localeIdentifier = normalizedOptionalString(
        options?["localeIdentifier"] as? String
      )
    }
    if let value = options?["purchaseHandlingMode"] as? String {
      configuration.purchaseHandlingMode = value == "observer" ? .observer : .full
    }
    if let value = options?["testStoreEnabled"] as? Bool {
      configuration.testStoreEnabled = value
    }
    if usePurchaseController {
      configuration.purchaseDelegate = purchaseDelegate
    }

    return configuration
  }

  private func featureAccessDictionary(_ access: FeatureAccess) -> [String: Any] {
    [
      "allowed": access.allowed,
      "unlimited": access.unlimited,
      "balance": nullable(access.balance),
      "type": access.type.rawValue,
    ]
  }

  private func featureUsageResultDictionary(_ result: FeatureUsageResult) -> [String: Any] {
    let usage: Any
    if let value = result.usage {
      usage = [
        "current": value.current,
        "limit": nullable(value.limit),
        "remaining": nullable(value.remaining),
      ]
    } else {
      usage = NSNull()
    }

    return [
      "success": result.success,
      "featureId": result.featureId,
      "amountUsed": result.amountUsed,
      "message": nullable(result.message),
      "usage": usage,
      "authoritativeAccess": nullable(result.authoritativeAccess.map(featureAccessDictionary)),
    ]
  }
  #endif

  private func mapResult<T>(
    _ result: Result<T, Error>,
    transform: (T) -> Any? = { $0 }
  ) -> String {
    switch result {
    case .success(let value):
      return okResponse(transform(value))
    case .failure(let error):
      return errorResponse(
        code: "NATIVE_ERROR",
        message: error.localizedDescription,
        nativeStack: String(describing: error)
      )
    }
  }

  private func okResponse(_ value: Any?) -> String {
    serialize(["ok": true, "value": nullable(value)])
  }

  private func errorResponse(
    code: String,
    message: String,
    nativeStack: String? = nil
  ) -> String {
    var error: [String: Any] = ["code": code, "message": message]
    if let nativeStack {
      error["nativeStack"] = nativeStack
    }
    return serialize(["ok": false, "error": error])
  }

  private func serialize(_ value: Any) -> String {
    guard JSONSerialization.isValidJSONObject(value),
          let data = try? JSONSerialization.data(withJSONObject: value),
          let string = String(data: data, encoding: .utf8)
    else {
      return "{\"ok\":false,\"error\":{\"code\":\"NATIVE_ERROR\",\"message\":\"JSON serialization failed.\"}}"
    }
    return string
  }

  private func emit(_ type: String, _ payload: [String: Any]) {
    let message = serialize([
      "type": type,
      "timestampMs": Int(Date().timeIntervalSince1970 * 1_000),
      "payload": payload,
    ])
    eventLock.withLock {
      pendingEvents.append(message)
    }
  }
}

private final class UnsafeSendable<Value>: @unchecked Sendable {
  let value: Value

  init(_ value: Value) {
    self.value = value
  }
}

private final class BlockingResultBox<Value>: @unchecked Sendable {
  private let lock = NSLock()
  private var value: Result<Value, Error>?

  func store(_ value: Result<Value, Error>) {
    lock.withLock {
      self.value = value
    }
  }

  func load() -> Result<Value, Error>? {
    lock.withLock { value }
  }
}

private func nullable(_ value: Any?) -> Any {
  value ?? NSNull()
}

func blockOnAsyncOperation<T>(
  _ operation: @escaping @Sendable () async throws -> T
) -> Result<T, Error> {
  let semaphore = DispatchSemaphore(value: 0)
  let box = BlockingResultBox<T>()
  Task.detached {
    do {
      box.store(.success(try await operation()))
    } catch {
      box.store(.failure(error))
    }
    semaphore.signal()
  }
  semaphore.wait()
  return box.load() ?? .failure(
    NSError(
      domain: "ai.nuxie.unreal",
      code: -1,
      userInfo: [NSLocalizedDescriptionKey: "Native operation did not produce a result."]
    )
  )
}

func runOnMainActorBlocking<T: Sendable>(
  _ operation: @escaping @MainActor @Sendable () throws -> T
) -> Result<T, Error> {
  if Thread.isMainThread {
    return Result {
      try MainActor.assumeIsolated {
        try operation()
      }
    }
  }

  return blockOnAsyncOperation {
    try await MainActor.run {
      try operation()
    }
  }
}

func decodeScalarMap(_ raw: Any?) -> [String: Any]? {
  guard let encoded = raw as? [String: Any] else {
    return nil
  }

  var decoded: [String: Any] = [:]
  decoded.reserveCapacity(encoded.count)
  for (key, rawValue) in encoded {
    guard let value = rawValue as? [String: Any],
          let type = value["type"] as? String
    else {
      return nil
    }

    switch type {
    case "string":
      guard let scalar = value["value"] as? String else { return nil }
      decoded[key] = scalar
    case "integer":
      guard let text = value["value"] as? String,
            let scalar = Int64(text)
      else { return nil }
      decoded[key] = scalar
    case "number":
      guard let text = value["value"] as? String,
            text.count == 16,
            let bits = UInt64(text, radix: 16)
      else { return nil }
      decoded[key] = Double(bitPattern: bits)
    case "boolean":
      guard let scalar = value["value"] as? Bool else { return nil }
      decoded[key] = scalar
    default:
      return nil
    }
  }
  return decoded
}

private func encodeNumber(_ value: Double) -> String {
  let encoded = String(value.bitPattern, radix: 16, uppercase: false)
  return String(repeating: "0", count: 16 - encoded.count) + encoded
}

#if canImport(Nuxie)
@MainActor
private final class NuxieUnrealDelegateBridge: NuxieDelegate {
  private let emit: @Sendable (String, [String: Any]) -> Void

  init(emit: @escaping @Sendable (String, [String: Any]) -> Void) {
    self.emit = emit
  }

  func featureAccessDidChange(
    _ featureId: String,
    from oldValue: FeatureAccess?,
    to newValue: FeatureAccess
  ) {
    emit("feature_access_changed", [
      "featureId": featureId,
      "from": nullable(oldValue.map(featureAccessDictionary)),
      "to": featureAccessDictionary(newValue),
    ])
  }

  func nuxieDidEmit(_ info: NuxieActivityInfo) {
    emit("activity", [
      "schemaVersion": NuxieActivityInfo.schemaVersion,
      "id": info.id,
      "timestampMs": Int(info.timestamp.timeIntervalSince1970 * 1_000),
      "receivedAtMs": Int(info.receivedAt.timeIntervalSince1970 * 1_000),
      "name": info.name,
      "properties": info.properties.mapValues(activityValue),
    ])
  }

  func nuxie(_ sdk: NuxieSDK, didRequestAppAction action: AppAction) {
    emit("app_action", [
      "name": action.name,
      "payload": nullable(action.payload?.mapValues(appActionValue)),
      "experience": [
        "experienceId": action.experience.experienceId,
        "experienceVersion": nullable(action.experience.experienceVersion),
        "journeyId": nullable(action.experience.journeyId),
      ],
    ])
  }
}

private func featureAccessDictionary(_ access: FeatureAccess) -> [String: Any] {
  [
    "allowed": access.allowed,
    "unlimited": access.unlimited,
    "balance": nullable(access.balance),
    "type": access.type.rawValue,
  ]
}

private func activityValue(_ value: NuxieActivityValue) -> [String: Any] {
  switch value {
  case .string(let value): ["type": "string", "value": value]
  case .int(let value): ["type": "integer", "value": String(value)]
  case .double(let value): ["type": "number", "value": encodeNumber(value)]
  case .bool(let value): ["type": "boolean", "value": value]
  @unknown default: ["type": "string", "value": String(describing: value)]
  }
}

private func appActionValue(_ value: AppActionValue) -> [String: Any] {
  switch value {
  case .string(let value): ["type": "string", "value": value]
  case .int(let value): ["type": "integer", "value": String(value)]
  case .double(let value): ["type": "number", "value": encodeNumber(value)]
  case .bool(let value): ["type": "boolean", "value": value]
  @unknown default: ["type": "string", "value": String(describing: value)]
  }
}

private final class NuxieUnrealPurchaseDelegateBridge: NuxiePurchaseDelegate, @unchecked Sendable {
  private let emit: @Sendable (String, [String: Any]) -> Void
  private let timeoutSeconds: TimeInterval
  private let lock = NSLock()
  private var purchases: [String: CheckedContinuation<PurchaseResult, Never>] = [:]
  private var restores: [String: CheckedContinuation<RestoreResult, Never>] = [:]

  init(
    timeoutSeconds: TimeInterval = 60,
    emit: @escaping @Sendable (String, [String: Any]) -> Void
  ) {
    self.timeoutSeconds = timeoutSeconds
    self.emit = emit
  }

  func purchase(product: StoreProduct) async -> PurchaseResult {
    let requestId = UUID().uuidString
    return await withCheckedContinuation { continuation in
      lock.withLock {
        purchases[requestId] = continuation
      }
      emit("purchase_request", [
        "request_id": requestId,
        "platform": "ios",
        "product_id": product.productId,
        "store_product_id": product.storeProductId,
        "base_plan_id": NSNull(),
        "purchase_option_id": NSNull(),
        "offer_id": NSNull(),
        "placement_id": nullable(product.placementId),
        "display_name": product.name,
        "display_price": product.price,
        "timestamp_ms": Int(Date().timeIntervalSince1970 * 1_000),
      ])
      schedulePurchaseTimeout(requestId)
    }
  }

  func restorePurchases() async -> RestoreResult {
    let requestId = UUID().uuidString
    return await withCheckedContinuation { continuation in
      lock.withLock {
        restores[requestId] = continuation
      }
      emit("restore_request", [
        "request_id": requestId,
        "platform": "ios",
        "timestamp_ms": Int(Date().timeIntervalSince1970 * 1_000),
      ])
      scheduleRestoreTimeout(requestId)
    }
  }

  func completePurchase(requestId: String, payload: [String: Any]) {
    let continuation = lock.withLock { purchases.removeValue(forKey: requestId) }
    continuation?.resume(returning: purchaseResult(payload))
  }

  func completeRestore(requestId: String, payload: [String: Any]) {
    let continuation = lock.withLock { restores.removeValue(forKey: requestId) }
    continuation?.resume(returning: restoreResult(payload))
  }

  func cancelPending(reason: String) {
    let pending = lock.withLock {
      let pending = (Array(purchases.values), Array(restores.values))
      purchases.removeAll()
      restores.removeAll()
      return pending
    }
    pending.0.forEach { $0.resume(returning: .failed(bridgeError(reason))) }
    pending.1.forEach { $0.resume(returning: .failed(bridgeError(reason))) }
  }

  private func schedulePurchaseTimeout(_ requestId: String) {
    Task { [weak self] in
      guard let self else { return }
      try? await Task.sleep(nanoseconds: UInt64(self.timeoutSeconds * 1_000_000_000))
      let continuation = self.lock.withLock {
        self.purchases.removeValue(forKey: requestId)
      }
      continuation?.resume(returning: .failed(self.bridgeError("purchase_timeout")))
    }
  }

  private func scheduleRestoreTimeout(_ requestId: String) {
    Task { [weak self] in
      guard let self else { return }
      try? await Task.sleep(nanoseconds: UInt64(self.timeoutSeconds * 1_000_000_000))
      let continuation = self.lock.withLock {
        self.restores.removeValue(forKey: requestId)
      }
      continuation?.resume(returning: .failed(self.bridgeError("restore_timeout")))
    }
  }

  private func purchaseResult(_ payload: [String: Any]) -> PurchaseResult {
    switch (payload["type"] as? String)?.lowercased() {
    case "purchased": .purchased
    case "cancelled": .cancelled
    case "pending": .pending
    default: .failed(bridgeError((payload["message"] as? String) ?? "purchase_failed"))
    }
  }

  private func restoreResult(_ payload: [String: Any]) -> RestoreResult {
    switch (payload["type"] as? String)?.lowercased() {
    case "restored": .restored
    case "no_purchases": .noPurchases
    default: .failed(bridgeError((payload["message"] as? String) ?? "restore_failed"))
    }
  }

  private func bridgeError(_ message: String) -> Error {
    NSError(
      domain: "ai.nuxie.unreal",
      code: 1,
      userInfo: [NSLocalizedDescriptionKey: message]
    )
  }
}
#endif
