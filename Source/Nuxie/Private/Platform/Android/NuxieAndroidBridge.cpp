#include "Platform/Android/NuxieAndroidBridge.h"

#include "Async/Async.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include <cstdarg>

#if PLATFORM_ANDROID
#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#endif

namespace
{
  static constexpr const TCHAR* BridgeErrorCode = TEXT("NATIVE_ERROR");

  bool ParseBoolValue(const FString& Value)
  {
    return Value == TEXT("1") || Value.Equals(TEXT("true"), ESearchCase::IgnoreCase);
  }

  int32 ParseIntValue(const FString& Value)
  {
    return FCString::Atoi(*Value);
  }

  int64 ParseInt64Value(const FString& Value)
  {
    return FCString::Atoi64(*Value);
  }

#if PLATFORM_ANDROID
  FString JStringToFString(JNIEnv* Env, jstring Value)
  {
    if (Value == nullptr)
    {
      return FString();
    }

    const char* UtfChars = Env->GetStringUTFChars(Value, nullptr);
    FString Out = UTF8_TO_TCHAR(UtfChars);
    Env->ReleaseStringUTFChars(Value, UtfChars);
    return Out;
  }

  jclass GetBridgeClass(JNIEnv* Env)
  {
    static jclass CachedClass = nullptr;
    if (CachedClass == nullptr)
    {
      jclass LocalClass = Env->FindClass("io/nuxie/unreal/NuxieBridge");
      if (LocalClass == nullptr)
      {
        return nullptr;
      }
      CachedClass = reinterpret_cast<jclass>(Env->NewGlobalRef(LocalClass));
      Env->DeleteLocalRef(LocalClass);
    }

    return CachedClass;
  }

  jobject MakeJavaInteger(JNIEnv* Env, int32 Value)
  {
    jclass IntegerClass = Env->FindClass("java/lang/Integer");
    if (IntegerClass == nullptr)
    {
      return nullptr;
    }

    jmethodID ValueOf = Env->GetStaticMethodID(IntegerClass, "valueOf", "(I)Ljava/lang/Integer;");
    if (ValueOf == nullptr)
    {
      Env->DeleteLocalRef(IntegerClass);
      return nullptr;
    }

    jobject IntegerObject = Env->CallStaticObjectMethod(IntegerClass, ValueOf, static_cast<jint>(Value));
    Env->DeleteLocalRef(IntegerClass);
    return IntegerObject;
  }
#endif
}

FNuxieAndroidBridge::FNuxieAndroidBridge() = default;
FNuxieAndroidBridge::~FNuxieAndroidBridge()
{
#if PLATFORM_ANDROID
  if (bConfigured)
  {
    FNuxieError IgnoreError;
    CallVoidMethod(IgnoreError, "setNativeHandle", "(J)V", static_cast<jlong>(0));
  }
#endif
}

void FNuxieAndroidBridge::SetListener(INuxiePlatformBridgeListener* InListener)
{
  Listener = InListener;
}

FString FNuxieAndroidBridge::EncodeMap(const TMap<FString, FString>& Values)
{
  FString Out;
  bool bFirst = true;
  for (const TPair<FString, FString>& Pair : Values)
  {
    if (!bFirst)
    {
      Out += TEXT("&");
    }

    Out += FGenericPlatformHttp::UrlEncode(Pair.Key);
    Out += TEXT("=");
    Out += FGenericPlatformHttp::UrlEncode(Pair.Value);
    bFirst = false;
  }

  return Out;
}

TMap<FString, FString> FNuxieAndroidBridge::DecodeMap(const FString& Encoded)
{
  TMap<FString, FString> Out;
  if (Encoded.IsEmpty())
  {
    return Out;
  }

  TArray<FString> Pairs;
  Encoded.ParseIntoArray(Pairs, TEXT("&"), true);
  for (const FString& Pair : Pairs)
  {
    FString Key;
    FString Value;
    if (!Pair.Split(TEXT("="), &Key, &Value))
    {
      Out.Add(FGenericPlatformHttp::UrlDecode(Pair), FString());
      continue;
    }

    Out.Add(FGenericPlatformHttp::UrlDecode(Key), FGenericPlatformHttp::UrlDecode(Value));
  }

  return Out;
}

FString FNuxieAndroidBridge::BuildTriggerOptionsPayload(const FNuxieTriggerOptions& Options)
{
  TMap<FString, FString> Sections;
  Sections.Add(TEXT("properties"), EncodeMap(Options.Properties));
  Sections.Add(TEXT("user_properties"), EncodeMap(Options.UserProperties));
  Sections.Add(TEXT("user_properties_set_once"), EncodeMap(Options.UserPropertiesSetOnce));
  return EncodeMap(Sections);
}

FString FNuxieAndroidBridge::BuildConfigurePayload(const FNuxieConfigureOptions& Options)
{
  TMap<FString, FString> Fields;
  Fields.Add(TEXT("api_endpoint"), Options.ApiEndpoint);
  Fields.Add(TEXT("locale"), Options.LocaleIdentifier);
  Fields.Add(TEXT("console_logging"), Options.bEnableConsoleLogging ? TEXT("1") : TEXT("0"));
  Fields.Add(TEXT("file_logging"), Options.bEnableFileLogging ? TEXT("1") : TEXT("0"));
  Fields.Add(TEXT("debug"), Options.bIsDebugMode ? TEXT("1") : TEXT("0"));
  return EncodeMap(Fields);
}

FString FNuxieAndroidBridge::BuildPurchaseResultPayload(const FNuxiePurchaseResult& Result)
{
  TMap<FString, FString> Fields;

  FString Kind = TEXT("failed");
  switch (Result.Kind)
  {
  case ENuxiePurchaseResultKind::Success:
    Kind = TEXT("success");
    break;
  case ENuxiePurchaseResultKind::Cancelled:
    Kind = TEXT("cancelled");
    break;
  case ENuxiePurchaseResultKind::Pending:
    Kind = TEXT("pending");
    break;
  case ENuxiePurchaseResultKind::Failed:
  default:
    Kind = TEXT("failed");
    break;
  }

  Fields.Add(TEXT("kind"), Kind);
  Fields.Add(TEXT("product_id"), Result.ProductId);
  Fields.Add(TEXT("purchase_token"), Result.PurchaseToken);
  Fields.Add(TEXT("order_id"), Result.OrderId);
  Fields.Add(TEXT("transaction_id"), Result.TransactionId);
  Fields.Add(TEXT("original_transaction_id"), Result.OriginalTransactionId);
  Fields.Add(TEXT("transaction_jws"), Result.TransactionJws);
  Fields.Add(TEXT("message"), Result.Message);

  return EncodeMap(Fields);
}

FString FNuxieAndroidBridge::BuildRestoreResultPayload(const FNuxieRestoreResult& Result)
{
  TMap<FString, FString> Fields;

  FString Kind = TEXT("failed");
  switch (Result.Kind)
  {
  case ENuxieRestoreResultKind::Success:
    Kind = TEXT("success");
    break;
  case ENuxieRestoreResultKind::NoPurchases:
    Kind = TEXT("no_purchases");
    break;
  case ENuxieRestoreResultKind::Failed:
  default:
    Kind = TEXT("failed");
    break;
  }

  Fields.Add(TEXT("kind"), Kind);
  Fields.Add(TEXT("restored_count"), FString::FromInt(Result.RestoredCount));
  Fields.Add(TEXT("message"), Result.Message);

  return EncodeMap(Fields);
}

bool FNuxieAndroidBridge::ParseFeatureAccessPayload(const FString& Payload, FNuxieFeatureAccess& OutAccess)
{
  const TMap<FString, FString> Fields = DecodeMap(Payload);
  OutAccess.bAllowed = ParseBoolValue(Fields.FindRef(TEXT("allowed")));
  OutAccess.bUnlimited = ParseBoolValue(Fields.FindRef(TEXT("unlimited")));
  OutAccess.bHasBalance = ParseBoolValue(Fields.FindRef(TEXT("has_balance")));
  OutAccess.Balance = ParseIntValue(Fields.FindRef(TEXT("balance")));

  const FString Type = Fields.FindRef(TEXT("type"));
  if (Type == TEXT("metered"))
  {
    OutAccess.Type = ENuxieFeatureType::Metered;
  }
  else if (Type == TEXT("creditSystem") || Type == TEXT("credit_system"))
  {
    OutAccess.Type = ENuxieFeatureType::CreditSystem;
  }
  else
  {
    OutAccess.Type = ENuxieFeatureType::Boolean;
  }

  return true;
}

bool FNuxieAndroidBridge::ParseFeatureCheckPayload(const FString& Payload, FNuxieFeatureCheckResult& OutResult)
{
  const TMap<FString, FString> Fields = DecodeMap(Payload);
  OutResult.CustomerId = Fields.FindRef(TEXT("customer_id"));
  OutResult.FeatureId = Fields.FindRef(TEXT("feature_id"));
  OutResult.RequiredBalance = ParseIntValue(Fields.FindRef(TEXT("required_balance")));
  OutResult.Code = Fields.FindRef(TEXT("code"));
  OutResult.PreviewJson = Fields.FindRef(TEXT("preview"));
  ParseFeatureAccessPayload(Fields.FindRef(TEXT("access")), OutResult.Access);
  return true;
}

bool FNuxieAndroidBridge::ParseFeatureUsagePayload(const FString& Payload, FNuxieFeatureUsageResult& OutResult)
{
  const TMap<FString, FString> Fields = DecodeMap(Payload);
  OutResult.bSuccess = ParseBoolValue(Fields.FindRef(TEXT("success")));
  OutResult.FeatureId = Fields.FindRef(TEXT("feature_id"));
  OutResult.AmountUsed = static_cast<float>(FCString::Atod(*Fields.FindRef(TEXT("amount_used"))));
  OutResult.Message = Fields.FindRef(TEXT("message"));
  OutResult.bHasUsage = ParseBoolValue(Fields.FindRef(TEXT("has_usage")));
  OutResult.UsageCurrent = ParseIntValue(Fields.FindRef(TEXT("usage_current")));
  OutResult.bHasUsageLimit = ParseBoolValue(Fields.FindRef(TEXT("has_usage_limit")));
  OutResult.UsageLimit = ParseIntValue(Fields.FindRef(TEXT("usage_limit")));
  OutResult.bHasUsageRemaining = ParseBoolValue(Fields.FindRef(TEXT("has_usage_remaining")));
  OutResult.UsageRemaining = ParseIntValue(Fields.FindRef(TEXT("usage_remaining")));
  return true;
}

bool FNuxieAndroidBridge::ParseProfilePayload(const FString& Payload, FNuxieProfileResponse& OutProfile)
{
  const TMap<FString, FString> Fields = DecodeMap(Payload);
  OutProfile.CustomerId = Fields.FindRef(TEXT("customer_id"));
  OutProfile.RawJson = Fields.FindRef(TEXT("raw"));
  return true;
}

bool FNuxieAndroidBridge::ParseTriggerUpdatePayload(const FString& Payload, FNuxieTriggerUpdate& OutUpdate)
{
  const TMap<FString, FString> Fields = DecodeMap(Payload);
  const FString Kind = Fields.FindRef(TEXT("kind"));

  if (Kind == TEXT("decision"))
  {
    OutUpdate.Kind = ENuxieTriggerUpdateKind::Decision;
    const FString DecisionKind = Fields.FindRef(TEXT("decision_kind"));
    if (DecisionKind == TEXT("no_match"))
    {
      OutUpdate.DecisionKind = ENuxieTriggerDecisionKind::NoMatch;
    }
    else if (DecisionKind == TEXT("suppressed"))
    {
      OutUpdate.DecisionKind = ENuxieTriggerDecisionKind::Suppressed;
    }
    else if (DecisionKind == TEXT("journey_started"))
    {
      OutUpdate.DecisionKind = ENuxieTriggerDecisionKind::JourneyStarted;
    }
    else if (DecisionKind == TEXT("journey_resumed"))
    {
      OutUpdate.DecisionKind = ENuxieTriggerDecisionKind::JourneyResumed;
    }
    else if (DecisionKind == TEXT("flow_shown"))
    {
      OutUpdate.DecisionKind = ENuxieTriggerDecisionKind::FlowShown;
    }
    else if (DecisionKind == TEXT("allowed_immediate"))
    {
      OutUpdate.DecisionKind = ENuxieTriggerDecisionKind::AllowedImmediate;
    }
    else if (DecisionKind == TEXT("denied_immediate"))
    {
      OutUpdate.DecisionKind = ENuxieTriggerDecisionKind::DeniedImmediate;
    }

    const FString SuppressReason = Fields.FindRef(TEXT("suppress_reason"));
    if (SuppressReason == TEXT("already_active"))
    {
      OutUpdate.SuppressReason = ENuxieSuppressReason::AlreadyActive;
    }
    else if (SuppressReason == TEXT("reentry_limited"))
    {
      OutUpdate.SuppressReason = ENuxieSuppressReason::ReentryLimited;
    }
    else if (SuppressReason == TEXT("holdout"))
    {
      OutUpdate.SuppressReason = ENuxieSuppressReason::Holdout;
    }
    else if (SuppressReason == TEXT("no_flow"))
    {
      OutUpdate.SuppressReason = ENuxieSuppressReason::NoFlow;
    }
    else
    {
      OutUpdate.SuppressReason = ENuxieSuppressReason::Unknown;
    }

    OutUpdate.RawSuppressReason = Fields.FindRef(TEXT("raw_suppress_reason"));

    const TMap<FString, FString> JourneyRef = DecodeMap(Fields.FindRef(TEXT("journey_ref")));
    OutUpdate.JourneyRef.JourneyId = JourneyRef.FindRef(TEXT("journey_id"));
    OutUpdate.JourneyRef.CampaignId = JourneyRef.FindRef(TEXT("campaign_id"));
    OutUpdate.JourneyRef.FlowId = JourneyRef.FindRef(TEXT("flow_id"));
  }
  else if (Kind == TEXT("entitlement"))
  {
    OutUpdate.Kind = ENuxieTriggerUpdateKind::Entitlement;
    const FString EntitlementKind = Fields.FindRef(TEXT("entitlement_kind"));
    if (EntitlementKind == TEXT("allowed"))
    {
      OutUpdate.EntitlementKind = ENuxieEntitlementUpdateKind::Allowed;
    }
    else if (EntitlementKind == TEXT("denied"))
    {
      OutUpdate.EntitlementKind = ENuxieEntitlementUpdateKind::Denied;
    }
    else
    {
      OutUpdate.EntitlementKind = ENuxieEntitlementUpdateKind::Pending;
    }

    const FString GateSource = Fields.FindRef(TEXT("gate_source"));
    if (GateSource == TEXT("purchase"))
    {
      OutUpdate.GateSource = ENuxieGateSource::Purchase;
    }
    else if (GateSource == TEXT("restore"))
    {
      OutUpdate.GateSource = ENuxieGateSource::Restore;
    }
    else
    {
      OutUpdate.GateSource = ENuxieGateSource::Cache;
    }
  }
  else if (Kind == TEXT("journey"))
  {
    OutUpdate.Kind = ENuxieTriggerUpdateKind::Journey;
    const TMap<FString, FString> Journey = DecodeMap(Fields.FindRef(TEXT("journey")));
    OutUpdate.Journey.JourneyId = Journey.FindRef(TEXT("journey_id"));
    OutUpdate.Journey.CampaignId = Journey.FindRef(TEXT("campaign_id"));
    OutUpdate.Journey.FlowId = Journey.FindRef(TEXT("flow_id"));

    const FString ExitReason = Journey.FindRef(TEXT("exit_reason"));
    if (ExitReason == TEXT("goal_met"))
    {
      OutUpdate.Journey.ExitReason = ENuxieJourneyExitReason::GoalMet;
    }
    else if (ExitReason == TEXT("trigger_unmatched"))
    {
      OutUpdate.Journey.ExitReason = ENuxieJourneyExitReason::TriggerUnmatched;
    }
    else if (ExitReason == TEXT("expired"))
    {
      OutUpdate.Journey.ExitReason = ENuxieJourneyExitReason::Expired;
    }
    else if (ExitReason == TEXT("error"))
    {
      OutUpdate.Journey.ExitReason = ENuxieJourneyExitReason::Error;
    }
    else if (ExitReason == TEXT("cancelled"))
    {
      OutUpdate.Journey.ExitReason = ENuxieJourneyExitReason::Cancelled;
    }
    else
    {
      OutUpdate.Journey.ExitReason = ENuxieJourneyExitReason::Completed;
    }

    OutUpdate.Journey.bGoalMet = ParseBoolValue(Journey.FindRef(TEXT("goal_met")));
    OutUpdate.Journey.GoalMetAtEpochMillis = ParseInt64Value(Journey.FindRef(TEXT("goal_met_at")));
    OutUpdate.Journey.bHasDurationSeconds = ParseBoolValue(Journey.FindRef(TEXT("has_duration")));
    OutUpdate.Journey.DurationSeconds = static_cast<float>(FCString::Atod(*Journey.FindRef(TEXT("duration_seconds"))));
    OutUpdate.Journey.FlowExitReason = Journey.FindRef(TEXT("flow_exit_reason"));
  }
  else
  {
    OutUpdate.Kind = ENuxieTriggerUpdateKind::Error;
  }

  OutUpdate.Error.Code = Fields.FindRef(TEXT("error_code"));
  OutUpdate.Error.Message = Fields.FindRef(TEXT("error_message"));
  OutUpdate.TimestampMs = ParseInt64Value(Fields.FindRef(TEXT("timestamp_ms")));
  OutUpdate.bIsTerminal = ParseBoolValue(Fields.FindRef(TEXT("is_terminal")));

  return true;
}

bool FNuxieAndroidBridge::CaptureJavaException(FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  if (!Env->ExceptionCheck())
  {
    return false;
  }

  jthrowable Exception = Env->ExceptionOccurred();
  Env->ExceptionClear();

  jclass ThrowableClass = Env->FindClass("java/lang/Throwable");
  jmethodID ToStringMethod = Env->GetMethodID(ThrowableClass, "toString", "()Ljava/lang/String;");
  jstring MessageString = static_cast<jstring>(Env->CallObjectMethod(Exception, ToStringMethod));

  OutError = FNuxieError::Make(BridgeErrorCode, *JStringToFString(Env, MessageString));

  if (MessageString != nullptr)
  {
    Env->DeleteLocalRef(MessageString);
  }

  if (ThrowableClass != nullptr)
  {
    Env->DeleteLocalRef(ThrowableClass);
  }

  if (Exception != nullptr)
  {
    Env->DeleteLocalRef(Exception);
  }

  return true;
#else
  OutError = FNuxieError::Make(BridgeErrorCode, TEXT("Java bridge unavailable."));
  return true;
#endif
}

bool FNuxieAndroidBridge::CallVoidMethod(FNuxieError& OutError, const char* MethodName, const char* Signature, ...)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jclass BridgeClass = GetBridgeClass(Env);
  if (BridgeClass == nullptr)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieBridge Java class not found."));
    return false;
  }

  jmethodID Method = Env->GetStaticMethodID(BridgeClass, MethodName, Signature);
  if (Method == nullptr)
  {
    OutError = FNuxieError::Make(BridgeErrorCode, FString::Printf(TEXT("Missing Java method %s"), ANSI_TO_TCHAR(MethodName)));
    return false;
  }

  va_list Args;
  va_start(Args, Signature);
  Env->CallStaticVoidMethodV(BridgeClass, Method, Args);
  va_end(Args);

  if (CaptureJavaException(OutError))
  {
    return false;
  }

  return true;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

bool FNuxieAndroidBridge::CallBoolMethod(FNuxieError& OutError, const char* MethodName, const char* Signature, bool& OutValue, ...)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jclass BridgeClass = GetBridgeClass(Env);
  if (BridgeClass == nullptr)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieBridge Java class not found."));
    return false;
  }

  jmethodID Method = Env->GetStaticMethodID(BridgeClass, MethodName, Signature);
  if (Method == nullptr)
  {
    OutError = FNuxieError::Make(BridgeErrorCode, FString::Printf(TEXT("Missing Java method %s"), ANSI_TO_TCHAR(MethodName)));
    return false;
  }

  va_list Args;
  va_start(Args, Signature);
  const jboolean Value = Env->CallStaticBooleanMethodV(BridgeClass, Method, Args);
  va_end(Args);

  if (CaptureJavaException(OutError))
  {
    return false;
  }

  OutValue = Value == JNI_TRUE;
  return true;
#else
  OutValue = false;
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

bool FNuxieAndroidBridge::CallIntMethod(FNuxieError& OutError, const char* MethodName, const char* Signature, int32& OutValue, ...)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jclass BridgeClass = GetBridgeClass(Env);
  if (BridgeClass == nullptr)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieBridge Java class not found."));
    return false;
  }

  jmethodID Method = Env->GetStaticMethodID(BridgeClass, MethodName, Signature);
  if (Method == nullptr)
  {
    OutError = FNuxieError::Make(BridgeErrorCode, FString::Printf(TEXT("Missing Java method %s"), ANSI_TO_TCHAR(MethodName)));
    return false;
  }

  va_list Args;
  va_start(Args, Signature);
  const jint Value = Env->CallStaticIntMethodV(BridgeClass, Method, Args);
  va_end(Args);

  if (CaptureJavaException(OutError))
  {
    return false;
  }

  OutValue = static_cast<int32>(Value);
  return true;
#else
  OutValue = 0;
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

bool FNuxieAndroidBridge::CallStringMethod(FNuxieError& OutError, const char* MethodName, const char* Signature, FString& OutValue, ...)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jclass BridgeClass = GetBridgeClass(Env);
  if (BridgeClass == nullptr)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieBridge Java class not found."));
    return false;
  }

  jmethodID Method = Env->GetStaticMethodID(BridgeClass, MethodName, Signature);
  if (Method == nullptr)
  {
    OutError = FNuxieError::Make(BridgeErrorCode, FString::Printf(TEXT("Missing Java method %s"), ANSI_TO_TCHAR(MethodName)));
    return false;
  }

  va_list Args;
  va_start(Args, Signature);
  jstring Value = static_cast<jstring>(Env->CallStaticObjectMethodV(BridgeClass, Method, Args));
  va_end(Args);

  if (CaptureJavaException(OutError))
  {
    return false;
  }

  OutValue = JStringToFString(Env, Value);
  if (Value != nullptr)
  {
    Env->DeleteLocalRef(Value);
  }
  return true;
#else
  OutValue.Reset();
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

void FNuxieAndroidBridge::EmitError(const TCHAR* Code, const TCHAR* Message)
{
  if (Listener == nullptr)
  {
    return;
  }

  FNuxieTriggerUpdate Update;
  Update.Kind = ENuxieTriggerUpdateKind::Error;
  Update.Error = FNuxieError::Make(Code, Message);
  Update.TimestampMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
  Update.bIsTerminal = true;
  Listener->OnTriggerUpdate(TEXT(""), Update);
}

bool FNuxieAndroidBridge::Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jstring ApiKey = Env->NewStringUTF(TCHAR_TO_UTF8(*Options.ApiKey));
  jstring ConfigPayload = Env->NewStringUTF(TCHAR_TO_UTF8(*BuildConfigurePayload(Options)));
  jstring WrapperVersion = Env->NewStringUTF(TCHAR_TO_UTF8(TEXT("0.1.0")));

  if (!CallVoidMethod(OutError, "setNativeHandle", "(J)V", static_cast<jlong>(reinterpret_cast<intptr_t>(this))))
  {
    Env->DeleteLocalRef(ApiKey);
    Env->DeleteLocalRef(ConfigPayload);
    Env->DeleteLocalRef(WrapperVersion);
    return false;
  }

  const bool bSuccess = CallVoidMethod(
    OutError,
    "configure",
    "(Ljava/lang/String;Ljava/lang/String;ZLjava/lang/String;)V",
    ApiKey,
    ConfigPayload,
    static_cast<jboolean>(Options.bUsePurchaseController ? JNI_TRUE : JNI_FALSE),
    WrapperVersion);

  Env->DeleteLocalRef(ApiKey);
  Env->DeleteLocalRef(ConfigPayload);
  Env->DeleteLocalRef(WrapperVersion);

  bConfigured = bSuccess;
  return bSuccess;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

bool FNuxieAndroidBridge::Shutdown(FNuxieError& OutError)
{
  const bool bSuccess = CallVoidMethod(OutError, "shutdown", "()V");
  FNuxieError IgnoreError;
  CallVoidMethod(IgnoreError, "setNativeHandle", "(J)V", static_cast<jlong>(0));
  bConfigured = false;
  return bSuccess;
}

bool FNuxieAndroidBridge::Identify(
  const FString& DistinctId,
  const TMap<FString, FString>& UserProperties,
  const TMap<FString, FString>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jstring Distinct = Env->NewStringUTF(TCHAR_TO_UTF8(*DistinctId));
  jstring Props = Env->NewStringUTF(TCHAR_TO_UTF8(*EncodeMap(UserProperties)));
  jstring PropsOnce = Env->NewStringUTF(TCHAR_TO_UTF8(*EncodeMap(UserPropertiesSetOnce)));

  const bool bSuccess = CallVoidMethod(
    OutError,
    "identify",
    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
    Distinct,
    Props,
    PropsOnce);

  Env->DeleteLocalRef(Distinct);
  Env->DeleteLocalRef(Props);
  Env->DeleteLocalRef(PropsOnce);

  return bSuccess;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

bool FNuxieAndroidBridge::Reset(bool bKeepAnonymousId, FNuxieError& OutError)
{
  return CallVoidMethod(OutError, "reset", "(Z)V", static_cast<jboolean>(bKeepAnonymousId ? JNI_TRUE : JNI_FALSE));
}

FString FNuxieAndroidBridge::GetDistinctId() const
{
  FNuxieError Error;
  FString Value;
  const_cast<FNuxieAndroidBridge*>(this)->CallStringMethod(Error, "getDistinctId", "()Ljava/lang/String;", Value);
  return Value;
}

FString FNuxieAndroidBridge::GetAnonymousId() const
{
  FNuxieError Error;
  FString Value;
  const_cast<FNuxieAndroidBridge*>(this)->CallStringMethod(Error, "getAnonymousId", "()Ljava/lang/String;", Value);
  return Value;
}

bool FNuxieAndroidBridge::IsIdentified() const
{
  FNuxieError Error;
  bool bValue = false;
  const_cast<FNuxieAndroidBridge*>(this)->CallBoolMethod(Error, "getIsIdentified", "()Z", bValue);
  return bValue;
}

bool FNuxieAndroidBridge::StartTrigger(
  const FString& RequestId,
  const FString& EventName,
  const FNuxieTriggerOptions& Options,
  FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jstring Request = Env->NewStringUTF(TCHAR_TO_UTF8(*RequestId));
  jstring Event = Env->NewStringUTF(TCHAR_TO_UTF8(*EventName));
  jstring Payload = Env->NewStringUTF(TCHAR_TO_UTF8(*BuildTriggerOptionsPayload(Options)));

  const bool bSuccess = CallVoidMethod(
    OutError,
    "startTrigger",
    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",
    Request,
    Event,
    Payload);

  Env->DeleteLocalRef(Request);
  Env->DeleteLocalRef(Event);
  Env->DeleteLocalRef(Payload);

  return bSuccess;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

bool FNuxieAndroidBridge::CancelTrigger(const FString& RequestId, FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jstring Request = Env->NewStringUTF(TCHAR_TO_UTF8(*RequestId));
  const bool bSuccess = CallVoidMethod(OutError, "cancelTrigger", "(Ljava/lang/String;)V", Request);
  Env->DeleteLocalRef(Request);
  return bSuccess;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

bool FNuxieAndroidBridge::ShowFlow(const FString& FlowId, FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jstring Flow = Env->NewStringUTF(TCHAR_TO_UTF8(*FlowId));
  const bool bSuccess = CallVoidMethod(OutError, "showFlow", "(Ljava/lang/String;)V", Flow);
  Env->DeleteLocalRef(Flow);
  return bSuccess;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

void FNuxieAndroidBridge::RunAsyncBool(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError, TFunction<bool(FNuxieError&)> Work)
{
  Async(EAsyncExecution::ThreadPool, [OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError), Work = MoveTemp(Work)]() mutable
  {
    FNuxieError Error;
    bool bValue = false;
    bValue = Work(Error);

    AsyncTask(ENamedThreads::GameThread, [OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError), Error, bValue]() mutable
    {
      if (!Error.Code.IsEmpty())
      {
        OnError(Error);
      }
      else
      {
        OnSuccess(bValue);
      }
    });
  });
}

void FNuxieAndroidBridge::RunAsyncInt(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError, TFunction<bool(int32&, FNuxieError&)> Work)
{
  Async(EAsyncExecution::ThreadPool, [OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError), Work = MoveTemp(Work)]() mutable
  {
    FNuxieError Error;
    int32 Value = 0;
    Work(Value, Error);

    AsyncTask(ENamedThreads::GameThread, [OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError), Error, Value]() mutable
    {
      if (!Error.Code.IsEmpty())
      {
        OnError(Error);
      }
      else
      {
        OnSuccess(Value);
      }
    });
  });
}

void FNuxieAndroidBridge::RunAsyncVoid(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError, TFunction<bool(FNuxieError&)> Work)
{
  Async(EAsyncExecution::ThreadPool, [OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError), Work = MoveTemp(Work)]() mutable
  {
    FNuxieError Error;
    Work(Error);

    AsyncTask(ENamedThreads::GameThread, [OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError), Error]() mutable
    {
      if (!Error.Code.IsEmpty())
      {
        OnError(Error);
      }
      else
      {
        OnSuccess.ExecuteIfBound();
      }
    });
  });
}

void FNuxieAndroidBridge::RunAsyncProfile(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  Async(EAsyncExecution::ThreadPool, [this, OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError)]() mutable
  {
    FNuxieError Error;
    FString Payload;
    FNuxieProfileResponse Profile;

    if (!CallStringMethod(Error, "refreshProfile", "()Ljava/lang/String;", Payload) || !ParseProfilePayload(Payload, Profile))
    {
      AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError), Error]() mutable
      {
        OnError(Error.Code.IsEmpty() ? FNuxieError::Make(BridgeErrorCode, TEXT("Failed to refresh profile.")) : Error);
      });
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [OnSuccess = MoveTemp(OnSuccess), Profile]() mutable
    {
      OnSuccess(Profile);
    });
  });
}

void FNuxieAndroidBridge::RunAsyncHasFeature(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  Async(EAsyncExecution::ThreadPool, [this, FeatureId, RequiredBalance, EntityId, OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError)]() mutable
  {
    FNuxieError Error;
    FString Payload;

#if PLATFORM_ANDROID
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    jstring Feature = Env->NewStringUTF(TCHAR_TO_UTF8(*FeatureId));
    jobject Required = MakeJavaInteger(Env, RequiredBalance);
    jstring Entity = Env->NewStringUTF(TCHAR_TO_UTF8(*EntityId));
    const bool bSuccess = CallStringMethod(
      Error,
      "hasFeature",
      "(Ljava/lang/String;Ljava/lang/Integer;Ljava/lang/String;)Ljava/lang/String;",
      Payload,
      Feature,
      Required,
      Entity);
    Env->DeleteLocalRef(Feature);
    if (Required != nullptr)
    {
      Env->DeleteLocalRef(Required);
    }
    Env->DeleteLocalRef(Entity);

    FNuxieFeatureAccess Access;
    if (!bSuccess || !ParseFeatureAccessPayload(Payload, Access))
    {
      AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError), Error]() mutable
      {
        OnError(Error.Code.IsEmpty() ? FNuxieError::Make(BridgeErrorCode, TEXT("Failed to fetch feature access.")) : Error);
      });
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [OnSuccess = MoveTemp(OnSuccess), Access]() mutable
    {
      OnSuccess(Access);
    });
#else
    AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
    {
      OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
    });
#endif
  });
}

void FNuxieAndroidBridge::RunAsyncCheckFeature(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  bool bForceRefresh,
  FNuxieFeatureCheckSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  Async(EAsyncExecution::ThreadPool, [this, FeatureId, RequiredBalance, EntityId, bForceRefresh, OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError)]() mutable
  {
    FNuxieError Error;
    FString Payload;

#if PLATFORM_ANDROID
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    jstring Feature = Env->NewStringUTF(TCHAR_TO_UTF8(*FeatureId));
    jobject Required = MakeJavaInteger(Env, RequiredBalance);
    jstring Entity = Env->NewStringUTF(TCHAR_TO_UTF8(*EntityId));

    const bool bSuccess = CallStringMethod(
      Error,
      "checkFeature",
      "(Ljava/lang/String;Ljava/lang/Integer;Ljava/lang/String;Z)Ljava/lang/String;",
      Payload,
      Feature,
      Required,
      Entity,
      static_cast<jboolean>(bForceRefresh ? JNI_TRUE : JNI_FALSE));

    Env->DeleteLocalRef(Feature);
    if (Required != nullptr)
    {
      Env->DeleteLocalRef(Required);
    }
    Env->DeleteLocalRef(Entity);

    FNuxieFeatureCheckResult Result;
    if (!bSuccess || !ParseFeatureCheckPayload(Payload, Result))
    {
      AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError), Error]() mutable
      {
        OnError(Error.Code.IsEmpty() ? FNuxieError::Make(BridgeErrorCode, TEXT("Failed to check feature.")) : Error);
      });
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [OnSuccess = MoveTemp(OnSuccess), Result]() mutable
    {
      OnSuccess(Result);
    });
#else
    AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
    {
      OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
    });
#endif
  });
}

void FNuxieAndroidBridge::RunAsyncUseFeatureAndWait(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FString>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  Async(EAsyncExecution::ThreadPool, [this, FeatureId, Amount, EntityId, bSetUsage, Metadata, OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError)]() mutable
  {
    FNuxieError Error;
    FString Payload;

#if PLATFORM_ANDROID
    JNIEnv* Env = FAndroidApplication::GetJavaEnv();
    jstring Feature = Env->NewStringUTF(TCHAR_TO_UTF8(*FeatureId));
    jstring Entity = Env->NewStringUTF(TCHAR_TO_UTF8(*EntityId));
    jstring MetadataPayload = Env->NewStringUTF(TCHAR_TO_UTF8(*EncodeMap(Metadata)));

    const bool bSuccess = CallStringMethod(
      Error,
      "useFeatureAndWait",
      "(Ljava/lang/String;DLjava/lang/String;ZLjava/lang/String;)Ljava/lang/String;",
      Payload,
      Feature,
      static_cast<jdouble>(Amount),
      Entity,
      static_cast<jboolean>(bSetUsage ? JNI_TRUE : JNI_FALSE),
      MetadataPayload);

    Env->DeleteLocalRef(Feature);
    Env->DeleteLocalRef(Entity);
    Env->DeleteLocalRef(MetadataPayload);

    FNuxieFeatureUsageResult Result;
    if (!bSuccess || !ParseFeatureUsagePayload(Payload, Result))
    {
      AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError), Error]() mutable
      {
        OnError(Error.Code.IsEmpty() ? FNuxieError::Make(BridgeErrorCode, TEXT("Failed to use feature.")) : Error);
      });
      return;
    }

    AsyncTask(ENamedThreads::GameThread, [OnSuccess = MoveTemp(OnSuccess), Result]() mutable
    {
      OnSuccess(Result);
    });
#else
    AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
    {
      OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build.")));
    });
#endif
  });
}

void FNuxieAndroidBridge::RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  RunAsyncProfile(MoveTemp(OnSuccess), MoveTemp(OnError));
}

void FNuxieAndroidBridge::HasFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  RunAsyncHasFeature(FeatureId, RequiredBalance, EntityId, MoveTemp(OnSuccess), MoveTemp(OnError));
}

void FNuxieAndroidBridge::CheckFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  bool bForceRefresh,
  FNuxieFeatureCheckSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  RunAsyncCheckFeature(FeatureId, RequiredBalance, EntityId, bForceRefresh, MoveTemp(OnSuccess), MoveTemp(OnError));
}

bool FNuxieAndroidBridge::UseFeature(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  const TMap<FString, FString>& Metadata,
  FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jstring Feature = Env->NewStringUTF(TCHAR_TO_UTF8(*FeatureId));
  jstring Entity = Env->NewStringUTF(TCHAR_TO_UTF8(*EntityId));
  jstring MetadataPayload = Env->NewStringUTF(TCHAR_TO_UTF8(*EncodeMap(Metadata)));

  const bool bSuccess = CallVoidMethod(
    OutError,
    "useFeature",
    "(Ljava/lang/String;DLjava/lang/String;Ljava/lang/String;)V",
    Feature,
    static_cast<jdouble>(Amount),
    Entity,
    MetadataPayload);

  Env->DeleteLocalRef(Feature);
  Env->DeleteLocalRef(Entity);
  Env->DeleteLocalRef(MetadataPayload);
  return bSuccess;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

void FNuxieAndroidBridge::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FString>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  RunAsyncUseFeatureAndWait(FeatureId, Amount, EntityId, bSetUsage, Metadata, MoveTemp(OnSuccess), MoveTemp(OnError));
}

void FNuxieAndroidBridge::FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  RunAsyncBool(MoveTemp(OnSuccess), MoveTemp(OnError), [this](FNuxieError& Error)
  {
    bool bValue = false;
    if (!CallBoolMethod(Error, "flushEvents", "()Z", bValue))
    {
      return false;
    }
    return bValue;
  });
}

void FNuxieAndroidBridge::GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  RunAsyncInt(MoveTemp(OnSuccess), MoveTemp(OnError), [this](int32& OutValue, FNuxieError& Error)
  {
    return CallIntMethod(Error, "getQueuedEventCount", "()I", OutValue);
  });
}

void FNuxieAndroidBridge::PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  RunAsyncVoid(MoveTemp(OnSuccess), MoveTemp(OnError), [this](FNuxieError& Error)
  {
    return CallVoidMethod(Error, "pauseEventQueue", "()V");
  });
}

void FNuxieAndroidBridge::ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  RunAsyncVoid(MoveTemp(OnSuccess), MoveTemp(OnError), [this](FNuxieError& Error)
  {
    return CallVoidMethod(Error, "resumeEventQueue", "()V");
  });
}

bool FNuxieAndroidBridge::CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jstring Request = Env->NewStringUTF(TCHAR_TO_UTF8(*RequestId));
  jstring Payload = Env->NewStringUTF(TCHAR_TO_UTF8(*BuildPurchaseResultPayload(Result)));

  const bool bSuccess = CallVoidMethod(
    OutError,
    "completePurchase",
    "(Ljava/lang/String;Ljava/lang/String;)V",
    Request,
    Payload);

  Env->DeleteLocalRef(Request);
  Env->DeleteLocalRef(Payload);
  return bSuccess;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

bool FNuxieAndroidBridge::CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError)
{
#if PLATFORM_ANDROID
  JNIEnv* Env = FAndroidApplication::GetJavaEnv();
  jstring Request = Env->NewStringUTF(TCHAR_TO_UTF8(*RequestId));
  jstring Payload = Env->NewStringUTF(TCHAR_TO_UTF8(*BuildRestoreResultPayload(Result)));

  const bool bSuccess = CallVoidMethod(
    OutError,
    "completeRestore",
    "(Ljava/lang/String;Ljava/lang/String;)V",
    Request,
    Payload);

  Env->DeleteLocalRef(Request);
  Env->DeleteLocalRef(Payload);
  return bSuccess;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Android bridge JNI wiring is not linked in this build."));
  return false;
#endif
}

void FNuxieAndroidBridge::HandleTriggerUpdate(const FString& RequestId, const FString& Payload, bool bTerminal, int64 TimestampMs)
{
  if (Listener == nullptr)
  {
    return;
  }

  FNuxieTriggerUpdate Update;
  ParseTriggerUpdatePayload(Payload, Update);
  Update.bIsTerminal = bTerminal;
  if (TimestampMs > 0)
  {
    Update.TimestampMs = TimestampMs;
  }

  Listener->OnTriggerUpdate(RequestId, Update);
}

void FNuxieAndroidBridge::HandleFeatureAccessChanged(const FString& FeatureId, const FString& FromPayload, const FString& ToPayload)
{
  if (Listener == nullptr)
  {
    return;
  }

  FNuxieFeatureAccess From;
  FNuxieFeatureAccess To;
  ParseFeatureAccessPayload(FromPayload, From);
  ParseFeatureAccessPayload(ToPayload, To);
  Listener->OnFeatureAccessChanged(FeatureId, From, To);
}

void FNuxieAndroidBridge::HandlePurchaseRequest(const FString& Payload)
{
  if (Listener == nullptr)
  {
    return;
  }

  const TMap<FString, FString> Fields = DecodeMap(Payload);
  FNuxiePurchaseRequest Request;
  Request.RequestId = Fields.FindRef(TEXT("request_id"));
  Request.Platform = Fields.FindRef(TEXT("platform"));
  Request.ProductId = Fields.FindRef(TEXT("product_id"));
  Request.BasePlanId = Fields.FindRef(TEXT("base_plan_id"));
  Request.OfferId = Fields.FindRef(TEXT("offer_id"));
  Request.DisplayName = Fields.FindRef(TEXT("display_name"));
  Request.DisplayPrice = Fields.FindRef(TEXT("display_price"));
  Request.bHasPrice = ParseBoolValue(Fields.FindRef(TEXT("has_price")));
  Request.Price = static_cast<float>(FCString::Atod(*Fields.FindRef(TEXT("price"))));
  Request.CurrencyCode = Fields.FindRef(TEXT("currency_code"));
  Request.TimestampMs = ParseInt64Value(Fields.FindRef(TEXT("timestamp_ms")));

  Listener->OnPurchaseRequest(Request);
}

void FNuxieAndroidBridge::HandleRestoreRequest(const FString& Payload)
{
  if (Listener == nullptr)
  {
    return;
  }

  const TMap<FString, FString> Fields = DecodeMap(Payload);
  FNuxieRestoreRequest Request;
  Request.RequestId = Fields.FindRef(TEXT("request_id"));
  Request.Platform = Fields.FindRef(TEXT("platform"));
  Request.TimestampMs = ParseInt64Value(Fields.FindRef(TEXT("timestamp_ms")));

  Listener->OnRestoreRequest(Request);
}

void FNuxieAndroidBridge::HandleFlowPresented(const FString& FlowId)
{
  if (Listener == nullptr)
  {
    return;
  }

  Listener->OnFlowPresented(FlowId);
}

void FNuxieAndroidBridge::HandleFlowDismissed(const FString& Payload)
{
  if (Listener == nullptr)
  {
    return;
  }

  const TMap<FString, FString> Fields = DecodeMap(Payload);
  Listener->OnFlowDismissed(Fields.FindRef(TEXT("flow_id")));
}

#if PLATFORM_ANDROID
extern "C"
{
  JNIEXPORT void JNICALL Java_io_nuxie_unreal_NuxieBridge_nativeOnTriggerUpdate(
    JNIEnv* Env,
    jclass,
    jlong NativeHandle,
    jstring RequestId,
    jstring Payload,
    jboolean Terminal,
    jlong TimestampMs)
  {
    FNuxieAndroidBridge* Bridge = reinterpret_cast<FNuxieAndroidBridge*>(static_cast<intptr_t>(NativeHandle));
    if (Bridge == nullptr)
    {
      return;
    }

    Bridge->HandleTriggerUpdate(
      JStringToFString(Env, RequestId),
      JStringToFString(Env, Payload),
      Terminal == JNI_TRUE,
      static_cast<int64>(TimestampMs));
  }

  JNIEXPORT void JNICALL Java_io_nuxie_unreal_NuxieBridge_nativeOnFeatureAccessChanged(
    JNIEnv* Env,
    jclass,
    jlong NativeHandle,
    jstring FeatureId,
    jstring FromPayload,
    jstring ToPayload,
    jlong TimestampMs)
  {
    FNuxieAndroidBridge* Bridge = reinterpret_cast<FNuxieAndroidBridge*>(static_cast<intptr_t>(NativeHandle));
    if (Bridge == nullptr)
    {
      return;
    }

    Bridge->HandleFeatureAccessChanged(
      JStringToFString(Env, FeatureId),
      JStringToFString(Env, FromPayload),
      JStringToFString(Env, ToPayload));
  }

  JNIEXPORT void JNICALL Java_io_nuxie_unreal_NuxieBridge_nativeOnPurchaseRequest(
    JNIEnv* Env,
    jclass,
    jlong NativeHandle,
    jstring Payload)
  {
    FNuxieAndroidBridge* Bridge = reinterpret_cast<FNuxieAndroidBridge*>(static_cast<intptr_t>(NativeHandle));
    if (Bridge == nullptr)
    {
      return;
    }

    Bridge->HandlePurchaseRequest(JStringToFString(Env, Payload));
  }

  JNIEXPORT void JNICALL Java_io_nuxie_unreal_NuxieBridge_nativeOnRestoreRequest(
    JNIEnv* Env,
    jclass,
    jlong NativeHandle,
    jstring Payload)
  {
    FNuxieAndroidBridge* Bridge = reinterpret_cast<FNuxieAndroidBridge*>(static_cast<intptr_t>(NativeHandle));
    if (Bridge == nullptr)
    {
      return;
    }

    Bridge->HandleRestoreRequest(JStringToFString(Env, Payload));
  }

  JNIEXPORT void JNICALL Java_io_nuxie_unreal_NuxieBridge_nativeOnFlowPresented(
    JNIEnv* Env,
    jclass,
    jlong NativeHandle,
    jstring FlowId,
    jlong TimestampMs)
  {
    FNuxieAndroidBridge* Bridge = reinterpret_cast<FNuxieAndroidBridge*>(static_cast<intptr_t>(NativeHandle));
    if (Bridge == nullptr)
    {
      return;
    }

    Bridge->HandleFlowPresented(JStringToFString(Env, FlowId));
  }

  JNIEXPORT void JNICALL Java_io_nuxie_unreal_NuxieBridge_nativeOnFlowDismissed(
    JNIEnv* Env,
    jclass,
    jlong NativeHandle,
    jstring Payload,
    jlong TimestampMs)
  {
    FNuxieAndroidBridge* Bridge = reinterpret_cast<FNuxieAndroidBridge*>(static_cast<intptr_t>(NativeHandle));
    if (Bridge == nullptr)
    {
      return;
    }

    Bridge->HandleFlowDismissed(JStringToFString(Env, Payload));
  }
}
#endif
