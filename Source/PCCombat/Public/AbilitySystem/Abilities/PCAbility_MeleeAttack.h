#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_MeleeAttack.generated.h"

/**
 * 근접 공격. 판정 구간 동안 정면 부채꼴을 반복해서 훑고, 처음 닿은 대상 하나에 적용한다.
 * 검 베기, 방패 밀기, 할퀴기가 모두 이 클래스이고 데이터만 다르다.
 */
UCLASS()
class PCCOMBAT_API UPCAbility_MeleeAttack : public UPCGameplayAbility
{
	GENERATED_BODY()

public:
	UPCAbility_MeleeAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) override;

private:
	void ExecuteWindupCue();
	void TryHit();
	void StopHitChecks();

	bool bHasHit = false;
	FTimerHandle HitCheckTimer;
};
