import Foundation
import Testing
@testable import NuxieUnrealBridge

@Suite("Nuxie Unreal bridge contract")
struct BridgeContractTests {
  @Test("rejects malformed arguments through the versioned C ABI")
  func rejectsMalformedArguments() {
    let response = "configure".withCString { method in
      "not-json".withCString { arguments in
        NuxieUnreal_Invoke(method, arguments)
      }
    }
    defer { NuxieUnreal_FreeCString(response) }

    #expect(response != nil)
    let json = String(cString: response!)
    #expect(json.contains(#""code":"INVALID_ARGUMENTS""#))
  }

  @Test("pending event queue is empty before configuration")
  func pendingQueueStartsEmpty() {
    #expect(NuxieUnreal_PopPendingEvent() == nil)
  }

  @Test("tagged scalar codec preserves int64 and numeric type")
  func taggedScalarCodec() {
    let encoded: [String: Any] = [
      "integer": ["type": "integer", "value": String(Int64.max)],
      "number": ["type": "number", "value": "3ff0000000000000"],
      "nan": ["type": "number", "value": "7ff8000000000001"],
    ]
    let decoded = decodeScalarMap(encoded)

    #expect(decoded?["integer"] as? Int64 == Int64.max)
    #expect(decoded?["number"] as? Double == 1.0)
    #expect(
      (decoded?["nan"] as? Double)?.bitPattern == 0x7ff8_0000_0000_0001
    )
  }

  @Test("main-actor bridge work does not block the main actor")
  @MainActor
  func mainActorFastPath() throws {
    let result = runOnMainActorBlocking { Thread.isMainThread }
    #expect(try result.get())
  }
}
