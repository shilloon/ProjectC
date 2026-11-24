using UnrealBuildTool;

public class PCCombatEditor : ModuleRules
{
	public PCCombatEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"UnrealEd",
			"AssetRegistry",
			"GameplayTags",
			"GameplayAbilities",
			"AIModule",
			"StateTreeModule",
			"StateTreeEditorModule",
			"GameplayStateTreeModule",
			"PropertyBindingUtils",
			"PCCore",
			"PCCombat"
		});
	}
}
