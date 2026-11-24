#pragma once

#include "CoreMinimal.h"
#include "Types/PCCombatTypes.h"

/**
 * 판정 규칙이 있는 유일한 곳.
 * "확정타 없음": 공격은 범위 안에 실제로 있는 대상에게만 닿는다.
 */
struct PCCOMBAT_API FPCHitResolver
{
	/** Target이 Origin에서 Forward 방향 부채꼴(Range, HalfAngleDeg) 안에 있는가. 높이는 무시한다. */
	static bool IsWithinSector(const FVector& Origin, const FVector& Forward, const FVector& Target, float Range, float HalfAngleDeg);

	/** 공격이 방어자의 정면 방어 각도 안에서 왔는가 */
	static bool IsAttackFromFront(const FVector& DefenderLocation, const FVector& DefenderForward, const FVector& AttackOrigin, float GuardHalfAngleDeg);

	/** Target이 판정 모양 안에 있는가. ExtraRange에는 대상의 몸 반지름을 넣는다. */
	static bool IsWithinShape(const FPCHitShape& Shape, const FVector& Origin, const FVector& Forward, const FVector& Target, float ExtraRange);

	/** 공격 범위 안의 IPCHittable 액터를 공격자에게 가까운 순서로 모은다 */
	static void FindHittables(const FPCHitInfo& Hit, TArray<AActor*>& OutNearestFirst);

	/** 공격 범위 안에서 공격자에게 가장 가까운 IPCHittable 액터를 찾는다. 없으면 nullptr. */
	static AActor* FindFirstHittable(const FPCHitInfo& Hit);

	/**
	 * "처음 닿은 하나" 규칙으로 공격을 적용한다.
	 * 가까운 순서로 시도해서 처음으로 공격을 받은(막거나 맞은) 대상에서 멈춘다. 없으면 Miss.
	 */
	static EPCHitOutcome ResolveAttack(const FPCHitInfo& Hit, AActor*& OutVictim);
};
