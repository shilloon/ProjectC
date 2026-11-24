#pragma once

#include "CoreMinimal.h"
#include "Types/PCCombatTypes.h"

/**
 * 판정 규칙이 있는 유일한 곳.
 * "확정타 없음": 공격은 범위 안에서 처음 닿은 대상 하나에만 적용된다.
 */
struct PCCOMBAT_API FPCHitResolver
{
	/** Target이 Origin에서 Forward 방향 부채꼴(Range, HalfAngleDeg) 안에 있는가. 높이는 무시한다. */
	static bool IsWithinSector(const FVector& Origin, const FVector& Forward, const FVector& Target, float Range, float HalfAngleDeg);

	/** 공격이 방어자의 정면 방어 각도 안에서 왔는가 */
	static bool IsAttackFromFront(const FVector& DefenderLocation, const FVector& DefenderForward, const FVector& AttackOrigin, float GuardHalfAngleDeg);
};
