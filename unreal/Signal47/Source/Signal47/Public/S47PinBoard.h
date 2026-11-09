#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class FS47RuleBook;
struct FS47BoardCard;
struct FS47Doc;

// CASEBOARD-20261108 (Tom, 8 November: "sporbok/kart som fylles selv med notater, bilder og peilinger, og tråd mellom funn på SARO").
// Freeform's part of the case board (B): the field's own material and only that. What the player kept out there (field records,
// cassettes with something on them), the prints developed in the lab and the bearings, each with where and when. A record goes up by
// itself once the player is back at SARO; until then its card says it is still in the notebook. Loose threads the station draws by
// itself tie what shares a place, a line of inquiry or a bearing, from those facts alone. The player can tie threads of their own,
// and the board never says whether one holds. The ordinary papers (mail, the Ledger, telexes, the night logs, Dr. Ward's letters)
// lie in one tray by the board, a card that opens it; one that names a place on the land can be pinned up from there as a
// clipping. Freeform only; saved under "pinBoard".
class FS47PinBoard
{
public:
	explicit FS47PinBoard(US47Main* InMain);
	~FS47PinBoard();
	void Begin(const S47J::FObj& Saved);
	void Stop();
	void Step(double Dt);
	S47J::FObj Save() const;
	// RecordEvidence: where and when a field record was kept (road-local metres, the session's clock). A cassette's is where it ran.
	void Kept(const FString& FindingId);
	// The tray by the board: the ordinary papers, to read, and to pin up one that names a place.
	void OpenTray();
	int32 UpCount() const { return Up.Num(); }
	int32 ThreadCount() const { return Links.Num(); }
	int32 ClipCount() const { return Clips.Num(); }
	FS47RuleBook* Book() const { return BookPtr.Get(); }
	// What the board shows (F10, tests): its cards, and the station's own threads between them.
	TArray<FS47BoardCard> Cards() const;
	TArray<TPair<FString, FString>> StationThreads() const;
	// The papers in the tray: every document that is not field material on the board.
	TArray<FS47Doc> Papers() const;
	bool AtSaro() const;
	// F10: pin a paper as if chosen from the tray; true if it names a place.
	bool DevClip(const FString& DocId);

	// ---------- the pure rules (tested) ----------
	struct FItem
	{
		FString Id; double X = 0, Z = 0, Clock = 0; bool bAt = false;
		int32 Line = -1; // a line of inquiry (FS47FreeformCases), -1 none
		bool bBearing = false; double Az = 0, Spread = 0;
	};
	// The station's loose threads, from facts only: the same place (within 120 m, each to the latest before it there), the same line
	// of inquiry (each to the one before it), and a bearing from SARO over a place where something was kept (at most three, nearest the
	// bearing's middle, more than 200 m out). Pairs of ids, each once.
	static TArray<TPair<FString, FString>> Ties(const TArray<FItem>& Items, double SaroX, double SaroZ);
	// The place on the land a paper names (an FS47Places id) and the sentence that names it; empty when none.
	static FString PlaceNamed(const FString& Text, FString* OutSentence = nullptr);
	// The night a moment belongs to: night N runs from 06:00 on day N to 06:00 the next morning. "D2 23:40".
	static int32 NightOf(double Clock);
	static FString StampOf(double Clock);
	static constexpr int32 MostCards = 16, MostClips = 30, MostLinks = 120, MostKept = 400;
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TUniquePtr<FS47RuleBook> BookPtr;
	bool bListed = false;
	struct FWhere { double X = 0, Z = 0, Clock = 0; };
	TMap<FString, FWhere> Where; // finding id: where and when it was kept
	TSet<FString> Up;            // card ids on the board
	TArray<FString> Clips;       // document ids chosen from the tray, in order
	TArray<FString> Links;       // the player's own threads ('a|b', sorted)
	double Look = 0;
	TSharedPtr<uint8> Life;
	void List(bool bOn);
	void PinWhatCameBack(bool bQuiet);
	bool SaroAt(double& OutX, double& OutZ) const;
	TArray<FItem> Items() const;
};
