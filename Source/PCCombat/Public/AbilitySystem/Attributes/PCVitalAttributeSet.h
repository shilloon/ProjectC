#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PCVitalAttributeSet.generated.h"

/** 체력과 기력 */
UCLASS()
class PCCOMBAT_API UPCVitalAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UPCVitalAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UPCVitalAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS_BASIC(UPCVitalAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Vital")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS_BASIC(UPCVitalAttributeSet, MaxStamina)

	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;

private:
	/** 체력과 기력을 0과 최대값 사이로 제한한다 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
