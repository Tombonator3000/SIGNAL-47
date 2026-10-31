using UnrealBuildTool;

public class Signal47 : ModuleRules
{
	public Signal47(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "Slate", "SlateCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RHI", "ApplicationCore", "JsonUtilities", "UMG", "RenderCore", "ImageWrapper", "AudioMixer", "MoviePlayer", "HairStrandsCore", "AnimGraphRuntime", "Niagara" });
		// Opt-in developer finale capture; ordinary screenshots retain their existing path.
		PrivateDependencyModuleNames.Add("ImageWriteQueue");
		if (Target.bBuildEditor)
		{
			// HUM01 pose conversion and read-only cinema material mapping are editor-only.
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MeshDescription", "StaticMeshDescription", "SkeletalMeshDescription", "RawMesh", "AnimationCore", "AssetRegistry" });
		}
		// (Unreal only) where the free models stand, read at run time (S47Props.h)
		RuntimeDependencies.Add("$(ProjectDir)/Config/Props/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/Free/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/RadioArray/Station/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/CalicheStreet/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/MesaDiner/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/TownBuildings/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/CalicheHomes/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/CalicheAccess/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/MotelV2/*.json", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/CalicheStreet/Nature/*.json", StagedFileType.UFS);
	}
}
