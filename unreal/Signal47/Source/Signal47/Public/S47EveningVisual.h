#pragma once

#include "CoreMinimal.h"

class US47Main;
class UStaticMeshComponent;
class FS47Group;
struct FS47Truck;
struct FS47Node;

/** EveningShift presentation only. The host owns purchase, arrival and save state. */
class SIGNAL47_API FS47EveningVisual
{
public:
	explicit FS47EveningVisual(US47Main* InMain);
	~FS47EveningVisual();
	void Tick(bool bCommute, bool bDriving, FS47Truck* Carrier, bool bArrived,
		bool bFoodBought, bool bFoodTaken, bool bFoodEaten, bool bWater, bool bBurger);
	/** Position and heading use the game's metres/Y-up/radians. Separates a stop on the service pad. */
	void ParkPersonal(const FVector& Position, double Heading);
	/** Restore from intro_arrived. Visitor gravel is clear of the annex plinth and SARO 07. */
	void SetParked(bool bOn);
	void RestorePersonal(const FVector& Position,double Heading,int32 ParkingVersion);
	static FVector ArrivalPark() { return FVector(-12.8,-.60,14.3); }
	// Arrival is based on the whole asphalt visitor area, not paint or car orientation.
	static constexpr double ArrivalMinX=-18.2, ArrivalMaxX=-7.8, ArrivalMinZ=10.5, ArrivalMaxZ=20.5;
	static bool FitsArrivalArea(const FVector& Position,double Heading);
	bool ServiceWaitingVisible() const;
	void Hide();
	FVector ParkedPosition() const { return PersonalPark; }
	double ParkedHeading() const { return PersonalHeading; }
	/** Presentation diagnostics: the personal car owns its body, interior and opening leaf. */
	bool PersonalVisible() const;
	bool PersonalDoorVisible() const;
	double PersonalDoorAngle() const { return DoorAngle; }
	/** Car doors close after use (Tom, 2 November 2026). Out of the wagon, its driver door stands
	 *  open; it swings shut when the player leaves its swing room, uses "Close the door", or drives. */
	bool PersonalDoorOpen() const { return bDoorOpen; }
	void ClosePersonalDoor();
	/** The door's state step (Tick and SelfTest): FromHinge in metres on the ground, Dt seconds. */
	void StepPersonalDoor(bool bShown, bool bDriving, bool bCarrier, double FromHinge, double Dt);
	static constexpr double PersonalDoorOpenAngle = 1.02, PersonalDoorRoom = 2.6;
	FVector PersonalPosition() const { return PresentedPosition; }
	static FVector PersonalDriverEye() { return FVector(-0.35, 1.08, -0.05); }
	/** Credited model bounds after preparation; pivot is exactly at the footprint centre. */
	static double PersonalHalfWidth() { return 0.9280036428; }
	static double PersonalHalfLength() { return 2.5138963728; }

private:
	US47Main* M = nullptr;
	TWeakObjectPtr<UStaticMeshComponent> Personal, PersonalDoor;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> ServiceParts;
	TArray<FTransform> ServiceLocal;
	TUniquePtr<FS47Group> CounterFood, HeldFood;
	FS47Node* CounterBurger = nullptr;
	FS47Node* CounterSandwich = nullptr;
	FS47Node* CounterCup = nullptr;
	FS47Node* CounterWater = nullptr;
	FS47Node* HeldBurger = nullptr;
	FS47Node* HeldSandwich = nullptr;
	FS47Node* HeldBag = nullptr;
	FS47Node* HeldPlate = nullptr;
	FS47Node* HeldCup = nullptr;
	bool bBuilt = false, bParked = false;
	FVector PersonalPark = ArrivalPark();
	double PersonalHeading = 0;
	double DoorAngle = 0;
	double DoorK = 0, DoorRoom = PersonalDoorRoom; // DoorK: 0 shut, 1 open (eased into DoorAngle)
	bool bDoorOpen = false, bWasDriving = false, bWasCarrier = false, bDoorPickable = false;
	FVector PresentedPosition = FVector::ZeroVector;
	void Build();
};
