#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class AS47Web;
class SWidget;
class UPrimitiveComponent;
class UStaticMeshComponent;
struct FS47MapView;
struct FS47AlbumShot;

// TOWN-USE-20261108 (Tom, 8 November: "bygningene i byen som må ha noe mere funksjon, kjøpe noe der e.l.: radiostasjon, Feed & Seed,
// tourist tours souvenirsjappa"). Freeform only. Three of Caliche's buildings open for business, each at a counter of its own:
//
//   1420 AM    the lobby's cork board: give Earl a tip for the air (FS47RadioTips reads it on 1420 and a caller rings back about it, as
//              often as not about some other place, which is then heard of); used radio parts and dry cells. The same tip line is on
//              SARO's phone (the request line).
//   Feed & Seed  radiator water, a spare wheel, dry cells, shielding foil, a tow rope and come-along (FS47TruckCare digs out quicker),
//              fence wire (FS47TripCamera's line reaches further), salt and range cubes to set out at a place on the land: in the
//              morning what came to them says whether something was about there in the night (animals as a warning).
//   Souvenirs  a hand-drawn map of saucer sites (some on the land, some nonsense, all pencilled onto the survey sheet), the owner's
//              tour (two hours; he always knows one more place), film, postcards, rubber spacemen; and he buys copies of the
//              player's own prints, more for better ones.
//
// What is bought goes as the gas station's does: paid at the counter, a parcel in the hands, loaded in the truck, unpacked at SARO's
// receiving table into the station's stock (FS47FreeformWorld::TakePostedParcel). What the town sees the player do, it talks about
// (Seen, read by FS47RadioTips' lines). 1986 American English, no brand names.
class FS47TownUse
{
public:
	explicit FS47TownUse(US47Main* Main) : M(Main) {}
	void Begin(const S47J::FObj& Saved);
	void Stop();
	void Step(double Dt);
	S47J::FObj Save() const;

	// The counters. bPhone: 1420's request line from SARO's phone: tips only.
	void OpenRadio(bool bPhone);
	void OpenFeed();
	void OpenSouvenir();
	// When each is open, by the hour of the day (0 to 24).
	static bool OpenAt(const FString& Counter, double Hour);
	static FString HoursOf(const FString& Counter);

	// What the town has seen: "lure", "spaceman", "postcard", "sold_odd", "tour", "map". FS47RadioTips' "shop:" lines need these.
	bool Seen(const FString& What) const { return Flags.Contains(What); }
	void Note(const FString& What);

	// The saucer-site map and the lures set out, pencilled onto the survey sheet (US47Flow::MapView).
	void FillMap(FS47MapView& V) const;
	struct FTourSite { const TCHAR* Label; double X, Z; const TCHAR* Place; }; // Place: FS47Places' id where it is real; null: nonsense
	static const TArray<FTourSite>& SaucerMap();
	bool HasMap() const { return bMap; }

	// Salt or range cubes set out at a place on the land (FS47Places' panel), and what came to them by the morning.
	void LureRows(const FString& Place, TArray<TSharedRef<SWidget>>& Rows);
	bool SetLure(const FString& Place, const FString& Kind);
	// What the lure at a place says now (empty: none, or not a night past yet). The first reading of a shy one keeps a field record.
	FString ReadLure(const FString& Place);
	// The rule: shy (untouched, tracks turn back) when something used a place within ShyReach of it that night.
	static constexpr double ShyReach = 500;
	static bool Shy(const FVector2D& LureAt, int32 SetNight, const TArray<TPair<FVector2D, int32>>& UsedPlaces);

	// What the souvenir man pays for a copy of a print (0: he will not, and why). bRecord: a sighting in the station's field record.
	static int32 PrintValue(const FS47AlbumShot& Shot, bool bRecord, FString& Why);
	static constexpr int32 PrintsADay = 3;

	// F10 and the tests.
	FString DevTour();
	bool DevLureNight(const FString& Place, bool bShy, FString& Said);
	static bool SelfTest(TArray<FString>& Lines);
	// A supply a counter sells, its price there and what it is for.
	struct FWare { const TCHAR* Id; int32 Price; const TCHAR* For; };

private:
	US47Main* M = nullptr;
	// The counters' uses outlive a session (the world keeps them): they hold this, and do nothing while bLive is false.
	TSharedPtr<uint8> Alive = MakeShared<uint8>(0);
	bool bLive = false;
	TWeakObjectPtr<AS47Web> Scene;
	bool bBuilt = false, bMap = false;
	TSet<FString> Flags;
	TSet<FString> SoldPrints;
	int32 TourDay = 0, SoldDay = 0, SoldToday = 0, Postcards = 0;
	struct FLure { FString Place, Kind; int32 Night = 0; bool bRead = false; FString Said; };
	TArray<FLure> Lures;
	// What is in the basket at the counter now: supply id to count. Emptied when paid, or on leaving.
	TMap<FString, int32> Basket;
	FString BasketAt;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> Props;
	// What is out in the world because of what was bought (critics, round 3: not only text): a salt block or a sack at each lure and
	// the tracks round it by morning; the rubber spaceman on the dash; the player's picture in the souvenir shop's window.
	TArray<TWeakObjectPtr<UStaticMeshComponent>> LureParts, Spaceman, PrintParts;
	TWeakObjectPtr<class UTextRenderComponent> PrintWords;
	static constexpr int32 LuresDrawn = 4, TracksPerLure = 6, LureBits = 3; // the salt block, its licked hollow, the sack
	double ThingsIn = 0;
	void StepThings();
	// Where something went on that the player was near enough to know of: "place:night" (FS47EventPlaces' places). Earl's tip list
	// offers only these and the places found (critics, round 3: the list was an oracle for nights the player never saw).
	TSet<FString> Witnessed;
	double WitnessIn = 0;
	void StepWitness();
	bool Build();
	void SetPropsShown(bool bShow);
	// The shared basket: a row a supply with its price and what it is for, + and -, and a pay button that hands over the parcel.
	void BasketRows(const FString& Counter, const TArray<FWare>& Wares, TArray<TSharedRef<SWidget>>& Rows, TFunction<void()> Reopen);
	bool Pay(const FString& Counter, const TArray<FWare>& Wares);
	FVector CounterPoint(const FString& Counter) const;
	double Hour() const;
	int32 Night() const;
	bool InTown() const;
	FString TipRows(TArray<TSharedRef<SWidget>>& Rows, bool bPhone);
	void SellRows(TArray<TSharedRef<SWidget>>& Rows);
	bool SellCopy(const FString& PhotoId);
	bool Tour();
	bool BuyMap();
};
