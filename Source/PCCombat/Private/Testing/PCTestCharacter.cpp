#include "Testing/PCTestCharacter.h"
#include "Data/PCAbilityData.h"
#include "Equipment/PCEquipmentComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Presentation/PCCombatPresenterComponent.h"

APCTestCharacter::APCTestCharacter()
{
	Presenter->bShowWeapons = false;

	// 무기 목록이 비어 있으면 아무 일도 하지 않으므로, 무기를 쓰지 않는 테스트에는 영향이 없다
	Equipment = CreateDefaultSubobject<UPCEquipmentComponent>(TEXT("Equipment"));

	// 조종하는 컨트롤러가 없어도 중력과 밀려남이 적용되게 한다
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void APCTestCharacter::AddStartingAbility(UPCAbilityData* Data)
{
	StartingAbilities.Add(Data);
}

FVector APCTestCharacter::GetAimDirection(const FVector& From) const
{
	return AimOverride.IsNearlyZero() ? Super::GetAimDirection(From) : AimOverride.GetSafeNormal();
}
