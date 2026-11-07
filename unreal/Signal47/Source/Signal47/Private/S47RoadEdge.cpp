#include "S47RoadEdge.h"
#include "S47FreeformTerrain.h"
#include "S47GrassWind.h"
#include "S47GroundCover.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "StaticMeshResources.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47RoadEdge, Log, All);

namespace
{
	const TCHAR* const EdgeMaterialPackage = TEXT("/Game/RoadEdge/M_S47_RoadEdge");
	const TCHAR* const EdgeMaterialPath = TEXT("/Game/RoadEdge/M_S47_RoadEdge.M_S47_RoadEdge");
	constexpr int32 EdgeSitePlate = 19;          // the highway through SARO's site: an 8 m plate, geometry 14
	constexpr int32 EdgeCorridorNode = 952;      // saroCorridor (S47FreeformTerrain ReliefCorridorNode), road-local z SARO_FROM..600
	constexpr double EdgeLiftRoad = .02, EdgeLiftLand = .03;   // over what it lies on, in metres: the depth test keeps it there and nowhere else
	constexpr double EdgeSiteX = -24, EdgeSiteY = -.58, EdgeSiteHalf = 4, EdgeSiteZ0 = -480, EdgeSiteZ1 = 36, EdgeSiteToRoadZ = -300;
	// The columns of roadTerrain.ts' strips: the asphalt's to 4.8, the shoulder's to 6, the verge's to 16.5. Nothing on the crown of the road.
	const double EdgeCols[] = { 2.4, 4.8, 6, 11.25, 16.5 };

	struct FEdgeState
	{
		TWeakObjectPtr<AS47Web> World;
		TWeakObjectPtr<UMaterialInstanceDynamic> Land, Corridor;
		TArray<int32> Nodes;
		bool bTried = false, bShown = false;
		int32 Vertices = 0, Triangles = 0, Instances = 0;
	};
	FEdgeState EdgeRun;

	struct FEdgeVertex { FVector Pos; FVector2f UV; FLinearColor Color; };
	struct FEdgeMesh { TArray<FEdgeVertex> V; TArray<uint32> Indices; };

	double EdgeSign(double X) { return X < 0 ? -1. : 1.; }
	// roadTerrain.ts tint: the desert's vertex colour, which the ground's material multiplies by.
	FLinearColor EdgeTint(double X, double Z)
	{
		const double N = .8 + .12 * FMath::Sin(X * .013) * FMath::Cos(Z * .011) + .16 * (S47RoadShape::Hash(FMath::RoundToDouble(X), FMath::RoundToDouble(Z)) - .5);
		const double Sand = .22 * S47RoadShape::WashAt(X, Z);
		return FLinearColor(float(N + Sand), float(N * .98 + Sand * .9), float(N * .95 + Sand * .75), 1.f);
	}
	double EdgeY(double X, double Z)
	{
		const double Ax = FMath::Abs(X);
		return S47RoadShape::HLow(X, Z) + (Ax <= S47RoadTerrain::HWY.Shoulder ? S47RoadShape::HwyLift(Ax) + EdgeLiftRoad : EdgeLiftLand);
	}
	// Where a lot, a sidewalk, the survey track, the old road or the ranch's road meets the highway the edge stands back: no dust over someone's apron.
	bool EdgeKeepRoad(double X, double Z)
	{
		const double Side = EdgeSign(X);
		return S47GroundCover::RoadSeam(Side * 5.9, Z) > 0 && S47GroundCover::RoadCover(Side * FMath::Max(FMath::Abs(X), 7.), Z) > 0;
	}
	bool EdgeKeepSite(double Dx, double Z)
	{
		const double Side = EdgeSign(Dx);
		return S47GroundCover::SiteCover(EdgeSiteX + Side * FMath::Max(FMath::Abs(Dx), 6.5), Z) > 0;
	}
	// roadTerrain.ts highwayPath's rows, and a row between each two 8 m apart so that a mouth fades over 4 m. A row between lies on the chord: the strip's own surface.
	TArray<double> EdgeHighwayRows(double Z0, double Z1)
	{
		TArray<double> Rows;
		for (double Z = Z0; Z <= Z1; Z += FMath::Abs(Z + 4 - S47RoadShape::DINER().Z) < 16 ? 1 : 8) Rows.Add(Z);
		return Rows;
	}
	void EdgeAddGrid(FEdgeMesh& M, int32 RowCount, int32 ColCount, TFunctionRef<FEdgeVertex(int32, int32)> At)
	{
		const uint32 Base = M.V.Num();
		for (int32 R = 0; R < RowCount; ++R) for (int32 C = 0; C < ColCount; ++C) M.V.Add(At(R, C));
		// Rows along +Z, columns along +X: up-facing in three's terms, and its swap to UE keeps them so (see S47FarmWorld GroundMesh).
		for (int32 R = 0; R + 1 < RowCount; ++R) for (int32 C = 0; C + 1 < ColCount; ++C)
		{
			const uint32 A = Base + R * ColCount + C, Next = A + 1, Over = A + ColCount, Far = Over + 1;
			M.Indices.Append({ A, Over, Next, Next, Over, Far });
		}
	}
	// Both sides of a stretch of the highway, in road-local metres. Columns run outwards; on the west side they are mirrored so x still rises across each grid.
	void EdgeAddHighway(FEdgeMesh& M, const TArray<double>& Rows)
	{
		const int32 Cols = UE_ARRAY_COUNT(EdgeCols);
		TArray<FVector2D> Spaced;   // z, and how far to the next exported row (0 on an exported row)
		for (int32 I = 0; I < Rows.Num(); ++I)
		{
			Spaced.Add(FVector2D(Rows[I], 0));
			if (I + 1 < Rows.Num() && Rows[I + 1] - Rows[I] >= 8) Spaced.Add(FVector2D((Rows[I] + Rows[I + 1]) / 2, Rows[I + 1] - Rows[I]));
		}
		for (const double Side : { -1., 1. })
		{
			EdgeAddGrid(M, Spaced.Num(), Cols, [&](int32 R, int32 C) -> FEdgeVertex
			{
				const double X = Side * EdgeCols[Side < 0 ? Cols - 1 - C : C], Z = Spaced[R].X, Half = Spaced[R].Y / 2;
				const double Y = Half > 0 ? (EdgeY(X, Z - Half) + EdgeY(X, Z + Half)) / 2 : EdgeY(X, Z);
				FLinearColor Tint = Half > 0 ? (EdgeTint(X, Z - Half) + EdgeTint(X, Z + Half)) * .5f : EdgeTint(X, Z);
				Tint.A = EdgeKeepRoad(X, Z) ? 1.f : 0.f;
				return { FVector(X, Y, Z), FVector2f(float(X), float(Z)), Tint };
			});
		}
	}
	// The site's own stretch is an 8 m plate with no shoulder, its ground (node 7) lower by some 5-10 cm. Its edge counts as the 4.8 m of the map, so the UVs read .8 m wider than it is.
	void EdgeAddSite(FEdgeMesh& M)
	{
		const int32 Cols = UE_ARRAY_COUNT(EdgeCols);
		const int32 RowCount = int32((EdgeSiteZ1 - EdgeSiteZ0) / 4) + 1;
		const double Widen = 4.8 - EdgeSiteHalf;
		for (const double Side : { -1., 1. })
		{
			EdgeAddGrid(M, RowCount, Cols, [&](int32 R, int32 C) -> FEdgeVertex
			{
				const double U = Side * EdgeCols[Side < 0 ? Cols - 1 - C : C], Z = EdgeSiteZ0 + R * 4.;
				const double Dx = Side * (FMath::Abs(U) - Widen);
				return { FVector(EdgeSiteX + Dx, EdgeSiteY + EdgeLiftLand, Z), FVector2f(float(U), float(Z + EdgeSiteToRoadZ)), FLinearColor(.81f, .81f, .81f, EdgeKeepSite(Dx, Z) ? 1.f : 0.f) };
			});
		}
	}
	UStaticMesh* MakeEdgeMesh(AS47Web* W, const FName Name, const FEdgeMesh& M, UMaterialInterface* Material)
	{
		const uint32 Count = M.V.Num();
		if (!Count || M.Indices.IsEmpty()) return nullptr;
		auto Data = MakeUnique<FStaticMeshRenderData>();
		Data->AllocateLODResources(1); Data->ScreenSize[0].Default = 1; Data->NumInlinedLODs = 1;
		FStaticMeshLODResources& L = Data->LODResources[0];
		L.VertexBuffers.PositionVertexBuffer.Init(Count, true);
		auto& T = L.VertexBuffers.StaticMeshVertexBuffer;
		T.SetUseFullPrecisionUVs(true);   // metres along a road five kilometres long
		T.Init(Count, 2, true);
		const FVector Up = S47Three::DirToUE(FVector(0, 1, 0)), Along = FVector::CrossProduct(FVector::RightVector, Up).GetSafeNormal(), Across = FVector::CrossProduct(Up, Along);
		TArray<FColor> Colors;
		Colors.Reserve(Count);
		FBox Bounds(ForceInit);
		for (uint32 I = 0; I < Count; ++I)
		{
			const FVector At = S47Three::ToUE(M.V[I].Pos);
			Bounds += At;
			L.VertexBuffers.PositionVertexBuffer.VertexPosition(I) = FVector3f(At);
			T.SetVertexTangents(I, FVector3f(Along), FVector3f(Across), FVector3f(Up));
			T.SetVertexUV(I, 0, M.V[I].UV);
			T.SetVertexUV(I, 1, FVector2f(0.f, 0.f));   // no sediment and no stone on the verge: the same as the ground's flat strips
			Colors.Add(M.V[I].Color.ToFColor(false));
		}
		L.VertexBuffers.ColorVertexBuffer.InitFromColorArray(Colors);
		FStaticMeshSection S;
		S.MaterialIndex = 0; S.FirstIndex = 0; S.NumTriangles = M.Indices.Num() / 3; S.MinVertexIndex = 0; S.MaxVertexIndex = Count - 1; S.bEnableCollision = false; S.bCastShadow = false;
		L.Sections.Add(S);
		L.bHasColorVertexData = true; L.bBuffersInlined = true; L.bIsOptionalLOD = false; L.MaxDeviation = 0; L.SourceMeshBounds = FBoxSphereBounds(Bounds);
		L.IndexBuffer.SetIndices(M.Indices, EIndexBufferStride::Force32Bit);
		auto* Mesh = NewObject<UStaticMesh>(W, Name, RF_Transient);
		Mesh->NeverStream = true; Mesh->bSupportRayTracing = false;
		FStaticMaterial Slot(Material);
		Slot.UVChannelData = FMeshUVChannelInfo(1.f);
		Mesh->GetStaticMaterials().Add(Slot);
		Data->Bounds = FBoxSphereBounds(Bounds); Data->CurrentFirstLODIdx = 0; Data->bReadyForStreaming = false;
		Mesh->SetRenderData(MoveTemp(Data));
		Mesh->SetExtendedBounds(FBoxSphereBounds(Bounds));
		Mesh->InitResources();
		EdgeRun.Vertices += Count; EdgeRun.Triangles += M.Indices.Num() / 3;
		return Mesh;
	}
	void EdgeAddRibbon(AS47Web* W, int32 Parent, const TCHAR* Name, const FEdgeMesh& M, UMaterialInterface* Material)
	{
		const FS47WebNode* Group = W->Node(Parent);
		USceneComponent* Under = Group ? Group->Comp.Get() : nullptr;
		UStaticMesh* Mesh = Under ? MakeEdgeMesh(W, FName(Name), M, Material) : nullptr;
		if (!Mesh) { UE_LOG(LogS47RoadEdge, Warning, TEXT("S47: road edge %s: no node %d or no mesh"), Name, Parent); return; }
		auto* C = NewObject<UStaticMeshComponent>(W);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Mesh);
		C->SetMaterial(0, Material);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetGenerateOverlapEvents(false);
		C->SetCanEverAffectNavigation(false);
		C->bUseAsOccluder = false; C->bVisibleInReflectionCaptures = false; C->bVisibleInRealTimeSkyCaptures = false; C->bVisibleInRayTracing = false;
		C->bAffectDistanceFieldLighting = false; C->bAffectDynamicIndirectLighting = false;
		C->SetupAttachment(Under);
		C->RegisterComponent();
		EdgeRun.Nodes.Add(W->AdoptNode(C, Parent, FString(TEXT("roadedge:")) + Name));
	}
	// Small stones and dry grass right up to the edge, thicker in some stretches than in others. Culled at 70-130 m, so a few dozen of them draw at a time.
	struct FEdgeKit { const TCHAR* Id; int32 Cull; double Density, MinX, MaxX, Scale; };
	const FEdgeKit EdgeKits[] = {
		// These stones are made to be half a metre across: at a sixth of that they read as gravel, not as boulders put down along the road.
		{ TEXT("Caliche_A"), 6000, .55, 5.1, 7.8, .15 },
		{ TEXT("Grass_A"), 13000, .13, 5.9, 8.6, .68 }, { TEXT("Grass_B"), 13000, .09, 6., 9.5, .72 } };
	void EdgeAddStones(AS47Web* W, int32 Parent, const TCHAR* Name, double Z0, double Z1)
	{
		const FS47WebNode* Group = W->Node(Parent);
		USceneComponent* Under = Group ? Group->Comp.Get() : nullptr;
		if (!Under) return;
		for (int32 Ki = 0; Ki < UE_ARRAY_COUNT(EdgeKits); ++Ki)
		{
			const FEdgeKit& K = EdgeKits[Ki];
			const FString MeshName = FString(TEXT("SM_CN_")) + K.Id;
			UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *(TEXT("/Game/CalicheStreet/NatureVegetation/") + MeshName + TEXT(".") + MeshName));
			if (!Mesh) continue;
			auto* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(W);
			C->SetMobility(EComponentMobility::Movable);
			C->SetStaticMesh(Mesh);
			C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			C->SetCastShadow(false);
			C->SetCullDistances(int32(K.Cull * .82), K.Cull);
			C->bAutoRebuildTreeOnInstanceChanges = false;
			S47GrassWind::Apply(C, K.Id);
			C->SetupAttachment(Under);
			C->RegisterComponent();
			FRandomStream Random(47260 + Ki * 131 + int32(Z0));
			int32 Count = 0;
			for (double Z = Z0; Z < Z1; Z += 1.4) for (const double Side : { -1., 1. })
			{
				const double Patches = FMath::Clamp(.75 + 1.1 * FMath::PerlinNoise1D(float(Z * .037 + Side * 31 + Ki * 7)), .05, 1.5);
				if (Random.FRand() > K.Density * Patches) continue;
				const double T = Random.FRand(), X = Side * FMath::Lerp(K.MinX, K.MaxX, T * T);
				const double Pz = Z + Random.FRandRange(-.7f, .7f);
				if (!EdgeKeepRoad(X, Pz)) continue;
				const double Ax = FMath::Abs(X);
				const double Y = S47RoadShape::HLow(X, Pz) + (Ax <= S47RoadTerrain::HWY.Shoulder ? S47RoadShape::HwyLift(Ax) : 0);
				const double Scale = K.Scale * Random.FRandRange(.65f, 1.25f);
				C->AddInstance(FTransform(FRotator(0, Random.FRandRange(0.f, 360.f), 0), S47Three::ToUE(X, Y, Pz), FVector(Scale)), false);
				++Count;
			}
			C->BuildTreeIfOutdated(false, true);
			EdgeRun.Instances += Count;
			EdgeRun.Nodes.Add(W->AdoptNode(C, Parent, FString::Printf(TEXT("roadedge:%s:%s"), Name, K.Id)));
		}
	}
	void EdgeBuild(AS47Web* W)
	{
		EdgeRun.bTried = true;
		if (FParse::Param(FCommandLine::Get(), TEXT("S47RoadEdgeOff"))) { UE_LOG(LogS47RoadEdge, Display, TEXT("S47: road edge off by -S47RoadEdgeOff")); return; }
		if (!FPackageName::DoesPackageExist(EdgeMaterialPackage)) { UE_LOG(LogS47RoadEdge, Display, TEXT("S47: road edge is not imported: the road stays as it was")); return; }
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, EdgeMaterialPath);
		const int32 RoadRoot = W->Handle(TEXT("world.legs.road.area.group")), Site = W->Handle(TEXT("ext.group"));
		if (!Material || RoadRoot < 0) { UE_LOG(LogS47RoadEdge, Warning, TEXT("S47: road edge: no material or no road node")); return; }
		const auto NewMid = [&]() -> UMaterialInstanceDynamic*
		{
			auto* Mid = UMaterialInstanceDynamic::Create(Material, W);
			// The same reason as the ground's (S47FreeformTerrain): the low mips of these sets read far paler than the full texture.
			Mid->SetForceMipLevelsToBeResident(true, true, -1.f, 0, true);
			if (UMaterialInstanceDynamic* Ground = S47FreeformTerrain::GroundMid(W)) Mid->CopyParameterOverrides(Ground);
			return Mid;
		};
		EdgeRun.Land = NewMid(); EdgeRun.Corridor = NewMid();
		const double Started = FPlatformTime::Seconds();
		{
			FEdgeMesh M;
			EdgeAddHighway(M, EdgeHighwayRows(-2000, 3400));
			EdgeAddRibbon(W, RoadRoot, TEXT("S47RoadEdgeRoad"), M, EdgeRun.Land.Get());
		}
		EdgeAddStones(W, RoadRoot, TEXT("road"), -2000, 3396);
		if (W->Node(EdgeCorridorNode))
		{
			FEdgeMesh M;
			EdgeAddHighway(M, EdgeHighwayRows(S47RoadTerrain::SARO_FROM, 600));
			EdgeAddRibbon(W, EdgeCorridorNode, TEXT("S47RoadEdgeCorridor"), M, EdgeRun.Corridor.Get());
			EdgeAddStones(W, EdgeCorridorNode, TEXT("corridor"), S47RoadTerrain::SARO_FROM, 600);
		}
		if (Site >= 0)
		{
			FEdgeMesh M;
			EdgeAddSite(M);
			EdgeAddRibbon(W, Site, TEXT("S47RoadEdgeSite"), M, EdgeRun.Land.Get());
		}
		UE_LOG(LogS47RoadEdge, Display, TEXT("S47: road edge built in %.1f ms: %d vertices, %d triangles, %d instances, %d parts"), (FPlatformTime::Seconds() - Started) * 1000, EdgeRun.Vertices, EdgeRun.Triangles, EdgeRun.Instances, EdgeRun.Nodes.Num());
	}
	// The ground's day and night (S47MesaRock::ApplyFreeformGround) go on this as they go on the ground, so the soil stays the same soil.
	void EdgeTone(AS47Web* W)
	{
		const UMaterialInstanceDynamic* Ground = S47FreeformTerrain::GroundMid(W);
		if (!Ground) return;
		for (const TWeakObjectPtr<UMaterialInstanceDynamic>& Weak : { EdgeRun.Land, EdgeRun.Corridor })
		{
			UMaterialInstanceDynamic* Mid = Weak.Get();
			if (!Mid) continue;
			for (const TCHAR* Name : { TEXT("GroundTint"), TEXT("CrustTint"), TEXT("GravelTint") })
			{
				FLinearColor Want, Have;
				if (Ground->GetVectorParameterValue(Name, Want) && (!Mid->GetVectorParameterValue(Name, Have) || !Have.Equals(Want, .001f))) Mid->SetVectorParameterValue(Name, Want);
			}
			for (const TCHAR* Name : { TEXT("GravelAmount"), TEXT("CrustAmount") })
			{
				float Want = 0, Have = 0;
				if (Ground->GetScalarParameterValue(Name, Want) && (!Mid->GetScalarParameterValue(Name, Have) || !FMath::IsNearlyEqual(Have, Want, .001f))) Mid->SetScalarParameterValue(Name, Want);
			}
		}
	}
}

namespace S47RoadEdge
{
	void Reconcile(AS47Web* W, bool bOn)
	{
		if (!W) return;
		if (EdgeRun.World.Get() != W) { EdgeRun = FEdgeState(); EdgeRun.World = W; }
		bOn = bOn && S47FreeformTerrain::IsOn(W) && W->HighwayLandscapeReady();
		if (bOn && !EdgeRun.bTried) EdgeBuild(W);
		if (EdgeRun.Nodes.IsEmpty()) return;
		if (bOn != EdgeRun.bShown)
		{
			EdgeRun.bShown = bOn;
			for (const int32 Node : EdgeRun.Nodes) W->SetVisible(Node, bOn);
			// The site's plate lies 4-9 cm over its ground and draws a black line along itself with its own shadow, however the soil meets it. Back in Story.
			if (const FS47WebNode* Plate = W->Node(EdgeSitePlate)) if (auto* C = Cast<UPrimitiveComponent>(Plate->Comp.Get())) C->SetCastShadow(!bOn);
		}
		if (bOn) EdgeTone(W);
	}
	void Report(const AS47Web* W, TArray<FString>& Lines)
	{
		if (EdgeRun.World.Get() != W) { Lines.Add(TEXT("road edge: not built")); return; }
		Lines.Add(FString::Printf(TEXT("road edge: shown %d, %d parts, %d vertices, %d triangles, %d instances"), EdgeRun.bShown, EdgeRun.Nodes.Num(), EdgeRun.Vertices, EdgeRun.Triangles, EdgeRun.Instances));
	}
}
