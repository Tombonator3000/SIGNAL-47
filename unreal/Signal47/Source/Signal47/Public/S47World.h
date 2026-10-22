#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "UObject/Object.h"
#include "S47World.generated.h"

class US47Main;

// world/World.ts: the areas and the trips between them. The areas are SARO (with the motel and
// the antenna field), the road (with the diner and the town), STATION 01, room 6 and the old road
// to Roswell; each lies far from the others in the one scene (AS47Web holds them all). Entering
// an area shows it, hides the others and gives the player its walk zones.
//
// The trips (driveOut, driveBack, driveToMotel, driveToDiner, roam, driveOldRoad) are driven in
// the web version (drive/). Until drive/ is ported they are a fade and the arrival, with the same
// callbacks, so the chapters that use them play on.
UCLASS()
class SIGNAL47_API US47World : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM) { M = InM; }

	FString Area = TEXT("saro");
	bool bDriving = false;
	FString TruckAt = TEXT("saro");  // where the truck stands: 'saro', 'station01', 'motel', 'diner', 'road' ...
	FString Leg;                     // the leg being driven ('' on foot)
	bool bBusy = false;

	// the hooks main.ts sets
	TFunction<void()> OnArriveStation, OnArriveSaro, OnGetIn, OnEnterRoom6, OnLeaveRoom6;
	TFunction<void(const FString&)> OnParked, OnEnter;
	struct FChoice { FString Label; TFunction<void()> Use; };
	TFunction<FChoice(const FString& Place)> TruckChoice; // what using the truck at a place offers

	void Enter(const FString& NewArea);
	void PlaceAtStation();
	void PlaceAtDiner(const FString& At = TEXT("arrive"));
	void GoDiner(const FString& At = TEXT("arrive"));
	void GoRoom6();
	void LeaveRoom6();

	void DriveOut(bool bOnTheRoad = false);
	void DriveBack();
	void DriveToMotel(TFunction<void()> OnArrive);
	void DriveToDiner(TFunction<void()> OnArrive);
	void Roam(const FString& From);
	void DriveOldRoad(TFunction<void()> Ready);
	TOptional<double> TripSecs() const;   // seconds into a story trip, while driving
	double DrivingSpeed() const { return 0; }
	bool CanGetOut(bool bDead = false) const;
	void GetOutHere(bool bDead = false);
	void StopDriving();

	S47J::FObj TruckSave() const;
	void PlaceTruck(const FString& At, const S47J::FObj& Pose);
	S47J::FObj OdoSave() const;
	void OdoRestore(const S47J::FObj& O);
	double Odometer() const { return OdoM / 1609.344 + OdoX; }
	void AddMiles(double Miles) { OdoX += Miles; }

	void Update(double Dt, double T);
	bool Indoors(const FVector& P) const; // World.indoors: under a roof in the area the player is in

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	double OdoM = 0, OdoX = 0;
	double TripStart = -1;
	// the stand-in for a drive: the black, the area, the place, the arrival
	void Arrive(const FString& ToArea, const FString& TruckPlace, TFunction<void()> PlacePlayer, TFunction<void()> OnArrive);
	void Place(double X, double Z, double Yaw);
	void PlaceAnchor(const FString& Path);
};
