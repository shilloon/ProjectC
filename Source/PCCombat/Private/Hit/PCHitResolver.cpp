#include "Hit/PCHitResolver.h"
#include "Hit/PCHittable.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Data/PCAbilityData.h"
#include "PCGameplayTags.h"
#include "PCLog.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** 범위 검색에 더하는 여유. 대상의 몸 반지름만큼 멀리 있어도 후보에 들어오게 한다. */
	constexpr float MaxTargetRadius = 150.f;

	/** 원형 판정은 방향을 가리지 않는다 */
	constexpr float FullCircleHalfAngleDeg = 180.f;

	struct FPCHitCandidate
	{
		AActor* Actor = nullptr;
		float Distance = 0.f;
	};
}

bool FPCHitResolver::IsWithinSector(const FVector& Origin, const FVector& Forward, const FVector& Target, float Range, float HalfAngleDeg)
{
	FVector ToTarget = Target - Origin;
	ToTarget.Z = 0.f;

	const float Distance = ToTarget.Size();
	if (Distance > Range)
	{
		return false;
	}
	if (Distance < KINDA_SMALL_NUMBER)
	{
		return true;
	}

	FVector FlatForward = Forward;
	FlatForward.Z = 0.f;
	if (!FlatForward.Normalize())
	{
		return false;
	}

	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(HalfAngleDeg));
	return FVector::DotProduct(FlatForward, ToTarget / Distance) >= CosHalfAngle;
}

bool FPCHitResolver::IsAttackFromFront(const FVector& DefenderLocation, const FVector& DefenderForward, const FVector& AttackOrigin, float GuardHalfAngleDeg)
{
	return IsWithinSector(DefenderLocation, DefenderForward, AttackOrigin, UE_BIG_NUMBER, GuardHalfAngleDeg);
}

bool FPCHitResolver::IsWithinShape(const FPCHitShape& Shape, const FVector& Origin, const FVector& Forward, const FVector& Target, float ExtraRange)
{
	const float HalfAngle = Shape.ShapeType == EPCHitShapeType::Circle ? FullCircleHalfAngleDeg : Shape.HalfAngleDeg;
	return IsWithinSector(Origin, Forward, Target, Shape.Range + ExtraRange, HalfAngle);
}

void FPCHitResolver::FindHittables(const FPCHitInfo& Hit, TArray<AActor*>& OutNearestFirst)
{
	OutNearestFirst.Reset();

	if (!Hit.Attacker || !Hit.Data)
	{
		return;
	}

	UWorld* World = Hit.Attacker->GetWorld();
	if (!World)
	{
		return;
	}

	const FPCHitShape& Shape = Hit.Data->HitShape;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PCHitResolver), false, Hit.Attacker);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Hit.Origin, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(Shape.Range + MaxTargetRadius), QueryParams);

	TArray<FPCHitCandidate> Candidates;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || !Actor->Implements<UPCHittable>())
		{
			continue;
		}
		// 한 액터의 컴포넌트가 여러 개 겹쳐도 한 번만 센다
		if (Candidates.ContainsByPredicate([Actor](const FPCHitCandidate& Candidate) { return Candidate.Actor == Actor; }))
		{
			continue;
		}

		const float TargetRadius = Actor->GetSimpleCollisionRadius();
		const FVector TargetLocation = Actor->GetActorLocation();
		if (!IsWithinShape(Shape, Hit.Origin, Hit.Direction, TargetLocation, TargetRadius))
		{
			continue;
		}

		Candidates.Add({ Actor, static_cast<float>(FVector::Dist2D(Hit.Origin, TargetLocation)) - TargetRadius });
	}

	Candidates.Sort([](const FPCHitCandidate& A, const FPCHitCandidate& B) { return A.Distance < B.Distance; });

	for (const FPCHitCandidate& Candidate : Candidates)
	{
		OutNearestFirst.Add(Candidate.Actor);
	}
}

AActor* FPCHitResolver::FindFirstHittable(const FPCHitInfo& Hit)
{
	TArray<AActor*> Candidates;
	FindHittables(Hit, Candidates);
	return Candidates.Num() > 0 ? Candidates[0] : nullptr;
}

EPCHitOutcome FPCHitResolver::ApplyHit(AActor* Target, const FPCHitInfo& Hit)
{
	IPCHittable* Hittable = Cast<IPCHittable>(Target);
	const EPCHitOutcome Outcome = (Hittable && Hit.Data) ? Hittable->ReceiveHit(Hit) : EPCHitOutcome::Miss;
	if (Outcome == EPCHitOutcome::Miss)
	{
		return Outcome;
	}

	// 때린 쪽의 겉모습(타격 멈춤 등)에 공격이 닿았음을 알린다
	if (UAbilitySystemComponent* AttackerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Hit.Attacker))
	{
		FGameplayCueParameters Parameters;
		Parameters.Instigator = Target;
		Parameters.SourceObject = Hit.Data;
		Parameters.Location = Target->GetActorLocation();
		Parameters.RawMagnitude = Outcome == EPCHitOutcome::Blocked ? 0.f : 1.f;
		AttackerASC->ExecuteGameplayCue(PCTags::GameplayCue_Combat_HitConfirm, Parameters);
	}

	UE_LOG(LogPC, Log, TEXT("[전투] %s의 %s → %s: %s"), *GetNameSafe(Hit.Attacker), *Hit.Data->AbilityTag.ToString(), *GetNameSafe(Target),
		Outcome == EPCHitOutcome::Blocked ? TEXT("막힘") : TEXT("맞음"));
	return Outcome;
}

EPCHitOutcome FPCHitResolver::ResolveAttack(const FPCHitInfo& Hit, AActor*& OutVictim)
{
	OutVictim = nullptr;

	TArray<AActor*> Candidates;
	FindHittables(Hit, Candidates);

	for (AActor* Candidate : Candidates)
	{
		IPCHittable* Hittable = Cast<IPCHittable>(Candidate);
		const EPCHitOutcome Outcome = Hittable ? Hittable->ReceiveHit(Hit) : EPCHitOutcome::Miss;

		// 이미 죽은 대상처럼 공격을 받지 않는 대상은 가리지 않으므로 다음 대상으로 넘어간다
		if (Outcome != EPCHitOutcome::Miss)
		{
			OutVictim = Candidate;
			return Outcome;
		}
	}

	return EPCHitOutcome::Miss;
}
