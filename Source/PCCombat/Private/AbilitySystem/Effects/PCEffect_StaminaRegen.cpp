#include "AbilitySystem/Effects/PCEffect_StaminaRegen.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "PCGameplayTags.h"

UPCEffect_StaminaRegen::UPCEffect_StaminaRegen()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	Period = FScalableFloat(PeriodSeconds);
	bExecutePeriodicEffectOnApplication = false;
	AddSetByCallerModifier(UPCVitalAttributeSet::GetStaminaAttribute(), PCTags::Data_Stamina);

	FGameplayTagContainer PauseTags;
	PauseTags.AddTag(PCTags::State_Guarding);
	PauseTags.AddTag(PCTags::State_Dead);
	PauseTags.AddTag(PCTags::State_StaminaRegenPaused);
	PauseWhileTargetHasAnyTag(PauseTags);
}
