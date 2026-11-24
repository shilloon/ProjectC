#include "AbilitySystem/Abilities/PCAbility_Guard.h"
#include "Characters/PCCharacterBase.h"
#include "Data/PCAbilityData.h"
#include "PCGameplayTags.h"

UPCAbility_Guard::UPCAbility_Guard()
{
	SetAssetTags(FGameplayTagContainer(PCTags::Ability_Guard));
	ActivationOwnedTags.AddTag(PCTags::State_Guarding);
}

void UPCAbility_Guard::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UPCAbilityData* Data = GetAbilityData();
	APCCharacterBase* Character = GetPCCharacter();
	if (!Data || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Character->SetGuardHalfAngle(Data->GuardHalfAngleDeg);
}
