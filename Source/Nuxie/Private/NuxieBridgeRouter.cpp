#include "NuxieBridgeRouter.h"

#include "NuxieJsonBridge.h"
#include "Platform/NuxieNoopBridge.h"

TUniquePtr<INuxiePlatformBridge> CreateNuxiePlatformBridge()
{
#if PLATFORM_IOS
  return MakeUnique<FNuxieJsonBridge>(CreateNuxieIOSTransport());
#elif PLATFORM_ANDROID
  return MakeUnique<FNuxieJsonBridge>(CreateNuxieAndroidTransport());
#else
  return MakeUnique<FNuxieNoopBridge>();
#endif
}
