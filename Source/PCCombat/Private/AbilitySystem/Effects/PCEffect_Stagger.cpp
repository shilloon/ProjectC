#include "AbilitySystem/Effects/PCEffect_Stagger.h"
#include "PCGameplayTags.h"

UPCEffect_Stagger::UPCEffect_Stagger()
{
	SetDurationFromCaller(PCTags::Data_Duration);
	GrantTagToTarget(PCTags::State_Staggered);
}
