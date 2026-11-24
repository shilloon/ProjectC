#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_Dodge.generated.h"

/** 회피 스텝. 입력 방향으로 짧게 이동한다. 무적 시간은 없고, 범위 밖으로 나가야 피한 것이다. */
UCLASS()
class PCCOMBAT_API UPCAbility_Dodge : public UPCGameplayAbility
{
	GENERATED_BODY()

public:
	UPCAbility_Dodge();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) override;

private:
	FVector DodgeDirection = FVector::ForwardVector;
};
