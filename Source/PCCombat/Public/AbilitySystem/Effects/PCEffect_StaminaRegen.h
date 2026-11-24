#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/PCGameplayEffect.h"
#include "PCEffect_StaminaRegen.generated.h"

/** 기력 회복. PeriodSeconds마다 SetByCaller(Data.Stamina)만큼 기력을 올린다. 방어 중, 사망 중, 기력을 쓴 직후에는 멈춘다. */
UCLASS()
class PCCOMBAT_API UPCEffect_StaminaRegen : public UPCGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_StaminaRegen();

	/** 회복 주기. 주기당 회복량은 (초당 회복량 * PeriodSeconds)로 넣는다. */
	static constexpr float PeriodSeconds = 0.1f;
};
