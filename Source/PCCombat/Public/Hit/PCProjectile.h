#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Types/PCCombatTypes.h"
#include "PCProjectile.generated.h"

class UStaticMeshComponent;

/**
 * 날아가는 공격 (화살, 나중에는 몬스터의 투사체).
 * "확정타 없음": 겨눈 대상이 아니라 날아가는 경로에서 실제로 처음 닿은 대상에게 적용된다.
 * 방패 정면에 닿으면 막히고, 벽이나 바닥에 닿으면 사라진다.
 * 매 틱 이동한 구간을 직접 훑기 때문에 빠르게 날아도 대상을 뚫고 지나가지 않는다.
 */
UCLASS()
class PCCOMBAT_API APCProjectile : public AActor
{
	GENERATED_BODY()

public:
	APCProjectile();

	/** 스폰을 마치기 전에 호출한다. Hit.Origin에서 Hit.Direction으로 날아간다. */
	void Launch(const FPCHitInfo& Hit, const FPCProjectileSpec& Spec);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UStaticMeshComponent> Visual;

private:
	/** From에서 To까지 훑어서 닿은 것을 처리한다. 더 날아가지 못하면 true. */
	bool SweepSegment(const FVector& From, const FVector& To);

	UPROPERTY()
	TObjectPtr<AActor> Attacker;

	UPROPERTY()
	TObjectPtr<const UPCAbilityData> AbilityData;

	/** 쏜 위치. 방패가 어느 쪽에서 온 공격인지 판단하는 기준이 된다. */
	FVector LaunchOrigin = FVector::ZeroVector;

	FVector Velocity = FVector::ZeroVector;
	float DamageMultiplier = 1.f;
	float GravityScale = 0.f;
	float MaxDistance = 0.f;
	float Radius = 0.f;
	float TravelledDistance = 0.f;
};
