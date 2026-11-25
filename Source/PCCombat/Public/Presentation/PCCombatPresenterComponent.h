#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PCCombatPresenterComponent.generated.h"

class APCCharacterBase;
class UPCWeaponData;
class UStaticMeshComponent;
struct FGameplayCueParameters;
struct FPCHitShape;

/**
 * 임시 겉모습. 판정 코드가 내보낸 GameplayCue와 상태 태그를 듣고 도형과 디버그 표시로 보여준다.
 * 모델과 애니메이션이 준비되면 이 컴포넌트만 교체한다. 판정 코드는 이 컴포넌트를 모른다.
 */
UCLASS(ClassGroup = "PC", meta = (BlueprintSpawnableComponent))
class PCCOMBAT_API UPCCombatPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPCCombatPresenterComponent();

	/** 지금 든 무기의 도형을 손에 붙일지. 도형은 무기 에셋의 외형 목록에서 온다. */
	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	bool bShowWeapons = true;

protected:
	virtual void BeginPlay() override;

private:
	void HandleCombatCue(FGameplayTag CueTag, const FGameplayCueParameters& Parameters);
	void OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** 든 무기가 바뀌면 손의 도형을 새 무기의 것으로 바꾼다 */
	void OnWeaponChanged(const UPCWeaponData* NewWeapon, const UPCWeaponData* OldWeapon);
	/** 아주 짧게 액터의 시간을 멈춘다 */
	void PlayHitStop(float Duration);
	void EndHitStop();

	/** 플레이어가 조종하는 캐릭터일 때만 카메라를 흔든다 */
	void PlayCameraShake(float Scale) const;

	FTimerHandle HitStopTimer;

	void DrawSector(const FVector& Origin, const FVector& Forward, const FPCHitShape& Shape, const FColor& Color, float Duration) const;

	UPROPERTY()
	TObjectPtr<APCCharacterBase> OwnerCharacter;

	/** 지금 든 무기의 도형들 */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> WeaponParts;

	/** WeaponParts 중 방어할 때 몸 앞으로 올라오는 것 (방패). 없으면 nullptr. */
	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GuardPart;

	FName GuardPartSocket;
};
