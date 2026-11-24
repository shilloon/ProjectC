#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PCMonsterData.generated.h"

class UPCAbilityData;
class USkeletalMesh;
class UStateTree;

/**
 * 몬스터가 고를 수 있는 공격 하나와, 그 공격을 고르는 조건.
 * "언제 어떤 공격을 쓰는가"는 이 목록이 정한다. 행동 트리는 뼈대만 갖는다.
 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCMonsterAttackOption
{
	GENERATED_BODY()

	/** 쓸 기술. 몬스터의 기술 목록에 있어야 한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	/** 대상과의 거리가 이 범위일 때 후보가 된다 (대상의 몸 표면까지의 거리) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "cm"))
	float MinRange = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "cm"))
	float MaxRange = 200.f;

	/** 후보가 여럿일 때 뽑힐 비중. 0이면 뽑히지 않는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0))
	float Weight = 1.f;

	/** 쓴 뒤 다시 후보가 되기까지의 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "s"))
	float Cooldown = 0.f;

	/** 자신의 체력 비율이 이 범위일 때만 후보가 된다 (보스 페이즈용) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, ClampMax = 1))
	float MinHealthRatio = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, ClampMax = 1))
	float MaxHealthRatio = 1.f;

	/** 이 공격을 쓴 뒤 다음 행동까지 숨을 고르는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", meta = (ClampMin = 0, Units = "s"))
	float RestAfter = 1.f;
};

/** 몬스터 한 종류의 정의. 종류, 등급, 서식지, 수치, 기술, 공격 선택 규칙, 행동을 한곳에 모은다. */
UCLASS(BlueprintType)
class PCCOMBAT_API UPCMonsterData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	/** 등급 규칙 (FPCMonsterRankRow) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (RowType = "/Script/PCCombat.PCMonsterRankRow"))
	FDataTableRowHandle RankRow;

	/** 서식지. 지역별 스포너가 이 태그로 몬스터를 고른다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (Categories = "Monster.Habitat"))
	FGameplayTagContainer HabitatTags;

	/** 기본 수치 (FPCCombatantRow) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (RowType = "/Script/PCCombat.PCCombatantRow"))
	FDataTableRowHandle StatsRow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TArray<TObjectPtr<UPCAbilityData>> Abilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TArray<FPCMonsterAttackOption> AttackOptions;

	/** 행동의 뼈대 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTree> BehaviorTree;

	/** 이 거리 안의 대상을 발견한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 0, Units = "cm"))
	float DetectionRange = 1500.f;

	/** 대상이 이 거리보다 멀어지면 놓친다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI", meta = (ClampMin = 0, Units = "cm"))
	float LoseTargetRange = 2500.f;

	/** 대상을 따라 몸을 돌리는 속도. 느릴수록 등 뒤를 잡기 쉽다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Aim", meta = (ClampMin = 1, Units = "deg/s"))
	float TurnRate = 360.f;

	/** 대상이 정면에서 이 각도 안에 들어와야 공격을 시작한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Aim", meta = (ClampMin = 1, ClampMax = 180, Units = "deg"))
	float AimToleranceDeg = 12.f;

	/** 이 시간 안에 조준하지 못하면 보고 있는 방향 그대로 공격한다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Aim", meta = (ClampMin = 0, Units = "s"))
	float MaxAimTime = 0.8f;

	/** 숨 고르는 동안 대상 주위를 도는 속도 배율. 0이면 가만히 서 있는다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Rest", meta = (ClampMin = 0, ClampMax = 1))
	float RestMoveSpeedScale = 0.35f;

	/** 숨 고르는 동안 유지하려는 거리 (대상의 몸 표면까지). 이보다 가까우면 물러나고 멀면 다가간다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Rest", meta = (ClampMin = 0, Units = "cm"))
	float RestDistanceMin = 220.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Rest", meta = (ClampMin = 0, Units = "cm"))
	float RestDistanceMax = 380.f;

	/** 비어 있으면 블루프린트에 설정된 메시를 쓴다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = 0.1))
	float Scale = 1.f;
};
