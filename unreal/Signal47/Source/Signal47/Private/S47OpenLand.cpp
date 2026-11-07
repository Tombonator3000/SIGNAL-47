#include "S47OpenLand.h"
#include "S47Geo.h"
#include "S47Relief.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Web.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47OpenLand, Log, All);

namespace
{
	bool bLive = false;
	struct FCoreRect { double X0, X1, Z0, Z1; };
	// SARO's site with the array and 40 m round it (S47GroundCover ARRAY_FLAT in road terms), the farm's yard and the motel. Chosen
	// on the two lands' difference, GridY + HillLift against SaroLand (Off is common to both): up to 53 m at the ring's edge, so the
	// ring must not be driven far. Worst on these lines 2.4 m, west of the array; the eye is put on the new floor at once.
	const FCoreRect SaroRects[] = { { -315, 350, -861, 250 }, { -150, 300, 250, 480 } };
	constexpr double StationCoreR = 110;
	constexpr double FenceOut = 1.5, FenceStep = 5, FencePostH = 1.25, FenceSteepM = 1.5;

	TWeakObjectPtr<AS47Web> FenceWorld;
	TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> PostsC, WiresC;
	int32 PostCount = 0;
	bool bFenceShown = false;

	double WrapAngle(double A) { return FMath::Atan2(FMath::Sin(A), FMath::Cos(A)); }

	UHierarchicalInstancedStaticMeshComponent* NewFencePart(AS47Web* W, UStaticMesh* Cube, UMaterialInterface* Material, int32 Cull)
	{
		auto* C = NewObject<UHierarchicalInstancedStaticMeshComponent>(W);
		C->SetMobility(EComponentMobility::Movable);
		C->SetStaticMesh(Cube);
		if (Material) C->SetMaterial(0, Material);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetCullDistances(int32(Cull * .85), Cull);
		C->bAutoRebuildTreeOnInstanceChanges = false;
		C->SetupAttachment(W->GetRootComponent());
		C->RegisterComponent();
		return C;
	}

	void BuildFence(AS47Web* W)
	{
		FenceWorld = W;
		PostCount = 0;
		UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!Cube) { UE_LOG(LogS47OpenLand, Warning, TEXT("S47: no cube for the rim's fence: it is not drawn")); return; }
		auto* Wood = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TownBuildings/Materials/M_Town_DarkWood.M_Town_DarkWood"));
		auto* Metal = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/TownBuildings/Materials/M_Town_Metal.M_Town_Metal"));
		UHierarchicalInstancedStaticMeshComponent* Posts = NewFencePart(W, Cube, Wood, 130000), * Wires = NewFencePart(W, Cube, Metal, 45000);
		using namespace S47OpenLand;
		const double X0 = RimX0 - FenceOut, X1 = RimX1 + FenceOut, Z0 = RimZ0 - FenceOut, Z1 = RimZ1 + FenceOut;
		const FVector2D Corners[] = { { X0, Z0 }, { X1, Z0 }, { X1, Z1 }, { X0, Z1 }, { X0, Z0 } };
		const auto Stands = [](double X, double Z) { return S47Relief::HillAbove(X, Z) < FenceSteepM; };
		const auto Top = [](double X, double Z, double H) { return S47Three::ToUE(X + 8000, S47RoadTerrain::SurfaceY(X, Z) + H, Z); };
		for (int32 Side = 0; Side < 4; ++Side)
		{
			const FVector2D A = Corners[Side], B = Corners[Side + 1];
			const int32 Count = FMath::CeilToInt((B - A).Size() / FenceStep);
			for (int32 I = 0; I < Count; ++I)
			{
				const FVector2D P = FMath::Lerp(A, B, double(I) / Count), Q = FMath::Lerp(A, B, double(I + 1) / Count);
				if (!Stands(P.X, P.Y)) continue;
				const double Lean = FMath::Sin(P.X * .37 + P.Y * .29) * 1.6;
				Posts->AddInstance(FTransform(FRotator(Lean, P.X + P.Y, -Lean * .6), Top(P.X, P.Y, FencePostH * .5 - .05), FVector(.09, .09, FencePostH)), true);
				++PostCount;
				if (!Stands(Q.X, Q.Y)) continue;
				for (const double H : { .55, 1.05 })
				{
					const FVector From = Top(P.X, P.Y, H), To = Top(Q.X, Q.Y, H);
					const FVector Along = To - From;
					Wires->AddInstance(FTransform(FRotationMatrix::MakeFromX(Along.GetSafeNormal()).ToQuat(), (From + To) * .5, FVector(Along.Size() / 100, .012, .012)), true);
				}
			}
		}
		Posts->BuildTreeIfOutdated(false, true);
		Wires->BuildTreeIfOutdated(false, true);
		PostsC = Posts; WiresC = Wires;
		bFenceShown = true;
		UE_LOG(LogS47OpenLand, Display, TEXT("S47: the rim's fence: %d posts, %d wires"), PostCount, Wires->GetInstanceCount());
	}
}

namespace S47OpenLand
{
	bool Off() { static const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("S47OpenLandOff")); return bOff; }
	bool Live() { return bLive; }
	void SetLive(bool bOn) { bLive = bOn && !Off(); }
	FSuspend::FSuspend() : bWas(bLive) { bLive = false; }
	FSuspend::~FSuspend() { bLive = bWas; }

	bool InRim(double RoadX, double RoadZ, double Inset)
	{
		return RoadX >= RimX0 + Inset && RoadX <= RimX1 - Inset && RoadZ >= RimZ0 + Inset && RoadZ <= RimZ1 - Inset;
	}
	bool InSaroCore(double RoadX, double RoadZ, double Grow)
	{
		if (Grow >= 0)
		{
			for (const FCoreRect& R : SaroRects) if (RoadX >= R.X0 - Grow && RoadX <= R.X1 + Grow && RoadZ >= R.Z0 - Grow && RoadZ <= R.Z1 + Grow) return true;
			return false;
		}
		// Shrunk: in by that much from every edge of the two rectangles together, which share the line z 250 from x -150 to 300.
		const double In = -Grow;
		for (const FCoreRect& R : SaroRects)
		{
			if (RoadX < R.X0 || RoadX > R.X1 || RoadZ < R.Z0 || RoadZ > R.Z1) continue;
			bool bClear = true;
			for (const FVector2D& D : { FVector2D(-In, 0), FVector2D(In, 0), FVector2D(0, -In), FVector2D(0, In) }) bClear &= InSaroCore(RoadX + D.X, RoadZ + D.Y, 0);
			if (bClear) return true;
		}
		return false;
	}
	bool InStationCore(double RoadX, double RoadZ, double Grow)
	{
		const S47Geo::FXZ At = S47Geo::StationToRoad(0, 0);
		return FMath::Square(RoadX - At.X) + FMath::Square(RoadZ - At.Z) <= FMath::Square(StationCoreR + Grow);
	}
	bool IsBoundaryLine(double MinX, double MaxX, double MinZ, double MaxZ)
	{
		const double Wide = MaxX - MinX, Deep = MaxZ - MinZ;
		return (Wide <= .6 && Deep >= 25) || (Deep <= 1.1 && Wide >= 25);
	}

	bool RoadOf(const FString& Leg, double X, double Z, double& OutRoadX, double& OutRoadZ)
	{
		if (Leg == TEXT("saro") || Leg == TEXT("motel")) { OutRoadX = X + S47Geo::SARO_IN_ROAD.X; OutRoadZ = Z + S47Geo::SARO_IN_ROAD.Z; return true; }
		if (Leg == TEXT("road") || Leg == TEXT("diner")) { OutRoadX = X - 8000; OutRoadZ = Z; return true; }
		if (Leg == TEXT("station01"))
		{
			const S47Geo::FXZ R = S47Geo::StationToRoad(X, Z - 8000);
			OutRoadX = R.X; OutRoadZ = R.Z;
			return true;
		}
		return false;
	}
	FVector FromRoad(const FString& Leg, double RoadX, double RoadZ)
	{
		if (Leg == TEXT("saro") || Leg == TEXT("motel")) return FVector(RoadX - S47Geo::SARO_IN_ROAD.X, 0, RoadZ - S47Geo::SARO_IN_ROAD.Z);
		if (Leg == TEXT("station01")) { const S47Geo::FXZ S = S47Geo::RoadToStation(RoadX, RoadZ); return FVector(S.X, 0, S.Z + 8000); }
		return FVector(RoadX + 8000, 0, RoadZ);
	}
	double Turned(const FString& FromLeg, const FString& ToLeg, double Heading)
	{
		const double Turn = (ToLeg == TEXT("station01") ? 1 : 0) - (FromLeg == TEXT("station01") ? 1 : 0);
		return Turn == 0 ? Heading : WrapAngle(Heading + Turn * S47Geo::STATION_TURN());
	}
	FString LegFor(const FString& FromLeg, double RoadX, double RoadZ)
	{
		if (FromLeg == TEXT("saro")) return InSaroCore(RoadX, RoadZ, HandOverM) ? FromLeg : FString(TEXT("road"));
		if (FromLeg == TEXT("station01")) return InStationCore(RoadX, RoadZ, HandOverM) ? FromLeg : FString(TEXT("road"));
		if (FromLeg == TEXT("road"))
		{
			if (InSaroCore(RoadX, RoadZ, -HandOverM)) return TEXT("saro");
			if (InStationCore(RoadX, RoadZ, -HandOverM)) return TEXT("station01");
		}
		return FromLeg;
	}
	TOptional<S47Legs::FExit> Exit(const FString& Leg, double X, double Z, double Heading)
	{
		double Rx = 0, Rz = 0;
		if (Leg == TEXT("roswell") || !RoadOf(Leg, X, Z, Rx, Rz)) return {};
		const FString To = LegFor(Leg, Rx, Rz);
		if (To == Leg) return {};
		const FVector P = FromRoad(To, Rx, Rz);
		return S47Legs::FExit{ To, P.X, P.Z, Turned(Leg, To, Heading) };
	}

	void ReconcileFence(AS47Web* Web, bool bOn, const FString& Area)
	{
		if (!Web) return;
		// (it reads the land: after the relief is on, or when it never will be)
		if (bOn && FenceWorld.Get() != Web && (S47Relief::Active() || !S47Relief::Ready())) BuildFence(Web);
		if (FenceWorld.Get() != Web) return;
		const bool bShow = bOn && (Area == TEXT("road") || Area == TEXT("diner"));
		if (bShow == bFenceShown) return;
		bFenceShown = bShow;
		if (auto* C = PostsC.Get()) C->SetVisibility(bShow);
		if (auto* C = WiresC.Get()) C->SetVisibility(bShow);
	}
	int32 FencePosts() { return PostCount; }
}
