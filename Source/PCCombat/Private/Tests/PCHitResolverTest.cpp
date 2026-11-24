#include "Misc/AutomationTest.h"
#include "Hit/PCHitResolver.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCHitResolverSectorTest, "PC.Combat.HitResolver.Sector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCHitResolverSectorTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	const FVector Forward = FVector::ForwardVector;

	TestTrue(TEXT("정면, 범위 안이면 맞는다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(100, 0, 0), 150.f, 45.f));
	TestFalse(TEXT("정면이어도 범위 밖이면 빗나간다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(200, 0, 0), 150.f, 45.f));
	TestFalse(TEXT("옆은 빗나간다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(0, 100, 0), 150.f, 45.f));
	TestFalse(TEXT("뒤는 빗나간다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(-100, 0, 0), 150.f, 45.f));
	TestTrue(TEXT("각도 경계 안쪽(약 42도)은 맞는다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(100, 90, 0), 150.f, 45.f));
	TestFalse(TEXT("각도 경계 바깥(약 48도)은 빗나간다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(100, 110, 0), 150.f, 45.f));
	TestTrue(TEXT("높이 차이는 무시한다"), FPCHitResolver::IsWithinSector(Origin, Forward, FVector(100, 0, 500), 150.f, 45.f));
	TestTrue(TEXT("같은 위치는 맞는다"), FPCHitResolver::IsWithinSector(Origin, Forward, Origin, 150.f, 45.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPCHitResolverGuardTest, "PC.Combat.HitResolver.Guard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPCHitResolverGuardTest::RunTest(const FString& Parameters)
{
	const FVector Defender = FVector::ZeroVector;
	const FVector Facing = FVector::ForwardVector;

	TestTrue(TEXT("정면에서 온 공격은 막을 수 있다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(200, 0, 0), 60.f));
	TestFalse(TEXT("뒤에서 온 공격은 막을 수 없다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(-200, 0, 0), 60.f));
	TestFalse(TEXT("옆에서 온 공격은 방어 각도 60도로 막을 수 없다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(0, 200, 0), 60.f));
	TestTrue(TEXT("방어 각도가 100도면 옆도 막는다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(0, 200, 0), 100.f));
	TestTrue(TEXT("멀리서 온 공격도 방향만 맞으면 막는다"), FPCHitResolver::IsAttackFromFront(Defender, Facing, FVector(5000, 0, 0), 60.f));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
