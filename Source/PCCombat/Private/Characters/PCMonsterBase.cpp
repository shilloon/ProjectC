#include "Characters/PCMonsterBase.h"
#include "AI/PCMonsterAIController.h"
#include "AbilitySystem/Attributes/PCCombatAttributeSet.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/PCAbilityData.h"
#include "Data/PCMonsterData.h"
#include "Data/PCMonsterRankRow.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Presentation/PCCombatPresenterComponent.h"

namespace
{
	/** 대상을 따라 몸을 돌리는 속도 (도/초) */
	constexpr float MonsterTurnRate = 360.f;
}

APCMonsterBase::APCMonsterBase()
{
	AIControllerClass = APCMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 이동 방향이 아니라 AI가 바라보는 대상 쪽으로 부드럽게 돈다
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, MonsterTurnRate, 0.f);

	Presenter->bShowWeapons = false;
}

void APCMonsterBase::BeginPlay()
{
	Super::BeginPlay();

	// 능력 시스템과 수치가 준비된 뒤에 행동을 시작한다
	if (APCMonsterAIController* MonsterController = Cast<APCMonsterAIController>(GetController()))
	{
		MonsterController->StartBehavior();
	}
}

FDataTableRowHandle APCMonsterBase::GetStatsRowHandle() const
{
	return MonsterData ? MonsterData->StatsRow : Super::GetStatsRowHandle();
}

void APCMonsterBase::GetStartingAbilities(TArray<const UPCAbilityData*>& OutAbilities) const
{
	Super::GetStartingAbilities(OutAbilities);
	if (MonsterData)
	{
		OutAbilities.Append(MonsterData->Abilities);
	}
}

void APCMonsterBase::OnStatsInitialized()
{
	Super::OnStatsInitialized();

	if (!MonsterData)
	{
		return;
	}

	ApplyRank();
	AbilitySystemComponent->AddLooseGameplayTags(MonsterData->HabitatTags);

	if (USkeletalMesh* MeshOverride = MonsterData->Mesh.LoadSynchronous())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshOverride);
	}
	SetActorScale3D(FVector(MonsterData->Scale));
}

void APCMonsterBase::ApplyRank()
{
	if (MonsterData->RankRow.IsNull())
	{
		return;
	}

	const FPCMonsterRankRow* Rank = MonsterData->RankRow.GetRow<FPCMonsterRankRow>(TEXT("APCMonsterBase"));
	if (!Rank)
	{
		return;
	}

	const float MaxHealth = VitalSet->GetMaxHealth() * Rank->HealthMultiplier;
	VitalSet->InitMaxHealth(MaxHealth);
	VitalSet->InitHealth(MaxHealth);

	CombatSet->InitAttackPower(Rank->DamageMultiplier);
	CombatSet->InitStaggerResistance(1.f - Rank->StaggerTimeMultiplier);

	if (Rank->RankTag.IsValid())
	{
		AbilitySystemComponent->AddLooseGameplayTag(Rank->RankTag);
	}
}
