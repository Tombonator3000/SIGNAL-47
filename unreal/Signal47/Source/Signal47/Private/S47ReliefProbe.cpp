#include "S47ReliefProbe.h"
#include "S47Drive.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47FreeformTerrain.h"
#include "S47Geo.h"
#include "S47GroundCover.h"
#include "S47Main.h"
#include "S47Relief.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ReliefProbe, Log, All);

namespace
{
	constexpr double ReliefProbeLimit = .15;   // the floor off the meshes, in metres
	constexpr double ReliefProbeLive = .05;    // ground this far off flat counts as having seen the relief

	// Road-local eye and what it looks at. The array's dishes stand at x -230..210, z -420..-42 in SARO's terms (road x + 24,
	// z - 300), the antenna view is from its west side. No mesa foot is known by a name: this one is the farthest open land west.
	struct FReliefView { const TCHAR* Name; const TCHAR* Area; double X, Z, LookX, LookZ, Lift; };
	const FReliefView ReliefViews[] = {
		{ TEXT("road"), TEXT("road"), -40, 900, -400, 880, 2 },
		{ TEXT("town"), TEXT("road"), -40, 1640, -10, 2000, 4 },
		{ TEXT("array"), TEXT("saro"), -320, -500, 0, -520, 6 },
		{ TEXT("farm"), TEXT("saro"), 25, 300, 100, 215, 2 },
		{ TEXT("mesa"), TEXT("road"), -1300, 1000, -2600, 1000, 30 },
	};
	const double ReliefHours[] = { 11, 18.6, 23.5 };
	// Road-local again. The road area walks z over 450; SARO's within 600 m of the site, where the table may well be 0 all the
	// way (its mask): that walk proves the ring is under the feet and need not have met any relief.
	struct FReliefWalk { const TCHAR* Area; double X, Z, Bearing; bool bNeedRelief; };
	const FReliefWalk ReliefWalks[] = { { TEXT("road"), -300, 1000, 0, true }, { TEXT("road"), -700, 1400, 1.2, true }, { TEXT("saro"), -426, -400, 0, false } };

	struct FReliefRun
	{
		int32 Phase = 0, WalkAt = 0, HourAt = 0, ViewAt = 0;
		double Now = 0, Since = 0, SampleAt = 0;
		int32 Void = 0, Samples = 0;
		double Worst = 0, Seen = 0;                 // the current walk or trip
		double DriveTravel = 0, TruckOff = 0;
		FVector LastTruck = FVector::ZeroVector;
		bool bAllGood = true, bHolding = false, bLookDone = false;
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
	void ProbeSample(US47Main* M, const FVector& At, double Floor)
	{
		FReliefRun& R = ReliefRun;
		++R.Samples;
		FHitResult Hit;
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(S47ReliefProbe), true);
		UWorld* World = M->Web->GetWorld();
		if (!(World && World->LineTraceSingleByChannel(Hit, S47Three::ToUE(FVector(At.X, Floor + 2, At.Z)), S47Three::ToUE(FVector(At.X, Floor - 6, At.Z)), ECC_Visibility, Query))) ++R.Void;
		else R.Worst = FMath::Max(R.Worst, FMath::Abs(Hit.ImpactPoint.Z / 100.0 - Floor));
		double Rx = 0, Rz = 0;
		ProbeRoadXZ(M->Web->Area(), At.X, At.Z, Rx, Rz);
		R.Seen = FMath::Max(R.Seen, FMath::Abs(S47Relief::Off(Rx, Rz)));
	}
	void ProbeReset()
	{
		FReliefRun& R = ReliefRun;
		R.Void = 0; R.Samples = 0; R.Worst = 0; R.Seen = 0; R.SampleAt = R.Now;
	}
	// The relief mesh itself against the floor, one component, at random points where the table is live.
	void ProbeMesh(const TCHAR* Name, UStaticMeshComponent* Comp, TFunctionRef<bool(FRandomStream&, FVector&, double&)> NextPoint)
	{
		if (!Comp) { ProbeCheck(false, FString::Printf(TEXT("%s ground has no component"), Name)); return; }
		FRandomStream Random(47203);
		int32 Tested = 0, Missed = 0;
		double Largest = 0;
		const FCollisionQueryParams Query(SCENE_QUERY_STAT(S47ReliefProbeMesh), true);
		for (int32 I = 0; I < 12000 && Tested < 3000; ++I)
		{
			FVector At; double Floor = 0;
			if (!NextPoint(Random, At, Floor)) continue;
			++Tested;
			FHitResult Hit;
			if (!Comp->LineTraceComponent(Hit, S47Three::ToUE(FVector(At.X, Floor + 3, At.Z)), S47Three::ToUE(FVector(At.X, Floor - 6, At.Z)), Query)) ++Missed;
			else Largest = FMath::Max(Largest, FMath::Abs(Hit.ImpactPoint.Z / 100.0 - Floor));
		}
		ProbeCheck(Tested > 100 && Missed == 0 && Largest < ReliefProbeLimit, FString::Printf(TEXT("%s mesh against the floor: %d points on live ground, %d with no mesh, largest %.3f m"), Name, Tested, Missed, Largest));
	}
	void ProbeMeshes(US47Main* M)
	{
		AS47Web* W = M->Web.Get();
		const FS47WebNode* RoadNode = W->Node(1242);
		const FS47WebNode* SaroNode = W->Node(7);
		ProbeMesh(TEXT("road"), RoadNode ? Cast<UStaticMeshComponent>(RoadNode->Comp.Get()) : nullptr, [](FRandomStream& Random, FVector& At, double& Floor)
		{
			const double X = Random.FRandRange(-2900.f, 2200.f), Z = Random.FRandRange(-2600.f, 4300.f);
			if (FMath::Abs(S47Relief::Sample(X, Z)) < ReliefProbeLive) return false;
			At = FVector(X + 8000, 0, Z); Floor = S47RoadTerrain::SurfaceY(X, Z);
			return true;
		});
		UStaticMeshComponent* SaroComp = SaroNode ? Cast<UStaticMeshComponent>(SaroNode->Comp.Get()) : nullptr;
		const FBox Ring = SaroComp ? SaroComp->Bounds.GetBox() : FBox(ForceInit);
		const double Reach = SaroComp ? FMath::Min(Ring.GetExtent().X, Ring.GetExtent().Y) * .9 / 100 : 0;
		const FVector Middle = SaroComp ? S47Three::ToThree(Ring.GetCenter()) : FVector::ZeroVector;
		ProbeMesh(TEXT("SARO"), SaroComp, [Reach, Middle](FRandomStream& Random, FVector& At, double& Floor)
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
			ProbeMeshes(M);
			R.Phase = 1; R.Since = R.Now;
			return true;
		}

		if (R.Phase == 1) // a walk: into its area, stood at its start
		{
			if (R.WalkAt >= UE_ARRAY_COUNT(ReliefWalks)) { R.Phase = 3; R.Since = R.Now; return true; }
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
			ProbeCheck(R.Void == 0 && R.Worst < ReliefProbeLimit && (R.Seen >= ReliefProbeLive || !ReliefWalks[R.WalkAt].bNeedRelief) && R.Samples > 10,
				FString::Printf(TEXT("walk %d in %s: %d samples, void %d, floor off the meshes at most %.3f m, largest relief under it %.2f m"), R.WalkAt, *Life->Area, R.Samples, R.Void, R.Worst, R.Seen));
			++R.WalkAt; R.Phase = 1; R.Since = R.Now;
			return true;
		}

		if (R.Phase == 3) // the trip: out on the open land, straight on at a moderate speed
		{
			FS47TripOpts Opts;
			const double Sx = -900, Sz = 1500;
			Opts.AtPos = FVector(8000 + Sx, S47RoadTerrain::SurfaceY(Sx, Sz), Sz); Opts.AtHeading = UE_DOUBLE_PI; Opts.Leg = TEXT("road");
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
				ProbeSample(M, Cab->Pos, Floor);
				R.TruckOff = FMath::Max(R.TruckOff, FMath::Abs(Cab->Pos.Y - Floor));
				if (!R.LastTruck.IsZero()) R.DriveTravel += FVector2D::Distance(FVector2D(Cab->Pos.X, Cab->Pos.Z), FVector2D(R.LastTruck.X, R.LastTruck.Z));
				R.LastTruck = Cab->Pos;
			}
			if (R.Phase == 4)
			{
				if (R.Now - R.Since < 16) return true;
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
			ProbeCheck(R.Void == 0 && R.Worst < ReliefProbeLimit && R.Seen >= ReliefProbeLive && R.DriveTravel > 40,
				FString::Printf(TEXT("trip: %.0f m, %d samples, void %d, floor off the meshes at most %.3f m, largest relief under it %.2f m (truck's own height off the floor at most %.3f m, for reading only)"), R.DriveTravel, R.Samples, R.Void, R.Worst, R.Seen, R.TruckOff));
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
			ProbeCheck(R.Void == 0 && R.Worst < ReliefProbeLimit && R.Samples > 10, FString::Printf(TEXT("walk from the truck: %d samples, void %d, floor off the meshes at most %.3f m"), R.Samples, R.Void, R.Worst));
			R.Phase = 7; R.Since = R.Now;
			return true;
		}

		// The views. A place, then two and a half seconds for the textures, and the picture the frame after it is asked for.
		if (R.Phase == 7)
		{
			if (R.HourAt >= UE_ARRAY_COUNT(ReliefHours)) return ProbeFinish();
			ProbeSetHour(M, ReliefHours[R.HourAt]);
			R.ViewAt = 0; R.Phase = 8; R.Since = R.Now;
			return true;
		}
		if (R.Phase == 8)
		{
			if (R.ViewAt >= UE_ARRAY_COUNT(ReliefViews)) { ++R.HourAt; R.Phase = 7; R.Since = R.Now; return true; }
			const FReliefView& View = ReliefViews[R.ViewAt];
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
			const FReliefView& View = ReliefViews[R.ViewAt];
			const int32 Hour = FMath::FloorToInt(ReliefHours[R.HourAt]);
			const FString Shot = FString::Printf(TEXT("relief-%02d-%s.png"), Hour, View.Name);
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("AlbumTest") / Shot, true, false);
			double Rx = 0, Rz = 0;
			ProbeRoadXZ(Life->Area, P->Pos.X, P->Pos.Z, Rx, Rz);
			ProbeLine(FString::Printf(TEXT("%s: frame %.1f ms on average, %.1f ms at worst over %d frames; Off under the eye %.2f m, floor %.3f"), *Shot, R.FrameCount ? R.FrameSum / R.FrameCount * 1000 : 0., R.FrameMost * 1000, R.FrameCount, S47Relief::Off(Rx, Rz), P->FloorY));
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
