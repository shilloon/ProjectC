#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Abilities/PCAbility_MeleeAttack.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Data/PCAbilityData.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Hit/PCHitResolver.h"
#include "PCGameplayTags.h"
#include "Tests/PCTestCharacter.h"

namespace
{
	constexpr float TestTickStep = 0.02f;

	/** 테스트용 게임 월드. 엔진의 GAS 테스트와 같은 방식으로 만들고 직접 틱을 돌린다. */
	struct FPCTestWorld
	{
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
			for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += TestTickStep)
			{
				++GFrameCounter;
				World->Tick(LEVELTICK_All, TestTickStep);
			}
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
	};

	UPCAbilityData* MakeAttackData(float Damage)
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

	FPCHitInfo MakeHit(AActor* Attacker, const UPCAbilityData* Data)
	{
		FPCHitInfo Hit;
		Hit.Attacker = Attacker;
		Hit.Origin = Attacker->GetActorLocation();
		Hit.Direction = Attacker->GetActorForwardVector();
		Hit.Data = Data;
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCCombatReceiveHitTest, "PC.Combat.Rules.ReceiveHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCCombatReceiveHitTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	UPCAbilityData* Attack = MakeAttackData(20.f);
	APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 0, 200), 0.f);

	// 맞음
	{
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		TestEqual(TEXT("막지 않으면 맞는다"), Victim->ReceiveHit(MakeHit(Attacker, Attack)), EPCHitOutcome::Hit);
		TestEqual(TEXT("맞으면 체력이 피해만큼 줄어든다"), Victim->GetHealth(), 80.f);
		Victim->Destroy();
	}

	// 정면 방어
	{
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		Victim->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
		TestEqual(TEXT("정면에서 온 공격은 막는다"), Victim->ReceiveHit(MakeHit(Attacker, Attack)), EPCHitOutcome::Blocked);
		TestEqual(TEXT("막으면 체력은 그대로다"), Victim->GetHealth(), 100.f);
		TestEqual(TEXT("막으면 기력이 줄어든다"), Victim->GetStamina(), 70.f);
		Victim->Destroy();
	}

	// 등 뒤에서 온 공격
	{
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 200), 0.f);
		Victim->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
		TestEqual(TEXT("방패를 들어도 등 뒤 공격은 맞는다"), Victim->ReceiveHit(MakeHit(Attacker, Attack)), EPCHitOutcome::Hit);
		TestEqual(TEXT("등 뒤 공격은 체력을 깎는다"), Victim->GetHealth(), 80.f);
		Victim->Destroy();
	}

	// 막을 수 없는 공격
	{
		UPCAbilityData* Unblockable = MakeAttackData(20.f);
		Unblockable->ThreatTag = PCTags::Threat_Unblockable;
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		Victim->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
		TestEqual(TEXT("막을 수 없는 공격은 정면 방어를 뚫는다"), Victim->ReceiveHit(MakeHit(Attacker, Unblockable)), EPCHitOutcome::Hit);
		Victim->Destroy();
	}

	// 방어 중 기력 고갈
	{
		UPCAbilityData* Heavy = MakeAttackData(20.f);
		Heavy->GuardStaminaDamage = 500.f;
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		Victim->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
		TestEqual(TEXT("기력이 바닥나도 그 공격은 막는다"), Victim->ReceiveHit(MakeHit(Attacker, Heavy)), EPCHitOutcome::Blocked);
		TestTrue(TEXT("기력이 바닥나면 경직된다"), Victim->HasStateTag(PCTags::State_Staggered));
		Victim->Destroy();
	}

	// 예비 동작 중에만 경직시키는 공격 (방패 밀기)
	{
		UPCAbilityData* Bash = MakeAttackData(3.f);
		Bash->StaggerTime = 1.f;
		Bash->bStaggerOnlyOnWindup = true;

		APCTestCharacter* Idle = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		Idle->ReceiveHit(MakeHit(Attacker, Bash));
		TestFalse(TEXT("예비 동작 중이 아니면 경직되지 않는다"), Idle->HasStateTag(PCTags::State_Staggered));
		Idle->Destroy();

		APCTestCharacter* WindingUp = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		WindingUp->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Action_Windup);
		WindingUp->ReceiveHit(MakeHit(Attacker, Bash));
		TestTrue(TEXT("예비 동작 중이면 경직된다"), WindingUp->HasStateTag(PCTags::State_Staggered));
		WindingUp->Destroy();
	}

	// 사망
	{
		UPCAbilityData* Lethal = MakeAttackData(500.f);
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		Victim->ReceiveHit(MakeHit(Attacker, Lethal));
		TestTrue(TEXT("체력이 0이 되면 사망 상태가 된다"), Victim->IsDead());
		TestEqual(TEXT("죽은 대상은 더 맞지 않는다"), Victim->ReceiveHit(MakeHit(Attacker, Attack)), EPCHitOutcome::Miss);
		Victim->Destroy();
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCCombatFirstHitTest, "PC.Combat.Rules.FirstHit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCCombatFirstHitTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	UPCAbilityData* Attack = MakeAttackData(20.f);
	Attack->HitShape.Range = 400.f;

	APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 0, 200), 0.f);
	APCTestCharacter* Far = TestWorld.Spawn(FVector(300, 0, 200), 180.f);
	TestWorld.Tick(0.1f);

	AActor* Victim = nullptr;
	TestEqual(TEXT("범위 안에 대상이 하나면 그 대상이 맞는다"), FPCHitResolver::ResolveAttack(MakeHit(Attacker, Attack), Victim), EPCHitOutcome::Hit);
	TestTrue(TEXT("맞은 대상은 뒤쪽 캐릭터다"), Victim == Far);
	TestEqual(TEXT("뒤쪽 캐릭터 체력이 줄었다"), Far->GetHealth(), 80.f);

	// 누군가 사이에 끼어든다
	APCTestCharacter* Near = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
	TestWorld.Tick(0.1f);

	Victim = nullptr;
	FPCHitResolver::ResolveAttack(MakeHit(Attacker, Attack), Victim);
	TestTrue(TEXT("사이에 끼어든 캐릭터가 대신 맞는다"), Victim == Near);
	TestEqual(TEXT("끼어든 캐릭터 체력이 줄었다"), Near->GetHealth(), 80.f);
	TestEqual(TEXT("뒤쪽 캐릭터는 더 맞지 않았다"), Far->GetHealth(), 80.f);

	// 끼어든 캐릭터가 방패를 들면 막힌다
	Near->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
	Victim = nullptr;
	TestEqual(TEXT("끼어든 캐릭터가 방패로 막는다"), FPCHitResolver::ResolveAttack(MakeHit(Attacker, Attack), Victim), EPCHitOutcome::Blocked);
	TestEqual(TEXT("막았으므로 뒤쪽 캐릭터는 무사하다"), Far->GetHealth(), 80.f);

	// 범위 밖
	Attack->HitShape.Range = 50.f;
	Victim = nullptr;
	TestEqual(TEXT("범위 밖이면 빗나간다"), FPCHitResolver::ResolveAttack(MakeHit(Attacker, Attack), Victim), EPCHitOutcome::Miss);
	TestNull(TEXT("빗나가면 맞은 대상이 없다"), Victim);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCCombatAbilityFlowTest, "PC.Combat.Rules.AbilityFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCCombatAbilityFlowTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	UPCAbilityData* Attack = MakeAttackData(20.f);

	APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 0, 200), 0.f, Attack);
	APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
	UPCAbilitySystemComponent* ASC = Attacker->GetPCAbilitySystemComponent();
	TestWorld.Tick(0.1f);

	TestTrue(TEXT("기술이 발동한다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
	TestEqual(TEXT("발동하면 기력이 소모된다"), Attacker->GetStamina(), 90.f);
	TestTrue(TEXT("발동 직후는 예비 동작이다"), Attacker->HasStateTag(PCTags::State_Action_Windup));
	TestEqual(TEXT("예비 동작 중에는 아직 맞지 않는다"), Victim->GetHealth(), 100.f);
	TestFalse(TEXT("행동 중에는 다시 발동할 수 없다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));

	TestWorld.Tick(0.5f);

	TestEqual(TEXT("판정 구간에 대상이 맞는다"), Victim->GetHealth(), 80.f);
	TestFalse(TEXT("후딜이 끝나면 행동 상태가 풀린다"), Attacker->HasStateTag(PCTags::State_Action));
	TestTrue(TEXT("끝난 뒤에는 다시 발동할 수 있다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));

	// 예비 동작 중에 경직되면 공격이 끊긴다
	Attacker->ApplyStagger(1.f);
	TestFalse(TEXT("경직되면 행동 상태가 풀린다"), Attacker->HasStateTag(PCTags::State_Action));
	TestWorld.Tick(0.5f);
	TestEqual(TEXT("끊긴 공격은 피해를 주지 않는다"), Victim->GetHealth(), 80.f);
	TestFalse(TEXT("경직 중에는 발동할 수 없다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));

	// 기력 부족
	TestWorld.Tick(1.f);
	UPCAbilityData* Expensive = MakeAttackData(20.f);
	Expensive->StaminaCost = 500.f;
	APCTestCharacter* Tired = TestWorld.Spawn(FVector(0, 1000, 200), 0.f, Expensive);
	TestFalse(TEXT("기력이 부족하면 발동하지 않는다"), Tired->GetPCAbilitySystemComponent()->TryActivateByAbilityTag(PCTags::Ability_Attack));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCCombatStateEffectsTest, "PC.Combat.Rules.StateEffects",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCCombatStateEffectsTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	UPCAbilityData* Attack = MakeAttackData(20.f);
	const FGameplayTagContainer StaggerTags(PCTags::State_Staggered);
	const FGameplayTagContainer DeadTags(PCTags::State_Dead);

	// 경직: 이펙트가 태그를 붙이고 시간이 지나면 스스로 뗀다
	{
		APCTestCharacter* Character = TestWorld.Spawn(FVector(0, 0, 200), 0.f, Attack);
		UPCAbilitySystemComponent* ASC = Character->GetPCAbilitySystemComponent();
		TestWorld.Tick(0.1f);

		Character->ApplyStagger(0.5f);
		TestTrue(TEXT("경직 태그가 붙는다"), Character->HasStateTag(PCTags::State_Staggered));
		TestEqual(TEXT("경직은 활성 이펙트로 걸려 있다"), ASC->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(StaggerTags)).Num(), 1);
		TestFalse(TEXT("경직 중에는 발동할 수 없다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));

		TestWorld.Tick(0.7f);
		TestFalse(TEXT("시간이 지나면 경직이 풀린다"), Character->HasStateTag(PCTags::State_Staggered));
		TestTrue(TEXT("경직이 풀리면 다시 발동할 수 있다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
		Character->Destroy();
	}

	// 기력 회복: 쓴 직후에는 멈췄다가, 지연 시간이 지나면 주기적으로 오른다
	{
		APCTestCharacter* Character = TestWorld.Spawn(FVector(0, 500, 200), 0.f, Attack);
		UPCAbilitySystemComponent* ASC = Character->GetPCAbilitySystemComponent();
		TestWorld.Tick(0.1f);

		TestTrue(TEXT("기술이 발동한다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
		TestEqual(TEXT("기력이 소모된다"), Character->GetStamina(), 90.f);
		TestTrue(TEXT("기력을 쓰면 회복 지연 태그가 붙는다"), Character->HasStateTag(PCTags::State_StaminaRegenPaused));

		TestWorld.Tick(0.6f);
		TestEqual(TEXT("지연 시간 동안은 회복되지 않는다"), Character->GetStamina(), 90.f);

		TestWorld.Tick(1.5f);
		TestFalse(TEXT("지연 시간이 지나면 지연 태그가 떨어진다"), Character->HasStateTag(PCTags::State_StaminaRegenPaused));
		TestTrue(TEXT("지연 시간이 지나면 기력이 회복된다"), Character->GetStamina() > 90.f);

		TestWorld.Tick(3.f);
		TestEqual(TEXT("기력은 최대값을 넘지 않는다"), Character->GetStamina(), Character->GetMaxStamina());
		Character->Destroy();
	}

	// 방어 중에는 회복이 멈춘다
	{
		APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 1000, 200), 0.f);
		APCTestCharacter* Defender = TestWorld.Spawn(FVector(150, 1000, 200), 180.f);
		TestWorld.Tick(0.1f);

		Defender->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
		Defender->ReceiveHit(MakeHit(Attacker, Attack));
		TestEqual(TEXT("막으면 기력이 줄어든다"), Defender->GetStamina(), 70.f);

		TestWorld.Tick(3.f);
		TestEqual(TEXT("방패를 든 동안에는 회복되지 않는다"), Defender->GetStamina(), 70.f);

		Defender->GetPCAbilitySystemComponent()->RemoveLooseGameplayTag(PCTags::State_Guarding);
		TestWorld.Tick(1.f);
		TestTrue(TEXT("방패를 내리면 회복된다"), Defender->GetStamina() > 70.f);
		Attacker->Destroy();
		Defender->Destroy();
	}

	// 사망과 부활
	{
		UPCAbilityData* Lethal = MakeAttackData(500.f);
		APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 1500, 200), 0.f);
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 1500, 200), 180.f, Attack);
		UPCAbilitySystemComponent* ASC = Victim->GetPCAbilitySystemComponent();
		TestWorld.Tick(0.1f);

		Victim->ApplyStagger(10.f);
		Victim->ReceiveHit(MakeHit(Attacker, Lethal));
		TestTrue(TEXT("체력이 0이 되면 사망 태그가 붙는다"), Victim->IsDead());
		TestEqual(TEXT("사망은 활성 이펙트로 걸려 있다"), ASC->GetActiveEffects(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(DeadTags)).Num(), 1);
		TestFalse(TEXT("죽으면 경직 이펙트는 제거된다"), Victim->HasStateTag(PCTags::State_Staggered));
		TestFalse(TEXT("죽은 동안에는 발동할 수 없다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));

		TestWorld.Tick(3.5f);
		TestFalse(TEXT("부활 시간이 지나면 사망 태그가 떨어진다"), Victim->IsDead());
		TestEqual(TEXT("부활하면 체력이 가득 찬다"), Victim->GetHealth(), Victim->GetMaxHealth());
		TestTrue(TEXT("부활하면 다시 발동할 수 있다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
