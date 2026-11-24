#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Hit/PCHittable.h"
#include "PCCharacterBase.generated.h"

class UPCAbilitySystemComponent;
class UPCVitalAttributeSet;
class UPCAbilityData;
class UPCCombatPresenterComponent;
struct FOnAttributeChangeData;

/** 판정 코드가 내보낸 GameplayCue를 겉모습 쪽에 전달한다 */
DECLARE_MULTICAST_DELEGATE_TwoParams(FPCOnCombatCue, FGameplayTag /*CueTag*/, const FGameplayCueParameters& /*Parameters*/);

/**
 * 전투에 참여하는 모든 캐릭터의 부모.
 * 능력 시스템, 체력/기력, 피격 처리를 가진다.
 * 경직, 사망, 기력 회복은 GameplayEffect가 태그와 수치를 관리하고, 이 클래스는 태그 변화에 반응만 한다.
 */
UCLASS(Abstract)
class PCCOMBAT_API APCCharacterBase : public ACharacter, public IAbilitySystemInterface, public IPCHittable, public IGameplayCueInterface
{
	GENERATED_BODY()

public:
	APCCharacterBase();

	// ~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// ~IPCHittable
	virtual EPCHitOutcome ReceiveHit(const FPCHitInfo& Hit) override;

	// ~IGameplayCueInterface
	virtual void HandleGameplayCue(AActor* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

	UPCAbilitySystemComponent* GetPCAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** 방어 능력이 발동할 때 막을 수 있는 각도를 알려준다 */
	void SetGuardHalfAngle(float HalfAngleDeg) { GuardHalfAngleDeg = HalfAngleDeg; }

	/** 경직 이펙트를 적용한다. State.Staggered가 붙으면 진행 중인 능력은 모두 취소된다. */
	void ApplyStagger(float Duration);

	bool IsDead() const;
	bool HasStateTag(FGameplayTag Tag) const;

	float GetHealth() const;
	float GetMaxHealth() const;
	float GetStamina() const;
	float GetMaxStamina() const;

	FPCOnCombatCue OnCombatCue;

protected:
	virtual void BeginPlay() override;

	virtual void HandleDeath();
	virtual void Revive();

	/** 기본 수치를 읽어 올 데이터테이블 행. 비어 있으면 FPCCombatantRow의 기본값을 쓴다. */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Data", meta = (RowType = "/Script/PCCombat.PCCombatantRow"))
	FDataTableRowHandle CombatantRow;

	/** 시작할 때 부여하는 기술 목록 */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Data")
	TArray<TObjectPtr<UPCAbilityData>> StartingAbilities;

	/** 방어 중 기력이 바닥났을 때의 경직 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Combat", meta = (ClampMin = 0, Units = "s"))
	float GuardBreakStaggerTime = 1.2f;

	/** 사망 후 부활까지의 시간. 실험용이라 죽어도 다시 일어난다. */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Combat", meta = (ClampMin = 0, Units = "s"))
	float ReviveDelay = 3.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UPCAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPCVitalAttributeSet> VitalSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UPCCombatPresenterComponent> Presenter;

private:
	void InitializeFromRow();
	void GrantStartingAbilities();
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnStaminaChanged(const FOnAttributeChangeData& Data);
	void OnStaggerTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void ExecuteCue(FGameplayTag CueTag, const FPCHitInfo& Hit);

	float GuardHalfAngleDeg = 70.f;
	float StaminaRegenDelay = 1.f;

	FTimerHandle ReviveTimer;
};
