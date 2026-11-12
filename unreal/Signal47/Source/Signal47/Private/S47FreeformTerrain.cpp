#include "S47FreeformTerrain.h"
#include "S47GroundEdits.h"
#include "S47FarmWorld.h"
#include "S47Geo.h"
#include "S47Main.h"
#include "S47OpenLand.h"
#include "S47Relief.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformTime.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "StaticMeshResources.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47FreeformTerrain, Log, All);

// Held by the world (AS47Web::FreeformTerrain), so it and the meshes it keeps alive go when the world goes.
struct FS47FreeformTerrainState
{
	struct FGroundSwap { TWeakObjectPtr<UStaticMeshComponent> C; TStrongObjectPtr<UStaticMesh> Flat, Relief; TArray<TStrongObjectPtr<UMaterialInterface>> FlatMaterials; };
	struct FInstanceRow { TWeakObjectPtr<UInstancedStaticMeshComponent> C; TArray<int32> Index; TArray<FTransform> Flat, Lifted; };
	struct FPropLift { TWeakObjectPtr<UStaticMeshComponent> C; FTransform Flat; double Dy = 0; };
	struct FWireSwap { TWeakObjectPtr<UStaticMeshComponent> C; TStrongObjectPtr<UStaticMesh> Flat, Lifted; };
	// The mesas' own component takes the range: S47Sky, S47HighwayLandscape and its transform stay as they are.
	struct FRangeSwap { bool bMesaShadow=true; TWeakObjectPtr<UStaticMeshComponent> C; TStrongObjectPtr<UStaticMesh> Mesas, Range; TArray<TStrongObjectPtr<UMaterialInterface>> MesaMaterials; };
	TArray<FGroundSwap> Grounds;
	TArray<FInstanceRow> Rows;
	TArray<FPropLift> Props;
	TArray<FWireSwap> Wires;
	FRangeSwap Range;
	TStrongObjectPtr<UMaterialInstanceDynamic> Mid, RangeMid, CorridorMid;
	TWeakObjectPtr<UStaticMeshComponent> Corridor;
	TArray<TStrongObjectPtr<UMaterialInterface>> CorridorFlat;
	TArray<FString> Notes;
	int32 Instances = 0, Segments = 0;
	bool bPrepared = false, bFailed = false, bOn = false;
};

namespace
{
	const TCHAR* const ReliefRoadPath = TEXT("/Game/FreeformTerrain/SM_FF_Ground_Road.SM_FF_Ground_Road");
	const TCHAR* const ReliefSaroPath = TEXT("/Game/FreeformTerrain/SM_FF_Ground_Saro.SM_FF_Ground_Saro");
	const TCHAR* const ReliefMaterialPath = TEXT("/Game/FreeformTerrain/M_S47_FreeformGround.M_S47_FreeformGround");
	const TCHAR* const RangeMeshPath = TEXT("/Game/FreeformTerrain/SM_FF_Range.SM_FF_Range");
	const TCHAR* const RangeMaterialPath = TEXT("/Game/FreeformTerrain/M_S47_FreeformRange.M_S47_FreeformRange");
	const int32 ReliefRoadNode = 1242, ReliefRoadGeo = 873, ReliefSaroNode = 7, ReliefSaroGeo = 4, ReliefCorridorNode = 952, ReliefCorridorGroundNode = 953;
	constexpr double ReliefMinMove = .002;   // less than this is not worth the update: 0 in the mask, and nothing to see

	// Which set of coordinates a node stands in. The shared landscape follows the area; the others stay where they are.
	enum class EReliefFrame : uint8 { None, Shared, Saro, Road };
	EReliefFrame ReliefFrameOf(const AS47Web* W, int32 Node)
	{
		if (W->IsHighwayShared(Node)) return EReliefFrame::Shared;
		const int32 Ext = W->Handle(TEXT("ext.group")), RoadRoot = W->Handle(TEXT("world.legs.road.area.group"));
		for (int32 I = Node; I >= 0;)
		{
			const FS47WebNode* N = W->Node(I);
			if (!N) break;
			if (I == RoadRoot) return EReliefFrame::Road;
			if (I == Ext || I == ReliefCorridorNode) return EReliefFrame::Saro;
			I = N->Parent;
		}
		return EReliefFrame::None;   // indoors, the station's own ground, the old road: nothing of the table's
	}
	bool ReliefRoadXZ(const AS47Web* W, EReliefFrame Frame, const FVector& WorldUE, double& OutX, double& OutZ)
	{
		FVector P = S47Three::ToThree(WorldUE);
		if (Frame == EReliefFrame::Shared) { P = W->HighwayMapEye(P); P.X -= 8000; }
		else if (Frame == EReliefFrame::Road) P.X -= 8000;
		else if (Frame == EReliefFrame::Saro) { P.X += S47Geo::SARO_IN_ROAD.X; P.Z += S47Geo::SARO_IN_ROAD.Z; }
		else return false;
		OutX = P.X; OutZ = P.Z;
		return true;
	}
	// A wire is a chain of cylinders whose ends meet: each end goes by the table under it, and the piece turns to reach them,
	// so the chain stays whole. (BasicShapes' cylinder: 100 units tall on Z, its pivot in the middle; see S47HighwayPower Span.)
	bool LiftSegment(const AS47Web* W, EReliefFrame Frame, const FTransform& CompT, const FVector& UpLocal, const FTransform& Local, FTransform& Lifted)
	{
		const FVector A = Local.TransformPosition(FVector(0, 0, -50)), B = Local.TransformPosition(FVector(0, 0, 50));
		double Ax = 0, Az = 0, Bx = 0, Bz = 0;
		if (!ReliefRoadXZ(W, Frame, CompT.TransformPosition(A), Ax, Az) || !ReliefRoadXZ(W, Frame, CompT.TransformPosition(B), Bx, Bz)) return false;
		const double DyA = S47Relief::Sample(Ax, Az), DyB = S47Relief::Sample(Bx, Bz);
		if (FMath::Abs(DyA) < ReliefMinMove && FMath::Abs(DyB) < ReliefMinMove) return false;
		const FVector NewA = A + UpLocal * DyA, NewB = B + UpLocal * DyB, Along = NewB - NewA;
		const double Length = Along.Size();
		if (Length < 1) return false;
		const FVector Scale = Local.GetScale3D();
		Lifted = FTransform(FRotationMatrix::MakeFromZ(Along / Length).ToQuat(), (NewA + NewB) * .5, FVector(Scale.X, Scale.Y, Length / 100));
		return true;
	}
	void ScanInstances(AS47Web* W, FS47FreeformTerrainState& S)
	{
		for (int32 I = 0; I < W->PartCount(); ++I)
		{
			const FS47WebNode* N = W->Node(I);
			auto* C = N ? Cast<UInstancedStaticMeshComponent>(N->Comp.Get()) : nullptr;
			if (!C || !C->GetStaticMesh()) continue;
			// Flora makes its cells again on the live ground; the herd and the tumbleweeds read it as they go.
			if (N->Name.StartsWith(TEXT("flora")) || N->Name.StartsWith(TEXT("herd:")) || N->Name.StartsWith(TEXT("life:"))) continue;
			const EReliefFrame Frame = ReliefFrameOf(W, I);
			if (Frame == EReliefFrame::None) continue;
			const FTransform CompT = C->GetComponentTransform();
			const FVector UpLocal = CompT.InverseTransformVector(FVector(0, 0, 100));   // a metre up, in the component's own terms
			const bool bSegments = C->GetStaticMesh()->GetPathName() == TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
			FS47FreeformTerrainState::FInstanceRow Row;
			Row.C = C;
			for (int32 K = 0; K < C->GetInstanceCount(); ++K)
			{
				FTransform Local;
				if (!C->GetInstanceTransform(K, Local, false)) continue;
				FTransform Lifted = Local;
				if (bSegments)
				{
					if (!LiftSegment(W, Frame, CompT, UpLocal, Local, Lifted)) continue;
					++S.Segments;
				}
				else
				{
					double X = 0, Z = 0;
					if (!ReliefRoadXZ(W, Frame, CompT.TransformPosition(Local.GetLocation()), X, Z)) continue;
					const double Dy = S47Relief::Sample(X, Z);
					if (FMath::Abs(Dy) < ReliefMinMove) continue;
					Lifted.AddToTranslation(UpLocal * Dy);
				}
				Row.Index.Add(K); Row.Flat.Add(Local); Row.Lifted.Add(Lifted);
			}
			if (Row.Index.Num()) { S.Instances += Row.Index.Num(); S.Rows.Add(MoveTemp(Row)); }
		}
	}
	// One whole mesh placed with "at" (Config/Props). It goes up by what the table says under it, if the table says about the
	// same all over it: a big group that straddles a mask stays put, and is named in the log rather than moved by a guess.
	void ScanProps(AS47Web* W, FS47FreeformTerrainState& S)
	{
		int32 Straddling = 0;
		for (int32 I = 0; I < W->PartCount(); ++I)
		{
			const FS47WebNode* N = W->Node(I);
			if (!N || !N->Name.StartsWith(TEXT("prop:")) || W->IsHighwayShared(I)) continue;
			auto* C = Cast<UStaticMeshComponent>(N->Comp.Get());
			if (!C || !C->GetStaticMesh() || Cast<UInstancedStaticMeshComponent>(C)) continue;
			const EReliefFrame Frame = ReliefFrameOf(W, I);
			if (Frame == EReliefFrame::None) continue;
			const FBox Box = C->Bounds.GetBox();
			double Least = 1e30, Most = -1e30, Middle = 0;
			for (int32 K = 0; K < 5; ++K)
			{
				const FVector At = K == 0 ? Box.GetCenter() : FVector(K & 1 ? Box.Min.X : Box.Max.X, K & 2 ? Box.Min.Y : Box.Max.Y, Box.Min.Z);
				double X = 0, Z = 0;
				if (!ReliefRoadXZ(W, Frame, At, X, Z)) continue;
				const double Dy = S47Relief::Sample(X, Z);
				if (K == 0) Middle = Dy;
				Least = FMath::Min(Least, Dy); Most = FMath::Max(Most, Dy);
			}
			if (FMath::Max(FMath::Abs(Least), FMath::Abs(Most)) < ReliefMinMove) continue;
			if (Box.GetExtent().Size2D() > 8000 || FMath::Sign(Least) != FMath::Sign(Most) || FMath::Abs(Least) < ReliefMinMove || FMath::Abs(Most) < ReliefMinMove)
			{
				++Straddling;
				UE_LOG(LogS47FreeformTerrain, Display, TEXT("S47: relief leaves %s where it is: %.2f to %.2f m under it"), *N->Name, Least, Most);
				continue;
			}
			FS47FreeformTerrainState::FPropLift Lift;
			Lift.C = C; Lift.Flat = C->GetRelativeTransform(); Lift.Dy = Middle;
			S.Props.Add(Lift);
		}
		if (Straddling) S.Notes.Add(FString::Printf(TEXT("%d whole props straddle a mask and stay where they are"), Straddling));
	}
	// g684 and g918, the fence wires as single meshes, have every vertex in road-local metres whatever their parents are
	// (S47HighwayPower). The same copy S47HighwayPower makes of them, with each vertex lifted by the table under it.
	UStaticMesh* LiftedCopy(AS47Web* W, UStaticMesh* Source, int32& Moved)
	{
		Moved = 0;
		if (!Source || !Source->bAllowCPUAccess || !Source->GetRenderData() || Source->GetRenderData()->LODResources.Num() != 1) return nullptr;
		const FStaticMeshLODResources& From = Source->GetRenderData()->LODResources[0];
		const FStaticMeshVertexBuffers& B = From.VertexBuffers;
		const uint32 Count = B.PositionVertexBuffer.GetNumVertices();
		if (!Count || !B.PositionVertexBuffer.GetVertexData() || !B.StaticMeshVertexBuffer.GetTangentData() || !B.StaticMeshVertexBuffer.GetTexCoordData() || From.IndexBuffer.GetIndexDataSize() == 0) return nullptr;
		TArray<FVector3f> Lifted;
		Lifted.Reserve(Count);
		FBox Bounds(ForceInit);
		for (uint32 I = 0; I < Count; ++I)
		{
			const FVector At(B.PositionVertexBuffer.VertexPosition(I));
			const FVector Three = S47Three::ToThree(At);
			const double Dy = S47Relief::Sample(Three.X, Three.Z);
			if (FMath::Abs(Dy) >= ReliefMinMove) ++Moved;
			const FVector Raised = At + FVector(0, 0, Dy * 100);
			Lifted.Add(FVector3f(Raised));
			Bounds += Raised;
		}
		TArray<uint32> Source3;
		From.IndexBuffer.GetCopy(Source3);
		// Freeform's open land (S47OpenLand): of the highway's and the pasture's fences only a short stretch here and there keeps its wire.
		TArray<uint32> Indices;
		Indices.Reserve(Source3.Num());
		TArray<FStaticMeshSection> Sections;
		int32 Cut = 0;
		const auto ThreeOf = [&](uint32 V) { return S47Three::ToThree(FVector(B.PositionVertexBuffer.VertexPosition(V))); };
		for (const FStaticMeshSection& Section : From.Sections)
		{
			FStaticMeshSection Part = Section;
			Part.FirstIndex = Indices.Num(); Part.NumTriangles = 0;
			for (uint32 I = Section.FirstIndex; I + 2 < uint32(Source3.Num()) && I < Section.FirstIndex + Section.NumTriangles * 3; I += 3)
			{
				const FVector P0 = ThreeOf(Source3[I]), P1 = ThreeOf(Source3[I + 1]), P2 = ThreeOf(Source3[I + 2]);
				const bool bOnFence = S47OpenLand::OnRoadFence(P0.X, P0.Z) && S47OpenLand::OnRoadFence(P1.X, P1.Z) && S47OpenLand::OnRoadFence(P2.X, P2.Z);
				if (!S47OpenLand::Off() && bOnFence && !(S47OpenLand::FenceWireStays(P0.X, P0.Z) && S47OpenLand::FenceWireStays(P1.X, P1.Z) && S47OpenLand::FenceWireStays(P2.X, P2.Z))) { ++Cut; continue; }
				Indices.Append({ Source3[I], Source3[I + 1], Source3[I + 2] });
				++Part.NumTriangles;
			}
			if (Part.NumTriangles) Sections.Add(Part);
		}
		if (!Moved && !Cut) return nullptr;
		if (Indices.IsEmpty()) return nullptr;
		UE_LOG(LogS47FreeformTerrain, Display, TEXT("S47: wires: %d triangles of the old fences cut, %d left"), Cut, Indices.Num() / 3);
		UStaticMesh* Copy = NewObject<UStaticMesh>(W, NAME_None, RF_Transient);
		Copy->bAllowCPUAccess = true; Copy->NeverStream = true; Copy->bSupportRayTracing = false;
		Copy->GetStaticMaterials() = Source->GetStaticMaterials();
		auto Data = MakeUnique<FStaticMeshRenderData>();
		Data->AllocateLODResources(1);
		Data->Bounds = FBoxSphereBounds(Bounds); Data->ScreenSize[0].Default = 1; Data->NumInlinedLODs = 1; Data->CurrentFirstLODIdx = 0; Data->bReadyForStreaming = false;
		FStaticMeshLODResources& To = Data->LODResources[0];
		To.Sections.Append(Sections);
		To.VertexBuffers.PositionVertexBuffer.Init(Lifted, true);
		auto& Tangents = To.VertexBuffers.StaticMeshVertexBuffer;
		Tangents.SetUseHighPrecisionTangentBasis(B.StaticMeshVertexBuffer.GetUseHighPrecisionTangentBasis());
		Tangents.SetUseFullPrecisionUVs(B.StaticMeshVertexBuffer.GetUseFullPrecisionUVs());
		Tangents.Init(B.StaticMeshVertexBuffer.GetNumVertices(), B.StaticMeshVertexBuffer.GetNumTexCoords(), true);
		for (uint32 I = 0; I < B.StaticMeshVertexBuffer.GetNumVertices(); ++I)
		{
			Tangents.SetVertexTangents(I, FVector3f(B.StaticMeshVertexBuffer.VertexTangentX(I)), B.StaticMeshVertexBuffer.VertexTangentY(I), FVector3f(B.StaticMeshVertexBuffer.VertexTangentZ(I)));
			for (uint32 U = 0; U < B.StaticMeshVertexBuffer.GetNumTexCoords(); ++U) Tangents.SetVertexUV(I, U, B.StaticMeshVertexBuffer.GetVertexUV(I, U));
		}
		const uint32 Colors = B.ColorVertexBuffer.GetNumVertices();
		if (Colors)
		{
			if (!B.ColorVertexBuffer.GetVertexData()) return nullptr;
			To.VertexBuffers.ColorVertexBuffer.Init(Colors, true);
			for (uint32 I = 0; I < Colors; ++I) To.VertexBuffers.ColorVertexBuffer.VertexColor(I) = B.ColorVertexBuffer.VertexColor(I);
		}
		To.bHasColorVertexData = Colors > 0; To.bBuffersInlined = true; To.bIsOptionalLOD = false;
		To.SourceMeshBounds = Data->Bounds; To.MaxDeviation = 0;
		To.IndexBuffer.SetIndices(Indices, EIndexBufferStride::Force32Bit);
		Copy->SetRenderData(MoveTemp(Data));
		Copy->SetExtendedBounds(FBoxSphereBounds(Bounds));
		Copy->InitResources();
		return Copy;
	}
	void ScanWires(AS47Web* W, FS47FreeformTerrainState& S)
	{
		for (int32 I = 0; I < W->PartCount(); ++I)
		{
			const FS47WebNode* N = W->Node(I);
			if (!N) continue;
			auto* C = Cast<UStaticMeshComponent>(N->Comp.Get());
			if (!C || Cast<UInstancedStaticMeshComponent>(C)) continue;
			if (N->Geo == 684 || N->Geo == 918)
			{
				UStaticMesh* Source = C->GetStaticMesh();
				int32 Moved = 0;
				UStaticMesh* Lifted = LiftedCopy(W, Source, Moved);
				if (Lifted)
				{
					FS47FreeformTerrainState::FWireSwap Entry;
					Entry.C = C; Entry.Flat.Reset(Source); Entry.Lifted.Reset(Lifted);
					S.Wires.Add(MoveTemp(Entry));
					S.Notes.Add(FString::Printf(TEXT("g%d wires follow the table at %d vertices"), N->Geo, Moved));
				}
				else if (Moved == 0 && Source && Source->bAllowCPUAccess) continue;
				else S.Notes.Add(FString::Printf(TEXT("g%d wires could not be copied (no CPU access) and stay where they were"), N->Geo));
			}
			else if (N->Geo >= 910 && N->Geo <= 914)
			{
				// The ranch's own meshes belong to S47HighwayRanch, which swaps and fingerprints them. They are measured, not lifted.
				const UStaticMesh* Mesh = C->GetStaticMesh();
				const FStaticMeshRenderData* Data = Mesh && Mesh->bAllowCPUAccess ? Mesh->GetRenderData() : nullptr;
				if (!Data || Data->LODResources.Num() < 1 || !Data->LODResources[0].VertexBuffers.PositionVertexBuffer.GetVertexData()) continue;
				const FPositionVertexBuffer& Positions = Data->LODResources[0].VertexBuffers.PositionVertexBuffer;
				double Largest = 0;
				for (uint32 V = 0; V < Positions.GetNumVertices(); ++V)
				{
					const FVector Three = S47Three::ToThree(FVector(Positions.VertexPosition(V)));
					Largest = FMath::Max(Largest, FMath::Abs(S47Relief::Sample(Three.X, Three.Z)));
				}
				if (Largest > .05) S.Notes.Add(FString::Printf(TEXT("g%d, a ranch mesh, stays fixed over up to %.2f m of table: the table should be 0 under it"), N->Geo, Largest));
			}
		}
	}
	// The real hills in place of the mesas, if they are installed. Without them the mesas stay, with one line in the log.
	void PrepareRange(AS47Web* W, FS47FreeformTerrainState& S)
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("S47RangeOff"))) { S.Notes.Add(TEXT("-S47RangeOff: the mesas stay")); return; }
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, RangeMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, RangeMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Mesh || !Material) { S.Notes.Add(TEXT("the range is not installed: the mesas stay")); return; }
		for (int32 I = 0; I < W->PartCount(); ++I)
		{
			const FS47WebNode* N = W->Node(I);
			if (!N || N->Name != TEXT("prop:streetGroundMesas")) continue;
			auto* C = Cast<UStaticMeshComponent>(N->Comp.Get());
			if (!C || !C->GetStaticMesh()) continue;
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, GetTransientPackage());
			if (!Mid) { S.Notes.Add(TEXT("the range's material would not instance: the mesas stay")); return; }
			Mid->SetForceMipLevelsToBeResident(true, true, -1.f);
			S.RangeMid.Reset(Mid);
			S.Range.C = C; S.Range.Mesas.Reset(C->GetStaticMesh()); S.Range.Range.Reset(Mesh);
			return;
		}
		S.Notes.Add(TEXT("no prop:streetGroundMesas in this world: no range"));
	}
	void ApplyRange(FS47FreeformTerrainState& S, bool bOn)
	{
		FS47FreeformTerrainState::FRangeSwap& R = S.Range;
		UStaticMeshComponent* C = R.C.Get();
		if (!C) return;
		if (bOn)
		{
			// What S47MesaRock has given the mesas so far comes back in Story.
			R.MesaMaterials.Reset();
			for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot) R.MesaMaterials.Emplace(C->GetMaterial(Slot));
			C->SetStaticMesh(R.Range.Get());
			C->SetMaterial(0, S.RangeMid.Get());
			// A 24 km range would throw its shadow across the whole plain; hills keep their shading, and the land its sun.
			R.bMesaShadow = C->CastShadow;
			C->SetCastShadow(false);
		}
		else
		{
			C->SetStaticMesh(R.Mesas.Get());
			C->SetCastShadow(R.bMesaShadow);
			for (int32 Slot = 0; Slot < R.MesaMaterials.Num(); ++Slot) C->SetMaterial(Slot, R.MesaMaterials[Slot].Get());
		}
	}
	bool PrepareRelief(AS47Web* W, FS47FreeformTerrainState& S)
	{
		const auto Fail = [&](const FString& Why)
		{
			S.bFailed = true;
			UE_LOG(LogS47FreeformTerrain, Display, TEXT("S47: Freeform's land stays flat: %s"), *Why);
			return false;
		};
		if (FParse::Param(FCommandLine::Get(), TEXT("S47ReliefOff"))) return Fail(TEXT("-S47ReliefOff"));
		if (!S47Relief::Ready()) return Fail(TEXT("no usable relief table"));
		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, ReliefMaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
		if (!Material) return Fail(TEXT("the relief material is not installed"));
		const struct { int32 Node, Geo; const TCHAR* Path; } Wanted[] = { { ReliefRoadNode, ReliefRoadGeo, ReliefRoadPath }, { ReliefSaroNode, ReliefSaroGeo, ReliefSaroPath } };
		TArray<FS47FreeformTerrainState::FGroundSwap> Swaps;
		for (const auto& Want : Wanted)
		{
			const FS47WebNode* N = W->Node(Want.Node);
			auto* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
			if (!C || N->Geo != Want.Geo || !C->GetStaticMesh()) return Fail(FString::Printf(TEXT("node %d is not the ground it should be"), Want.Node));
			UStaticMesh* Relief = LoadObject<UStaticMesh>(nullptr, Want.Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
			if (!Relief) return Fail(FString::Printf(TEXT("%s is not installed"), Want.Path));
			if (!Relief->GetBodySetup()) UE_LOG(LogS47FreeformTerrain, Warning, TEXT("S47: %s has no collision: the walker's void guard will refuse it"), Want.Path);
			FS47FreeformTerrainState::FGroundSwap Entry;
			Entry.C = C; Entry.Flat.Reset(C->GetStaticMesh()); Entry.Relief.Reset(Relief);
			Swaps.Add(MoveTemp(Entry));
		}
		UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Material, GetTransientPackage());
		if (!Mid) return Fail(TEXT("the relief material would not instance"));
		// The low mips of these sets read far paler than the full texture (see S47MesaRock): a game opened to a white yard.
		Mid->SetForceMipLevelsToBeResident(true, true, -1.f);
		S.Mid.Reset(Mid);
		S.Grounds = MoveTemp(Swaps);
		// Tom, 7 November: "overlapp mellom to forskjellige teksturer". The SARO corridor's flat ground (node 953) is no relief mesh, so it stayed on M_S47_DesertGround, another
		// set of layers and tints, with a straight edge along the farm at road x 107.66 and along the whole strip. It takes the relief's material now. It has no sediment in its UV1
		// and no vertex colours: PlainMesh stands for both, with the shade the ring and the yard carry (MakeYardGround).
		if (const FS47WebNode* CorridorNode = W->Node(ReliefCorridorGroundNode))
			if (auto* CorridorComp = Cast<UStaticMeshComponent>(CorridorNode->Comp.Get()); CorridorComp && CorridorNode->Name == TEXT("saroCorridorGround"))
				if (UMaterialInstanceDynamic* CorridorMid = UMaterialInstanceDynamic::Create(Material, GetTransientPackage()))
				{
					CorridorMid->SetScalarParameterValue(TEXT("PlainMesh"), 1.f);
					CorridorMid->SetForceMipLevelsToBeResident(true, true, -1.f, 0, true);
					S.CorridorMid.Reset(CorridorMid);
					S.Corridor = CorridorComp;
				}
		const double Started = FPlatformTime::Seconds();
		ScanInstances(W, S);
		ScanProps(W, S);
		ScanWires(W, S);
		PrepareRange(W, S);
		TArray<FString> Zones;
		if (!S47Relief::CheckZones(Zones)) for (const FString& Line : Zones) if (Line.StartsWith(TEXT("FAIL"))) UE_LOG(LogS47FreeformTerrain, Warning, TEXT("S47: %s"), *Line);
		UE_LOG(LogS47FreeformTerrain, Display, TEXT("S47: Freeform's land ready: %d instances (%d wire pieces) and %d props to lift, measured in %.1f ms"), S.Instances, S.Segments, S.Props.Num(), (FPlatformTime::Seconds() - Started) * 1000);
		for (const FString& Note : S.Notes) UE_LOG(LogS47FreeformTerrain, Display, TEXT("S47: relief: %s"), *Note);
		S.bPrepared = true;
		return true;
	}
	void SwapMeshKeepingMaterials(UStaticMeshComponent* C, UStaticMesh* Mesh)
	{
		const TArray<UMaterialInterface*> Materials = C->GetMaterials();
		C->SetStaticMesh(Mesh);
		for (int32 Slot = 0; Slot < Materials.Num(); ++Slot) C->SetMaterial(Slot, Materials[Slot]);
	}
	// Caliche's tiles of mineral soil lie 16 mm over the old ground: where the land has moved they float or sink under it, so
	// those keep out of the way. In town, where it has not moved, they stay.
	void HideOldSoil(AS47Web* W, bool bOn)
	{
		for (int32 Tile = 0; const FS47WebNode* TileNode = W->Node(Tile); ++Tile)
		{
			if (!TileNode->Name.StartsWith(TEXT("prop:streetGroundNature"))) continue;
			UPrimitiveComponent* TileMesh = Cast<UPrimitiveComponent>(TileNode->Comp.Get());
			if (!TileMesh) continue;
			double Rx = 0, Rz = 0;
			const bool bKnown = ReliefRoadXZ(W, ReliefFrameOf(W, Tile), TileMesh->Bounds.Origin, Rx, Rz);
			bool bSite = false;   // CALICHE-GROUND: nor may a tile lie over a site's pad, bank or cut (S47GroundEdits): it would stand in the areaway
			if (bOn && bKnown)
			{
				const FVector E = TileMesh->Bounds.BoxExtent / 100.0;
				const FBox2D SoilBox(FVector2D(Rx - E.X, Rz - E.Y), FVector2D(Rx + E.X, Rz + E.Y));
				for (const FBox2D& B : S47GroundEdits::ClearBoxes()) bSite |= SoilBox.Intersect(B);
			}
			TileMesh->SetHiddenInGame(bOn && (!bKnown || bSite || FMath::Abs(S47Relief::Sample(Rx, Rz)) > .03));
		}
		// The finale's ground stands hidden here, its collision on: a ray in Freeform must not find land nobody sees.
		for (int32 Finale : {1820, 1821})
			if (const FS47WebNode* FinaleNode = W->Node(Finale))
				if (UPrimitiveComponent* FinaleMesh = Cast<UPrimitiveComponent>(FinaleNode->Comp.Get()))
					FinaleMesh->SetCollisionEnabled(bOn ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
	}
	void ApplyRelief(FS47FreeformTerrainState& S, bool bOn)
	{
		const double Started = FPlatformTime::Seconds();
		S47Relief::SetActive(bOn);
		for (auto& G : S.Grounds)
		{
			UStaticMeshComponent* C = G.C.Get();
			if (!C) continue;
			if (bOn)
			{
				// What S47MesaRock has given it so far comes back in Story. Collision settings stay with the component.
				G.FlatMaterials.Reset();
				for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot) G.FlatMaterials.Emplace(C->GetMaterial(Slot));
				C->SetStaticMesh(G.Relief.Get());
				for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot) C->SetMaterial(Slot, S.Mid.Get());
			}
			else
			{
				C->SetStaticMesh(G.Flat.Get());
				for (int32 Slot = 0; Slot < G.FlatMaterials.Num(); ++Slot) C->SetMaterial(Slot, G.FlatMaterials[Slot].Get());
			}
		}
		if (UStaticMeshComponent* C = S.Corridor.Get())
		{
			if (bOn)
			{
				S.CorridorFlat.Reset();
				for (int32 Slot = 0; Slot < C->GetNumMaterials(); ++Slot) S.CorridorFlat.Emplace(C->GetMaterial(Slot));
				C->SetMaterial(0, S.CorridorMid.Get());
			}
			else for (int32 Slot = 0; Slot < S.CorridorFlat.Num(); ++Slot) C->SetMaterial(Slot, S.CorridorFlat[Slot].Get());
		}
		for (const auto& Row : S.Rows)
		{
			UInstancedStaticMeshComponent* C = Row.C.Get();
			if (!C) continue;
			// (tens of thousands in Caliche's nature: one tree build at the end, not one for each)
			auto* H = Cast<UHierarchicalInstancedStaticMeshComponent>(C);
			const bool bAutoTree = H && H->bAutoRebuildTreeOnInstanceChanges;
			if (H) H->bAutoRebuildTreeOnInstanceChanges = false;
			for (int32 K = 0; K < Row.Index.Num(); ++K) C->UpdateInstanceTransform(Row.Index[K], bOn ? Row.Lifted[K] : Row.Flat[K], false, false, true);
			C->MarkRenderStateDirty();
			if (H) { H->BuildTreeIfOutdated(false, true); H->bAutoRebuildTreeOnInstanceChanges = bAutoTree; }
		}
		for (const auto& Lift : S.Props)
		{
			UStaticMeshComponent* C = Lift.C.Get();
			if (!C) continue;
			if (bOn) C->SetWorldLocation(C->GetComponentLocation() + FVector(0, 0, Lift.Dy * 100));
			else C->SetRelativeTransform(Lift.Flat);
		}
		for (const auto& Wire : S.Wires) if (UStaticMeshComponent* C = Wire.C.Get()) SwapMeshKeepingMaterials(C, bOn ? Wire.Lifted.Get() : Wire.Flat.Get());
		ApplyRange(S, bOn);
		S.bOn = bOn;
		UE_LOG(LogS47FreeformTerrain, Display, TEXT("S47: Freeform's land %s in %.1f ms"), bOn ? TEXT("on") : TEXT("off"), (FPlatformTime::Seconds() - Started) * 1000);
	}
}

namespace S47FreeformTerrain
{
	bool Reconcile(US47Main* Main)
	{
		AS47Web* W = Main ? Main->Web.Get() : nullptr;
		if (!W || !W->IsLoaded()) return false;
		if (!W->FreeformTerrain.IsValid())
		{
			// A new world always starts flat (AS47Web::BeginPlay turned the table off for its build).
			if (!S47FarmWorld::Enabled(Main)) return false;
			W->FreeformTerrain = MakeShared<FS47FreeformTerrainState>();
		}
		FS47FreeformTerrainState& S = *W->FreeformTerrain;
		const bool bWant = S47FarmWorld::Enabled(Main) && W->HighwayLandscapeReady();
		if (bWant && !S.bPrepared && !S.bFailed) PrepareRelief(W, S);
		const bool bNext = bWant && S.bPrepared;
		if (bNext != S.bOn) { ApplyRelief(S, bNext); HideOldSoil(W, bNext); }
		else if (S.bOn)
		{
			// Whoever sets a material on the ground later must not take it back to the flat land's.
			for (const auto& G : S.Grounds) if (UStaticMeshComponent* C = G.C.Get()) if (C->GetMaterial(0) != S.Mid.Get()) C->SetMaterial(0, S.Mid.Get());
			if (UStaticMeshComponent* C = S.Range.C.Get()) if (C->GetMaterial(0) != S.RangeMid.Get()) C->SetMaterial(0, S.RangeMid.Get());
			if (UStaticMeshComponent* C = S.Corridor.Get()) if (C->GetMaterial(0) != S.CorridorMid.Get()) C->SetMaterial(0, S.CorridorMid.Get());
		}
		return S.bOn;
	}
	void SitesChanged(AS47Web* Web) { if (Web && IsOn(Web)) HideOldSoil(Web, true); }
	bool IsOn(const AS47Web* Web) { return Web && Web->FreeformTerrain.IsValid() && Web->FreeformTerrain->bOn; }
	bool IsRelief(const AS47Web* Web, const UStaticMesh* Mesh)
	{
		if (!IsOn(Web) || !Mesh) return false;
		for (const auto& G : Web->FreeformTerrain->Grounds) if (G.Relief.Get() == Mesh) return true;
		return false;
	}
	const TCHAR* RoadMeshPath() { return ReliefRoadPath; }
	bool HoldsMaterial(const AS47Web* Web, const UStaticMeshComponent* Component)
	{
		if (!IsOn(Web)) return false;
		for (const auto& G : Web->FreeformTerrain->Grounds) if (G.C.Get() == Component) return true;
		return Component && (Web->FreeformTerrain->Range.C.Get() == Component || Web->FreeformTerrain->Corridor.Get() == Component);
	}
	UMaterialInstanceDynamic* GroundMid(const AS47Web* Web) { return IsOn(Web) ? Web->FreeformTerrain->Mid.Get() : nullptr; }
	UMaterialInstanceDynamic* RangeMid(const AS47Web* Web) { return IsOn(Web) ? Web->FreeformTerrain->RangeMid.Get() : nullptr; }
	UMaterialInstanceDynamic* CorridorMid(const AS47Web* Web) { return IsOn(Web) ? Web->FreeformTerrain->CorridorMid.Get() : nullptr; }
	void Report(const AS47Web* Web, TArray<FString>& Lines)
	{
		if (!Web || !Web->FreeformTerrain.IsValid()) { Lines.Add(TEXT("relief: not prepared")); return; }
		const FS47FreeformTerrainState& S = *Web->FreeformTerrain;
		Lines.Add(FString::Printf(TEXT("relief: on %d, %d grounds, %d instances (%d wire pieces), %d props, %d wire meshes"), S.bOn, S.Grounds.Num(), S.Instances, S.Segments, S.Props.Num(), S.Wires.Num()));
		const UStaticMeshComponent* RangeC = S.Range.C.Get();
		Lines.Add(FString::Printf(TEXT("range: %s, on the mesas' node: %s"), RangeC ? TEXT("installed") : TEXT("not in use"), RangeC && RangeC->GetStaticMesh() ? *RangeC->GetStaticMesh()->GetName() : TEXT("-")));
		Lines.Append(S.Notes);
	}
}
