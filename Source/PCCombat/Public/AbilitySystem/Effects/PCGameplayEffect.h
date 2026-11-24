#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PCGameplayEffect.generated.h"

/**
 * C++로 정의하는 이펙트의 부모. 자식은 생성자에서 아래 도우미로 자신을 구성한다.
 * 수치는 모두 SetByCaller로 받아서, 이펙트 클래스에는 숫자를 두지 않는다.
 */
UCLASS(Abstract)
class PCCOMBAT_API UPCGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()

protected:
	/** Attribute에 SetByCaller(DataTag) 값을 더하는 수정자를 추가한다 */
	void AddSetByCallerModifier(const FGameplayAttribute& Attribute, FGameplayTag DataTag);

	/** 지속 시간을 SetByCaller(DataTag)로 받는 기간제 이펙트로 만든다 */
	void SetDurationFromCaller(FGameplayTag DataTag);

	/** 적용되어 있는 동안 대상에게 태그를 부여한다 */
	void GrantTagToTarget(FGameplayTag Tag);

	/** 대상이 이 태그 중 하나라도 가진 동안에는 효과가 멈춘다 */
	void PauseWhileTargetHasAnyTag(const FGameplayTagContainer& Tags);
};
