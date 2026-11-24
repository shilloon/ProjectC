#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PCAbilitySystemComponent.generated.h"

class UPCAbilityData;

/** 데이터 에셋으로 기술을 부여하고, 기술 태그로 발동/취소한다 */
UCLASS(ClassGroup = "PC", meta = (BlueprintSpawnableComponent))
class PCCOMBAT_API UPCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	/** 데이터 에셋이 지정한 능력 클래스를 부여한다. 데이터는 능력의 SourceObject로 전달된다. */
	FGameplayAbilitySpecHandle GrantAbilityFromData(const UPCAbilityData* Data);

	/** 기술 태그로 발동을 시도한다. 발동했으면 true. */
	bool TryActivateByAbilityTag(FGameplayTag AbilityTag);

	/** 기술 태그에 해당하는 능력이 실행 중이면 취소한다 */
	void CancelByAbilityTag(FGameplayTag AbilityTag);

	/** SetByCaller 수치 하나를 받는 이펙트를 자신에게 적용한다. 지속되는 이펙트면 나중에 제거할 수 있는 핸들을 돌려준다. */
	FActiveGameplayEffectHandle ApplySetByCallerEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, FGameplayTag DataTag, float Magnitude, AActor* Instigator);

private:
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> HandlesByAbilityTag;
};
