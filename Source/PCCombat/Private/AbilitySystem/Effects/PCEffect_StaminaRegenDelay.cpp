#include "AbilitySystem/Effects/PCEffect_StaminaRegenDelay.h"
#include "PCGameplayTags.h"

UPCEffect_StaminaRegenDelay::UPCEffect_StaminaRegenDelay()
{
	SetDurationFromCaller(PCTags::Data_Duration);
	GrantTagToTarget(PCTags::State_StaminaRegenPaused);

	// 한 대상에 하나만 유지하고, 다시 적용되면 지속 시간을 갱신한다
	// UE 5.8: SetStackingType()은 선언만 있고 모듈 밖으로 공개되지 않아 링크되지 않는다. 멤버를 직접 설정한다.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}
