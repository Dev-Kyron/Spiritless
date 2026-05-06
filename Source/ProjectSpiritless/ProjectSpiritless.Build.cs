using UnrealBuildTool;

public class ProjectSpiritless : ModuleRules
{
	public ProjectSpiritless(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"PaperZD",
			"Paper2D",
			"GameplayTasks",
			"CinematicCamera",
			"UMG",
			"Slate",
			"SlateCore",
			"Niagara",
			"NiagaraCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
