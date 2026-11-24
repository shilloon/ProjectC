#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Effects/PCGameplayEffect.h"
#include "PCEffect_Stagger.generated.h"

/** 경직. SetByCaller(Data.Duration) 동안 State.Staggered를 부여한다. */
UCLASS()
class PCCOMBAT_API UPCEffect_Stagger : public UPCGameplayEffect
{
	GENERATED_BODY()

public:
	UPCEffect_Stagger();
};
