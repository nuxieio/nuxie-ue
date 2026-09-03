#include "NuxieJsonBridge.h"

#if PLATFORM_ANDROID

#include "Android/AndroidApplication.h"
#include "Android/AndroidJNI.h"
#include "HAL/CriticalSection.h"
#include "Misc/ScopeLock.h"

namespace
{
  FString JStringToFString(JNIEnv* Env, jstring Value)
  {
    if (Value == nullptr)
    {
      return FString();
    }

    const char* Characters = Env->GetStringUTFChars(Value, nullptr);
    const FString Result = UTF8_TO_TCHAR(Characters);
    Env->ReleaseStringUTFChars(Value, Characters);
    return Result;
  }

  jclass GetBridgeClass(JNIEnv* Env)
  {
    static jclass BridgeClass = nullptr;
    static FCriticalSection BridgeClassMutex;
    FScopeLock Lock(&BridgeClassMutex);
    if (BridgeClass != nullptr)
    {
      return BridgeClass;
    }

    jclass LocalClass =
      Env->FindClass("ai/nuxie/unreal/NuxieUnrealBridge");
    if (LocalClass == nullptr)
    {
      Env->ExceptionClear();
      return nullptr;
    }
    BridgeClass =
      static_cast<jclass>(Env->NewGlobalRef(LocalClass));
    Env->DeleteLocalRef(LocalClass);
    return BridgeClass;
  }

  bool CaptureException(JNIEnv* Env, FNuxieError& OutError)
  {
    if (!Env->ExceptionCheck())
    {
      return false;
    }

    jthrowable Exception = Env->ExceptionOccurred();
    Env->ExceptionClear();
    jclass ThrowableClass = Env->FindClass("java/lang/Throwable");
    const jmethodID ToString = Env->GetMethodID(
      ThrowableClass,
      "toString",
      "()Ljava/lang/String;");
    jstring Message = static_cast<jstring>(
      Env->CallObjectMethod(Exception, ToString));
    OutError = FNuxieError::Make(
      TEXT("NATIVE_ERROR"),
      JStringToFString(Env, Message));
    Env->DeleteLocalRef(Message);
    Env->DeleteLocalRef(ThrowableClass);
    Env->DeleteLocalRef(Exception);
    return true;
  }

  class FNuxieAndroidTransport final : public INuxieNativeTransport
  {
  public:
    virtual FString Invoke(
      const FString& Method,
      const FString& ArgumentsJson,
      FNuxieError& OutError) const override
    {
      JNIEnv* Env = FAndroidApplication::GetJavaEnv();
      jclass BridgeClass = GetBridgeClass(Env);
      if (BridgeClass == nullptr)
      {
        OutError = FNuxieError::Make(
          TEXT("NATIVE_UNAVAILABLE"),
          TEXT("Nuxie Android bridge class is unavailable."));
        return FString();
      }

      const jmethodID InvokeMethod = Env->GetStaticMethodID(
        BridgeClass,
        "invoke",
        "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;)"
        "Ljava/lang/String;");
      if (InvokeMethod == nullptr)
      {
        CaptureException(Env, OutError);
        if (OutError.Code.IsEmpty())
        {
          OutError = FNuxieError::Make(
            TEXT("NATIVE_UNAVAILABLE"),
            TEXT("Nuxie Android invoke method is unavailable."));
        }
        return FString();
      }

      jstring JavaMethod =
        Env->NewStringUTF(TCHAR_TO_UTF8(*Method));
      jstring JavaArguments =
        Env->NewStringUTF(TCHAR_TO_UTF8(*ArgumentsJson));
      jstring Response = static_cast<jstring>(
        Env->CallStaticObjectMethod(
          BridgeClass,
          InvokeMethod,
          FJavaWrapper::GameActivityThis,
          JavaMethod,
          JavaArguments));
      Env->DeleteLocalRef(JavaMethod);
      Env->DeleteLocalRef(JavaArguments);
      if (CaptureException(Env, OutError))
      {
        return FString();
      }

      const FString Result = JStringToFString(Env, Response);
      if (Response != nullptr)
      {
        Env->DeleteLocalRef(Response);
      }
      return Result;
    }

    virtual bool PopPendingEvent(
      FString& OutEventJson) const override
    {
      JNIEnv* Env = FAndroidApplication::GetJavaEnv();
      jclass BridgeClass = GetBridgeClass(Env);
      if (BridgeClass == nullptr)
      {
        return false;
      }

      const jmethodID PopMethod = Env->GetStaticMethodID(
        BridgeClass,
        "popPendingEvent",
        "()Ljava/lang/String;");
      if (PopMethod == nullptr)
      {
        Env->ExceptionClear();
        return false;
      }

      jstring Event = static_cast<jstring>(
        Env->CallStaticObjectMethod(BridgeClass, PopMethod));
      if (Env->ExceptionCheck())
      {
        Env->ExceptionClear();
        return false;
      }
      if (Event == nullptr)
      {
        return false;
      }

      OutEventJson = JStringToFString(Env, Event);
      Env->DeleteLocalRef(Event);
      return !OutEventJson.IsEmpty();
    }
  };
}

TSharedRef<INuxieNativeTransport> CreateNuxieAndroidTransport()
{
  return MakeShared<FNuxieAndroidTransport>();
}

#endif
