#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/PCGameplayEffect.h"
#include "PCEffect_Stamina.generated.h"

/** 즉시 기력을 바꾼다. 변화량은 SetByCaller(Data.Stamina)로 받는다. 소모는 음수로 넣는다. */
UCLASS()
class PCCOMBAT_API UPCEffect_Stamina : public UPCGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_Stamina();
};
