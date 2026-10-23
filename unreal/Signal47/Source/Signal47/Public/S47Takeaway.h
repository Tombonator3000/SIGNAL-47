#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"

class US47Main;
class FS47EveningPack;
class FS47Group;
class UStaticMeshComponent;

/** One paid meal, including coffee. Placement and consumption are independent of dialogue cursors. */
class SIGNAL47_API FS47Takeaway
{
public:
	FS47Takeaway(US47Main* InMain, FS47EveningPack& InPack);
	~FS47Takeaway();
	void Begin(const S47J::FObj& Saved = nullptr);
	void Stop();
	void Step(double Dt);
	bool Use(const FString& Id);
	S47J::FObj Save() const;
	bool Eat(const FString& SpotId = FString());
	bool CanEat() const;
	bool Carried() const;
	FString PlacedSpot() const;
	bool Eaten() const;
	bool MealBusy() const { return MealLeft > 0; }
	bool HeldVisible() const;
	bool PlacedVisible() const;
	/** Empty coffee cup and used wrapper left on the meal surface. */
	bool TraceVisible() const;
	bool HandVisible() const { return HeldVisible(); }
	TArray<FString> SpotIds() const;
	FVector SpotPosition(const FString& Id) const;
	FString SpotArea(const FString& Id) const;
	FString Location() const;
	static bool SelfTest(TArray<FString>& Lines);

private:
	enum class EMode : uint8 { None, Held, Placed, Eaten };
	struct FSpot
	{
		FString Id, Area;
		FVector Position = FVector::ZeroVector;
		FS47Obj Place, Pickup, Eat;
		FS47Obj RoadPlace, RoadPickup, RoadEat;
	};
	US47Main* M = nullptr;
	FS47EveningPack& Pack;
	EMode Mode = EMode::None;
	FString Spot;
	TArray<FSpot> Spots;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Targets;
	TUniquePtr<FS47Group> Held, Placed, Trace;
	double MealLeft = 0;
	bool bEnabled = false, bBuilt = false, bOwnFade = false;
	void Build();
	void SyncPack();
	void UpdateSpots();
	void Present();
	bool Here(const FSpot& S) const;
	bool Free() const;
	bool PlaceAt(const FString& Id);
	bool PickupAt(const FString& Id);
	FSpot* Find(const FString& Id);
	const FSpot* Find(const FString& Id) const;
	void Changed();
};
