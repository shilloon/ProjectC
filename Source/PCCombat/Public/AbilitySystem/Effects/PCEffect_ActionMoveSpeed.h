#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/PCGameplayEffect.h"
#include "PCEffect_ActionMoveSpeed.generated.h"

/** 기술을 쓰는 동안의 이동 속도 배율. 제거될 때까지 MoveSpeedMultiplier에 SetByCaller(Data.MoveSpeed)를 곱한다. */
UCLASS()
class PCCOMBAT_API UPCEffect_ActionMoveSpeed : public UPCGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_ActionMoveSpeed();
};
