#pragma once

#include "CoreMinimal.h"
#include "Characters/PCCharacterBase.h"
#include "PCPlayerCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
struct FInputActionValue;

/** 입력 하나와 기술 태그 하나의 연결 */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCAbilityInputBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	/** true면 누르는 동안 유지하고 떼면 취소한다 (방패 올리기) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	bool bHold = false;
};

/** 입력 하나와 입력 슬롯 하나의 연결. 슬롯에 어떤 기술이 들어가는지는 지금 든 무기가 정한다. */
USTRUCT(BlueprintType)
struct PCCOMBAT_API FPCWeaponSlotInputBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InputAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (Categories = "Input"))
	FGameplayTag InputTag;
};

/** 플레이어 캐릭터. 카메라와 입력만 맡고, 기술은 태그로 능력 시스템에 요청한다. */
UCLASS(Abstract)
class PCCOMBAT_API APCPlayerCharacter : public APCCharacterBase
{
	GENERATED_BODY()

public:
	APCPlayerCharacter();

	/** 화면 가운데가 가리키는 곳을 향한다 */
	virtual FVector GetAimDirection(const FVector& From) const override;

protected:
	virtual void BeginPlay() override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TArray<TObjectPtr<UInputMappingContext>> MappingContexts;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TObjectPtr<UInputAction> MouseLookAction;

	/** 무기와 상관없이 항상 쓰는 기술 (회피) */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TArray<FPCAbilityInputBinding> AbilityBindings;

	/** 무기가 기술을 정하는 입력 슬롯 (주 공격, 보조, 특수) */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TArray<FPCWeaponSlotInputBinding> WeaponSlotBindings;

	/** 무기 선택 입력. 순서가 장비 컴포넌트의 무기 번호와 같다. */
	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TArray<TObjectPtr<UInputAction>> WeaponSelectActions;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UCameraComponent> FollowCamera;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnAbilityInputPressed(FGameplayTag AbilityTag);
	void OnAbilityInputReleased(FGameplayTag AbilityTag);

	void OnWeaponSlotStarted(FGameplayTag InputTag);
	void OnWeaponSlotHeld(FGameplayTag InputTag);
	void OnWeaponSlotReleased(FGameplayTag InputTag);
	void OnWeaponSelect(int32 Index);

	/** 방어하거나 겨누는 동안에는 카메라 방향을 바라보게 해서 방패와 조준 방향을 정할 수 있게 한다 */
	void OnFaceCameraTagChanged(const FGameplayTag Tag, int32 NewCount);
};
