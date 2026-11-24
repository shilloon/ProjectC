#pragma once

#include "CoreMinimal.h"
#include "Characters/PCCharacterBase.h"
#include "PCTestCharacter.generated.h"

class UPCAbilityData;

/** 자동화 테스트 전용 캐릭터. 에셋 없이 코드로 기술을 넣어 스폰할 수 있다. */
UCLASS(NotBlueprintable, NotPlaceable, HideDropdown)
class APCTestCharacter : public APCCharacterBase
{
	GENERATED_BODY()

public:
	APCTestCharacter();

	/** 스폰이 끝나기 전에 호출해 시작 기술을 넣는다 */
	void AddStartingAbility(UPCAbilityData* Data);
};
