#include "Hit/PCHitResolver.h"

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
