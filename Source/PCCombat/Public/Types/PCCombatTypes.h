#pragma once

#include "CoreMinimal.h"
#include "PCCombatTypes.generated.h"

class AActor;
class UPCAbilityData;

/** 공격 판정 결과 */
UENUM(BlueprintType)
enum class EPCHitOutcome : uint8
{
	Miss,
	Hit,
	Blocked
};

/** 행동의 진행 단계 */
UENUM(BlueprintType)
enum class EPCActionPhase : uint8
{
	None,
	Windup,
	Active,
	Recovery
};

/** 행동 단계별 시간 (초) */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCActionTiming
{
	GENERATED_BODY()

	/** 예비 동작: 상대가 읽고 반응할 수 있는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0, Units = "s"))
	float Windup = 0.3f;

	/** 판정 구간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0, Units = "s"))
	float Active = 0.1f;

	/** 후딜 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Timing", meta = (ClampMin = 0, Units = "s"))
	float Recovery = 0.4f;
};

/** 공격 범위: 공격자 정면의 부채꼴 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCHitShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, Units = "cm"))
	float Range = 180.f;

	/** 정면 기준 좌우 반각 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, ClampMax = 180, Units = "deg"))
	float HalfAngleDeg = 50.f;
};

/** 판정 한 번에 필요한 정보. 판정하는 동안에만 쓰고 보관하지 않는다. */
struct FPCHitInfo
{
	AActor* Attacker = nullptr;
	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	const UPCAbilityData* Data = nullptr;
};
