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
	void Hide();
	FVector ParkedPosition() const { return PersonalPark; }
	double ParkedHeading() const { return PersonalHeading; }

private:
	US47Main* M = nullptr;
	TWeakObjectPtr<UStaticMeshComponent> Personal, Service;
	TUniquePtr<FS47Group> Hood, CounterFood, HeldFood;
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
	FVector PersonalPark = FVector(4.5, -0.62, 14);
	double PersonalHeading = UE_DOUBLE_PI;
	void Build();
};
