#pragma once

#include "CoreMinimal.h"
#include "Shakes/LegacyCameraShake.h"
#include "PCCameraShake_Hit.generated.h"

/** 타격 순간의 짧은 카메라 흔들림. 강도는 재생할 때의 배율로 조절한다. */
UCLASS()
class PCCOMBAT_API UPCCameraShake_Hit : public ULegacyCameraShake
{
	GENERATED_BODY()

public:
	UPCCameraShake_Hit();
};
