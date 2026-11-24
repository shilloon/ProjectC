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

/** 플레이어 캐릭터. 카메라와 입력만 맡고, 기술은 태그로 능력 시스템에 요청한다. */
UCLASS(Abstract)
class PCCOMBAT_API APCPlayerCharacter : public APCCharacterBase
{
	GENERATED_BODY()

public:
	APCPlayerCharacter();

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

	UPROPERTY(EditDefaultsOnly, Category = "PC|Input")
	TArray<FPCAbilityInputBinding> AbilityBindings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PC|Components")
	TObjectPtr<UCameraComponent> FollowCamera;

private:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void OnAbilityInputPressed(FGameplayTag AbilityTag);
	void OnAbilityInputReleased(FGameplayTag AbilityTag);

	/** 방어 중에는 카메라 방향을 바라보게 해서 방패 방향을 조준할 수 있게 한다 */
	void OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount);
};
