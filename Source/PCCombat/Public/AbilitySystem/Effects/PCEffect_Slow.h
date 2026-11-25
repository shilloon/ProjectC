#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/PCGameplayEffect.h"
#include "PCEffect_Slow.generated.h"

/**
 * 둔화. 지속 시간(Data.Duration) 동안 이동 속도에 배율(Data.Magnitude)을 곱한다.
 * 타격 반응의 지속 효과로 쓴다. 여러 번 걸려도 겹치지 않고 시간만 갱신된다.
 */
UCLASS()
class PCCOMBAT_API UPCEffect_Slow : public UPCGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_Slow();
};
