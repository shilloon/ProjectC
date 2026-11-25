#include "UI/PCCombatHUD.h"
#include "Characters/PCCharacterBase.h"
#include "Data/PCWeaponData.h"
#include "Equipment/PCEquipmentComponent.h"
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
	const FLinearColor CurrentWeaponColor(1.f, 1.f, 1.f, 1.f);
	const FLinearColor OtherWeaponColor(0.55f, 0.55f, 0.55f, 1.f);
	const FLinearColor CrosshairColor(1.f, 1.f, 1.f, 0.8f);

	constexpr float WeaponTextGap = 26.f;
	constexpr float WeaponTextSpacing = 150.f;
	constexpr float WeaponTextScale = 1.3f;
	constexpr float CrosshairSize = 4.f;

	/** 기본 글꼴에 한글이 없어서 무기 태그의 마지막 이름(SwordShield 등)을 쓴다 */
	FString WeaponLabel(const UPCWeaponData& Weapon)
	{
		FString Name = Weapon.WeaponTag.ToString();
		int32 DotIndex = INDEX_NONE;
		if (Name.FindLastChar(TEXT('.'), DotIndex))
		{
			Name.RightChopInline(DotIndex + 1);
		}
		return Name;
	}

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

		// 들고 다니는 무기 목록. 지금 든 무기를 밝게 표시한다.
		if (const UPCEquipmentComponent* Equipment = Player->GetEquipment())
		{
			for (int32 Index = 0; Index < Equipment->Loadout.Num(); ++Index)
			{
				if (const UPCWeaponData* Weapon = Equipment->GetWeaponAt(Index))
				{
					const bool bCurrent = Index == Equipment->GetCurrentIndex();
					const FString Text = FString::Printf(TEXT("%s%d %s"), bCurrent ? TEXT("> ") : TEXT(""), Index + 1, *WeaponLabel(*Weapon));
					DrawText(Text, bCurrent ? CurrentWeaponColor : OtherWeaponColor, PlayerBarMargin + WeaponTextSpacing * Index, HealthY - WeaponTextGap, nullptr, WeaponTextScale);
				}
			}
		}

		// 화면 가운데의 조준점. 원거리 무기가 이곳을 향해 쏜다.
		DrawRect(CrosshairColor, Canvas->SizeX * 0.5f - CrosshairSize * 0.5f, Canvas->SizeY * 0.5f - CrosshairSize * 0.5f, CrosshairSize, CrosshairSize);
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
