#include "AbilitySystem/Abilities/PCAbility_Dodge.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Characters/PCCharacterBase.h"
#include "Data/PCAbilityData.h"
#include "PCGameplayTags.h"

namespace
{
	/** 이동 속도 계산에서 0으로 나누지 않기 위한 최소 이동 시간 */
	constexpr float MinDodgeDuration = 0.05f;
}

UPCAbility_Dodge::UPCAbility_Dodge()
{
	SetAssetTags(FGameplayTagContainer(PCTags::Ability_Dodge));
}

void UPCAbility_Dodge::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UPCAbilityData* Data = GetAbilityData();
	APCCharacterBase* Character = GetPCCharacter();
	if (!Data || !Character || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 입력이 있으면 그 방향, 없으면 뒤로
	DodgeDirection = Character->GetLastMovementInputVector();
	DodgeDirection.Z = 0.f;
	if (!DodgeDirection.Normalize())
	{
		DodgeDirection = -Character->GetActorForwardVector();
	}

	StartPhase(EPCActionPhase::Windup, Data->Timing.Windup);
}

void UPCAbility_Dodge::OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase)
{
	if (NewPhase != EPCActionPhase::Active)
	{
		return;
	}

	const UPCAbilityData* Data = GetAbilityData();
	if (!Data)
	{
		return;
	}

	const float Duration = FMath::Max(Data->Timing.Active, MinDodgeDuration);
	const float Strength = Data->DodgeDistance / Duration;

	UAbilityTask_ApplyRootMotionConstantForce* Task = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this, NAME_None, DodgeDirection, Strength, Duration,
		false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, true);
	Task->ReadyForActivation();
}
