#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"
#include "S47BoardRules.h"
#include "S47Docs.h"
#include "S47FieldBookView.h"
#include "S47Seen.h"

class US47UI;
class US47Audio;

// web/src/story/fieldBook.ts. T. Vega's third field book, in the drawer under the bench in the hut at
// STATION 01, and the envelope for plate 4 on the counter of the motel office (FORSLAG.md, 20
// October, item 3 and small tweak 3). Tomás and Nora were the survey crew in 1947 (HISTORIE.md: he
// stood on C with the last reading when she cut the cable, and was not there afterwards). This is his
// book from June and July that year: the survey, and then, in July, the things that happened to them
// on the roads round the station. They are the things that happen tonight (HENDELSER.md, events 2, 7,
// 8, 12 and 14), in the same words, and where the player has already seen one, at the same minute:
// 1947 took in this night (HISTORIE.md, "Avtrykket går begge veier over 39 år"). The book is read once
// and stays as it was read (R4). What it has that the player has not seen yet comes on the way back
// (the herd turned for the truck, the knocks on the cab's roof, the lights along B northbound too),
// each in its own director, one thing at a time as always.
// In June, one night out along C, he lost an hour and came back with one side of his face burned
// (FORSLAG.md, 20 October, item 9 C). The last page: "Three on plate 4. There were two of us." The
// envelope from the photo shop in Roswell came back for him after he was gone; nobody opened it. Held
// up to the lamp: the transit, and three small figures by it, the third not as sharp.
// On the case board each thing in the book has its card from tonight, tied to the key the event
// already keeps (ev:, ne:, mark:); a thread between the two holds: "He wrote it in July 1947. I saw it
// tonight." Kept with chapter three's part of the case (Ch3State.fb) and, for the envelope, with the
// town's ('env:lamp').

/** What the book keeps once it has been read: when, the minutes it has (the ones the player had seen
 *  by then), which of the rest have been seen since, and the threads on the board (FbSave). */
struct SIGNAL47_API FS47FbSave
{
	double At = 0;
	TArray<TPair<FString, FString>> Times; // Partial<Record<FbEvent, string>>, in the order they were had
	TArray<FString> Had, Told, Links;
	TOptional<int32> Page;

	FString Time(const FString& E) const { const TPair<FString, FString>* P = Times.FindByPredicate([&E](const TPair<FString, FString>& X) { return X.Key == E; }); return P ? P->Value : FString(); }
	TSharedPtr<FJsonValue> ToJson() const;
	static TOptional<FS47FbSave> FromJson(const TSharedPtr<FJsonValue>& V);
};

struct FS47FieldBookDeps
{
	TWeakObjectPtr<US47UI> UI;
	TWeakObjectPtr<US47Audio> Audio;
	/** The town's part of the case (the night's events keep their keys there). */
	FS47TownSeen* Seen = nullptr;
	TFunction<TArray<FString>()> Notes;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, float)> Toast;
	TFunction<double()> Clock;
	/** Game time: fn after s seconds of it (paused with the game). */
	TFunction<void(double, TFunction<void()>)> After;
	/** Chapter three's state.fb (Ch3State.fb: the FbSave as JSON; null until read). */
	struct FState { TFunction<TSharedPtr<FJsonValue>()> Get; TFunction<void(const TSharedPtr<FJsonValue>&)> Set; };
	FState State;
	TFunction<void(const FS47Doc&)> File;
	TFunction<void()> Save;
	TFunction<void()> Refresh;
	/** The drawer in the hut, out or in (Station01.setDrawer), if the station is built. */
	TFunction<void(bool bOpen, bool bNow)> Drawer;
	/** The book has been read: what it has not seen yet may come on the way back (main.ts). */
	TFunction<void()> Foretell;
};

class SIGNAL47_API FS47FieldBook
{
public:
	/** Seconds into a drive after the book before the knocks may come (the track out of the station,
	 *  between the gate and the highway, on the way back). */
	static constexpr double KNOCK_AFTER = 20;
	/** Seconds with nothing else of the night's going on before them. */
	static constexpr double KNOCK_QUIET = 30;
	static const TCHAR* FB_TITLE;

	explicit FS47FieldBook(FS47FieldBookDeps InD);

	/** P06 to P09 have the station's records; this has the book and what it shares with tonight. */
	TUniquePtr<FS47RuleBook> Book;

	bool Read() const { return S() != nullptr; }
	/** The book's part of the board, once it has been read. */
	TArray<FS47RuleBook*> Books() { return Read() ? TArray<FS47RuleBook*>{ Book.Get() } : TArray<FS47RuleBook*>(); }

	/** The pages, with the minutes the player had seen by the time the book was read. */
	static TArray<S47FieldBookView::FBookPage> FbPages(const TArray<TPair<FString, FString>>& Times);

	// ---------- the drawer and the book ----------
	FString DrawerLabel() const;
	FString DrawerExamine() const;
	void UseDrawer();
	/** The book, open at a page (-1: where it was left). It is read once the last page has been turned
	 *  to and it is put back; until then it keeps the minutes it had when it was first opened. */
	void OpenBook(int32 Start = -1);
	/** The book as a paper in the notebook (opened in its own view, main.ts). */
	FS47Doc Doc() const;

	// ---------- the envelope in the motel office ----------
	FString EnvelopeLabel() const;
	S47FieldBookView::FEnvelopeView UseEnvelope();
	FS47Doc EnvelopeDoc();

	/** Each frame: what the book has that comes tonight after it was read is written down (once), and
	 *  while driving, the knocks on the roof it has, if the night has not had them yet. */
	void Update(double Dt, bool bOk, bool bDriving, TFunctionRef<bool()> Knock);
	/** After a load: the papers back in the notebook, the drawer as it was left. */
	void Restore();
	/** The drawer as the case has it, once the station has been built (main.ts). */
	void DrawerNow();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47FieldBookDeps D;
	TMap<FString, double> Pending;
	double DriveT = 0;
	double Calm = 0;
	bool bKnocked = false;
	bool bOpening = false;
	FString FrontArt, LitArt;
	TOptional<TArray<TPair<FString, FString>>> Draft;
	TArray<FString> NoLinks;
	mutable TSharedPtr<FJsonValue> CachedJson;
	mutable TOptional<FS47FbSave> Cached;

	FS47FbSave* S() const;
	void Write();
	bool Has(const FString& E) const;
	FString NoteOf(const FString& E) const;
	FString TimeOf(const FString& E) const;
	TArray<TPair<FString, FString>> TimesNow() const;
	void FirstRead(const TArray<TPair<FString, FString>>& Times, int32 Last);
	FString EnvLit() const;
	void Held();
	TArray<FS47BoardCard> Cards() const;
};
