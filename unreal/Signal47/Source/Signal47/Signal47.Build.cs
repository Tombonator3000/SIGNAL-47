using UnrealBuildTool;

public class Signal47 : ModuleRules
{
	public Signal47(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "Slate", "SlateCore" });
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "RHI", "ApplicationCore", "JsonUtilities", "UMG", "RenderCore", "ImageWrapper", "AudioMixer", "MoviePlayer", "HairStrandsCore", "AnimGraphRuntime", "Niagara", "PhysicsCore" }); // D46: the trip camera reads its body setup
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
		// Freeform's height table (S47Relief), read from disk beside the cooked relief meshes in the same folder.
		RuntimeDependencies.Add("$(ProjectDir)/Content/FreeformTerrain/*.bin", StagedFileType.UFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/FreeformTerrain/*.json", StagedFileType.UFS);
		// The rhythm of a Freeform night (S47Rhythm), read at run time.
		RuntimeDependencies.Add("$(ProjectDir)/Content/Director/*.json", StagedFileType.UFS);
		// Tom's scripted events (S47ScriptPlayer), one file each.
		RuntimeDependencies.Add("$(ProjectDir)/Content/Director/scripted/*.json", StagedFileType.UFS);
		// D46-20261110: the night's events' own scripts (S47EventScript), one file each.
		RuntimeDependencies.Add("$(ProjectDir)/Content/Director/Events/*.json", StagedFileType.UFS);
		// STORY-ON-FREEFORM-01: the story director's acts (S47StoryDirector), kept apart from Freeform's events.
		RuntimeDependencies.Add("$(ProjectDir)/Content/Director/Story/*.json", StagedFileType.UFS);
	}
}
