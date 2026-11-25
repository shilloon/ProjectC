#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "PCCombatTypes.generated.h"

class AActor;
class APCProjectile;
class UGameplayEffect;
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

/** 타격 반응이 적용되는 판정 결과 */
UENUM(BlueprintType)
enum class EPCHitReactionWhen : uint8
{
	/** 대상이 맞았을 때 */
	Hit,
	/** 대상이 방패로 막았을 때 */
	Blocked,
	/** 맞았든 막았든 */
	Any
};

/**
 * 타격 반응 한 줄: "조건이 맞으면 대상에게 이 결과를 준다".
 * 기술마다 여러 줄을 둘 수 있다. 조건이 맞는 줄이 여럿이면 경직과 밀려남은 가장 큰 값 하나만, 효과는 모두 적용한다.
 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCHitReaction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition")
	EPCHitReactionWhen When = EPCHitReactionWhen::Hit;

	/** 대상이 맞는 순간 이 태그를 모두 갖고 있어야 한다 (예: State.Action.Windup이면 예비 동작 중일 때만) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (Categories = "State"))
	FGameplayTagContainer RequiredTargetTags;

	/** 대상이 이 태그 중 하나라도 갖고 있으면 적용하지 않는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Condition", meta = (Categories = "State"))
	FGameplayTagContainer IgnoredTargetTags;

	/** 경직 시간. 0이면 경직 없음. 대상의 경직 저항만큼 줄어든다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (ClampMin = 0, Units = "s"))
	float StaggerTime = 0.f;

	/** 공격이 온 반대쪽으로 밀려나는 거리. 대상의 경직 저항만큼 줄어든다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (ClampMin = 0, Units = "cm"))
	float KnockbackDistance = 0.f;

	/** 대상에게 거는 지속 효과 (둔화 등). 지속 시간은 Data.Duration, 세기는 Data.Magnitude로 전달된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result", meta = (ClampMin = 0, Units = "s"))
	float EffectDuration = 0.f;

	/** 효과의 세기. 뜻은 효과마다 다르다 (둔화: 이동 속도 배율). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Result")
	float EffectMagnitude = 1.f;

	bool Matches(EPCHitOutcome Outcome, const FGameplayTagContainer& TargetTags) const
	{
		const bool bOutcomeMatches = When == EPCHitReactionWhen::Any
			|| (When == EPCHitReactionWhen::Hit && Outcome == EPCHitOutcome::Hit)
			|| (When == EPCHitReactionWhen::Blocked && Outcome == EPCHitOutcome::Blocked);
		return bOutcomeMatches && TargetTags.HasAll(RequiredTargetTags) && !TargetTags.HasAny(IgnoredTargetTags);
	}
};

/** 투사체를 쏘는 기술의 설정 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCProjectileSpec
{
	GENERATED_BODY()

	/** 비어 있으면 기본 투사체(APCProjectile)를 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<APCProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = 1, Units = "cm/s"))
	float Speed = 3000.f;

	/** 0이면 직선으로 날아가고, 1이면 월드 중력을 그대로 받는다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = 0))
	float GravityScale = 0.3f;

	/** 이만큼 날아가면 사라진다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = 0, Units = "cm"))
	float MaxDistance = 4000.f;

	/** 투사체의 굵기 (판정용 구의 반지름) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = 0, Units = "cm"))
	float Radius = 8.f;

	/** 쏘는 위치: 캐릭터 중심에서 조준 방향으로 띄우는 거리와 위로 올리는 높이 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (Units = "cm"))
	float SpawnForwardOffset = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (Units = "cm"))
	float SpawnHeightOffset = 40.f;
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
