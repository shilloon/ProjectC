#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "PCWeaponData.generated.h"

class UPCAbilityData;
class UStaticMesh;

/** 입력 슬롯 하나에 들어가는 기술 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCWeaponAbilitySlot
{
	GENERATED_BODY()

	/** 어느 입력 슬롯인가 (Input.Primary 등) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot", meta = (Categories = "Input"))
	FGameplayTag InputTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
	TObjectPtr<UPCAbilityData> Ability;

	/** true면 누르는 동안 유지하고 떼면 취소한다 (방패 올리기) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
	bool bHold = false;
};

/** 임시 외형의 도형 하나. 모델이 준비되면 이 목록만 바꾼다. */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCWeaponVisualPart
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FName Socket = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FVector Scale = FVector::OneVector;

	/** true면 방어하는 동안 몸 앞으로 올라온다 (방패) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	bool bRaiseOnGuard = false;
};

/**
 * 무기 하나의 정의. 들었을 때 쓸 수 있는 기술 묶음과 넣고 드는 시간을 담는다.
 * 새 무기는 이 에셋을 새로 만들어 장비 컴포넌트의 목록에 넣는다.
 */
UCLASS(BlueprintType)
class PCCOMBAT_API UPCWeaponData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (Categories = "Weapon"))
	FGameplayTag WeaponTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	/** 이 무기를 들었을 때 입력 슬롯별로 쓰는 기술 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (TitleProperty = "InputTag"))
	TArray<FPCWeaponAbilitySlot> AbilitySlots;

	/** 이 무기를 집어넣는 데 걸리는 시간. 다른 무기로 바꿀 때 먼저 든다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swap", meta = (ClampMin = 0, Units = "s"))
	float SheatheTime = 0.4f;

	/** 이 무기를 꺼내 드는 데 걸리는 시간 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Swap", meta = (ClampMin = 0, Units = "s"))
	float DrawTime = 0.5f;

	/** 이 무기를 들고 있는 동안의 이동 속도 배율. 1이면 그대로. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Held", meta = (ClampMin = 0, ClampMax = 2))
	float HeldMoveSpeedScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TArray<FPCWeaponVisualPart> VisualParts;

	const FPCWeaponAbilitySlot* FindSlot(FGameplayTag InputTag) const
	{
		return AbilitySlots.FindByPredicate([InputTag](const FPCWeaponAbilitySlot& Slot) { return Slot.InputTag == InputTag; });
	}
};
