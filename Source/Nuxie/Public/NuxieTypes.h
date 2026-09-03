#pragma once

#include "CoreMinimal.h"

#include "NuxieTypes.generated.h"

UENUM(BlueprintType)
enum class ENuxieEnvironment : uint8
{
  Production,
  Development,
};

UENUM(BlueprintType)
enum class ENuxieLogLevel : uint8
{
  Verbose,
  Debug,
  Info,
  Warning,
  Error,
  None,
};

UENUM(BlueprintType)
enum class ENuxiePurchaseHandlingMode : uint8
{
  Full,
  Observer,
};

UENUM(BlueprintType)
enum class ENuxieFeatureCheckPolicy : uint8
{
  CacheFirst,
  Remote,
};

UENUM(BlueprintType)
enum class ENuxieFeatureType : uint8
{
  Boolean,
  Metered,
  CreditSystem,
};

UENUM(BlueprintType)
enum class ENuxieScalarType : uint8
{
  String,
  Integer,
  Number,
  Boolean,
};

UENUM(BlueprintType)
enum class ENuxiePurchaseResultType : uint8
{
  Purchased,
  Cancelled,
  Pending,
  Failed,
};

UENUM(BlueprintType)
enum class ENuxieRestoreResultType : uint8
{
  Restored,
  NoPurchases,
  Failed,
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieError
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Code;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Message;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString NativeStack;

  static FNuxieError Make(const FString& InCode, const FString& InMessage)
  {
    FNuxieError Error;
    Error.Code = InCode;
    Error.Message = InMessage;
    return Error;
  }
};

/** A portable scalar accepted by Journey events, activity, and App Actions. */
USTRUCT(BlueprintType)
struct NUXIE_API FNuxieScalarValue
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieScalarType Type = ENuxieScalarType::String;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString StringValue;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int64 IntegerValue = 0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  double NumberValue = 0.0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bBooleanValue = false;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieConfigureOptions
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString ApiKey;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieEnvironment Environment = ENuxieEnvironment::Production;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieLogLevel LogLevel = ENuxieLogLevel::Warning;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bEnableConsoleLogging = true;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bRedactSensitiveData = true;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString LocaleIdentifier;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxiePurchaseHandlingMode PurchaseHandlingMode = ENuxiePurchaseHandlingMode::Full;

  /** iOS-only Test Store switch. Android ignores this value. */
  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bTestStoreEnabled = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bUsePurchaseController = false;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureAccess
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bAllowed = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bUnlimited = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasBalance = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  double Balance = 0.0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieFeatureType Type = ENuxieFeatureType::Boolean;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureAccessChanged
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString FeatureId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasPreviousAccess = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieFeatureAccess PreviousAccess;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieFeatureAccess CurrentAccess;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int64 TimestampMs = 0;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureUsageInfo
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  double Current = 0.0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasLimit = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  double Limit = 0.0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasRemaining = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  double Remaining = 0.0;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureUsageResult
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bSuccess = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString FeatureId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  double AmountUsed = 0.0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Message;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasUsage = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieFeatureUsageInfo Usage;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasAuthoritativeAccess = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieFeatureAccess AuthoritativeAccess;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieActivityInfo
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 SchemaVersion = 1;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Id;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int64 TimestampMs = 0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int64 ReceivedAtMs = 0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Name;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  TMap<FString, FNuxieScalarValue> Properties;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieExperienceRef
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString ExperienceId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString ExperienceVersion;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString JourneyId;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieAppAction
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Name;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  TMap<FString, FNuxieScalarValue> Payload;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieExperienceRef Experience;
};

/** Portable checkout request; native bridges encode these fields as snake_case. */
USTRUCT(BlueprintType)
struct NUXIE_API FNuxiePurchaseRequest
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString RequestId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Platform;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString ProductId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString StoreProductId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString BasePlanId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString PurchaseOptionId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString OfferId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString PlacementId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString DisplayName;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString DisplayPrice;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int64 TimestampMs = 0;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieRestoreRequest
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString RequestId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Platform;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int64 TimestampMs = 0;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxiePurchaseResult
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxiePurchaseResultType Type = ENuxiePurchaseResultType::Failed;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Message;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieRestoreResult
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieRestoreResultType Type = ENuxieRestoreResultType::Failed;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Message;
};
