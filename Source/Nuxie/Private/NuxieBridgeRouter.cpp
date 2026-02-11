#include "NuxieBridgeRouter.h"

#include "Platform/NuxieNoopBridge.h"

#if PLATFORM_IOS
#include "Platform/IOS/NuxieIOSBridge.h"
#endif

#if PLATFORM_ANDROID
#include "Platform/Android/NuxieAndroidBridge.h"
#endif

TUniquePtr<INuxiePlatformBridge> CreateNuxiePlatformBridge()
{
#if PLATFORM_IOS
  return MakeUnique<FNuxieIOSBridge>();
#elif PLATFORM_ANDROID
  return MakeUnique<FNuxieAndroidBridge>();
#else
  return MakeUnique<FNuxieNoopBridge>();
#endif
}
