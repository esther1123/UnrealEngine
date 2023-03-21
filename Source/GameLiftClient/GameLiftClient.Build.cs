using UnrealBuildTool;

public class GameLiftClient : ModuleRules
{
	public GameLiftClient(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnforceIWYU = false;
		bLegacyPublicIncludePaths = false;
		ShadowVariableWarningLevel = WarningLevel.Error;
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"HTTP", 
			"Json", 
			"JsonUtilities",
			"WebBrowserWidget",
			"UMG",
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	}
}