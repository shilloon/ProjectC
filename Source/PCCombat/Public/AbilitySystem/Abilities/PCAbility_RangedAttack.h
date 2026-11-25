#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_RangedAttack.generated.h"

/**
 * 원거리 공격. 예비 동작(시위 당김)이 끝나는 순간 캐릭터가 겨눈 방향으로 투사체를 쏜다.
 * 투사체는 날아가는 경로에서 처음 닿은 대상에게 적용된다. 쏜 뒤의 판정은 투사체가 맡는다.
 */
UCLASS()
class PCCOMBAT_API UPCAbility_RangedAttack : public UPCGameplayAbility
{
	GENERATED_BODY()

public:
	UPCAbility_RangedAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) override;

private:
	void Fire();
};
