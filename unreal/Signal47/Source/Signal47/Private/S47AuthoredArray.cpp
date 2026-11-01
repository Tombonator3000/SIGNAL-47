// Unreal-only original Blender antenna. Existing pointing, stories and beacon positions remain authoritative.
#include "S47Web.h"
#include "S47GroundCover.h"
#include "S47Three.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
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
	// The authored plinths end at root Y=0, while the graded array ground is
	// Y=-0.62. Extend only their concrete below grade: the moving assemblies,
	// feed/beacon coordinates and story sight lines keep their established pivots.
	UMaterialInterface* Concrete = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/RadioArray/Dish/Materials/M_Array_Concrete.M_Array_Concrete"));
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube && Concrete)
	{
		UInstancedStaticMeshComponent* Footings = NewObject<UInstancedStaticMeshComponent>(this, TEXT("ArrayGroundFootings"));
		Footings->SetupAttachment(Nodes[Parent].Comp.Get());
		Footings->SetMobility(EComponentMobility::Movable);
		Footings->SetStaticMesh(Cube);
		Footings->SetMaterial(0, Concrete); // FBX compresses unused slots; resolve the original by asset identity.
		Footings->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Footings->SetCollisionResponseToAllChannels(ECR_Ignore);
		Footings->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		Footings->SetCastShadow(true);
		Footings->RegisterComponent();
		AdoptNode(Footings, Parent, TEXT("ArrayGroundFootings"));
		for (const FDish& D : Dishes) for (double SideX : {-1., 1.}) for (double SideZ : {-1., 1.})
		{
			const double X = D.X + SideX * 2.35 * D.Scale, Z = D.Z + SideZ * 2.35 * D.Scale;
			const double HalfWidth = .725 * D.Scale;
			double Bottom = S47GroundCover::SaroLand(X, Z);
			for (double Dx : {-HalfWidth, HalfWidth}) for (double Dz : {-HalfWidth, HalfWidth})
				Bottom = FMath::Min(Bottom, S47GroundCover::SaroLand(X + Dx, Z + Dz));
			Bottom -= .08; // Bury the bottom under the rendered land, including the pad's edge.
			const double Top = .02 * D.Scale;
			if (Bottom >= Top) continue;
			Footings->AddInstance(FTransform(FQuat::Identity,
				S47Three::ToUE(FVector(X, (Top + Bottom) * .5, Z)),
				FVector(HalfWidth * 2, HalfWidth * 2, Top - Bottom)));
		}
		UE_LOG(LogTemp, Display, TEXT("S47: array foundations seated: %d concrete extensions on terrain"), Footings->GetInstanceCount());
	}

	DrawDishes();
	UE_LOG(LogTemp, Display, TEXT("S47: original Blender array: %d antennas, 3 instanced assemblies, existing azimuth/elevation and beacons"), Dishes.Num());
}
