#include "AbilitySystem/Effects/PCEffect_Dead.h"
#include "PCGameplayTags.h"

UPCEffect_Dead::UPCEffect_Dead()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	GrantTagToTarget(PCTags::State_Dead);
}
