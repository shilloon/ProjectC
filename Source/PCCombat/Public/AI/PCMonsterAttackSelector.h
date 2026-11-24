#pragma once

#include "CoreMinimal.h"
#include "Data/PCMonsterData.h"

/**
 * 몬스터의 공격 선택 규칙. 상태를 갖지 않는 계산이라 월드 없이 테스트할 수 있다.
 */
struct PCCOMBAT_API FPCMonsterAttackSelector
{
	/** 지금 고를 수 있는 공격인가 (거리, 체력 구간, 재사용 시간, 비중) */
	static bool IsAvailable(const FPCMonsterAttackOption& Option, float Distance, float HealthRatio, double Now, const TMap<FGameplayTag, double>& LastUsedTimes);

	/** 고를 수 있는 공격 중 하나를 비중에 따라 뽑는다. 없으면 INDEX_NONE. */
	static int32 Select(TConstArrayView<FPCMonsterAttackOption> Options, float Distance, float HealthRatio, double Now,
		const TMap<FGameplayTag, double>& LastUsedTimes, FRandomStream& Random);

	/**
	 * 공격하려면 대상에게 이 거리 안으로 다가가야 한다.
	 * 지금 거리(Distance)보다 가까이 가야 쓸 수 있는 공격 중 사거리가 가장 긴 것을 고른다.
	 * 쓸 수 있는 것이 없으면 재사용 대기 중인 것까지 포함해서 보고, 그것도 없으면 Distance를 그대로 돌려준다.
	 * 공격들의 사거리 사이에 빈 구간이 있어도(예: 180 이하와 300 이상) 그 안에 멈춰 서지 않는다.
	 */
	static float GetApproachRange(TConstArrayView<FPCMonsterAttackOption> Options, float Distance, float HealthRatio, double Now,
		const TMap<FGameplayTag, double>& LastUsedTimes);
};
