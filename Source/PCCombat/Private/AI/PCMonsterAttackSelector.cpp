#include "AI/PCMonsterAttackSelector.h"

namespace
{
	/** 쓸 수 있는 공격이 하나도 없을 때의 접근 거리 */
	constexpr float FallbackApproachRange = 150.f;

	bool IsHealthInRange(const FPCMonsterAttackOption& Option, float HealthRatio)
	{
		return HealthRatio >= Option.MinHealthRatio && HealthRatio <= Option.MaxHealthRatio;
	}

	bool IsOffCooldown(const FPCMonsterAttackOption& Option, double Now, const TMap<FGameplayTag, double>& LastUsedTimes)
	{
		const double* LastUsed = LastUsedTimes.Find(Option.AbilityTag);
		return !LastUsed || (Now - *LastUsed) >= Option.Cooldown;
	}
}

bool FPCMonsterAttackSelector::IsAvailable(const FPCMonsterAttackOption& Option, float Distance, float HealthRatio, double Now, const TMap<FGameplayTag, double>& LastUsedTimes)
{
	return Option.AbilityTag.IsValid()
		&& Option.Weight > 0.f
		&& Distance >= Option.MinRange && Distance <= Option.MaxRange
		&& IsHealthInRange(Option, HealthRatio)
		&& IsOffCooldown(Option, Now, LastUsedTimes);
}

int32 FPCMonsterAttackSelector::Select(TConstArrayView<FPCMonsterAttackOption> Options, float Distance, float HealthRatio, double Now,
	const TMap<FGameplayTag, double>& LastUsedTimes, FRandomStream& Random)
{
	float TotalWeight = 0.f;
	for (const FPCMonsterAttackOption& Option : Options)
	{
		if (IsAvailable(Option, Distance, HealthRatio, Now, LastUsedTimes))
		{
			TotalWeight += Option.Weight;
		}
	}
	if (TotalWeight <= 0.f)
	{
		return INDEX_NONE;
	}

	float Roll = Random.FRandRange(0.f, TotalWeight);
	int32 LastAvailable = INDEX_NONE;
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		if (!IsAvailable(Options[Index], Distance, HealthRatio, Now, LastUsedTimes))
		{
			continue;
		}

		LastAvailable = Index;
		Roll -= Options[Index].Weight;
		if (Roll <= 0.f)
		{
			return Index;
		}
	}
	return LastAvailable;
}

float FPCMonsterAttackSelector::GetApproachRange(TConstArrayView<FPCMonsterAttackOption> Options, float HealthRatio, double Now,
	const TMap<FGameplayTag, double>& LastUsedTimes)
{
	float ReadyRange = -1.f;
	float AnyRange = -1.f;
	for (const FPCMonsterAttackOption& Option : Options)
	{
		if (!Option.AbilityTag.IsValid() || Option.Weight <= 0.f || !IsHealthInRange(Option, HealthRatio))
		{
			continue;
		}

		AnyRange = FMath::Max(AnyRange, Option.MaxRange);
		if (IsOffCooldown(Option, Now, LastUsedTimes))
		{
			ReadyRange = FMath::Max(ReadyRange, Option.MaxRange);
		}
	}

	if (ReadyRange >= 0.f)
	{
		return ReadyRange;
	}
	return AnyRange >= 0.f ? AnyRange : FallbackApproachRange;
}
