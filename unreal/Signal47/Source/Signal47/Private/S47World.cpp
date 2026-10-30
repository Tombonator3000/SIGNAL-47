#include "S47World.h"
#include "S47Spine.h"
#include "S47Gamepad.h"
#include "S47Evening.h"
#include "S47EveningVisual.h"
#include "S47Timeline.h"
#include "S47Keys.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "S47Audio.h"
#include "S47FieldCamera.h"
#include "S47FreeModels.h"
#include "S47Props.h"
#include "S47Geo.h"
#include "S47GroundCover.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47Obj.h"
#include "S47OldRoad.h"
#include "S47RoadArea.h"
#include "S47RoadShape.h"
#include "S47RoadTerrain.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "TimerManager.h"
#include "UnrealClient.h"

#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogS47World, Log, All);

namespace
{
	// Unreal-only visitor lot. The exported service corridors and obstacles stay authoritative
	// elsewhere; enable this extension only when its native geometry/floors validated.
	class FArrivalSaroDrive final : public FS47SaroDrive
	{
	public:
		using FS47SaroDrive::FS47SaroDrive;
		bool bVisitorLot = false;
		bool OnVisitorLot(double X, double Z) const
		{
			return bVisitorLot && ((X >= -18.2 && X <= -7.8 && Z >= 10.5 && Z <= 20.5)
				|| (X >= -16.5 && X <= -9.5 && Z >= 20.5 && Z <= 23.5));
		}
		virtual bool Blocked(double X, double Z) override
		{
			return OnVisitorLot(X,Z) ? false : FS47SaroDrive::Blocked(X,Z);
		}
		virtual FS47Surface Surface(double X, double Z) const override
		{
			return OnVisitorLot(X,Z) ? FS47SaroDrive::Surface(8.9,8) : FS47SaroDrive::Surface(X,Z);
		}
	};
	// world/World.ts: FOG and FOG_OLDROAD (the old road's air is thinner at dawn)
	constexpr float FOG = 0.0021f, FOG_OLDROAD = 0.00055f;
	const FVector ROAD_ORIGIN(8000, 0, 0);
	const FVector STATION_ORIGIN(0, 0, 8000);
	FS47Obstacle ClosedRanchGate(const FVector& RoadToArea)
	{
		// roadProps.ts ranchRoad: the closed tube gate spans RANCH_GAP (+/-3.4 m)
		// between the exported radius-0.2 posts; its widest tube has radius 0.028 m.
		// Unreal's SARO drive corridor bounds only the centre, so give the existing
		// oriented-footprint collision the physical gate in both coordinate frames.
		const double X = S47RoadTerrain::HWY.Fence + RoadToArea.X;
		const double Z = S47RoadTerrain::RanchZ(S47RoadTerrain::HWY.Fence) + RoadToArea.Z;
		return FS47Obstacle::Box(X - 0.028, X + 0.028, Z - 3.4, Z + 3.4);
	}
	// Mesa Diner stands in the road area, beside the highway 2 km south of the start (roadShape.ts)
	FVector DINER_ORIGIN() { const S47RoadShape::FDinerAt& D = S47RoadShape::DINER(); return FVector(8000 + D.X, D.Y + 0.1, D.Z); }
	/** The places the truck can be left on (a lot with a park box): SARO's pad, the motel's lot,
	 *  the gravel outside STATION 01's gate, the diner's lot. */
	const TCHAR* PLACES[4] = { TEXT("saro"), TEXT("motel"), TEXT("station01"), TEXT("diner") };
	/** The places with a truck of their own (the motel's is SARO's: the same area), and the old road,
	 *  where chapter six's truck is left when the player gets out on it (Tom 22 October 2026). */
	const TCHAR* TRUCKS[4] = { TEXT("saro"), TEXT("station01"), TEXT("diner"), TEXT("roswell") };
	/** How far from the truck the ground round it is walked, where its area has no walk zones by it. */
	constexpr double GROUND_R = 30;
	/** What the open land keeps from a building, a raised slab or a fence line besides the player's
	 *  width: more than any step, so the way in is only by the zones and their gates. */
	constexpr double LAND_KEEP = 0.4;
	/** The area a place is in (the motel's lot is SARO's). */
	FString AreaOf(const FString& L) { return L == TEXT("motel") ? FString(TEXT("saro")) : L; }
	bool InBox(const TOptional<FS47Box2>& B, const FVector& P) { return B && P.X >= B->MinX && P.X <= B->MaxX && P.Z >= B->MinZ && P.Z <= B->MaxZ; }
	bool Same(const FS47Rect& R, const FS47Area& A) { return A.bValid && FMath::Abs(R.MinX - A.MinX) < 1e-3 && FMath::Abs(R.MaxX - A.MaxX) < 1e-3 && FMath::Abs(R.MinZ - A.MinZ) < 1e-3 && FMath::Abs(R.MaxZ - A.MaxZ) < 1e-3; }
	// a point turned about the up axis by h (Vector3.applyAxisAngle(UP, h))
	FVector AboutUp(const FVector& V, double H) { const double C = FMath::Cos(H), S = FMath::Sin(H); return FVector(V.X * C + V.Z * S, V.Y, -V.X * S + V.Z * C); }
	// three's Euler.setFromQuaternion in the YXZ order: (x, y, z)
	FVector EulerYXZ(const FQuat& Q)
	{
		double E[16];
		S47Three::Compose(FVector::ZeroVector, Q, FVector::OneVector, E);
		const double m11 = E[0], m13 = E[8], m21 = E[1], m22 = E[5], m23 = E[9], m31 = E[2], m33 = E[10];
		const double X = FMath::Asin(-FMath::Clamp(m23, -1.0, 1.0));
		if (FMath::Abs(m23) < 0.9999999) return FVector(X, FMath::Atan2(m13, m33), FMath::Atan2(m21, m22));
		return FVector(X, FMath::Atan2(-m31, m11), 0);
	}
	const TCHAR* EXAMINE_TRUCK = TEXT("The service truck: a white pickup with tired paint and a toolbox in the bed. The keys are in it, like always.");

	// the export's run-time data for the drive (export.js extras: drive), and its readers
	TSharedPtr<FJsonObject> JObj(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { const TSharedPtr<FJsonObject>* V = nullptr; return O.IsValid() && O->TryGetObjectField(K, V) ? *V : nullptr; }
	TArray<FS47Obstacle> ObstaclesOf(const TSharedPtr<FJsonObject>& Leg)
	{
		TArray<FS47Obstacle> Out;
		for (const TSharedPtr<FJsonValue>& V : S47J::Arr(Leg, TEXT("obstacles")))
		{
			const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
			if (A.Num() == 3) Out.Add(FS47Obstacle::Circle(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber()));
			else if (A.Num() == 4) Out.Add(FS47Obstacle::Box(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber(), A[3]->AsNumber()));
		}
		return Out;
	}
}

// ---------- the deps, the keys, the truck to use ----------

void US47World::Init(US47Main* InM)
{
	M = InM;
	if (!M) return;
	// main.ts truckChoice: the truck any time (Tom 15 October), the night's own drive if a chapter
	// has one from here, otherwise one of the player's own; not while the shift's first hours are
	// still running. (The chapters' truckTrip, ch3, ch4 and ch5, go in front of the player's own
	// drive as they are ported.)
	if (!TruckChoice)
	{
		TruckChoice = [this](const FString& Place) -> FChoice
		{
			const bool EarlyReferenceTrip = M && M->Story && M->Story->bEveningTimeline && M->Story->Phase() == ES47Phase::Shift && M->Evening && M->Evening->Pack.Flag(TEXT("handover_done"));
			// the 00:13 feeder trip is ordered whatever phase the signal has moved the night to
			const bool ServiceTrip = M && M->Evening && M->Evening->ServicePending();
			if (M && M->Story && !M->Story->At(ES47Phase::Ch1) && !EarlyReferenceTrip && !ServiceTrip)
			{
				return { TEXT("Service truck"), [this] { Toast(TEXT("The service truck. Not now: someone has to sit the console tonight."), 3.4f); } };
			}
			return { ServiceTrip ? TEXT("Drive / Caliche Junction inspection") : EarlyReferenceTrip ? TEXT("Drive / optional Caliche reference round") : TEXT("Drive"), [this, Place] { Roam(Place); } };
		};
	}
	// main.ts input.onKey and useCurrent: in the cab Use turns the key, uses the radio or gets out;
	// R is the radio
	// (the keys as bound, S47Keys: Use's and a click, the radio's)
	M->OnAction(TEXT("use"), [this] { return CabUse(); });
	M->OnKey(EKeys::LeftMouseButton, [this] { return CabUse(); });
	M->OnAction(TEXT("radio"), [this] { return CabRadio(); });
	// the truck where it stands: the yard's (Chapter3.bindSaroTruck) and the station's (bindSite);
	// the chapter adds its own with the same ids when it is ported
	if (M->Inter && M->Web)
	{
		FS47Interactable T;
		T.Id = TEXT("truck");
		T.Object = M->Web->Obj(TEXT("yard.objs.truck"));
		T.Range = 3.2;
		T.Label = [this] { if (M->Spine) { const FString L = M->Spine->Label(TEXT("truck")); if (!L.IsEmpty()) return L; } return TruckChoice ? TruckChoice(TEXT("saro")).Label : FString(); };
		T.Use = [this] { if (TruckChoice) TruckChoice(TEXT("saro")).Use(); };
		T.Examine = [] { return FString(EXAMINE_TRUCK); };
		M->Inter->Add(T);
		FS47Interactable S;
		S.Id = TEXT("s1truck");
		S.Object = M->Web->Obj(TEXT("world.site.proxies.truckSpot"));
		S.Range = 3.2;
		FS47Obj Spot = S.Object;
		S.Label = [this, Spot] { return Area == TEXT("station01") && Spot.Position().Y > -100 && TruckChoice ? TruckChoice(TEXT("station01")).Label : FString(); };
		S.Use = [this] { if (TruckChoice) TruckChoice(TEXT("station01")).Use(); };
		S.Examine = [this, Spot] { return Area == TEXT("station01") && Spot.Position().Y > -100 ? FString(EXAMINE_TRUCK) : FString(); };
		M->Inter->Add(S);
		// the truck left on the old road (out of it anywhere, Tom 22 October 2026: the web version has no
		// way out there), got into again where it stands; chapter six's drive goes on
		FS47Interactable O;
		O.Id = TEXT("oldTruck");
		O.Object = M->Web->Obj(TEXT("world.oldTruck.group"));
		O.Range = 3.2;
		auto Left = [this] { return Area == TEXT("roswell") && TruckAt == TEXT("roswell") && !bDriving && !bBusy; };
		O.Label = [Left] { return Left() ? FString(TEXT("Drive")) : FString(); };
		O.Use = [this, Left] { if (Left()) DriveOnOldRoad(); };
		O.Examine = [Left] { return Left() ? FString(EXAMINE_TRUCK) : FString(); };
		if (O.Object) M->Inter->Add(O);
	}
	// the view from the cab while the truck is driven
	if (M->Player) M->Player->CameraOverride = [this](double Dt, const FVector2D& Move, const FVector2D& LookIn, FS47View& Out) { return Frame(Dt, Move, LookIn, Out); };
	// the open land of the area the player is in (Tom 22 October 2026: walk round freely there)
	if (M->Player)
	{
		TWeakObjectPtr<US47World> Self = this;
		M->Player->OpenLand = [Self](double X, double Z) { return Self.IsValid() && Self->OnLand(X, Z); };
		// (its ground as it is drawn there; in the zones the area's floor as before: Grounds.floorAt,
		// Town.floorAt, or read off the world at STATION 01)
		M->Player->LandFloor = [Self](double X, double Z) { return Self.IsValid() ? Self->LandY(X, Z) : std::numeric_limits<double>::quiet_NaN(); };
	}
	// a jump on foot (Tom 22 October 2026): not in the truck, not with a panel open or the story holding
	// the player, not with the field camera raised (Space is its shutter then)
	if (M->Player) M->Player->CanJump = [this]
	{
		return M && !bDriving && !bBusy && !M->ModalOpen() && !(M->Story && M->Story->IsCinematic()) && !(M->FieldCamera && M->FieldCamera->bRaised);
	};

	// Tests: -S47Drive=<from>:<to> starts a story trip as soon as the game is in play;
	// -S47Throttle=<0..1> holds the throttle with the autopilot steering (World.testInput, autoInput);
	// -S47Auto=<m/s> drives by itself the whole way (World.autopilot, the developer menu's).
	FString DriveArg;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47Drive="), DriveArg) && M->Web)
	{
		float Throttle = -1.f, AutoSpeed = -1.f;
		FParse::Value(FCommandLine::Get(), TEXT("S47Throttle="), Throttle);
		FParse::Value(FCommandLine::Get(), TEXT("S47Auto="), AutoSpeed);
		FString From, To;
		if (!DriveArg.Split(TEXT(":"), &From, &To)) { From = TruckAt; To = DriveArg; }
		TWeakObjectPtr<US47World> Self = this;
		FTimerHandle H;
		M->Web->GetWorldTimerManager().SetTimer(H, [Self, From, To, Throttle, AutoSpeed]
		{
			if (!Self.IsValid()) return;
			US47World* W = Self.Get();
			W->EnsureLegs();
			// (from 'road': the drive out started on the road, World.driveOut(true))
			const bool bOnRoad = From == TEXT("road");
			if (!bOnRoad && From != W->TruckAt) W->PlaceTruck(From);
			if (Throttle >= 0) W->TestInput = [W, Throttle] { FS47DriveIn A = W->AutoInput(0); A.Throttle = Throttle; return A; };
			if (AutoSpeed > 0) W->Autopilot = AutoSpeed;
			UE_LOG(LogS47World, Display, TEXT("S47: test drive %s -> %s"), *From, *To);
			if (To == TEXT("station01")) W->DriveOut(bOnRoad);
			else if (To == TEXT("saro")) W->DriveBack();
			else if (To == TEXT("motel")) W->DriveToMotel(nullptr);
			else if (To == TEXT("diner")) W->DriveToDiner(nullptr);
			else if (To == TEXT("roswell")) W->DriveOldRoad([] {});
			else W->Roam(From);
		}, 1.0f, false);
	}
	// -S47GetOut[=<s>]: that many seconds into a trip (12 by default) the truck is braked to a stop
	// where it is and the player gets out (with -S47OffRoad=<s>, first that long hard right at half
	// throttle, off the way); the log says where they are put and what they can walk, then they walk
	// away, come back to the truck and get in again (Tom 22 October 2026)
	float GetOutAt = 0;
	if ((FParse::Value(FCommandLine::Get(), TEXT("S47GetOut="), GetOutAt) || FParse::Param(FCommandLine::Get(), TEXT("S47GetOut"))) && M->Web) TestGetOut(GetOutAt > 0 ? GetOutAt : 12);
	// -S47Walk=<s>[,<bearing in degrees>]: from 3 s on the player runs that long straight ahead (or on
	// that bearing, three's yaw), wherever they stand (-S47Face=at:..., or -S47WalkFrom=<area>:<x>;<z>),
	// and the log says how far they got, the floor and whether the area's ground meshes were under
	// every step (the open land's edges)
	FString WalkFrom;
	const bool bWalkFrom = FParse::Value(FCommandLine::Get(), TEXT("S47WalkFrom="), WalkFrom, false);
	FString WalkArg;
	if (FParse::Value(FCommandLine::Get(), TEXT("S47Walk="), WalkArg, false) && M->Web && M->Player)
	{
		FString A, B;
		if (!WalkArg.Split(TEXT(","), &A, &B)) A = WalkArg;
		const double Secs = FMath::Max(1.0, FCString::Atod(*A));
		const TOptional<double> Bearing = B.IsEmpty() ? TOptional<double>() : TOptional<double>(FMath::DegreesToRadians(FCString::Atod(*B)));
		struct FWalkTest { double T = 0, Far = 0, Off = 0; int32 Void = 0, Stuck = 0; FVector Start = FVector::ZeroVector, Last = FVector::ZeroVector; bool bOn = false; };
		TSharedRef<FWalkTest> Wk = MakeShared<FWalkTest>();
		TWeakObjectPtr<US47World> Self = this;
		FTimerHandle H;
		const FString WalkFromArg = bWalkFrom ? WalkFrom : FString();
		M->Web->GetWorldTimerManager().SetTimer(H, [Self, Wk, Secs, Bearing, WalkFromArg]
		{
			if (!Self.IsValid() || !Self->M || !Self->M->Player || Wk->T > Secs + 1) return;
			AS47Walker* P = Self->M->Player;
			if (!Wk->bOn)
			{
				FString FromArea, FromAt, Xs, Zs;
				if (!WalkFromArg.IsEmpty() && WalkFromArg.Split(TEXT(":"), &FromArea, &FromAt) && FromAt.Split(TEXT(";"), &Xs, &Zs))
				{
					Self->Enter(FromArea);
					P->Place(FCString::Atod(*Xs), FCString::Atod(*Zs), P->Yaw);
				}
				Wk->bOn = true; Wk->Start = Wk->Last = P->Pos;
				if (Bearing) P->Yaw = *Bearing;
				P->Pitch = -0.06; P->TestMove = FVector2D(0, -1); P->bTestRun = true;
				UE_LOG(LogS47World, Display, TEXT("S47: walk test from %.1f %.1f in %s, yaw %.2f, floor %.3f, %s"), P->Pos.X, P->Pos.Z, *Self->Area, P->Yaw, P->FloorY, P->Walkable(P->Pos.X, P->Pos.Z) ? TEXT("walkable") : TEXT("NOT WALKABLE"));
				return;
			}
			Wk->T += 0.25;
			FHitResult Hit;
			const FCollisionQueryParams Q(SCENE_QUERY_STAT(S47WalkTest), true);
			UWorld* Wd = Self->M->Web->GetWorld();
			if (!(Wd && Wd->LineTraceSingleByChannel(Hit, S47Three::ToUE(FVector(P->Pos.X, P->FloorY + 2, P->Pos.Z)), S47Three::ToUE(FVector(P->Pos.X, P->FloorY - 6, P->Pos.Z)), ECC_Visibility, Q)))
				++Wk->Void;
			else Wk->Off = FMath::Max(Wk->Off, FMath::Abs(Hit.ImpactPoint.Z / 100.0 - P->FloorY));
			const double D = FVector2D::Distance(FVector2D(P->Pos.X, P->Pos.Z), FVector2D(Wk->Start.X, Wk->Start.Z));
			Wk->Far = FMath::Max(Wk->Far, D);
			Wk->Stuck = FVector2D::Distance(FVector2D(P->Pos.X, P->Pos.Z), FVector2D(Wk->Last.X, Wk->Last.Z)) < 0.05 ? Wk->Stuck + 1 : 0;
			Wk->Last = P->Pos;
			if (FMath::FloorToInt(Wk->T / 5) != FMath::FloorToInt((Wk->T - 0.25) / 5) || Wk->T >= Secs)
			{
				UE_LOG(LogS47World, Display, TEXT("S47: walk test at %.1f s: %.1f %.1f, %.1f m from the start%s, floor %.3f"), Wk->T, P->Pos.X, P->Pos.Z, D, Wk->Stuck >= 4 ? TEXT(" (held)") : TEXT(""), P->FloorY);
			}
			if (Wk->T >= Secs)
			{
				P->TestMove.Reset(); P->bTestRun = false;
				UE_LOG(LogS47World, Display, TEXT("S47: walk test done: %.1f m at most from the start, ended at %.1f %.1f%s; ground meshes under every step: %s (worst floor off the meshes %.2f m)"),
					Wk->Far, P->Pos.X, P->Pos.Z, Wk->Stuck >= 4 ? TEXT(", held there") : TEXT(""), Wk->Void ? TEXT("NO") : TEXT("yes"), Wk->Off);
			}
		}, 0.25f, true, 3.0f);
	}
	// -S47Hop[=<n>]: the walker jumps n times (3 by default) from 3 s on, 1.6 s apart, the second while
	// walking and the last by the Jump key; then once more with the field camera raised (refused)
	int32 Hops = 0;
	if ((FParse::Value(FCommandLine::Get(), TEXT("S47Hop="), Hops) || FParse::Param(FCommandLine::Get(), TEXT("S47Hop"))) && M->Web && M->Player)
	{
		Hops = Hops > 0 ? Hops : 3;
		TWeakObjectPtr<US47World> Self = this;
		for (int32 i = 0; i <= Hops; ++i)
		{
			FTimerHandle H;
			M->Web->GetWorldTimerManager().SetTimer(H, [Self, i, Hops]
			{
				if (!Self.IsValid() || !Self->M || !Self->M->Player) return;
				AS47Walker* P = Self->M->Player;
				P->TestMove.Reset();
				if (i == Hops)
				{
					// with the camera raised Space is the shutter: no jump
					US47FieldCamera* F = Self->M->FieldCamera;
					if (F) F->Raise(true);
					P->Jump();
					UE_LOG(LogS47World, Display, TEXT("S47: hop with the camera %s: %s"), F && F->bRaised ? TEXT("raised") : TEXT("missing"), P->InAir() ? TEXT("jumped (wrong)") : TEXT("refused"));
					if (F) F->Raise(false);
					return;
				}
				if (i == 1) P->TestMove = FVector2D(0, -1);
				if (i == Hops - 1 && i > 0) { S47Keys::TestPresses(TEXT("SpaceBar"), 0.01f); UE_LOG(LogS47World, Display, TEXT("S47: hop %d by the Jump key (%s)"), i + 1, *S47Keys::Label(TEXT("jump"))); }
				else { P->Jump(); UE_LOG(LogS47World, Display, TEXT("S47: hop %d%s: %s"), i + 1, i == 1 ? TEXT(", walking") : TEXT(""), P->InAir() ? TEXT("up") : TEXT("refused")); }
			}, 3.0f + 1.6f * i, false);
		}
	}
	// -S47FreeModels[=name,...]: the free models in a row in front of the player (Unreal only)
	S47FreeModels::Test(M);
	AS47Props::TestPlace(M); // -S47PropsAt=<area>:x;z;yaw;pitch (Unreal only)
}

void US47World::TestGetOut(double At)
{
	float OffRoad = 0;
	FParse::Value(FCommandLine::Get(), TEXT("S47OffRoad="), OffRoad);
	TWeakObjectPtr<US47World> Self = this;
	struct FTest
	{
		int32 Stage = 0; double T = 0, Since = 0, Last = 1e9; bool bOff = false; FString Id; FVector Truck = FVector::ZeroVector;
		double Wander = 3, Bearing = TNumericLimits<double>::Max(), Far = 0, Off = 0; int32 Void = 0; FString Shot; TArray<FVector2D> Crumbs;
	};
	TSharedRef<FTest> S = MakeShared<FTest>();
	// -S47Wander=<s>[,<bearing in degrees>]: the walk away from the truck that long, running, on that
	// bearing (three's yaw; away from the truck by default); the picture at the far end beside -S47Shot's
	{
		FString Wander;
		if (FParse::Value(FCommandLine::Get(), TEXT("S47Wander="), Wander, false))
		{
			FString A, B;
			if (!Wander.Split(TEXT(","), &A, &B)) A = Wander;
			S->Wander = FMath::Max(1.0, FCString::Atod(*A));
			if (!B.IsEmpty()) S->Bearing = FMath::DegreesToRadians(FCString::Atod(*B));
			FString Shot;
			if (FParse::Value(FCommandLine::Get(), TEXT("S47Shot="), Shot)) S->Shot = FPaths::GetBaseFilename(Shot, false) + TEXT("-far.png");
		}
	}
	FTimerHandle H;
	M->Web->GetWorldTimerManager().SetTimer(H, [Self, S, At, OffRoad]
	{
		if (!Self.IsValid() || !Self->M || !Self->M->Player) return;
		US47World* W = Self.Get();
		AS47Walker* P = W->M->Player;
		S->T += 0.25;
		auto Dist = [&] { return FMath::Sqrt(FMath::Square(P->Pos.X - S->Truck.X) + FMath::Square(P->Pos.Z - S->Truck.Z)); };
		auto Face = [&](bool bAway)
		{
			const double Dx = S->Truck.X - P->Pos.X, Dz = S->Truck.Z - P->Pos.Z;
			P->Yaw = FMath::Atan2(-Dx, -Dz) + (bAway ? UE_DOUBLE_PI : 0);
			P->Pitch = bAway ? -0.06 : FMath::Atan2(S->Truck.Y + 0.9 - (P->FloorY + P->Eye), FMath::Max(0.5, FMath::Sqrt(Dx * Dx + Dz * Dz)));
		};
		switch (S->Stage)
		{
		case 0: // driving: off the way first, then braked to a stop
		{
			const TOptional<double> Secs = W->TripSecs();
			if (!W->bDriving || !Secs) return;
			if (OffRoad > 0 && *Secs >= At - OffRoad && *Secs < At && !S->bOff)
			{
				S->bOff = true; W->Autopilot.Reset();
				W->TestInput = [] { FS47DriveIn A; A.Steer = 1; A.Throttle = 0.5; return A; };
				UE_LOG(LogS47World, Display, TEXT("S47: test get out: off the way for %.1f s"), OffRoad);
			}
			if (*Secs < At) return;
			W->Autopilot.Reset();
			W->TestInput = [W] { FS47DriveIn A; const double V = W->DrivingSpeed(); A.Throttle = V > 0.15 ? -1 : V < -0.15 ? 1 : 0; return A; };
			UE_LOG(LogS47World, Display, TEXT("S47: test get out: braking at %.1f m/s, leg %s"), W->DrivingSpeed(), *W->Leg);
			S->Stage = 1; S->Since = S->T;
			return;
		}
		case 1: // stopped: out
		{
			if (!W->CanGetOut())
			{
				if (S->T - S->Since > 6 && FMath::Abs(W->DrivingSpeed()) < 0.6)
				{
					UE_LOG(LogS47World, Warning, TEXT("S47: test get out: FAILED, refused (busy %d, wants the key %d, chapter six's %d, trip %d)"), W->bBusy, W->Trouble.WantsKey(), W->OldControl.IsSet(), W->Trip.IsSet());
					S->Stage = 9;
				}
				return;
			}
			const FS47Truck* T = W->CabTruck();
			if (T) S->Truck = T->Position;
			const double H = T ? T->Rotation.Y : 0;
			const FString From = W->Leg;
			W->TestInput = nullptr;
			W->GetOutHere();
			UE_LOG(LogS47World, Display, TEXT("S47: test get out: out of the truck in leg %s, truck at %.2f %.2f %.2f heading %.2f"), *From, S->Truck.X, S->Truck.Y, S->Truck.Z, H);
			S->Stage = 2; S->Since = S->T;
			return;
		}
		case 2: // put down: where, on what, and what is walked round them
		{
			if (S->T - S->Since < 1.0 || W->bBusy || (W->M->Story && W->M->Story->IsCinematic())) return;
			auto Round = [&](double R) { int32 N = 0; for (int32 k = 0; k < 16; ++k) { const double A = k * UE_DOUBLE_PI / 8; N += P->Walkable(S->Truck.X + FMath::Sin(A) * R, S->Truck.Z + FMath::Cos(A) * R) ? 1 : 0; } return N; };
			const double Floor = P->Floor ? P->Floor(P->Pos.X, P->Pos.Z) : P->FloorY;
			UE_LOG(LogS47World, Display, TEXT("S47: test get out: put at %s in %s: player %.2f %.2f, %.2f m from the truck's middle, floor %.3f (ground there %.3f, truck %.3f), walkable %s, ground round the truck %s; walkable round it of 16: %d at 3 m, %d at 10 m, %d at 28 m, %d at 33 m"),
				*W->TruckAt, *W->Area, P->Pos.X, P->Pos.Z, Dist(), P->FloorY, Floor, S->Truck.Y, P->Walkable(P->Pos.X, P->Pos.Z) ? TEXT("yes") : TEXT("NO"), P->Ground && P->GroundArea == W->Area ? TEXT("yes") : TEXT("no"), Round(3), Round(10), Round(28), Round(33));
			Face(true);
			if (S->Bearing < TNumericLimits<double>::Max()) P->Yaw = S->Bearing;
			P->TestMove = FVector2D(0, -1);
			P->bTestRun = S->Wander > 3;
			S->Stage = 3; S->Since = S->T;
			return;
		}
		case 3: // away from it (-S47Wander: that long, running, on a bearing), the way taken kept to come back by
		{
			if (P->TestMove) S->Crumbs.Add(FVector2D(P->Pos.X, P->Pos.Z));
			// the floor under the feet, and whether the area's ground meshes are under them (never the void)
			{
				FHitResult Hit;
				const FCollisionQueryParams Q(SCENE_QUERY_STAT(S47Wander), true);
				const bool bGround = W->M->Web && W->M->Web->GetWorld() && W->M->Web->GetWorld()->LineTraceSingleByChannel(Hit, S47Three::ToUE(FVector(P->Pos.X, P->FloorY + 2, P->Pos.Z)), S47Three::ToUE(FVector(P->Pos.X, P->FloorY - 6, P->Pos.Z)), ECC_Visibility, Q);
				if (!bGround) ++S->Void;
				else S->Off = FMath::Max(S->Off, FMath::Abs(Hit.ImpactPoint.Z / 100.0 - P->FloorY));
				S->Far = FMath::Max(S->Far, Dist());
			}
			if (FMath::FloorToInt(S->T / 5) != FMath::FloorToInt((S->T - 0.25) / 5))
			{
				UE_LOG(LogS47World, Display, TEXT("S47: test get out: wandering, at %.1f %.1f, %.1f m from the truck, floor %.2f, %s"), P->Pos.X, P->Pos.Z, Dist(), P->FloorY, P->Walkable(P->Pos.X, P->Pos.Z) ? TEXT("walkable") : TEXT("NOT WALKABLE"));
			}
			if (S->T - S->Since < S->Wander) return;
			P->TestMove.Reset(); P->bTestRun = false;
			UE_LOG(LogS47World, Display, TEXT("S47: test get out: walked away to %.2f m from the truck (farthest %.1f m), at %.1f %.1f, floor %.3f; ground meshes under every step: %s (worst floor off the meshes %.2f m)"),
				Dist(), S->Far, P->Pos.X, P->Pos.Z, P->FloorY, S->Void ? TEXT("NO") : TEXT("yes"), S->Off);
			if (!S->Shot.IsEmpty()) FScreenshotRequest::RequestScreenshot(S->Shot, true, false);
			S->Stage = 30; S->Since = S->T;
			return;
		}
		case 30: // back the way they came, to the truck
		{
			if (S->T - S->Since < 1.0) return; // (the far picture first)
			while (S->Crumbs.Num() && FVector2D::Distance(S->Crumbs.Last(), FVector2D(P->Pos.X, P->Pos.Z)) < 0.8) S->Crumbs.Pop();
			if (S->Crumbs.Num() && Dist() > 3.5 && S->T - S->Since < S->Wander * 2 + 10)
			{
				const FVector2D C = S->Crumbs.Last();
				P->Yaw = FMath::Atan2(-(C.X - P->Pos.X), -(C.Y - P->Pos.Z)); P->Pitch = -0.06;
				P->TestMove = FVector2D(0, -1); P->bTestRun = S->Wander > 3;
				return;
			}
			P->TestMove.Reset(); P->bTestRun = false;
			Face(false);
			P->TestMove = FVector2D(0, -1);
			S->Stage = 4; S->Since = S->T;
			return;
		}
		case 4: // back to it, until the truck itself stops them (or they are by it)
		{
			const double D = Dist(), Was = S->Last;
			S->Last = D;
			Face(false);
			if (S->T - S->Since < 6.0 && D > 2.0 && (S->T - S->Since < 0.6 || Was - D > 0.04)) return;
			P->TestMove.Reset();
			S->Stage = 5; S->Since = S->T;
			return;
		}
		case 5: // in reach: get in again
		{
			if (S->T - S->Since < 0.5) return;
			S->Id = P->FocusedId();
			const FString Label = S->Id.IsEmpty() || !W->M->Inter ? FString() : W->M->Inter->LabelOf(S->Id);
			UE_LOG(LogS47World, Display, TEXT("S47: test get out: back by the truck, %.2f m from its middle, floor %.3f; looking at '%s' (%s)"), Dist(), P->FloorY, *S->Id, *Label);
			if (S->Id.IsEmpty() || !W->M->Inter) { UE_LOG(LogS47World, Warning, TEXT("S47: test get out: FAILED, the truck is not in reach")); S->Stage = 9; return; }
			W->M->Inter->UseId(S->Id);
			S->Stage = 6; S->Since = S->T;
			return;
		}
		case 6:
			if (W->bDriving) { UE_LOG(LogS47World, Display, TEXT("S47: test get out: back in, driving, leg %s"), *W->Leg); S->Stage = 9; return; }
			if (S->T - S->Since > 4) { UE_LOG(LogS47World, Warning, TEXT("S47: test get out: FAILED, not back in the truck")); S->Stage = 9; }
			return;
		default: return;
		}
	}, 0.25f, true);
}

void US47World::BeginDestroy()
{
	Engine.Reset();
	Legs.Reset();
	SaroTruckP.Reset(); TruckP.Reset(); ParkedP.Reset(); OldTruckP.Reset();
	Super::BeginDestroy();
}

void US47World::Hold(bool bOn) { if (M && M->Story) M->Story->SetCinematic(bOn); }
void US47World::Fade(bool bOn) { if (M && M->UI) M->UI->Fade(bOn, TEXT("")); }
void US47World::Toast(const FString& Text, float Secs) { if (M && M->UI) M->UI->Toast(Text, Secs); }
bool US47World::FirstTime(const FString& Id)
{
	// core/hints.ts: said once (on a device there; once a run here, nothing is kept yet)
	if (Hints.Contains(Id)) return false;
	Hints.Add(Id);
	return true;
}

// ---------- the legs: each area's ground, its copy of the truck and its controller ----------

void US47World::EnsureLegs()
{
	if (bLegsMade || !M || !M->Web) return;
	bLegsMade = true;
	AS47Web* W = M->Web;
	const TSharedPtr<FJsonObject> D = W->DriveData();
	const TSharedPtr<FJsonObject> DLegs = JObj(D, TEXT("legs")), DTrucks = JObj(D, TEXT("trucks"));
	if (!D.IsValid()) UE_LOG(LogS47World, Warning, TEXT("S47: the web manifest has no drive data (export.js extras: drive): posts, poles and rocks do not stop the truck, and the cabs are unlit; re-run Tools/webexport/import.sh"));
	auto CabSet = [&](const TCHAR* Key) { return int32(S47J::Num(JObj(DTrucks, Key), TEXT("cab"), -1)); };
	// the copies of the truck, each lit by its area's floods
	SaroTruckP = MakeUnique<FS47Truck>(); SaroTruckP->Bind(M, TEXT("world.saroTruck"), CabSet(TEXT("saroTruck")), this);
	TruckP = MakeUnique<FS47Truck>(); TruckP->Bind(M, TEXT("world.legs.road.drive.truck"), CabSet(TEXT("truck")), this);
	ParkedP = MakeUnique<FS47Truck>(); ParkedP->Bind(M, TEXT("world.legs.station01.drive.truck"), CabSet(TEXT("parked")), this);
	OldTruckP = MakeUnique<FS47Truck>(); OldTruckP->Bind(M, TEXT("world.oldTruck"), CabSet(TEXT("oldTruck")), this);
	for (FS47Truck* T : { SaroTruckP.Get(), TruckP.Get(), ParkedP.Get(), OldTruckP.Get() }) if (T->DashTarget()) Keep.Add(T->DashTarget());
	// initSaro: SARO's on its pad by the service yard (YARD.truck); ensureStation: the one the
	// player came in, parked outside the gate (Station01 anchors.truck)
	SaroTruckP->SetPose(FVector(8.9, 0, 8.0), FVector(0, UE_DOUBLE_PI, 0));
	ParkedP->SetPose(FVector(S47Geo::STATION_TRUCK.X + STATION_ORIGIN.X, 0, S47Geo::STATION_TRUCK.Z + STATION_ORIGIN.Z), FVector(0, S47Geo::STATION_TRUCK.Heading, 0));
	auto Boxes = [](const TArray<FS47Rect>& Rs, int32 Skip)
	{
		TArray<FS47Box2> Out;
		for (int32 i = 0; i < Rs.Num(); ++i) if (i != Skip) Out.Add({ Rs[i].MinX, Rs[i].MaxX, Rs[i].MinZ, Rs[i].MaxZ });
		return Out;
	};
	// the trucks' own colliders in the walk lists, and their handles (World.parkAt moves them)
	{
		const FS47Area YardCol = W->Rect(TEXT("yard.truckCol")), SiteCol = W->Rect(TEXT("world.site.truckCollider"));
		const TArray<FS47Rect>& Saro = W->CollidersMutable(TEXT("saro"));
		for (int32 i = 0; i < Saro.Num() && SaroCol < 0; ++i) if (Same(Saro[i], YardCol)) SaroCol = i;
		const TArray<FS47Rect>& St = W->CollidersMutable(TEXT("station01"));
		for (int32 i = 0; i < St.Num() && StationCol < 0; ++i) if (Same(St[i], SiteCol)) StationCol = i;
		// (the diner's collider round its parked truck: moved out of the way while the truck is not on the lot)
		const FVector O = DINER_ORIGIN();
		const TArray<FS47Rect>& Di = W->CollidersMutable(TEXT("diner"));
		for (int32 i = 0; i < Di.Num() && DinerCol < 0; ++i) if (Di[i].MinX > 1e8 || (FMath::Abs(Di[i].MinX - (O.X + 7.85)) < 0.01 && FMath::Abs(Di[i].MinZ - (O.Z + 3.2)) < 0.01)) DinerCol = i;
		SaroProxy = W->Obj(TEXT("yard.objs.truck"));
		StationProxy = W->Obj(TEXT("world.site.proxies.truckSpot"));
		DinerProxy = W->Obj(TEXT("world.dinerTruckProxy")); // (an export from before 22 October has none)
		// (the old road's truck, where the player has got out of it on the old road: out of the way until then)
		FS47Rect Old; Old.Id = TEXT("oldTruck"); Old.MinX = Old.MaxX = Old.MinZ = Old.MaxZ = 1e9;
		OldCol = W->CollidersMutable(TEXT("roswell")).Add(Old);
	}
	auto Head = [&](IS47DriveArea& A, const TCHAR* Id, const TCHAR* FallbackSet, const TArray<int32>& FallbackSlots)
	{
		const TSharedPtr<FJsonObject> H = JObj(JObj(DLegs, Id), TEXT("head"));
		IS47DriveArea::FHeadlights L;
		if (H.IsValid())
		{
			L.Set = int32(S47J::Num(H, TEXT("set"), -1));
			for (double V : S47J::Nums(H, TEXT("slots"))) L.Slots.Add(int32(V));
		}
		else if (FallbackSet) { L.Set = W->FloodSet(FallbackSet); L.Slots = FallbackSlots; }
		if (L.Set >= 0 && L.Slots.Num()) A.Headlights = L;
	};
	auto Fill = [&](IS47DriveArea& A, const TCHAR* Id, TFunction<void()> Fallback)
	{
		const TSharedPtr<FJsonObject> L = JObj(DLegs, Id);
		if (L.IsValid()) A.Obstacles = ObstaclesOf(L);
		else if (Fallback) Fallback();
	};
	auto Thud = [this](double Speed)
	{
		// d.thud: audio.play('thudSoft', { gain, rate: 0.8 })
		if (!M || !M->Audio) return;
		FS47Sound S; S.Gain = float(FMath::Min(1.0, Speed / 8)); S.Rate = 0.8f;
		M->Audio->Sfx(TEXT("thud_soft"), S);
	};
	// SARO's own ground (ensureSaroLeg): its colliders without the truck's own, and the posts of
	// the road's stretch (the export's list has both)
	{
		TUniquePtr<FArrivalSaroDrive> A = MakeUnique<FArrivalSaroDrive>(Boxes(W->CollidersMutable(TEXT("saro")), SaroCol), TArray<FS47Obstacle>());
		TArray<FString> ArrivalChecks;
		A->bVisitorLot = W->AuditSaroArrival(ArrivalChecks);
		UE_LOG(LogS47World,Display,TEXT("S47: SARO visitor driving corridor ready=%d; original service corridors retained"),A->bVisitorLot);
		Fill(*A, TEXT("saro"), nullptr);
		A->Obstacles.Add(ClosedRanchGate(FVector(-S47Geo::SARO_IN_ROAD.X, 0, -S47Geo::SARO_IN_ROAD.Z)));
		Head(*A, TEXT("saro"), nullptr, {});
		FLegRec& L = Legs.Add(TEXT("saro"));
		L.Truck = SaroTruckP.Get();
		L.Drive = MakeUnique<FS47DriveController>(L.Truck, A.Get());
		L.Drive->OnBump = Thud;
		L.Area = MoveTemp(A);
	}
	// the road (ensureRoad): the highway and the survey track, the diner's lot and the town on it
	{
		TUniquePtr<FS47RoadArea> A = MakeUnique<FS47RoadArea>(ROAD_ORIGIN);
		A->ClosedGroup = W->Obj(TEXT("world.legs.road.area.closed.group"));
		Fill(*A, TEXT("road"), [&]
		{
			// (an older export: the boxes it named, the diner's walls and the town's)
			for (const FString& P : W->PathsUnder(TEXT("world.legs.road.area.obstacles[")))
			{
				const FS47Area R = W->Rect(P);
				if (R.bValid) A->Obstacles.Add(FS47Obstacle::Box(R.MinX, R.MaxX, R.MinZ, R.MaxZ));
			}
		});
		A->Obstacles.Add(ClosedRanchGate(ROAD_ORIGIN));
		Head(*A, TEXT("road"), TEXT("road"), { 0, 1, 2 });
		// the diner's lot is gravel, its top at the diner's height (ensureDiner: road.addPlace)
		const FVector O = DINER_ORIGIN();
		A->Lots.Add({ { O.X + 3.6, O.X + 16, O.Z - 16, O.Z + 16 }, [] { FS47Surface S; S.Kind = ES47SurfaceKind::Gravel; S.Grip = 0.75; S.Top = 20; S.Rough = 0.4; return S; }(), O.Y });
		DinerPark = { O.X + 5, O.X + 14, O.Z - 4, O.Z + 14 };
		A->OnCattleGuard = [this](double Speed) { if (Engine) Engine->Rattle(Speed); };
		Road = A.Get();
		FLegRec& L = Legs.Add(TEXT("road"));
		L.Truck = TruckP.Get();
		L.Drive = MakeUnique<FS47DriveController>(L.Truck, A.Get());
		L.Drive->OnBump = Thud;
		L.Area = MoveTemp(A);
	}
	// STATION 01 (ensureStation): the last of the track and the pad
	{
		TUniquePtr<FS47StationDrive> A = MakeUnique<FS47StationDrive>(Boxes(W->CollidersMutable(TEXT("station01")), StationCol), TArray<FS47Obstacle>());
		Fill(*A, TEXT("station01"), nullptr);
		Head(*A, TEXT("station01"), nullptr, {});
		FLegRec& L = Legs.Add(TEXT("station01"));
		L.Truck = ParkedP.Get();
		L.Drive = MakeUnique<FS47DriveController>(L.Truck, A.Get());
		L.Drive->OnBump = Thud;
		L.Area = MoveTemp(A);
	}
	// the old road at dawn (ensureOldRoad), with the diner's walls and lot
	{
		TUniquePtr<FS47OldRoad> A = MakeUnique<FS47OldRoad>(ROAD_ORIGIN);
		Fill(*A, TEXT("roswell"), [&]
		{
			for (const FString& P : W->PathsUnder(TEXT("world.oldRoad.obstacles[")))
			{
				const FS47Area R = W->Rect(P);
				if (R.bValid) A->Obstacles.Add(FS47Obstacle::Box(R.MinX, R.MaxX, R.MinZ, R.MaxZ));
			}
		});
		Head(*A, TEXT("roswell"), TEXT("oldroad"), FS47OldRoad::OLD_HEADLIGHTS());
		const FVector O = DINER_ORIGIN();
		A->AddPlace({}, { O.X + 3.6, O.X + 16, O.Z - 16, O.Z + 16 }, O.Y);
		FLegRec& L = Legs.Add(TEXT("roswell"));
		L.Truck = OldTruckP.Get();
		L.Drive = MakeUnique<FS47DriveController>(L.Truck, A.Get());
		L.Drive->OnBump = Thud;
		L.Area = MoveTemp(A);
	}
	UE_LOG(LogS47World, Display, TEXT("S47: drive legs: saro %d, road %d, station01 %d, roswell %d obstacles%s"),
		Legs[TEXT("saro")].Area->Obstacles.Num(), Legs[TEXT("road")].Area->Obstacles.Num(), Legs[TEXT("station01")].Area->Obstacles.Num(), Legs[TEXT("roswell")].Area->Obstacles.Num(),
		D.IsValid() ? TEXT("") : TEXT(" (no drive data in the export)"));
	for (const TCHAR* P : TRUCKS) ParkAt(P);
}

FS47Truck* US47World::TruckOf(const FString& Id) const
{
	if (Id == TEXT("saro") || Id == TEXT("motel")) return SaroTruckP.Get();
	if (Id == TEXT("station01")) return ParkedP.Get();
	if (Id == TEXT("diner")) return TruckP.Get();
	return nullptr;
}

FS47OldRoad* US47World::OldRoad() const { const FLegRec* L = Legs.Find(TEXT("roswell")); return L ? static_cast<FS47OldRoad*>(L->Area.Get()) : nullptr; }
FS47DriveController* US47World::OldDrive() const { const FLegRec* L = Legs.Find(TEXT("roswell")); return L ? L->Drive.Get() : nullptr; }

FS47Truck* US47World::CabTruck() const
{
	const FLegRec* L = bDriving && !Leg.IsEmpty() ? Legs.Find(Leg) : nullptr;
	return L ? L->Truck : nullptr;
}

FS47DriveController* US47World::CabDrive() const
{
	const FLegRec* L = bDriving && !Leg.IsEmpty() ? Legs.Find(Leg) : nullptr;
	return L ? L->Drive.Get() : nullptr;
}

// ---------- areas ----------

void US47World::Enter(const FString& NewArea)
{
	// World.enter: the area shown, the others hidden, the player's zones (AS47Web keeps them by
	// area), the fog; the far plane needs nothing here (Unreal has no far plane to move)
	Area = NewArea;
	if (M && M->Web)
	{
		M->Web->Enter(NewArea);
		M->Web->SetFogDensity(NewArea == TEXT("roswell") ? FOG_OLDROAD : FOG);
	}
	// a truck shows where it stands, or where it is being driven (AS47Web shows every truck with
	// its area; World.enter's rules after it)
	EnsureLegs();
	if (bLegsMade)
	{
		for (FS47Truck* T : {SaroTruckP.Get(), TruckP.Get()}) if (T) T->SetPersonalVehicle(bCommuteVehicle);
		auto Here = [this](const TCHAR* Id) { return Area == Id && (TruckAt == Id || Leg == Id); };
		// (on the motel's lot it is SARO's truck too, in SARO's area)
		SaroTruckP->Group.SetVisible(Here(TEXT("saro")) || (Area == TEXT("saro") && TruckAt == TEXT("motel")));
		const bool bOnRoad = Area == TEXT("road") || Area == TEXT("diner");
		TruckP->Group.SetVisible(bOnRoad && (Leg == TEXT("road") || TruckAt == TEXT("diner")));
		ParkedP->Group.SetVisible(Here(TEXT("station01")));
		OldTruckP->Group.SetVisible(Area == TEXT("roswell"));
		if (bCommuteVehicle) {SaroTruckP->Group.SetVisible(false);TruckP->Group.SetVisible(false);}
	}
	if (OnEnter) OnEnter(NewArea);
	// on foot on the old road (out of the truck anywhere, Tom 22 October 2026): its ground's height,
	// as the truck has it (the web version is never on foot there)
	if (NewArea == TEXT("roswell") && M && M->Player)
	{
		TWeakObjectPtr<US47World> Self = this;
		M->Player->Floor = [Self](double X, double Z) { const FS47OldRoad* R = Self.IsValid() ? Self->OldRoad() : nullptr; return R ? R->Height(X, Z) : 0.0; };
	}
}

double US47World::SkyYaw() const { return Area == TEXT("station01") ? S47Geo::STATION_TURN() : 0; }

void US47World::SetStoryPlaces(bool bStationOpen, bool bOldRoadOpen)
{
	// (the walk gap at STATION 01 is the station's: Station01.setWalkGate, with the station's port)
	(void)bStationOpen;
	EnsureLegs();
	if (Road) Road->SetRoadClosed(!bOldRoadOpen);
}

void US47World::Place(double X, double Z, double Yaw)
{
	if (M && M->Player) M->Player->Place(X, Z, Yaw);
}

void US47World::PlaceAnchor(const FString& Path)
{
	const FS47Pose P = M && M->Web ? M->Web->Pose(Path) : FS47Pose();
	if (P.bValid) Place(P.X, P.Z, P.Yaw);
	else UE_LOG(LogS47World, Warning, TEXT("S47: no anchor %s"), *Path);
}

void US47World::PlaceAtStation() { PlaceAnchor(TEXT("world.site.anchors.arrive")); }

void US47World::PlaceAtDiner(const FString& At) { PlaceAnchor(TEXT("world.diner.anchors.") + At); }

void US47World::GoDiner(const FString& At)
{
	if (bBusy || !M || !M->Story) return;
	bBusy = true;
	Hold(true);
	Fade(true);
	M->Story->After(0.5, [this, At]
	{
		Enter(TEXT("diner"));
		PlaceAtDiner(At);
		bBusy = false;
		Hold(false);
		Fade(false);
	});
}

void US47World::GoRoom6()
{
	// World.goRoom6: through the door of room 6 (a short fade)
	if (bBusy || !M || !M->Story) return;
	bBusy = true;
	Hold(true);
	Fade(true);
	M->Story->After(0.5, [this]
	{
		Enter(TEXT("room6"));
		PlaceAnchor(TEXT("world.room6.anchors.arrive"));
		bBusy = false;
		if (OnEnterRoom6) OnEnterRoom6();
		Hold(false);
		Fade(false);
	});
}

void US47World::LeaveRoom6()
{
	// World.leaveRoom6: out onto the motel's front, back in SARO's scene
	if (bBusy || !M || !M->Story) return;
	Hold(true);
	Fade(true);
	M->Story->After(0.6, [this]
	{
		Enter(TEXT("saro"));
		const FString Path = M->Web->Pose(TEXT("world.court.anchors.fromRoom6")).bValid ? TEXT("world.court.anchors.fromRoom6") : TEXT("world.motel.anchors.fromRoom6");
		PlaceAnchor(Path);
		if (OnLeaveRoom6) OnLeaveRoom6();
		Hold(false);
		Fade(false);
	});
}

// ---------- the drives ----------

void US47World::DriveOut(bool bOnTheRoad)
{
	EnsureLegs();
	FS47TripOpts O;
	if (bOnTheRoad && Road) { O.AtPos = Road->Start.Pos; O.AtHeading = Road->Start.Heading; }
	O.Hint = TEXT("Leave the truck on the gravel outside the gate: stop there.");
	O.OnArrive = [this] { if (OnArriveStation) OnArriveStation(); };
	StartTrip(bOnTheRoad ? FString(TEXT("road")) : TruckAt.IsEmpty() ? FString(TEXT("saro")) : TruckAt, TEXT("station01"), O);
}

void US47World::Roam(const FString& From)
{
	FS47TripOpts O;
	O.Legs = { TEXT("saro"), TEXT("road"), TEXT("station01") };
	O.Places = { TEXT("diner") };
	O.OnStart = [this]
	{
		// the controls the first time on this device, where the truck can be left the first drive of one's own
		TArray<FString> Say;
		if (FirstTime(TEXT("drive"))) Say.Add(FString::Printf(TEXT("%s, %s, %s and %s: drive. %s: radio. Stop and press %s to get out."), *S47Keys::Said(TEXT("forward")), *S47Keys::Said(TEXT("back")), *S47Keys::Said(TEXT("left")), *S47Keys::Said(TEXT("right")), *S47Keys::Said(TEXT("radio")), *S47Keys::Said(TEXT("use"))));
		if (FirstTime(TEXT("roam"))) Say.Add(TEXT("On the highway, stop at the motel or the diner."));
		if (Say.Num()) Toast(FString::Join(Say, TEXT(" ")), 5.5f);
	};
	StartTrip(From, FString(), O);
}

void US47World::DriveToDiner(TFunction<void()> OnArrive)
{
	FS47TripOpts O;
	O.PerMetre = 0.19;
	O.OnArrive = MoveTemp(OnArrive);
	O.Hint = TEXT("Mesa Diner is the lit sign on the right. Leave the truck on its lot: stop there.");
	StartTrip(TruckAt.IsEmpty() ? FString(TEXT("saro")) : TruckAt, TEXT("diner"), O);
}

void US47World::DriveToMotel(TFunction<void()> OnArrive)
{
	FS47TripOpts O;
	O.OnArrive = MoveTemp(OnArrive);
	O.Hint = TEXT("Sierra Motor Court is the lit sign on the right. Leave the truck on its lot by the office: stop there.");
	StartTrip(TruckAt.IsEmpty() ? FString(TEXT("saro")) : TruckAt, TEXT("motel"), O);
}

void US47World::DriveBack()
{
	FS47TripOpts O;
	O.Hint = bCommuteVehicle ? TEXT("Park anywhere in the visitor parking area left of the SARO sign. Come to a full stop, then use the entrance by the ramp.") : TEXT("The truck goes back on its pad by the service yard: up the ramp, and stop there.");
	O.OnArrive = [this]
	{
		if (OnArriveSaro) OnArriveSaro();
		// (left on SARO's ground somewhere else, out among the masts say, it is back at SARO but not on its pad)
		// Evening's OnParked starts 18:00 after this callback. Its own arrival card
		// replaces the service return message, which would otherwise quote 17:20.
		if (!bCommuteVehicle && InBox(ParkBox(TEXT("saro")), SaroTruckP->Position) && M && M->Story) Toast(FString::Printf(TEXT("%s. The truck is back on its pad."), *M->Story->ClockText(false)), 3.f);
	};
	StartTrip(TruckAt.IsEmpty() ? FString(TEXT("station01")) : TruckAt, TEXT("saro"), O);
}

void US47World::DriveOldRoad(TFunction<void()> Ready)
{
	EnsureLegs();
	// (a chapter started from the menu has the truck somewhere else, and the player may have left
	// it on the highway or in Caliche's street: the old road goes out from the lot)
	if (TruckAt != TEXT("diner") || !InBox(DinerPark, TruckP->Position)) PlaceTruck(TEXT("diner"));
	FS47TripOpts O;
	O.Leg = TEXT("roswell");
	O.OnStart = [this, Ready]
	{
		if (FS47OldRoad* R = OldRoad()) R->bHitBack = false;
		OldControl.Reset();
		if (Ready) Ready();
		if (FirstTime(TEXT("drive"))) Toast(FString::Printf(TEXT("%s and %s: throttle and brake (%s at a stop: reverse). %s and %s: steer."), *S47Keys::Said(TEXT("forward")), *S47Keys::Said(TEXT("back")), *S47Keys::Said(TEXT("back")), *S47Keys::Said(TEXT("left")), *S47Keys::Said(TEXT("right"))), 4.5f);
	};
	StartTrip(TEXT("diner"), TEXT("roswell"), O);
}

// ---------- trips ----------

void US47World::ParkAt(FString Id)
{
	// The parked truck's collider and its handle for "use" go where the truck stands, or out of
	// the way while it is gone. (At the motel it is SARO's truck, collider and handle; on the old road
	// its truck's collider only: the truck itself is what is used there.)
	if (Id == TEXT("motel")) Id = TEXT("saro");
	FS47Truck* T = TruckAtPlace(Id);
	if (!T || !M || !M->Web) return;
	const bool bHere = TruckAt == Id || (Id == TEXT("saro") && TruckAt == TEXT("motel"));
	const double H = T->Rotation.Y, C = FMath::Abs(FMath::Cos(H)), Sn = FMath::Abs(FMath::Sin(H));
	const double Hx = 1.05 * C + 2.75 * Sn, Hz = 1.05 * Sn + 2.75 * C;
	const FVector& G = T->Position;
	auto Put = [&](const TCHAR* WalkArea, int32 Index)
	{
		TArray<FS47Rect>& Cols = M->Web->CollidersMutable(WalkArea);
		if (!Cols.IsValidIndex(Index)) return;
		FS47Rect& R = Cols[Index];
		if (bHere) { R.MinX = G.X - Hx; R.MaxX = G.X + Hx; R.MinZ = G.Z - Hz; R.MaxZ = G.Z + Hz; }
		else { R.MinX = R.MaxX = R.MinZ = R.MaxZ = 1e9; }
		R.Id = TEXT("truck"); // (the walker keeps to the truck itself, AS47Walker::TruckBox)
	};
	// the walker: the truck as it is turned, in its area (its box above is far bigger at an angle)
	if (M->Player)
	{
		const FString A = AreaOf(Id);
		if (bHere) { AS47Walker::FTruckBox B; B.Area = A; B.X = G.X; B.Z = G.Z; B.Heading = H; M->Player->TruckBox = B; }
		else if (M->Player->TruckBox && M->Player->TruckBox->Area == A) M->Player->TruckBox.Reset();
	}
	if (Id == TEXT("saro")) Put(TEXT("saro"), SaroCol);
	else if (Id == TEXT("station01")) Put(TEXT("station01"), StationCol);
	// (the diner's walls are walked in the diner's area and in the road's, World.roadWalk: the same list there)
	else if (Id == TEXT("diner")) { Put(TEXT("diner"), DinerCol); Put(TEXT("road"), DinerCol); }
	else if (Id == TEXT("roswell")) Put(TEXT("roswell"), OldCol);
	FS47Obj Proxy = Id == TEXT("saro") ? SaroProxy : Id == TEXT("station01") ? StationProxy : Id == TEXT("diner") ? DinerProxy : FS47Obj();
	if (Proxy.IsValid())
	{
		const FS47Obj Up = Proxy.Parent();
		const FVector At = Up.IsValid() ? Up.WorldToLocal(G) : G;
		Proxy.SetPosition(FVector(At.X, bHere ? At.Y + 0.95 : -500, At.Z));   // the diner's lot is 4 m up
		Proxy.SetRotation(FVector(0, H, 0));
	}
}

void US47World::StartTrip(const FString& From, const FString& To, const FS47TripOpts& O)
{
	if (bBusy || bDriving || !M || !M->Story) return;
	EnsureLegs();
	bBusy = true;
	Hold(true);
	Fade(true);
	UE_LOG(LogS47World, Display, TEXT("S47: trip %s -> %s"), *From, To.IsEmpty() ? TEXT("anywhere") : *To);
	// (the web version loads the ways and waits 0.4 s behind the black; everything is built here)
	M->Story->After(0.4, [this, From, To, O]
	{
		// a place can stand in another's area (the diner in the road's): the trip starts in that leg
		const FString Leg0 = O.Leg.IsEmpty() ? S47Legs::LegOf(From) : O.Leg;
		FLegRec* L = Legs.Find(Leg0);
		if (!L)
		{
			UE_LOG(LogS47World, Warning, TEXT("S47: no way from %s"), *From);
			Toast(TEXT("The way could not be loaded. Check your connection and try the truck again."), 5.f);
			bBusy = false; Hold(false); Fade(false);
			return;
		}
		FS47Truck* G = TruckOf(From) ? TruckOf(From) : L->Truck;
		// where every parked truck stands now, so a trip that is given up puts them back
		TrucksHome.Reset();
		for (const TCHAR* Id : TRUCKS) if (FS47Truck* T = TruckOf(Id)) TrucksHome.Add({ T, T->Position.X, T->Position.Y, T->Position.Z, T->Rotation.Y, T->Rotation.X, T->Rotation.Z });
		FTrip Tr;
		Tr.From = From; Tr.To = To; Tr.PerMetre = O.PerMetre; Tr.OnArrive = O.OnArrive; Tr.Home = TruckAt; Tr.Hint = O.Hint;
		Tr.bFree = O.bFree; Tr.StartPos = O.AtPos ? *O.AtPos : G->Position; Tr.N = ++TripNo;
		Trip = Tr;
		Leg = Leg0;
		if (!O.bFree) { TruckAt.Reset(); for (const TCHAR* Id : TRUCKS) ParkAt(Id); SetGround(FString()); }
		if (!O.bFree && From == TEXT("saro")) OdoAtSaro = Odometer();
		L->Drive->Place(O.AtPos ? *O.AtPos : G->Position, O.AtPos ? O.AtHeading : G->Rotation.Y);
		LightsOn(*L->Truck);
		Enter(AreaOf(Leg0));
		StartEngine();
		bDriving = true;
		// after a time away (story/abduction.ts) the engine has died where it stood: the key
		if (bKeyStall) { bKeyStall = false; FS47TroubleOpts K; K.bKey = true; Trouble.Start(ES47Trouble::Stall, K); }
		if (OnGetIn) OnGetIn();
		Auto = FAuto();
		if (O.OnStart) O.OnStart();
		else if (From == To && !O.Hint.IsEmpty()) Toast(O.Hint, 5.f);
		// how to drive, the first time on this device (the notebook's Controls tab has it after)
		else if (FirstTime(TEXT("drive"))) Toast(FString::Printf(TEXT("%s and %s: throttle and brake (%s at a stop: reverse). %s and %s: steer. %s: radio. Stopped, %s gets out."), *S47Keys::Said(TEXT("forward")), *S47Keys::Said(TEXT("back")), *S47Keys::Said(TEXT("back")), *S47Keys::Said(TEXT("left")), *S47Keys::Said(TEXT("right")), *S47Keys::Said(TEXT("radio")), *S47Keys::Said(TEXT("use"))), 5.f);
		bBusy = false; Hold(false); Fade(false);
		UE_LOG(LogS47World, Display, TEXT("S47: driving, leg %s, truck at %.1f %.1f"), *Leg, L->Drive->Pos.X, L->Drive->Pos.Z);
	});
}

void US47World::LightsOn(FS47Truck& T)
{
	T.SetDriving(true); T.SetHeadlights(true); T.SetLightLevel(1); T.SetHeadColor({}); T.SetDashLevel({});
}

// Every frame of a trip, after the truck has moved: on into the next area, or arrived.
void US47World::TripStep(double Dt)
{
	const FString Id = Leg;
	FLegRec* L = Legs.Find(Id);
	if (!L) return;
	FS47DriveController& D = *L->Drive;
	// (chapter six's old road is not a trip: there are no ways out of it)
	const TOptional<S47Legs::FExit> Ex = S47Legs::ExitFrom(Id, D.Pos.X, D.Pos.Z, D.Heading);
	if (Ex && Legs.Contains(Ex->To)) { Transfer(*Ex); return; }
	if (!Trip) return;
	FTrip& T = *Trip;
	// (a line in the log every five seconds of a trip, for the test runs)
	if (FMath::FloorToInt(T.Secs / 5) != FMath::FloorToInt((T.Secs + Dt) / 5))
	{
		UE_LOG(LogS47World, Display, TEXT("S47: trip %.0f s, leg %s, at %.1f %.1f, %.1f m/s"), T.Secs + Dt, *Id, D.Pos.X, D.Pos.Z, D.Speed);
	}
	T.Secs += Dt;
	T.Metres += FMath::Abs(D.Speed) * Dt;
	if (T.PerMetre && M && M->Story) M->Story->SetClock(M->Story->bEveningTimeline ? FMath::Min(S47Timeline::Departure, M->Story->Clock() + FMath::Abs(D.Speed) * Dt * T.PerMetre) : M->Story->Clock() + FMath::Abs(D.Speed) * Dt * T.PerMetre);
	const bool bHere = !T.To.IsEmpty() && S47Legs::LegOf(T.To) == Id;
	const TOptional<FS47Box2> P = !T.To.IsEmpty() ? ParkBox(T.To) : TOptional<FS47Box2>();
	// where to leave the truck, said once it is in sight
	if (!T.Hint.IsEmpty() && bHere && P && FMath::Sqrt(FMath::Square(D.Pos.X - (P->MinX + P->MaxX) / 2) + FMath::Square(D.Pos.Z - (P->MinZ + P->MaxZ) / 2)) < 110)
	{
		Toast(T.Hint, 5.f); T.Hint.Reset();
	}
	// the truck is left where it stops: on the trip's place, or (once it has been driven off the
	// one it stood on) on any place in this area; a free drive on none
	if (!T.bLeft && (Id != S47Legs::LegOf(T.From) || FMath::Sqrt(FMath::Square(D.Pos.X - T.StartPos.X) + FMath::Square(D.Pos.Z - T.StartPos.Z)) > 12)) T.bLeft = true;
	FString Stop;
	if (!T.bFree && FMath::Abs(D.Speed) < 0.35)
	{
		for (const TCHAR* Pl : PLACES)
		{
			if ((Pl == T.To || T.bLeft) && S47Legs::LegOf(Pl) == Id && (bCommuteVehicle && FString(Pl)==TEXT("saro") ? FS47EveningVisual::FitsArrivalArea(D.Pos,D.Heading) : InBox(ParkBox(Pl), D.Pos))) { Stop = Pl; break; }
		}
	}
	if (!Stop.IsEmpty())
	{
		T.Still += Dt;
		if (T.Still > 0.7) Park(Stop);
	}
	else T.Still = 0;
}

TOptional<FS47Box2> US47World::ParkBox(const FString& Place) const
{
	if (Place == TEXT("saro") && bCommuteVehicle) return FS47Box2{FS47EveningVisual::ArrivalMinX,FS47EveningVisual::ArrivalMaxX,FS47EveningVisual::ArrivalMinZ,FS47EveningVisual::ArrivalMaxZ};
	if (Place == TEXT("diner")) return bLegsMade ? TOptional<FS47Box2>(DinerPark) : TOptional<FS47Box2>();
	if (Place == TEXT("motel")) { const FLegRec* L = Legs.Find(TEXT("saro")); return L ? TOptional<FS47Box2>(static_cast<const FS47SaroDrive*>(L->Area.Get())->MotelPark) : TOptional<FS47Box2>(); }
	const FLegRec* L = Legs.Find(Place);
	return L ? L->Area->Park : TOptional<FS47Box2>();
}

// Into the next area: its truck goes on from here, as fast and as turned as this one.
void US47World::Transfer(const S47Legs::FExit& Ex)
{
	FLegRec& From = Legs[Leg];
	FLegRec& To = Legs[Ex.To];
	Leg = Ex.To;
	To.Drive->Adopt(*From.Drive, Ex.X, Ex.Z, Ex.Heading);
	To.Truck->CopyCab(*From.Truck);
	Trouble.HandOver(*From.Truck, *From.Drive);   // the copy left behind is itself again
	DashT = 0;   // the clock on its dash is set this frame
	LightsOn(*To.Truck);
	From.Truck->SetDriving(false);
	Enter(AreaOf(Ex.To));
	UE_LOG(LogS47World, Display, TEXT("S47: leg %s"), *Leg);
}

// Stopped where the truck is left: the engine off, and the player gets out by the door.
void US47World::Park(const FString& Place)
{
	FLegRec& L = Legs[Leg];
	// the trip's place reached; on the highway that is the diner's lot or Caliche's street (the old
	// road is no place to arrive at: the truck is only left there)
	const bool bArrived = Trip && Trip->To == Place && Place != TEXT("roswell") && (Place != TEXT("diner") || AtDiner(L.Drive->Pos));
	TFunction<void()> OnArrive = Trip ? Trip->OnArrive : nullptr;
	Trouble.Reset(L.Truck, L.Drive.Get());
	bDriving = false;
	if (Engine) Engine->Stop();
	L.Truck->SetDriving(false);
	L.Truck->SetHeadlights(false);
	TruckAt = Place; Leg.Reset(); Trip.Reset(); Autopilot.Reset();
	ParkAt(Place);
	Hold(true);
	Fade(true);
	// out by the door at once: from this frame the camera is the player's again; where no walk zone
	// of the area is by a door, onto the ground round the truck
	Enter(AreaOf(Place));
	SetGround(Place);
	GetOut(*L.Truck);
	UE_LOG(LogS47World, Display, TEXT("S47: parked at %s%s"), *Place, bArrived ? TEXT(", arrived") : TEXT(""));
	if (M && M->Story) M->Story->After(0.4, [this, bArrived, OnArrive, Place]
	{
		Hold(false);
		Fade(false);
		if (bArrived && OnArrive) OnArrive();
		if (OnParked) OnParked(Place);
	});
}

void US47World::GetOut(FS47Truck& T)
{
	const double H = T.Rotation.Y;
	TOptional<FVector> Spot = DoorSpot(T);
	// (out anywhere, Tom 22 October 2026: where neither door has room, beside the truck a little
	// further off, or at an end of it)
	if (!Spot) Spot = RoomBy(T);
	const FVector V = Spot ? *Spot : AboutUp(FVector(-1.75, 0, -0.3), H) + T.Position;
	// (on the ground under them: where the area's floor is read off the world, it is read from the
	// truck's ground, not from wherever the walker last stood)
	if (M && M->Player) M->Player->FloorY = T.Position.Y;
	Place(V.X, V.Z, H);
}

/** Ground to stand on by one of the truck's doors, in the given walk (the area's walkable zones
 *  and what stands on them; the player's own by default, with the ground round the truck), clear of
 *  everything in the way, or unset. */
TOptional<FVector> US47World::DoorSpot(const FS47Truck& T, const FString& WalkArea) const
{
	if (!M || !M->Web || !M->Player) return {};
	const double H = T.Rotation.Y, R = M->Player->Radius, E = 1e-6;
	const TArray<FS47Rect>& Zones = WalkArea.IsEmpty() ? M->Web->Zones() : M->Web->ZonesMutable(WalkArea);
	const TArray<FS47Rect>& Cols = WalkArea.IsEmpty() ? M->Web->Colliders() : M->Web->CollidersMutable(WalkArea);
	for (const double Side : { -1.0, 1.0, -1.4, 1.4 })
	{
		const FVector V = AboutUp(FVector(Side * 1.75, 0, -0.3), H) + T.Position;
		bool bWalk = WalkArea.IsEmpty() && M->Player->Walkable(V.X, V.Z);
		if (!WalkArea.IsEmpty()) for (const FS47Rect& B : Zones) if (B.bEnabled && V.X >= B.MinX + R - E && V.X <= B.MaxX - R + E && V.Z >= B.MinZ + R - E && V.Z <= B.MaxZ - R + E) { bWalk = true; break; }
		if (!bWalk) continue;
		// (the box the walk keeps round the truck is passed over: these spots are outside the truck
		// itself, and the box is far bigger than the truck when it stands at an angle)
		bool bHit = false;
		for (const FS47Rect& C : Cols) if (C.Id != TEXT("truck") && V.X + R > C.MinX && V.X - R < C.MaxX && V.Z + R > C.MinZ && V.Z - R < C.MaxZ) { bHit = true; break; }
		if (!bHit) return V;
	}
	return {};
}

/** Room beside the truck where neither door has any (a wall, a fence, a post by it, or the truck
 *  turned across the box the walk keeps round it): further out from each side, then behind it and
 *  in front, in the player's own walk. */
TOptional<FVector> US47World::RoomBy(const FS47Truck& T) const
{
	if (!M || !M->Web || !M->Player) return {};
	const double H = T.Rotation.Y, R = M->Player->Radius;
	TArray<FVector, TInlineAllocator<16>> Tries;
	for (const double D : { 2.4, 3.2, 4.0, 4.8 }) { Tries.Add(FVector(-D, 0, -0.3)); Tries.Add(FVector(D, 0, -0.3)); }
	for (const double D : { 3.6, 4.4 }) { Tries.Add(FVector(0, 0, D)); Tries.Add(FVector(0, 0, -D)); }
	for (const FVector& L : Tries)
	{
		const FVector V = AboutUp(L, H) + T.Position;
		if (!M->Player->Walkable(V.X, V.Z)) continue;
		bool bHit = false;
		for (const FS47Rect& C : M->Web->Colliders()) if (C.Id != TEXT("truck") && V.X + R > C.MinX && V.X - R < C.MaxX && V.Z + R > C.MinZ && V.Z - R < C.MaxZ) { bHit = true; break; }
		if (!bHit) return V;
	}
	return {};
}

// ---------- the ground round the truck where it is left (Tom 22 October 2026) ----------

FS47Truck* US47World::TruckAtPlace(const FString& Place) const
{
	return Place == TEXT("roswell") ? OldTruckP.Get() : TruckOf(Place);
}

/** The walk a place's area has (AS47Web: the diner's area walks the road's). */
FString US47World::WalkOf(const FString& Place) const
{
	if (Place == TEXT("motel")) return TEXT("saro");
	if (Place == TEXT("diner")) return M && M->Web && M->Web->ZonesMutable(TEXT("road")).Num() ? FString(TEXT("road")) : Place;
	return Place;
}

void US47World::SetGround(const FString& Place)
{
	// The web version lets the player out only where the area has walk zones by a door (the lots, the
	// yard and the field, Caliche, the road's stops by the herd and the deputy's car), as nothing else
	// is walked. Here the truck can be left anywhere: where a door has a walk zone by it, that is
	// walked as it is; elsewhere the player walks where the truck itself could stand (its leg's
	// ground, clear of the leg's posts, rocks, walls and fences), within GROUND_R of it, so they can
	// walk away from it and back, and get in again.
	if (!M || !M->Player) return;
	AS47Walker* P = M->Player;
	P->Ground = nullptr;
	P->GroundArea.Reset();
	GroundLeg.Reset();
	if (Place.IsEmpty() || bDriving || !bLegsMade) return;
	const FString LegId = Place == TEXT("roswell") ? Place : S47Legs::LegOf(Place);
	const FS47Truck* T = TruckAtPlace(Place);
	if (!T || !Legs.Contains(LegId)) return;
	// (a walk zone or the open land by a door: the area's own walk will do)
	const FString A = AreaOf(Place);
	if (A == Area ? DoorSpot(*T).IsSet() : DoorSpot(*T, WalkOf(Place)).IsSet()) return;
	if (A != Area) for (const double Side : { -1.0, 1.0 })
	{
		const FVector V = AboutUp(FVector(Side * 1.75, 0, -0.3), T->Rotation.Y) + T->Position;
		if (OnLand(V.X, V.Z, A)) return;
	}
	GroundLeg = LegId;
	GroundAt = T->Position;
	P->GroundArea = AreaOf(Place);
	TWeakObjectPtr<US47World> Self = this;
	P->Ground = [Self](double X, double Z) { return Self.IsValid() && Self->OnGround(X, Z); };
	UE_LOG(LogS47World, Display, TEXT("S47: ground round the truck at %s (leg %s, %.1f %.1f)"), *Place, *LegId, GroundAt.X, GroundAt.Z);
}

bool US47World::OnGround(double X, double Z)
{
	FLegRec* L = GroundLeg.IsEmpty() ? nullptr : Legs.Find(GroundLeg);
	if (!L || !M || !M->Player) return false;
	if (FMath::Square(X - GroundAt.X) + FMath::Square(Z - GroundAt.Z) > GROUND_R * GROUND_R) return false;
	IS47DriveArea& A = *L->Area;
	const double R = M->Player->Radius;
	// where the truck could stand, the player's whole width (the old road's test also notes that the
	// highway was run into, for chapter six's word about it: not for a walk)
	FS47OldRoad* Old = GroundLeg == TEXT("roswell") ? OldRoad() : nullptr;
	const bool bHitBack = Old && Old->bHitBack;
	bool bOff = false;
	for (const FVector2D& D : { FVector2D(0, 0), FVector2D(-R, -R), FVector2D(R, -R), FVector2D(-R, R), FVector2D(R, R) })
	{
		if (A.Blocked(X + D.X, Z + D.Y)) { bOff = true; break; }
	}
	if (Old) Old->bHitBack = bHitBack;
	if (bOff) return false;
	// and clear of what stops the truck there
	for (const FS47Obstacle& O : A.Obstacles)
	{
		if (O.bCircle ? FMath::Square(X - O.X) + FMath::Square(Z - O.Z) < FMath::Square(O.R + R) : X > O.MinX - R && X < O.MaxX + R && Z > O.MinZ - R && Z < O.MaxZ + R) return false;
	}
	return true;
}

// ---------- the open land (Tom 22 October 2026: once out of the truck, walk round freely there) ----------

void US47World::MakeLands()
{
	// Each outdoor area's open land, in world terms, from what the web version's builders draw there
	// (the sources are named at each). The web version walks only its zones; these are walked as well.
	Lands.Reset();
	// ---- SARO, in the site's terms (the control room at the origin, north is -z) ----
	{
		FLand L;
		L.Leg = TEXT("saro");
		L.In = [](double X, double Z)
		{
			// Exterior.ts ground: SARO's own land, a ring 1900 m out (flat to 300 m, rolling beyond, the
			// array graded flat: arrayLayout.ts ARRAY_FLAT), walked within 600 m. South of the site the
			// road's piece of ground lies over it (corridors.ts saroCorridorGround, site x -144..83.66,
			// z 100..900, with the ring sunk under it and a little round it): there only that piece, to
			// z 860 (the road's area takes the drive over at 784.8)
			if (X >= -146 && X <= 98 && Z >= 98 && Z <= 902) return X >= -143.5 && X <= 83 && Z >= 100.5 && Z <= 860;
			return X * X + Z * Z <= 600.0 * 600.0 || (X >= -296 && X <= 276 && Z >= -521 && Z <= 0);
		};
		L.Out = {
			// the control room and the annex on their plinths, the fire exit's stoop (ControlRoom.ts,
			// Annex.ts, Grounds.ts RAISED), the photo lab and the east wing (ServiceYard.ts, Exterior.ts):
			// their walls are no colliders, the zones keep the player to the doors
			{ -6.7, 6.7, -5.9, 5.9 }, { -6.6, 2.6, 5.8, 11.1 }, { -7.75, -6.0, 4.75, 6.65 }, { 12.6, 19.2, -6.0, 1.0 }, { 20.2, 39.0, -5.9, 5.9 },
			// the slabs at floor level, 0.6 m over the ground, their edges and rails held by the zones
			// (ServiceYard.ts): the landing, the east walk, the S-03 apron, the lab path, the pad and its ramp
			{ 6.3, 11.0, -0.6, 3.4 }, { 8.0, 11.0, -19.6, -0.6 }, { 11.0, 12.6, -13.4, -8.6 }, { 11.0, 12.6, -1.9, -0.6 }, { 6.6, 13.0, 3.4, 12.4 }, { 6.6, 13.0, 12.4, 16.6 },
			// the array's chain-link fence at z -20 (Exterior.ts), its gate shut unless the gate's own zone
			// is open (Field.ts); the highway's opening through it as it is drawn
			{ -80, -28.6, -20, -20 }, { -19.4, 60, -20, -20 },
			// the site's ranch fence (Grounds.ts)
			{ -18.6, 46.5, 34.5, 34.5 }, { 46.5, 46.5, -19.9, 34.5 },
			// the highway's right-of-way fences on the road's piece (roadProps.ts fences, road x -21 and 21,
			// drawn from site z 100), open at the motel and at the cattle guard; the ranch's gate is shut
			{ -45, -45, 100, 640 }, { -45, -45, 732, 817 }, { -45, -45, 823, 860 }, { -3, -3, 100, 860 },
			// and the pasture's north fence
			{ -143.5, -45.3, 740, 740 },
		};
		L.Y = [](double X, double Z)
		{
			// groundCover.ts saroFlora's ground, in the site's terms: the road's surface on its piece,
			// SARO's land elsewhere, the verge of the road's stretch south of z 36
			const double Rx = X + S47Geo::SARO_IN_ROAD.X, Rz = Z + S47Geo::SARO_IN_ROAD.Z;
			if (Rz >= -200 && S47Geo::InRect(S47Geo::SARO_PATCH, Rx, Rz)) return S47RoadTerrain::SurfaceY(Rx, Rz);
			double Y = S47GroundCover::SaroLand(X, Z);
			if (Z > 36 && FMath::Abs(Rx) < S47RoadTerrain::HWY.Flat) Y = FMath::Max(Y, S47RoadShape::HLow(Rx, Rz));
			return Y;
		};
		Lands.Add(TEXT("saro"), MoveTemp(L));
	}
	// ---- the road, the diner and Caliche (the diner's area walks it; road terms are world x - 8000) ----
	{
		FLand L;
		L.Leg = TEXT("road");
		L.In = [](double X, double Z)
		{
			// roadTerrain.ts groundGeometry: the desert grid, road x -3000..2300, z -2700..4400, less 50 m.
			// North of z 450 this area shows SARO's stretch with its motel and site as far views (their own
			// area has them), and the pasture's west end the far view of STATION 01 (roadPlaces.ts)
			const double Lx = X - ROAD_ORIGIN.X;
			if (Lx < -2950 || Lx > 2250 || Z < 450 || Z > 4350) return false;
			return !(Lx < -560 && Z < 730);
		};
		// the barricades across the old road's mouth, until chapter five sends the truck out on it
		// (roadProps.ts roadClosed: their posts at road x 25.5; the wall behind them at x 30 holds always)
		L.Closed = [this](double X, double Z) { return Road && Road->RoadClosed() && X - ROAD_ORIGIN.X > 24.5 && Z > 1981.95 && Z < 2017.95; };
		Lands.Add(TEXT("diner"), MoveTemp(L));
	}
	// ---- STATION 01 (station terms are world z - 8000) ----
	{
		FLand L;
		L.Leg = TEXT("station01");
		L.In = [](double X, double Z)
		{
			// Station01.ts terrain: its own land runs 2.8 km out, with the last of the track's ground on it
			// (corridors.ts stationCorridorGround); walked within 600 m of the station (the highway, 630 m
			// off, is not drawn here)
			const double Lz = Z - STATION_ORIGIN.Z;
			return X * X + Lz * Lz <= 600.0 * 600.0;
		};
		// the compound's fence (stationLayout.ts STATION_FENCE: posts and wire, no colliders): its zones
		// and the walk gap, chained until chapter three, are the only way in
		L.Out = { { S47Geo::STATION_FENCE.X0, S47Geo::STATION_FENCE.X1, S47Geo::STATION_FENCE.Z0 + STATION_ORIGIN.Z, S47Geo::STATION_FENCE.Z1 + STATION_ORIGIN.Z } };
		// (legs.ts StationDrive.height: the track's ground on its piece, the station's own land elsewhere)
		L.Y = [this](double X, double Z) { const FLegRec* G = Legs.Find(TEXT("station01")); return G ? G->Area->Height(X, Z) : 0.0; };
		Lands.Add(TEXT("station01"), MoveTemp(L));
	}
	// ---- the old road at dawn ----
	{
		FLand L;
		L.Leg = TEXT("roswell");
		L.In = [this](double X, double Z)
		{
			// OldRoad.ts blocked: between the old road's fences (17 m out; the truck keeps 14.5 m), and on
			// the highway by the diner. (Its Blocked notes the highway run into, for chapter six's word to
			// the driver: not for a walk.)
			FS47OldRoad* R = OldRoad();
			if (!R) return false;
			const bool bHit = R->bHitBack;
			const bool bOn = !R->Blocked(X, Z);
			R->bHitBack = bHit;
			return bOn;
		};
		// Caliche at dawn is seen from here, but its buildings hold nothing on this road (OldRoad.ts)
		L.Out = { { ROAD_ORIGIN.X - 42, ROAD_ORIGIN.X + 48, 2017.95, 2300 } };
		L.Y = [this](double X, double Z) { const FS47OldRoad* R = OldRoad(); return R ? R->Height(X, Z) : 0.0; };
		Lands.Add(TEXT("roswell"), MoveTemp(L));
	}
}

double US47World::LandY(double X, double Z)
{
	const double None = std::numeric_limits<double>::quiet_NaN();
	if (!bLegsMade || InZone(X, Z)) return None;
	if (Lands.IsEmpty()) MakeLands();
	const FLand* L = Lands.Find(Area);
	return L && L->Y && L->In && L->In(X, Z) ? L->Y(X, Z) : None;
}

bool US47World::InZone(double X, double Z) const
{
	if (!M || !M->Web) return false;
	for (const FS47Rect& B : M->Web->Zones()) if (B.bEnabled && X >= B.MinX && X <= B.MaxX && Z >= B.MinZ && Z <= B.MaxZ) return true;
	return false;
}

bool US47World::OnLand(double X, double Z, const FString& InArea)
{
	// On the area's ground, the player's whole width; not into a building, a raised slab, a fenced
	// compound or across a fence (with a margin wider than a step: their own zones and gates lead in);
	// within the story's limits now; clear of what stands there (the drive leg's posts, poles, rocks,
	// fences and walls; the walk's colliders and the truck hold as ever); and with the area's ground
	// meshes under it (never into the void at an area's edge).
	const FString& A = InArea.IsEmpty() ? Area : InArea;
	if (!M || !M->Player || !bLegsMade) return false;
	if (Lands.IsEmpty()) MakeLands();
	const FLand* L = Lands.Find(A);
	if (!L || !L->In) return false;
	const double R = M->Player->Radius;
	for (const FVector2D& D : { FVector2D(0, 0), FVector2D(-R, -R), FVector2D(R, -R), FVector2D(-R, R), FVector2D(R, R) })
	{
		if (!L->In(X + D.X, Z + D.Y)) return false;
	}
	const double K = R + LAND_KEEP;
	for (const FS47Box2& B : L->Out) if (X > B.MinX - K && X < B.MaxX + K && Z > B.MinZ - K && Z < B.MaxZ + K) return false;
	if (L->Closed && L->Closed(X, Z)) return false;
	if (const FLegRec* G = Legs.Find(L->Leg))
	{
		for (const FS47Obstacle& O : G->Area->Obstacles)
		{
			if (O.bCircle ? FMath::Square(X - O.X) + FMath::Square(Z - O.Z) < FMath::Square(O.R + R) : X > O.MinX - R && X < O.MaxX + R && Z > O.MinZ - R && Z < O.MaxZ + R) return false;
		}
	}
	return GroundUnder(A, *L, X, Z);
}

bool US47World::GroundUnder(const FString& InArea, const FLand& L, double X, double Z)
{
	// a ray down through each corner of the player's width onto the ground's height there, found once
	// per half-metre cell (the meshes stand still): none, and it is the edge of the world
	const FLegRec* G = Legs.Find(L.Leg);
	UWorld* W = M && M->Web ? M->Web->GetWorld() : nullptr;
	if (!G || !W || !M->Player) return false;
	TMap<FIntPoint, bool>& Cells = Under.FindOrAdd(InArea);
	const double R = M->Player->Radius;
	for (const FVector2D& D : { FVector2D(-R, -R), FVector2D(R, -R), FVector2D(-R, R), FVector2D(R, R) })
	{
		const FIntPoint C(FMath::FloorToInt((X + D.X) * 2), FMath::FloorToInt((Z + D.Y) * 2));
		if (const bool* Have = Cells.Find(C)) { if (!*Have) return false; continue; }
		const double Cx = (C.X + 0.5) / 2, Cz = (C.Y + 0.5) / 2, H = L.Y ? L.Y(Cx, Cz) : G->Area->Height(Cx, Cz);
		FHitResult Hit;
		const FCollisionQueryParams Q(SCENE_QUERY_STAT(S47Land), true);
		const bool bHit = W->LineTraceSingleByChannel(Hit, S47Three::ToUE(FVector(Cx, H + 3, Cz)), S47Three::ToUE(FVector(Cx, H - 6, Cz)), ECC_Visibility, Q);
		Cells.Add(C, bHit);
		if (!bHit) return false;
	}
	return true;
}

void US47World::DriveOnOldRoad()
{
	// back in the truck left on the old road: the drive goes on from where it stands (DriveOldRoad's
	// trip, without its start: the clock and chapter six's road are as they were)
	FS47TripOpts O;
	O.Leg = TEXT("roswell");
	StartTrip(TEXT("roswell"), TEXT("roswell"), O);
}

FS47Truck* US47World::TruckHere() const
{
	if (Area == TEXT("roswell") || Leg == TEXT("roswell")) return nullptr;
	if (bDriving) { const FLegRec* L = Leg.IsEmpty() ? nullptr : Legs.Find(Leg); return L ? L->Truck : nullptr; }
	const FString& At = TruckAt;
	if ((At == TEXT("saro") || At == TEXT("motel")) && Area == TEXT("saro")) return SaroTruckP.Get();
	if (At == TEXT("station01") && Area == TEXT("station01")) return ParkedP.Get();
	if (At == TEXT("diner") && (Area == TEXT("diner") || Area == TEXT("road"))) return TruckP.Get();
	return nullptr;
}

// ---------- getting out wherever the truck is stopped (Tom, 16 and 19 October) ----------

FString US47World::PlaceOfLeg(const FString& Id) const
{
	// (the old road: its own truck is left where it stands, Tom 22 October 2026)
	return Id == TEXT("saro") || Id == TEXT("station01") || Id == TEXT("roswell") ? Id : Id == TEXT("road") ? FString(TEXT("diner")) : FString();
}

bool US47World::AtDiner(const FVector& Q) const
{
	// by the diner's lot (within 6 m) or on Caliche's street (Town.inTown: its walk zones)
	if (M && M->Web)
	{
		for (const FString& P : M->Web->PathsUnder(TEXT("world.town.zones[")))
		{
			const FS47Area Z = M->Web->Rect(P);
			if (Z.Contains(Q.X, Q.Z)) return true;
		}
	}
	const FS47Box2& B = DinerPark;
	return bLegsMade && Q.X > B.MinX - 6 && Q.X < B.MaxX + 6 && Q.Z > B.MinZ - 6 && Q.Z < B.MaxZ + 6;
}

bool US47World::TruckOnLot() const
{
	const FS47Truck* T = TruckAt.IsEmpty() ? nullptr : TruckOf(TruckAt);
	const TOptional<FS47Box2> B = TruckAt.IsEmpty() ? TOptional<FS47Box2>() : ParkBox(TruckAt);
	if (!T || !B) return false;
	if (bCommuteVehicle && TruckAt==TEXT("saro")) return FS47EveningVisual::FitsArrivalArea(T->Position,T->Rotation.Y);
	const FVector& Q = T->Position;
	return Q.X > B->MinX - 2 && Q.X < B->MaxX + 2 && Q.Z > B->MinZ - 2 && Q.Z < B->MaxZ + 2;
}

bool US47World::CanGetOut(bool bDead) const
{
	const FString& Id = Leg;
	if (!bDriving || bBusy || Id.IsEmpty() || !Trip || Trip->bFree || (Trouble.WantsKey() && !bDead)) return false;
	// (chapter six has the truck: its pedals in THE EVENT, its camera after it)
	if (OldControl) return false;
	const FLegRec* L = Legs.Find(Id);
	const FString Place = PlaceOfLeg(Id);
	if (!L || Place.IsEmpty() || FMath::Abs(L->Drive->Speed) > 0.6) return false;
	// Out wherever it is stopped (Tom 22 October 2026). The web version asks here for walk zones by
	// a door (World.walkOf: SARO's ground, STATION 01's, the diner's and Caliche's, the road's stops)
	// and keeps the player in the cab elsewhere; Park gives the ground round the truck there.
	return true;
}

bool US47World::GetOutHere(bool bDead)
{
	if (!CanGetOut(bDead)) return false;
	const FVector Q = Legs[Leg].Drive->Pos;
	const FString Place = Leg == TEXT("saro") && InBox(ParkBox(TEXT("motel")), Q) ? FString(TEXT("motel")) : PlaceOfLeg(Leg);
	UE_LOG(LogS47World, Display, TEXT("S47: getting out, leg %s at %.1f %.1f"), *Leg, Q.X, Q.Z);
	// (the diner's area is always there: no building it first behind the black)
	Park(Place);
	return true;
}

// The autopilot (tests, the developer menu): along the way to the next area, or to where the
// truck is left. Pure pursuit, slowed in time for the bends ahead; when the way lies behind, a
// turn in three points (forward on full lock, back on the other lock when it can go no further,
// and so on until it faces the way). Dt: the step (the stall timer of the turn); 0 to only look.
FS47DriveIn US47World::AutoInput(double Dt)
{
	FS47DriveIn In;
	FLegRec* L = Legs.Find(Leg);
	if (!L) return In;
	FS47DriveController& D = *L->Drive;
	const FString& Id = Leg;
	FString Next;
	if (Trip && !Trip->To.IsEmpty()) { if (S47Legs::LegOf(Trip->To) == Id) Next = TEXT("park"); else { const TArray<FString> B = S47Legs::LegsBetween(Id, Trip->To); Next = B.IsValidIndex(1) ? B[1] : FString(); } }
	const FString RoadTo = Next == TEXT("park") ? Trip->To : Next;
	// to the motel's lot: from the north, or from the south once past it
	const FString Key = Next == TEXT("park") && Trip->To == TEXT("motel") ? (D.Pos.Z > S47Geo::MOTEL_RECT().MinZ + 20 ? FString(TEXT("motelS")) : FString(TEXT("motel"))) : Next;
	const TArray<FVector>* Route = nullptr;
	if (!Key.IsEmpty()) Route = Id == TEXT("road") ? &S47Legs::RoadRoute(RoadTo, D.Pos.Z - ROAD_ORIGIN.Z) : L->Area->Routes.Find(Key);
	const bool Visitor = bCommuteVehicle && Id==TEXT("saro") && Next==TEXT("park") && Trip->To==TEXT("saro");
	// Personal arrival turns off before the ramp. Service routes remain unchanged.
	static const TArray<FVector> VisitorRoute={FVector(-22.2,-.62,60),FVector(-22.2,-.62,32),FVector(-20,-.62,26),FVector(-16.8,-.62,24),FVector(-13,-.62,22),FVector(-12.8,-.62,18),FS47EveningVisual::ArrivalPark()};
	if (Visitor) Route=&VisitorRoute;
	if (!Route || !Route->Num()) { In.Throttle = D.Speed > 0.2 ? -0.6 : 0; return In; }
	const TArray<FVector>& R = *Route;
	int32 K = 0;
	double Best = TNumericLimits<double>::Max();
	for (int32 i = 0; i < R.Num(); ++i) { const double Dd = FVector::DistSquared(R[i], D.Pos); if (Dd < Best) { Best = Dd; K = i; } }
	const double Sp = FMath::Abs(D.Speed), Look = FMath::Clamp(3.5 + Sp * 0.7, 4.0, 16.0);
	int32 J = K;
	while (J < R.Num() - 1 && FVector::Dist(R[J], D.Pos) < Look) J++;
	const FVector& Tg = R[J];
	double Err = FMath::Atan2(-(Tg.X - D.Pos.X), -(Tg.Z - D.Pos.Z)) - D.Heading;
	Err = FMath::Atan2(FMath::Sin(Err), FMath::Cos(Err));
	FAuto& A = Auto;
	// ---------- stuck against something: back off a little, then on ----------
	if (A.Back > 0)
	{
		A.Back -= Dt;
		if (!D.Reversing()) { In.Throttle = -1; return In; }
		In.Steer = Err > 0 ? 0.6 : -0.6; In.Throttle = Sp < 1.2 ? -0.6 : 0;
		return In;
	}
	// ---------- turning round ----------
	if (FMath::Abs(Err) > 0.9 || A.bRev)
	{
		const double Want = A.bRev ? -1 : 1;
		const bool bMoving = D.Reversing() == A.bRev && Sp > 0.25;
		A.Stall = bMoving ? 0 : A.Stall + Dt;
		if (A.bRev && (FMath::Abs(Err) < 0.6 || A.Stall > 1.2)) { A.bRev = false; A.Stall = 0; }
		else if (!A.bRev && A.Stall > 1.2 && FMath::Abs(Err) > 0.9) { A.bRev = true; A.Stall = 0; }
		const double Lock = Err > 0 ? 1 : -1;
		if (A.bRev) { In.Steer = Lock; In.Throttle = D.Reversing() ? (Sp < 1.8 ? -0.7 : 0) : -1; return In; }
		if (D.Reversing()) { In.Steer = -Lock; In.Throttle = 1; return In; }
		In.Steer = -Lock; In.Throttle = FMath::Clamp((2 * Want - D.Speed) * 0.6, -1.0, 0.7);
		return In;
	}
	A.Stall = 0;
	if (D.Reversing()) { In.Throttle = 1; return In; }
	// ---------- along the way: the speed each bend ahead allows, braked to in time ----------
	double V = FMath::Min(Autopilot.Get(15), D.Surface.Top * 0.7);
	double Dist = FVector::Dist(R[K], D.Pos);
	for (int32 i = K; i < R.Num() - 2 && Dist < 90; ++i)
	{
		const FVector& P0 = R[i];
		const FVector& P1 = R[i + 1];
		const FVector& P2 = R[i + 2];
		const double H1 = FMath::Atan2(P1.X - P0.X, P1.Z - P0.Z), H2 = FMath::Atan2(P2.X - P1.X, P2.Z - P1.Z);
		const double Bend = FMath::Abs(FMath::Atan2(FMath::Sin(H2 - H1), FMath::Cos(H2 - H1)));
		const double Curv = Bend / FMath::Max((FVector::Dist(P1, P0) + FVector::Dist(P2, P1)) / 2, 0.5);
		const double Vb = FMath::Sqrt(2.2 / FMath::Max(Curv, 1e-4));       // about 2.2 m/s² sideways
		V = FMath::Min(V, FMath::Sqrt(Vb * Vb + 2 * 2.5 * Dist));          // braking at 2.5 m/s²
		Dist += FVector::Dist(P1, P0);
	}
	const double End = Next == TEXT("park") ? FVector::Dist(R.Last(), D.Pos) : TNumericLimits<double>::Max();
	const double StopDistance = Visitor ? .12 : 2.2;
	V = FMath::Min(V, FMath::Max(0.0, (End - StopDistance) * 0.6)) * FMath::Clamp(1.15 - FMath::Abs(Err) * 1.4, 0.3, 1.0);
	if (End < StopDistance) V = 0;
	// wanting to go but not going (a post, a kerb): back off for a moment
	A.Stuck = V > 0.8 && Sp < 0.15 ? A.Stuck + Dt : 0;
	if (A.Stuck > 1.5) { A.Stuck = 0; A.Back = 1.4; }
	// pure pursuit: the curvature that reaches the aim point, turned into the wheel the truck has
	// at this speed on this ground (the same lock as the physics, Drive.ts)
	const double Kappa = 2 * FMath::Sin(Err) / Look;
	In.Steer = D.SteerFor(-Kappa);
	In.Throttle = FMath::Clamp((V - D.Speed) * 0.5, -1.0, 1.0);
	return In;
}

void US47World::StartEngine()
{
	if (!M || !M->Audio) return;
	if (!Engine) Engine = MakeUnique<FS47EngineSound>(M->Audio.Get());
	// the keys, left in it, jingle as it is turned (the start's recording is quiet at its head)
	if (!Engine->On()) { FS47Sound S; S.Gain = 0.6f; M->Audio->Sfx(M->Audio->Or(TEXT("keys")), S); }
	Engine->Start();
}

void US47World::StopDriving()
{
	const FString Id = Leg;
	FLegRec* L = Id.IsEmpty() ? nullptr : Legs.Find(Id);
	Trouble.Reset(L ? L->Truck : nullptr, L ? L->Drive.Get() : nullptr);
	bDriving = false;
	OldControl.Reset();
	if (Engine) Engine->Stop();
	if (L) { L->Truck->SetDriving(false); L->Truck->SetHeadlights(false); }
	if (Trip)
	{
		for (const FHome& H : TrucksHome) { H.T->SetPose(FVector(H.X, H.Y, H.Z), FVector(H.P, H.H, H.R)); H.T->SetDriving(false); }
		TruckAt = Trip->Home;
	}
	Leg.Reset(); Trip.Reset(); Autopilot.Reset();
	for (const TCHAR* P : TRUCKS) ParkAt(P);
	SetGround(TruckAt);
	Free.Reset(); FreeBack.Reset();
}

TOptional<double> US47World::TripSecs() const
{
	if (!bDriving || !Trip || Trip->bFree) return {};
	return Trip->Secs;
}

double US47World::DrivingSpeed() const
{
	const FLegRec* L = bDriving && !Leg.IsEmpty() ? Legs.Find(Leg) : nullptr;
	return L ? L->Drive->Speed : 0;
}

S47J::FObj US47World::TruckSave() const
{
	const FString At = Trip ? Trip->Home : TruckAt;
	const FS47Truck* T = At.IsEmpty() ? nullptr : TruckOf(At);
	if (At.IsEmpty() || !T) return nullptr;
	const FHome* Home = Trip ? TrucksHome.FindByPredicate([T](const FHome& H) { return H.T == T; }) : nullptr;
	S47J::FObj Pose = S47J::New();
	S47J::Set(Pose, TEXT("at"), At);
	S47J::Set(Pose, TEXT("x"), Home ? Home->X : T->Position.X);
	S47J::Set(Pose, TEXT("y"), Home ? Home->Y : T->Position.Y);
	S47J::Set(Pose, TEXT("z"), Home ? Home->Z : T->Position.Z);
	S47J::Set(Pose, TEXT("h"), Home ? Home->H : T->Rotation.Y);
	S47J::Set(Pose, TEXT("p"), Home ? Home->P : T->Rotation.X);
	S47J::Set(Pose, TEXT("r"), Home ? Home->R : T->Rotation.Z);
	S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("at"), At);
	S47J::Set(O, TEXT("area"), AreaOf(At));
	S47J::Set(O, TEXT("pose"), Pose);
	return O;
}

void US47World::PlaceTruck(const FString& At, const S47J::FObj& Pose)
{
	EnsureLegs();
	StopDriving();
	TruckAt = At;
	auto Put = [this](FS47Truck* T, double X, double Z, double H, double Y)
	{
		if (!T) return;
		// Canonical lots are level. The commute's last suspension pitch/roll belongs
		// to that road pose, not to SARO 07 waiting on its original concrete pad.
		T->SetParkPose(FVector(X, Y, Z), H);
		T->SetPersonalVehicle(bCommuteVehicle && (T == SaroTruckP.Get() || T == TruckP.Get()));
	};
	// YARD.truck: on its pad by the service yard
	const S47Geo::FAt& MA = S47Geo::MOTEL_AT();
	// on the motel's lot, nosed in towards the office
	if (At == TEXT("motel")) Put(SaroTruckP.Get(), -35.3 + MA.X, 9.5 + MA.Z, UE_DOUBLE_PI / 2, MA.Y);
	else Put(SaroTruckP.Get(), 8.9, 8.0, UE_DOUBLE_PI, 0);
	// the station's truck anchor (Station01 anchors.truck: STATION_TRUCK at its origin)
	Put(ParkedP.Get(), S47Geo::STATION_TRUCK.X + STATION_ORIGIN.X, S47Geo::STATION_TRUCK.Z + STATION_ORIGIN.Z, S47Geo::STATION_TRUCK.Heading, 0);
	// at the diner it is the road's truck that stands on the lot (Diner anchors.truckPark)
	const FVector DO = DINER_ORIGIN();
	if (At == TEXT("diner")) Put(TruckP.Get(), DO.X + 9, DO.Z + 6, 0, DO.Y);
	// a save from 18 October 2026 on knows the very spot: only for the place it is at, and only
	// within reach of that place (the areas lie kilometres apart), or it goes on the place as before
	FS47Truck* T = TruckOf(At);
	auto Ok = [&Pose](const TCHAR* K) { double V = 0; return Pose.IsValid() && Pose->TryGetNumberField(K, V) && FMath::IsFinite(V); };
	if (T && Pose.IsValid() && S47J::Str(Pose, TEXT("at")) == At && Ok(TEXT("x")) && Ok(TEXT("y")) && Ok(TEXT("z")) && Ok(TEXT("h"))
		&& FMath::Sqrt(FMath::Square(S47J::Num(Pose, TEXT("x")) - T->Position.X) + FMath::Square(S47J::Num(Pose, TEXT("z")) - T->Position.Z)) < 2600)
	{
		T->SetPose(FVector(S47J::Num(Pose, TEXT("x")), S47J::Num(Pose, TEXT("y")), S47J::Num(Pose, TEXT("z"))),
			FVector(Ok(TEXT("p")) ? S47J::Num(Pose, TEXT("p")) : 0, S47J::Num(Pose, TEXT("h")), Ok(TEXT("r")) ? S47J::Num(Pose, TEXT("r")) : 0));
		// Repair old opening saves: the flat original pad cannot inherit ramp tilt.
		if (At == TEXT("saro") && T->Position.Equals(FVector(8.9, 0, 8), 0.01))
			T->SetParkPose(T->Position, T->Rotation.Y);
	}
	for (const TCHAR* P : TRUCKS) ParkAt(P);
	// (a save made on foot by the truck left somewhere with no walk of its own: the ground round it again)
	SetGround(At);
}

S47J::FObj US47World::OdoSave() const
{
	S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("m"), double(FMath::RoundToDouble(OdoM)));
	S47J::Set(O, TEXT("x"), FMath::RoundToDouble(OdoX * 1000) / 1000);
	if (OdoAtSaro) S47J::Set(O, TEXT("s"), FMath::RoundToDouble(*OdoAtSaro * 1000) / 1000);
	return O;
}

void US47World::OdoRestore(const S47J::FObj& O)
{
	auto N = [&O](const TCHAR* K) { double V = 0; return O.IsValid() && O->TryGetNumberField(K, V) && FMath::IsFinite(V) && V >= 0 ? V : 0.0; };
	OdoM = N(TEXT("m")); OdoX = N(TEXT("x"));
	double S = 0;
	if (O.IsValid() && O->TryGetNumberField(TEXT("s"), S) && FMath::IsFinite(S)) OdoAtSaro = S; else OdoAtSaro.Reset();
	EnsureLegs();
	for (FS47Truck* T : { SaroTruckP.Get(), TruckP.Get(), ParkedP.Get(), OldTruckP.Get() }) if (T) T->SetOdometer(Odometer());
}

bool US47World::Indoors(const FVector& P) const
{
	if (!M || !M->Web) return false;
	if (Area == TEXT("room6")) return P.Z < -8000.0 + 3.0;  // ROOM6_ORIGIN.z + 3: in the room, not on the walk outside
	if (Area == TEXT("diner"))
	{
		for (const FS47Rect& Z : M->Web->Zones())
		{
			if ((Z.Id == TEXT("inside") || Z.Id == TEXT("phone")) && P.X >= Z.MinX && P.X <= Z.MaxX && P.Z >= Z.MinZ && P.Z <= Z.MaxZ) return true;
		}
		return false;
	}
	if (Area != TEXT("station01")) return false;
	return M->Web->Rect(TEXT("world.site.hutBounds")).Contains(P);
}

bool US47World::MoveDriven(double X, double Z, double Heading)
{
	// World.moveDriven
	FLegRec* L = Leg.IsEmpty() ? nullptr : Legs.Find(Leg);
	if (!bDriving || !L || !Trip) return false;
	L->Drive->Place(FVector(X, 0, Z), Heading);
	L->Drive->bEngine = false;
	if (Engine) Engine->Still();
	const FS47Truck& G = *L->Truck;
	for (FHome& H : TrucksHome) if (H.T == L->Truck) { H.X = G.Position.X; H.Y = G.Position.Y; H.Z = G.Position.Z; H.H = G.Rotation.Y; H.P = G.Rotation.X; H.R = G.Rotation.Z; }
	const FString Place = PlaceOfLeg(Leg);
	if (!Place.IsEmpty()) Trip->Home = Place;
	Trip->StartPos = G.Position; Trip->bLeft = true;
	if (const TOptional<FVector> V = DoorSpot(G, Leg)) this->Place(V->X, V->Z, Heading);
	return true;
}

bool US47World::MoveParked(double X, double Z, double Heading, double Y)
{
	// World.moveParked
	FS47Truck* T = SaroTruckP.Get();
	if (bDriving || !T || (TruckAt != TEXT("saro") && TruckAt != TEXT("motel"))) return false;
	T->SetPose(FVector(X, Y, Z), FVector(0, Heading, 0));
	TruckAt = TEXT("saro");
	ParkAt(TEXT("saro"));
	SetGround(TruckAt);
	return true;
}

void US47World::AddMiles(double Miles)
{
	OdoX += Miles;
	if (FS47Truck* T = TruckHere()) T->SetOdometer(Odometer());
}

// ---------- free drive (the developer menu): the truck on a road, any time, no story ----------

void US47World::FreeDrive(const FString& Which)
{
	if (bBusy || !M || !M->Player) return;
	EnsureLegs();
	const FFreeBack Back = FreeBack ? *FreeBack : FFreeBack{ Area, M->Player->Pos.X, M->Player->Pos.Z, M->Player->Yaw, M->Player->Pitch };
	if (bDriving) StopDriving();
	FreeBack = Back;
	const bool bRoad = Which == TEXT("road");
	FS47TripOpts O;
	if (bRoad && Road) { O.AtPos = Road->Start.Pos; O.AtHeading = Road->Start.Heading; }
	else if (FS47OldRoad* R = OldRoad()) { O.AtPos = R->Start.Pos; O.AtHeading = R->Start.Heading; R->bHitBack = false; }
	O.bFree = true;
	O.Legs = bRoad ? TArray<FString>{ TEXT("saro"), TEXT("road"), TEXT("station01") } : TArray<FString>{ TEXT("roswell") };
	O.OnStart = [this, Which, Back]
	{
		Free = Which == TEXT("road") ? Which : FString(TEXT("oldroad")); FreeBack = Back;
		Toast(TEXT("Free drive. The developer menu takes you back."), 3.5f);
	};
	StartTrip(bRoad ? FString(TEXT("road")) : FString(TEXT("roswell")), FString(), O);
}

void US47World::EndFreeDrive()
{
	const TOptional<FFreeBack> Back = FreeBack;
	if (Free.IsEmpty() || !Back) return;
	StopDriving();
	Enter(Back->Area);
	Place(Back->X, Back->Z, Back->Yaw);
	if (M && M->Player) M->Player->Pitch = Back->Pitch;
}

// ---------- in the cab ----------

bool US47World::RadioAimed() const
{
	// main.ts radioAimed: a ray from the view's centre to the radio's faceplate, 1.6 m at most
	const FS47Truck* T = bDriving ? TruckHere() : nullptr;
	if (!T || !bHaveView) return false;
	const FQuat Q = S47Three::Euler(LastView.Pitch, LastView.Yaw, LastView.Roll, TEXT("YXZ"));
	return T->RadioHit(LastView.Eye, Q.RotateVector(FVector(0, 0, -1)), 1.6);
}

bool US47World::CabUse()
{
	// main.ts useCurrent, in the truck: Use turns the key when the engine has died and waits for
	// it, uses the radio when it is looked at, or gets out where the truck stands when it is stopped
	if (!M || !bDriving || M->ModalOpen() || (M->Story && M->Story->IsCinematic())) return false;
	if (OnCabUse && OnCabUse()) return true;
	if (Trouble.TurnKey()) return true;
	if (OnRadio && RadioAimed()) { OnRadio(); return true; }
	// (not out in the middle of the truck's count on B, nor by Use in its warning on B: main.ts useCurrent)
	if (CabStay && CabStay()) return true;
	GetOutHere();
	return true;
}

bool US47World::CabRadio()
{
	// main.ts turnRadio: on, then the next station along the dial, and after the last, off
	if (!M || !bDriving || M->ModalOpen() || (M->Story && M->Story->IsCinematic())) return false;
	if (!TruckHere()) { Toast(TEXT("The radio stays on the Roswell station."), 2.5f); return true; }
	if (OnRadio) OnRadio();
	else
	{
		static bool bTold = false;
		if (!bTold) { bTold = true; UE_LOG(LogS47World, Display, TEXT("S47: the truck's radio is not ported yet")); }
	}
	return true;
}

// ---------- per frame ----------

void US47World::Update(double Dt, double T) { (void)Dt; (void)T; }

bool US47World::Frame(double Dt, const FVector2D& Move, const FVector2D& LookIn, FS47View& Out)
{
	if (!M || !bLegsMade) return false;
	if (Engine) Engine->Tick();
	const bool bModal = M->ModalOpen();
	// World.update: the road's cattle guard under the truck driven there (before the step, as there)
	if (!bModal && (Area == TEXT("road") || Area == TEXT("diner")) && Road)
	{
		if (const FLegRec* R = Legs.Find(TEXT("road"))) Road->Update(Dt, M->T, R->Drive->Pos);
	}
	// drivingStep: the truck being driven, by the keys, by chapter six, by the autopilot or by a test
	const FString Id = Leg;
	FLegRec* L = !Id.IsEmpty() && Id == Area ? Legs.Find(Id) : nullptr;
	if (!L) return false;
	// (main.ts: nothing is stepped while a panel is open; the view stays where it was)
	if (bModal) { if (bHaveView) Out = LastView; return bHaveView && (bDriving || (OldControl && OldControl->Camera)); }
	const FOldControl* C = Id == TEXT("roswell") && OldControl ? OldControl.GetPtrOrNull() : nullptr;
	bool bCamera = false;
	if (C && C->Camera) { C->Camera(Dt, Out); bCamera = true; }
	else if (bDriving)
	{
		// main.ts: steer = move.x, throttle = -move.z
		FS47DriveIn In;
		// Xbox uses proportional triggers for throttle/brake; left-stick forward never accelerates.
		In.Steer = FMath::Clamp(Move.X, -1., 1.);
		In.Throttle = FMath::Clamp(-Move.Y, -1., 1.);
		if (C && C->Input) In = C->Input(Dt);
		else if (Autopilot) In = AutoInput(Dt);
		else if (TestInput) In = TestInput();
		L->Drive->bCameraMotion = !M->Player || M->Player->bHeadMotion;
		L->Drive->Update(Dt, In, LookIn);
		TripStep(Dt);
	}
	FLegRec* N = (!Leg.IsEmpty() ? Legs.Find(Leg) : nullptr);
	if (!N) N = L;
	FS47DriveController& D = *N->Drive;
	// trouble with the truck, on the story's trips and the player's own (not chapter six's road)
	if (bDriving && Trip && Id != TEXT("roswell")) Trouble.Step(Dt, D, *N->Truck, Engine.Get(), !bCommuteVehicle && TroubleOk ? TroubleOk() : false);
	if (Engine) { Engine->Set(D.Rpm, D.Load); Engine->Tyres(D.Surface.Kind, FMath::Abs(D.Speed)); }
	// the odometer turns with the miles driven (not on a free drive)
	if (bDriving && !bCommuteVehicle && !(Trip && Trip->bFree) && Free.IsEmpty()) OdoM += FMath::Abs(D.Speed) * Dt;
	DashT -= Dt;
	if (DashT <= 0)
	{
		DashT = 0.1;
		N->Truck->SetDash(D.Mph, D.Rpm, M->Story && !bCommuteVehicle ? M->Story->ClockText(false) : FString(), Id == TEXT("roswell") ? 0.55 : 0.62);
		N->Truck->SetOdometer(Odometer());
	}
	// main.ts: the compass on the dash of the truck being driven reads the sky's north here
	if (bDriving) N->Truck->CompassNorth = SkyYaw();
	if (!bCamera)
	{
		const FVector E = EulerYXZ(D.CamQuat);
		Out.Eye = D.CamPos; Out.Pitch = E.X; Out.Yaw = E.Y; Out.Roll = E.Z;
	}
	// a camera that has left the cab sees the whole truck in this very frame
	N->Truck->ViewFrom(Out.Eye);
	if (bCommuteVehicle) N->Truck->SetPersonalVehicle(true);
	const bool bView = bDriving || bCamera;
	if (bView) { LastView = Out; bHaveView = true; }
	// main.ts: in the cab the prompt is the key, the radio or the way out
	if (bDriving && M->UI && !(M->Story && M->Story->IsCinematic()))
	{
		// (held at the wheel on waking: no key; in the truck's warning on B, Q gets out: story/abduction.ts, truckTaken.ts)
		const bool bKey = (Trouble.WantsKey() || (CabWantsKey && CabWantsKey())) && !(CabHeld && CabHeld()), bRadio = !bKey && OnRadio && RadioAimed(), bOut = !bKey && !bRadio && !(CabStay && CabStay()) && CanGetOut();
		const bool bRun = CabRunOffered && CabRunOffered();
		M->UI->Prompt(bKey ? TEXT("Turn the key") : bRadio ? (!M->CarRadio || !M->CarRadio->bOn ? TEXT("Turn the radio on") : M->CarRadio->Station == TEXT("far") ? TEXT("Turn the radio off") : FString::Printf(TEXT("AM %d: next station"), S47Radio::Freq(M->CarRadio->Station))) : bOut ? TEXT("Get out") : TEXT(""), bRun ? TEXT("Get out") : TEXT(""));
	}
	return bView;
}

// World.ts tripNow: only the actual night trip, with the direction inferred on an open drive.
TOptional<FS47TripNow> US47World::TripNow() const
{
	if (!bDriving || !Trip.IsSet() || Trip->bFree || Leg.IsEmpty() || Leg == TEXT("roswell")) return {};
	const FLegRec* L = Legs.Find(Leg); if (!L || !L->Drive) return {};
	const FS47DriveController& D = *L->Drive;
	return FS47TripNow{Trip->N, Trip->From, Trip->To, !Trip->To.IsEmpty() ? Trip->To : S47Legs::BoundOn(Leg, D.Pos.X, D.Pos.Z, D.Heading, D.Reversing()), Trip->Metres};
}
bool US47World::AtMotel(const FVector& P) const
{
	const auto& R = S47Geo::MOTEL_RECT(); return P.X >= R.MinX && P.X <= R.MaxX + 1 && P.Z >= R.MinZ - 4 && P.Z <= R.MaxZ + 4;
}
bool US47World::RoadStopAt(const FVector& P) const
{
	if (Area != TEXT("diner") || !Road || Indoors(P)) return false;
	if (M && M->Web) for (int32 I = 0;; I++)
	{
		const FS47Area Zone = M->Web->Rect(FString::Printf(TEXT("world.town.zones[%d]"), I)); if (!Zone.bValid) break; if (Zone.Contains(P)) return false;
	}
	const double X = P.X - ROAD_ORIGIN.X, Z = P.Z - ROAD_ORIGIN.Z;
	for (const auto& R : S47RoadTerrain::ROAD_STOPS()) if (X >= R.MinX && X <= R.MaxX && Z >= R.MinZ && Z <= R.MaxZ) return true;
	return false;
}

// World.ensureTown: the patrol car's mutable collider also belongs to the road's obstacles.
void US47World::TownCarMoved(const FS47Area& Box)
{
	EnsureLegs(); if (!Road || !M || !M->Web) return;
	if (TownCarObstacle < 0)
	{
		const auto Original = M->Web->Rect(TEXT("world.town.carCol"));
		TownCarObstacle = Road->Obstacles.IndexOfByPredicate([&](const FS47Obstacle& O) { return !O.bCircle && FMath::Abs(O.MinX - Original.MinX) < .01 && FMath::Abs(O.MinZ - Original.MinZ) < .01; });
	}
	if (Road->Obstacles.IsValidIndex(TownCarObstacle)) Road->Obstacles[TownCarObstacle] = FS47Obstacle::Box(Box.MinX, Box.MaxX, Box.MinZ, Box.MaxZ);
}

// Evening opening: drive the existing continuous highway north through Caliche.
void US47World::BeginCommute(const S47J::FObj& SavedCar)
{
 EnsureLegs(); StopDriving(); bCommuteVehicle = true;
 for (FS47Truck* T : {SaroTruckP.Get(),TruckP.Get()}) if (T) T->SetPersonalVehicle(true);
 FS47TripOpts O;
 const bool Saved = SavedCar.IsValid();
 const FString A = Saved ? S47J::Str(SavedCar, TEXT("leg"), TEXT("road")) : FString(TEXT("road"));
 const FString Leg0 = A == TEXT("saro") ? FString(TEXT("saro")) : FString(TEXT("road"));
 O.Leg = Leg0;
 O.AtPos = Saved ? FVector(S47J::Num(SavedCar,TEXT("x")), S47J::Num(SavedCar,TEXT("y")), S47J::Num(SavedCar,TEXT("z"))) : FVector(8001.8,0,S47RoadShape::TOWN().Z1+35);
 O.AtHeading = Saved ? S47J::Num(SavedCar,TEXT("h")) : 0;
 O.Hint = TEXT("SARO: stop anywhere in the visitor parking area left of the sign. Supper at Mesa Diner is optional.");
 O.OnStart = [this] { Toast(S47Gamepad::UsingGamepad() ? TEXT("RT: throttle. LT: brake / reverse. LS: steer. D-pad down: radio. Stop and press A to get out. SARO is north through town; the diner stop is optional.") : TEXT("W/S: throttle and brake. A/D: steer. R: radio. Stop and press E to get out. SARO is north through town; the diner stop is optional."),9); };
 TruckAt = TEXT("saro");
 FString Route;
 const bool bTest=FParse::Value(FCommandLine::Get(),TEXT("S47EveningRoute="),Route);
 if (bTest) {float Speed=24;FParse::Value(FCommandLine::Get(),TEXT("S47Auto="),Speed); Autopilot=Speed;}
 StartTrip(Leg0,bTest && Route!=TEXT("direct")?TEXT("diner"):TEXT("saro"),O);
}

void US47World::FinishCommute()
{
 StopDriving(); bCommuteVehicle = false;
 for (FS47Truck* T : {SaroTruckP.Get(),TruckP.Get()}) if (T) T->SetPersonalVehicle(false);
 // SARO 07 has stayed at work. The player's car is retained separately by Evening.
 PlaceTruck(TEXT("saro"));
 if (SaroTruckP) { SaroTruckP->Body.SetVisible(true); SaroTruckP->Shell.SetVisible(true); }
 if (TruckP) { TruckP->Body.SetVisible(true); TruckP->Shell.SetVisible(true); }
 Enter(TEXT("saro"));
}

void US47World::SetPersonalPark(const FVector& Position, double Heading, bool bVisible)
{
 if (!M || !M->Web || !M->Player) return;
 auto& Cols=M->Web->CollidersMutable(TEXT("saro"));
 // Doors append/remove leaves in this same array. Never retain a stale index.
 PersonalCol=Cols.IndexOfByPredicate([](const FS47Rect& R){return R.Id==TEXT("personalCar");});
 if (PersonalCol<0) {FS47Rect R;R.Id=TEXT("personalCar");PersonalCol=Cols.Add(R);}
 FS47Rect& R=Cols[PersonalCol]; R.bEnabled=bVisible;
 if (!bVisible)
 {
  R.MinX=R.MaxX=R.MinZ=R.MaxZ=1e9;M->Player->PersonalCarBox.Reset();return;
 }
 AS47Walker::FTruckBox B; B.Area=TEXT("saro"); B.X=Position.X; B.Z=Position.Z; B.Heading=Heading;
 B.HalfW=FS47EveningVisual::PersonalHalfWidth();B.HalfL=FS47EveningVisual::PersonalHalfLength();
 const double C=FMath::Abs(FMath::Cos(Heading)),S=FMath::Abs(FMath::Sin(Heading));
 const double Hx=B.HalfW*C+B.HalfL*S,Hz=B.HalfW*S+B.HalfL*C;
 R.MinX=B.X-Hx;R.MaxX=B.X+Hx;R.MinZ=B.Z-Hz;R.MaxZ=B.Z+Hz;
 M->Player->PersonalCarBox=B;
}

void US47World::SetArrivalServiceWaiting(bool bVisible)
{
 if (!M || !M->Web) return;
 auto& Cols=M->Web->CollidersMutable(TEXT("saro"));
 int32 Index=Cols.IndexOfByPredicate([](const FS47Rect& R){return R.Id==TEXT("arrivalServicePickup");});
 if(Index<0){if(!bVisible)return;FS47Rect R;R.Id=TEXT("arrivalServicePickup");Index=Cols.Add(R);}
 FS47Rect& R=Cols[Index];R.bEnabled=bVisible;
 // Same original pad as SARO 07; the regular TruckBox takes over after arrival.
 R.MinX=bVisible?7.85:1e9;R.MaxX=bVisible?9.95:1e9;
 R.MinZ=bVisible?5.25:1e9;R.MaxZ=bVisible?10.75:1e9;
 if(FLegRec* L=Legs.Find(TEXT("saro")))
 {
  auto& Obstacles=L->Area->Obstacles;
  if(!Obstacles.IsValidIndex(ArrivalServiceObstacle))
  {
   if(!bVisible)return;
   ArrivalServiceObstacle=Obstacles.Add(FS47Obstacle::Box(1e9,1e9,1e9,1e9));
  }
  Obstacles[ArrivalServiceObstacle]=FS47Obstacle::Box(R.MinX,R.MaxX,R.MinZ,R.MaxZ);
 }
}
