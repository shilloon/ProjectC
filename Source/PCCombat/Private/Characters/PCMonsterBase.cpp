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
	/** 정의 에셋이 없을 때의 회전 속도 (도/초) */
	constexpr float DefaultTurnRate = 360.f;

	/** 급출발과 급정지를 줄이기 위한 가속과 감속 */
	constexpr float MonsterAcceleration = 1200.f;
	constexpr float MonsterBrakingDeceleration = 700.f;
}

APCMonsterBase::APCMonsterBase()
{
	AIControllerClass = APCMonsterAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// 이동 방향이 아니라 AI가 바라보는 대상 쪽으로 부드럽게 돈다
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, DefaultTurnRate, 0.f);

	// 경로를 따라갈 때도 가속과 감속을 거쳐서 움직임이 뚝뚝 끊기지 않게 한다
	GetCharacterMovement()->bRequestedMoveUseAcceleration = true;
	GetCharacterMovement()->MaxAcceleration = MonsterAcceleration;
	GetCharacterMovement()->BrakingDecelerationWalking = MonsterBrakingDeceleration;

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
	GetCharacterMovement()->RotationRate = FRotator(0.f, MonsterData->TurnRate, 0.f);

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
