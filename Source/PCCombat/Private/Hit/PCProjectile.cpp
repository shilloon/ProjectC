#include "Hit/PCProjectile.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PCAbilityData.h"
#include "Engine/HitResult.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Hit/PCHitResolver.h"
#include "Hit/PCHittable.h"
#include "PCLog.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	/** 임시 외형: 가늘고 긴 막대 */
	const FVector VisualScale(0.6f, 0.03f, 0.03f);
}

APCProjectile::APCProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// 판정은 매 틱 직접 훑어서 하므로 컴포넌트에는 충돌을 두지 않는다
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeScale3D(VisualScale);
	RootComponent = Visual;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		Visual->SetStaticMesh(CubeMesh.Object);
	}
}

void APCProjectile::Launch(const FPCHitInfo& Hit, const FPCProjectileSpec& Spec)
{
	Attacker = Hit.Attacker;
	AbilityData = Hit.Data;
	LaunchOrigin = Hit.Origin;
	DamageMultiplier = Hit.DamageMultiplier;
	Velocity = Hit.Direction.GetSafeNormal() * Spec.Speed;
	GravityScale = Spec.GravityScale;
	MaxDistance = Spec.MaxDistance;
	Radius = Spec.Radius;
}

void APCProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const UWorld* World = GetWorld();
	if (!World || !AbilityData)
	{
		Destroy();
		return;
	}

	Velocity.Z += World->GetGravityZ() * GravityScale * DeltaSeconds;

	const FVector From = GetActorLocation();
	const FVector To = From + Velocity * DeltaSeconds;
	if (SweepSegment(From, To))
	{
		Destroy();
		return;
	}

	SetActorLocationAndRotation(To, Velocity.Rotation());

	TravelledDistance += static_cast<float>(FVector::Dist(From, To));
	if (TravelledDistance >= MaxDistance)
	{
		UE_LOG(LogPC, Log, TEXT("[전투] %s의 %s: 빗나감"), *GetNameSafe(Attacker), *AbilityData->AbilityTag.ToString());
		Destroy();
	}
}

bool APCProjectile::SweepSegment(const FVector& From, const FVector& To)
{
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PCProjectile), false, this);
	QueryParams.AddIgnoredActor(Attacker);

	TArray<FHitResult> Hits;
	GetWorld()->SweepMultiByObjectType(Hits, From, To, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Radius), QueryParams);
	Hits.Sort([](const FHitResult& A, const FHitResult& B) { return A.Distance < B.Distance; });

	FPCHitInfo Hit;
	Hit.Attacker = Attacker;
	Hit.Origin = LaunchOrigin;
	Hit.Direction = Velocity.GetSafeNormal();
	Hit.Data = AbilityData;
	Hit.DamageMultiplier = DamageMultiplier;

	// 가까운 순서로 본다. 한 액터의 컴포넌트가 여러 개 걸려도 한 번만 처리한다.
	TArray<AActor*, TInlineAllocator<8>> Seen;
	for (const FHitResult& Result : Hits)
	{
		AActor* Other = Result.GetActor();
		if (!Other || Seen.Contains(Other))
		{
			continue;
		}
		Seen.Add(Other);

		if (!Other->Implements<UPCHittable>())
		{
			// 벽이나 바닥
			UE_LOG(LogPC, Log, TEXT("[전투] %s의 %s: 빗나감 (%s에 박힘)"), *GetNameSafe(Attacker), *AbilityData->AbilityTag.ToString(), *GetNameSafe(Other));
			return true;
		}

		// 이미 죽은 대상처럼 공격을 받지 않는 대상은 가리지 않으므로 그대로 지나간다
		if (FPCHitResolver::ApplyHit(Other, Hit) != EPCHitOutcome::Miss)
		{
			return true;
		}
	}
	return false;
}
