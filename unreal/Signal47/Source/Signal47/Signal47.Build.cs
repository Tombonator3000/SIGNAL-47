using UnrealBuildTool;

public class Signal47 : ModuleRules
{
	public Signal47(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RHI", "ApplicationCore", "Json", "JsonUtilities", "UMG" });
	}
}
