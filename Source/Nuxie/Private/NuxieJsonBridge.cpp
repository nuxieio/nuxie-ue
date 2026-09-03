#include "NuxieJsonBridge.h"

#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "Misc/LexFromString.h"
#include "Misc/LexToString.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogNuxieBridge, Log, All);

namespace
{
  using FJsonObjectPtr = TSharedPtr<FJsonObject>;
  using FJsonValuePtr = TSharedPtr<FJsonValue>;

  FString EncodeNumber(double Value)
  {
    uint64 Bits = 0;
    FMemory::Memcpy(&Bits, &Value, sizeof(Bits));
    static constexpr TCHAR HexDigits[] = TEXT("0123456789abcdef");
    TCHAR Encoded[17];
    for (int32 Index = 0; Index < 16; ++Index)
    {
      const uint32 Shift = static_cast<uint32>((15 - Index) * 4);
      Encoded[Index] = HexDigits[(Bits >> Shift) & 0x0f];
    }
    Encoded[16] = TEXT('\0');
    return FString(Encoded);
  }

  bool DecodeNumber(const FString& Encoded, double& OutValue)
  {
    if (Encoded.Len() != 16)
    {
      return false;
    }
    uint64 Bits = 0;
    for (const TCHAR Character : Encoded)
    {
      uint64 Digit = 0;
      if (Character >= TEXT('0') && Character <= TEXT('9'))
      {
        Digit = Character - TEXT('0');
      }
      else if (Character >= TEXT('a') && Character <= TEXT('f'))
      {
        Digit = Character - TEXT('a') + 10;
      }
      else if (Character >= TEXT('A') && Character <= TEXT('F'))
      {
        Digit = Character - TEXT('A') + 10;
      }
      else
      {
        return false;
      }
      Bits = (Bits << 4) | Digit;
    }

    FMemory::Memcpy(&OutValue, &Bits, sizeof(Bits));
    return true;
  }

  FString SerializeJson(const FJsonObjectPtr& Object)
  {
    FString Output;
    const TSharedRef<TJsonWriter<>> Writer =
      TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Object.ToSharedRef(), Writer);
    return Output;
  }

  bool ParseJsonObject(const FString& Raw, FJsonObjectPtr& OutObject)
  {
    const TSharedRef<TJsonReader<>> Reader =
      TJsonReaderFactory<>::Create(Raw);
    return FJsonSerializer::Deserialize(Reader, OutObject)
      && OutObject.IsValid();
  }

  FJsonValuePtr ScalarToJson(const FNuxieScalarValue& Value)
  {
    const FJsonObjectPtr Encoded = MakeShared<FJsonObject>();
    switch (Value.Type)
    {
    case ENuxieScalarType::Integer:
      Encoded->SetStringField(TEXT("type"), TEXT("integer"));
      Encoded->SetStringField(
        TEXT("value"),
        LexToString(Value.IntegerValue));
      break;
    case ENuxieScalarType::Number:
      Encoded->SetStringField(TEXT("type"), TEXT("number"));
      Encoded->SetStringField(
        TEXT("value"),
        EncodeNumber(Value.NumberValue));
      break;
    case ENuxieScalarType::Boolean:
      Encoded->SetStringField(TEXT("type"), TEXT("boolean"));
      Encoded->SetBoolField(TEXT("value"), Value.bBooleanValue);
      break;
    case ENuxieScalarType::String:
    default:
      Encoded->SetStringField(TEXT("type"), TEXT("string"));
      Encoded->SetStringField(TEXT("value"), Value.StringValue);
      break;
    }
    return MakeShared<FJsonValueObject>(Encoded);
  }

  FJsonObjectPtr ScalarMapToJson(
    const TMap<FString, FNuxieScalarValue>& Values)
  {
    const FJsonObjectPtr Object = MakeShared<FJsonObject>();
    for (const TPair<FString, FNuxieScalarValue>& Pair : Values)
    {
      Object->SetField(Pair.Key, ScalarToJson(Pair.Value));
    }
    return Object;
  }

  bool JsonToScalar(
    const FJsonValuePtr& Value,
    FNuxieScalarValue& OutScalar)
  {
    if (!Value.IsValid())
    {
      return false;
    }

    if (Value->Type != EJson::Object)
    {
      return false;
    }

    const FJsonObjectPtr Encoded = Value->AsObject();
    FString Type;
    if (!Encoded.IsValid()
      || !Encoded->TryGetStringField(TEXT("type"), Type))
    {
      return false;
    }

    if (Type == TEXT("string"))
    {
      OutScalar.Type = ENuxieScalarType::String;
      return Encoded->TryGetStringField(
        TEXT("value"),
        OutScalar.StringValue);
    }
    if (Type == TEXT("integer"))
    {
      FString Integer;
      OutScalar.Type = ENuxieScalarType::Integer;
      return Encoded->TryGetStringField(TEXT("value"), Integer)
        && LexTryParseString(OutScalar.IntegerValue, *Integer);
    }
    if (Type == TEXT("number"))
    {
      FString Number;
      OutScalar.Type = ENuxieScalarType::Number;
      return Encoded->TryGetStringField(TEXT("value"), Number)
        && DecodeNumber(Number, OutScalar.NumberValue);
    }
    if (Type == TEXT("boolean"))
    {
      OutScalar.Type = ENuxieScalarType::Boolean;
      return Encoded->TryGetBoolField(
        TEXT("value"),
        OutScalar.bBooleanValue);
    }
    return false;
  }

  void JsonToScalarMap(
    const FJsonObjectPtr& Object,
    TMap<FString, FNuxieScalarValue>& OutValues)
  {
    if (!Object.IsValid())
    {
      return;
    }

    for (const TPair<FString, FJsonValuePtr>& Pair : Object->Values)
    {
      FNuxieScalarValue Value;
      if (JsonToScalar(Pair.Value, Value))
      {
        OutValues.Add(Pair.Key, MoveTemp(Value));
      }
    }
  }

  FString EnvironmentName(ENuxieEnvironment Environment)
  {
    return Environment == ENuxieEnvironment::Development
      ? TEXT("development")
      : TEXT("production");
  }

  FString LogLevelName(ENuxieLogLevel Level)
  {
    switch (Level)
    {
    case ENuxieLogLevel::Verbose: return TEXT("verbose");
    case ENuxieLogLevel::Debug: return TEXT("debug");
    case ENuxieLogLevel::Info: return TEXT("info");
    case ENuxieLogLevel::Error: return TEXT("error");
    case ENuxieLogLevel::None: return TEXT("none");
    case ENuxieLogLevel::Warning:
    default: return TEXT("warning");
    }
  }

  bool Invoke(
    const TSharedRef<INuxieNativeTransport>& Transport,
    const FString& Method,
    const FJsonObjectPtr& Arguments,
    FJsonValuePtr& OutValue,
    FNuxieError& OutError)
  {
    const FString Raw =
      Transport->Invoke(Method, SerializeJson(Arguments), OutError);
    if (Raw.IsEmpty())
    {
      if (OutError.Code.IsEmpty())
      {
        OutError = FNuxieError::Make(
          TEXT("NATIVE_ERROR"),
          TEXT("Native bridge returned an empty response."));
      }
      return false;
    }

    FJsonObjectPtr Response;
    if (!ParseJsonObject(Raw, Response))
    {
      OutError = FNuxieError::Make(
        TEXT("NATIVE_ERROR"),
        TEXT("Native bridge returned invalid JSON."));
      return false;
    }

    bool bOk = false;
    if (!Response->TryGetBoolField(TEXT("ok"), bOk) || !bOk)
    {
      const FJsonObjectPtr* ErrorObject = nullptr;
      if (Response->TryGetObjectField(TEXT("error"), ErrorObject)
        && ErrorObject != nullptr
        && ErrorObject->IsValid())
      {
        (*ErrorObject)->TryGetStringField(TEXT("code"), OutError.Code);
        (*ErrorObject)->TryGetStringField(TEXT("message"), OutError.Message);
        (*ErrorObject)->TryGetStringField(
          TEXT("nativeStack"),
          OutError.NativeStack);
      }
      if (OutError.Code.IsEmpty())
      {
        OutError.Code = TEXT("NATIVE_ERROR");
      }
      if (OutError.Message.IsEmpty())
      {
        OutError.Message = TEXT("Native bridge operation failed.");
      }
      return false;
    }

    const FJsonValuePtr* Value = Response->Values.Find(TEXT("value"));
    OutValue = Value != nullptr ? *Value : MakeShared<FJsonValueNull>();
    return true;
  }

  bool InvokeVoid(
    const TSharedRef<INuxieNativeTransport>& Transport,
    const FString& Method,
    const FJsonObjectPtr& Arguments,
    FNuxieError& OutError)
  {
    FJsonValuePtr Ignored;
    return Invoke(Transport, Method, Arguments, Ignored, OutError);
  }

  bool ParseFeatureAccess(
    const FJsonObjectPtr& Object,
    FNuxieFeatureAccess& OutAccess)
  {
    if (!Object.IsValid())
    {
      return false;
    }

    Object->TryGetBoolField(TEXT("allowed"), OutAccess.bAllowed);
    Object->TryGetBoolField(TEXT("unlimited"), OutAccess.bUnlimited);

    const FJsonValuePtr* Balance = Object->Values.Find(TEXT("balance"));
    OutAccess.bHasBalance =
      Balance != nullptr
      && Balance->IsValid()
      && (*Balance)->Type == EJson::Number;
    if (OutAccess.bHasBalance)
    {
      OutAccess.Balance = (*Balance)->AsNumber();
    }

    FString Type;
    Object->TryGetStringField(TEXT("type"), Type);
    if (Type == TEXT("metered"))
    {
      OutAccess.Type = ENuxieFeatureType::Metered;
    }
    else if (Type == TEXT("creditSystem")
      || Type == TEXT("credit_system"))
    {
      OutAccess.Type = ENuxieFeatureType::CreditSystem;
    }
    else
    {
      OutAccess.Type = ENuxieFeatureType::Boolean;
    }
    return true;
  }

  bool ParseFeatureUsage(
    const FJsonObjectPtr& Object,
    FNuxieFeatureUsageResult& OutResult)
  {
    if (!Object.IsValid())
    {
      return false;
    }

    Object->TryGetBoolField(TEXT("success"), OutResult.bSuccess);
    Object->TryGetStringField(TEXT("featureId"), OutResult.FeatureId);
    Object->TryGetNumberField(TEXT("amountUsed"), OutResult.AmountUsed);
    Object->TryGetStringField(TEXT("message"), OutResult.Message);

    const FJsonObjectPtr* Usage = nullptr;
    OutResult.bHasUsage =
      Object->TryGetObjectField(TEXT("usage"), Usage)
      && Usage != nullptr
      && Usage->IsValid();
    if (OutResult.bHasUsage)
    {
      (*Usage)->TryGetNumberField(
        TEXT("current"),
        OutResult.Usage.Current);
      OutResult.Usage.bHasLimit =
        (*Usage)->TryGetNumberField(
          TEXT("limit"),
          OutResult.Usage.Limit);
      OutResult.Usage.bHasRemaining =
        (*Usage)->TryGetNumberField(
          TEXT("remaining"),
          OutResult.Usage.Remaining);
    }

    const FJsonObjectPtr* Access = nullptr;
    OutResult.bHasAuthoritativeAccess =
      Object->TryGetObjectField(TEXT("authoritativeAccess"), Access)
      && Access != nullptr
      && ParseFeatureAccess(*Access, OutResult.AuthoritativeAccess);
    return true;
  }

  FJsonObjectPtr MakeArguments()
  {
    return MakeShared<FJsonObject>();
  }

  void RunAsyncVoid(
    TSharedRef<INuxieNativeTransport> Transport,
    FString Method,
    FJsonObjectPtr Arguments,
    FSimpleDelegate OnSuccess,
    FNuxieErrorCallback OnError)
  {
    Async(EAsyncExecution::ThreadPool, [
      Transport,
      Method = MoveTemp(Method),
      Arguments = MoveTemp(Arguments),
      OnSuccess = MoveTemp(OnSuccess),
      OnError = MoveTemp(OnError)]() mutable
    {
      FNuxieError Error;
      const bool bSuccess =
        InvokeVoid(Transport, Method, Arguments, Error);
      AsyncTask(ENamedThreads::GameThread, [
        bSuccess,
        Error,
        OnSuccess = MoveTemp(OnSuccess),
        OnError = MoveTemp(OnError)]() mutable
      {
        if (bSuccess)
        {
          OnSuccess.ExecuteIfBound();
        }
        else
        {
          OnError(Error);
        }
      });
    });
  }

  FString StringField(
    const FJsonObjectPtr& Object,
    const TCHAR* Name)
  {
    FString Value;
    if (Object.IsValid())
    {
      Object->TryGetStringField(Name, Value);
    }
    return Value;
  }

  int64 IntegerField(
    const FJsonObjectPtr& Object,
    const TCHAR* Name)
  {
    double Value = 0.0;
    if (Object.IsValid())
    {
      Object->TryGetNumberField(Name, Value);
    }
    return static_cast<int64>(Value);
  }
}

FNuxieJsonBridge::FNuxieJsonBridge(
  TSharedRef<INuxieNativeTransport> InTransport)
  : Transport(MoveTemp(InTransport))
{
  TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
    FTickerDelegate::CreateRaw(this, &FNuxieJsonBridge::Tick));
}

FNuxieJsonBridge::~FNuxieJsonBridge()
{
  FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
}

void FNuxieJsonBridge::SetListener(
  INuxiePlatformBridgeListener* InListener)
{
  Listener = InListener;
}

bool FNuxieJsonBridge::Configure(
  const FNuxieConfigureOptions& Options,
  FNuxieError& OutError)
{
  const FJsonObjectPtr NativeOptions = MakeArguments();
  NativeOptions->SetStringField(
    TEXT("environment"),
    EnvironmentName(Options.Environment));
  NativeOptions->SetStringField(
    TEXT("logLevel"),
    LogLevelName(Options.LogLevel));
  NativeOptions->SetBoolField(
    TEXT("enableConsoleLogging"),
    Options.bEnableConsoleLogging);
  NativeOptions->SetBoolField(
    TEXT("redactSensitiveData"),
    Options.bRedactSensitiveData);
  NativeOptions->SetStringField(
    TEXT("localeIdentifier"),
    Options.LocaleIdentifier);
  NativeOptions->SetStringField(
    TEXT("purchaseHandlingMode"),
    Options.PurchaseHandlingMode == ENuxiePurchaseHandlingMode::Observer
      ? TEXT("observer")
      : TEXT("full"));
  NativeOptions->SetBoolField(
    TEXT("testStoreEnabled"),
    Options.bTestStoreEnabled);

  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(TEXT("apiKey"), Options.ApiKey);
  Arguments->SetObjectField(TEXT("options"), NativeOptions);
  Arguments->SetBoolField(
    TEXT("usingPurchaseController"),
    Options.bUsePurchaseController);
  Arguments->SetStringField(TEXT("wrapperVersion"), TEXT("0.2.0"));
  return InvokeVoid(Transport, TEXT("configure"), Arguments, OutError);
}

void FNuxieJsonBridge::ShutdownAsync(
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  RunAsyncVoid(
    Transport,
    TEXT("shutdown"),
    MakeArguments(),
    MoveTemp(OnSuccess),
    MoveTemp(OnError));
}

bool FNuxieJsonBridge::Identify(
  const FString& DistinctId,
  const TMap<FString, FNuxieScalarValue>& UserProperties,
  const TMap<FString, FNuxieScalarValue>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(TEXT("distinctId"), DistinctId);
  Arguments->SetObjectField(
    TEXT("userProperties"),
    ScalarMapToJson(UserProperties));
  Arguments->SetObjectField(
    TEXT("userPropertiesSetOnce"),
    ScalarMapToJson(UserPropertiesSetOnce));
  return InvokeVoid(Transport, TEXT("identify"), Arguments, OutError);
}

bool FNuxieJsonBridge::Reset(
  bool bKeepAnonymousId,
  FNuxieError& OutError)
{
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetBoolField(TEXT("keepAnonymousId"), bKeepAnonymousId);
  return InvokeVoid(Transport, TEXT("reset"), Arguments, OutError);
}

FString FNuxieJsonBridge::GetDistinctId() const
{
  FNuxieError Error;
  FJsonValuePtr Value;
  if (Invoke(
    Transport,
    TEXT("getDistinctId"),
    MakeArguments(),
    Value,
    Error)
    && Value.IsValid()
    && Value->Type == EJson::String)
  {
    return Value->AsString();
  }
  return FString();
}

FString FNuxieJsonBridge::GetAnonymousId() const
{
  FNuxieError Error;
  FJsonValuePtr Value;
  if (Invoke(
    Transport,
    TEXT("getAnonymousId"),
    MakeArguments(),
    Value,
    Error)
    && Value.IsValid()
    && Value->Type == EJson::String)
  {
    return Value->AsString();
  }
  return FString();
}

bool FNuxieJsonBridge::IsIdentified() const
{
  FNuxieError Error;
  FJsonValuePtr Value;
  return Invoke(
      Transport,
      TEXT("getIsIdentified"),
      MakeArguments(),
      Value,
      Error)
    && Value.IsValid()
    && Value->Type == EJson::Boolean
    && Value->AsBool();
}

void FNuxieJsonBridge::Trigger(
  const FString& EventName,
  const TMap<FString, FNuxieScalarValue>& Properties)
{
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(TEXT("eventName"), EventName);
  Arguments->SetObjectField(
    TEXT("properties"),
    ScalarMapToJson(Properties));

  FNuxieError Error;
  if (!InvokeVoid(Transport, TEXT("trigger"), Arguments, Error))
  {
    UE_LOG(
      LogNuxieBridge,
      Warning,
      TEXT("Nuxie trigger bridge failed: %s"),
      *Error.Message);
  }
}

void FNuxieJsonBridge::DismissAsync(
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  RunAsyncVoid(
    Transport,
    TEXT("dismiss"),
    MakeArguments(),
    MoveTemp(OnSuccess),
    MoveTemp(OnError));
}

void FNuxieJsonBridge::SetLocaleIdentifierAsync(
  const FString& LocaleIdentifier,
  FSimpleDelegate OnSuccess,
  FNuxieErrorCallback OnError)
{
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(
    TEXT("localeIdentifier"),
    LocaleIdentifier);
  RunAsyncVoid(
    Transport,
    TEXT("setLocaleIdentifier"),
    Arguments,
    MoveTemp(OnSuccess),
    MoveTemp(OnError));
}

void FNuxieJsonBridge::HasFeatureAsync(
  const FString& FeatureId,
  double RequiredBalance,
  const FString& EntityId,
  ENuxieFeatureCheckPolicy Policy,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(TEXT("featureId"), FeatureId);
  Arguments->SetNumberField(TEXT("requiredBalance"), RequiredBalance);
  Arguments->SetStringField(TEXT("entityId"), EntityId);
  Arguments->SetStringField(
    TEXT("policy"),
    Policy == ENuxieFeatureCheckPolicy::Remote
      ? TEXT("remote")
      : TEXT("cache_first"));

  const TSharedRef<INuxieNativeTransport> NativeTransport = Transport;
  Async(EAsyncExecution::ThreadPool, [
    NativeTransport,
    Arguments,
    OnSuccess = MoveTemp(OnSuccess),
    OnError = MoveTemp(OnError)]() mutable
  {
    FNuxieError Error;
    FJsonValuePtr Value;
    FNuxieFeatureAccess Access;
    const bool bSuccess =
      Invoke(
        NativeTransport,
        TEXT("hasFeature"),
        Arguments,
        Value,
        Error)
      && Value.IsValid()
      && Value->Type == EJson::Object
      && ParseFeatureAccess(Value->AsObject(), Access);
    if (!bSuccess && Error.Code.IsEmpty())
    {
      Error = FNuxieError::Make(
        TEXT("NATIVE_ERROR"),
        TEXT("Native bridge returned invalid Feature access."));
    }

    AsyncTask(ENamedThreads::GameThread, [
      bSuccess,
      Access,
      Error,
      OnSuccess = MoveTemp(OnSuccess),
      OnError = MoveTemp(OnError)]() mutable
    {
      if (bSuccess)
      {
        OnSuccess(Access);
      }
      else
      {
        OnError(Error);
      }
    });
  });
}

void FNuxieJsonBridge::UseFeature(
  const FString& FeatureId,
  double Amount,
  const FString& EntityId,
  const TMap<FString, FNuxieScalarValue>& Metadata)
{
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(TEXT("featureId"), FeatureId);
  Arguments->SetNumberField(TEXT("amount"), Amount);
  Arguments->SetStringField(TEXT("entityId"), EntityId);
  Arguments->SetObjectField(
    TEXT("metadata"),
    ScalarMapToJson(Metadata));

  FNuxieError Error;
  if (!InvokeVoid(Transport, TEXT("useFeature"), Arguments, Error))
  {
    UE_LOG(
      LogNuxieBridge,
      Warning,
      TEXT("Nuxie Feature usage bridge failed: %s"),
      *Error.Message);
  }
}

void FNuxieJsonBridge::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  double Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FNuxieScalarValue>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(TEXT("featureId"), FeatureId);
  Arguments->SetNumberField(TEXT("amount"), Amount);
  Arguments->SetStringField(TEXT("entityId"), EntityId);
  Arguments->SetBoolField(TEXT("setUsage"), bSetUsage);
  Arguments->SetObjectField(
    TEXT("metadata"),
    ScalarMapToJson(Metadata));

  const TSharedRef<INuxieNativeTransport> NativeTransport = Transport;
  Async(EAsyncExecution::ThreadPool, [
    NativeTransport,
    Arguments,
    OnSuccess = MoveTemp(OnSuccess),
    OnError = MoveTemp(OnError)]() mutable
  {
    FNuxieError Error;
    FJsonValuePtr Value;
    FNuxieFeatureUsageResult Result;
    const bool bSuccess =
      Invoke(
        NativeTransport,
        TEXT("useFeatureAndWait"),
        Arguments,
        Value,
        Error)
      && Value.IsValid()
      && Value->Type == EJson::Object
      && ParseFeatureUsage(Value->AsObject(), Result);
    if (!bSuccess && Error.Code.IsEmpty())
    {
      Error = FNuxieError::Make(
        TEXT("NATIVE_ERROR"),
        TEXT("Native bridge returned invalid Feature usage."));
    }

    AsyncTask(ENamedThreads::GameThread, [
      bSuccess,
      Result,
      Error,
      OnSuccess = MoveTemp(OnSuccess),
      OnError = MoveTemp(OnError)]() mutable
    {
      if (bSuccess)
      {
        OnSuccess(Result);
      }
      else
      {
        OnError(Error);
      }
    });
  });
}

bool FNuxieJsonBridge::CompletePurchase(
  const FString& RequestId,
  const FNuxiePurchaseResult& Result,
  FNuxieError& OutError)
{
  FString Type = TEXT("failed");
  switch (Result.Type)
  {
  case ENuxiePurchaseResultType::Purchased: Type = TEXT("purchased"); break;
  case ENuxiePurchaseResultType::Cancelled: Type = TEXT("cancelled"); break;
  case ENuxiePurchaseResultType::Pending: Type = TEXT("pending"); break;
  case ENuxiePurchaseResultType::Failed:
  default: break;
  }

  const FJsonObjectPtr ResultObject = MakeArguments();
  ResultObject->SetStringField(TEXT("type"), Type);
  ResultObject->SetStringField(TEXT("message"), Result.Message);
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(TEXT("requestId"), RequestId);
  Arguments->SetObjectField(TEXT("result"), ResultObject);
  return InvokeVoid(
    Transport,
    TEXT("completePurchase"),
    Arguments,
    OutError);
}

bool FNuxieJsonBridge::CompleteRestore(
  const FString& RequestId,
  const FNuxieRestoreResult& Result,
  FNuxieError& OutError)
{
  FString Type = TEXT("failed");
  switch (Result.Type)
  {
  case ENuxieRestoreResultType::Restored: Type = TEXT("restored"); break;
  case ENuxieRestoreResultType::NoPurchases:
    Type = TEXT("no_purchases");
    break;
  case ENuxieRestoreResultType::Failed:
  default: break;
  }

  const FJsonObjectPtr ResultObject = MakeArguments();
  ResultObject->SetStringField(TEXT("type"), Type);
  ResultObject->SetStringField(TEXT("message"), Result.Message);
  const FJsonObjectPtr Arguments = MakeArguments();
  Arguments->SetStringField(TEXT("requestId"), RequestId);
  Arguments->SetObjectField(TEXT("result"), ResultObject);
  return InvokeVoid(
    Transport,
    TEXT("completeRestore"),
    Arguments,
    OutError);
}

bool FNuxieJsonBridge::Tick(float DeltaSeconds)
{
  constexpr int32 MaxEventsPerTick = 128;
  for (int32 Index = 0; Index < MaxEventsPerTick; ++Index)
  {
    FString EventJson;
    if (!Transport->PopPendingEvent(EventJson))
    {
      break;
    }
    DispatchEvent(EventJson);
  }
  return true;
}

void FNuxieJsonBridge::DispatchEvent(const FString& EventJson)
{
  if (Listener == nullptr)
  {
    return;
  }

  FJsonObjectPtr Envelope;
  if (!ParseJsonObject(EventJson, Envelope))
  {
    UE_LOG(
      LogNuxieBridge,
      Warning,
      TEXT("Discarding invalid native event JSON."));
    return;
  }

  FString Type;
  Envelope->TryGetStringField(TEXT("type"), Type);
  double EnvelopeTimestamp = 0.0;
  Envelope->TryGetNumberField(TEXT("timestampMs"), EnvelopeTimestamp);
  const FJsonObjectPtr* PayloadPtr = nullptr;
  if (!Envelope->TryGetObjectField(TEXT("payload"), PayloadPtr)
    || PayloadPtr == nullptr)
  {
    return;
  }
  const FJsonObjectPtr Payload = *PayloadPtr;

  if (Type == TEXT("feature_access_changed"))
  {
    FNuxieFeatureAccessChanged Event;
    Payload->TryGetStringField(TEXT("featureId"), Event.FeatureId);
    Event.TimestampMs = static_cast<int64>(EnvelopeTimestamp);
    const FJsonObjectPtr* Previous = nullptr;
    Event.bHasPreviousAccess =
      Payload->TryGetObjectField(TEXT("from"), Previous)
      && Previous != nullptr
      && ParseFeatureAccess(*Previous, Event.PreviousAccess);
    const FJsonObjectPtr* Current = nullptr;
    if (Payload->TryGetObjectField(TEXT("to"), Current)
      && Current != nullptr
      && ParseFeatureAccess(*Current, Event.CurrentAccess))
    {
      Listener->OnFeatureAccessChanged(Event);
    }
    return;
  }

  if (Type == TEXT("activity"))
  {
    FNuxieActivityInfo Activity;
    double SchemaVersion = 1.0;
    Payload->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion);
    Activity.SchemaVersion = static_cast<int32>(SchemaVersion);
    Activity.Id = StringField(Payload, TEXT("id"));
    Activity.TimestampMs = IntegerField(Payload, TEXT("timestampMs"));
    Activity.ReceivedAtMs =
      IntegerField(Payload, TEXT("receivedAtMs"));
    Activity.Name = StringField(Payload, TEXT("name"));
    const FJsonObjectPtr* Properties = nullptr;
    if (Payload->TryGetObjectField(TEXT("properties"), Properties)
      && Properties != nullptr)
    {
      JsonToScalarMap(*Properties, Activity.Properties);
    }
    Listener->OnActivity(Activity);
    return;
  }

  if (Type == TEXT("app_action"))
  {
    FNuxieAppAction Action;
    Action.Name = StringField(Payload, TEXT("name"));
    const FJsonObjectPtr* ActionPayload = nullptr;
    if (Payload->TryGetObjectField(TEXT("payload"), ActionPayload)
      && ActionPayload != nullptr)
    {
      JsonToScalarMap(*ActionPayload, Action.Payload);
    }
    const FJsonObjectPtr* Experience = nullptr;
    if (Payload->TryGetObjectField(TEXT("experience"), Experience)
      && Experience != nullptr)
    {
      Action.Experience.ExperienceId =
        StringField(*Experience, TEXT("experienceId"));
      Action.Experience.ExperienceVersion =
        StringField(*Experience, TEXT("experienceVersion"));
      Action.Experience.JourneyId =
        StringField(*Experience, TEXT("journeyId"));
    }
    Listener->OnAppAction(Action);
    return;
  }

  if (Type == TEXT("purchase_request"))
  {
    FNuxiePurchaseRequest Request;
    Request.RequestId = StringField(Payload, TEXT("request_id"));
    Request.Platform = StringField(Payload, TEXT("platform"));
    Request.ProductId = StringField(Payload, TEXT("product_id"));
    Request.StoreProductId =
      StringField(Payload, TEXT("store_product_id"));
    Request.BasePlanId = StringField(Payload, TEXT("base_plan_id"));
    Request.PurchaseOptionId =
      StringField(Payload, TEXT("purchase_option_id"));
    Request.OfferId = StringField(Payload, TEXT("offer_id"));
    Request.PlacementId = StringField(Payload, TEXT("placement_id"));
    Request.DisplayName = StringField(Payload, TEXT("display_name"));
    Request.DisplayPrice = StringField(Payload, TEXT("display_price"));
    Request.TimestampMs = IntegerField(Payload, TEXT("timestamp_ms"));
    Listener->OnPurchaseRequest(Request);
    return;
  }

  if (Type == TEXT("restore_request"))
  {
    FNuxieRestoreRequest Request;
    Request.RequestId = StringField(Payload, TEXT("request_id"));
    Request.Platform = StringField(Payload, TEXT("platform"));
    Request.TimestampMs = IntegerField(Payload, TEXT("timestamp_ms"));
    Listener->OnRestoreRequest(Request);
  }
}
