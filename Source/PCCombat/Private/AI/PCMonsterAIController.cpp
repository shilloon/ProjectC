#include "AI/PCMonsterAIController.h"
#include "AI/PCMonsterAttackSelector.h"
#include "Characters/PCMonsterBase.h"
#include "Components/StateTreeAIComponent.h"
#include "Data/PCMonsterData.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PCLog.h"
#include "StateTree.h"

APCMonsterAIController::APCMonsterAIController()
{
	StateTreeAI = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAI"));

	// 몬스터의 능력 시스템이 준비된 뒤 StartBehavior에서 직접 시작한다
	bStartAILogicOnPossess = false;
	StateTreeAI->SetStartLogicAutomatically(false);

	Random.GenerateNewSeed();
}

void APCMonsterAIController::StartBehavior()
{
	const UPCMonsterData* Data = GetMonsterData();
	if (!Data || !Data->BehaviorTree)
	{
		UE_LOG(LogPC, Log, TEXT("'%s': 몬스터 정의나 행동 트리가 없어 AI를 시작하지 않습니다"), *GetNameSafe(GetPawn()));
		return;
	}

	StateTreeAI->SetStateTree(Data->BehaviorTree);
	StateTreeAI->StartLogic();
}

APCMonsterBase* APCMonsterAIController::GetMonster() const
{
	return Cast<APCMonsterBase>(GetPawn());
}

const UPCMonsterData* APCMonsterAIController::GetMonsterData() const
{
	const APCMonsterBase* Monster = GetMonster();
	return Monster ? Monster->GetMonsterData() : nullptr;
}

bool APCMonsterAIController::AcquireTarget()
{
	const APCMonsterBase* Monster = GetMonster();
	const UPCMonsterData* Data = GetMonsterData();
	if (!Monster || !Data || Monster->IsDead())
	{
		return false;
	}

	APCCharacterBase* Nearest = nullptr;
	float NearestDistance = Data->DetectionRange;

	for (TActorIterator<APCCharacterBase> It(GetWorld()); It; ++It)
	{
		APCCharacterBase* Candidate = *It;

		// 몬스터가 아닌 전투 캐릭터(플레이어, 나중의 아군)가 적대 대상이다
		if (Candidate->IsA<APCMonsterBase>() || Candidate->IsDead())
		{
			continue;
		}

		const float Distance = FVector::Dist2D(Monster->GetActorLocation(), Candidate->GetActorLocation());
		if (Distance <= NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Candidate;
		}
	}

	Target = Nearest;
	if (Nearest)
	{
		SetFocus(Nearest);
	}
	return Nearest != nullptr;
}

bool APCMonsterAIController::HasValidTarget()
{
	const APCMonsterBase* Monster = GetMonster();
	const UPCMonsterData* Data = GetMonsterData();
	APCCharacterBase* Current = Target.Get();

	const bool bValid = Monster && Data && Current && !Current->IsDead()
		&& FVector::Dist2D(Monster->GetActorLocation(), Current->GetActorLocation()) <= Data->LoseTargetRange;

	if (!bValid && Target.IsValid())
	{
		Target = nullptr;
		ClearFocus(EAIFocusPriority::Gameplay);
	}
	return bValid;
}

float APCMonsterAIController::GetDistanceToTarget() const
{
	const APawn* Monster = GetPawn();
	const APCCharacterBase* Current = Target.Get();
	if (!Monster || !Current)
	{
		return UE_BIG_NUMBER;
	}
	return FVector::Dist2D(Monster->GetActorLocation(), Current->GetActorLocation()) - Current->GetSimpleCollisionRadius();
}

float APCMonsterAIController::GetAngleToTargetDeg() const
{
	const APawn* Monster = GetPawn();
	const APCCharacterBase* Current = Target.Get();
	if (!Monster || !Current)
	{
		return 180.f;
	}

	FVector ToTarget = Current->GetActorLocation() - Monster->GetActorLocation();
	ToTarget.Z = 0.f;
	FVector Forward = Monster->GetActorForwardVector();
	Forward.Z = 0.f;
	if (!ToTarget.Normalize() || !Forward.Normalize())
	{
		return 0.f;
	}

	return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.f, 1.f)));
}

bool APCMonsterAIController::IsAimedAtTarget() const
{
	const UPCMonsterData* Data = GetMonsterData();
	return Data && GetAngleToTargetDeg() <= Data->AimToleranceDeg;
}

bool APCMonsterAIController::HasAvailableAttack() const
{
	const UPCMonsterData* Data = GetMonsterData();
	if (!Data)
	{
		return false;
	}

	const float Distance = GetDistanceToTarget();
	const float HealthRatio = GetHealthRatio();
	const double Now = GetNow();
	for (const FPCMonsterAttackOption& Option : Data->AttackOptions)
	{
		if (FPCMonsterAttackSelector::IsAvailable(Option, Distance, HealthRatio, Now, LastUsedTimes))
		{
			return true;
		}
	}
	return false;
}

FGameplayTag APCMonsterAIController::SelectAttack()
{
	const UPCMonsterData* Data = GetMonsterData();
	if (!Data)
	{
		return FGameplayTag();
	}

	const int32 Index = FPCMonsterAttackSelector::Select(Data->AttackOptions, GetDistanceToTarget(), GetHealthRatio(), GetNow(), LastUsedTimes, Random);
	return Data->AttackOptions.IsValidIndex(Index) ? Data->AttackOptions[Index].AbilityTag : FGameplayTag();
}

void APCMonsterAIController::NotifyAttackUsed(FGameplayTag AbilityTag)
{
	LastUsedTimes.Add(AbilityTag, GetNow());

	if (const UPCMonsterData* Data = GetMonsterData())
	{
		const FPCMonsterAttackOption* Option = Data->AttackOptions.FindByPredicate(
			[AbilityTag](const FPCMonsterAttackOption& Candidate) { return Candidate.AbilityTag == AbilityTag; });
		PendingRestTime = Option ? Option->RestAfter : 0.f;
	}
}

float APCMonsterAIController::GetApproachRange() const
{
	const UPCMonsterData* Data = GetMonsterData();
	const float Distance = GetDistanceToTarget();
	return Data ? FPCMonsterAttackSelector::GetApproachRange(Data->AttackOptions, Distance, GetHealthRatio(), GetNow(), LastUsedTimes) : Distance;
}

bool APCMonsterAIController::IsWithinApproachRange() const
{
	return GetDistanceToTarget() <= GetApproachRange();
}

float APCMonsterAIController::ConsumeRestTime()
{
	const float Rest = PendingRestTime;
	PendingRestTime = 0.f;
	return Rest;
}

float APCMonsterAIController::GetHealthRatio() const
{
	const APCMonsterBase* Monster = GetMonster();
	return (Monster && Monster->GetMaxHealth() > 0.f) ? Monster->GetHealth() / Monster->GetMaxHealth() : 0.f;
}

double APCMonsterAIController::GetNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}
