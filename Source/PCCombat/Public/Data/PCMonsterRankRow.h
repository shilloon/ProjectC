#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PCMonsterRankRow.generated.h"

/** 몬스터 등급 하나의 규칙. 같은 종류의 몬스터도 이 행만 바꾸면 엘리트나 보스가 된다. */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCMonsterRankRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 이 등급의 몬스터에게 붙는 태그 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rank", meta = (Categories = "Monster.Rank"))
	FGameplayTag RankTag;

	/** 최대 체력에 곱한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rank", meta = (ClampMin = 0))
	float HealthMultiplier = 1.f;

	/** 주는 피해에 곱한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rank", meta = (ClampMin = 0))
	float DamageMultiplier = 1.f;

	/** 받는 경직 시간에 곱한다. 0이면 경직되지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rank", meta = (ClampMin = 0, ClampMax = 1))
	float StaggerTimeMultiplier = 1.f;
};
