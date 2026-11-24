#include "Characters/PCCharacterBase.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "AbilitySystem/Effects/PCEffect_Damage.h"
#include "AbilitySystem/Effects/PCEffect_Dead.h"
#include "AbilitySystem/Effects/PCEffect_Stagger.h"
#include "AbilitySystem/Effects/PCEffect_Stamina.h"
#include "AbilitySystem/Effects/PCEffect_StaminaRegen.h"
#include "AbilitySystem/Effects/PCEffect_StaminaRegenDelay.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/PCAbilityData.h"
#include "Data/PCCombatantRow.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "Hit/PCHitResolver.h"
#include "PCGameplayTags.h"
#include "Presentation/PCCombatPresenterComponent.h"
#include "TimerManager.h"

APCCharacterBase::APCCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	AbilitySystemComponent = CreateDefaultSubobject<UPCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	VitalSet = CreateDefaultSubobject<UPCVitalAttributeSet>(TEXT("VitalSet"));
	Presenter = CreateDefaultSubobject<UPCCombatPresenterComponent>(TEXT("Presenter"));
}

UAbilitySystemComponent* APCCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APCCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPCVitalAttributeSet::GetHealthAttribute())
		.AddUObject(this, &APCCharacterBase::OnHealthChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPCVitalAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &APCCharacterBase::OnStaminaChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(PCTags::State_Staggered, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APCCharacterBase::OnStaggerTagChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(PCTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APCCharacterBase::OnDeadTagChanged);

	InitializeFromRow();
	GrantStartingAbilities();
}

void APCCharacterBase::InitializeFromRow()
{
	FPCCombatantRow Row;
	if (!CombatantRow.IsNull())
	{
		if (const FPCCombatantRow* Found = CombatantRow.GetRow<FPCCombatantRow>(TEXT("APCCharacterBase")))
		{
			Row = *Found;
		}
	}

	VitalSet->InitMaxHealth(Row.MaxHealth);
	VitalSet->InitHealth(Row.MaxHealth);
	VitalSet->InitMaxStamina(Row.MaxStamina);
	VitalSet->InitStamina(Row.MaxStamina);

	StaminaRegenDelay = Row.StaminaRegenDelay;
	GetCharacterMovement()->MaxWalkSpeed = Row.MoveSpeed;

	// 기력 회복은 계속 걸려 있는 주기 이펙트가 맡는다
	if (Row.StaminaRegenPerSec > 0.f)
	{
		AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_StaminaRegen::StaticClass(), PCTags::Data_Stamina,
			Row.StaminaRegenPerSec * UPCEffect_StaminaRegen::PeriodSeconds, this);
	}
}

void APCCharacterBase::GrantStartingAbilities()
{
	for (const UPCAbilityData* Data : StartingAbilities)
	{
		AbilitySystemComponent->GrantAbilityFromData(Data);
	}
}

bool APCCharacterBase::IsDead() const
{
	return HasStateTag(PCTags::State_Dead);
}

bool APCCharacterBase::HasStateTag(FGameplayTag Tag) const
{
	return AbilitySystemComponent->HasMatchingGameplayTag(Tag);
}

float APCCharacterBase::GetHealth() const { return VitalSet->GetHealth(); }
float APCCharacterBase::GetMaxHealth() const { return VitalSet->GetMaxHealth(); }
float APCCharacterBase::GetStamina() const { return VitalSet->GetStamina(); }
float APCCharacterBase::GetMaxStamina() const { return VitalSet->GetMaxStamina(); }

EPCHitOutcome APCCharacterBase::ReceiveHit(const FPCHitInfo& Hit)
{
	if (!Hit.Data || IsDead())
	{
		return EPCHitOutcome::Miss;
	}

	const bool bGuarding = HasStateTag(PCTags::State_Guarding);
	const bool bBlockable = !Hit.Data->ThreatTag.MatchesTag(PCTags::Threat_Unblockable);
	const bool bFromFront = FPCHitResolver::IsAttackFromFront(GetActorLocation(), GetActorForwardVector(), Hit.Origin, GuardHalfAngleDeg);

	if (bGuarding && bBlockable && bFromFront)
	{
		if (Hit.Data->GuardStaminaDamage > 0.f)
		{
			AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_Stamina::StaticClass(), PCTags::Data_Stamina, -Hit.Data->GuardStaminaDamage, Hit.Attacker);
		}
		ExecuteCue(PCTags::GameplayCue_Combat_Block, Hit);

		if (GetStamina() <= 0.f)
		{
			ApplyStagger(GuardBreakStaggerTime);
		}
		return EPCHitOutcome::Blocked;
	}

	// 피해로 죽거나 경직되면 상태가 바뀌므로, 예비 동작 중이었는지는 먼저 기록한다
	const bool bWasWindingUp = HasStateTag(PCTags::State_Action_Windup);

	if (Hit.Data->Damage > 0.f)
	{
		AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_Damage::StaticClass(), PCTags::Data_Damage, -Hit.Data->Damage, Hit.Attacker);
	}
	ExecuteCue(PCTags::GameplayCue_Combat_Hit, Hit);

	if (!IsDead() && Hit.Data->StaggerTime > 0.f && (!Hit.Data->bStaggerOnlyOnWindup || bWasWindingUp))
	{
		ApplyStagger(Hit.Data->StaggerTime);
	}
	return EPCHitOutcome::Hit;
}

void APCCharacterBase::ExecuteCue(FGameplayTag CueTag, const FPCHitInfo& Hit)
{
	FGameplayCueParameters Parameters;
	Parameters.Instigator = Hit.Attacker;
	Parameters.SourceObject = Hit.Data;
	Parameters.Location = GetActorLocation();
	AbilitySystemComponent->ExecuteGameplayCue(CueTag, Parameters);
}

void APCCharacterBase::HandleGameplayCue(AActor* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	IGameplayCueInterface::HandleGameplayCue(Self, GameplayCueTag, EventType, Parameters);
	OnCombatCue.Broadcast(GameplayCueTag, Parameters);
}

void APCCharacterBase::ApplyStagger(float Duration)
{
	if (Duration <= 0.f || IsDead())
	{
		return;
	}

	AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_Stagger::StaticClass(), PCTags::Data_Duration, Duration, this);

	FGameplayCueParameters Parameters;
	Parameters.Location = GetActorLocation();
	Parameters.RawMagnitude = Duration;
	AbilitySystemComponent->ExecuteGameplayCue(PCTags::GameplayCue_Combat_Interrupt, Parameters);
}

void APCCharacterBase::OnStaggerTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		AbilitySystemComponent->CancelAbilities();
		GetCharacterMovement()->StopMovementImmediately();
	}
}

void APCCharacterBase::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && !IsDead())
	{
		HandleDeath();
	}
}

void APCCharacterBase::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	// 기력을 썼으면 잠시 회복을 멈춘다. 다시 쓰면 멈추는 시간이 갱신된다.
	if (Data.NewValue < Data.OldValue && StaminaRegenDelay > 0.f)
	{
		AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_StaminaRegenDelay::StaticClass(), PCTags::Data_Duration, StaminaRegenDelay, this);
	}
}

void APCCharacterBase::HandleDeath()
{
	AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PCTags::State_Staggered));
	AbilitySystemComponent->ApplyGameplayEffectToSelf(GetDefault<UPCEffect_Dead>(), 1.f, AbilitySystemComponent->MakeEffectContext());
}

void APCCharacterBase::OnDeadTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		AbilitySystemComponent->CancelAbilities();
		GetCharacterMovement()->DisableMovement();
		GetWorldTimerManager().SetTimer(ReviveTimer, this, &APCCharacterBase::Revive, ReviveDelay, false);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
}

void APCCharacterBase::Revive()
{
	AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PCTags::State_Dead));
	VitalSet->SetHealth(GetMaxHealth());
	VitalSet->SetStamina(GetMaxStamina());
}
