using UnrealBuildTool;

public class Signal47 : ModuleRules
{
	public Signal47(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "Slate", "SlateCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RHI", "ApplicationCore", "JsonUtilities", "UMG", "RenderCore", "ImageWrapper", "AudioMixer" });
		// (Unreal only) where the free models stand, read at run time (S47Props.h)
		RuntimeDependencies.Add("$(ProjectDir)/Config/Props/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/Free/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/Free/Trim/*.json", StagedFileType.UFS);
	}
}
