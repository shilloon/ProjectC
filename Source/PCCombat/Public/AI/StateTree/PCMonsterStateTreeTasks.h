#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreeTaskBase.h"
#include "PCMonsterStateTreeTasks.generated.h"

class AAIController;

/**
 * 몬스터 StateTree 부품이 공통으로 받는 컨텍스트.
 * AI 컨트롤러는 트리의 컨텍스트에서 자동으로 연결된다. 부품은 APCMonsterAIController로 캐스팅해서 쓴다.
 */
USTRUCT()
struct PCCOMBAT_API FPCMonsterStateTreeContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AAIController> AIController;
};

// ---------------------------------------------------------------------------------------------

USTRUCT()
struct PCCOMBAT_API FPCStateTreeAcquireTargetInstanceData : public FPCMonsterStateTreeContext
{
	GENERATED_BODY()

	/** 대상을 못 찾았을 때 다시 찾아보는 간격 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0.05, Units = "s"))
	float RetryInterval = 0.3f;

	UPROPERTY()
	float TimeUntilRetry = 0.f;
};

/** 발견 범위 안의 적대 대상을 찾을 때까지 기다린다. 찾으면 성공으로 끝난다. */
USTRUCT(meta = (DisplayName = "PC: 대상 찾기", Category = "PC|Monster"))
struct PCCOMBAT_API FPCStateTreeTask_AcquireTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPCStateTreeAcquireTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};

// ---------------------------------------------------------------------------------------------

USTRUCT()
struct PCCOMBAT_API FPCStateTreeMoveToTargetInstanceData : public FPCMonsterStateTreeContext
{
	GENERATED_BODY()
};

/**
 * 공격할 수 있는 거리까지 대상에게 다가간다. 거리는 몬스터 정의의 공격 목록에서 나온다.
 * 길찾기를 쓰고, 내비게이션 메시가 없으면 대상 방향으로 직진한다.
 */
USTRUCT(meta = (DisplayName = "PC: 대상에게 접근", Category = "PC|Monster"))
struct PCCOMBAT_API FPCStateTreeTask_MoveToTarget : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPCStateTreeMoveToTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------------------------

USTRUCT()
struct PCCOMBAT_API FPCStateTreeUseAttackInstanceData : public FPCMonsterStateTreeContext
{
	GENERATED_BODY()

	/** 비워 두면 몬스터 정의의 공격 목록에서 고른다. 지정하면 그 기술을 쓴다 (정해진 연속 공격을 만들 때). */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;
};

/**
 * 대상을 바라보고 공격 하나를 쓴 뒤, 그 공격이 끝날 때까지 기다린다.
 * 공격이 시작되면 방향이 고정되므로, 대상이 옆으로 비켜서면 빗나간다.
 */
USTRUCT(meta = (DisplayName = "PC: 공격 사용", Category = "PC|Monster"))
struct PCCOMBAT_API FPCStateTreeTask_UseAttack : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPCStateTreeUseAttackInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ---------------------------------------------------------------------------------------------

USTRUCT()
struct PCCOMBAT_API FPCStateTreeRestInstanceData : public FPCMonsterStateTreeContext
{
	GENERATED_BODY()

	/** 0이면 마지막 공격이 요구한 숨 고르기 시간을 쓴다 */
	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (ClampMin = 0, Units = "s"))
	float Duration = 0.f;

	UPROPERTY()
	float Remaining = 0.f;
};

/** 정해진 시간 동안 아무것도 하지 않는다 */
USTRUCT(meta = (DisplayName = "PC: 숨 고르기", Category = "PC|Monster"))
struct PCCOMBAT_API FPCStateTreeTask_Rest : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPCStateTreeRestInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
