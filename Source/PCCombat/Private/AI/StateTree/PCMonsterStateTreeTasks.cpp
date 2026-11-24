#include "AI/StateTree/PCMonsterStateTreeTasks.h"
#include "AI/PCMonsterAIController.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Characters/PCMonsterBase.h"
#include "Navigation/PathFollowingComponent.h"
#include "PCGameplayTags.h"
#include "StateTreeExecutionContext.h"

namespace
{
	/** 이동 요청의 도착 판정 거리. 실제 멈춤은 태스크가 공격 거리로 판단한다. */
	constexpr float MoveAcceptanceRadius = 10.f;

	APCMonsterAIController* GetMonsterController(const FPCMonsterStateTreeContext& Data)
	{
		return Cast<APCMonsterAIController>(Data.AIController);
	}

	bool CanAct(const APCMonsterBase* Monster)
	{
		return Monster && !Monster->IsDead() && !Monster->HasStateTag(PCTags::State_Staggered);
	}

	/** 이동을 요청한다. 길찾기도 직선 경로도 받아들여지지 않으면 false. */
	bool RequestMove(APCMonsterAIController& Controller)
	{
		AActor* Target = Controller.GetTarget();
		EPathFollowingRequestResult::Type Result = Controller.MoveToActor(Target, MoveAcceptanceRadius, false, true, false);
		if (Result == EPathFollowingRequestResult::Failed)
		{
			// 내비게이션 메시가 없는 곳에서는 대상 방향으로 직진한다
			Result = Controller.MoveToActor(Target, MoveAcceptanceRadius, false, false, false);
		}
		return Result != EPathFollowingRequestResult::Failed;
	}

	/** 경로 추종 없이 대상 방향으로 한 프레임 걸어간다 */
	void SteerTowardTarget(const APCMonsterAIController& Controller)
	{
		APawn* Monster = Controller.GetPawn();
		const AActor* Target = Controller.GetTarget();
		if (Monster && Target)
		{
			FVector ToTarget = Target->GetActorLocation() - Monster->GetActorLocation();
			ToTarget.Z = 0.f;
			Monster->AddMovementInput(ToTarget.GetSafeNormal());
		}
	}
}

// ---- 대상 찾기 ----

EStateTreeRunStatus FPCStateTreeTask_AcquireTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	APCMonsterAIController* Controller = GetMonsterController(Data);
	if (!Controller)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.TimeUntilRetry = Data.RetryInterval;
	return Controller->AcquireTarget() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPCStateTreeTask_AcquireTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	APCMonsterAIController* Controller = GetMonsterController(Data);
	if (!Controller)
	{
		return EStateTreeRunStatus::Failed;
	}

	Data.TimeUntilRetry -= DeltaTime;
	if (Data.TimeUntilRetry > 0.f)
	{
		return EStateTreeRunStatus::Running;
	}

	Data.TimeUntilRetry = Data.RetryInterval;
	return Controller->AcquireTarget() ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}

// ---- 대상에게 접근 ----

EStateTreeRunStatus FPCStateTreeTask_MoveToTarget::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	APCMonsterAIController* Controller = GetMonsterController(Data);
	if (!Controller || !CanAct(Controller->GetMonster()) || !Controller->HasValidTarget())
	{
		return EStateTreeRunStatus::Failed;
	}
	if (Controller->IsWithinApproachRange())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	Data.bSteeringDirectly = !RequestMove(*Controller);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPCStateTreeTask_MoveToTarget::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	APCMonsterAIController* Controller = GetMonsterController(Data);
	if (!Controller || !CanAct(Controller->GetMonster()) || !Controller->HasValidTarget())
	{
		return EStateTreeRunStatus::Failed;
	}
	if (Controller->IsWithinApproachRange())
	{
		return EStateTreeRunStatus::Succeeded;
	}

	if (Data.bSteeringDirectly)
	{
		SteerTowardTarget(*Controller);
	}
	else if (Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		// 이동이 끝났는데 아직 멀면 다시 요청한다
		Data.bSteeringDirectly = !RequestMove(*Controller);
	}
	return EStateTreeRunStatus::Running;
}

void FPCStateTreeTask_MoveToTarget::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	if (APCMonsterAIController* Controller = GetMonsterController(Context.GetInstanceData(*this)))
	{
		Controller->StopMovement();
	}
}

// ---- 공격 사용 ----

EStateTreeRunStatus FPCStateTreeTask_UseAttack::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	APCMonsterAIController* Controller = GetMonsterController(Data);
	APCMonsterBase* Monster = Controller ? Controller->GetMonster() : nullptr;
	if (!CanAct(Monster) || !Controller->HasValidTarget())
	{
		return EStateTreeRunStatus::Failed;
	}

	const FGameplayTag AbilityTag = Data.AbilityTag.IsValid() ? Data.AbilityTag : Controller->SelectAttack();
	if (!AbilityTag.IsValid())
	{
		return EStateTreeRunStatus::Failed;
	}

	// 대상을 바라본 뒤 방향을 고정한다. 공격이 끝나면 ExitState에서 다시 대상을 따라 돈다.
	Controller->FaceTarget();
	Controller->ClearFocus(EAIFocusPriority::Gameplay);

	if (!Monster->GetPCAbilitySystemComponent()->TryActivateByAbilityTag(AbilityTag))
	{
		return EStateTreeRunStatus::Failed;
	}

	Controller->NotifyAttackUsed(AbilityTag);
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FPCStateTreeTask_UseAttack::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	APCMonsterAIController* Controller = GetMonsterController(Context.GetInstanceData(*this));
	const APCMonsterBase* Monster = Controller ? Controller->GetMonster() : nullptr;
	if (!Monster)
	{
		return EStateTreeRunStatus::Failed;
	}

	// 공격이 끝났거나 경직으로 끊기면 행동 태그가 사라진다
	return Monster->HasStateTag(PCTags::State_Action) ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

void FPCStateTreeTask_UseAttack::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	APCMonsterAIController* Controller = GetMonsterController(Context.GetInstanceData(*this));
	if (Controller && Controller->GetTarget())
	{
		Controller->SetFocus(Controller->GetTarget());
	}
}

// ---- 숨 고르기 ----

EStateTreeRunStatus FPCStateTreeTask_Rest::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	APCMonsterAIController* Controller = GetMonsterController(Data);

	Data.Remaining = Data.Duration;
	if (Data.bAddAttackRest && Controller)
	{
		Data.Remaining += Controller->ConsumeRestTime();
	}
	return Data.Remaining > 0.f ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}

EStateTreeRunStatus FPCStateTreeTask_Rest::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.Remaining -= DeltaTime;
	return Data.Remaining > 0.f ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded;
}
