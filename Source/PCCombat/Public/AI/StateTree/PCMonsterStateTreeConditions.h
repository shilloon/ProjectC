#pragma once

#include "CoreMinimal.h"
#include "AI/StateTree/PCMonsterStateTreeTasks.h"
#include "GameplayTagContainer.h"
#include "StateTreeConditionBase.h"
#include "PCMonsterStateTreeConditions.generated.h"

USTRUCT()
struct PCCOMBAT_API FPCStateTreeStateTagInstanceData : public FPCMonsterStateTreeContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter", meta = (Categories = "State"))
	FGameplayTag Tag;

	/** true면 태그가 없을 때 통과한다 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;
};

/** 몬스터가 상태 태그를 가지고 있는가 (경직 중인가, 죽었는가 등) */
USTRUCT(meta = (DisplayName = "PC: 상태 태그 보유", Category = "PC|Monster"))
struct PCCOMBAT_API FPCStateTreeCondition_StateTag : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPCStateTreeStateTagInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ---------------------------------------------------------------------------------------------

/** 몬스터의 전투 상황에 대한 질문 */
UENUM()
enum class EPCMonsterCheck : uint8
{
	/** 유효한 대상이 있는가 */
	HasTarget,
	/** 지금 거리에서 쓸 수 있는 공격이 있는가 */
	AttackAvailable,
	/** 공격할 수 있는 거리 안에 있는가 */
	WithinApproachRange
};

USTRUCT()
struct PCCOMBAT_API FPCStateTreeMonsterCheckInstanceData : public FPCMonsterStateTreeContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Parameter")
	EPCMonsterCheck Check = EPCMonsterCheck::HasTarget;

	/** true면 결과를 뒤집는다 */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	bool bInvert = false;
};

/** 대상 유무, 공격 가능 여부, 접근 거리 같은 전투 상황을 확인한다 */
USTRUCT(meta = (DisplayName = "PC: 전투 상황 확인", Category = "PC|Monster"))
struct PCCOMBAT_API FPCStateTreeCondition_MonsterCheck : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FPCStateTreeMonsterCheckInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};
