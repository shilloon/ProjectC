#include "AbilitySystem/Effects/PCEffect_Stamina.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "PCGameplayTags.h"

UPCEffect_Stamina::UPCEffect_Stamina()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	AddSetByCallerModifier(UPCVitalAttributeSet::GetStaminaAttribute(), PCTags::Data_Stamina);
}
