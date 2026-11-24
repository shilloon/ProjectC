#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "PCMonsterAIController.generated.h"

class APCCharacterBase;
class APCMonsterBase;
class UPCMonsterData;
class UStateTreeAIComponent;

/**
 * 몬스터의 AI 컨트롤러.
 * 행동의 뼈대는 StateTree가 정하고, 이 클래스는 트리의 부품들이 쓰는 판단 재료를 제공한다:
 * 대상 찾기, 대상과의 거리, 몬스터 정의의 공격 목록에서 공격 고르기.
 */
UCLASS()
class PCCOMBAT_API APCMonsterAIController : public AAIController
{
	GENERATED_BODY()

public:
	APCMonsterAIController();

	/** 몬스터가 초기화를 마친 뒤 호출한다. 몬스터 정의의 StateTree로 행동을 시작한다. */
	void StartBehavior();

	APCMonsterBase* GetMonster() const;
	const UPCMonsterData* GetMonsterData() const;

	// ---- 대상 ----

	/** 발견 범위 안에서 가장 가까운 적대 대상을 고른다. 찾았으면 true. */
	bool AcquireTarget();

	/** 대상이 아직 유효한가. 죽었거나 너무 멀어졌으면 놓는다. */
	bool HasValidTarget();

	APCCharacterBase* GetTarget() const { return Target.Get(); }

	/** 대상의 몸 표면까지의 수평 거리. 대상이 없으면 아주 큰 값. */
	float GetDistanceToTarget() const;

	/** 몬스터의 정면과 대상 방향 사이의 수평 각도 (0~180). 대상이 없으면 180. */
	float GetAngleToTargetDeg() const;

	/** 대상이 조준 허용 각도 안에 있는가 */
	bool IsAimedAtTarget() const;

	// ---- 공격 선택 ----

	/** 지금 거리에서 쓸 수 있는 공격이 있는가 */
	bool HasAvailableAttack() const;

	/** 쓸 수 있는 공격 중 하나를 고른다. 없으면 빈 태그. */
	FGameplayTag SelectAttack();

	/** 공격을 썼음을 기록한다. 재사용 시간과 숨 고르기 시간이 여기서 정해진다. */
	void NotifyAttackUsed(FGameplayTag AbilityTag);

	/** 공격하려면 다가가야 하는 거리 */
	float GetApproachRange() const;

	bool IsWithinApproachRange() const;

	/** 마지막 공격이 요구한 숨 고르기 시간을 가져가고 0으로 되돌린다 */
	float ConsumeRestTime();

private:
	float GetHealthRatio() const;
	double GetNow() const;

	UPROPERTY(VisibleAnywhere, Category = "PC|AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAI;

	TWeakObjectPtr<APCCharacterBase> Target;
	TMap<FGameplayTag, double> LastUsedTimes;
	float PendingRestTime = 0.f;
	FRandomStream Random;
};
