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
	 * 지금 쓸 수 있는 공격 중 가장 긴 사거리이고, 전부 재사용 대기 중이면 대기 중인 것까지 포함해서 본다.
	 */
	static float GetApproachRange(TConstArrayView<FPCMonsterAttackOption> Options, float HealthRatio, double Now,
		const TMap<FGameplayTag, double>& LastUsedTimes);
};
