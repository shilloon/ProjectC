#include "Characters/PCCharacterBase.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "AbilitySystem/Attributes/PCVitalAttributeSet.h"
#include "AbilitySystem/Effects/PCEffect_Damage.h"
#include "AbilitySystem/Effects/PCEffect_Stamina.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Data/PCAbilityData.h"
#include "Data/PCCombatantRow.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffectTypes.h"
#include "Hit/PCHitResolver.h"
#include "PCGameplayTags.h"
#include "TimerManager.h"

namespace
{
	/** 기력 회복 타이머 간격 */
	constexpr float StaminaRegenInterval = 0.1f;
}

APCCharacterBase::APCCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	AbilitySystemComponent = CreateDefaultSubobject<UPCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	VitalSet = CreateDefaultSubobject<UPCVitalAttributeSet>(TEXT("VitalSet"));
}

UAbilitySystemComponent* APCCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APCCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	InitializeFromRow();
	GrantStartingAbilities();

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPCVitalAttributeSet::GetHealthAttribute())
		.AddUObject(this, &APCCharacterBase::OnHealthChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPCVitalAttributeSet::GetStaminaAttribute())
		.AddUObject(this, &APCCharacterBase::OnStaminaChanged);

	GetWorldTimerManager().SetTimer(StaminaRegenTimer, this, &APCCharacterBase::RegenStamina, StaminaRegenInterval, true);
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

	StaminaRegenPerSec = Row.StaminaRegenPerSec;
	StaminaRegenDelay = Row.StaminaRegenDelay;
	GetCharacterMovement()->MaxWalkSpeed = Row.MoveSpeed;
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

	if (!bStaggered)
	{
		bStaggered = true;
		AbilitySystemComponent->AddLooseGameplayTag(PCTags::State_Staggered);
	}

	AbilitySystemComponent->CancelAbilities();
	GetCharacterMovement()->StopMovementImmediately();

	FGameplayCueParameters Parameters;
	Parameters.Location = GetActorLocation();
	Parameters.RawMagnitude = Duration;
	AbilitySystemComponent->ExecuteGameplayCue(PCTags::GameplayCue_Combat_Interrupt, Parameters);

	GetWorldTimerManager().SetTimer(StaggerTimer, this, &APCCharacterBase::EndStagger, Duration, false);
}

void APCCharacterBase::EndStagger()
{
	if (bStaggered)
	{
		bStaggered = false;
		AbilitySystemComponent->RemoveLooseGameplayTag(PCTags::State_Staggered);
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
	if (Data.NewValue < Data.OldValue)
	{
		LastStaminaSpendTime = GetWorld()->GetTimeSeconds();
	}
}

void APCCharacterBase::RegenStamina()
{
	if (IsDead() || HasStateTag(PCTags::State_Guarding))
	{
		return;
	}
	if (GetWorld()->GetTimeSeconds() - LastStaminaSpendTime < StaminaRegenDelay)
	{
		return;
	}
	if (GetStamina() < GetMaxStamina())
	{
		AbilitySystemComponent->ApplyModToAttribute(UPCVitalAttributeSet::GetStaminaAttribute(), EGameplayModOp::Additive, StaminaRegenPerSec * StaminaRegenInterval);
	}
}

void APCCharacterBase::HandleDeath()
{
	GetWorldTimerManager().ClearTimer(StaggerTimer);
	EndStagger();

	AbilitySystemComponent->AddLooseGameplayTag(PCTags::State_Dead);
	AbilitySystemComponent->CancelAbilities();
	GetCharacterMovement()->DisableMovement();

	GetWorldTimerManager().SetTimer(ReviveTimer, this, &APCCharacterBase::Revive, ReviveDelay, false);
}

void APCCharacterBase::Revive()
{
	AbilitySystemComponent->RemoveLooseGameplayTag(PCTags::State_Dead);
	VitalSet->SetHealth(GetMaxHealth());
	VitalSet->SetStamina(GetMaxStamina());
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
