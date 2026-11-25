#include "Equipment/PCEquipmentComponent.h"
#include "AbilitySystem/Effects/PCEffect_ActionMoveSpeed.h"
#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Data/PCAbilityData.h"
#include "Data/PCWeaponData.h"
#include "PCGameplayTags.h"
#include "PCLog.h"

UPCEquipmentComponent::UPCEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPCEquipmentComponent::InitializeEquipment(UPCAbilitySystemComponent* InAbilitySystem)
{
	AbilitySystem = InAbilitySystem;
	if (!AbilitySystem)
	{
		return;
	}

	if (SwapAbility)
	{
		AbilitySystem->GrantAbilityFromData(SwapAbility);
	}
	if (Loadout.Num() > 0)
	{
		EquipIndex(Loadout.IsValidIndex(StartingIndex) ? StartingIndex : 0);
	}
}

const UPCWeaponData* UPCEquipmentComponent::GetCurrentWeapon() const
{
	return GetWeaponAt(CurrentIndex);
}

const UPCWeaponData* UPCEquipmentComponent::GetWeaponAt(int32 Index) const
{
	return Loadout.IsValidIndex(Index) ? Loadout[Index].Get() : nullptr;
}

const FPCWeaponAbilitySlot* UPCEquipmentComponent::FindSlot(FGameplayTag InputTag) const
{
	const UPCWeaponData* Weapon = GetCurrentWeapon();
	return Weapon ? Weapon->FindSlot(InputTag) : nullptr;
}

bool UPCEquipmentComponent::RequestSwap(int32 Index)
{
	if (!AbilitySystem || !GetWeaponAt(Index) || Index == CurrentIndex)
	{
		return false;
	}

	// 교체 기술은 발동할 때 이 번호를 읽어 간다. 발동하지 못하면 요청은 버린다.
	PendingIndex = Index;
	const bool bStarted = AbilitySystem->TryActivateByAbilityTag(PCTags::Ability_SwapWeapon);
	if (!bStarted)
	{
		PendingIndex = INDEX_NONE;
	}
	return bStarted;
}

void UPCEquipmentComponent::EquipIndex(int32 Index)
{
	const UPCWeaponData* NewWeapon = GetWeaponAt(Index);
	if (!AbilitySystem || !NewWeapon || Index == CurrentIndex)
	{
		return;
	}

	const UPCWeaponData* OldWeapon = GetCurrentWeapon();
	if (OldWeapon)
	{
		RemoveWeapon(*OldWeapon);
	}

	CurrentIndex = Index;
	PendingIndex = INDEX_NONE;
	GrantWeapon(*NewWeapon);

	UE_LOG(LogPC, Log, TEXT("[전투] %s: 무기 교체 %s → %s"), *GetNameSafe(GetOwner()),
		OldWeapon ? *OldWeapon->WeaponTag.ToString() : TEXT("없음"), *NewWeapon->WeaponTag.ToString());
	OnWeaponChanged.Broadcast(NewWeapon, OldWeapon);
}

void UPCEquipmentComponent::GrantWeapon(const UPCWeaponData& Weapon)
{
	for (const FPCWeaponAbilitySlot& Slot : Weapon.AbilitySlots)
	{
		if (Slot.Ability)
		{
			AbilitySystem->GrantAbilityFromData(Slot.Ability);
		}
	}

	if (!FMath::IsNearlyEqual(Weapon.HeldMoveSpeedScale, 1.f))
	{
		HeldEffectHandle = AbilitySystem->ApplySetByCallerEffectToSelf(UPCEffect_ActionMoveSpeed::StaticClass(), PCTags::Data_MoveSpeed, Weapon.HeldMoveSpeedScale, GetOwner());
	}
}

void UPCEquipmentComponent::RemoveWeapon(const UPCWeaponData& Weapon)
{
	for (const FPCWeaponAbilitySlot& Slot : Weapon.AbilitySlots)
	{
		if (Slot.Ability)
		{
			AbilitySystem->RemoveAbilityByTag(Slot.Ability->AbilityTag);
		}
	}

	if (HeldEffectHandle.IsValid())
	{
		AbilitySystem->RemoveActiveGameplayEffect(HeldEffectHandle);
		HeldEffectHandle.Invalidate();
	}
}
