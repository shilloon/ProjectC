#include "AbilitySystem/Effects/PCEffect_Stamina.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "PCGameplayTags.h"

UPCEffect_Stamina::UPCEffect_Stamina()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = PCTags::Data_Stamina;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UPCVitalAttributeSet::GetStaminaAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
