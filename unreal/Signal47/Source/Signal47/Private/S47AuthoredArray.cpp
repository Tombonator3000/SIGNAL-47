// Unreal-only original Blender antenna. Existing pointing, stories and beacon positions remain authoritative.
#include "S47Web.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/CommandLine.h"

void AS47Web::BuildAuthoredArray()
{
	// Opt-out exists for reproducible before/after captures of the same packaged game.
	if (FParse::Param(FCommandLine::Get(), TEXT("S47LegacyArray"))) return;
	const int32 Parent = Handle(TEXT("ext.dishArray.group"));
	if (!Nodes.IsValidIndex(Parent) || Dishes.IsEmpty()) return;
	UStaticMesh* Meshes[3];
	const TCHAR* Names[] = { TEXT("SM_Array_Base"), TEXT("SM_Array_Azimuth"), TEXT("SM_Array_Elevation") };
	for (int32 Level = 0; Level < 3; ++Level)
	{
		Meshes[Level] = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/RadioArray/Dish/%s.%s"), Names[Level], Names[Level]));
		if (!Meshes[Level])
		{
			UE_LOG(LogTemp, Error, TEXT("S47: original array asset missing: %s; retaining complete legacy array"), Names[Level]);
			return;
		}
	}
	// Hide only the nine original instanced parts; beacons and all gameplay handles stay in place.
	for (const FDishPart& Part : DishParts)
	{
		if (!Nodes.IsValidIndex(Part.Node)) continue;
		SetVisible(Part.Node, false);
		if (UPrimitiveComponent* Old = Cast<UPrimitiveComponent>(Nodes[Part.Node].Comp.Get()))
			Old->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	DishParts.Reset();
	for (int32 Level = 0; Level < 3; ++Level)
	{
		UInstancedStaticMeshComponent* Instances = NewObject<UInstancedStaticMeshComponent>(this, Names[Level]);
		Instances->SetupAttachment(Nodes[Parent].Comp.Get());
		Instances->SetMobility(EComponentMobility::Movable);
		Instances->SetStaticMesh(Meshes[Level]);
		Instances->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Instances->SetCollisionResponseToAllChannels(ECR_Ignore);
		Instances->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Instances->SetCastShadow(true);
		Instances->RegisterComponent();
		FDishPart Part;
		Part.Node = AdoptNode(Instances, Parent, Names[Level]);
		Part.Level = Level;
		for (int32 I = 0; I < Dishes.Num(); ++I)
		{
			Instances->AddInstance(FTransform::Identity);
			Part.DishIdx.Add(I);
		}
		DishParts.Add(MoveTemp(Part));
	}
	DrawDishes();
	UE_LOG(LogTemp, Display, TEXT("S47: original Blender array: %d antennas, 3 instanced assemblies, existing azimuth/elevation and beacons"), Dishes.Num());
}
