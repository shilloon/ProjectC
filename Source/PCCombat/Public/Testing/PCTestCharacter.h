#pragma once

#include "CoreMinimal.h"
#include "Characters/PCCharacterBase.h"
#include "PCTestCharacter.generated.h"

class UPCAbilityData;

/** 자동화 테스트 전용 캐릭터. 에셋 없이 코드로 기술을 넣어 스폰할 수 있다. */
UCLASS(NotBlueprintable, NotPlaceable, HideDropdown)
class PCCOMBAT_API APCTestCharacter : public APCCharacterBase
{
	GENERATED_BODY()

public:
	APCTestCharacter();

	/** 스폰이 끝나기 전에 호출해 시작 기술을 넣는다 */
	void AddStartingAbility(UPCAbilityData* Data);

	/** 테스트에서 겨누는 방향을 직접 정한다. 영벡터면 정면을 겨눈다. */
	void SetAimDirection(const FVector& Direction) { AimOverride = Direction; }
	virtual FVector GetAimDirection(const FVector& From) const override;

private:
	FVector AimOverride = FVector::ZeroVector;
};
