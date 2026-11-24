#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PCCombatGameMode.generated.h"

/** 전투 실험용 게임 모드. 기본 폰은 블루프린트 자식에서 지정한다. */
UCLASS()
class PCCOMBAT_API APCCombatGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APCCombatGameMode();
};
