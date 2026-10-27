// Unreal-only authored desert dressing. Original road/walker/story surfaces remain authoritative.
#include "S47Web.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47GrassWind.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47CalicheNature, Log, All);

namespace
{
double OldRoadDistance(double X, double Z)
{
	double Best = TNumericLimits<double>::Max();
	const TArray<S47RoadTerrain::FStubPt>& Points = S47RoadTerrain::OldStub();
	for (int32 I = 0; I + 1 < Points.Num(); ++I)
	{
		const double Dx = Points[I+1].X - Points[I].X, Dz = Points[I+1].Z - Points[I].Z;
		const double T = FMath::Clamp(((X-Points[I].X)*Dx+(Z-Points[I].Z)*Dz)/(Dx*Dx+Dz*Dz), 0.0, 1.0);
		Best = FMath::Min(Best, FMath::Sqrt(FMath::Square(X-Points[I].X-T*Dx)+FMath::Square(Z-Points[I].Z-T*Dz)));
	}
	return Best;
}
}

void AS47Web::BuildCalicheNature()
{
	const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("S47CalicheNatureOff")) ||
		FParse::Param(FCommandLine::Get(), TEXT("S47LegacyCalicheStreet")) ||
		FParse::Param(FCommandLine::Get(), TEXT("S47PropsOff"));
	// Terrain overlays never become photo occluders, ground traces or walker colliders.
	for (int32 I = 0; I < Nodes.Num(); ++I)
		if (Nodes[I].Name.StartsWith(TEXT("prop:streetGroundNature")))
		{
			if (UPrimitiveComponent* C = Cast<UPrimitiveComponent>(Nodes[I].Comp.Get()))
			{
				C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				// The original terrain supplies land shadows. These 16mm surface
				// finishes receive light/shadows without casting duplicate seams.
				C->SetCastShadow(false);
			}
			if (bOff) SetVisible(I, false);
		}
	if (bOff) return;
	FString Text;
	TSharedPtr<FJsonObject> Document;
	if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectContentDir() / TEXT("CalicheStreet/Nature/nature-placement.json"))) ||
		!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Document) || !Document.IsValid())
		return; // A missing optional kit preserves the installed street and original ground.
	const int32 Parent = Handle(TEXT("world.town.group"));
	const TArray<TSharedPtr<FJsonValue>>* Families = nullptr;
	if (!Nodes.IsValidIndex(Parent) || !Document->TryGetArrayField(TEXT("families"), Families) || Families->Num() != 12)
	{
		UE_LOG(LogS47CalicheNature, Error, TEXT("S47: nature rejected: missing parent or twelve prototype families"));
		return;
	}
	struct FFamily
	{
		FString Id;
		UStaticMesh* Mesh = nullptr;
		TArray<FTransform> Instances;
		int32 Cull = 0;
		bool bShadow = true;
	};
	TArray<FFamily> Ready;
	TSet<FString> Ids;
	int32 Total = 0;
	bool bValid = true;
	const TArray<TSharedPtr<FJsonValue>>* Exclusions = nullptr;
	bValid &= Document->TryGetArrayField(TEXT("exclusions"), Exclusions);
	const TArray<TSharedPtr<FJsonValue>>* HeightProbes = nullptr;
	bValid &= Document->TryGetArrayField(TEXT("height_probes"), HeightProbes);
	if (HeightProbes) for (const TSharedPtr<FJsonValue>& Probe : *HeightProbes)
	{
		const TArray<TSharedPtr<FJsonValue>>& V = Probe->AsArray();
		if (V.Num() != 3) { bValid = false; break; }
		bValid &= FMath::Abs(S47RoadTerrain::SurfaceY(V[0]->AsNumber(), V[1]->AsNumber()) - V[2]->AsNumber()) < .0002;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Families)
	{
		const TSharedPtr<FJsonObject> Row = Value->AsObject();
		FFamily Family;
		FString Path;
		const TArray<TSharedPtr<FJsonValue>>* Placements = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Lods = nullptr;
		if (!Row.IsValid() || !Row->TryGetStringField(TEXT("id"), Family.Id) || Ids.Contains(Family.Id) ||
			!Row->TryGetStringField(TEXT("mesh"), Path) || !Path.StartsWith(TEXT("/Game/CalicheStreet/NatureVegetation/")) ||
			!Row->TryGetArrayField(TEXT("instances"), Placements) || !Row->TryGetArrayField(TEXT("lod_triangles"), Lods))
		{ bValid = false; break; }
		Ids.Add(Family.Id);
		Family.Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
		double Cull = 0;
		if (!Family.Mesh || !Family.Mesh->GetRenderData() || Lods->Num() != 3 ||
			Family.Mesh->GetRenderData()->LODResources.Num() != 3 ||
			!Row->TryGetNumberField(TEXT("cull_metres"), Cull) || Cull < 40 || Cull > 2000 || Placements->Num() > 20000)
		{ bValid = false; break; }
		// A static-mesh preview can pass while the cooked HISM draw falls back
		// to gray. Require the real instancing permutation before any scatter.
		for (const FStaticMaterial& Slot : Family.Mesh->GetStaticMaterials())
		{
			UMaterialInterface* Mat = Slot.MaterialInterface;
			const bool bHISM = Mat && Mat->GetUsageByFlag(MATUSAGE_InstancedStaticMeshes) &&
				Mat->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes);
			if (!bHISM)
				UE_LOG(LogS47CalicheNature, Error, TEXT("S47: nature rejected: HISM material usage %s"), *Family.Id);
			bValid &= bHISM;
		}
		Family.Cull = FMath::RoundToInt(Cull * 100);
		Row->TryGetBoolField(TEXT("shadow"), Family.bShadow);
		const double ExpectedScreens[3] = {1.0, .12, .025};
		for (int32 L = 0; L < 3; ++L)
		{
			bValid &= Family.Mesh->GetRenderData()->LODResources[L].GetNumTriangles() == int32((*Lods)[L]->AsNumber());
			// Check cooked render behavior directly. UE5.8 protects the editor's
			// deprecated auto-compute flag from Python; a private flag is not
			// needed to prove these actual runtime switching thresholds.
			bValid &= FMath::Abs(Family.Mesh->GetRenderData()->ScreenSize[L].GetValue() - ExpectedScreens[L]) < 1e-6;
		}
		for (const TSharedPtr<FJsonValue>& Placement : *Placements)
		{
			const TArray<TSharedPtr<FJsonValue>>& V = Placement->AsArray();
			if (V.Num() != 7) { bValid = false; break; }
			double P[7];
			for (int32 K = 0; K < 7; ++K) { P[K] = V[K]->AsNumber(); bValid &= FMath::IsFinite(P[K]); }
			const double X = P[0] - 8000, Z = P[2];
			if (!bValid || X < -600 || X > 600 || Z < 1650 || Z > 2450 ||
				P[4] < .2 || P[4] > 2.5 || P[5] < .2 || P[5] > 2.5 || P[6] < .2 || P[6] > 2.5 ||
				FMath::Abs(P[1] - S47RoadTerrain::SurfaceY(X, Z)) > .30)
			{ bValid = false; break; }
			const FBox Bounds = Family.Mesh->GetBoundingBox();
			const double Radius = FMath::Sqrt(FMath::Square(FMath::Max(FMath::Abs(Bounds.Min.X), FMath::Abs(Bounds.Max.X)) * P[4]) +
				FMath::Square(FMath::Max(FMath::Abs(Bounds.Min.Y), FMath::Abs(Bounds.Max.Y)) * P[5])) / 100.0;
			bValid &= FMath::Abs(X) >= 7 + Radius && OldRoadDistance(X, Z) >= 17 + Radius;
			if (Exclusions) for (const TSharedPtr<FJsonValue>& Exclusion : *Exclusions)
			{
				const TArray<TSharedPtr<FJsonValue>>& B = Exclusion->AsArray();
				if (B.Num() != 4) { bValid = false; break; }
				bValid &= !(X >= B[0]->AsNumber()-Radius && X <= B[1]->AsNumber()+Radius &&
					Z >= B[2]->AsNumber()-Radius && Z <= B[3]->AsNumber()+Radius);
			}
			if (!bValid) break;
			// Authored FBX axes map Blender(x,y,z) to UE(x,-y,z); around Z is native yaw.
			Family.Instances.Emplace(FRotator(0, P[3], 0), S47Three::ToUE(P[0], P[1], P[2]), FVector(P[4], P[5], P[6]));
		}
		Total += Family.Instances.Num();
		Ready.Add(MoveTemp(Family));
		if (!bValid) break;
	}
	if (!bValid || Ready.Num() != 12 || Total < 1000 || Total > 80000)
	{
		UE_LOG(LogS47CalicheNature, Error, TEXT("S47: nature rejected: prototype LOD, placement or protected route mismatch"));
		return;
	}
	// Unreal-only: sub-12cm contact gravel does not need a separate direct shadow.
	// Split only those small instances, preserving every larger stone and plant caster,
	// the existing model/LOD distances and RT visibility. No per-frame repartitioning.
	const bool bLegacyShadows = FParse::Param(FCommandLine::Get(), TEXT("S47LegacySmallShadows"));
	constexpr double TinyHeightCm = 12.0;
	int32 Components = 0, TinyShadowless = 0;
	for (const FFamily& Family : Ready)
	{
		TArray<FTransform> Normal, Tiny;
		const bool bSplit = !bLegacyShadows && Family.bShadow &&
			(Family.Id == TEXT("Caliche_B") || Family.Id == TEXT("Sandstone_A"));
		const double HeightCm = Family.Mesh->GetBoundingBox().GetSize().Z;
		for (const FTransform& Transform : Family.Instances)
			if (bSplit && HeightCm * FMath::Abs(Transform.GetScale3D().Z) < TinyHeightCm) Tiny.Add(Transform);
			else Normal.Add(Transform);
		auto BuildBatch = [&](const TArray<FTransform>& Instances, bool bTiny, bool bShadow)
		{
			if (Instances.IsEmpty()) return 0;
			const FString Id = Family.Id + (bTiny ? TEXT(":tiny") : TEXT(""));
			UHierarchicalInstancedStaticMeshComponent* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(this,
				*FString::Printf(TEXT("Nature_%s%s"), *Family.Id, bTiny ? TEXT("_Tiny") : TEXT("")));
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(Family.Mesh);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(bShadow);
			S47GrassWind::Apply(C, Family.Id);
			C->SetCullDistances(FMath::RoundToInt(Family.Cull * .82), Family.Cull);
			C->bAutoRebuildTreeOnInstanceChanges = false;
			C->RegisterComponent();
			C->AttachToComponent(Nodes[Parent].Comp.IsValid() ? Nodes[Parent].Comp.Get() : GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			// Keep the GPU instance offsets near Caliche, not 8km absolute translations.
			C->SetWorldTransform(FTransform(S47Three::ToUE(8000, 0, 2060)));
			AdoptNode(C, Parent, TEXT("nature:") + Id);
			for (const FTransform& Transform : Instances) C->AddInstance(Transform, true);
			C->BuildTreeIfOutdated(false, true);
			++Components;
			int32 StartCull = 0, EndCull = 0;
			C->GetCullDistances(StartCull, EndCull);
			bool bActual = C->GetInstanceCount() == Instances.Num() && EndCull == Family.Cull &&
				StartCull == FMath::RoundToInt(Family.Cull * .82) && C->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
				bool(C->CastShadow) == bShadow;
			// Check each batch after parent adoption and tree construction, including its final transform.
			for (int32 Index : {0, Instances.Num()-1})
			{
				FTransform Actual;
				bActual &= C->GetInstanceTransform(Index, Actual, true) && Actual.Equals(Instances[Index], .01);
			}
			UE_LOG(LogS47CalicheNature, Display, TEXT("S47: nature component %s: instances=%d cull_cm=%d,%d world_transform_checks=%s shadow=%d"),
				*Id, C->GetInstanceCount(), StartCull, EndCull, bActual ? TEXT("PASS") : TEXT("FAIL"), bShadow);
			if (!bActual) UE_LOG(LogS47CalicheNature, Error, TEXT("S47: nature runtime component mismatch: %s"), *Id);
			return C->GetInstanceCount();
		};
		const int32 NormalCount = BuildBatch(Normal, false, Family.bShadow);
		const int32 TinyCount = BuildBatch(Tiny, true, false);
		TinyShadowless += TinyCount;
		UE_LOG(LogS47CalicheNature, Display, TEXT("S47: nature family %s: authored=%d normal=%d tiny=%d summed_count_checks=%s"),
			*Family.Id, Family.Instances.Num(), NormalCount, TinyCount,
			NormalCount + TinyCount == Family.Instances.Num() ? TEXT("PASS") : TEXT("FAIL"));
		if (NormalCount + TinyCount != Family.Instances.Num())
			UE_LOG(LogS47CalicheNature, Error, TEXT("S47: nature family lost instances: %s"), *Family.Id);
		UE_LOG(LogS47CalicheNature, Display, TEXT("S47: nature cooked LOD screens %s: %.5f,%.5f,%.5f"), *Family.Id,
			Family.Mesh->GetRenderData()->ScreenSize[0].GetValue(), Family.Mesh->GetRenderData()->ScreenSize[1].GetValue(), Family.Mesh->GetRenderData()->ScreenSize[2].GetValue());
	}
	UE_LOG(LogS47CalicheNature, Display, TEXT("S47: small-object shadow policy: mode=%s threshold_cm=%.1f tiny_shadowless=%d"),
		bLegacyShadows ? TEXT("legacy") : TEXT("contact-gravel"), TinyHeightCm, TinyShadowless);
	UE_LOG(LogS47CalicheNature, Display, TEXT("S47: Caliche nature ready: %d authored instances in %d prototypes, %d HISM components, 3 LODs, no collision; road/story/photo exclusions preserved"), Total, Ready.Num(), Components);
}
