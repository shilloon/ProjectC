#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI/PCMonsterAttackSelector.h"
#include "AbilitySystem/Abilities/PCAbility_LungeAttack.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Data/PCMonsterRankRow.h"
#include "Engine/DataTable.h"
#include "Hit/PCHitResolver.h"
#include "Testing/PCTestWorld.h"

using namespace PCTest;

namespace
{
	FPCMonsterAttackOption MakeOption(FGameplayTag Tag, float MinRange, float MaxRange, float Cooldown = 0.f)
	{
		FPCMonsterAttackOption Option;
		Option.AbilityTag = Tag;
		Option.MinRange = MinRange;
		Option.MaxRange = MaxRange;
		Option.Cooldown = Cooldown;
		return Option;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCMonsterAttackSelectorTest, "PC.Combat.Monster.AttackSelector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCMonsterAttackSelectorTest::RunTest(const FString& Parameters)
{
	// 서로 다른 태그 두 개가 필요할 뿐이라 있는 태그를 빌려 쓴다
	const FGameplayTag Claw = PCTags::Ability_Attack;
	const FGameplayTag Lunge = PCTags::Ability_Dodge;

	TArray<FPCMonsterAttackOption> Options;
	Options.Add(MakeOption(Claw, 0.f, 200.f));
	Options.Add(MakeOption(Lunge, 300.f, 600.f, 6.f));

	TMap<FGameplayTag, double> LastUsed;
	FRandomStream Random(1);

	TestEqual(TEXT("가까우면 할퀴기"), FPCMonsterAttackSelector::Select(Options, 100.f, 1.f, 0.0, LastUsed, Random), 0);
	TestEqual(TEXT("중거리면 돌진"), FPCMonsterAttackSelector::Select(Options, 400.f, 1.f, 0.0, LastUsed, Random), 1);
	TestEqual(TEXT("어느 범위에도 안 맞으면 없음"), FPCMonsterAttackSelector::Select(Options, 250.f, 1.f, 0.0, LastUsed, Random), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("멀리 있으면 더 긴 사거리까지만 다가간다"), FPCMonsterAttackSelector::GetApproachRange(Options, 700.f, 1.f, 0.0, LastUsed), 600.f);
	TestEqual(TEXT("두 사거리 사이의 빈 구간에서는 가까운 쪽 공격 거리까지 다가간다"), FPCMonsterAttackSelector::GetApproachRange(Options, 250.f, 1.f, 0.0, LastUsed), 200.f);
	TestEqual(TEXT("이미 가장 가까운 사거리 안이면 더 다가가지 않는다"), FPCMonsterAttackSelector::GetApproachRange(Options, 100.f, 1.f, 0.0, LastUsed), 100.f);

	// 돌진을 10초에 썼다
	LastUsed.Add(Lunge, 10.0);
	TestEqual(TEXT("재사용 대기 중이면 뽑히지 않는다"), FPCMonsterAttackSelector::Select(Options, 400.f, 1.f, 12.0, LastUsed, Random), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("돌진이 대기 중이면 할퀴기 거리까지 다가간다"), FPCMonsterAttackSelector::GetApproachRange(Options, 700.f, 1.f, 12.0, LastUsed), 200.f);
	TestEqual(TEXT("재사용 시간이 지나면 다시 뽑힌다"), FPCMonsterAttackSelector::Select(Options, 400.f, 1.f, 17.0, LastUsed, Random), 1);

	// 체력 구간
	Options[0].MinHealthRatio = 0.5f;
	TestEqual(TEXT("체력 구간 밖이면 뽑히지 않는다"), FPCMonsterAttackSelector::Select(Options, 100.f, 0.3f, 0.0, LastUsed, Random), static_cast<int32>(INDEX_NONE));
	TestEqual(TEXT("체력 구간 안이면 뽑힌다"), FPCMonsterAttackSelector::Select(Options, 100.f, 0.8f, 0.0, LastUsed, Random), 0);

	// 비중
	Options[0].MinHealthRatio = 0.f;
	Options[0].Weight = 0.f;
	TestEqual(TEXT("비중이 0이면 뽑히지 않는다"), FPCMonsterAttackSelector::Select(Options, 100.f, 1.f, 0.0, LastUsed, Random), static_cast<int32>(INDEX_NONE));

	// 같은 조건의 공격 둘은 비중에 따라 둘 다 나온다
	TArray<FPCMonsterAttackOption> Mixed;
	Mixed.Add(MakeOption(Claw, 0.f, 200.f));
	Mixed.Add(MakeOption(Lunge, 0.f, 200.f));
	TMap<FGameplayTag, double> NoHistory;
	int32 Counts[2] = { 0, 0 };
	for (int32 Index = 0; Index < 200; ++Index)
	{
		const int32 Picked = FPCMonsterAttackSelector::Select(Mixed, 100.f, 1.f, 0.0, NoHistory, Random);
		if (Picked == 0 || Picked == 1)
		{
			++Counts[Picked];
		}
	}
	TestTrue(TEXT("비중이 같으면 두 공격이 모두 뽑힌다"), Counts[0] > 50 && Counts[1] > 50);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCMonsterHitShapeTest, "PC.Combat.Monster.HitShapes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCMonsterHitShapeTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;

	// 범위 안 전원: 앞뒤로 선 둘이 모두 맞는다
	{
		UPCAbilityData* Sweep = MakeAttackData(20.f);
		Sweep->HitShape.Range = 400.f;
		Sweep->HitShape.TargetRule = EPCHitTargetRule::AllInArea;

		APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 0, 200), 0.f, Sweep);
		APCTestCharacter* Near = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		APCTestCharacter* Far = TestWorld.Spawn(FVector(300, 0, 200), 180.f);
		TestWorld.Tick(0.1f);

		Attacker->GetPCAbilitySystemComponent()->TryActivateByAbilityTag(PCTags::Ability_Attack);
		TestWorld.Tick(0.5f);

		TestEqual(TEXT("휩쓸기: 앞의 대상이 맞는다"), Near->GetHealth(), 80.f);
		TestEqual(TEXT("휩쓸기: 뒤의 대상도 맞는다"), Far->GetHealth(), 80.f);

		Attacker->Destroy();
		Near->Destroy();
		Far->Destroy();
	}

	// 처음 닿은 하나: 같은 배치에서 앞의 대상만 맞는다
	{
		UPCAbilityData* Slash = MakeAttackData(20.f);
		Slash->HitShape.Range = 400.f;

		APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 1000, 200), 0.f, Slash);
		APCTestCharacter* Near = TestWorld.Spawn(FVector(150, 1000, 200), 180.f);
		APCTestCharacter* Far = TestWorld.Spawn(FVector(300, 1000, 200), 180.f);
		TestWorld.Tick(0.1f);

		Attacker->GetPCAbilitySystemComponent()->TryActivateByAbilityTag(PCTags::Ability_Attack);
		TestWorld.Tick(0.5f);

		TestEqual(TEXT("베기: 앞의 대상만 맞는다"), Near->GetHealth(), 80.f);
		TestEqual(TEXT("베기: 뒤의 대상은 가려진다"), Far->GetHealth(), 100.f);

		Attacker->Destroy();
		Near->Destroy();
		Far->Destroy();
	}

	// 원형: 등 뒤의 대상도 범위에 든다
	{
		UPCAbilityData* Slam = MakeAttackData(20.f);
		APCTestCharacter* Attacker = TestWorld.Spawn(FVector(0, 2000, 200), 0.f);
		APCTestCharacter* Behind = TestWorld.Spawn(FVector(-150, 2000, 200), 0.f);
		TestWorld.Tick(0.1f);

		TestNull(TEXT("부채꼴은 등 뒤를 치지 못한다"), FPCHitResolver::FindFirstHittable(MakeHit(Attacker, Slam)));

		Slam->HitShape.ShapeType = EPCHitShapeType::Circle;
		TestTrue(TEXT("원형은 등 뒤도 친다"), FPCHitResolver::FindFirstHittable(MakeHit(Attacker, Slam)) == Behind);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCMonsterRankTest, "PC.Combat.Monster.Rank",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCMonsterRankTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	UPCAbilityData* Claw = MakeAttackData(20.f);

	// 엘리트: 체력 2.5배, 피해 1.3배, 경직 시간 절반
	{
		UPCMonsterData* Elite = NewObject<UPCMonsterData>(GetTransientPackage());
		Elite->Abilities.Add(Claw);
		SetRank(*Elite, PCTags::Monster_Rank_Elite, 2.5f, 1.3f, 0.5f);

		APCTestMonster* Monster = TestWorld.SpawnMonster(FVector(0, 0, 200), 0.f, Elite);
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(150, 0, 200), 180.f);
		TestWorld.Tick(0.1f);

		TestEqual(TEXT("엘리트: 최대 체력에 배율이 적용된다"), Monster->GetMaxHealth(), 250.f);
		TestEqual(TEXT("엘리트: 체력이 가득 찬 채로 시작한다"), Monster->GetHealth(), 250.f);
		TestTrue(TEXT("엘리트: 등급 태그가 붙는다"), Monster->HasStateTag(PCTags::Monster_Rank_Elite));
		TestTrue(TEXT("엘리트: 상위 태그로도 조회된다"), Monster->HasStateTag(PCTags::Monster_Rank));

		TestTrue(TEXT("정의의 기술이 부여되어 발동한다"), Monster->GetPCAbilitySystemComponent()->TryActivateByAbilityTag(PCTags::Ability_Attack));
		TestWorld.Tick(0.5f);
		TestEqual(TEXT("엘리트: 피해에 배율이 적용된다"), Victim->GetHealth(), 74.f);

		Monster->ApplyStagger(1.f);
		TestTrue(TEXT("엘리트: 경직은 걸린다"), Monster->HasStateTag(PCTags::State_Staggered));
		TestWorld.Tick(0.7f);
		TestFalse(TEXT("엘리트: 경직이 절반 시간에 풀린다"), Monster->HasStateTag(PCTags::State_Staggered));

		Monster->Destroy();
		Victim->Destroy();
	}

	// 보스: 경직되지 않는다
	{
		UPCMonsterData* Boss = NewObject<UPCMonsterData>(GetTransientPackage());
		SetRank(*Boss, PCTags::Monster_Rank_Boss, 10.f, 1.5f, 0.f);

		APCTestMonster* Monster = TestWorld.SpawnMonster(FVector(0, 1000, 200), 0.f, Boss);
		TestWorld.Tick(0.1f);

		TestEqual(TEXT("보스: 최대 체력에 배율이 적용된다"), Monster->GetMaxHealth(), 1000.f);
		Monster->ApplyStagger(5.f);
		TestFalse(TEXT("보스: 경직되지 않는다"), Monster->HasStateTag(PCTags::State_Staggered));
		Monster->Destroy();
	}

	// 등급이 없으면 기본 수치 그대로
	{
		UPCMonsterData* Plain = NewObject<UPCMonsterData>(GetTransientPackage());
		Plain->Scale = 1.5f;
		APCTestMonster* Monster = TestWorld.SpawnMonster(FVector(0, 2000, 200), 0.f, Plain);
		TestWorld.Tick(0.1f);

		TestEqual(TEXT("등급 없음: 기본 체력"), Monster->GetMaxHealth(), 100.f);
		TestEqual(TEXT("등급 없음: 기본 공격력 배율"), Monster->GetAttackPower(), 1.f);
		TestEqual(TEXT("정의의 크기가 적용된다"), static_cast<float>(Monster->GetActorScale3D().X), 1.5f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCMonsterLungeTest, "PC.Combat.Monster.Lunge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCMonsterLungeTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	TestWorld.SpawnFloor();

	UPCAbilityData* Lunge = MakeAttackData(30.f);
	Lunge->AbilityClass = UPCAbility_LungeAttack::StaticClass();
	Lunge->Timing.Windup = 0.2f;
	Lunge->Timing.Active = 0.25f;
	Lunge->HitShape.Range = 120.f;
	Lunge->LungeDistance = 400.f;

	UPCMonsterData* MonsterData = NewObject<UPCMonsterData>(GetTransientPackage());
	MonsterData->Abilities.Add(Lunge);

	// 제자리에서는 닿지 않는 거리
	APCTestMonster* Monster = TestWorld.SpawnMonster(FVector(0, 0, 100), 0.f, MonsterData);
	APCTestCharacter* Victim = TestWorld.Spawn(FVector(450, 0, 100), 180.f);
	TestWorld.Tick(0.5f);

	const float StartX = Monster->GetActorLocation().X;
	TestNull(TEXT("돌진 전에는 범위 밖이다"), FPCHitResolver::FindFirstHittable(MakeHit(Monster, Lunge)));

	TestTrue(TEXT("돌진이 발동한다"), Monster->GetPCAbilitySystemComponent()->TryActivateByAbilityTag(PCTags::Ability_Attack));
	TestWorld.Tick(0.15f);
	TestTrue(TEXT("예비 동작 중에는 움직이지 않는다"), FMath::IsNearlyEqual(Monster->GetActorLocation().X, StartX, 5.f));

	TestWorld.Tick(1.f);
	TestTrue(TEXT("판정 구간 동안 앞으로 이동한다"), Monster->GetActorLocation().X - StartX > 150.f);
	TestEqual(TEXT("이동하면서 대상을 친다"), Victim->GetHealth(), 70.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
