// Unreal-only original 1420 AM station. The closed studio, signs and anomaly proxies retain state.
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Props.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47RadioStation, Log, All);

void AS47Web::BuildRadioStation()
{
	bool bHaveShell = false;
	for (const FS47WebNode& N : Nodes) bHaveShell |= N.Name == TEXT("prop:radioStationShell");
	if (!bHaveShell) return; // Missing native assets or -S47PropsOff: keep the complete old station.
	// Props has already placed the authored station and applied the gas-station trim. This trim
	// composes both buildings' cuts: the meshes share town materials, so a radio-only cut would
	// otherwise bring part of the old gas station back. Never hide the town's entire merged mesh.
	FString Text;
	TSharedPtr<FJsonObject> Patches;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("RadioArray/Station/station-patches.json"))) ||
		!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Patches) || !Patches.IsValid())
	{
		UE_LOG(LogS47RadioStation, Warning, TEXT("S47: 1420 AM station trim unavailable"));
		return;
	}
	const TSharedPtr<FJsonObject>* Meshes = nullptr;
	FString GroupPath;
	if (!Patches->TryGetStringField(TEXT("group"), GroupPath) || !Patches->TryGetObjectField(TEXT("meshes"), Meshes)) return;
	const int32 Group = Handle(GroupPath);
	if (!Nodes.IsValidIndex(Group)) return;
	struct FPatch { UStaticMeshComponent* Comp; UStaticMesh* Mesh; int32 Node; bool bHide; };
	TArray<FPatch> Ready;
	int32 Matched = 0;
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		const FS47WebNode& N = Nodes[I];
		if (N.Parent != Group || N.Geo < 0) continue;
		const FString Name = FString::Printf(TEXT("g%d"), N.Geo);
		const TSharedPtr<FJsonObject>* Patch = nullptr;
		if (!(*Meshes)->TryGetObjectField(Name, Patch)) continue;
		++Matched;
		UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N.Comp.Get());
		const UStaticMesh* Previous = C ? C->GetStaticMesh() : nullptr;
		const int32 Tris = Previous && Previous->GetRenderData() && Previous->GetRenderData()->LODResources.Num() ?
			Previous->GetRenderData()->LODResources[0].GetNumTriangles() : -1;
		const int32 Original = (*Patch)->GetIntegerField(TEXT("tris"));
		const int32 GasRemoved = (*Patch)->GetIntegerField(TEXT("gasRemoved"));
		const int32 Result = (*Patch)->GetIntegerField(TEXT("resultTris"));
		if (Tris == Result) continue; // Already applied; do not accumulate scene changes.
		const bool bHide = (*Patch)->GetBoolField(TEXT("hidden"));
		UStaticMesh* Cut = bHide ? nullptr : LoadObject<UStaticMesh>(nullptr, *(*Patch)->GetStringField(TEXT("asset")));
		if (!C || (Tris != Original && Tris != Original - GasRemoved) || (!bHide && !Cut))
		{
			UE_LOG(LogS47RadioStation, Error, TEXT("S47: 1420 AM trim %s stale or missing (triangles %d, expected %d or %d); no partial patch applied"),
				*Name, Tris, Original, Original - GasRemoved);
			return;
		}
		// Material slots are held on the existing component: the town's original materials and
		// flood/state bindings remain intact even though the imported patch has default materials.
		Ready.Add({ C, Cut, I, bHide });
	}
	if (Matched != (*Meshes)->Values.Num())
	{
		UE_LOG(LogS47RadioStation, Error, TEXT("S47: 1420 AM trim group changed; refusing a partial patch"));
		return;
	}
	for (const FPatch& P : Ready)
	{
		if (P.bHide)
		{
			SetVisible(P.Node, false);
			P.Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		else
		{
			// Preserve even materials supplied by the original mesh rather than an override.
			TArray<UMaterialInterface*> Materials;
			for (int32 Slot = 0; Slot < P.Comp->GetNumMaterials(); ++Slot) Materials.Add(P.Comp->GetMaterial(Slot));
			P.Comp->SetStaticMesh(P.Mesh);
			for (int32 Slot = 0; Slot < Materials.Num(); ++Slot) P.Comp->SetMaterial(Slot, Materials[Slot]);
		}
	}
	// Town.ts baked its old room photograph into a quad directly over the front window. Keep
	// that dynamic atlas (presenter on duty / empty after sign-off) flush against the studio's
	// rear wall, so it cannot cut through the room or show its reverse to an interior camera.
	// build_station.py: back lining centre Blender Y=9.7, thickness .04; its inner face is
	// world X=8029.68. Put the quad .02 m in front of that face, at world X=8029.66.
	// Exported quad X=19.965 is baked into its vertices; the runtime parent is identity and
	// Caliche translates by +8000 X. Thus its node needs offset 29.66-19.965=9.695, once.
	for (const FString& Path : PathsUnder(TEXT("world.town.runtime.panels.studio.")))
	{
		FS47Obj Backdrop = Obj(Path);
		FVector At = Backdrop.Position();
		At.X = 9.695; // World x 8029.66000015, two centimetres before the walnut lining.
		Backdrop.SetPosition(At);
	}
	// These two giant archival sprites stood in for a lit window and rooftop board.
	// Real room/facade fixtures now supply their light; a sprite would float in the glass.
	if (FS47GlowPoints* G = Glow(TEXT("world.town.glow.points")))
		for (int32 I = 0; I < G->Pos.Num(); ++I)
		{
			const FVector P = G->Pos[I];
			if ((FMath::Abs(P.X - 19.6) < .02 && FMath::Abs(P.Z - 2037.2) < .02) ||
				(FMath::Abs(P.X - 19.7) < .02 && FMath::Abs(P.Z - 2036.1) < .02)) G->SetSize(I, 0.f);
		}
	// The public lobby has its own door. The existing studio buzzer keeps its dialogue and moves
	// to the jamb so that its old broad invisible proxy cannot swallow the new door's use trace.
	int32 DoorNode = -1;
	for (int32 I = 0; I < Nodes.Num(); ++I) if (Nodes[I].Name == TEXT("prop:radioStationDoor")) { DoorNode = I; break; }
	if (Nodes.IsValidIndex(DoorNode))
	{
		if (UPrimitiveComponent* Door = Cast<UPrimitiveComponent>(Nodes[DoorNode].Comp.Get()))
			Door->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		FS47Obj Buzzer = Obj(TEXT("world.town.proxies.amDoor"));
		if (Buzzer) { FVector At = Buzzer.Position(); At.Z = 2032.6; Buzzer.SetPosition(At); }
		// AS47Web builds before the HUD initializes Main/Inter. Register once that owner exists,
		// including a world rebuilt by Load case. Weak delegates are cancelled with this actor.
		const TSharedPtr<FTimerHandle> Timer = MakeShared<FTimerHandle>();
		const TSharedPtr<int32> Attempts = MakeShared<int32>(0);
		GetWorldTimerManager().SetTimer(*Timer, FTimerDelegate::CreateWeakLambda(this, [this, DoorNode, Timer, Attempts]
		{
			US47Main* M = US47Main::Get(this);
			if (!M || M->Web != this || !M->Inter)
			{
				if (++*Attempts >= 100) { GetWorldTimerManager().ClearTimer(*Timer); UE_LOG(LogS47RadioStation, Warning, TEXT("S47: station lobby interaction owner unavailable")); }
				return;
			}
			M->Inter->Add({ TEXT("radio:lobbyDoor"), ObjAt(DoorNode),
				[] { return AS47Props::IsDoorOpen(TEXT("radioLobbyDoor")) ? FString(TEXT("Close station lobby door")) : FString(TEXT("Open station lobby door")); },
				[] { AS47Props::UseDoor(TEXT("radioLobbyDoor")); }, 2.8,
				[] { return FString(TEXT("1420 AM. A narrow public lobby, a noticeboard and a buzzer beside the entrance.")); }, 5 });
			GetWorldTimerManager().ClearTimer(*Timer);
			UE_LOG(LogS47RadioStation, Display, TEXT("S47: 1420 AM public lobby door registered; studio buzzer dialogue retained"));
		}), 0.1f, true);
	}
	UE_LOG(LogS47RadioStation, Display, TEXT("S47: 1420 AM original station: %d shared meshes patched; studio, ON AIR, 1420 AM, mast and story proxies preserved"), Ready.Num());
}
