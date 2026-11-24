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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float Damage = 0.f;

	/** 막혔을 때 상대 기력을 깎는 양 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float GuardStaminaDamage = 0.f;

	/** 맞은 대상의 경직 시간. 0이면 경직 없음. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "s"))
	float StaggerTime = 0.f;

	/** true면 대상이 예비 동작 중일 때만 경직시킨다 (방패 밀기: 타이밍을 맞춰야 끊는다) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	bool bStaggerOnlyOnWindup = false;

	/** 위험 등급. 예고 강도와 방어 가능 여부를 정한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (Categories = "Threat"))
	FGameplayTag ThreatTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FPCHitShape HitShape;

	/** 돌진 공격 전용: 판정 구간 동안 앞으로 이동하는 거리 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "cm"))
	float LungeDistance = 0.f;

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
