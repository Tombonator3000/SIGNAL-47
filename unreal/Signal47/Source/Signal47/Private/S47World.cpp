#include "S47World.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "S47Audio.h"
#include "S47Geo.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogS47World, Log, All);

namespace
{
	// world/World.ts: FOG and FOG_OLDROAD (the old road's air is thinner at dawn)
	constexpr float FOG = 0.0021f, FOG_OLDROAD = 0.00055f;
	const FVector ROAD_ORIGIN(8000, 0, 0);
	const FVector STATION_ORIGIN(0, 0, 8000);
	// Mesa Diner stands in the road area, beside the highway 2 km south of the start (roadShape.ts)
	FVector DINER_ORIGIN() { const S47RoadShape::FDinerAt& D = S47RoadShape::DINER(); return FVector(8000 + D.X, D.Y + 0.1, D.Z); }
	/** The places the truck can be left on (a lot with a park box): SARO's pad, the motel's lot,
	 *  the gravel outside STATION 01's gate, the diner's lot. */
	const TCHAR* PLACES[4] = { TEXT("saro"), TEXT("motel"), TEXT("station01"), TEXT("diner") };
	/** The places with a truck of their own (the motel's is SARO's: the same area). */
	const TCHAR* TRUCKS[3] = { TEXT("saro"), TEXT("station01"), TEXT("diner") };
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
	const TCHAR* EXAMINE_TRUCK = TEXT("SARO 07: a white pickup with tired paint and a toolbox in the bed. The keys are in it, like always.");

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
			if (M && M->Story && !M->Story->At(ES47Phase::Ch1))
			{
				return { TEXT("Service truck"), [this] { Toast(TEXT("SARO 07, the service truck. Not now: someone has to sit the console tonight."), 3.4f); } };
			}
			return { TEXT("Drive"), [this, Place] { Roam(Place); } };
		};
	}
	// main.ts input.onKey and useCurrent: in the cab Use turns the key, uses the radio or gets out;
	// R is the radio
	for (const FKey& K : { EKeys::E, EKeys::Enter, EKeys::LeftMouseButton }) M->OnKey(K, [this] { return CabUse(); });
	M->OnKey(EKeys::R, [this] { return CabRadio(); });
	// the truck where it stands: the yard's (Chapter3.bindSaroTruck) and the station's (bindSite);
	// the chapter adds its own with the same ids when it is ported
	if (M->Inter && M->Web)
	{
		FS47Interactable T;
		T.Id = TEXT("truck");
		T.Object = M->Web->Obj(TEXT("yard.objs.truck"));
		T.Range = 3.2;
		T.Label = [this] { return TruckChoice ? TruckChoice(TEXT("saro")).Label : FString(); };
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
	}
	// the view from the cab while the truck is driven
	if (M->Player) M->Player->CameraOverride = [this](double Dt, const FVector2D& Move, const FVector2D& LookIn, FS47View& Out) { return Frame(Dt, Move, LookIn, Out); };

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
		TUniquePtr<FS47SaroDrive> A = MakeUnique<FS47SaroDrive>(Boxes(W->CollidersMutable(TEXT("saro")), SaroCol), TArray<FS47Obstacle>());
		Fill(*A, TEXT("saro"), nullptr);
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
		auto Here = [this](const TCHAR* Id) { return Area == Id && (TruckAt == Id || Leg == Id); };
		// (on the motel's lot it is SARO's truck too, in SARO's area)
		SaroTruckP->Group.SetVisible(Here(TEXT("saro")) || (Area == TEXT("saro") && TruckAt == TEXT("motel")));
		const bool bOnRoad = Area == TEXT("road") || Area == TEXT("diner");
		TruckP->Group.SetVisible(bOnRoad && (Leg == TEXT("road") || TruckAt == TEXT("diner")));
		ParkedP->Group.SetVisible(Here(TEXT("station01")));
		OldTruckP->Group.SetVisible(Area == TEXT("roswell"));
	}
	if (OnEnter) OnEnter(NewArea);
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
		if (FirstTime(TEXT("drive"))) Say.Add(TEXT("W, S, A and D: drive. R: radio. Stop and press E to get out."));
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
	O.Hint = TEXT("The truck goes back on its pad by the service yard: up the ramp, and stop there.");
	O.OnArrive = [this]
	{
		if (OnArriveSaro) OnArriveSaro();
		// (left on SARO's ground somewhere else, out among the masts say, it is back at SARO but not on its pad)
		if (InBox(ParkBox(TEXT("saro")), SaroTruckP->Position) && M && M->Story) Toast(FString::Printf(TEXT("%s. The truck is back on its pad."), *M->Story->ClockText(false)), 3.f);
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
		if (FirstTime(TEXT("drive"))) Toast(TEXT("W and S: throttle and brake (S at a stop: reverse). A and D: steer."), 4.5f);
	};
	StartTrip(TEXT("diner"), TEXT("roswell"), O);
}

// ---------- trips ----------

void US47World::ParkAt(FString Id)
{
	// The parked truck's collider and its handle for "use" go where the truck stands, or out of
	// the way while it is gone. (At the motel it is SARO's truck, collider and handle.)
	if (Id == TEXT("motel")) Id = TEXT("saro");
	FS47Truck* T = TruckOf(Id);
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
	};
	if (Id == TEXT("saro")) Put(TEXT("saro"), SaroCol);
	else if (Id == TEXT("station01")) Put(TEXT("station01"), StationCol);
	// (the diner's walls are walked in the diner's area and in the road's, World.roadWalk: the same list there)
	else if (Id == TEXT("diner")) { Put(TEXT("diner"), DinerCol); Put(TEXT("road"), DinerCol); }
	FS47Obj Proxy = Id == TEXT("saro") ? SaroProxy : Id == TEXT("station01") ? StationProxy : FS47Obj();
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
		if (!O.bFree) { TruckAt.Reset(); for (const TCHAR* Id : TRUCKS) ParkAt(Id); }
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
		else if (FirstTime(TEXT("drive"))) Toast(TEXT("W and S: throttle and brake (S at a stop: reverse). A and D: steer. R: radio. Stopped, E gets out."), 5.f);
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
	if (T.PerMetre && M && M->Story) M->Story->SetClock(M->Story->Clock() + FMath::Abs(D.Speed) * Dt * T.PerMetre);
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
			if ((Pl == T.To || T.bLeft) && S47Legs::LegOf(Pl) == Id && InBox(ParkBox(Pl), D.Pos)) { Stop = Pl; break; }
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
	// the trip's place reached; on the highway that is the diner's lot or Caliche's street
	const bool bArrived = Trip && Trip->To == Place && (Place != TEXT("diner") || AtDiner(L.Drive->Pos));
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
	// out by the door at once: from this frame the camera is the player's again
	Enter(AreaOf(Place));
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
	const TOptional<FVector> Spot = DoorSpot(T);
	const FVector V = Spot ? *Spot : AboutUp(FVector(-1.75, 0, -0.3), H) + T.Position;
	Place(V.X, V.Z, H);
}

/** Ground to stand on by one of the truck's doors, in the given walk (the area's walkable zones
 *  and what stands on them; the player's own by default), clear of everything in the way, or unset. */
TOptional<FVector> US47World::DoorSpot(const FS47Truck& T, const FString& WalkArea) const
{
	if (!M || !M->Web || !M->Player) return {};
	const double H = T.Rotation.Y, R = M->Player->Radius, E = 1e-6;
	const TArray<FS47Rect>& Zones = WalkArea.IsEmpty() ? M->Web->Zones() : M->Web->ZonesMutable(WalkArea);
	const TArray<FS47Rect>& Cols = WalkArea.IsEmpty() ? M->Web->Colliders() : M->Web->CollidersMutable(WalkArea);
	for (const double Side : { -1.0, 1.0, -1.4, 1.4 })
	{
		const FVector V = AboutUp(FVector(Side * 1.75, 0, -0.3), H) + T.Position;
		bool bWalk = false;
		for (const FS47Rect& B : Zones) if (B.bEnabled && V.X >= B.MinX + R - E && V.X <= B.MaxX - R + E && V.Z >= B.MinZ + R - E && V.Z <= B.MaxZ - R + E) { bWalk = true; break; }
		if (!bWalk) continue;
		bool bHit = false;
		for (const FS47Rect& C : Cols) if (V.X + R > C.MinX && V.X - R < C.MaxX && V.Z + R > C.MinZ && V.Z - R < C.MaxZ) { bHit = true; break; }
		if (!bHit) return V;
	}
	return {};
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
	return Id == TEXT("saro") || Id == TEXT("station01") ? Id : Id == TEXT("road") ? FString(TEXT("diner")) : FString();
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
	const FVector& Q = T->Position;
	return Q.X > B->MinX - 2 && Q.X < B->MaxX + 2 && Q.Z > B->MinZ - 2 && Q.Z < B->MaxZ + 2;
}

bool US47World::CanGetOut(bool bDead) const
{
	const FString& Id = Leg;
	if (!bDriving || bBusy || Id.IsEmpty() || !Trip || Trip->bFree || (Trouble.WantsKey() && !bDead)) return false;
	const FLegRec* L = Legs.Find(Id);
	const FString Place = PlaceOfLeg(Id);
	if (!L || Place.IsEmpty() || FMath::Abs(L->Drive->Speed) > 0.6) return false;
	// (by the diner and anywhere in Caliche's street, as since 16 October)
	if (Id == TEXT("road") && AtDiner(L->Drive->Pos)) return true;
	// what is walked where a stop in this leg leaves the player (World.walkOf): SARO's ground,
	// STATION 01's, or on the highway the diner's area with the road's own stops
	return DoorSpot(*L->Truck, Id).IsSet();
}

bool US47World::GetOutHere(bool bDead)
{
	if (!CanGetOut(bDead)) return false;
	const FVector Q = Legs[Leg].Drive->Pos;
	const FString Place = Leg == TEXT("saro") && InBox(ParkBox(TEXT("motel")), Q) ? FString(TEXT("motel")) : PlaceOfLeg(Leg);
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
	V = FMath::Min(V, FMath::Max(0.0, (End - 2.2) * 0.6)) * FMath::Clamp(1.15 - FMath::Abs(Err) * 1.4, 0.3, 1.0);
	if (End < 2.2) V = 0;
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
	auto Put = [](FS47Truck* T, double X, double Z, double H, double Y)
	{
		if (!T) return;
		T->SetPose(FVector(X, Y, Z), FVector(T->Rotation.X, H, T->Rotation.Z));
		T->SetDriving(false);
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
	}
	for (const TCHAR* P : TRUCKS) ParkAt(P);
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
		FS47DriveIn In; In.Steer = Move.X; In.Throttle = -Move.Y;
		if (C && C->Input) In = C->Input(Dt);
		else if (Autopilot) In = AutoInput(Dt);
		else if (TestInput) In = TestInput();
		L->Drive->Update(Dt, In, LookIn);
		TripStep(Dt);
	}
	FLegRec* N = (!Leg.IsEmpty() ? Legs.Find(Leg) : nullptr);
	if (!N) N = L;
	FS47DriveController& D = *N->Drive;
	// trouble with the truck, on the story's trips and the player's own (not chapter six's road)
	if (bDriving && Trip && Id != TEXT("roswell")) Trouble.Step(Dt, D, *N->Truck, Engine.Get(), TroubleOk ? TroubleOk() : false);
	if (Engine) { Engine->Set(D.Rpm, D.Load); Engine->Tyres(D.Surface.Kind, FMath::Abs(D.Speed)); }
	// the odometer turns with the miles driven (not on a free drive)
	if (bDriving && !(Trip && Trip->bFree) && Free.IsEmpty()) OdoM += FMath::Abs(D.Speed) * Dt;
	DashT -= Dt;
	if (DashT <= 0)
	{
		DashT = 0.1;
		N->Truck->SetDash(D.Mph, D.Rpm, M->Story ? M->Story->ClockText(false) : FString(), Id == TEXT("roswell") ? 0.55 : 0.62);
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
	const bool bView = bDriving || bCamera;
	if (bView) { LastView = Out; bHaveView = true; }
	// main.ts: in the cab the prompt is the key, the radio or the way out
	if (bDriving && M->UI && !(M->Story && M->Story->IsCinematic()))
	{
		const bool bKey = Trouble.WantsKey(), bRadio = !bKey && OnRadio && RadioAimed(), bOut = !bKey && !bRadio && CanGetOut();
		M->UI->Prompt(bKey ? TEXT("Turn the key") : bRadio ? (!M->CarRadio || !M->CarRadio->bOn ? TEXT("Turn the radio on") : M->CarRadio->Station == TEXT("far") ? TEXT("Turn the radio off") : FString::Printf(TEXT("AM %d: next station"), S47Radio::Freq(M->CarRadio->Station))) : bOut ? TEXT("Get out") : TEXT(""));
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
