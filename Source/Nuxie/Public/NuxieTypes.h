#pragma once

#include "CoreMinimal.h"

#include "NuxieTypes.generated.h"

UENUM(BlueprintType)
enum class ENuxieEnvironment : uint8
{
  Production,
  Staging,
  Development,
  Custom,
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
enum class ENuxieEventLinkingPolicy : uint8
{
  KeepSeparate,
  MigrateOnIdentify,
};

UENUM(BlueprintType)
enum class ENuxieTriggerUpdateKind : uint8
{
  Decision,
  Entitlement,
  Journey,
  Error,
};

UENUM(BlueprintType)
enum class ENuxieTriggerDecisionKind : uint8
{
  NoMatch,
  Suppressed,
  JourneyStarted,
  JourneyResumed,
  FlowShown,
  AllowedImmediate,
  DeniedImmediate,
};

UENUM(BlueprintType)
enum class ENuxieSuppressReason : uint8
{
  AlreadyActive,
  ReentryLimited,
  Holdout,
  NoFlow,
  Unknown,
};

UENUM(BlueprintType)
enum class ENuxieEntitlementUpdateKind : uint8
{
  Pending,
  Allowed,
  Denied,
};

UENUM(BlueprintType)
enum class ENuxieGateSource : uint8
{
  Cache,
  Purchase,
  Restore,
};

UENUM(BlueprintType)
enum class ENuxieJourneyExitReason : uint8
{
  Completed,
  GoalMet,
  TriggerUnmatched,
  Expired,
  Error,
  Cancelled,
};

UENUM(BlueprintType)
enum class ENuxieFeatureType : uint8
{
  Boolean,
  Metered,
  CreditSystem,
};

UENUM(BlueprintType)
enum class ENuxiePurchaseResultKind : uint8
{
  Success,
  Cancelled,
  Pending,
  Failed,
};

UENUM(BlueprintType)
enum class ENuxieRestoreResultKind : uint8
{
  Success,
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

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieConfigureOptions
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString ApiKey;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieEnvironment Environment = ENuxieEnvironment::Production;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString ApiEndpoint;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieLogLevel LogLevel = ENuxieLogLevel::Warning;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bEnableConsoleLogging = true;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bEnableFileLogging = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bRedactSensitiveData = true;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 RequestTimeoutSeconds = 30;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 RetryCount = 3;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 RetryDelaySeconds = 2;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 SyncIntervalSeconds = 3600;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bEnableCompression = true;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 EventBatchSize = 50;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 FlushAt = 20;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 FlushIntervalSeconds = 30;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 MaxQueueSize = 1000;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieEventLinkingPolicy EventLinkingPolicy = ENuxieEventLinkingPolicy::MigrateOnIdentify;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString LocaleIdentifier;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bIsDebugMode = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bRespectDoNotTrack = true;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bUsePurchaseController = false;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieTriggerOptions
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  TMap<FString, FString> Properties;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  TMap<FString, FString> UserProperties;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  TMap<FString, FString> UserPropertiesSetOnce;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieJourneyRef
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString JourneyId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString CampaignId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString FlowId;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieJourneyUpdate
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString JourneyId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString CampaignId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString FlowId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieJourneyExitReason ExitReason = ENuxieJourneyExitReason::Completed;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bGoalMet = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int64 GoalMetAtEpochMillis = 0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasDurationSeconds = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  float DurationSeconds = 0.0f;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString FlowExitReason;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieTriggerUpdate
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieTriggerUpdateKind Kind = ENuxieTriggerUpdateKind::Error;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieTriggerDecisionKind DecisionKind = ENuxieTriggerDecisionKind::NoMatch;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieSuppressReason SuppressReason = ENuxieSuppressReason::Unknown;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString RawSuppressReason;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieEntitlementUpdateKind EntitlementKind = ENuxieEntitlementUpdateKind::Pending;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieGateSource GateSource = ENuxieGateSource::Cache;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieJourneyRef JourneyRef;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieJourneyUpdate Journey;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieError Error;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bIsTerminal = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int64 TimestampMs = 0;
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
  int32 Balance = 0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieFeatureType Type = ENuxieFeatureType::Boolean;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieFeatureCheckResult
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString CustomerId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString FeatureId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 RequiredBalance = 1;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Code;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FNuxieFeatureAccess Access;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString PreviewJson;
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
  float AmountUsed = 0.0f;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Message;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasUsage = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 UsageCurrent = 0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasUsageLimit = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 UsageLimit = 0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasUsageRemaining = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 UsageRemaining = 0;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieProfileResponse
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString CustomerId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString RawJson;
};

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
  FString BasePlanId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString OfferId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString DisplayName;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString DisplayPrice;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  bool bHasPrice = false;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  float Price = 0.0f;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString CurrencyCode;

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
  ENuxiePurchaseResultKind Kind = ENuxiePurchaseResultKind::Failed;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString ProductId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString PurchaseToken;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString OrderId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString TransactionId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString OriginalTransactionId;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString TransactionJws;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Message;
};

USTRUCT(BlueprintType)
struct NUXIE_API FNuxieRestoreResult
{
  GENERATED_BODY()

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  ENuxieRestoreResultKind Kind = ENuxieRestoreResultKind::Failed;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  int32 RestoredCount = 0;

  UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nuxie")
  FString Message;
};

namespace Nuxie
{
  struct NUXIE_API FTriggerContract
  {
    static bool IsTerminal(const FNuxieTriggerUpdate& Update)
    {
      if (Update.Kind == ENuxieTriggerUpdateKind::Error || Update.Kind == ENuxieTriggerUpdateKind::Journey)
      {
        return true;
      }

      if (Update.Kind == ENuxieTriggerUpdateKind::Decision)
      {
        return Update.DecisionKind == ENuxieTriggerDecisionKind::NoMatch
          || Update.DecisionKind == ENuxieTriggerDecisionKind::Suppressed
          || Update.DecisionKind == ENuxieTriggerDecisionKind::AllowedImmediate
          || Update.DecisionKind == ENuxieTriggerDecisionKind::DeniedImmediate;
      }

      if (Update.Kind == ENuxieTriggerUpdateKind::Entitlement)
      {
        return Update.EntitlementKind == ENuxieEntitlementUpdateKind::Allowed
          || Update.EntitlementKind == ENuxieEntitlementUpdateKind::Denied;
      }

      return false;
    }
  };
}
