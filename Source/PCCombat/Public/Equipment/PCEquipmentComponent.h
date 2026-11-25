#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PCEquipmentComponent.generated.h"

class UPCAbilityData;
class UPCAbilitySystemComponent;
class UPCWeaponData;
struct FPCWeaponAbilitySlot;

DECLARE_MULTICAST_DELEGATE_TwoParams(FPCOnWeaponChanged, const UPCWeaponData* /*NewWeapon*/, const UPCWeaponData* /*OldWeapon*/);

/**
 * 들고 다니는 무기 목록과 지금 든 무기를 관리한다.
 * 무기가 바뀌면 이전 무기의 기술을 회수하고 새 무기의 기술을 부여한다.
 * 무기를 쓰는 캐릭터만 이 컴포넌트를 가진다.
 */
UCLASS(ClassGroup = "PC", meta = (BlueprintSpawnableComponent))
class PCCOMBAT_API UPCEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPCEquipmentComponent();

	/** 들고 다니는 무기. 순서가 선택 번호(1, 2, 3...)가 된다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PC|Equipment")
	TArray<TObjectPtr<UPCWeaponData>> Loadout;

	/** 시작할 때 들고 있는 무기의 번호 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PC|Equipment", meta = (ClampMin = 0))
	int32 StartingIndex = 0;

	/** 무기 교체 기술. 넣고 드는 시간은 무기 에셋에서 온다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PC|Equipment")
	TObjectPtr<UPCAbilityData> SwapAbility;

	/** 교체 중에 맞으면 공격의 경직 값과 관계없이 최소 이만큼 경직된다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PC|Equipment|Swap Penalty", meta = (ClampMin = 0, Units = "s"))
	float SwapInterruptStaggerTime = 0.6f;

	/** 교체 중에 맞을 때 받는 피해에 곱하는 배율. 1이면 추가 피해 없음. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PC|Equipment|Swap Penalty", meta = (ClampMin = 0))
	float SwapDamageTakenMultiplier = 1.f;

	/** 캐릭터의 능력 시스템이 준비된 뒤 한 번 호출한다. 교체 기술을 부여하고 시작 무기를 든다. */
	void InitializeEquipment(UPCAbilitySystemComponent* InAbilitySystem);

	const UPCWeaponData* GetCurrentWeapon() const;
	int32 GetCurrentIndex() const { return CurrentIndex; }

	/** 지금 든 무기에서 이 입력 슬롯에 들어 있는 기술. 없으면 nullptr. */
	const FPCWeaponAbilitySlot* FindSlot(FGameplayTag InputTag) const;

	/** 넣고 드는 과정을 거쳐 무기를 바꾸기 시작한다. 시작했으면 true. */
	bool RequestSwap(int32 Index);

	/** RequestSwap으로 요청된 무기 번호. 교체 기술이 읽는다. */
	int32 GetPendingIndex() const { return PendingIndex; }
	const UPCWeaponData* GetWeaponAt(int32 Index) const;

	/** 과정 없이 바로 무기를 바꾼다. 교체 기술이 넣기와 들기의 경계에서 호출한다. */
	void EquipIndex(int32 Index);

	FPCOnWeaponChanged OnWeaponChanged;

private:
	void GrantWeapon(const UPCWeaponData& Weapon);
	void RemoveWeapon(const UPCWeaponData& Weapon);

	UPROPERTY()
	TObjectPtr<UPCAbilitySystemComponent> AbilitySystem;

	int32 CurrentIndex = INDEX_NONE;
	int32 PendingIndex = INDEX_NONE;

	/** 지금 든 무기가 걸어 둔 이동 속도 배율 */
	FActiveGameplayEffectHandle HeldEffectHandle;
};
