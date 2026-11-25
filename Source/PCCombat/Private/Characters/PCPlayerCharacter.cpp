#include "Characters/PCPlayerCharacter.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "Data/PCAbilityData.h"
#include "Data/PCWeaponData.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "Equipment/PCEquipmentComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "PCGameplayTags.h"
#include "PCLog.h"

namespace
{
	/** 화면 가운데에서 이 거리까지를 겨눌 수 있는 곳으로 본다 */
	constexpr float AimTraceDistance = 10000.f;
}

APCPlayerCharacter::APCPlayerCharacter()
{
	Equipment = CreateDefaultSubobject<UPCEquipmentComponent>(TEXT("Equipment"));

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 720.f, 0.f);
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 450.f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void APCPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->RegisterGameplayTagEvent(PCTags::State_Guarding, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APCPlayerCharacter::OnFaceCameraTagChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(PCTags::State_Aiming, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APCPlayerCharacter::OnFaceCameraTagChanged);
}

FVector APCPlayerCharacter::GetAimDirection(const FVector& From) const
{
	if (!GetController())
	{
		return Super::GetAimDirection(From);
	}

	// 카메라는 캐릭터 뒤 위쪽에 있으므로, 화면 가운데가 가리키는 지점을 찾고 손에서 그 지점을 향한다
	const FVector CameraLocation = FollowCamera->GetComponentLocation();
	const FVector CameraForward = FollowCamera->GetForwardVector();
	FVector AimPoint = CameraLocation + CameraForward * AimTraceDistance;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PCPlayerAim), false, this);
	FHitResult Result;
	if (GetWorld()->LineTraceSingleByChannel(Result, CameraLocation, AimPoint, ECC_Visibility, QueryParams))
	{
		// 카메라와 캐릭터 사이에 있는 것을 겨누면 뒤로 쏘게 되므로, 캐릭터보다 앞에 있을 때만 쓴다
		if (FVector::DotProduct(Result.ImpactPoint - From, CameraForward) > 0.f)
		{
			AimPoint = Result.ImpactPoint;
		}
	}

	const FVector Direction = (AimPoint - From).GetSafeNormal();
	return Direction.IsNearlyZero() ? CameraForward : Direction;
}

void APCPlayerCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		for (const UInputMappingContext* Context : MappingContexts)
		{
			if (Context)
			{
				Subsystem->AddMappingContext(Context, 0);
			}
		}
	}
}

void APCPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		UE_LOG(LogPC, Error, TEXT("'%s': Enhanced Input 컴포넌트가 없습니다"), *GetNameSafe(this));
		return;
	}

	if (MoveAction)
	{
		Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::Move);
	}
	if (LookAction)
	{
		Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::Look);
	}
	if (MouseLookAction)
	{
		Input->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::Look);
	}

	for (const FPCAbilityInputBinding& Binding : AbilityBindings)
	{
		if (!Binding.InputAction || !Binding.AbilityTag.IsValid())
		{
			continue;
		}

		if (Binding.bHold)
		{
			// 누르는 동안 매 프레임 발동을 시도한다. 다른 행동이 끝나는 즉시 방어가 올라온다.
			Input->BindAction(Binding.InputAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::OnAbilityInputPressed, Binding.AbilityTag);
			Input->BindAction(Binding.InputAction, ETriggerEvent::Completed, this, &APCPlayerCharacter::OnAbilityInputReleased, Binding.AbilityTag);
		}
		else
		{
			Input->BindAction(Binding.InputAction, ETriggerEvent::Started, this, &APCPlayerCharacter::OnAbilityInputPressed, Binding.AbilityTag);
		}
	}

	// 슬롯의 기술이 누르는 동안 유지하는 것인지는 무기마다 다르므로, 세 가지 입력 시점을 모두 받아 그때 판단한다
	for (const FPCWeaponSlotInputBinding& Binding : WeaponSlotBindings)
	{
		if (!Binding.InputAction || !Binding.InputTag.IsValid())
		{
			continue;
		}

		Input->BindAction(Binding.InputAction, ETriggerEvent::Started, this, &APCPlayerCharacter::OnWeaponSlotStarted, Binding.InputTag);
		Input->BindAction(Binding.InputAction, ETriggerEvent::Triggered, this, &APCPlayerCharacter::OnWeaponSlotHeld, Binding.InputTag);
		Input->BindAction(Binding.InputAction, ETriggerEvent::Completed, this, &APCPlayerCharacter::OnWeaponSlotReleased, Binding.InputTag);
	}

	for (int32 Index = 0; Index < WeaponSelectActions.Num(); ++Index)
	{
		if (WeaponSelectActions[Index])
		{
			Input->BindAction(WeaponSelectActions[Index], ETriggerEvent::Started, this, &APCPlayerCharacter::OnWeaponSelect, Index);
		}
	}
}

void APCPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!GetController() || HasStateTag(PCTags::State_Staggered) || IsDead())
	{
		return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRotation(0.f, GetController()->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawRotation);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), Axis.X);
}

void APCPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void APCPlayerCharacter::OnAbilityInputPressed(FGameplayTag AbilityTag)
{
	AbilitySystemComponent->TryActivateByAbilityTag(AbilityTag);
}

void APCPlayerCharacter::OnAbilityInputReleased(FGameplayTag AbilityTag)
{
	AbilitySystemComponent->CancelByAbilityTag(AbilityTag);
}

void APCPlayerCharacter::OnWeaponSlotStarted(FGameplayTag InputTag)
{
	const FPCWeaponAbilitySlot* Slot = Equipment->FindSlot(InputTag);
	if (Slot && Slot->Ability && !Slot->bHold)
	{
		AbilitySystemComponent->TryActivateByAbilityTag(Slot->Ability->AbilityTag);
	}
}

void APCPlayerCharacter::OnWeaponSlotHeld(FGameplayTag InputTag)
{
	// 누르는 동안 매 프레임 발동을 시도한다. 다른 행동이 끝나는 즉시 방어가 올라온다.
	const FPCWeaponAbilitySlot* Slot = Equipment->FindSlot(InputTag);
	if (Slot && Slot->Ability && Slot->bHold)
	{
		AbilitySystemComponent->TryActivateByAbilityTag(Slot->Ability->AbilityTag);
	}
}

void APCPlayerCharacter::OnWeaponSlotReleased(FGameplayTag InputTag)
{
	const FPCWeaponAbilitySlot* Slot = Equipment->FindSlot(InputTag);
	if (Slot && Slot->Ability && Slot->bHold)
	{
		AbilitySystemComponent->CancelByAbilityTag(Slot->Ability->AbilityTag);
	}
}

void APCPlayerCharacter::OnWeaponSelect(int32 Index)
{
	Equipment->RequestSwap(Index);
}

void APCPlayerCharacter::OnFaceCameraTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bFaceCamera = HasStateTag(PCTags::State_Guarding) || HasStateTag(PCTags::State_Aiming);
	bUseControllerRotationYaw = bFaceCamera;
	GetCharacterMovement()->bOrientRotationToMovement = !bFaceCamera;
}
