#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PCCombatPresenterComponent.generated.h"

class APCCharacterBase;
class UStaticMesh;
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

	/** 손에 검과 방패 도형을 붙일지 */
	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	bool bShowWeapons = true;

	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	TSoftObjectPtr<UStaticMesh> SwordMesh;

	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	TSoftObjectPtr<UStaticMesh> ShieldMesh;

	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	FName SwordSocket = TEXT("hand_r");

	UPROPERTY(EditAnywhere, Category = "PC|Presentation")
	FName ShieldSocket = TEXT("hand_l");

protected:
	virtual void BeginPlay() override;

private:
	void HandleCombatCue(FGameplayTag CueTag, const FGameplayCueParameters& Parameters);
	void OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount);

	UStaticMeshComponent* CreateWeaponMesh(const TSoftObjectPtr<UStaticMesh>& Mesh, FName Socket, const FVector& Scale);
	void DrawSector(const FVector& Origin, const FVector& Forward, const FPCHitShape& Shape, const FColor& Color, float Duration) const;

	UPROPERTY()
	TObjectPtr<APCCharacterBase> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ShieldComponent;
};
