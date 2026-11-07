#include "S47ReliefProbe.h"
#include "S47Drive.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47FreeformTerrain.h"
#include "S47Geo.h"
#include "S47GroundCover.h"
#include "S47Main.h"
#include "S47Relief.h"
#include "S47RoadEdge.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ReliefProbe, Log, All);

namespace
{
	constexpr double ReliefProbeLimit = .15;   // the floor off the meshes, in metres, where the land is walked and driven most
	// Far out the collision meshes are coarser by design (build_mesh.py COL_TOL: .25 m at 1000 m from what stays put; they were
	// read back at up to .41 there, .12 within 350 m of the highway). Nothing reads them that finely: the floor is SurfaceY, and
	// GroundUnder's ray reaches from 3 m over it to 6 m under.
	constexpr double ReliefProbeFarLimit = .45;
	constexpr double ReliefProbeLive = .05;    // ground this far off flat counts as having seen the relief
	constexpr double ReliefProbeEye = .5;      // the walker's floor this far off the ground under it is a floor gone stale
	double ProbeLimitAt(double RoadX, double RoadZ)
	{
		return FMath::Abs(RoadX) < 350 || FVector2D::Distance(FVector2D(RoadX, RoadZ), FVector2D(S47Geo::SARO_IN_ROAD.X, S47Geo::SARO_IN_ROAD.Z)) < 650 ? ReliefProbeLimit : ReliefProbeFarLimit;
	}

	// Road-local eye and what it looks at. The array's dishes stand at x -230..210, z -420..-42 in SARO's terms (road x + 24,
	// z - 300), the antenna view is from its west side. No mesa foot is known by a name: this one is the farthest open land west.
	// On foot the road's land is the diner's area (US47World::MakeLands). "road" is the drive's: it has no land, so nothing walks
	// there and the floor stays where it was. The town view was taken 9 cm off the ground that way and the mesa's from under it.
	struct FReliefView { const TCHAR* Name; const TCHAR* Area; double X, Z, LookX, LookZ, Lift; };
	const FReliefView ReliefViews[] = {
		{ TEXT("road"), TEXT("diner"), -40, 900, -400, 880, 2 },
		{ TEXT("town"), TEXT("diner"), -40, 1640, -10, 2000, 4 },
		{ TEXT("array"), TEXT("saro"), -320, -500, 0, -520, 6 },
		{ TEXT("farm"), TEXT("saro"), 25, 300, 100, 215, 2 },
		{ TEXT("mesa"), TEXT("diner"), -1300, 1000, -2600, 1000, 30 },
		{ TEXT("highway"), TEXT("diner"), -14, 700, 6, 1500, 8 },
		{ TEXT("station"), TEXT("station01"), -560, 560, -1400, 700, 8 },
	};
	const double ReliefHours[] = { 11, 18.6, 23.5 };
	// -S47RoadEdgeViews: only the highway's edge, by day and in the low sun, and none of the walks: Tom's picture, and from the shoulder, the road and afar.
	// Road-local, as above; the SARO views stand in its own stretches.
	const FReliefView EdgeViews[] = {
		{ TEXT("edge-along"), TEXT("diner"), 8.5, 900, 3, 1500, 0 },
		{ TEXT("edge-tom"), TEXT("diner"), 16, 700, 2, 1100, 2 },
		{ TEXT("edge-down"), TEXT("diner"), 6.6, 1210, 4.2, 1214.5, -1.2 },
		{ TEXT("edge-shoulder"), TEXT("diner"), 5.4, 1300, 4.8, 1400, 0 },
		{ TEXT("edge-car"), TEXT("diner"), 2.2, 1100, 2.2, 1700, 0 },
		{ TEXT("edge-west"), TEXT("diner"), -7.5, 1500, -5, 1000, 0 },
		{ TEXT("edge-far"), TEXT("diner"), -14, 700, 6, 1500, 8 },
		{ TEXT("edge-town"), TEXT("diner"), 9, 1880, 2, 2150, 0 },
		{ TEXT("edge-corridor"), TEXT("saro"), 9, 150, 3, 560, 0 },
		{ TEXT("edge-site"), TEXT("saro"), 7, -500, 2, -900, 0 },
		{ TEXT("edge-farm"), TEXT("saro"), 9, 170, 16, 200, 0 },
	};
	const double EdgeHours[] = { 11, 18.6 };
	// Road-local again. The road's land is walked over z 450; SARO's within 600 m of the site, where the table may well be 0 all the
	// way (its mask): that walk proves the ring is under the feet and need not have met any relief.
	constexpr double ReliefFootZ = 1400;   // the line along which the foot of a hill is tried, road-local
	struct FReliefWalk { const TCHAR* Area; double X, Z, Bearing; bool bNeedRelief; };
	const FReliefWalk ReliefWalks[] = { { TEXT("diner"), -300, 1000, 0, true }, { TEXT("diner"), -700, 1400, 1.2, true }, { TEXT("saro"), -426, -400, 0, false } };

	struct FReliefRun
	{
		int32 Phase = 0, WalkAt = 0, HourAt = 0, ViewAt = 0;
		double Now = 0, Since = 0, SampleAt = 0, FootX = -1000;
		int32 Void = 0, Samples = 0;
		double Worst = 0, Seen = 0;                 // the current walk or trip
		double WorstX = 0, WorstZ = 0;              // where, road-local
		FString WorstWhat, TopWhat;                 // what the ray met there; what stands over the truck's floor
		bool bOver = false;                         // past the limit for its place
		double TopOver = 0;
		double DriveTravel = 0, TruckOff = 0;
		FVector LastTruck = FVector::ZeroVector;
		bool bAllGood = true, bHolding = false, bLookDone = false, bEdgeOnly = false;
		double FrameSum = 0, FrameMost = 0; int32 FrameCount = 0;
		TArray<FString> Lines;
	};
	FReliefRun ReliefRun;

	void ProbeLine(const FString& Line)
	{
		UE_LOG(LogS47ReliefProbe, Display, TEXT("S47: relief probe: %s"), *Line);
		ReliefRun.Lines.Add(Line);
	}
	void ProbeCheck(bool bPass, const FString& What)
	{
		ProbeLine(FString(bPass ? TEXT("PASS ") : TEXT("FAIL ")) + What);
		ReliefRun.bAllGood &= bPass;
	}
	bool ProbeFinish(const TCHAR* Why = nullptr)
	{
		if (Why) ProbeCheck(false, Why);
		UE_LOG(LogS47ReliefProbe, Display, TEXT("S47: relief probe %s"), ReliefRun.bAllGood ? TEXT("PASS") : TEXT("FAIL"));
		FPlatformMisc::RequestExitWithStatus(false, ReliefRun.bAllGood ? 0 : 1);
		return false;
	}
	void ProbeRoadXZ(const FString& Area, double X, double Z, double& OutX, double& OutZ)
	{
		if (Area == TEXT("saro")) { OutX = X + S47Geo::SARO_IN_ROAD.X; OutZ = Z + S47Geo::SARO_IN_ROAD.Z; }
		else { OutX = X - 8000; OutZ = Z; }
	}
	// What -S47Walk and -S47GetOut measure: from the floor, is the area's ground under it, and how far off.
	// With Only, that one component and nothing standing on it: under a truck the world's ray meets the truck first.
	void ProbeSample(US47Main* M, const FVector& At, double Floor, UPrimitiveComponent* Only = nullptr)
	{
		FReliefRun& R = ReliefRun;
		++R.Samples;
		{
			// (a floor on a fan stands over the ground mesh by the fan: the fan is the range's, which has no collision)
			double LiftX = 0, LiftZ = 0;
			ProbeRoadXZ(M->Web->Area(), At.X, At.Z, LiftX, LiftZ);
			if (M->Web->Area() == TEXT("diner") || M->Web->Area() == TEXT("road")) Floor -= S47Relief::HillLift(LiftX, LiftZ);
		}
		FHitResult Hit;
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(S47ReliefProbe), true);
		UWorld* World = M->Web->GetWorld();
		const FVector From = S47Three::ToUE(FVector(At.X, Floor + 2, At.Z)), To = S47Three::ToUE(FVector(At.X, Floor - 6, At.Z));
		double Rx = 0, Rz = 0;
		ProbeRoadXZ(M->Web->Area(), At.X, At.Z, Rx, Rz);
		const bool bMet = Only ? Only->LineTraceComponent(Hit, From, To, Query) : World && World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Query);
		if (!bMet) ++R.Void;
		else
		{
			const double Off = FMath::Abs(Hit.ImpactPoint.Z / 100.0 - Floor);
			R.bOver |= Off >= ProbeLimitAt(Rx, Rz);
			if (Off >= R.Worst)
			{
				R.Worst = Off; R.WorstX = Rx; R.WorstZ = Rz;
				const UPrimitiveComponent* What = Hit.GetComponent();
				R.WorstWhat = What ? What->GetName() : FString(TEXT("nothing named"));
			}
		}
		R.Seen = FMath::Max(R.Seen, FMath::Abs(S47Relief::Off(Rx, Rz)));
	}
	void ProbeReset()
	{
		FReliefRun& R = ReliefRun;
		R.Void = 0; R.Samples = 0; R.Worst = 0; R.Seen = 0; R.SampleAt = R.Now;
		R.WorstX = 0; R.WorstZ = 0; R.WorstWhat.Reset(); R.bOver = false; R.TopOver = 0; R.TopWhat.Reset();
	}
	// The ground mesh under a point, whichever of the three reaches here: the road's land, SARO's ring, the corridor.
	bool ProbeGroundY(AS47Web* W, const FVector& At, double Floor, double& Out)
	{
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(S47ReliefProbeGround), true);
		bool bFound = false;
		for (const int32 Part : { 1242, 7, 953 })
		{
			const FS47WebNode* N = W->Node(Part);
			UPrimitiveComponent* C = N ? Cast<UPrimitiveComponent>(N->Comp.Get()) : nullptr;
			FHitResult Ray;
			if (!C || !C->LineTraceComponent(Ray, S47Three::ToUE(FVector(At.X, Floor + 40, At.Z)), S47Three::ToUE(FVector(At.X, Floor - 40, At.Z)), Query)) continue;
			const double Y = Ray.ImpactPoint.Z / 100.0;
			if (!bFound || FMath::Abs(Y - Floor) < FMath::Abs(Out - Floor)) Out = Y;
			bFound = true;
		}
		return bFound;
	}
	// What draws under the eye, for a picture that looks wrong: meshes by their bounds, instances within 3 m.
	void ProbeUnder(AS47Web* W, const FString& Shot, const FVector& At, double Floor)
	{
		const FVector Here = S47Three::ToUE(FVector(At.X, Floor, At.Z));
		FString Found;
		int32 Count = 0;
		for (int32 I = 0; I < W->PartCount(); ++I)
		{
			const FS47WebNode* N = W->Node(I);
			const UStaticMeshComponent* C = N ? Cast<UStaticMeshComponent>(N->Comp.Get()) : nullptr;
			if (!C || !C->GetStaticMesh() || !C->IsVisible() || C->bHiddenInGame) continue;
			bool bHere = false;
			if (const UInstancedStaticMeshComponent* Many = Cast<UInstancedStaticMeshComponent>(C)) bHere = Many->GetInstancesOverlappingSphere(Here, 300.f, true).Num() > 0;
			else
			{
				const FBox Box = C->Bounds.GetBox();
				bHere = Here.X >= Box.Min.X && Here.X <= Box.Max.X && Here.Y >= Box.Min.Y && Here.Y <= Box.Max.Y && Here.Z >= Box.Min.Z - 300 && Here.Z <= Box.Max.Z + 300;
			}
			if (!bHere) continue;
			++Count;
			if (Found.Len() < 3000) Found += FString::Printf(TEXT(" %d:%s[%s]"), I, *N->Name, *C->GetStaticMesh()->GetName());
		}
		ProbeLine(FString::Printf(TEXT("under %s: %d:%s"), *Shot, Count, *Found));
	}
	// The relief mesh itself against the floor, one component, at random points where the table is live.
	void ProbeMesh(const TCHAR* Name, UStaticMeshComponent* Comp, const FVector2D& ToRoad, TFunctionRef<bool(FRandomStream&, FVector&, double&)> NextPoint)
	{
		if (!Comp) { ProbeCheck(false, FString::Printf(TEXT("%s ground has no component"), Name)); return; }
		FRandomStream Random(47203);
		int32 Tested = 0, Missed = 0, OverNear = 0, OverFar = 0;
		double Largest = 0, LargestX = 0, LargestZ = 0, LargestY = 0;
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(S47ReliefProbeMesh), true);
		for (int32 I = 0; I < 12000 && Tested < 3000; ++I)
		{
			FVector At; double Floor = 0;
			if (!NextPoint(Random, At, Floor)) continue;
			++Tested;
			FHitResult Hit;
			if (!Comp->LineTraceComponent(Hit, S47Three::ToUE(FVector(At.X, Floor + 3, At.Z)), S47Three::ToUE(FVector(At.X, Floor - 6, At.Z)), Query)) { ++Missed; continue; }
			const double Rx = At.X + ToRoad.X, Rz = At.Z + ToRoad.Y, Signed = Hit.ImpactPoint.Z / 100.0 - Floor, Off = FMath::Abs(Signed), Limit = ProbeLimitAt(Rx, Rz);
			if (Off >= Limit) { if (Limit < ReliefProbeFarLimit) ++OverNear; else ++OverFar; }
			if (Off >= Largest) { Largest = Off; LargestX = Rx; LargestZ = Rz; LargestY = Signed; }
		}
		ProbeCheck(Tested > 100 && Missed == 0 && OverNear + OverFar == 0, FString::Printf(TEXT("%s mesh against the floor: %d points on live ground, %d with no mesh, largest %.3f m (mesh minus floor %.3f) at road %.1f %.1f, %d past %.2f m near the roads and the site, %d past %.2f m further out"), Name, Tested, Missed, Largest, LargestY, LargestX, LargestZ, OverNear, ReliefProbeLimit, OverFar, ReliefProbeFarLimit));
	}
	void ProbeMeshes(US47Main* M)
	{
		AS47Web* W = M->Web.Get();
		const FS47WebNode* RoadNode = W->Node(1242);
		const FS47WebNode* SaroNode = W->Node(7);
		ProbeMesh(TEXT("road"), RoadNode ? Cast<UStaticMeshComponent>(RoadNode->Comp.Get()) : nullptr, FVector2D(-8000, 0), [](FRandomStream& Random, FVector& At, double& Floor)
		{
			const double X = Random.FRandRange(-2900.f, 2200.f), Z = Random.FRandRange(-2600.f, 4300.f);
			if (FMath::Abs(S47Relief::Sample(X, Z)) < ReliefProbeLive) return false;
			At = FVector(X + 8000, 0, Z); Floor = S47RoadTerrain::SurfaceY(X, Z) - S47Relief::HillLift(X, Z);
			return true;
		});
		UStaticMeshComponent* SaroComp = SaroNode ? Cast<UStaticMeshComponent>(SaroNode->Comp.Get()) : nullptr;
		const FBox Ring = SaroComp ? SaroComp->Bounds.GetBox() : FBox(ForceInit);
		const double Reach = SaroComp ? FMath::Min(Ring.GetExtent().X, Ring.GetExtent().Y) * .9 / 100 : 0;
		const FVector Middle = SaroComp ? S47Three::ToThree(Ring.GetCenter()) : FVector::ZeroVector;
		ProbeMesh(TEXT("SARO"), SaroComp, FVector2D(S47Geo::SARO_IN_ROAD.X, S47Geo::SARO_IN_ROAD.Z), [Reach, Middle](FRandomStream& Random, FVector& At, double& Floor)
		{
			const double A = Random.FRandRange(0.f, 6.2831853f), D = Reach * FMath::Sqrt(Random.FRand());
			const double X = Middle.X + FMath::Cos(A) * D, Z = Middle.Z + FMath::Sin(A) * D;
			if (FMath::Abs(S47Relief::Sample(X + S47Geo::SARO_IN_ROAD.X, Z + S47Geo::SARO_IN_ROAD.Z)) < ReliefProbeLive) return false;
			At = FVector(X, 0, Z); Floor = S47GroundCover::SaroLand(X, Z);
			return true;
		});
	}
	void ProbeSetHour(US47Main* M, double Hour)
	{
		const double Clock = FMath::Fmod(M->Freeform->Clock(), 86400.);
		const double Ahead = FMath::Fmod(Hour * 3600. - Clock + 86400., 86400.);
		if (Ahead > 1 && !M->Freeform->TestAdvance(Ahead)) ProbeCheck(false, FString::Printf(TEXT("the clock would not go to %.1f"), Hour));
	}
}

namespace S47ReliefProbe
{
	void Begin(US47Main* Main)
	{
		(void)Main;
		ReliefRun = FReliefRun();
		ReliefRun.bEdgeOnly = FParse::Param(FCommandLine::Get(), TEXT("S47RoadEdgeViews"));
	}

	bool Step(US47Main* M, float Dt)
	{
		FReliefRun& R = ReliefRun;
		R.Now += Dt;
		if (!M || !M->Web || !M->World || !M->Player || !M->Freeform) return ProbeFinish(TEXT("the game went away"));
		AS47Web* W = M->Web.Get();
		US47World* Life = M->World;
		AS47Walker* P = M->Player;

		if (R.Phase == 0)
		{
			if (R.Now < 2) return true;
			if (!S47FreeformTerrain::IsOn(W) || !S47Relief::Active()) return ProbeFinish(TEXT("the relief is not on: see S47FreeformTerrain's line in the log"));
			TArray<FString> Lines;
			S47FreeformTerrain::Report(W, Lines);
			for (const FString& Line : Lines) ProbeLine(Line);
			Lines.Reset();
			const bool bZones = S47Relief::CheckZones(Lines);
			for (const FString& Line : Lines) ProbeLine(Line);
			ProbeCheck(bZones, TEXT("the table is 0 where nothing follows it"));
			Lines.Reset();
			S47RoadEdge::Report(W, Lines);
			for (const FString& Line : Lines) ProbeLine(Line);
			if (R.bEdgeOnly) { R.Phase = 7; R.Since = R.Now; return true; }
			ProbeMeshes(M);
			R.Phase = 1; R.Since = R.Now;
			return true;
		}

		if (R.Phase == 1) // a walk: into its area, stood at its start
		{
			if (R.WalkAt >= UE_ARRAY_COUNT(ReliefWalks)) { R.Phase = 20; R.Since = R.Now; return true; }
			const FReliefWalk& Walk = ReliefWalks[R.WalkAt];
			if (Life->Area != Walk.Area) { Life->Enter(Walk.Area); R.Since = R.Now; return true; }
			if (R.Now - R.Since < .5) return true;
			const FVector Start = S47FarmWorld::RoadToArea(Walk.Area, FVector(Walk.X, 0, Walk.Z));
			P->Place(Start.X, Start.Z, Walk.Bearing);
			P->Pitch = -.06; P->TestMove = FVector2D(0, -1); P->bTestRun = true;
			ProbeReset();
			R.Phase = 2; R.Since = R.Now;
			return true;
		}
		if (R.Phase == 2)
		{
			if (R.Now - R.SampleAt >= .25)
			{
				R.SampleAt = R.Now;
				if (R.Now - R.Since > 1) ProbeSample(M, P->Pos, P->FloorY);
			}
			if (R.Now - R.Since < 12) return true;
			P->TestMove.Reset(); P->bTestRun = false;
			// A walk that stood still proves nothing: at a run it is some 35 m.
			const FReliefWalk& Done = ReliefWalks[R.WalkAt];
			const FVector Began = S47FarmWorld::RoadToArea(Done.Area, FVector(Done.X, 0, Done.Z));
			const double Walked = FVector2D::Distance(FVector2D(Began.X, Began.Z), FVector2D(P->Pos.X, P->Pos.Z));
			ProbeCheck(R.Void == 0 && !R.bOver && (R.Seen >= ReliefProbeLive || !Done.bNeedRelief) && R.Samples > 10 && Walked > 10,
				FString::Printf(TEXT("walk %d in %s: %.0f m, %d samples, void %d, floor off the meshes at most %.3f m at road %.1f %.1f (%s), largest relief under it %.2f m, floor at the end %.3f"), R.WalkAt, *Life->Area, Walked, R.Samples, R.Void, R.Worst, R.WorstX, R.WorstZ, *R.WorstWhat, R.Seen, P->FloorY));
			++R.WalkAt; R.Phase = 1; R.Since = R.Now;
			return true;
		}

		if (R.Phase == 20) // the foot of a hill, the first west of the road on this line: run at it from 20 m out. The range has no collision.
		{
			if (Life->Area != TEXT("diner")) { Life->Enter(TEXT("diner")); R.Since = R.Now; return true; }
			if (R.Now - R.Since < .5) return true;
			R.FootX = 0;
			for (double X = -400; X > -2800 && !R.FootX; X -= 2) if (S47Relief::HillStops(X, ReliefFootZ, P->Radius)) R.FootX = X;
			if (!R.FootX) { ProbeCheck(false, TEXT("no hill's foot on the line: is range_foot.bin installed?")); R.FootX = -1000; R.Phase = 3; R.Since = R.Now; return true; }
			const FVector Start = S47FarmWorld::RoadToArea(TEXT("diner"), FVector(R.FootX + 20, 0, ReliefFootZ));
			P->Place(Start.X, Start.Z, UE_DOUBLE_PI / 2);
			P->Pitch = -.06; P->TestMove = FVector2D(0, -1); P->bTestRun = true;
			R.Phase = 21; R.Since = R.Now;
			return true;
		}
		if (R.Phase == 21)
		{
			if (R.Now - R.Since < 12) return true;
			P->TestMove.Reset(); P->bTestRun = false;
			const double Rx = P->Pos.X - 8000, Above = S47Relief::HillAbove(Rx, P->Pos.Z);
			ProbeCheck(!S47Relief::HillStops(Rx, P->Pos.Z, P->Radius) && Rx >= R.FootX - 1 && Rx <= R.FootX + 6, FString::Printf(TEXT("the foot stops the walk: first it cannot climb at road x %.1f, stood at %.1f, %.2f m up the fan, floor %.2f"), R.FootX, Rx, Above, P->FloorY));
			R.Phase = 3; R.Since = R.Now;
			return true;
		}

		if (R.Phase == 3) // the trip: out on the open land, straight at the same foot at a moderate speed, where it must stop
		{
			FS47TripOpts Opts;
			const double Sx = R.FootX + 70, Sz = ReliefFootZ;
			Opts.AtPos = FVector(8000 + Sx, S47RoadTerrain::SurfaceY(Sx, Sz), Sz); Opts.AtHeading = UE_DOUBLE_PI / 2; Opts.Leg = TEXT("road");
			Life->StartTrip(TEXT("road"), TEXT("diner"), Opts);
			Life->TestInput = [Life] { FS47DriveIn In; const FS47DriveController* D = Life->CabDrive(); In.Throttle = D && D->Speed < 11 ? 1 : 0; return In; };
			ProbeReset();
			R.DriveTravel = 0; R.TruckOff = 0; R.LastTruck = FVector::ZeroVector;
			R.Phase = 4; R.Since = R.Now;
			return true;
		}
		if (R.Phase == 4 || R.Phase == 5)
		{
			const FS47DriveController* Cab = Life->CabDrive();
			if (!Cab)
			{
				if (R.Now - R.Since > 15) { Life->TestInput = nullptr; ProbeCheck(false, TEXT("the trip did not start")); R.Phase = 7; R.Since = R.Now; }
				return true;
			}
			if (R.Now - R.SampleAt >= .25 && Life->bDriving && !Life->bBusy)
			{
				R.SampleAt = R.Now;
				const double Floor = S47RoadTerrain::SurfaceY(Cab->Pos.X - 8000, Cab->Pos.Z);
				// The land itself; the truck's own parts take the Visibility channel like everything of the web's (S47Web BuildNode).
				// Cab->Pos.Y stays 0 through a drive: the body's height is in the truck's pose.
				const FS47WebNode* LandNode = W->Node(1242);
				ProbeSample(M, Cab->Pos, Floor, LandNode ? Cast<UPrimitiveComponent>(LandNode->Comp.Get()) : nullptr);
				if (const FS47Truck* Riding = Life->TruckHere()) R.TruckOff = FMath::Max(R.TruckOff, FMath::Abs(Riding->Position.Y - Floor));
				FHitResult Top;
				const FCollisionQueryParams TopQuery(SCENE_QUERY_STAT(S47ReliefProbeTop), true);
				if (W->GetWorld() && W->GetWorld()->LineTraceSingleByChannel(Top, S47Three::ToUE(FVector(Cab->Pos.X, Floor + 2, Cab->Pos.Z)), S47Three::ToUE(FVector(Cab->Pos.X, Floor - 6, Cab->Pos.Z)), ECC_Visibility, TopQuery))
				{
					const double Over = Top.ImpactPoint.Z / 100.0 - Floor;
					if (Over >= R.TopOver) { R.TopOver = Over; const UPrimitiveComponent* TopComp = Top.GetComponent(); R.TopWhat = TopComp ? TopComp->GetName() : FString(); }
				}
				if (!R.LastTruck.IsZero()) R.DriveTravel += FVector2D::Distance(FVector2D(Cab->Pos.X, Cab->Pos.Z), FVector2D(R.LastTruck.X, R.LastTruck.Z));
				R.LastTruck = Cab->Pos;
			}
			if (R.Phase == 4)
			{
				// By samples, a quarter of a second of driving each: the first drive's loading has eaten the whole 16 s of a clock, frames of 14 s.
				if (R.Samples < 64 && R.Now - R.Since < 120) return true;
				// braked to a stop, as -S47GetOut does
				Life->TestInput = [Life] { FS47DriveIn In; const double V = Life->DrivingSpeed(); In.Throttle = V > .15 ? -1 : V < -.15 ? 1 : 0; return In; };
				R.Phase = 5; R.Since = R.Now;
				return true;
			}
			if (!Life->CanGetOut())
			{
				if (R.Now - R.Since > 10) { Life->TestInput = nullptr; ProbeCheck(false, TEXT("could not get out of the truck")); R.Phase = 7; R.Since = R.Now; }
				return true;
			}
			{
				const double Rx = Cab->Pos.X - 8000;
				ProbeCheck(Rx >= R.FootX - 1 && Rx <= R.FootX + 12, FString::Printf(TEXT("the foot stops the truck: first the walker cannot climb at road x %.1f, its centre at %.1f, %.2f m up the fan"), R.FootX, Rx, S47Relief::HillAbove(Rx, Cab->Pos.Z)));
			}
			ProbeCheck(R.Void == 0 && !R.bOver && R.Seen >= ReliefProbeLive && R.DriveTravel > 40,
				FString::Printf(TEXT("trip: %.0f m, %d samples, void %d, floor off the land mesh at most %.3f m at road %.1f %.1f, largest relief under it %.2f m (for reading only: truck's pose off the floor at most %.3f m; the world's ray from 2 m up meets %s at %.3f m over the floor)"), R.DriveTravel, R.Samples, R.Void, R.Worst, R.WorstX, R.WorstZ, R.Seen, R.TruckOff, *R.TopWhat, R.TopOver));
			Life->TestInput = nullptr;
			Life->GetOutHere();
			R.Phase = 6; R.Since = R.Now;
			return true;
		}
		if (R.Phase == 6) // out, and a short walk away from the truck
		{
			if (R.Now - R.Since < 1 || Life->bBusy) { if (R.Now - R.Since > 15) { ProbeCheck(false, TEXT("still busy after getting out")); R.Phase = 7; R.Since = R.Now; } return true; }
			if (!P->TestMove)
			{
				P->Pitch = -.06; P->TestMove = FVector2D(0, -1); P->bTestRun = true;
				ProbeReset();
				R.Since = R.Now;
				return true;
			}
			if (R.Now - R.SampleAt >= .25) { R.SampleAt = R.Now; ProbeSample(M, P->Pos, P->FloorY); }
			if (R.Now - R.Since < 8) return true;
			P->TestMove.Reset(); P->bTestRun = false;
			ProbeCheck(R.Void == 0 && !R.bOver && R.Samples > 10, FString::Printf(TEXT("walk from the truck: %d samples, void %d, floor off the meshes at most %.3f m at road %.1f %.1f (%s)"), R.Samples, R.Void, R.Worst, R.WorstX, R.WorstZ, *R.WorstWhat));
			R.Phase = 7; R.Since = R.Now;
			return true;
		}

		// The views. A place, then two and a half seconds for the textures, and the picture the frame after it is asked for.
		if (R.Phase == 7)
		{
			if (R.HourAt >= (R.bEdgeOnly ? UE_ARRAY_COUNT(EdgeHours) : UE_ARRAY_COUNT(ReliefHours))) return ProbeFinish();
			ProbeSetHour(M, R.bEdgeOnly ? EdgeHours[R.HourAt] : ReliefHours[R.HourAt]);
			R.ViewAt = 0; R.Phase = 8; R.Since = R.Now;
			return true;
		}
		if (R.Phase == 8)
		{
			if (R.ViewAt >= (R.bEdgeOnly ? UE_ARRAY_COUNT(EdgeViews) : UE_ARRAY_COUNT(ReliefViews))) { ++R.HourAt; R.Phase = 7; R.Since = R.Now; return true; }
			const FReliefView& View = R.bEdgeOnly ? EdgeViews[R.ViewAt] : ReliefViews[R.ViewAt];
			if (Life->Area != View.Area) { Life->Enter(View.Area); R.Since = R.Now; return true; }
			const FVector Eye = S47FarmWorld::RoadToArea(View.Area, FVector(View.X, 0, View.Z)), Look = S47FarmWorld::RoadToArea(View.Area, FVector(View.LookX, 0, View.LookZ));
			const FVector Along(Look.X - Eye.X, View.Lift, Look.Z - Eye.Z);
			P->Place(Eye.X, Eye.Z, FMath::Atan2(-Along.X, -Along.Z));
			P->Pitch = FMath::Atan2(Along.Y, FVector2D(Along.X, Along.Z).Size());
			R.FrameSum = 0; R.FrameMost = 0; R.FrameCount = 0;
			R.Phase = 9; R.Since = R.Now;
			return true;
		}
		if (R.Phase == 9)
		{
			// Over the settled second and a half only: the first second carries the jump's own streaming.
			if (R.Now - R.Since > 1) { R.FrameSum += Dt; R.FrameMost = FMath::Max(R.FrameMost, double(Dt)); ++R.FrameCount; }
			if (R.Now - R.Since < 2.5) return true;
			const FReliefView& View = R.bEdgeOnly ? EdgeViews[R.ViewAt] : ReliefViews[R.ViewAt];
			const int32 Hour = FMath::FloorToInt(R.bEdgeOnly ? EdgeHours[R.HourAt] : ReliefHours[R.HourAt]);
			const FString Shot = FString::Printf(TEXT("relief-%02d-%s.png"), Hour, View.Name);
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("AlbumTest") / Shot, true, false);
			double Rx = 0, Rz = 0;
			ProbeRoadXZ(Life->Area, P->Pos.X, P->Pos.Z, Rx, Rz);
			// The eye stands on the walker's floor: a floor gone stale takes the picture from under or just over the ground.
			double GroundY = 0;
			const bool bGround = ProbeGroundY(W, P->Pos, P->FloorY, GroundY);
			ProbeLine(FString::Printf(TEXT("%s: frame %.1f ms on average, %.1f ms at worst over %d frames; Off under the eye %.2f m, floor %.3f, ground mesh %.3f, eye %.2f m over it"), *Shot, R.FrameCount ? R.FrameSum / R.FrameCount * 1000 : 0., R.FrameMost * 1000, R.FrameCount, S47Relief::Off(Rx, Rz), P->FloorY, GroundY, P->FloorY + P->Eye - GroundY));
			// (the station's own ground is none of the meshes this looks in)
			if (Life->Area != TEXT("station01") && (!bGround || FMath::Abs(GroundY - P->FloorY) > ReliefProbeEye)) ProbeCheck(false, FString::Printf(TEXT("%s: the eye does not stand on the ground"), *Shot));
			if (R.HourAt == 0) ProbeUnder(W, Shot, P->Pos, P->FloorY);
			R.Phase = 10; R.Since = R.Now;
			return true;
		}
		if (R.Phase == 10)
		{
			// The picture is written in the next frame; nothing moves before then.
			if (R.Now - R.Since < .3) return true;
			++R.ViewAt; R.Phase = 8; R.Since = R.Now;
			return true;
		}
		return ProbeFinish(TEXT("lost its place"));
	}
}
