package ai.nuxie.unreal

import ai.nuxie.sdk.NuxieActivityValue
import ai.nuxie.sdk.features.FeatureAccess
import ai.nuxie.sdk.features.FeatureType
import ai.nuxie.sdk.features.FeatureUsageResult
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class BridgeContractsTest {
  @Test
  fun `feature access preserves fractional balances`() {
    val payload = FeatureAccess(
      allowed = true,
      unlimited = false,
      balance = 2.5,
      type = FeatureType.CREDIT_SYSTEM,
    ).toBridgeMap()

    assertEquals(true, payload["allowed"])
    assertEquals(2.5, payload["balance"])
    assertEquals("creditSystem", payload["type"])
  }

  @Test
  fun `atomic usage includes authoritative access`() {
    val payload = FeatureUsageResult(
      success = true,
      featureId = "credits",
      amountUsed = 1.25,
      message = null,
      usage = FeatureUsageResult.UsageInfo(
        current = 3.75,
        limit = 10.0,
        remaining = 6.25,
      ),
      authoritativeAccess = FeatureAccess(
        allowed = true,
        unlimited = false,
        balance = 6.25,
        type = FeatureType.METERED,
      ),
    ).toBridgeMap()

    assertEquals("credits", payload["featureId"])
    assertNull(payload["message"])
    assertEquals(
      6.25,
      (payload["authoritativeAccess"] as Map<*, *>)["balance"],
    )
  }

  @Test
  fun `tagged scalar codec preserves int64 and numeric type`() {
    val encoded = mapOf(
      "integer" to NuxieActivityValue.Int(Long.MAX_VALUE).toTaggedScalar(),
      "number" to NuxieActivityValue.Double(1.0).toTaggedScalar(),
      "nan" to NuxieActivityValue.Double(Double.fromBits(0x7ff8000000000001)).toTaggedScalar(),
    )

    assertEquals(Long.MAX_VALUE, decodeScalarMap(encoded)["integer"])
    assertEquals(1.0, decodeScalarMap(encoded)["number"])
    assertEquals(
      0x7ff8000000000001,
      (decodeScalarMap(encoded)["nan"] as Double).toRawBits(),
    )
    assertEquals("integer", encoded.getValue("integer")["type"])
    assertEquals("number", encoded.getValue("number")["type"])
  }
}
