#pragma once

#include "CoreMinimal.h"
#include "Characters/PCCharacterBase.h"
#include "PCMonsterBase.generated.h"

/**
 * 몬스터의 부모. 몬스터 종류는 이 클래스의 블루프린트 자식에 데이터와 기술 목록을 넣어 만든다.
 * 1단계에서는 AI 없이 정해진 간격으로 기술 하나를 반복한다. 2단계에서 StateTree가 이 반복을 대체한다.
 */
UCLASS(Abstract)
class PCCOMBAT_API APCMonsterBase : public APCCharacterBase
{
	GENERATED_BODY()

public:
	APCMonsterBase();

protected:
	virtual void BeginPlay() override;

	/** true면 AutoAttackInterval마다 AutoAttackTag 기술을 쓴다 */
	UPROPERTY(EditAnywhere, Category = "PC|AutoAttack")
	bool bAutoAttack = true;

	UPROPERTY(EditAnywhere, Category = "PC|AutoAttack", meta = (Categories = "Ability"))
	FGameplayTag AutoAttackTag;

	UPROPERTY(EditAnywhere, Category = "PC|AutoAttack", meta = (ClampMin = 0.1, Units = "s"))
	float AutoAttackInterval = 3.f;

private:
	/** 플레이어 쪽으로 몸을 돌린 뒤 공격한다. 공격이 시작된 뒤에는 방향이 고정된다. */
	void TryAutoAttack();

	FTimerHandle AutoAttackTimer;
};
