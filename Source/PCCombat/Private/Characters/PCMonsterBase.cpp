#include "Characters/PCMonsterBase.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "PCGameplayTags.h"
#include "Presentation/PCCombatPresenterComponent.h"
#include "TimerManager.h"

APCMonsterBase::APCMonsterBase()
{
	Presenter->bShowWeapons = false;
}

void APCMonsterBase::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoAttack && AutoAttackTag.IsValid())
	{
		GetWorldTimerManager().SetTimer(AutoAttackTimer, this, &APCMonsterBase::TryAutoAttack, AutoAttackInterval, true);
	}
}

void APCMonsterBase::TryAutoAttack()
{
	if (IsDead() || HasStateTag(PCTags::State_Staggered) || HasStateTag(PCTags::State_Action))
	{
		return;
	}

	if (const APawn* Target = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
		ToTarget.Z = 0.f;
		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(ToTarget.Rotation());
		}
	}

	AbilitySystemComponent->TryActivateByAbilityTag(AutoAttackTag);
}
