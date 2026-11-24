#include "AbilitySystem/Effects/PCEffect_Damage.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "PCGameplayTags.h"

UPCEffect_Damage::UPCEffect_Damage()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;

	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = PCTags::Data_Damage;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = UPCVitalAttributeSet::GetHealthAttribute();
	Modifier.ModifierOp = EGameplayModOp::Additive;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}
