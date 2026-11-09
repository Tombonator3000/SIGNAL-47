#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class AS47Web;
class SWidget;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UTextRenderComponent;
class UPointLightComponent;
struct FS47MapView;
struct FS47EventPlace;

// M4, Freeform only: people from outside Caliche. Stand-in shapes where there are no models yet (Tom's Tripo models are to come).
//
//   NEIGHBOR-20261108  Eloy Baca, an old rancher alone under the east rim (S47TownSites' "baca" box). He has watched the sky for forty
//                      years and tells one thing a visit: a night's lights he saw from his porch (often the wrong place or the wrong
//                      night: he is sure of it all the same), an old story (never 1947), now and then a place to go and look, real or
//                      not. What he says is kept as an unconfirmed field record. A night's event at his place can take him too: then
//                      his door stands open. After ten at night he does not come to the door.
//   ENTHUSIASTS-20261108  the Socorro Sky Watch: three amateurs in a painted van who turn up at the Mesa's lot every few days and want
//                      into SARO. Let them in (a donation, a newsletter with a rumor in it, and that evening the van at SARO's gate and
//                      the quiet one in the back reading your logbooks), send them away (they go out on their own, and are heard from;
//                      or their van is found standing empty out there, the door open), or ask about the quiet one.
//   MILITARY-20261108  after a big night's event at a place (a visit, a landing), now and then: a helicopter low over it at first
//                      light and two black sedans parked at its way in through the morning (they drive off when the player comes up).
//                      If they got there first, the ground has been gone over: raked smooth in a wide circle with tire tracks across
//                      it, kept as a field record for two days, and Cleared() says so to whatever left marks there (S47FarmSite's
//                      marks, S47Happenings' landing ring), which then are gone.
//
// Every morning-after rule settles in arrears (critics, round 3): each night over since the last one settled, from four in the morning
// after it, so that a rest or a motel room straight to 08:00 does not sleep through what the night set going.
class FS47Visitors
{
public:
	explicit FS47Visitors(US47Main* Main) : M(Main) {}
	void Begin(const S47J::FObj& Saved);
	void Stop();
	void Step(double Dt);
	S47J::FObj Save() const;
	// "baca_gone", "van_out", "tie_seen", "military" and the like, for FS47RadioTips' "visit:" lines.
	bool Has(const FString& Key) const;
	void FillMap(FS47MapView& V) const;

	// ---- the neighbor ----
	void OpenBaca();
	bool BacaGone() const { return BacaBack > 0; }
	// One thing he says (and whether it is true, for the tests): from what has gone on, by the game's seed and the day.
	struct FTale { FString Key, Text, Place; bool bTrue = true; };
	FTale Tale(int32 Day) const;
	bool VanishBaca(); // the night's events took him (an API, as S47Townsfolk::Vanish)

	// ---- the enthusiasts ----
	static bool VanDay(int32 Seed, int32 Day); // every few days from day 4
	bool VanHere() const;
	void OpenVan();
	// Where the van stands now, if anywhere: the Mesa's lot on its days (10 to 20), SARO's gate the evening they were let in (21 to
	// midnight), or out where they went, empty with its door open, for three days after their night. Area: "road" or "saro";
	// Road: road-local metres; bOpen: standing empty.
	bool VanWhere(FString& OutArea, FVector2D& OutRoad, bool& bOpen) const;

	// ---- the military ----
	// A big night: these kinds of event at a place bring them, now and then.
	static bool Big(const FString& Kind);
	static bool Comes(int32 Seed, int32 Night, const FString& Place, bool bKnown);
	// True: they came to this place after that night's event and went over the ground before the player looked (from the hour they
	// came). Whatever left marks there for that night shows none.
	bool Cleared(const FString& Place, int32 Night) const;
	// Where the marks at a cleared place were, road-local (S47FarmSite, S47Happenings): the raked circle is drawn there, not at the
	// place's own point. Once per visit.
	void RakeAt(const FString& Place, int32 Night, double RoadX, double RoadZ);
	// Where the two cars stand for a place, road-local (F10's camera): at its way in, off every building and off the highway.
	bool CarsSpot(const FString& Place, FVector2D& A, FVector2D& B) const;
	// The rule for that, without a world: Solid are road-local boxes to keep clear of (a metre round each car), the highway too.
	static bool CarsAt(const FS47EventPlace& P, const TArray<FBox2D>& Solid, FVector2D& A, FVector2D& B);
	// Room for Count vehicles side by side (each HalfW across x by HalfL along z, Gap apart in x) as near From as can be, and no
	// nearer Keep than Clear: off every Solid box (with a metre round) and off the highway. False: nowhere within forty metres.
	static bool FreeSpot(const FVector2D& From, const FVector2D& Keep, double Clear, int32 Count, double HalfW, double HalfL, double Gap, const TArray<FBox2D>& Solid, TArray<FVector2D>& Out);
	static bool OnHighway(double RoadX, double RoadZ);
	// F10 and the tests: their morning at a place now (helicopter, then the cars); a van day today; Baca gone; the van at the gate
	// tonight; the van empty at a place; the ground at a place as they left it hours ago.
	// bHold: they stay put for the pictures (about the first minute) however near the player stands.
	bool DevMilitary(const FString& Place, bool bHold = false);
	bool DevRaked(const FString& Place);
	void DevVan();
	void DevVanGate();
	bool DevVanEmpty(const FString& Place);
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TSharedPtr<uint8> Alive = MakeShared<uint8>(0);
	bool bLive = false, bBuilt = false;
	TWeakObjectPtr<AS47Web> Scene;
	// the neighbor
	TSet<FString> BacaTold;
	int32 BacaDay = 0, BacaBack = 0, BacaCheckedNight = 0;
	FString BacaLast;
	// the enthusiasts
	int32 VanChoiceDay = 0, ForcedVanDay = 0;
	FString VanChoice;           // "in", "away", "tie" for the day they were last here; "in_done" once seen off from the gate
	FString OutPlace; int32 OutNight = 0;
	TSet<FString> Flags;
	// the military. Leave: seconds since they began to drive off (not saved: a Continue finds them gone); the cars' spot, worked out once.
	struct FVisit
	{
		FString Place; int32 Night = 0; double Start = 0; bool bLooked = false, bGone = false;
		double RakeX = 0, RakeZ = 0; bool bRake = false;
		double Leave = -1; bool bSpot = false, bHold = false; FVector2D CarA = FVector2D::ZeroVector, CarB = FVector2D::ZeroVector;
	};
	TArray<FVisit> Visits;
	int32 CheckedNight = 0;
	double StepIn = 0, ThumpIn = 0;
	// The military's morning after a night, by the rule: false when they do not come. Gone already when the morning is past.
	static bool MilitaryAfter(int32 Seed, int32 Night, const FString& Place, bool bKnown, double Clock, FVisit& Out);
	// Night N, every morning rule (Baca, the Sky Watch's night out, the military).
	void Settle(int32 Night);
	// stand-ins in the world, one set for each area a place can be in (0 the road's land, 1 SARO's)
	struct FVanSet
	{
		TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts; TArray<FVector> Off; // UE axes, metres from its foot
		TArray<TWeakObjectPtr<UTextRenderComponent>> Words; TArray<FVector> WordsOff;
		TArray<TWeakObjectPtr<UStaticMeshComponent>> Open; TArray<FVector> OpenOff; // the door swung out, the dark doorway, a flashlight
		TWeakObjectPtr<UPointLightComponent> Dome;
	};
	FVanSet Vans[2];
	int32 VanArrivedDay = 0;
	// Where the van was last worked out to stand (a free spot at the gate or out on the land): by what it was worked out for.
	mutable FString VanSpotKey; mutable FVector2D VanSpotRoad = FVector2D::ZeroVector;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> HeliParts, CarParts, RakeParts;
	double HeliAge = -1;
	bool bCarsLogged = false, bCarsIdling = false;
	TWeakObjectPtr<UStaticMeshComponent> Porch;
	TWeakObjectPtr<UPointLightComponent> BacaLamp;
	FVector PorchHinge = FVector::ZeroVector; // UE, the door's west edge
	FString CarsArea;
	// What stops the walker and the truck at a parked car or the van (the walker's list and the drive's), by id; moved, never removed.
	struct FBlock { FString Area; double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0; bool bAdded = false, bApplied = false, bOn = false; FVector2D At = FVector2D::ZeroVector; int32 Since = 0; };
	TMap<FString, FBlock> Blocks;
	void SetBlock(const FString& Id, const FString& Area, bool bOn, const FVector2D& Road, double HalfX, double HalfZ);
	TArray<FBox2D> SolidNear(const FString& Area, const FVector2D& Road, double Within) const;
	bool Build();
	void ShowVan();
	void StepMilitary(double Dt);
	void PlaceCars(FVisit& V, bool bShow, double Dt);
	void PlaceHeli(const FVisit& V, double Age, double Dt);
	void PlaceRaked();
	void StepBaca();
	void OpenVanGate();
	void OpenVanEmpty();
	void Note(const FString& Flag);
	FString PlaceTitle(const FString& Id) const;
	double Hour() const;
};
