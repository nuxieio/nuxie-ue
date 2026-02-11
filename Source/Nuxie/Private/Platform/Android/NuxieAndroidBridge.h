#pragma once

#include "NuxiePlatformBridge.h"

class FNuxieAndroidBridge final : public INuxiePlatformBridge
{
public:
  FNuxieAndroidBridge();
  virtual ~FNuxieAndroidBridge() override;

  virtual void SetListener(INuxiePlatformBridgeListener* InListener) override;

  virtual bool Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError) override;
  virtual bool Shutdown(FNuxieError& OutError) override;
  virtual bool Identify(
    const FString& DistinctId,
    const TMap<FString, FString>& UserProperties,
    const TMap<FString, FString>& UserPropertiesSetOnce,
    FNuxieError& OutError) override;
  virtual bool Reset(bool bKeepAnonymousId, FNuxieError& OutError) override;
  virtual FString GetDistinctId() const override;
  virtual FString GetAnonymousId() const override;
  virtual bool IsIdentified() const override;
  virtual bool StartTrigger(
    const FString& RequestId,
    const FString& EventName,
    const FNuxieTriggerOptions& Options,
    FNuxieError& OutError) override;
  virtual bool CancelTrigger(const FString& RequestId, FNuxieError& OutError) override;
  virtual bool ShowFlow(const FString& FlowId, FNuxieError& OutError) override;
  virtual void RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void HasFeatureAsync(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    FNuxieFeatureAccessSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual void CheckFeatureAsync(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    bool bForceRefresh,
    FNuxieFeatureCheckSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual bool UseFeature(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    const TMap<FString, FString>& Metadata,
    FNuxieError& OutError) override;
  virtual void UseFeatureAndWaitAsync(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FString>& Metadata,
    FNuxieFeatureUsageSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError) override;
  virtual void FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) override;
  virtual void ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError) override;
  virtual bool CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError) override;
  virtual bool CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError) override;

private:
  static FString EncodeMap(const TMap<FString, FString>& Values);
  static TMap<FString, FString> DecodeMap(const FString& Encoded);
  static FString BuildTriggerOptionsPayload(const FNuxieTriggerOptions& Options);
  static FString BuildConfigurePayload(const FNuxieConfigureOptions& Options);
  static FString BuildPurchaseResultPayload(const FNuxiePurchaseResult& Result);
  static FString BuildRestoreResultPayload(const FNuxieRestoreResult& Result);

  static bool ParseFeatureAccessPayload(const FString& Payload, FNuxieFeatureAccess& OutAccess);
  static bool ParseFeatureCheckPayload(const FString& Payload, FNuxieFeatureCheckResult& OutResult);
  static bool ParseFeatureUsagePayload(const FString& Payload, FNuxieFeatureUsageResult& OutResult);
  static bool ParseProfilePayload(const FString& Payload, FNuxieProfileResponse& OutProfile);
  static bool ParseTriggerUpdatePayload(const FString& Payload, FNuxieTriggerUpdate& OutUpdate);

  bool CallVoidMethod(FNuxieError& OutError, const char* MethodName, const char* Signature, ...);
  bool CallBoolMethod(FNuxieError& OutError, const char* MethodName, const char* Signature, bool& OutValue, ...);
  bool CallIntMethod(FNuxieError& OutError, const char* MethodName, const char* Signature, int32& OutValue, ...);
  bool CallStringMethod(FNuxieError& OutError, const char* MethodName, const char* Signature, FString& OutValue, ...);
  bool CaptureJavaException(FNuxieError& OutError);
  void EmitError(const TCHAR* Code, const TCHAR* Message);

  void HandleTriggerUpdate(const FString& RequestId, const FString& Payload, bool bTerminal, int64 TimestampMs);
  void HandleFeatureAccessChanged(const FString& FeatureId, const FString& FromPayload, const FString& ToPayload);
  void HandlePurchaseRequest(const FString& Payload);
  void HandleRestoreRequest(const FString& Payload);
  void HandleFlowPresented(const FString& FlowId);
  void HandleFlowDismissed(const FString& Payload);

  void RunAsyncBool(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError, TFunction<bool(FNuxieError&)> Work);
  void RunAsyncInt(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError, TFunction<bool(int32&, FNuxieError&)> Work);
  void RunAsyncVoid(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError, TFunction<bool(FNuxieError&)> Work);
  void RunAsyncProfile(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError);
  void RunAsyncHasFeature(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    FNuxieFeatureAccessSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError);
  void RunAsyncCheckFeature(
    const FString& FeatureId,
    int32 RequiredBalance,
    const FString& EntityId,
    bool bForceRefresh,
    FNuxieFeatureCheckSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError);
  void RunAsyncUseFeatureAndWait(
    const FString& FeatureId,
    float Amount,
    const FString& EntityId,
    bool bSetUsage,
    const TMap<FString, FString>& Metadata,
    FNuxieFeatureUsageSuccessCallback OnSuccess,
    FNuxieErrorCallback OnError);

  INuxiePlatformBridgeListener* Listener = nullptr;
  bool bConfigured = false;
};
