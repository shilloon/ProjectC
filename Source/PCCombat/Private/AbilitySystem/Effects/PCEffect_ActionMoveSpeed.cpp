#include "AbilitySystem/Effects/PCEffect_ActionMoveSpeed.h"
#include "AbilitySystem/Attributes/PCCombatAttributeSet.h"
#include "PCGameplayTags.h"

UPCEffect_ActionMoveSpeed::UPCEffect_ActionMoveSpeed()
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;
	AddSetByCallerModifier(UPCCombatAttributeSet::GetMoveSpeedMultiplierAttribute(), PCTags::Data_MoveSpeed, EGameplayModOp::Multiplicitive);
}
