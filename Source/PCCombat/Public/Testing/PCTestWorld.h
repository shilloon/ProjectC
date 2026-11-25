#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PCAbility_MeleeAttack.h"
#include "AbilitySystem/Abilities/PCAbility_RangedAttack.h"
#include "AbilitySystem/Abilities/PCAbility_SwapWeapon.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PCAbilityData.h"
#include "Data/PCMonsterData.h"
#include "Data/PCMonsterRankRow.h"
#include "Data/PCWeaponData.h"
#include "Equipment/PCEquipmentComponent.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "PCGameplayTags.h"
#include "Testing/PCTestCharacter.h"
#include "Testing/PCTestMonster.h"
#include "Types/PCCombatTypes.h"

/**
 * 자동화 테스트용 게임 월드. 엔진의 GAS 테스트와 같은 방식으로 만들고 직접 틱을 돌린다.
 * 에셋 없이 코드만으로 캐릭터와 몬스터를 스폰해서 전투 규칙을 확인할 수 있다.
 */
struct FPCTestWorld
{
	static constexpr float TickStep = 0.02f;

	UWorld* World = nullptr;

	FPCTestWorld()
	{
		World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("PCCombatTestWorld"));
		FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
		Context.SetCurrentWorld(World);
		World->InitializeActorsForPlay(FURL());
		World->BeginPlay();
		// 게임 모드가 없는 월드는 BeginPlay를 스스로 시작하지 않으므로 직접 알린다
		World->GetWorldSettings()->NotifyBeginPlay();
	}

	~FPCTestWorld()
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	void Tick(float Seconds) const
	{
		for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += TickStep)
		{
			++GFrameCounter;
			World->Tick(LEVELTICK_All, TickStep);
		}
	}

	/** 윗면이 Z=0인 넓은 바닥을 놓는다. 걸어서 이동하는 테스트에 필요하다. */
	void SpawnFloor() const
	{
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector(0.f, 0.f, -50.f), FRotator::ZeroRotator);
		Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
		Floor->SetActorScale3D(FVector(200.f, 200.f, 1.f));
	}

	APCTestCharacter* Spawn(const FVector& Location, float Yaw, UPCAbilityData* Ability = nullptr) const
	{
		const FTransform Transform(FRotator(0.f, Yaw, 0.f), Location);
		APCTestCharacter* Character = World->SpawnActorDeferred<APCTestCharacter>(
			APCTestCharacter::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (Ability)
		{
			Character->AddStartingAbility(Ability);
		}
		Character->FinishSpawning(Transform);
		return Character;
	}

	/** 무기 목록과 교체 기술을 가진 캐릭터를 스폰한다. 첫 번째 무기를 들고 시작한다. */
	APCTestCharacter* SpawnArmed(const FVector& Location, float Yaw, const TArray<UPCWeaponData*>& Weapons, UPCAbilityData* SwapAbility) const
	{
		const FTransform Transform(FRotator(0.f, Yaw, 0.f), Location);
		APCTestCharacter* Character = World->SpawnActorDeferred<APCTestCharacter>(
			APCTestCharacter::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Character->GetEquipment()->Loadout.Append(Weapons);
		Character->GetEquipment()->SwapAbility = SwapAbility;
		Character->FinishSpawning(Transform);
		return Character;
	}

	APCTestMonster* SpawnMonster(const FVector& Location, float Yaw, UPCMonsterData* MonsterData) const
	{
		const FTransform Transform(FRotator(0.f, Yaw, 0.f), Location);
		APCTestMonster* Monster = World->SpawnActorDeferred<APCTestMonster>(
			APCTestMonster::StaticClass(), Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		Monster->SetMonsterData(MonsterData);
		Monster->FinishSpawning(Transform);
		return Monster;
	}
};

namespace PCTest
{
	/** 근접 공격 데이터. 기술 태그는 Ability.Attack, 기력 소모 10, 범위 200cm 부채꼴 45도. */
	inline UPCAbilityData* MakeAttackData(float Damage)
	{
		UPCAbilityData* Data = NewObject<UPCAbilityData>(GetTransientPackage());
		Data->AbilityClass = UPCAbility_MeleeAttack::StaticClass();
		Data->AbilityTag = PCTags::Ability_Attack;
		Data->Timing.Windup = 0.1f;
		Data->Timing.Active = 0.06f;
		Data->Timing.Recovery = 0.1f;
		Data->StaminaCost = 10.f;
		Data->Damage = Damage;
		Data->GuardStaminaDamage = 30.f;
		Data->ThreatTag = PCTags::Threat_Light;
		Data->HitShape.Range = 200.f;
		Data->HitShape.HalfAngleDeg = 45.f;
		return Data;
	}

	/** 타격 반응 한 줄을 추가하고, 조건을 더 붙일 수 있게 돌려준다 */
	inline FPCHitReaction& AddReaction(UPCAbilityData* Data, EPCHitReactionWhen When, float StaggerTime, float KnockbackDistance)
	{
		FPCHitReaction& Reaction = Data->HitReactions.AddDefaulted_GetRef();
		Reaction.When = When;
		Reaction.StaggerTime = StaggerTime;
		Reaction.KnockbackDistance = KnockbackDistance;
		return Reaction;
	}

	/** 원거리 공격 데이터. 기술 태그는 Ability.Attack, 직선으로 2000cm/s, 최대 3000cm. */
	inline UPCAbilityData* MakeRangedData(float Damage)
	{
		UPCAbilityData* Data = NewObject<UPCAbilityData>(GetTransientPackage());
		Data->AbilityClass = UPCAbility_RangedAttack::StaticClass();
		Data->AbilityTag = PCTags::Ability_Attack;
		Data->Timing.Windup = 0.1f;
		Data->Timing.Active = 0.04f;
		Data->Timing.Recovery = 0.1f;
		Data->StaminaCost = 10.f;
		Data->Damage = Damage;
		Data->GuardStaminaDamage = 30.f;
		Data->ThreatTag = PCTags::Threat_Light;
		Data->Projectile.Speed = 2000.f;
		Data->Projectile.GravityScale = 0.f;
		Data->Projectile.MaxDistance = 3000.f;
		return Data;
	}

	/** 무기 교체 기술 데이터. 넣고 드는 시간은 무기에서 오고, 그 밖의 시간은 0이다. */
	inline UPCAbilityData* MakeSwapData()
	{
		UPCAbilityData* Data = NewObject<UPCAbilityData>(GetTransientPackage());
		Data->AbilityClass = UPCAbility_SwapWeapon::StaticClass();
		Data->AbilityTag = PCTags::Ability_SwapWeapon;
		Data->Timing.Windup = 0.f;
		Data->Timing.Active = 0.f;
		Data->Timing.Recovery = 0.f;
		return Data;
	}

	/** 주 공격 슬롯에 기술 하나를 가진 무기 */
	inline UPCWeaponData* MakeWeapon(UPCAbilityData* PrimaryAbility, float SheatheTime, float DrawTime)
	{
		UPCWeaponData* Weapon = NewObject<UPCWeaponData>(GetTransientPackage());
		Weapon->WeaponTag = PCTags::Weapon;
		Weapon->SheatheTime = SheatheTime;
		Weapon->DrawTime = DrawTime;

		FPCWeaponAbilitySlot& Slot = Weapon->AbilitySlots.AddDefaulted_GetRef();
		Slot.InputTag = PCTags::Input_Primary;
		Slot.Ability = PrimaryAbility;
		return Weapon;
	}

	/** 등급 행 하나만 가진 데이터테이블을 메모리에 만들어 몬스터 정의에 연결한다 */
	inline void SetRank(UPCMonsterData& MonsterData, FGameplayTag RankTag, float Health, float Damage, float StaggerTime)
	{
		UDataTable* Table = NewObject<UDataTable>(GetTransientPackage());
		Table->RowStruct = FPCMonsterRankRow::StaticStruct();

		FPCMonsterRankRow Row;
		Row.RankTag = RankTag;
		Row.HealthMultiplier = Health;
		Row.DamageMultiplier = Damage;
		Row.StaggerTimeMultiplier = StaggerTime;
		Table->AddRow(TEXT("Rank"), Row);

		MonsterData.RankRow.DataTable = Table;
		MonsterData.RankRow.RowName = TEXT("Rank");
	}

	inline FPCHitInfo MakeHit(AActor* Attacker, const UPCAbilityData* Data)
	{
		FPCHitInfo Hit;
		Hit.Attacker = Attacker;
		Hit.Origin = Attacker->GetActorLocation();
		Hit.Direction = Attacker->GetActorForwardVector();
		Hit.Data = Data;
		return Hit;
	}
}
