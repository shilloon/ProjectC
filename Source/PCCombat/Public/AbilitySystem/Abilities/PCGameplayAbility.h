#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Types/PCCombatTypes.h"
#include "PCGameplayAbility.generated.h"

class APCCharacterBase;
class UPCAbilityData;

/**
 * 모든 전투 능력의 부모.
 * 수치는 부여될 때 SourceObject로 받은 UPCAbilityData에서 읽는다.
 * 예비 동작 → 판정 → 후딜 단계를 데이터의 시간대로 진행하고, 단계마다 상태 태그를 붙인다.
 */
UCLASS(Abstract)
class PCCOMBAT_API UPCGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPCGameplayAbility();

	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	/** 실행 중인 능력의 데이터 */
	const UPCAbilityData* GetAbilityData() const;

	APCCharacterBase* GetPCCharacter() const;

	EPCActionPhase GetCurrentPhase() const { return CurrentPhase; }

	/** 단계에 들어가고, Duration 뒤에 다음 단계로 넘어간다. Recovery가 끝나면 능력이 끝난다. */
	void StartPhase(EPCActionPhase Phase, float Duration);

	/** 단계가 바뀔 때 호출된다. 능력이 끝나면 NewPhase가 None이다. */
	virtual void OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase) {}

	/** 능력을 취소로 끝낸다 */
	void EndAsCancelled();

private:
	UFUNCTION()
	void AdvancePhase();

	void SetPhase(EPCActionPhase NewPhase);

	static const UPCAbilityData* GetDataFromSpec(const UGameplayAbility& Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo);
	static FGameplayTag PhaseToTag(EPCActionPhase Phase);

	EPCActionPhase CurrentPhase = EPCActionPhase::None;
};
