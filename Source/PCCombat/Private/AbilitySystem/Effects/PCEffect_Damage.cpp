#include "AbilitySystem/Effects/PCEffect_Damage.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "PCGameplayTags.h"

UPCEffect_Damage::UPCEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	AddSetByCallerModifier(UPCVitalAttributeSet::GetHealthAttribute(), PCTags::Data_Damage);
}
