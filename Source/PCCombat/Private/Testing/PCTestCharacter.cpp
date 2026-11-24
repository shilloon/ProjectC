#include "Testing/PCTestCharacter.h"
#include "Data/PCAbilityData.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Presentation/PCCombatPresenterComponent.h"

APCTestCharacter::APCTestCharacter()
{
	Presenter->bShowWeapons = false;

	// 조종하는 컨트롤러가 없어도 중력과 밀려남이 적용되게 한다
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
}

void APCTestCharacter::AddStartingAbility(UPCAbilityData* Data)
{
	StartingAbilities.Add(Data);
}
