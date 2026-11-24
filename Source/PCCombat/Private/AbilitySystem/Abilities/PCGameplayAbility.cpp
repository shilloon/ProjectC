#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "AbilitySystem/Effects/PCEffect_Stamina.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Characters/PCCharacterBase.h"
#include "Data/PCAbilityData.h"
#include "PCGameplayTags.h"

UPCGameplayAbility::UPCGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 한 번에 하나의 행동만 한다
	ActivationBlockedTags.AddTag(PCTags::State_Action);
	ActivationBlockedTags.AddTag(PCTags::State_Guarding);
	ActivationBlockedTags.AddTag(PCTags::State_Staggered);
	ActivationBlockedTags.AddTag(PCTags::State_Dead);
}

const UPCAbilityData* UPCGameplayAbility::GetDataFromSpec(const UGameplayAbility& Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo)
{
	return Cast<UPCAbilityData>(Ability.GetSourceObject(Handle, ActorInfo));
}

const UPCAbilityData* UPCGameplayAbility::GetAbilityData() const
{
	return GetDataFromSpec(*this, CurrentSpecHandle, CurrentActorInfo);
}

APCCharacterBase* UPCGameplayAbility::GetPCCharacter() const
{
	return Cast<APCCharacterBase>(GetAvatarActorFromActorInfo());
}

bool UPCGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
	const UPCAbilityData* Data = GetDataFromSpec(*this, Handle, ActorInfo);
	if (!Data || Data->StaminaCost <= 0.f)
	{
		return true;
	}

	const UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	return ASC && ASC->GetNumericAttribute(UPCVitalAttributeSet::GetStaminaAttribute()) >= Data->StaminaCost;
}

void UPCGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const UPCAbilityData* Data = GetDataFromSpec(*this, Handle, ActorInfo);
	UPCAbilitySystemComponent* ASC = ActorInfo ? Cast<UPCAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	if (Data && ASC && Data->StaminaCost > 0.f)
	{
		ASC->ApplySetByCallerEffectToSelf(UPCEffect_Stamina::StaticClass(), PCTags::Data_Stamina, -Data->StaminaCost, ActorInfo->AvatarActor.Get());
	}
}

void UPCGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsEndAbilityValid(Handle, ActorInfo))
	{
		SetPhase(EPCActionPhase::None);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPCGameplayAbility::EndAsCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UPCGameplayAbility::StartPhase(EPCActionPhase Phase, float Duration)
{
	SetPhase(Phase);

	if (Duration <= 0.f)
	{
		AdvancePhase();
		return;
	}

	UAbilityTask_WaitDelay* Task = UAbilityTask_WaitDelay::WaitDelay(this, Duration);
	Task->OnFinish.AddDynamic(this, &UPCGameplayAbility::AdvancePhase);
	Task->ReadyForActivation();
}

void UPCGameplayAbility::AdvancePhase()
{
	const UPCAbilityData* Data = GetAbilityData();
	if (!Data)
	{
		EndAsCancelled();
		return;
	}

	switch (CurrentPhase)
	{
	case EPCActionPhase::Windup:
		StartPhase(EPCActionPhase::Active, Data->Timing.Active);
		break;
	case EPCActionPhase::Active:
		StartPhase(EPCActionPhase::Recovery, Data->Timing.Recovery);
		break;
	default:
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		break;
	}
}

void UPCGameplayAbility::SetPhase(EPCActionPhase NewPhase)
{
	if (NewPhase == CurrentPhase)
	{
		return;
	}

	const EPCActionPhase OldPhase = CurrentPhase;
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		const FGameplayTag OldTag = PhaseToTag(OldPhase);
		const FGameplayTag NewTag = PhaseToTag(NewPhase);
		if (OldTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(OldTag);
		}
		if (NewTag.IsValid())
		{
			ASC->AddLooseGameplayTag(NewTag);
		}
	}

	CurrentPhase = NewPhase;
	OnPhaseChanged(OldPhase, NewPhase);
}

FGameplayTag UPCGameplayAbility::PhaseToTag(EPCActionPhase Phase)
{
	switch (Phase)
	{
	case EPCActionPhase::Windup:   return PCTags::State_Action_Windup;
	case EPCActionPhase::Active:   return PCTags::State_Action_Active;
	case EPCActionPhase::Recovery: return PCTags::State_Action_Recovery;
	default:                       return FGameplayTag();
	}
}
