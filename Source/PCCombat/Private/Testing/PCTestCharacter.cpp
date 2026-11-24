#include "Testing/PCTestCharacter.h"
#include "Data/PCAbilityData.h"
#include "Presentation/PCCombatPresenterComponent.h"

APCTestCharacter::APCTestCharacter()
{
	Presenter->bShowWeapons = false;
}

void APCTestCharacter::AddStartingAbility(UPCAbilityData* Data)
{
	StartingAbilities.Add(Data);
}
