#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Effects/PCEffect_Slow.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Testing/PCTestWorld.h"

using namespace PCTest;

namespace
{
	FGameplayTag ContentTag(const TCHAR* Name)
	{
		return FGameplayTag::RequestGameplayTag(FName(Name));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCWeaponSwapTest, "PC.Combat.Weapon.Swap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCWeaponSwapTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	TestWorld.SpawnFloor();

	const FGameplayTag SlashTag = ContentTag(TEXT("Ability.Attack.Slash"));
	const FGameplayTag CleaveTag = ContentTag(TEXT("Ability.Attack.Cleave"));

	UPCAbilityData* Slash = MakeAttackData(10.f);
	Slash->AbilityTag = SlashTag;
	UPCAbilityData* Cleave = MakeAttackData(20.f);
	Cleave->AbilityTag = CleaveTag;

	// 넣기 0.3초인 무기에서 들기 0.4초인 무기로: 0.3초에 바뀌고 0.7초에 끝난다
	UPCWeaponData* Sword = MakeWeapon(Slash, 0.3f, 0.2f);
	UPCWeaponData* Greatsword = MakeWeapon(Cleave, 0.2f, 0.4f);

	APCTestCharacter* Character = TestWorld.SpawnArmed(FVector(0, 0, 100), 0.f, { Sword, Greatsword }, MakeSwapData());
	UPCEquipmentComponent* Equipment = Character->GetEquipment();
	UPCAbilitySystemComponent* ASC = Character->GetPCAbilitySystemComponent();

	// 시작 상태: 첫 번째 무기의 기술만 갖고 있다
	TestEqual(TEXT("첫 번째 무기를 들고 시작한다"), Equipment->GetCurrentIndex(), 0);
	TestTrue(TEXT("든 무기의 기술을 갖고 있다"), ASC->HasAbility(SlashTag));
	TestFalse(TEXT("들지 않은 무기의 기술은 없다"), ASC->HasAbility(CleaveTag));
	const FPCWeaponAbilitySlot* PrimarySlot = Equipment->FindSlot(PCTags::Input_Primary);
	TestTrue(TEXT("주 공격 슬롯이 든 무기의 기술을 가리킨다"), PrimarySlot && PrimarySlot->Ability == Slash);
	TestNull(TEXT("비어 있는 슬롯은 기술이 없다"), Equipment->FindSlot(PCTags::Input_Secondary));

	TestFalse(TEXT("이미 든 무기로는 교체하지 않는다"), Equipment->RequestSwap(0));
	TestFalse(TEXT("없는 번호로는 교체하지 않는다"), Equipment->RequestSwap(5));

	// 교체 시작: 넣는 구간
	TestTrue(TEXT("다른 무기로 교체를 시작한다"), Equipment->RequestSwap(1));
	TestTrue(TEXT("교체 중 상태가 된다"), Character->HasStateTag(PCTags::State_Swapping));
	TestFalse(TEXT("교체 중에는 공격할 수 없다"), ASC->TryActivateByAbilityTag(SlashTag));

	TestWorld.Tick(0.2f);
	TestEqual(TEXT("넣는 구간에서는 아직 원래 무기다"), Equipment->GetCurrentIndex(), 0);

	// 드는 구간
	TestWorld.Tick(0.2f);
	TestEqual(TEXT("넣기가 끝나면 새 무기로 바뀐다"), Equipment->GetCurrentIndex(), 1);
	TestTrue(TEXT("드는 구간도 교체 중이다"), Character->HasStateTag(PCTags::State_Swapping));
	TestTrue(TEXT("새 무기의 기술이 부여된다"), ASC->HasAbility(CleaveTag));
	TestFalse(TEXT("이전 무기의 기술은 회수된다"), ASC->HasAbility(SlashTag));
	TestFalse(TEXT("드는 중에는 새 기술도 아직 쓸 수 없다"), ASC->TryActivateByAbilityTag(CleaveTag));

	// 끝
	TestWorld.Tick(0.5f);
	TestFalse(TEXT("드는 시간이 지나면 교체가 끝난다"), Character->HasStateTag(PCTags::State_Swapping));
	PrimarySlot = Equipment->FindSlot(PCTags::Input_Primary);
	TestTrue(TEXT("주 공격 슬롯이 새 무기의 기술을 가리킨다"), PrimarySlot && PrimarySlot->Ability == Cleave);
	TestTrue(TEXT("교체가 끝나면 새 무기의 기술을 쓸 수 있다"), ASC->TryActivateByAbilityTag(CleaveTag));

	// 무기를 들고 있는 동안의 이동 속도
	{
		UPCWeaponData* Heavy = MakeWeapon(Cleave, 0.f, 0.f);
		Heavy->HeldMoveSpeedScale = 0.8f;
		UPCWeaponData* Light = MakeWeapon(Slash, 0.f, 0.f);

		APCTestCharacter* Carrier = TestWorld.SpawnArmed(FVector(500, 0, 100), 0.f, { Light, Heavy }, MakeSwapData());
		const float BaseSpeed = Carrier->GetCharacterMovement()->MaxWalkSpeed;
		Carrier->GetEquipment()->EquipIndex(1);
		TestEqual(TEXT("무거운 무기를 들면 느려진다"), Carrier->GetCharacterMovement()->MaxWalkSpeed, BaseSpeed * 0.8f, 1.f);
		Carrier->GetEquipment()->EquipIndex(0);
		TestEqual(TEXT("다른 무기로 바꾸면 속도가 돌아온다"), Carrier->GetCharacterMovement()->MaxWalkSpeed, BaseSpeed, 1.f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCWeaponSwapInterruptTest, "PC.Combat.Weapon.SwapInterrupt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCWeaponSwapInterruptTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	TestWorld.SpawnFloor();

	UPCAbilityData* Slash = MakeAttackData(10.f);
	Slash->AbilityTag = ContentTag(TEXT("Ability.Attack.Slash"));
	UPCAbilityData* Cleave = MakeAttackData(20.f);
	Cleave->AbilityTag = ContentTag(TEXT("Ability.Attack.Cleave"));
	UPCWeaponData* Sword = MakeWeapon(Slash, 0.3f, 0.3f);
	UPCWeaponData* Greatsword = MakeWeapon(Cleave, 0.3f, 0.3f);

	// 경직도 밀려남도 없는 약한 공격
	UPCAbilityData* Poke = MakeAttackData(5.f);
	APCTestCharacter* Attacker = TestWorld.Spawn(FVector(-500, 0, 100), 0.f);

	// 교체 중이 아니면 이 공격으로는 경직되지 않는다
	{
		APCTestCharacter* Victim = TestWorld.SpawnArmed(FVector(0, 0, 100), 180.f, { Sword, Greatsword }, MakeSwapData());
		Victim->ReceiveHit(MakeHit(Attacker, Poke));
		TestFalse(TEXT("교체 중이 아니면 약한 공격에 경직되지 않는다"), Victim->HasStateTag(PCTags::State_Staggered));
		Victim->Destroy();
	}

	// 넣는 구간에서 맞으면 교체가 취소되고 원래 무기가 남는다
	{
		APCTestCharacter* Victim = TestWorld.SpawnArmed(FVector(0, 0, 100), 180.f, { Sword, Greatsword }, MakeSwapData());
		Victim->GetEquipment()->RequestSwap(1);
		TestWorld.Tick(0.1f);
		Victim->ReceiveHit(MakeHit(Attacker, Poke));

		TestTrue(TEXT("교체 중에 맞으면 약한 공격에도 경직된다"), Victim->HasStateTag(PCTags::State_Staggered));
		TestFalse(TEXT("맞으면 교체가 끊긴다"), Victim->HasStateTag(PCTags::State_Swapping));
		TestEqual(TEXT("추가 피해 배율이 1이면 피해는 그대로다"), Victim->GetHealth(), 95.f);

		TestWorld.Tick(1.5f);
		TestFalse(TEXT("경직이 풀린다"), Victim->HasStateTag(PCTags::State_Staggered));
		TestEqual(TEXT("넣는 중에 끊기면 원래 무기 그대로다"), Victim->GetEquipment()->GetCurrentIndex(), 0);
		TestTrue(TEXT("원래 무기의 기술을 그대로 쓸 수 있다"), Victim->GetPCAbilitySystemComponent()->TryActivateByAbilityTag(Slash->AbilityTag));
		Victim->Destroy();
	}

	// 드는 구간에서 맞으면 경직이 풀린 뒤 새 무기를 든 상태가 된다
	{
		APCTestCharacter* Victim = TestWorld.SpawnArmed(FVector(0, 0, 100), 180.f, { Sword, Greatsword }, MakeSwapData());
		Victim->GetEquipment()->RequestSwap(1);
		TestWorld.Tick(0.4f);
		TestTrue(TEXT("드는 구간에 들어와 있다"), Victim->HasStateTag(PCTags::State_Swapping));
		Victim->ReceiveHit(MakeHit(Attacker, Poke));

		TestTrue(TEXT("드는 중에 맞아도 경직된다"), Victim->HasStateTag(PCTags::State_Staggered));
		TestFalse(TEXT("드는 중에 맞아도 교체가 끊긴다"), Victim->HasStateTag(PCTags::State_Swapping));

		TestWorld.Tick(1.5f);
		TestEqual(TEXT("드는 중에 끊기면 새 무기를 든 상태다"), Victim->GetEquipment()->GetCurrentIndex(), 1);
		TestTrue(TEXT("새 무기의 기술을 쓸 수 있다"), Victim->GetPCAbilitySystemComponent()->TryActivateByAbilityTag(Cleave->AbilityTag));
		Victim->Destroy();
	}

	// 추가 피해 배율을 올리면 교체 중에 더 아프다
	{
		APCTestCharacter* Victim = TestWorld.SpawnArmed(FVector(0, 0, 100), 180.f, { Sword, Greatsword }, MakeSwapData());
		Victim->GetEquipment()->SwapDamageTakenMultiplier = 2.f;
		Victim->GetEquipment()->RequestSwap(1);
		TestWorld.Tick(0.1f);
		Victim->ReceiveHit(MakeHit(Attacker, Poke));
		TestEqual(TEXT("교체 중 피해에 배율이 곱해진다"), Victim->GetHealth(), 90.f);
		Victim->Destroy();
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCHitReactionsTest, "PC.Combat.Rules.HitReactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCHitReactionsTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	TestWorld.SpawnFloor();

	APCTestCharacter* Attacker = TestWorld.Spawn(FVector(-300, 0, 100), 0.f);

	// 반응이 없는 기술은 피해만 준다
	{
		APCTestCharacter* Victim = TestWorld.Spawn(FVector(0, 0, 100), 180.f);
		Victim->ReceiveHit(MakeHit(Attacker, MakeAttackData(10.f)));
		TestEqual(TEXT("피해는 들어간다"), Victim->GetHealth(), 90.f);
		TestFalse(TEXT("반응이 없으면 경직되지 않는다"), Victim->HasStateTag(PCTags::State_Staggered));
		Victim->Destroy();
	}

	// 맞았을 때와 막혔을 때의 반응이 따로 적용된다
	{
		UPCAbilityData* Smash = MakeAttackData(10.f);
		AddReaction(Smash, EPCHitReactionWhen::Hit, 0.5f, 0.f);

		APCTestCharacter* Open = TestWorld.Spawn(FVector(0, 0, 100), 180.f);
		Open->ReceiveHit(MakeHit(Attacker, Smash));
		TestTrue(TEXT("맞으면 경직 반응이 적용된다"), Open->HasStateTag(PCTags::State_Staggered));
		Open->Destroy();

		APCTestCharacter* Guarding = TestWorld.Spawn(FVector(0, 0, 100), 180.f);
		Guarding->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
		Guarding->ReceiveHit(MakeHit(Attacker, Smash));
		TestFalse(TEXT("막으면 '맞았을 때' 반응은 적용되지 않는다"), Guarding->HasStateTag(PCTags::State_Staggered));
		Guarding->Destroy();
	}

	// 조건이 맞는 줄이 여럿이면 경직은 가장 긴 것 하나만 쓴다
	{
		UPCAbilityData* Combo = MakeAttackData(1.f);
		AddReaction(Combo, EPCHitReactionWhen::Any, 0.3f, 0.f);
		AddReaction(Combo, EPCHitReactionWhen::Hit, 1.0f, 0.f).RequiredTargetTags.AddTag(PCTags::State_Swapping);
		AddReaction(Combo, EPCHitReactionWhen::Hit, 5.0f, 0.f).IgnoredTargetTags.AddTag(PCTags::State_Swapping);

		APCTestCharacter* Victim = TestWorld.Spawn(FVector(0, 0, 100), 180.f);
		Victim->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Swapping);
		Victim->ReceiveHit(MakeHit(Attacker, Combo));
		TestWorld.Tick(0.6f);
		TestTrue(TEXT("0.3초 줄이 아니라 조건이 맞는 1초 줄이 쓰인다"), Victim->HasStateTag(PCTags::State_Staggered));
		TestWorld.Tick(0.7f);
		TestFalse(TEXT("제외 조건에 걸린 5초 줄은 쓰이지 않는다"), Victim->HasStateTag(PCTags::State_Staggered));
		Victim->Destroy();
	}

	// 지속 효과: 둔화
	{
		UPCAbilityData* LegShot = MakeAttackData(5.f);
		FPCHitReaction& Slow = AddReaction(LegShot, EPCHitReactionWhen::Hit, 0.f, 0.f);
		Slow.EffectClass = UPCEffect_Slow::StaticClass();
		Slow.EffectDuration = 1.f;
		Slow.EffectMagnitude = 0.5f;

		APCTestCharacter* Victim = TestWorld.Spawn(FVector(0, 0, 100), 180.f);
		const float BaseSpeed = Victim->GetCharacterMovement()->MaxWalkSpeed;

		Victim->ReceiveHit(MakeHit(Attacker, LegShot));
		TestEqual(TEXT("맞으면 이동이 느려진다"), Victim->GetCharacterMovement()->MaxWalkSpeed, BaseSpeed * 0.5f, 1.f);

		Victim->ReceiveHit(MakeHit(Attacker, LegShot));
		TestEqual(TEXT("또 맞아도 둔화가 겹쳐 쌓이지 않는다"), Victim->GetCharacterMovement()->MaxWalkSpeed, BaseSpeed * 0.5f, 1.f);

		TestWorld.Tick(1.3f);
		TestEqual(TEXT("시간이 지나면 속도가 돌아온다"), Victim->GetCharacterMovement()->MaxWalkSpeed, BaseSpeed, 1.f);
		Victim->Destroy();
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCProjectileTest, "PC.Combat.Rules.Projectile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCProjectileTest::RunTest(const FString& Parameters)
{
	FPCTestWorld TestWorld;
	TestWorld.SpawnFloor();

	UPCAbilityData* Shot = MakeRangedData(10.f);
	APCTestCharacter* Shooter = TestWorld.Spawn(FVector(0, 0, 100), 0.f, Shot);
	UPCAbilitySystemComponent* ASC = Shooter->GetPCAbilitySystemComponent();

	// 두 대상이 한 줄로 서 있다. 둘 다 사수를 바라본다.
	APCTestCharacter* Front = TestWorld.Spawn(FVector(800, 0, 100), 180.f);
	APCTestCharacter* Back = TestWorld.Spawn(FVector(1100, 0, 100), 180.f);
	TestWorld.Tick(0.3f);

	// 쏘는 순간에는 아직 아무도 맞지 않는다. 화살이 날아가야 한다.
	TestTrue(TEXT("원거리 공격을 시작한다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
	TestTrue(TEXT("겨누는 동안 조준 상태가 된다"), Shooter->HasStateTag(PCTags::State_Aiming));
	TestWorld.Tick(0.14f);
	TestEqual(TEXT("쏜 직후에는 아직 맞지 않았다"), Front->GetHealth(), 100.f);

	TestWorld.Tick(1.f);
	TestEqual(TEXT("화살은 경로에서 처음 닿은 대상을 맞힌다"), Front->GetHealth(), 90.f);
	TestEqual(TEXT("앞 대상이 뒤 대상을 가린다"), Back->GetHealth(), 100.f);
	TestFalse(TEXT("쏘고 나면 조준 상태가 풀린다"), Shooter->HasStateTag(PCTags::State_Aiming));

	// 방패를 든 대상은 정면에서 온 화살을 막는다
	Front->GetPCAbilitySystemComponent()->AddLooseGameplayTag(PCTags::State_Guarding);
	const float StaminaBefore = Front->GetStamina();
	TestTrue(TEXT("다시 쏜다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
	TestWorld.Tick(1.f);
	TestEqual(TEXT("방패로 화살을 막으면 피해가 없다"), Front->GetHealth(), 90.f);
	TestTrue(TEXT("막으면 기력이 깎인다"), Front->GetStamina() < StaminaBefore);
	TestEqual(TEXT("막힌 화살은 뒤로 넘어가지 않는다"), Back->GetHealth(), 100.f);
	Front->GetPCAbilitySystemComponent()->RemoveLooseGameplayTag(PCTags::State_Guarding);

	// 다른 곳을 겨누면 맞지 않는다
	Shooter->SetAimDirection(FVector(0, 1, 0));
	TestTrue(TEXT("옆을 겨누고 쏜다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
	TestWorld.Tick(2.f);
	TestEqual(TEXT("겨누지 않은 대상은 맞지 않는다 (앞)"), Front->GetHealth(), 90.f);
	TestEqual(TEXT("겨누지 않은 대상은 맞지 않는다 (뒤)"), Back->GetHealth(), 100.f);
	Shooter->SetAimDirection(FVector::ZeroVector);

	// 죽은 대상은 화살을 가리지 않는다
	Front->ReceiveHit(MakeHit(Shooter, MakeAttackData(500.f)));
	TestTrue(TEXT("앞 대상이 죽었다"), Front->IsDead());
	TestTrue(TEXT("한 번 더 쏜다"), ASC->TryActivateByAbilityTag(PCTags::Ability_Attack));
	TestWorld.Tick(1.f);
	TestEqual(TEXT("죽은 대상을 지나쳐 뒤 대상이 맞는다"), Back->GetHealth(), 90.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
