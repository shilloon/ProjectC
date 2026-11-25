#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Testing/PCTestWorld.h"

using namespace PCTest;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCFeelKnockbackTest, "PC.Combat.Feel.Knockback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCFeelKnockbackTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	TestWorld.SpawnFloor();

	UPCAbilityData* Push = MakeAttackData(10.f);
	AddReaction(Push, EPCHitReactionWhen::Hit, 0.f, 120.f);
	AddReaction(Push, EPCHitReactionWhen::Blocked, 0.f, 40.f);

	APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 0, 100), 0.f);

	// 맞으면 공격자 반대쪽으로 밀려난다
	{
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 100), 180.f);
		TestWorld.Tick(0.5f);
		const float StartX = Victim->GetActorLocation().X;

		Victim->ReceiveHit(MakeHit(Attacker, Push));
		TestWorld.Tick(0.5f);
		const float Moved = Victim->GetActorLocation().X - StartX;
		TestTrue(FString::Printf(TEXT("맞으면 밀려난다 (120 근처여야 함, 실제 %.0f)"), Moved), FMath::IsNearlyEqual(Moved, 120.f, 30.f));
		TestTrue(TEXT("옆으로는 밀리지 않는다"), FMath::Abs(Victim->GetActorLocation().Y) < 5.f);
		Victim->Destroy();
	}

	// 막으면 덜 밀려난다
	{
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 100), 180.f);
		TestWorld.Tick(0.5f);
		Victim->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
		const float StartX = Victim->GetActorLocation().X;

		TestEqual(TEXT("정면 공격을 막는다"), Victim->ReceiveHit(MakeHit(Attacker, Push)), EPCHitOutcome::Blocked);
		TestWorld.Tick(0.5f);
		const float Moved = Victim->GetActorLocation().X - StartX;
		TestTrue(FString::Printf(TEXT("막으면 덜 밀려난다 (40 근처여야 함, 실제 %.0f)"), Moved), FMath::IsNearlyEqual(Moved, 40.f, 20.f));
		Victim->Destroy();
	}

	// 경직 저항이 최대인 몬스터(보스)는 밀리지 않는다
	{
		UPCMonsterData* Boss = NewObject<UPCMonsterData>(GetTransientPackage());
		SetRank(*Boss, PCTags::Monster_Rank_Boss, 10.f, 1.5f, 0.f);
		APCTestMonster* Monster = TestWorld.SpawnMonster(FVector(150, 0, 100), 180.f, Boss);
		TestWorld.Tick(0.5f);
		const float StartX = Monster->GetActorLocation().X;

		Monster->ReceiveHit(MakeHit(Attacker, Push));
		TestWorld.Tick(0.5f);
		TestTrue(TEXT("보스는 밀리지 않는다"), FMath::Abs(Monster->GetActorLocation().X - StartX) < 5.f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCFeelMoveSpeedTest, "PC.Combat.Feel.MoveSpeed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCFeelMoveSpeedTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;

	UPCAbilityData* Slow = MakeAttackData(10.f);
	Slow->MoveSpeedScale = 0.3f;
	Slow->Timing.Windup = 0.3f;

	APCTestCharacter* Character = TestWorld.Spawn(FVector(0, 0, 200), 0.f, Slow);
	UPCAbilitySystemComponent* ASC = Character->GetPCAbilitySystemComponent();
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	TestWorld.Tick(0.1f);

	const float BaseSpeed = Movement->MaxWalkSpeed;
	TestEqual(TEXT("평소 이동 속도는 기본값이다"), BaseSpeed, 500.f);

	// 기술을 쓰는 동안 느려지고, 끝나면 돌아온다
	TestTrue(TEXT("기술이 발동한다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
	TestEqual(TEXT("기술을 쓰는 동안 이동 속도가 줄어든다"), Movement->MaxWalkSpeed, 150.f);
	TestWorld.Tick(1.f);
	TestEqual(TEXT("기술이 끝나면 이동 속도가 돌아온다"), Movement->MaxWalkSpeed, BaseSpeed);

	// 경직으로 끊겨도 돌아온다
	TestWorld.Tick(1.5f);
	TestTrue(TEXT("다시 발동한다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
	TestEqual(TEXT("다시 느려진다"), Movement->MaxWalkSpeed, 150.f);
	Character->ApplyStagger(0.2f);
	TestEqual(TEXT("기술이 끊기면 이동 속도가 바로 돌아온다"), Movement->MaxWalkSpeed, BaseSpeed);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
