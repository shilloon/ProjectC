#include "Hit/PCHitResolver.h"
#include "Hit/PCHittable.h"
#include "Data/PCAbilityData.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace
{
	/** 범위 검색에 더하는 여유. 대상의 몸 반지름만큼 멀리 있어도 후보에 들어오게 한다. */
	constexpr float MaxTargetRadius = 150.f;
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

AActor* FPCHitResolver::FindFirstHittable(const FPCHitInfo& Hit)
{
	if (!Hit.Attacker || !Hit.Data)
	{
		return nullptr;
	}

	UWorld* World = Hit.Attacker->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FPCHitShape& Shape = Hit.Data->HitShape;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PCHitResolver), false, Hit.Attacker);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Hit.Origin, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(Shape.Range + MaxTargetRadius), QueryParams);

	AActor* Nearest = nullptr;
	float NearestDistance = UE_BIG_NUMBER;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == Nearest || !Candidate->Implements<UPCHittable>())
		{
			continue;
		}

		const float TargetRadius = Candidate->GetSimpleCollisionRadius();
		const FVector TargetLocation = Candidate->GetActorLocation();
		if (!IsWithinSector(Hit.Origin, Hit.Direction, TargetLocation, Shape.Range + TargetRadius, Shape.HalfAngleDeg))
		{
			continue;
		}

		const float Distance = FVector::Dist2D(Hit.Origin, TargetLocation) - TargetRadius;
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Candidate;
		}
	}

	return Nearest;
}

EPCHitOutcome FPCHitResolver::ResolveAttack(const FPCHitInfo& Hit, AActor*& OutVictim)
{
	OutVictim = FindFirstHittable(Hit);
	if (IPCHittable* Hittable = Cast<IPCHittable>(OutVictim))
	{
		return Hittable->ReceiveHit(Hit);
	}
	return EPCHitOutcome::Miss;
}
