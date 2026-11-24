#include "Presentation/PCCombatPresenterComponent.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Characters/PCCharacterBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PCAbilityData.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "GameplayEffectTypes.h"
#include "HAL/IConsoleManager.h"
#include "PCGameplayTags.h"

namespace
{
	TAutoConsoleVariable<int32> CVarShowTelegraph(
		TEXT("pc.Combat.ShowTelegraph"), 0,
		TEXT("1이면 예비 동작 동안 공격 범위를 바닥에 표시한다"));

	const FVector SwordScale(0.05f, 0.05f, 0.9f);
	const FVector ShieldScale(0.6f, 0.6f, 0.05f);
	const FVector ShieldGuardLocation(45.f, 0.f, 20.f);
	const FRotator ShieldGuardRotation(90.f, 0.f, 0.f);
	const FVector OverheadOffset(0.f, 0.f, 130.f);
	constexpr float OverheadMarkerRadius = 18.f;
	constexpr float PopupTextDuration = 0.6f;
	constexpr int32 SectorSegments = 16;

	FColor ThreatColor(const UPCAbilityData* Data)
	{
		if (Data && Data->ThreatTag.MatchesTag(PCTags::Threat_Unblockable))
		{
			return FColor::Red;
		}
		if (Data && Data->ThreatTag.MatchesTag(PCTags::Threat_Heavy))
		{
			return FColor::Orange;
		}
		return FColor::Yellow;
	}
}

UPCCombatPresenterComponent::UPCCombatPresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SwordMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cube.Cube")));
	ShieldMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
}

void UPCCombatPresenterComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<APCCharacterBase>(GetOwner());
	if (!OwnerCharacter)
	{
		return;
	}

	OwnerCharacter->OnCombatCue.AddUObject(this, &UPCCombatPresenterComponent::HandleCombatCue);

	if (bShowWeapons)
	{
		CreateWeaponMesh(SwordMesh, SwordSocket, SwordScale);
		ShieldComponent = CreateWeaponMesh(ShieldMesh, ShieldSocket, ShieldScale);

		OwnerCharacter->GetPCAbilitySystemComponent()
			->RegisterGameplayTagEvent(PCTags::State_Guarding, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UPCCombatPresenterComponent::OnGuardTagChanged);
	}
}

UStaticMeshComponent* UPCCombatPresenterComponent::CreateWeaponMesh(const TSoftObjectPtr<UStaticMesh>& Mesh, FName Socket, const FVector& Scale)
{
	UStaticMesh* Loaded = Mesh.LoadSynchronous();
	if (!Loaded)
	{
		return nullptr;
	}

	UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(OwnerCharacter);
	Component->SetStaticMesh(Loaded);
	Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Component->SetupAttachment(OwnerCharacter->GetMesh(), Socket);
	Component->RegisterComponent();
	Component->SetRelativeScale3D(Scale);
	return Component;
}

void UPCCombatPresenterComponent::OnGuardTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (!ShieldComponent || !OwnerCharacter)
	{
		return;
	}

	const FAttachmentTransformRules Rules(EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget, EAttachmentRule::KeepRelative, false);
	if (NewCount > 0)
	{
		// 방어 중: 방패를 몸 앞으로 올린다
		ShieldComponent->AttachToComponent(OwnerCharacter->GetRootComponent(), Rules);
		ShieldComponent->SetRelativeLocationAndRotation(ShieldGuardLocation, ShieldGuardRotation);
	}
	else
	{
		ShieldComponent->AttachToComponent(OwnerCharacter->GetMesh(), Rules, ShieldSocket);
	}
}

void UPCCombatPresenterComponent::HandleCombatCue(FGameplayTag CueTag, const FGameplayCueParameters& Parameters)
{
	UWorld* World = GetWorld();
	if (!World || !OwnerCharacter)
	{
		return;
	}

	const FVector Location = OwnerCharacter->GetActorLocation();
	const UPCAbilityData* Data = Cast<UPCAbilityData>(Parameters.SourceObject.Get());

	if (CueTag == PCTags::GameplayCue_Combat_Windup)
	{
		const FColor Color = ThreatColor(Data);
		const float Windup = Parameters.RawMagnitude;

		// 머리 위 표시: 예비 동작 동안 유지된다
		DrawDebugSphere(World, Location + OverheadOffset, OverheadMarkerRadius, 12, Color, false, Windup, 0, 2.f);

		if (Data && CVarShowTelegraph.GetValueOnGameThread() != 0)
		{
			DrawSector(Location, OwnerCharacter->GetActorForwardVector(), Data->HitShape, Color, Windup + Data->Timing.Active);
		}
	}
	else if (CueTag == PCTags::GameplayCue_Combat_Hit)
	{
		DrawDebugString(World, Location + OverheadOffset, TEXT("HIT"), nullptr, FColor::Red, PopupTextDuration);
	}
	else if (CueTag == PCTags::GameplayCue_Combat_Block)
	{
		DrawDebugString(World, Location + OverheadOffset, TEXT("BLOCK"), nullptr, FColor::Cyan, PopupTextDuration);
	}
	else if (CueTag == PCTags::GameplayCue_Combat_Interrupt)
	{
		DrawDebugString(World, Location + OverheadOffset * 1.2f, TEXT("STAGGER"), nullptr, FColor::Yellow, Parameters.RawMagnitude);
	}
}

void UPCCombatPresenterComponent::DrawSector(const FVector& Origin, const FVector& Forward, const FPCHitShape& Shape, const FColor& Color, float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector Ground = Origin;
	Ground.Z -= OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 5.f;

	FVector FlatForward = Forward;
	FlatForward.Z = 0.f;
	FlatForward.Normalize();

	FVector Previous = Ground;
	for (int32 Index = 0; Index <= SectorSegments; ++Index)
	{
		const float Angle = -Shape.HalfAngleDeg + (2.f * Shape.HalfAngleDeg) * Index / SectorSegments;
		const FVector Point = Ground + FlatForward.RotateAngleAxis(Angle, FVector::UpVector) * Shape.Range;
		DrawDebugLine(World, Previous, Point, Color, false, Duration, 0, 3.f);
		Previous = Point;
	}
	DrawDebugLine(World, Previous, Ground, Color, false, Duration, 0, 3.f);
}
