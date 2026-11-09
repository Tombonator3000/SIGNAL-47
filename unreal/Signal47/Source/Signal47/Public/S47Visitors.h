#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class AS47Web;
class SWidget;
class UStaticMeshComponent;
class UPrimitiveComponent;
class UTextRenderComponent;
struct FS47MapView;

// M4, Freeform only: people from outside Caliche. Stand-in shapes where there are no models yet (Tom's Tripo models are to come).
//
//   NEIGHBOR-20261108  Eloy Baca, an old rancher alone under the east rim (S47TownSites' "baca" box). He has watched the sky for forty
//                      years and tells one thing a visit: a night's event he saw from his porch (often the wrong place or the wrong
//                      night: he is sure of it all the same), an old story (never 1947), now and then a place to go and look, real or
//                      not. What he says is kept as an unconfirmed field record. A night's event at his place can take him too.
//   ENTHUSIASTS-20261108  the Socorro Sky Watch: three amateurs in a painted van who turn up at the Mesa's lot every few days and want
//                      into SARO. Let them in (a donation, a newsletter with a rumor in it, and the quiet one in the back sees your
//                      records), send them away (they go out on their own, and are heard from), or ask about the quiet one.
//   MILITARY-20261108  after a big night's event at a place (a visit, a landing), now and then: a helicopter low over it at first
//                      light and two black sedans parked at it through the morning. If they got there first, the ground has been
//                      gone over: kept as a field record, and Cleared() says so to whatever left marks there.
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

	// ---- the military ----
	// A big night: these kinds of event at a place bring them, now and then.
	static bool Big(const FString& Kind);
	static bool Comes(int32 Seed, int32 Night, const FString& Place, bool bKnown);
	// True: they came to this place after that night's event and went over the ground before the player looked.
	bool Cleared(const FString& Place, int32 Night) const;
	// F10 and the tests: their morning at a place now (helicopter, then the cars); a van day today; Baca gone.
	bool DevMilitary(const FString& Place);
	void DevVan();
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
	FString VanChoice;           // "in", "away", "tie" for the day they were last here
	FString OutPlace; int32 OutNight = 0;
	TSet<FString> Flags;
	// the military
	struct FVisit { FString Place; int32 Night = 0; double Start = 0; bool bLooked = false, bGone = false; };
	TArray<FVisit> Visits;
	int32 CheckedNight = 0;
	double StepIn = 0, ThumpIn = 0;
	// stand-ins in the world
	TArray<TWeakObjectPtr<UStaticMeshComponent>> VanParts, HeliParts, CarParts;
	TArray<TWeakObjectPtr<UTextRenderComponent>> VanText;
	double HeliAge = -1;
	bool bCarsLogged = false;
	TWeakObjectPtr<UStaticMeshComponent> Porch;
	FString CarsArea;
	bool Build();
	void ShowVan(bool bShow);
	void StepMilitary(double Dt);
	void PlaceCars(const FVisit& V, bool bShow);
	void PlaceHeli(const FVisit& V, double Age);
	void Note(const FString& Flag);
	FString PlaceTitle(const FString& Id) const;
	double Hour() const;
};
