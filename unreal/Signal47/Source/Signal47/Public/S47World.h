#pragma once

#include "CoreMinimal.h"
#include "S47Drive.h"
#include "S47EngineSound.h"
#include "S47Json.h"
#include "S47Legs.h"
#include "S47Trouble.h"
#include "S47Truck.h"
#include "S47TripRadio.h"
#include "S47Walker.h"
#include "UObject/Object.h"
#include "S47World.generated.h"

class US47Main;
class FS47RoadArea;
class FS47OldRoad;

/** World.startTrip's options: perMetre (the clock's skip per metre driven), onArrive, onStart, at
 *  (start there, not where the truck stands), legs and places (what the trip may pass through),
 *  free (the developer menu: the trucks go back after it), hint (said once on coming into the area
 *  where the truck is to be left), leg (start in this leg's area). */
struct FS47TripOpts
{
	double PerMetre = 0;
	TFunction<void()> OnArrive, OnStart;
	TOptional<FVector> AtPos;
	double AtHeading = 0;
	TArray<FString> Legs;
	bool bFree = false;
	FString Hint;
	FString Leg;
	TArray<FString> Places;
};

// world/World.ts: the areas and the trips between them. The areas are SARO (with the motel and
// the antenna field), the road (with the diner and the town), STATION 01, room 6 and the old road
// to Roswell; each lies far from the others in the one scene (AS47Web holds them all). Entering
// an area shows it, hides the others and gives the player its walk zones.
//
// The trips: the truck is driven from one place to another the whole way (drive/legs.ts). Each
// area it passes through is a leg with its own copy of the truck and its own controller; where
// two areas meet on a road the truck is handed from one to the next between two frames. The
// drive runs where the camera is placed, in the walker's tick (Frame, through the walker's
// CameraOverride), so the cab and the view move in the same frame.
UCLASS()
class SIGNAL47_API US47World : public UObject
{
	GENERATED_BODY()

public:
	/** The deps (main.ts); also wires the truck into the game: its keys in the cab, the yard's
	 *  'truck' and the station's 's1truck', the walker's camera and the test flags. */
	void Init(US47Main* InM);
	virtual void BeginDestroy() override;

	FString Area = TEXT("saro");
	bool bDriving = false;
	/** Where the truck stands when nobody drives it ('saro', 'station01', 'motel', 'diner'); empty
	 *  while it is out on a trip. */
	FString TruckAt = TEXT("saro");
	/** The area being driven in on a trip, or empty. */
	FString Leg;
	bool bBusy = false;

	// the hooks main.ts sets
	TFunction<void()> OnArriveStation, OnArriveSaro, OnGetIn, OnEnterRoom6, OnLeaveRoom6;
	/** The truck has been left on a place (any trip, the chapters' and the player's own). */
	TFunction<void(const FString&)> OnParked, OnEnter;
	struct FChoice { FString Label; TFunction<void()> Use; };
	/** What the truck where it stands offers (main.ts asks the chapters): the chapter's drive, or
	 *  one of the player's own. Init sets main.ts's when nothing else has. */
	TFunction<FChoice(const FString& Place)> TruckChoice;
	/** Whether random trouble may come now (main.ts: the night, no cinematic). */
	TFunction<bool()> TroubleOk;
	/** The truck's radio (main.ts turnRadio: R, or Use while looking at it in the cab). Set by main.ts when the airwaves are made. */
	TFunction<void()> OnRadio;
	/** The night's own claims on Use in the cab before the trouble's key and getting out (main.ts
	 *  useCurrent: Chapter6.turnKey, truckTaken.useKey, the abduction's hold): true takes the press. */
	TFunction<bool()> OnCabUse;
	/** The night's other holds on the cab (main.ts): no getting out by Use (the truck on B's count,
	 *  the truck's warning on B: story/truckOnB.ts, truckTaken.ts); the way out offered with Q (in that
	 *  warning, and chapter six's door); held at the wheel on waking (the key not offered). */
	TFunction<bool()> CabStay, CabRunOffered, CabHeld;
	/** Chapter six takes over on the old road: the pedals and wheel (Input), or the camera (the
	 *  pull-out at the end, while the truck stands). Cleared when the drive stops. */
	struct FOldControl { TFunction<FS47DriveIn(double)> Input; TFunction<void(double, FS47View&)> Camera; };
	TOptional<FOldControl> OldControl;
	/** Tests steer the truck through this instead of the keys: fixed input, or asked every step. */
	TFunction<FS47DriveIn()> TestInput;
	/** Drive by itself along the way at about this speed in m/s (tests, the developer menu), or unset. */
	TOptional<double> Autopilot;
	/** Trouble with the truck on a trip: misfires, a dead engine, flickering lights (drive/trouble.ts). */
	FS47TruckTrouble Trouble;
	/** The next time the truck is got into, its engine is dead and wants the key (story/abduction.ts). */
	bool bKeyStall = false;

	void Enter(const FString& NewArea);
	void PlaceAtStation();
	void PlaceAtDiner(const FString& At = TEXT("arrive"));
	void GoDiner(const FString& At = TEXT("arrive"));
	void GoRoom6();
	void LeaveRoom6();
	/** How the sky is turned in the current area (the station's terms are the road's turned a quarter round). */
	double SkyYaw() const;
	/** The places the truck any time can reach, as the story stands (main.ts, every frame). */
	void SetStoryPlaces(bool bStationOpen, bool bOldRoadOpen);

	// ---------- the drives ----------
	/** From the truck at SARO to STATION 01, driven the whole way; parked on the pad, the player gets out. */
	void DriveOut(bool bOnTheRoad = false);
	/** From the truck at the station (or wherever it was left) back to SARO, onto the pad. */
	void DriveBack();
	/** From the truck to Sierra Motor Court's lot, 650 m down the highway (chapter four). */
	void DriveToMotel(TFunction<void()> OnArrive);
	/** From the truck on SARO's pad to Mesa Diner (chapter five): the clock goes on faster per metre. */
	void DriveToDiner(TFunction<void()> OnArrive);
	/** Get in where the truck stands and drive anywhere (Tom 15 October: the truck any time). */
	void Roam(const FString& From);
	/** From the diner's lot out onto the old road (chapter six). Ready() runs behind the black. */
	void DriveOldRoad(TFunction<void()> Ready);

	/** Get in and drive from where the truck stands to another place, the whole way (to empty:
	 *  drive anywhere). */
	void StartTrip(const FString& From, const FString& To, const FS47TripOpts& O = FS47TripOpts());

	/** Seconds the truck has been driven on this trip (not on a free drive), or unset. */
	TOptional<double> TripSecs() const;
	TOptional<FS47TripNow> TripNow() const; // World.ts tripNow, for the radio
	bool AtMotel(const FVector& P) const; // Crossing.ts atMotel
	bool RoadStopAt(const FVector& P) const; // World.ts roadStopAt: whether it finds a stop
	/** The speed of the truck being driven (m/s, negative backing up); 0 when nobody drives. */
	double DrivingSpeed() const;
	/** Whether the player can get out now (stopped, on a trip, with ground by a door). */
	bool CanGetOut(bool bDead = false) const;
	/** Out of the truck where it stands; the trip ends as if it had been left on a lot. */
	bool GetOutHere(bool bDead = false);
	/** Stop driving without arriving: a trip given up puts the trucks back where they stood. */
	void StopDriving();

	/** Where a save finds the truck: {at, area, pose: {at, x, y, z, h, p, r}} (story/state.ts truckPose), or null. */
	S47J::FObj TruckSave() const;
	/** The truck stands where the story has left it; with a pose from a save, exactly there. */
	void PlaceTruck(const FString& At, const S47J::FObj& Pose = nullptr);
	S47J::FObj OdoSave() const;
	void OdoRestore(const S47J::FObj& O);
	/** The miles on the odometer since the shift began (the work order's MILES OUT is 84,217). */
	double Odometer() const { return OdoM / 1609.344 + OdoX; }
	/** Miles on the odometer that nobody drove (story/truckTaken.ts). */
	void AddMiles(double Miles);
	/** It read this when the truck last set out from SARO's ground (unset: not yet tonight). */
	TOptional<double> OdoAtSaro;
	/** Whether the truck stands on its place's own lot or pad (within 2 m of where it is left there). */
	bool TruckOnLot() const;

	/** The truck being driven and its controller, or null (on the old road too). */
	FS47Truck* CabTruck() const;
	FS47DriveController* CabDrive() const;
	/** Something holds the truck being driven back, m/s² (0: let go). */
	void HoldTruck(double K) { if (FS47DriveController* D = CabDrive()) D->Held = K; }
	/** The truck being driven, put down somewhere else on its area's ground, standing, the engine off
	 *  and quiet (story/truckTaken.ts: where the player wakes in it). The trip goes on from there; it is
	 *  where the truck stands now, for a save made in it (TruckSave) and for the player's body. */
	bool MoveDriven(double X, double Z, double Heading);
	/** The truck standing on SARO's ground, put down somewhere else on it (no one in it). */
	bool MoveParked(double X, double Z, double Heading, double Y = 0);
	/** Every sound of the truck gone (the engine, the starter, the tyres), or back. */
	void HushTruck(bool bOn) { if (Engine) Engine->Hush(bOn); }
	/** The engine sound, for the chapter that stops it. */
	FS47EngineSound* EngineSound() const { return Engine.Get(); }
	/** The truck the player has, if it is in this area, or null (not on the old road). */
	FS47Truck* TruckHere() const;
	/** The copies of the truck: on SARO's pad, on the road and the diner's lot, at the station, on the old road. */
	FS47Truck* SaroTruck() const { return SaroTruckP.Get(); }
	FS47Truck* OldTruck() const { return OldTruckP.Get(); }
	FS47OldRoad* OldRoad() const;
	FS47DriveController* OldDrive() const;

	// ---------- free drive (the developer menu) ----------
	/** Which road the truck is out on in a free drive ('road', 'oldroad'), or empty. */
	FString Free;
	void FreeDrive(const FString& Which);
	void EndFreeDrive();

	/** The frame while the truck is driven (World.update's drivingStep, with the road's cattle
	 *  guard and the engine's sound): the walker asks it for the view each tick. True: the cab's
	 *  view is in Out. */
	bool Frame(double Dt, const FVector2D& Move, const FVector2D& LookIn, FS47View& Out);
	void Update(double Dt, double T);
	bool Indoors(const FVector& P) const; // World.indoors: under a roof in the area the player is in

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	UPROPERTY() TArray<TObjectPtr<UObject>> Keep;
	double OdoM = 0, OdoX = 0;

	// ---------- the legs ----------
	struct FLegRec
	{
		TUniquePtr<IS47DriveArea> Area;
		TUniquePtr<FS47DriveController> Drive;
		FS47Truck* Truck = nullptr;
	};
	TMap<FString, FLegRec> Legs;
	TUniquePtr<FS47Truck> SaroTruckP, TruckP, ParkedP, OldTruckP;
	FS47RoadArea* Road = nullptr;
	TUniquePtr<FS47EngineSound> Engine;
	bool bLegsMade = false;
	void EnsureLegs();
	FS47Truck* TruckOf(const FString& Id) const;
	FS47Box2 DinerPark;
	/** The walk zones and colliders AS47Web keeps per area, and where the trucks' own colliders are
	 *  in them (World.parkAt moves them). */
	int32 SaroCol = -1, StationCol = -1, DinerCol = -1;
	FS47Obj SaroProxy, StationProxy;

	struct FTrip
	{
		FString From, To;
		double PerMetre = 0;
		TFunction<void()> OnArrive;
		double Still = 0;
		FString Home, Hint;
		bool bFree = false;
		FVector StartPos = FVector::ZeroVector;
		bool bLeft = false;
		double Secs = 0, Metres = 0;
		int32 N = 0;
	};
	TOptional<FTrip> Trip;
	int32 TripNo = 0;
	struct FHome { FS47Truck* T; double X, Y, Z, H, P, R; };
	TArray<FHome> TrucksHome;
	double DashT = 0;
	struct FAuto { bool bRev = false; double Stall = 0, Back = 0, Stuck = 0; } Auto;
	struct FFreeBack { FString Area; double X, Z, Yaw, Pitch; };
	TOptional<FFreeBack> FreeBack;
	FS47View LastView;
	bool bHaveView = false;

	void ParkAt(FString Id);
	void LightsOn(FS47Truck& T);
	void TripStep(double Dt);
	TOptional<FS47Box2> ParkBox(const FString& Place) const;
	void Transfer(const S47Legs::FExit& Ex);
	void Park(const FString& Place);
	void GetOut(FS47Truck& T);
	TOptional<FVector> DoorSpot(const FS47Truck& T, const FString& WalkArea = FString()) const;
	FString PlaceOfLeg(const FString& Id) const;
	bool AtDiner(const FVector& Q) const;
	FS47DriveIn AutoInput(double Dt = 0);
	void StartEngine();
	bool CabUse();
	bool CabRadio();
	bool RadioAimed() const;
	void Place(double X, double Z, double Yaw);
	void PlaceAnchor(const FString& Path);
	void Hold(bool bOn);
	void Fade(bool bOn);
	void Toast(const FString& Text, float Secs);
	bool FirstTime(const FString& Id);
	TSet<FString> Hints;
};
