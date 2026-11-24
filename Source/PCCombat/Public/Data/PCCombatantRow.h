#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PCCombatantRow.generated.h"

/** 캐릭터와 몬스터의 기본 수치. 데이터테이블 한 행이 한 종류다. */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCCombatantRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = 1))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = 0))
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = 0))
	float StaminaRegenPerSec = 25.f;

	/** 기력을 쓴 뒤 회복이 시작되기까지의 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vital", meta = (ClampMin = 0, Units = "s"))
	float StaminaRegenDelay = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement", meta = (ClampMin = 0, Units = "cm/s"))
	float MoveSpeed = 500.f;
};
