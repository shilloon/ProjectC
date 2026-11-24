#include "AbilitySystem/PCAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Data/PCAbilityData.h"
#include "PCLog.h"

FGameplayAbilitySpecHandle UPCAbilitySystemComponent::GrantAbilityFromData(const UPCAbilityData* Data)
{
	if (!Data || !Data->AbilityClass || !Data->AbilityTag.IsValid())
	{
		UE_LOG(LogPC, Warning, TEXT("GrantAbilityFromData: 능력 클래스나 태그가 비어 있는 데이터 '%s'"), *GetNameSafe(Data));
		return FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec Spec(Data->AbilityClass, 1, INDEX_NONE, const_cast<UPCAbilityData*>(Data));
	const FGameplayAbilitySpecHandle Handle = GiveAbility(Spec);
	HandlesByAbilityTag.Add(Data->AbilityTag, Handle);
	return Handle;
}

bool UPCAbilitySystemComponent::TryActivateByAbilityTag(FGameplayTag AbilityTag)
{
	const FGameplayAbilitySpecHandle* Handle = HandlesByAbilityTag.Find(AbilityTag);
	return Handle ? TryActivateAbility(*Handle) : false;
}

void UPCAbilitySystemComponent::CancelByAbilityTag(FGameplayTag AbilityTag)
{
	if (const FGameplayAbilitySpecHandle* Handle = HandlesByAbilityTag.Find(AbilityTag))
	{
		CancelAbilityHandle(*Handle);
	}
}

void UPCAbilitySystemComponent::ApplySetByCallerEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, FGameplayTag DataTag, float Magnitude, AActor* Instigator)
{
	FGameplayEffectContextHandle Context = MakeEffectContext();
	Context.AddInstigator(Instigator, Instigator);

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(DataTag, Magnitude);
		ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
