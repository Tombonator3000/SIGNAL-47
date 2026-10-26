// Unreal-only authored street slice. Story anchors and movement surfaces remain owned by Town.
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CalicheStreet, Log, All);

void AS47Web::BuildCalicheStreet()
{
	TArray<int32> Shop;
	const bool bLegacy = FParse::Param(FCommandLine::Get(), TEXT("S47LegacyCalicheStreet"));
	bool bShell = false, bInterior = false, bGlass = false;
	TSet<FString> Predecessors;
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		const FString& Name = Nodes[I].Name;
		if (Name.StartsWith(TEXT("prop:gas")) || Name.StartsWith(TEXT("prop:radioStation"))) Predecessors.Add(Name);
		if (Name.StartsWith(TEXT("prop:streetGround")))
		{
			// These meshes dress an existing surface; they never replace the walker/road floor
			// or become invisible blockers for photographs and interactions.
			if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(Nodes[I].Comp.Get())) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			if (bLegacy) SetVisible(I, false);
		}
		if (Name.StartsWith(TEXT("prop:souvenir")))
		{
			Shop.Add(I);
			bShell |= Name == TEXT("prop:souvenirShell");
			bInterior |= Name == TEXT("prop:souvenirInterior");
			bGlass |= Name == TEXT("prop:souvenirGlass");
			if (Name == TEXT("prop:souvenirGlass"))
				if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(Nodes[I].Comp.Get())) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	bool bPredecessors = true;
	for (const TCHAR* Name : {TEXT("prop:gasStation"), TEXT("prop:gasDoor"), TEXT("prop:gasSignFaces"),
		TEXT("prop:gasCanopyTubes"), TEXT("prop:gasInsideTubes"), TEXT("prop:radioStationShell"),
		TEXT("prop:radioStationInterior"), TEXT("prop:radioStationGlass")}) bPredecessors &= Predecessors.Contains(Name);
	if (!bShell || !bInterior || !bGlass || !bPredecessors || bLegacy)
	{
		for (int32 I : Shop)
		{
			SetVisible(I, false);
			if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(Nodes[I].Comp.Get())) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		return; // Partial/missing imports keep the complete original shop.
	}
	auto Reject = [&] {
		for (int32 I : Shop)
		{
			SetVisible(I, false);
			if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(Nodes[I].Comp.Get())) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		UE_LOG(LogS47CalicheStreet, Error, TEXT("S47: Caliche street trim rejected; original souvenir shop retained"));
	};
	FString Text;
	TSharedPtr<FJsonObject> Patches;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("CalicheStreet/street-patches.json"))) ||
		!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Patches) || !Patches.IsValid()) { Reject(); return; }
	const TSharedPtr<FJsonObject>* Meshes = nullptr;
	FString GroupPath;
	if (!Patches->TryGetStringField(TEXT("group"), GroupPath) || !Patches->TryGetObjectField(TEXT("meshes"), Meshes)) { Reject(); return; }
	const int32 Group = Handle(GroupPath);
	if (!Nodes.IsValidIndex(Group)) { Reject(); return; }
	struct FPatch { UStaticMeshComponent* Comp; UStaticMesh* Mesh; int32 Node; bool bHide; };
	TArray<FPatch> Ready;
	int32 Matched = 0;
	for (int32 I = 0; I < Nodes.Num(); ++I)
	{
		const FS47WebNode& N = Nodes[I];
		if (N.Parent != Group || N.Geo < 0) continue;
		const TSharedPtr<FJsonObject>* Row = nullptr;
		if (!(*Meshes)->TryGetObjectField(FString::Printf(TEXT("g%d"), N.Geo), Row)) continue;
		++Matched;
		UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N.Comp.Get());
		const UStaticMesh* Previous = C ? C->GetStaticMesh() : nullptr;
		const int32 Tris = Previous && Previous->GetRenderData() && Previous->GetRenderData()->LODResources.Num() ? Previous->GetRenderData()->LODResources[0].GetNumTriangles() : -1;
		const int32 Original = (*Row)->GetIntegerField(TEXT("tris"));
		const int32 Gas = (*Row)->GetIntegerField(TEXT("gasRemoved"));
		const int32 Radio = (*Row)->GetIntegerField(TEXT("radioRemoved"));
		const int32 Result = (*Row)->GetIntegerField(TEXT("resultTris"));
		const bool bHide = (*Row)->GetBoolField(TEXT("hidden"));
		UStaticMesh* Cut = bHide ? nullptr : LoadObject<UStaticMesh>(nullptr, *(*Row)->GetStringField(TEXT("asset")));
		if (!C || (Tris != Original && Tris != Original - Gas && Tris != Original - Gas - Radio && Tris != Result) || (!bHide && !Cut)) { Reject(); return; }
		Ready.Add({C, Cut, I, bHide});
	}
	if (Matched != (*Meshes)->Values.Num()) { Reject(); return; }
	for (const FPatch& P : Ready)
	{
		if (P.bHide) { SetVisible(P.Node, false); P.Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
		else
		{
			TArray<UMaterialInterface*> Materials;
			for (int32 Slot = 0; Slot < P.Comp->GetNumMaterials(); ++Slot) Materials.Add(P.Comp->GetMaterial(Slot));
			P.Comp->SetStaticMesh(P.Mesh);
			for (int32 Slot = 0; Slot < Materials.Num(); ++Slot) P.Comp->SetMaterial(Slot, Materials[Slot]);
		}
	}
	// The actual A02 figure already belongs to the anomaly system. Its former flat display
	// background would cover the new room. Keep the proxy, physical figure, price and film IDs.
	for (const FString& Path : PathsUnder(TEXT("world.town.runtime.panels.display."))) SetVisible(Handle(Path), false);
	// A painted facade replaces the old floating TOURS board; keep its familiar interaction ID.
	if (FS47Obj Sign = Obj(TEXT("world.town.proxies.souvenirSign"))) Sign.SetPosition(FVector(20.48, 8.332807474, 2064));
	bool bPaintedRadio = false;
	for (const FS47WebNode& N : Nodes) if (N.Name == TEXT("prop:radioStationShell"))
		if (UStaticMeshComponent* C = Cast<UStaticMeshComponent>(N.Comp.Get()))
			for (UMaterialInterface* Material : C->GetMaterials())
				bPaintedRadio |= Material && Material->GetName().Contains(TEXT("WeatheredFacadePaint"));
	if (bPaintedRadio)
		for (const FString& Path : PathsUnder(TEXT("world.town.runtime.panels.amSign."))) SetVisible(Handle(Path), false);
	UE_LOG(LogS47CalicheStreet, Display, TEXT("S47: Caliche street ready: %d composed gas/radio/souvenir patches; existing story anchors and walk floors preserved"), Ready.Num());
}
