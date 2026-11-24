#include "AbilitySystem/Abilities/PCAbility_LungeAttack.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Data/PCAbilityData.h"
#include "GameFramework/Actor.h"
#include "GameFramework/RootMotionSource.h"

namespace
{
	/** 이동 속도 계산에서 0으로 나누지 않기 위한 최소 이동 시간 */
	constexpr float MinLungeDuration = 0.05f;
}

void UPCAbility_LungeAttack::OnPhaseChanged(EPCActionPhase OldPhase, EPCActionPhase NewPhase)
{
	// 이동을 먼저 걸어 두고 판정을 시작한다
	if (NewPhase == EPCActionPhase::Active)
	{
		const UPCAbilityData* Data = GetAbilityData();
		const AActor* Avatar = GetAvatarActorFromActorInfo();
		if (Data && Avatar && Data->LungeDistance > 0.f)
		{
			const float Duration = FMath::Max(Data->Timing.Active, MinLungeDuration);
			const float Strength = Data->LungeDistance / Duration;

			UAbilityTask_ApplyRootMotionConstantForce* Task = UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
				this, NAME_None, Avatar->GetActorForwardVector(), Strength, Duration,
				false, nullptr, ERootMotionFinishVelocityMode::SetVelocity, FVector::ZeroVector, 0.f, true);
			Task->ReadyForActivation();
		}
	}

	Super::OnPhaseChanged(OldPhase, NewPhase);
}
