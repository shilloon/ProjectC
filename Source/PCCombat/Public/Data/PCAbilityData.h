#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "Types/PCCombatTypes.h"
#include "PCAbilityData.generated.h"

class UAnimMontage;
class UGameplayAbility;

/**
 * 기술 하나의 정의. 어떤 능력 클래스로 동작하는지와 모든 수치를 담는다.
 * 동작이 같고 수치만 다른 기술은 이 에셋만 새로 만든다.
 */
UCLASS(BlueprintType)
class PCCOMBAT_API UPCAbilityData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 이 기술을 실행하는 능력 클래스. UPCGameplayAbility의 자식을 지정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<UGameplayAbility> AbilityClass;

	/** 이 기술을 가리키는 태그. 입력과 AI가 이 태그로 발동한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FPCActionTiming Timing;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = 0))
	float StaminaCost = 0.f;

	/** 이 기술을 쓰는 동안의 이동 속도 배율. 1이면 그대로, 0이면 제자리. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability", meta = (ClampMin = 0, ClampMax = 1))
	float MoveSpeedScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float Damage = 0.f;

	/** 막혔을 때 상대 기력을 깎는 양 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float GuardStaminaDamage = 0.f;

	/**
	 * 공격이 닿았을 때 대상에게 일어나는 일 (경직, 밀려남, 지속 효과).
	 * 한 줄이 "조건 → 결과"이고, 기술마다 다르게 적는다. 비어 있으면 피해만 준다.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (TitleProperty = "When"))
	TArray<FPCHitReaction> HitReactions;

	/** 타격 순간 때린 쪽과 맞은 쪽이 멈추는 시간 (겉모습 전용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, ClampMax = 0.5, Units = "s"))
	float HitStopTime = 0.f;

	/** 위험 등급. 예고 강도와 방어 가능 여부를 정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (Categories = "Threat"))
	FGameplayTag ThreatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FPCHitShape HitShape;

	/** 돌진 공격 전용: 판정 구간 동안 앞으로 이동하는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "cm"))
	float LungeDistance = 0.f;

	/** 원거리 공격 전용: 쏘는 투사체 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ranged")
	FPCProjectileSpec Projectile;

	/** 방어 전용: 정면 기준 막을 수 있는 반각 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard", meta = (ClampMin = 0, ClampMax = 180, Units = "deg"))
	float GuardHalfAngleDeg = 70.f;

	/** 회피 전용: 이동 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dodge", meta = (ClampMin = 0, Units = "cm"))
	float DodgeDistance = 350.f;

	/** 비어 있어도 동작한다. 애니메이션이 준비되면 채운다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<UAnimMontage> Montage;
};
