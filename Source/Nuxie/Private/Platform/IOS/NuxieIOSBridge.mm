#include "Platform/IOS/NuxieIOSBridge.h"

#include "Async/Async.h"
#include "Misc/DateTime.h"

#if PLATFORM_IOS
#import <Foundation/Foundation.h>
#import <objc/message.h>
#import <objc/runtime.h>
#endif

namespace
{
#if PLATFORM_IOS
  NSMutableDictionary* GetIOSTriggerHandles()
  {
    static NSMutableDictionary* TriggerHandles = nil;
    static dispatch_once_t OnceToken;
    dispatch_once(&OnceToken, ^{
      TriggerHandles = [NSMutableDictionary dictionary];
    });
    return TriggerHandles;
  }

  id GetClassByName(const char* NameA, const char* NameB)
  {
    Class ClassA = NSClassFromString([NSString stringWithUTF8String:NameA]);
    if (ClassA != Nil)
    {
      return ClassA;
    }

    Class ClassB = NSClassFromString([NSString stringWithUTF8String:NameB]);
    if (ClassB != Nil)
    {
      return ClassB;
    }

    return nil;
  }

  id GetSDKInstance()
  {
    id SDKClass = GetClassByName("NuxieSDK", "Nuxie.NuxieSDK");
    if (SDKClass == nil)
    {
      return nil;
    }

    SEL SharedSel = NSSelectorFromString(@"shared");
    if (![SDKClass respondsToSelector:SharedSel])
    {
      return nil;
    }

    return ((id(*)(id, SEL))objc_msgSend)(SDKClass, SharedSel);
  }

  NSString* ToNSString(const FString& Value)
  {
    return [NSString stringWithUTF8String:TCHAR_TO_UTF8(*Value)];
  }

  FString ToFString(NSString* Value)
  {
    if (Value == nil)
    {
      return FString();
    }

    return UTF8_TO_TCHAR([Value UTF8String]);
  }

  NSDictionary* ToNSDictionary(const TMap<FString, FString>& Values)
  {
    NSMutableDictionary* Dict = [NSMutableDictionary dictionaryWithCapacity:Values.Num()];
    for (const TPair<FString, FString>& Pair : Values)
    {
      Dict[ToNSString(Pair.Key)] = ToNSString(Pair.Value);
    }
    return Dict;
  }

  id GetValueForGetter(id Object, const char* Getter)
  {
    if (Object == nil)
    {
      return nil;
    }

    SEL Sel = NSSelectorFromString([NSString stringWithUTF8String:Getter]);
    if (![Object respondsToSelector:Sel])
    {
      return nil;
    }

    return ((id(*)(id, SEL))objc_msgSend)(Object, Sel);
  }

  bool CallAsyncStringResult(id Target, SEL Selector, NSString* Arg, FString& OutValue, FString& OutError)
  {
    if (Target == nil || ![Target respondsToSelector:Selector])
    {
      OutError = TEXT("selector_unavailable");
      return false;
    }

    __block NSString* ResultString = nil;
    __block NSError* ResultError = nil;
    dispatch_semaphore_t Semaphore = dispatch_semaphore_create(0);

    void (^Completion)(id, NSError*) = ^(id Result, NSError* Error)
    {
      if ([Result isKindOfClass:[NSString class]])
      {
        ResultString = static_cast<NSString*>(Result);
      }
      else if (Result != nil)
      {
        ResultString = [Result description];
      }
      ResultError = Error;
      dispatch_semaphore_signal(Semaphore);
    };

    typedef void (*FnType)(id, SEL, NSString*, id);
    ((FnType)objc_msgSend)(Target, Selector, Arg, Completion);
    dispatch_semaphore_wait(Semaphore, dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(60 * NSEC_PER_SEC)));

    if (ResultError != nil)
    {
      OutError = ToFString([ResultError localizedDescription]);
      return false;
    }

    OutValue = ToFString(ResultString);
    return true;
  }

  bool CallAsyncObjectResult(id Target, SEL Selector, id Arg, id& OutObject, FString& OutError)
  {
    if (Target == nil || ![Target respondsToSelector:Selector])
    {
      OutError = TEXT("selector_unavailable");
      return false;
    }

    __block id Result = nil;
    __block NSError* ResultError = nil;
    dispatch_semaphore_t Semaphore = dispatch_semaphore_create(0);

    void (^Completion)(id, NSError*) = ^(id Value, NSError* Error)
    {
      Result = Value;
      ResultError = Error;
      dispatch_semaphore_signal(Semaphore);
    };

    typedef void (*FnType)(id, SEL, id, id);
    ((FnType)objc_msgSend)(Target, Selector, Arg, Completion);
    dispatch_semaphore_wait(Semaphore, dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(60 * NSEC_PER_SEC)));

    if (ResultError != nil)
    {
      OutError = ToFString([ResultError localizedDescription]);
      return false;
    }

    OutObject = Result;
    return true;
  }

  ENuxieFeatureType ParseFeatureType(const FString& Type)
  {
    if (Type == TEXT("metered") || Type == TEXT("METERED"))
    {
      return ENuxieFeatureType::Metered;
    }

    if (Type == TEXT("creditSystem") || Type == TEXT("credit_system") || Type == TEXT("CREDIT_SYSTEM"))
    {
      return ENuxieFeatureType::CreditSystem;
    }

    return ENuxieFeatureType::Boolean;
  }
#endif
}

void FNuxieIOSBridge::SetListener(INuxiePlatformBridgeListener* InListener)
{
  Listener = InListener;
}

bool FNuxieIOSBridge::Configure(const FNuxieConfigureOptions& Options, FNuxieError& OutError)
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  id ConfigClass = GetClassByName("NuxieConfiguration", "Nuxie.NuxieConfiguration");
  if (SDK == nil || ConfigClass == nil)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("Nuxie iOS SDK classes are unavailable."));
    return false;
  }

  SEL InitSel = NSSelectorFromString(@"initWithApiKey:");
  if (![ConfigClass instancesRespondToSelector:InitSel])
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieConfiguration initWithApiKey: unavailable."));
    return false;
  }

  id Config = ((id(*)(id, SEL, id))objc_msgSend)([ConfigClass alloc], InitSel, ToNSString(Options.ApiKey));

  @try {
    if (!Options.ApiEndpoint.IsEmpty())
    {
      NSURL* Endpoint = [NSURL URLWithString:ToNSString(Options.ApiEndpoint)];
      [Config setValue:Endpoint forKey:@"apiEndpoint"];
    }
    if (!Options.LocaleIdentifier.IsEmpty())
    {
      [Config setValue:ToNSString(Options.LocaleIdentifier) forKey:@"localeIdentifier"];
    }
    [Config setValue:@(Options.bEnableConsoleLogging) forKey:@"enableConsoleLogging"];
    [Config setValue:@(Options.bEnableFileLogging) forKey:@"enableFileLogging"];
    [Config setValue:@(Options.bRedactSensitiveData) forKey:@"redactSensitiveData"];
    [Config setValue:@(Options.bIsDebugMode) forKey:@"isDebugMode"];
  }
  @catch (NSException* Ex)
  {
    // Keep going with defaults if newer/older SDK doesn't expose a field.
  }

  SEL SetupWithErrorSel = NSSelectorFromString(@"setupWith:error:");
  if ([SDK respondsToSelector:SetupWithErrorSel])
  {
    NSError* Error = nil;
    BOOL Success = ((BOOL(*)(id, SEL, id, NSError**))objc_msgSend)(SDK, SetupWithErrorSel, Config, &Error);
    if (!Success)
    {
      OutError = FNuxieError::Make(TEXT("NATIVE_ERROR"), Error != nil ? ToFString([Error localizedDescription]) : TEXT("iOS setup failed."));
      return false;
    }
    return true;
  }

  SEL SetupSel = NSSelectorFromString(@"setupWith:");
  if ([SDK respondsToSelector:SetupSel])
  {
    ((void(*)(id, SEL, id))objc_msgSend)(SDK, SetupSel, Config);
    return true;
  }

  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK setup selector unavailable."));
  return false;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform."));
  return false;
#endif
}

bool FNuxieIOSBridge::Shutdown(FNuxieError& OutError)
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK unavailable."));
    return false;
  }

  SEL ShutdownSel = NSSelectorFromString(@"shutdownWithCompletionHandler:");
  if ([SDK respondsToSelector:ShutdownSel])
  {
    __block NSError* ResultError = nil;
    dispatch_semaphore_t Semaphore = dispatch_semaphore_create(0);
    void (^Completion)(NSError*) = ^(NSError* Error)
    {
      ResultError = Error;
      dispatch_semaphore_signal(Semaphore);
    };
    ((void(*)(id, SEL, id))objc_msgSend)(SDK, ShutdownSel, Completion);
    dispatch_semaphore_wait(Semaphore, dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(60 * NSEC_PER_SEC)));

    if (ResultError != nil)
    {
      OutError = FNuxieError::Make(TEXT("NATIVE_ERROR"), ToFString([ResultError localizedDescription]));
      return false;
    }
    return true;
  }

  SEL ShutdownSyncSel = NSSelectorFromString(@"shutdown");
  if ([SDK respondsToSelector:ShutdownSyncSel])
  {
    ((void(*)(id, SEL))objc_msgSend)(SDK, ShutdownSyncSel);
    return true;
  }

  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK shutdown selector unavailable."));
  return false;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform."));
  return false;
#endif
}

bool FNuxieIOSBridge::Identify(
  const FString& DistinctId,
  const TMap<FString, FString>& UserProperties,
  const TMap<FString, FString>& UserPropertiesSetOnce,
  FNuxieError& OutError)
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK unavailable."));
    return false;
  }

  SEL IdentifySel = NSSelectorFromString(@"identify:userProperties:userPropertiesSetOnce:");
  if (![SDK respondsToSelector:IdentifySel])
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("identify selector unavailable."));
    return false;
  }

  ((void(*)(id, SEL, id, id, id))objc_msgSend)(
    SDK,
    IdentifySel,
    ToNSString(DistinctId),
    ToNSDictionary(UserProperties),
    ToNSDictionary(UserPropertiesSetOnce));

  return true;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform."));
  return false;
#endif
}

bool FNuxieIOSBridge::Reset(bool bKeepAnonymousId, FNuxieError& OutError)
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK unavailable."));
    return false;
  }

  SEL ResetSel = NSSelectorFromString(@"resetWithKeepAnonymousId:");
  if ([SDK respondsToSelector:ResetSel])
  {
    ((void(*)(id, SEL, BOOL))objc_msgSend)(SDK, ResetSel, bKeepAnonymousId ? YES : NO);
    return true;
  }

  SEL LegacyResetSel = NSSelectorFromString(@"reset");
  if ([SDK respondsToSelector:LegacyResetSel])
  {
    ((void(*)(id, SEL))objc_msgSend)(SDK, LegacyResetSel);
    return true;
  }

  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("reset selector unavailable."));
  return false;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform."));
  return false;
#endif
}

FString FNuxieIOSBridge::GetDistinctId() const
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    return FString();
  }

  SEL Selector = NSSelectorFromString(@"getDistinctId");
  if (![SDK respondsToSelector:Selector])
  {
    return FString();
  }

  NSString* Value = ((NSString*(*)(id, SEL))objc_msgSend)(SDK, Selector);
  return ToFString(Value);
#else
  return FString();
#endif
}

FString FNuxieIOSBridge::GetAnonymousId() const
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    return FString();
  }

  SEL Selector = NSSelectorFromString(@"getAnonymousId");
  if (![SDK respondsToSelector:Selector])
  {
    return FString();
  }

  NSString* Value = ((NSString*(*)(id, SEL))objc_msgSend)(SDK, Selector);
  return ToFString(Value);
#else
  return FString();
#endif
}

bool FNuxieIOSBridge::IsIdentified() const
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    return false;
  }

  SEL Selector = NSSelectorFromString(@"isIdentified");
  if (![SDK respondsToSelector:Selector])
  {
    return false;
  }

  return ((BOOL(*)(id, SEL))objc_msgSend)(SDK, Selector) == YES;
#else
  return false;
#endif
}

bool FNuxieIOSBridge::StartTrigger(
  const FString& RequestId,
  const FString& EventName,
  const FNuxieTriggerOptions& Options,
  FNuxieError& OutError)
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK unavailable."));
    return false;
  }

  SEL Selector = NSSelectorFromString(@"trigger:properties:userProperties:userPropertiesSetOnce:handler:");
  if (![SDK respondsToSelector:Selector])
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("trigger selector unavailable."));
    return false;
  }

  __block INuxiePlatformBridgeListener* ListenerRef = Listener;
  void (^Handler)(id) = ^(id UpdateObject)
  {
    if (ListenerRef == nullptr)
    {
      return;
    }

    FNuxieTriggerUpdate Update;
    const NSString* WrapperName = NSStringFromClass([UpdateObject class]);
    const FString Wrapper = ToFString(const_cast<NSString*>(WrapperName));

    if (Wrapper.Contains(TEXT("Decision")))
    {
      Update.Kind = ENuxieTriggerUpdateKind::Decision;
    }
    else if (Wrapper.Contains(TEXT("Entitlement")))
    {
      Update.Kind = ENuxieTriggerUpdateKind::Entitlement;
    }
    else if (Wrapper.Contains(TEXT("Journey")))
    {
      Update.Kind = ENuxieTriggerUpdateKind::Journey;
    }
    else
    {
      Update.Kind = ENuxieTriggerUpdateKind::Error;
      Update.Error = FNuxieError::Make(TEXT("native_update_unknown"), Wrapper);
    }

    Update.TimestampMs = FDateTime::UtcNow().ToUnixTimestamp() * 1000;
    Update.bIsTerminal = Nuxie::FTriggerContract::IsTerminal(Update);

    AsyncTask(ENamedThreads::GameThread, [ListenerRef, RequestId, Update]()
    {
      if (ListenerRef != nullptr)
      {
        ListenerRef->OnTriggerUpdate(RequestId, Update);
      }
    });
  };

  typedef id (*TriggerFn)(id, SEL, id, id, id, id, id);
  id Handle = ((TriggerFn)objc_msgSend)(
    SDK,
    Selector,
    ToNSString(EventName),
    ToNSDictionary(Options.Properties),
    ToNSDictionary(Options.UserProperties),
    ToNSDictionary(Options.UserPropertiesSetOnce),
    Handler);

  if (Handle == nil)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_ERROR"), TEXT("trigger returned null handle."));
    return false;
  }

  NSMutableDictionary* TriggerHandles = GetIOSTriggerHandles();

  @synchronized (TriggerHandles)
  {
    TriggerHandles[ToNSString(RequestId)] = Handle;
  }

  return true;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform."));
  return false;
#endif
}

bool FNuxieIOSBridge::CancelTrigger(const FString& RequestId, FNuxieError& OutError)
{
#if PLATFORM_IOS
  NSMutableDictionary* TriggerHandles = GetIOSTriggerHandles();

  id Handle = nil;
  @synchronized (TriggerHandles)
  {
    Handle = TriggerHandles[ToNSString(RequestId)];
    [TriggerHandles removeObjectForKey:ToNSString(RequestId)];
  }

  if (Handle == nil)
  {
    OutError = FNuxieError::Make(TEXT("TRIGGER_NOT_FOUND"), TEXT("No trigger handle for request id."));
    return false;
  }

  SEL CancelSel = NSSelectorFromString(@"cancel");
  if ([Handle respondsToSelector:CancelSel])
  {
    ((void(*)(id, SEL))objc_msgSend)(Handle, CancelSel);
    return true;
  }

  OutError = FNuxieError::Make(TEXT("NATIVE_ERROR"), TEXT("Trigger handle cancel unavailable."));
  return false;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform."));
  return false;
#endif
}

bool FNuxieIOSBridge::ShowFlow(const FString& FlowId, FNuxieError& OutError)
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK unavailable."));
    return false;
  }

  SEL AsyncSelector = NSSelectorFromString(@"showFlowWith:completionHandler:");
  if ([SDK respondsToSelector:AsyncSelector])
  {
    __block NSError* ResultError = nil;
    dispatch_semaphore_t Semaphore = dispatch_semaphore_create(0);

    void (^Completion)(NSError*) = ^(NSError* Error)
    {
      ResultError = Error;
      dispatch_semaphore_signal(Semaphore);
    };

    typedef void (*FnType)(id, SEL, id, id);
    ((FnType)objc_msgSend)(SDK, AsyncSelector, ToNSString(FlowId), Completion);
    dispatch_semaphore_wait(Semaphore, dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(60 * NSEC_PER_SEC)));

    if (ResultError != nil)
    {
      OutError = FNuxieError::Make(TEXT("NATIVE_ERROR"), ToFString([ResultError localizedDescription]));
      return false;
    }

    if (Listener != nullptr)
    {
      Listener->OnFlowPresented(FlowId);
    }
    return true;
  }

  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("showFlow selector unavailable."));
  return false;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform."));
  return false;
#endif
}

void FNuxieIOSBridge::RefreshProfileAsync(FNuxieProfileSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
#if PLATFORM_IOS
  AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [OnSuccess = MoveTemp(OnSuccess), OnError = MoveTemp(OnError)]() mutable
  {
    id SDK = GetSDKInstance();
    if (SDK == nil)
    {
      AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
      {
        OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK unavailable.")));
      });
      return;
    }

    SEL Selector = NSSelectorFromString(@"refreshProfileWithCompletionHandler:");
    if (![SDK respondsToSelector:Selector])
    {
      AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
      {
        OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("refreshProfile selector unavailable.")));
      });
      return;
    }

    __block id Result = nil;
    __block NSError* ResultError = nil;
    dispatch_semaphore_t Semaphore = dispatch_semaphore_create(0);

    void (^Completion)(id, NSError*) = ^(id Value, NSError* Error)
    {
      Result = Value;
      ResultError = Error;
      dispatch_semaphore_signal(Semaphore);
    };

    ((void(*)(id, SEL, id))objc_msgSend)(SDK, Selector, Completion);
    dispatch_semaphore_wait(Semaphore, dispatch_time(DISPATCH_TIME_NOW, static_cast<int64_t>(60 * NSEC_PER_SEC)));

    if (ResultError != nil)
    {
      const FNuxieError Error = FNuxieError::Make(TEXT("NATIVE_ERROR"), ToFString([ResultError localizedDescription]));
      AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError), Error]() mutable
      {
        OnError(Error);
      });
      return;
    }

    FNuxieProfileResponse Profile;
    Profile.CustomerId = ToFString(static_cast<NSString*>(GetValueForGetter(Result, "customerId")));
    Profile.RawJson = ToFString([Result description]);

    AsyncTask(ENamedThreads::GameThread, [OnSuccess = MoveTemp(OnSuccess), Profile]() mutable
    {
      OnSuccess(Profile);
    });
  });
#else
  OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform.")));
#endif
}

void FNuxieIOSBridge::HasFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  FNuxieFeatureAccessSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("hasFeature async bridge not yet available on iOS dynamic runtime.")));
  });
}

void FNuxieIOSBridge::CheckFeatureAsync(
  const FString& FeatureId,
  int32 RequiredBalance,
  const FString& EntityId,
  bool bForceRefresh,
  FNuxieFeatureCheckSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("checkFeature async bridge not yet available on iOS dynamic runtime.")));
  });
}

bool FNuxieIOSBridge::UseFeature(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  const TMap<FString, FString>& Metadata,
  FNuxieError& OutError)
{
#if PLATFORM_IOS
  id SDK = GetSDKInstance();
  if (SDK == nil)
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("NuxieSDK unavailable."));
    return false;
  }

  SEL Selector = NSSelectorFromString(@"useFeature:amount:entityId:metadata:");
  if (![SDK respondsToSelector:Selector])
  {
    OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("useFeature selector unavailable."));
    return false;
  }

  ((void(*)(id, SEL, id, double, id, id))objc_msgSend)(
    SDK,
    Selector,
    ToNSString(FeatureId),
    static_cast<double>(Amount),
    ToNSString(EntityId),
    ToNSDictionary(Metadata));

  return true;
#else
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("iOS bridge unavailable on this platform."));
  return false;
#endif
}

void FNuxieIOSBridge::UseFeatureAndWaitAsync(
  const FString& FeatureId,
  float Amount,
  const FString& EntityId,
  bool bSetUsage,
  const TMap<FString, FString>& Metadata,
  FNuxieFeatureUsageSuccessCallback OnSuccess,
  FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("useFeatureAndWait async bridge not yet available on iOS dynamic runtime.")));
  });
}

void FNuxieIOSBridge::FlushEventsAsync(FNuxieBoolSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("flushEvents async bridge not yet available on iOS dynamic runtime.")));
  });
}

void FNuxieIOSBridge::GetQueuedEventCountAsync(FNuxieIntSuccessCallback OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("getQueuedEventCount async bridge not yet available on iOS dynamic runtime.")));
  });
}

void FNuxieIOSBridge::PauseEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("pauseEventQueue async bridge not yet available on iOS dynamic runtime.")));
  });
}

void FNuxieIOSBridge::ResumeEventQueueAsync(FSimpleDelegate OnSuccess, FNuxieErrorCallback OnError)
{
  AsyncTask(ENamedThreads::GameThread, [OnError = MoveTemp(OnError)]() mutable
  {
    OnError(FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("resumeEventQueue async bridge not yet available on iOS dynamic runtime.")));
  });
}

bool FNuxieIOSBridge::CompletePurchase(const FString& RequestId, const FNuxiePurchaseResult& Result, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("completePurchase is not yet available on iOS dynamic runtime."));
  return false;
}

bool FNuxieIOSBridge::CompleteRestore(const FString& RequestId, const FNuxieRestoreResult& Result, FNuxieError& OutError)
{
  OutError = FNuxieError::Make(TEXT("NATIVE_UNAVAILABLE"), TEXT("completeRestore is not yet available on iOS dynamic runtime."));
  return false;
}
