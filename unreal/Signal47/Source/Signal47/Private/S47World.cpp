#include "S47World.h"

#include "S47HUD.h"
#include "S47Main.h"
#include "S47Obj.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47World, Log, All);

namespace
{
	// world/World.ts: FOG and FOG_OLDROAD (the old road's air is thinner at dawn)
	constexpr float FOG = 0.0021f, FOG_OLDROAD = 0.00055f;
}

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
	if (OnEnter) OnEnter(NewArea);
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
	M->Story->SetCinematic(true);
	M->UI->Fade(true);
	M->Story->After(0.5, [this, At]
	{
		Enter(TEXT("diner"));
		PlaceAtDiner(At);
		bBusy = false;
		M->Story->SetCinematic(false);
		M->UI->Fade(false);
	});
}

void US47World::GoRoom6()
{
	// World.goRoom6: through the door of room 6 (a short fade)
	if (bBusy || !M || !M->Story) return;
	bBusy = true;
	M->Story->SetCinematic(true);
	M->UI->Fade(true);
	M->Story->After(0.5, [this]
	{
		Enter(TEXT("room6"));
		PlaceAnchor(TEXT("world.room6.anchors.arrive"));
		bBusy = false;
		if (OnEnterRoom6) OnEnterRoom6();
		M->Story->SetCinematic(false);
		M->UI->Fade(false);
	});
}

void US47World::LeaveRoom6()
{
	// World.leaveRoom6: out onto the motel's front, back in SARO's scene
	if (bBusy || !M || !M->Story) return;
	M->Story->SetCinematic(true);
	M->UI->Fade(true);
	M->Story->After(0.6, [this]
	{
		Enter(TEXT("saro"));
		const FString Path = M->Web->Pose(TEXT("world.court.anchors.fromRoom6")).bValid ? TEXT("world.court.anchors.fromRoom6") : TEXT("world.motel.anchors.fromRoom6");
		PlaceAnchor(Path);
		if (OnLeaveRoom6) OnLeaveRoom6();
		M->Story->SetCinematic(false);
		M->UI->Fade(false);
	});
}

// ---------- the trips (a stand-in until drive/ is ported) ----------

void US47World::Arrive(const FString& ToArea, const FString& TruckPlace, TFunction<void()> PlacePlayer, TFunction<void()> OnArrive)
{
	if (bBusy || !M || !M->Story) return;
	bBusy = true;
	bDriving = true;
	TripStart = M->Story->Gt();
	M->Story->SetCinematic(true);
	M->UI->Fade(true, TEXT(""));
	if (OnGetIn) OnGetIn();
	UE_LOG(LogS47World, Display, TEXT("S47: trip to %s (not driven yet: drive/ is not ported)"), *TruckPlace);
	M->Story->After(1.2, [this, ToArea, TruckPlace, PlacePlayer, OnArrive]
	{
		bDriving = false;
		Leg.Reset();
		TruckAt = TruckPlace;
		Enter(ToArea);
		if (PlacePlayer) PlacePlayer();
		bBusy = false;
		M->Story->SetCinematic(false);
		M->UI->Fade(false);
		if (OnParked) OnParked(TruckPlace);
		if (OnArrive) OnArrive();
	});
}

void US47World::DriveOut(bool bOnTheRoad)
{
	(void)bOnTheRoad;
	Arrive(TEXT("station01"), TEXT("station01"), [this] { PlaceAtStation(); }, [this] { if (OnArriveStation) OnArriveStation(); });
}

void US47World::DriveBack()
{
	Arrive(TEXT("saro"), TEXT("saro"), [this]
	{
		// out by the truck on its pad by the service yard (World.park: the door side, 1.75 m)
		const FS47Obj Truck = M->Web->Obj(TEXT("yard.objs.truck"));
		const FVector P = Truck.IsValid() ? Truck.WorldPosition() : FVector(12, 0, -14);
		Place(P.X + 1.75, P.Z, 0);
	}, [this] { if (OnArriveSaro) OnArriveSaro(); });
}

void US47World::DriveToMotel(TFunction<void()> OnArrive)
{
	Arrive(TEXT("saro"), TEXT("motel"), [this]
	{
		const FString Path = M->Web->Pose(TEXT("world.court.anchors.entry")).bValid ? TEXT("world.court.anchors.entry") : TEXT("world.motel.anchors.entry");
		PlaceAnchor(Path);
	}, MoveTemp(OnArrive));
}

void US47World::DriveToDiner(TFunction<void()> OnArrive)
{
	Arrive(TEXT("diner"), TEXT("diner"), [this] { PlaceAtDiner(TEXT("arrive")); }, MoveTemp(OnArrive));
}

void US47World::Roam(const FString& From)
{
	(void)From;
	M->UI->Toast(TEXT("The truck does not drive yet in this version."), 3.f);
}

void US47World::DriveOldRoad(TFunction<void()> Ready)
{
	Enter(TEXT("roswell"));
	if (Ready) Ready();
}

TOptional<double> US47World::TripSecs() const
{
	if (!bDriving || !M || !M->Story || TripStart < 0) return {};
	return M->Story->Gt() - TripStart;
}

bool US47World::CanGetOut(bool bDead) const { (void)bDead; return !bDriving; }
void US47World::GetOutHere(bool bDead) { (void)bDead; }
void US47World::StopDriving() { bDriving = false; Leg.Reset(); }

S47J::FObj US47World::TruckSave() const
{
	S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("at"), TruckAt);
	S47J::Set(O, TEXT("area"), Area);
	return O;
}

void US47World::PlaceTruck(const FString& At, const S47J::FObj& Pose)
{
	(void)Pose;
	TruckAt = At;
}

S47J::FObj US47World::OdoSave() const
{
	S47J::FObj O = S47J::New();
	S47J::Set(O, TEXT("m"), double(FMath::RoundToInt(OdoM)));
	S47J::Set(O, TEXT("x"), FMath::RoundToDouble(OdoX * 1000) / 1000);
	return O;
}

void US47World::OdoRestore(const S47J::FObj& O)
{
	OdoM = S47J::Num(O, TEXT("m"));
	OdoX = S47J::Num(O, TEXT("x"));
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

void US47World::Update(double Dt, double T)
{
	(void)Dt; (void)T;
}
