#include "Characters/PCPlayerCharacter.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
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

APCPlayerCharacter::APCPlayerCharacter()
{
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
		.AddUObject(this, &APCPlayerCharacter::OnGuardTagChanged);
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

void APCPlayerCharacter::OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	const bool bGuarding = NewCount > 0;
	bUseControllerRotationYaw = bGuarding;
	GetCharacterMovement()->bOrientRotationToMovement = !bGuarding;
}
