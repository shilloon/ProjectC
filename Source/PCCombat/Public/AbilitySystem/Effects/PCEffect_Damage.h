#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "PCEffect_Damage.generated.h"

/** 즉시 체력을 바꾼다. 변화량은 SetByCaller(Data.Damage)로 받는다. 피해는 음수로 넣는다. */
UCLASS()
class PCCOMBAT_API UPCEffect_Damage : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_Damage();
};
