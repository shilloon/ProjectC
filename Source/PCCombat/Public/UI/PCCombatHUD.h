#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PCCombatHUD.generated.h"

/** 임시 HUD. 플레이어의 체력과 기력, 다른 캐릭터의 머리 위 체력을 막대로 그린다. */
UCLASS()
class PCCOMBAT_API APCCombatHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

private:
	void DrawBar(float X, float Y, float Width, float Height, float Ratio, const FLinearColor& Color);
};
