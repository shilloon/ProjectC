// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectC : ModuleRules
{
	public ProjectC(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ProjectC",
			"ProjectC/Variant_Platforming",
			"ProjectC/Variant_Platforming/Animation",
			"ProjectC/Variant_Combat",
			"ProjectC/Variant_Combat/AI",
			"ProjectC/Variant_Combat/Animation",
			"ProjectC/Variant_Combat/Gameplay",
			"ProjectC/Variant_Combat/Interfaces",
			"ProjectC/Variant_Combat/UI",
			"ProjectC/Variant_SideScrolling",
			"ProjectC/Variant_SideScrolling/AI",
			"ProjectC/Variant_SideScrolling/Gameplay",
			"ProjectC/Variant_SideScrolling/Interfaces",
			"ProjectC/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
