#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/PCGameplayEffect.h"
#include "PCEffect_StaminaRegenDelay.generated.h"

/** 기력을 쓴 직후의 회복 지연. SetByCaller(Data.Duration) 동안 State.StaminaRegenPaused를 부여한다. 다시 적용하면 시간이 갱신된다. */
UCLASS()
class PCCOMBAT_API UPCEffect_StaminaRegenDelay : public UPCGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_StaminaRegenDelay();
};
