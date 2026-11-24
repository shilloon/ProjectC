#include "AI/StateTree/PCMonsterStateTreeConditions.h"
#include "AI/PCMonsterAIController.h"
#include "Characters/PCMonsterBase.h"
#include "StateTreeExecutionContext.h"

bool FPCStateTreeCondition_StateTag::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	const APCMonsterAIController* Controller = Cast<APCMonsterAIController>(Data.AIController);
	const APCMonsterBase* Monster = Controller ? Controller->GetMonster() : nullptr;

	const bool bHasTag = Monster && Monster->HasStateTag(Data.Tag);
	return bHasTag != Data.bInvert;
}

bool FPCStateTreeCondition_MonsterCheck::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	APCMonsterAIController* Controller = Cast<APCMonsterAIController>(Data.AIController);
	if (!Controller)
	{
		return false;
	}

	bool bResult = false;
	switch (Data.Check)
	{
	case EPCMonsterCheck::HasTarget:
		bResult = Controller->HasValidTarget();
		break;
	case EPCMonsterCheck::AttackAvailable:
		bResult = Controller->HasAvailableAttack();
		break;
	case EPCMonsterCheck::WithinApproachRange:
		bResult = Controller->IsWithinApproachRange();
		break;
	}
	return bResult != Data.bInvert;
}
