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
class UPCCombatAttributeSet;
class UPCAbilityData;
class UPCCombatPresenterComponent;
class UPCEquipmentComponent;
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

	/** 무기를 쓰는 캐릭터만 가진다. 없으면 nullptr. */
	UPCEquipmentComponent* GetEquipment() const { return Equipment; }

	/**
	 * From에서 쏠 때 겨누는 방향. 기본은 캐릭터의 정면이다.
	 * 플레이어는 화면 가운데가 가리키는 곳을 향하도록 바꾼다.
	 */
	virtual FVector GetAimDirection(const FVector& From) const;

	/** 방어 능력이 발동할 때 막을 수 있는 각도를 알려준다 */
	void SetGuardHalfAngle(float HalfAngleDeg) { GuardHalfAngleDeg = HalfAngleDeg; }

	/** 경직 이펙트를 적용한다. State.Staggered가 붙으면 진행 중인 능력은 모두 취소된다. */
	void ApplyStagger(float Duration);

	/** FromLocation의 반대쪽으로 Distance만큼 밀려난다. 경직 저항만큼 거리가 줄어든다. */
	void ApplyKnockback(const FVector& FromLocation, float Distance);

	bool IsDead() const;
	bool HasStateTag(FGameplayTag Tag) const;

	float GetHealth() const;
	float GetMaxHealth() const;
	float GetStamina() const;
	float GetMaxStamina() const;

	/** 주는 피해에 곱하는 배율 */
	float GetAttackPower() const;

	FPCOnCombatCue OnCombatCue;

protected:
	virtual void BeginPlay() override;

	virtual void HandleDeath();
	virtual void Revive();

	/** 기본 수치를 읽어 올 행. 자식이 다른 출처(몬스터 정의 등)로 바꿀 수 있다. */
	virtual FDataTableRowHandle GetStatsRowHandle() const { return CombatantRow; }

	/** 시작할 때 부여할 기술을 모은다 */
	virtual void GetStartingAbilities(TArray<const UPCAbilityData*>& OutAbilities) const;

	/** 기본 수치를 적용한 뒤, 기술을 부여하기 전에 호출된다 */
	virtual void OnStatsInitialized() {}

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

	UPROPERTY()
	TObjectPtr<UPCCombatAttributeSet> CombatSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UPCCombatPresenterComponent> Presenter;

	/** 자식이 생성자에서 만들어 넣는다. 무기를 쓰지 않는 캐릭터(몬스터)는 비워 둔다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UPCEquipmentComponent> Equipment;

private:
	void InitializeFromRow();
	void GrantStartingAbilities();
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnStaminaChanged(const FOnAttributeChangeData& Data);
	void OnStaggerTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount);
	void OnMoveSpeedMultiplierChanged(const FOnAttributeChangeData& Data);
	void ExecuteCue(FGameplayTag CueTag, const FPCHitInfo& Hit);

	/** 기술의 타격 반응 중 조건이 맞는 것을 적용한다. MinStaggerTime은 반응과 무관하게 보장하는 최소 경직이다. */
	void ApplyHitReactions(const FPCHitInfo& Hit, EPCHitOutcome Outcome, const FGameplayTagContainer& TagsWhenHit, float MinStaggerTime);

	float GuardHalfAngleDeg = 70.f;
	float StaminaRegenDelay = 1.f;

	/** 배율이 걸리기 전의 이동 속도 */
	float BaseMoveSpeed = 500.f;

	FTimerHandle ReviveTimer;
};
