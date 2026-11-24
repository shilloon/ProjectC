#include "UI/PCCombatHUD.h"
#include "Characters/PCCharacterBase.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"

namespace
{
	constexpr float PlayerBarWidth = 320.f;
	constexpr float PlayerBarHeight = 18.f;
	constexpr float PlayerBarMargin = 40.f;
	constexpr float PlayerBarGap = 8.f;
	constexpr float WorldBarWidth = 110.f;
	constexpr float WorldBarHeight = 8.f;
	constexpr float WorldBarHeightOffset = 115.f;
	constexpr float BarBorder = 2.f;

	const FLinearColor BarBackColor(0.f, 0.f, 0.f, 0.6f);
	const FLinearColor HealthColor(0.8f, 0.1f, 0.1f, 1.f);
	const FLinearColor StaminaColor(0.9f, 0.75f, 0.1f, 1.f);

	float SafeRatio(float Value, float Max)
	{
		return Max > 0.f ? FMath::Clamp(Value / Max, 0.f, 1.f) : 0.f;
	}
}

void APCCombatHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	const APCCharacterBase* Player = Cast<APCCharacterBase>(GetOwningPawn());
	if (Player)
	{
		const float StaminaY = Canvas->SizeY - PlayerBarMargin - PlayerBarHeight;
		const float HealthY = StaminaY - PlayerBarGap - PlayerBarHeight;
		DrawBar(PlayerBarMargin, HealthY, PlayerBarWidth, PlayerBarHeight, SafeRatio(Player->GetHealth(), Player->GetMaxHealth()), HealthColor);
		DrawBar(PlayerBarMargin, StaminaY, PlayerBarWidth, PlayerBarHeight, SafeRatio(Player->GetStamina(), Player->GetMaxStamina()), StaminaColor);
	}

	for (TActorIterator<APCCharacterBase> It(GetWorld()); It; ++It)
	{
		const APCCharacterBase* Other = *It;
		if (Other == Player)
		{
			continue;
		}

		const FVector Screen = Project(Other->GetActorLocation() + FVector(0.f, 0.f, WorldBarHeightOffset));
		if (Screen.Z > 0.f)
		{
			DrawBar(Screen.X - WorldBarWidth * 0.5f, Screen.Y, WorldBarWidth, WorldBarHeight, SafeRatio(Other->GetHealth(), Other->GetMaxHealth()), HealthColor);
		}
	}
}

void APCCombatHUD::DrawBar(float X, float Y, float Width, float Height, float Ratio, const FLinearColor& Color)
{
	DrawRect(BarBackColor, X - BarBorder, Y - BarBorder, Width + BarBorder * 2.f, Height + BarBorder * 2.f);
	DrawRect(Color, X, Y, Width * Ratio, Height);
}
