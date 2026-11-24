#include "Characters/PCCharacterBase.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/PCCombatAttributeSet.h"
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
#include "GameFramework/RootMotionSource.h"
#include "TimerManager.h"

namespace
{
	/** 밀려나는 데 걸리는 시간 */
	constexpr float KnockbackDuration = 0.15f;

	/** 돌진이나 회피 같은 다른 이동보다 밀려남이 우선한다 */
	constexpr uint16 KnockbackPriority = 10;

	const FName KnockbackSourceName(TEXT("PCKnockback"));
}

APCCharacterBase::APCCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	AbilitySystemComponent = CreateDefaultSubobject<UPCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	VitalSet = CreateDefaultSubobject<UPCVitalAttributeSet>(TEXT("VitalSet"));
	CombatSet = CreateDefaultSubobject<UPCCombatAttributeSet>(TEXT("CombatSet"));
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
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPCCombatAttributeSet::GetMoveSpeedMultiplierAttribute())
		.AddUObject(this, &APCCharacterBase::OnMoveSpeedMultiplierChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(PCTags::State_Staggered, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APCCharacterBase::OnStaggerTagChanged);
	AbilitySystemComponent->RegisterGameplayTagEvent(PCTags::State_Dead, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &APCCharacterBase::OnDeadTagChanged);

	InitializeFromRow();
	OnStatsInitialized();
	GrantStartingAbilities();
}

void APCCharacterBase::InitializeFromRow()
{
	FPCCombatantRow Row;
	const FDataTableRowHandle StatsRow = GetStatsRowHandle();
	if (!StatsRow.IsNull())
	{
		if (const FPCCombatantRow* Found = StatsRow.GetRow<FPCCombatantRow>(TEXT("APCCharacterBase")))
		{
			Row = *Found;
		}
	}

	VitalSet->InitMaxHealth(Row.MaxHealth);
	VitalSet->InitHealth(Row.MaxHealth);
	VitalSet->InitMaxStamina(Row.MaxStamina);
	VitalSet->InitStamina(Row.MaxStamina);

	StaminaRegenDelay = Row.StaminaRegenDelay;
	BaseMoveSpeed = Row.MoveSpeed;
	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * CombatSet->GetMoveSpeedMultiplier();

	// 기력 회복은 계속 걸려 있는 주기 이펙트가 맡는다
	if (Row.StaminaRegenPerSec > 0.f)
	{
		AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_StaminaRegen::StaticClass(), PCTags::Data_Stamina,
			Row.StaminaRegenPerSec * UPCEffect_StaminaRegen::PeriodSeconds, this);
	}
}

void APCCharacterBase::GrantStartingAbilities()
{
	TArray<const UPCAbilityData*> Abilities;
	GetStartingAbilities(Abilities);
	for (const UPCAbilityData* Data : Abilities)
	{
		AbilitySystemComponent->GrantAbilityFromData(Data);
	}
}

void APCCharacterBase::GetStartingAbilities(TArray<const UPCAbilityData*>& OutAbilities) const
{
	OutAbilities.Append(StartingAbilities);
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
float APCCharacterBase::GetAttackPower() const { return CombatSet->GetAttackPower(); }

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
		ApplyKnockback(Hit.Origin, Hit.Data->BlockedKnockbackDistance);

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
		AbilitySystemComponent->ApplySetByCallerEffectToSelf(UPCEffect_Damage::StaticClass(), PCTags::Data_Damage, -Hit.Data->Damage * Hit.DamageMultiplier, Hit.Attacker);
	}
	ExecuteCue(PCTags::GameplayCue_Combat_Hit, Hit);
	ApplyKnockback(Hit.Origin, Hit.Data->KnockbackDistance);

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
	// 경직 저항만큼 시간이 줄고, 저항이 1이면 경직되지 않는다
	Duration *= 1.f - CombatSet->GetStaggerResistance();
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

void APCCharacterBase::ApplyKnockback(const FVector& FromLocation, float Distance)
{
	Distance *= 1.f - CombatSet->GetStaggerResistance();
	if (Distance <= 0.f || IsDead())
	{
		return;
	}

	FVector Direction = GetActorLocation() - FromLocation;
	Direction.Z = 0.f;
	if (!Direction.Normalize())
	{
		Direction = -GetActorForwardVector();
	}

	// 짧은 시간 동안 일정한 속도로 밀어서 정해진 거리만큼 정확히 이동시킨다
	TSharedPtr<FRootMotionSource_ConstantForce> Force = MakeShared<FRootMotionSource_ConstantForce>();
	Force->InstanceName = KnockbackSourceName;
	Force->AccumulateMode = ERootMotionAccumulateMode::Override;
	Force->Priority = KnockbackPriority;
	Force->Force = Direction * (Distance / KnockbackDuration);
	Force->Duration = KnockbackDuration;
	Force->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	Force->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	GetCharacterMovement()->ApplyRootMotionSource(Force);
}

void APCCharacterBase::OnMoveSpeedMultiplierChanged(const FOnAttributeChangeData& Data)
{
	GetCharacterMovement()->MaxWalkSpeed = BaseMoveSpeed * Data.NewValue;
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
