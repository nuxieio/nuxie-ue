#pragma once

#include "CoreMinimal.h"

#include "NuxieTypes.h"
#include "NuxieDelegates.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNuxieTriggerUpdateEvent, const FString&, RequestId, const FNuxieTriggerUpdate&, Update);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNuxieFeatureAccessChangedEvent, const FString&, FeatureId, const FNuxieFeatureAccess&, PreviousAccess, const FNuxieFeatureAccess&, CurrentAccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxiePurchaseRequestEvent, const FNuxiePurchaseRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieRestoreRequestEvent, const FNuxieRestoreRequest&, Request);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieFlowPresentedEvent, const FString&, FlowId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNuxieFlowDismissedEvent, const FString&, FlowId);

DECLARE_MULTICAST_DELEGATE_TwoParams(FNuxieTriggerUpdateNativeEvent, const FString&, const FNuxieTriggerUpdate&);
