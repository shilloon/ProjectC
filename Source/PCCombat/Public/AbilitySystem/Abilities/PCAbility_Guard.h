#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCGameplayAbility.h"
#include "PCAbility_Guard.generated.h"

/** 방패 올리기. 취소될 때까지 State.Guarding을 유지한다. 막는 판정은 맞는 쪽(APCCharacterBase)이 한다. */
UCLASS()
class PCCOMBAT_API UPCAbility_Guard : public UPCGameplayAbility
{
	GENERATED_BODY()

public:
	UPCAbility_Guard();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
