#include "AbilitySystem/Abilities/PCAbility_MeleeAttack.h"
#include "AbilitySystemComponent.h"
#include "Data/PCAbilityData.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Hit/PCHitResolver.h"
#include "PCGameplayTags.h"
#include "PCLog.h"
#include "TimerManager.h"

namespace
{
	/** 판정 구간 동안 범위를 다시 훑는 간격 */
	constexpr float HitCheckInterval = 0.02f;
}

UPCAbility_MeleeAttack::UPCAbility_MeleeAttack()
{
	SetAssetTags(FGameplayTagContainer(PCTags::Ability_Attack));
}

void UPCAbility_MeleeAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	const UPCAbilityData* Data = GetAbilityData();
	if (!Data || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	bHasHit = false;
	StartPhase(EPCActionPhase::Windup, Data->Timing.Windup);
}

void UPCAbility_MeleeAttack::OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase)
{
	if (OldPhase == EPCActionPhase::Active)
	{
		StopHitChecks();
		if (!bHasHit && NewPhase == EPCActionPhase::Recovery)
		{
			const UPCAbilityData* Data = GetAbilityData();
			UE_LOG(LogPC, Log, TEXT("[전투] %s의 %s: 빗나감"), *GetNameSafe(GetAvatarActorFromActorInfo()), Data ? *Data->AbilityTag.ToString() : TEXT("?"));
		}
	}

	if (NewPhase == EPCActionPhase::Windup)
	{
		ExecuteWindupCue();
	}
	else if (NewPhase == EPCActionPhase::Active)
	{
		TryHit();
		if (!bHasHit && GetCurrentPhase() == EPCActionPhase::Active)
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(HitCheckTimer, this, &UPCAbility_MeleeAttack::TryHit, HitCheckInterval, true);
			}
		}
	}
}

void UPCAbility_MeleeAttack::ExecuteWindupCue()
{
	const UPCAbilityData* Data = GetAbilityData();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Data || !Avatar || !ASC)
	{
		return;
	}

	FGameplayCueParameters Parameters;
	Parameters.Instigator = Avatar;
	Parameters.SourceObject = Data;
	Parameters.Location = Avatar->GetActorLocation();
	Parameters.RawMagnitude = Data->Timing.Windup;
	ASC->ExecuteGameplayCue(PCTags::GameplayCue_Combat_Windup, Parameters);
}

void UPCAbility_MeleeAttack::TryHit()
{
	const UPCAbilityData* Data = GetAbilityData();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Data || !Avatar || bHasHit)
	{
		return;
	}

	FPCHitInfo Hit;
	Hit.Attacker = Avatar;
	Hit.Origin = Avatar->GetActorLocation();
	Hit.Direction = Avatar->GetActorForwardVector();
	Hit.Data = Data;

	AActor* Victim = nullptr;
	const EPCHitOutcome Outcome = FPCHitResolver::ResolveAttack(Hit, Victim);
	if (Outcome != EPCHitOutcome::Miss)
	{
		bHasHit = true;
		StopHitChecks();
		UE_LOG(LogPC, Log, TEXT("[전투] %s의 %s → %s: %s"), *GetNameSafe(Avatar), *Data->AbilityTag.ToString(), *GetNameSafe(Victim),
			Outcome == EPCHitOutcome::Blocked ? TEXT("막힘") : TEXT("맞음"));
	}
}

void UPCAbility_MeleeAttack::StopHitChecks()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitCheckTimer);
	}
}
