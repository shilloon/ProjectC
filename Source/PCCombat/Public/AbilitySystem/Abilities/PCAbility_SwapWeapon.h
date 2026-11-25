#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_SwapWeapon.generated.h"

class UPCEquipmentComponent;

/**
 * 무기 교체. 지금 무기를 넣는 구간(예비 동작)과 새 무기를 드는 구간(판정)으로 진행한다.
 * 두 구간의 길이는 기술 데이터가 아니라 무기 에셋의 넣는 시간과 드는 시간에서 온다.
 * 실제 교체는 두 구간의 경계에서 일어난다. 그래서 넣는 중에 끊기면 원래 무기가 남고,
 * 드는 중에 끊기면 새 무기를 든 상태가 된다.
 */
UCLASS()
class PCCOMBAT_API UPCAbility_SwapWeapon : public UPCGameplayAbility
{
	GENERATED_BODY()

public:
	UPCAbility_SwapWeapon();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual float GetPhaseDuration(EPCActionPhase Phase) const override;
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) override;

private:
	static UPCEquipmentComponent* GetEquipment(const FGameplayAbilityActorInfo* ActorInfo);

	/** 이번 교체로 들 무기의 번호 */
	int32 TargetIndex = INDEX_NONE;
};
