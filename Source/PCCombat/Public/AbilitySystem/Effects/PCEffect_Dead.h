#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/PCGameplayEffect.h"
#include "PCEffect_Dead.generated.h"

/** 사망. 제거될 때까지 State.Dead를 부여한다. */
UCLASS()
class PCCOMBAT_API UPCEffect_Dead : public UPCGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_Dead();
};
