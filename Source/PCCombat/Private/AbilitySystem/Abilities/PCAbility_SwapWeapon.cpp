#include "AbilitySystem/Abilities/PCAbility_SwapWeapon.h"
#include "AbilitySystemComponent.h"
#include "Data/PCAbilityData.h"
#include "Data/PCWeaponData.h"
#include "Equipment/PCEquipmentComponent.h"
#include "GameFramework/Actor.h"
#include "PCGameplayTags.h"

UPCAbility_SwapWeapon::UPCAbility_SwapWeapon()
{
	SetAssetTags(FGameplayTagContainer(PCTags::Ability_SwapWeapon));
	ActivationOwnedTags.AddTag(PCTags::State_Swapping);
}

UPCEquipmentComponent* UPCAbility_SwapWeapon::GetEquipment(const FGameplayAbilityActorInfo* ActorInfo)
{
	const AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	return Avatar ? Avatar->FindComponentByClass<UPCEquipmentComponent>() : nullptr;
}

bool UPCAbility_SwapWeapon::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const UPCEquipmentComponent* Equipment = GetEquipment(ActorInfo);
	return Equipment && Equipment->GetWeaponAt(Equipment->GetPendingIndex()) && Equipment->GetPendingIndex() != Equipment->GetCurrentIndex();
}

void UPCAbility_SwapWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UPCEquipmentComponent* Equipment = GetEquipment(ActorInfo);
	if (!GetAbilityData() || !Equipment || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TargetIndex = Equipment->GetPendingIndex();

	// 겉모습에 교체가 시작됐음을 알린다. 세기는 넣고 드는 데 걸리는 전체 시간이다.
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		FGameplayCueParameters Parameters;
		Parameters.SourceObject = Equipment->GetWeaponAt(TargetIndex);
		Parameters.RawMagnitude = GetPhaseDuration(EPCActionPhase::Windup) + GetPhaseDuration(EPCActionPhase::Active);
		ASC->ExecuteGameplayCue(PCTags::GameplayCue_Combat_Swap, Parameters);
	}

	StartPhase(EPCActionPhase::Windup, GetPhaseDuration(EPCActionPhase::Windup));
}

float UPCAbility_SwapWeapon::GetPhaseDuration(EPCActionPhase Phase) const
{
	const UPCEquipmentComponent* Equipment = GetEquipment(GetCurrentActorInfo());
	if (Equipment && Phase == EPCActionPhase::Windup)
	{
		// 넣는 구간: 아직 들고 있는 무기를 넣는 시간
		const UPCWeaponData* Current = Equipment->GetCurrentWeapon();
		return Current ? Current->SheatheTime : 0.f;
	}
	if (Equipment && Phase == EPCActionPhase::Active)
	{
		// 드는 구간: 새 무기를 꺼내는 시간
		const UPCWeaponData* Target = Equipment->GetWeaponAt(TargetIndex);
		return Target ? Target->DrawTime : 0.f;
	}
	return Super::GetPhaseDuration(Phase);
}

void UPCAbility_SwapWeapon::OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase)
{
	// 넣기가 끝나고 들기가 시작되는 순간에 실제로 무기가 바뀐다
	if (OldPhase == EPCActionPhase::Windup && NewPhase == EPCActionPhase::Active)
	{
		if (UPCEquipmentComponent* Equipment = GetEquipment(GetCurrentActorInfo()))
		{
			Equipment->EquipIndex(TargetIndex);
		}
	}
}
