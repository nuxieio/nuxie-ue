#include "NuxieJsonBridge.h"

#if PLATFORM_IOS

extern "C"
{
  char* NuxieUnreal_Invoke(
    const char* Method,
    const char* ArgumentsJson);
  char* NuxieUnreal_PopPendingEvent();
  void NuxieUnreal_FreeCString(char* Value);
}

namespace
{
  class FNuxieIOSTransport final : public INuxieNativeTransport
  {
  public:
    virtual FString Invoke(
      const FString& Method,
      const FString& ArgumentsJson,
      FNuxieError& OutError) const override
    {
      char* Response = NuxieUnreal_Invoke(
        TCHAR_TO_UTF8(*Method),
        TCHAR_TO_UTF8(*ArgumentsJson));
      if (Response == nullptr)
      {
        OutError = FNuxieError::Make(
          TEXT("NATIVE_ERROR"),
          TEXT("Nuxie iOS bridge returned no response."));
        return FString();
      }

      const FString Result = UTF8_TO_TCHAR(Response);
      NuxieUnreal_FreeCString(Response);
      return Result;
    }

    virtual bool PopPendingEvent(
      FString& OutEventJson) const override
    {
      char* Event = NuxieUnreal_PopPendingEvent();
      if (Event == nullptr)
      {
        return false;
      }

      OutEventJson = UTF8_TO_TCHAR(Event);
      NuxieUnreal_FreeCString(Event);
      return !OutEventJson.IsEmpty();
    }
  };
}

TSharedRef<INuxieNativeTransport> CreateNuxieIOSTransport()
{
  return MakeShared<FNuxieIOSTransport>();
}

#endif
