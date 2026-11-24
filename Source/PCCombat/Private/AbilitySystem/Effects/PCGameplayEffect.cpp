#include "AbilitySystem/Effects/PCGameplayEffect.h"
#include "GameplayEffectComponents/TargetTagRequirementsGameplayEffectComponent.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

void UPCGameplayEffect::AddSetByCallerModifier(const FGameplayAttribute& Attribute, FGameplayTag DataTag, EGameplayModOp::Type Operation)
{
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = DataTag;

	FGameplayModifierInfo Modifier;
	Modifier.Attribute = Attribute;
	Modifier.ModifierOp = Operation;
	Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
	Modifiers.Add(Modifier);
}

void UPCGameplayEffect::SetDurationFromCaller(FGameplayTag DataTag)
{
	FSetByCallerFloat SetByCaller;
	SetByCaller.DataTag = DataTag;

	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
}

void UPCGameplayEffect::GrantTagToTarget(FGameplayTag Tag)
{
	UTargetTagsGameplayEffectComponent* Component = CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("GrantedTags"));
	GEComponents.Add(Component);

	FInheritedTagContainer Tags;
	Tags.AddTag(Tag);
	Component->SetAndApplyTargetTagChanges(Tags);
}

void UPCGameplayEffect::PauseWhileTargetHasAnyTag(const FGameplayTagContainer& Tags)
{
	UTargetTagRequirementsGameplayEffectComponent* Component = CreateDefaultSubobject<UTargetTagRequirementsGameplayEffectComponent>(TEXT("TagRequirements"));
	GEComponents.Add(Component);

	Component->OngoingTagRequirements.IgnoreTags = Tags;
}
