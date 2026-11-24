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
	TestWorld.Tick(0.5f);
	TestNull(TEXT("대상이 죽으면 놓는다"), Controller->GetTarget());
	TestWorld.Tick(4.f);
	TestTrue(TEXT("대상이 되살아나면 다시 찾는다"), Controller->GetTarget() == Target);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
