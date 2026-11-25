#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_MeleeAttack.generated.h"

/**
 * 근접 공격. 판정 구간 동안 데이터가 정한 범위를 반복해서 훑는다.
 * 적용 규칙(처음 닿은 하나 / 범위 안 전원)도 데이터가 정한다.
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
	void TryHit();
	void StopHitChecks();

	/** 대상 하나에 공격을 적용하고, 공격을 받았으면(막거나 맞음) true */
	bool ApplyHitTo(AActor* Target, const FPCHitInfo& Hit);

	/** 이번 공격에서 이미 공격을 받은 대상. 같은 대상을 두 번 치지 않는다. */
	TSet<TWeakObjectPtr<AActor>> AlreadyHit;

	/** "처음 닿은 하나" 규칙에서 대상에 닿아 더 훑지 않는 상태 */
	bool bStoppedByContact = false;

	FTimerHandle HitCheckTimer;
};
