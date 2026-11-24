#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCAbility_MeleeAttack.h"
#include "PCAbility_LungeAttack.generated.h"

/**
 * 돌진 공격. 근접 공격과 같되, 판정 구간 동안 바라보는 방향으로 LungeDistance만큼 이동하면서 훑는다.
 * 방향은 예비 동작이 시작될 때 정해지므로, 옆으로 비켜서면 피할 수 있다.
 */
UCLASS()
class PCCOMBAT_API UPCAbility_LungeAttack : public UPCAbility_MeleeAttack
{
	GENERATED_BODY()

protected:
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) override;
};
