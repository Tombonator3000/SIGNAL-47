using UnrealBuildTool;

public class Signal47EditorTarget : TargetRules
{
	public Signal47EditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Signal47");
	}
}
