#include "AbilitySystem/Abilities/PCAbility_RangedAttack.h"
#include "Characters/PCCharacterBase.h"
#include "Data/PCAbilityData.h"
#include "Engine/World.h"
#include "Hit/PCProjectile.h"
#include "PCGameplayTags.h"
#include "PCLog.h"

UPCAbility_RangedAttack::UPCAbility_RangedAttack()
{
	SetAssetTags(FGameplayTagContainer(PCTags::Ability_Attack));

	// 겨누는 동안 캐릭터가 조준 방향을 바라본다
	ActivationOwnedTags.AddTag(PCTags::State_Aiming);
}

void UPCAbility_RangedAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!GetAbilityData() || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	StartPhase(EPCActionPhase::Windup, GetPhaseDuration(EPCActionPhase::Windup));
}

void UPCAbility_RangedAttack::OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase)
{
	if (NewPhase == EPCActionPhase::Windup)
	{
		ExecuteWindupCue();
	}
	else if (NewPhase == EPCActionPhase::Active)
	{
		Fire();
	}
}

void UPCAbility_RangedAttack::Fire()
{
	const UPCAbilityData* Data = GetAbilityData();
	APCCharacterBase* Character = GetPCCharacter();
	UWorld* World = GetWorld();
	if (!Data || !Character || !World)
	{
		return;
	}

	const FPCProjectileSpec& Spec = Data->Projectile;

	// 먼저 손 높이에서 겨눈 방향을 구하고, 그 방향으로 조금 띄운 곳에서 쏜다
	const FVector Hand = Character->GetActorLocation() + FVector(0.f, 0.f, Spec.SpawnHeightOffset);
	const FVector Direction = Character->GetAimDirection(Hand);
	const FVector Start = Hand + Direction * Spec.SpawnForwardOffset;

	FPCHitInfo Hit;
	Hit.Attacker = Character;
	Hit.Origin = Start;
	Hit.Direction = Direction;
	Hit.Data = Data;
	Hit.DamageMultiplier = Character->GetAttackPower();

	const TSubclassOf<APCProjectile> ProjectileClass = Spec.ProjectileClass ? Spec.ProjectileClass : TSubclassOf<APCProjectile>(APCProjectile::StaticClass());
	const FTransform Transform(Direction.Rotation(), Start);
	APCProjectile* Projectile = World->SpawnActorDeferred<APCProjectile>(ProjectileClass, Transform, Character, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Projectile)
	{
		return;
	}

	Projectile->Launch(Hit, Spec);
	Projectile->FinishSpawning(Transform);

	UE_LOG(LogPC, Log, TEXT("[전투] %s의 %s: 발사"), *GetNameSafe(Character), *Data->AbilityTag.ToString());
}
