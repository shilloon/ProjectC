#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/PCCombatTypes.h"
#include "PCHittable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UPCHittable : public UInterface
{
	GENERATED_BODY()
};

/** 공격 경로에 놓일 수 있는 대상. 캐릭터가 아니어도 구현할 수 있다. */
class PCCOMBAT_API IPCHittable
{
	GENERATED_BODY()

public:
	/** 공격을 받는다. 막았는지 맞았는지는 받는 쪽이 판단한다. */
	virtual EPCHitOutcome ReceiveHit(const FPCHitInfo& Hit) = 0;
};
