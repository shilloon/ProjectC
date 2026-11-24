#pragma once

#include "CoreMinimal.h"
#include "Characters/PCMonsterBase.h"
#include "PCTestMonster.generated.h"

/** 자동화 테스트 전용 몬스터. 에셋 없이 코드로 만든 몬스터 정의로 스폰할 수 있다. */
UCLASS(NotBlueprintable, NotPlaceable, HideDropdown)
class PCCOMBAT_API APCTestMonster : public APCMonsterBase
{
	GENERATED_BODY()
};
