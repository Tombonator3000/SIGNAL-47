using UnrealBuildTool;

public class Signal47 : ModuleRules
{
	public Signal47(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "Slate", "SlateCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RHI", "ApplicationCore", "JsonUtilities", "UMG", "RenderCore", "ImageWrapper", "AudioMixer", "MoviePlayer" });
		// (Unreal only) where the free models stand, read at run time (S47Props.h)
		RuntimeDependencies.Add("$(ProjectDir)/Config/Props/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/Free/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/RadioArray/Station/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/CalicheStreet/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/MesaDiner/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/CalicheStreet/Nature/*.json", StagedFileType.UFS);
	}
}
