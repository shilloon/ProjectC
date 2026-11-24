#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/PCMonsterAIController.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "PCStateTreeBuilderLibrary.h"
#include "StateTree.h"
#include "Testing/PCTestWorld.h"

using namespace PCTest;

/**
 * 코드로 만든 기본 행동 트리로 몬스터가 실제로 대상을 찾고, 다가가고, 공격하는지 확인한다.
 * 트리 생성이 에디터 전용이라 이 테스트는 에디터 모듈에 있다.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCMonsterBehaviorTest, "PC.Combat.Monster.Behavior",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCMonsterBehaviorTest::RunTest(const FString& Parameters)
{
	UStateTree* Tree = UPCStateTreeBuilderLibrary::BuildMeleeMonsterStateTree(GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("기본 행동 트리가 만들어지고 컴파일된다"), Tree))
	{
		return false;
	}
	TestTrue(TEXT("행동 트리가 실행 가능한 상태다"), Tree->IsReadyToRun());

	FPCTestWorld TestWorld;
	TestWorld.SpawnFloor();

	// 테스트가 끝날 때까지 대상이 살아 있도록 피해를 낮게 둔다
	UPCAbilityData* Claw = MakeAttackData(4.f);
	Claw->StaminaCost = 0.f;
	Claw->Timing.Windup = 0.3f;

	FPCMonsterAttackOption ClawOption;
	ClawOption.AbilityTag = PCTags::Ability_Attack;
	ClawOption.MinRange = 0.f;
	ClawOption.MaxRange = 150.f;
	ClawOption.RestAfter = 0.5f;

	UPCMonsterData* MonsterData = NewObject<UPCMonsterData>(GetTransientPackage());
	MonsterData->Abilities.Add(Claw);
	MonsterData->AttackOptions.Add(ClawOption);
	MonsterData->BehaviorTree = Tree;
	MonsterData->DetectionRange = 1500.f;

	// 대상은 발견 범위 안이지만 공격 거리보다 훨씬 멀리 있다
	APCTestCharacter* Target = TestWorld.Spawn(FVector(800, 0, 100), 180.f);
	APCTestMonster* Monster = TestWorld.SpawnMonster(FVector(0, 0, 100), 0.f, MonsterData);
	APCMonsterAIController* Controller = Cast<APCMonsterAIController>(Monster->GetController());
	if (!TestNotNull(TEXT("몬스터에 AI 컨트롤러가 붙는다"), Controller))
	{
		return false;
	}

	const float StartDistance = FVector::Dist2D(Monster->GetActorLocation(), Target->GetActorLocation());

	TestWorld.Tick(1.f);
	TestTrue(TEXT("발견 범위 안의 대상을 찾는다"), Controller->GetTarget() == Target);

	TestWorld.Tick(5.f);
	const float EndDistance = FVector::Dist2D(Monster->GetActorLocation(), Target->GetActorLocation());
	TestTrue(TEXT("대상에게 다가간다"), EndDistance < StartDistance - 300.f);
	TestTrue(TEXT("공격 거리에 들어가면 공격해서 맞힌다"), Target->GetHealth() < 100.f);

	// 경직되면 공격이 끊기고, 풀리면 다시 공격한다
	const float HealthBeforeStagger = Target->GetHealth();
	Monster->ApplyStagger(1.5f);
	TestWorld.Tick(1.f);
	TestEqual(TEXT("경직 중에는 공격하지 않는다"), Target->GetHealth(), HealthBeforeStagger);
	TestWorld.Tick(3.f);
	TestTrue(TEXT("경직이 풀리면 다시 공격한다"), Target->GetHealth() < HealthBeforeStagger);
	TestFalse(TEXT("테스트 동안 대상이 죽지 않았다"), Target->IsDead());

	// 대상이 죽으면 놓고, 되살아나면 다시 찾는다
	UPCAbilityData* Lethal = MakeAttackData(500.f);
	Target->ReceiveHit(MakeHit(Monster, Lethal));
	// 하던 행동(공격이나 숨 고르기)을 마친 뒤 대상을 다시 확인하므로 조금 기다린다
	TestWorld.Tick(1.5f);
	TestNull(TEXT("대상이 죽으면 놓는다"), Controller->GetTarget());
	TestWorld.Tick(3.5f);
	TestTrue(TEXT("대상이 되살아나면 다시 찾는다"), Controller->GetTarget() == Target);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCMonsterAimAndCircleTest, "PC.Combat.Monster.AimAndCircle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCMonsterAimAndCircleTest::RunTest(const FString& Parameters)
{
	UStateTree* Tree = UPCStateTreeBuilderLibrary::BuildMeleeMonsterStateTree(GetTransientPackage(), NAME_None, RF_Transient);
	if (!TestNotNull(TEXT("기본 행동 트리가 만들어진다"), Tree))
	{
		return false;
	}

	FPCTestWorld TestWorld;
	TestWorld.SpawnFloor();

	UPCAbilityData* Claw = MakeAttackData(4.f);
	Claw->StaminaCost = 0.f;
	Claw->Timing.Windup = 0.2f;

	FPCMonsterAttackOption ClawOption;
	ClawOption.AbilityTag = PCTags::Ability_Attack;
	ClawOption.MaxRange = 150.f;
	ClawOption.RestAfter = 2.f;

	// 느리게 도는 몬스터: 90도/초, 조준 제한 시간은 넉넉하게
	UPCMonsterData* MonsterData = NewObject<UPCMonsterData>(GetTransientPackage());
	MonsterData->Abilities.Add(Claw);
	MonsterData->AttackOptions.Add(ClawOption);
	MonsterData->BehaviorTree = Tree;
	MonsterData->TurnRate = 90.f;
	MonsterData->AimToleranceDeg = 10.f;
	MonsterData->MaxAimTime = 10.f;
	MonsterData->RestMoveSpeedScale = 0.5f;
	MonsterData->RestDistanceMin = 100.f;
	MonsterData->RestDistanceMax = 300.f;

	// 대상은 공격 거리 안이지만 몬스터의 등 뒤에 있다
	APCTestCharacter* Target = TestWorld.Spawn(FVector(-150, 0, 100), 0.f);
	APCTestMonster* Monster = TestWorld.SpawnMonster(FVector(0, 0, 100), 0.f, MonsterData);
	APCMonsterAIController* Controller = Cast<APCMonsterAIController>(Monster->GetController());
	if (!TestNotNull(TEXT("몬스터에 AI 컨트롤러가 붙는다"), Controller))
	{
		return false;
	}

	// 180도를 90도/초로 돌아야 하므로 1초 뒤에는 아직 조준 중이다
	TestWorld.Tick(1.f);
	TestEqual(TEXT("등 뒤의 대상을 바로 공격하지 못한다"), Target->GetHealth(), 100.f);
	TestFalse(TEXT("조준 중에는 아직 공격을 시작하지 않았다"), Monster->HasStateTag(PCTags::State_Action));
	TestTrue(TEXT("조준하는 동안 대상 쪽으로 돌고 있다"), Controller->GetAngleToTargetDeg() < 170.f);

	TestWorld.Tick(2.f);
	TestTrue(TEXT("몸을 다 돌린 뒤에 공격한다"), Target->GetHealth() < 100.f);

	// 공격 뒤 숨 고르는 동안 대상 주위를 돈다
	const FVector RestStart = Monster->GetActorLocation();
	float MaxSideways = 0.f;
	for (int32 Step = 0; Step < 8; ++Step)
	{
		TestWorld.Tick(0.2f);
		MaxSideways = FMath::Max(MaxSideways, FMath::Abs(static_cast<float>(Monster->GetActorLocation().Y - RestStart.Y)));
	}
	TestTrue(FString::Printf(TEXT("숨 고르는 동안 옆으로 움직인다 (실제 %.0fcm)"), MaxSideways), MaxSideways > 20.f);
	TestTrue(TEXT("숨 고르는 동안에도 대상을 바라본다"), Controller->GetAngleToTargetDeg() < 45.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
