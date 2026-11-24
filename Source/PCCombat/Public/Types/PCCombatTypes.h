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

/** 판정 범위의 모양 */
UENUM(BlueprintType)
enum class EPCHitShapeType : uint8
{
	/** 공격자 정면의 부채꼴 (휘두르기, 찌르기) */
	Sector,
	/** 공격자 주위의 원 (내려찍기, 포효) */
	Circle
};

/** 범위 안의 대상 중 누구에게 적용하는가 */
UENUM(BlueprintType)
enum class EPCHitTargetRule : uint8
{
	/** 처음 닿은 대상 하나. 그 대상이 뒤의 대상을 가린다. (무기 공격) */
	FirstOnly,
	/** 범위 안의 모든 대상. 같은 대상은 한 번만 맞는다. (크게 휩쓸기, 내려찍기) */
	AllInArea
};

/** 공격 범위 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCHitShape
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	EPCHitShapeType ShapeType = EPCHitShapeType::Sector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit")
	EPCHitTargetRule TargetRule = EPCHitTargetRule::FirstOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, Units = "cm"))
	float Range = 180.f;

	/** 부채꼴일 때만 쓴다: 정면 기준 좌우 반각 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit", meta = (ClampMin = 0, ClampMax = 180, Units = "deg", EditCondition = "ShapeType == EPCHitShapeType::Sector"))
	float HalfAngleDeg = 50.f;
};

/** 판정 한 번에 필요한 정보. 판정하는 동안에만 쓰고 보관하지 않는다. */
struct FPCHitInfo
{
	AActor* Attacker = nullptr;
	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	const UPCAbilityData* Data = nullptr;

	/** 공격자의 공격력 배율. 데이터의 피해에 곱한다. */
	float DamageMultiplier = 1.f;
};
