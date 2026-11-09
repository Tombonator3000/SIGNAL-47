#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class SWidget;

// TOWNSFOLK-20261108 (Tom, 8 November): Caliche's regular people, Freeform only: the attendant at the gas station, the waitress at the
// Mesa Diner, the deputy at the substation and the souvenir man. They remember what the player has done (places found, tips on 1420,
// what was bought in town, help given, a night lost), they talk about it and about what has gone on at places on the catalogue
// (S47EventPlaces), one new thing a day each, and the town notices when one of them is gone.
//
// Gone: the night's events take someone through Vanish() (an API for FS47Happenings and the F10 menu; Happenings itself is not
// changed here), and Step takes one of them of its own accord after a night's event at the place they work, now and then. While
// someone is gone their place is empty (the deputy's car and lit window, and the souvenir man on his stoop: FS47Town's own switches,
// which nothing else moves in Freeform),
// the others talk of nothing else, callers on 1420 ask after them (FS47RadioTips' "town:" lines), and asking about it keeps a field
// record. They come back after a few days and will not say where they were.
class FS47Townsfolk
{
public:
	explicit FS47Townsfolk(US47Main* Main) : M(Main) {}
	void Begin(const S47J::FObj& Saved);
	void Stop();
	// At exit or in the purge (FS47FreeformSession::Stop without bCanRestore): only the plain list of who is away, no figures.
	void ResetAway();
	void Step(double Dt);
	S47J::FObj Save() const;

	// "attendant", "waitress", "deputy", "souvenir_man"; and how the others speak of them ("the attendant at the gas station").
	static const TArray<FString>& People();
	static FString Called(const FString& Who);
	static FString Speaker(const FString& Who); // "THE ATTENDANT"
	// The place on the catalogue each one works at (S47EventPlaces): "gas", "diner", "sheriff", "souvenir".
	static FString WorksAt(const FString& Who);

	// One thing this person says today, as "THE DEPUTY: ...": the newest that has not been said by them, else the last one again.
	// Empty: they have nothing to say (or are gone).
	FString Gossip(const FString& Who);
	// Every line this person could say now, newest first (for the tests and F10).
	TArray<FString> Candidates(const FString& Who) const;

	// ---------- gone ----------
	// The night's events took Who (last seen at a catalogue place, may be empty). False: not one of them, or someone is already gone.
	bool Vanish(const FString& Who, const FString& Place);
	bool Away(const FString& Who) const;
	// Whoever is gone in the running game (one game at a time).
	static bool AwayNow(const FString& Who);
	// What the town has seen, for FS47RadioTips' "town:" lines: "away_deputy" while the deputy is gone, "back_deputy" for three
	// days after he is back.
	bool Has(const FString& Key) const;
	// Their post with nobody at it: the panel instead of a talk. True when Who is gone and the panel was shown.
	bool ShowAway(const FString& Who);
	// The waitress has no talk of her own in Freeform: the diner's counter (FS47FreeformSession::Use, "diner:waitress").
	bool Use(const FString& Id);
	void OpenDiner();
	// For a counter's panel: a line of the attendant's or the souvenir man's, or who is minding it while they are gone.
	FString CounterLine(const FString& Who);
	// The odds and the wait, worked out from the game's seed: whether a night's event at their place takes them, how long they stay away.
	static bool TakenBy(int32 Seed, int32 Night, const FString& Who);
	static int32 DaysAway(int32 Seed, const FString& Who, int32 Day);
	static constexpr int32 RestDays = 7; // at least this many days between one going and the next
	// Who a night's events took (empty: nobody): the first of the four whose place was used that night and whose roll says so.
	// LastNightOf: the night a catalogue place was last used (FS47EventPlaces::LastNight).
	static FString TakenAfter(int32 Seed, int32 Night, TFunctionRef<int32(const FString&)> LastNightOf);
	// What the town makes of a night's event by its kind (FS47EventPlaces::LastKind), for talk: "lights" (seen from far off),
	// "doors" (somebody at a door or a window), "figure" (somebody out walking), "stock" (cattle). Empty: nothing anybody talks of.
	static FString Talk(const FString& Kind);

	// Where someone gone was last seen, pencilled on the survey sheet (US47Flow::MapView).
	void FillMap(struct FS47MapView& V) const;
	// F10
	void DevReturnAll();
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	struct FGone { FString Who, Place; int32 Day = 0, Back = 0; };
	TArray<FGone> Gone;      // gone now
	TArray<FGone> Returned;  // came back (the town still talks of it for a while)
	TSet<FString> Told;      // "who:key": said once
	TMap<FString, int32> SaidDay; TMap<FString, FString> LastSaid;
	int32 LastVanishDay = -100, CheckedNight = 0;
	double StepIn = 0;
	void Publish();
	// The town's own figures as who is gone says: the deputy's car and window, the man on the stoop. bAll: everyone back (Stop).
	void ShowPosts(bool bAll);
	struct FLine { FString Key, Text; int32 Rank = 0; };
	TArray<FLine> Lines(const FString& Who) const;
};
