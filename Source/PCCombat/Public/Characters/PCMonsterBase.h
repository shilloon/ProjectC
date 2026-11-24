#pragma once

#include "CoreMinimal.h"
#include "Characters/PCCharacterBase.h"
#include "PCMonsterBase.generated.h"

class UPCMonsterData;

/**
 * 몬스터의 부모. 몬스터 종류마다 클래스를 만들지 않고, 몬스터 정의 에셋(UPCMonsterData) 하나로 구성한다.
 * 수치, 등급, 기술, 공격 선택 규칙, 행동 트리가 모두 그 에셋에서 온다.
 */
UCLASS(Abstract)
class PCCOMBAT_API APCMonsterBase : public APCCharacterBase
{
	GENERATED_BODY()

public:
	APCMonsterBase();

	const UPCMonsterData* GetMonsterData() const { return MonsterData; }

	/** 스폰이 끝나기 전에 정의를 지정한다 (스포너, 테스트용) */
	void SetMonsterData(UPCMonsterData* InMonsterData) { MonsterData = InMonsterData; }

protected:
	virtual void BeginPlay() override;

	// ~APCCharacterBase
	virtual FDataTableRowHandle GetStatsRowHandle() const override;
	virtual void GetStartingAbilities(TArray<const UPCAbilityData*>& OutAbilities) const override;
	virtual void OnStatsInitialized() override;

	/** 이 몬스터의 정의. 레벨에 놓은 개체마다 바꿀 수 있다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PC|Data")
	TObjectPtr<UPCMonsterData> MonsterData;

private:
	/** 등급 규칙(체력, 공격력, 경직 저항 배율)과 분류 태그를 적용한다 */
	void ApplyRank();
};
