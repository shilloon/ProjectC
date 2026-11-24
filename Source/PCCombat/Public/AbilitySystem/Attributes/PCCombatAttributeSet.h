#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PCCombatAttributeSet.generated.h"

/** 전투 계산에 쓰는 배율. 몬스터 등급이 초기값을 정하고, 나중에 장비나 버프가 같은 속성을 바꿀 수 있다. */
UCLASS()
class PCCOMBAT_API UPCCombatAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPCCombatAttributeSet();

	/** 주는 피해에 곱하는 배율. 1이 기본. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS_BASIC(UPCCombatAttributeSet, AttackPower)

	/** 받는 경직 시간을 줄이는 비율. 0이면 그대로, 1이면 경직되지 않는다. */
	UPROPERTY(BlueprintReadOnly, Category = "Combat")
	FGameplayAttributeData StaggerResistance;
	ATTRIBUTE_ACCESSORS_BASIC(UPCCombatAttributeSet, StaggerResistance)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

private:
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
