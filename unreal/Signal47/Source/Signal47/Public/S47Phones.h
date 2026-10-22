#pragma once

#include "CoreMinimal.h"
#include "S47ArchivePhone.h"
#include "S47Docs.h"
#include "S47OfficePhone.h"
#include "S47RoofSteps.h"
#include "S47Seen.h"
#include "UObject/Object.h"
#include "S47Phones.generated.h"

class US47Main;
class FS47ArrayTip;
class FS47DishArray;
class FS47FieldBook;
class FS47SameThread;
class UTexture;

// main.ts's wiring of the phones, the archive and the records: what it builds for them in boot() (the
// archive's phone and the dishes it tips, story/archivePhone.ts and archiveDishes.ts; the motel
// office's crank telephone, story/officePhone.ts; the steps on the records room's roof,
// story/roofSteps.ts; T. Vega's field book and the envelope, story/fieldBook.ts; the thread nobody
// tied, story/sameThread.ts; what a closer look says, story/looks.ts) and what it does with them:
// arcWhere(), arcIn(), roofIn(), officeIn(), their steps in step()'s order, their part of seenStep()
// and seenSync(), the closer look (the zoom and ui.examine), their hooks into the chapters, the
// printer, the signal processor's deck and FS47NightHooks, and their part of startGame, leavePlay,
// toTitle, saveBlock and openDoc. Held by US47Main (Phones); US47Main::Step and US47Night::Step call it
// in main.ts's order.
//
// What these need of systems ported elsewhere comes in through the hooks below, set by those ports (or
// the integrator); unset, the thing is not there (no reel 047 deck, no time away, no CB ...).
UCLASS()
class SIGNAL47_API US47Phones : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);

	TSharedPtr<FS47ArrayTip> ArrayTip;
	TSharedPtr<FS47ArchivePhone> Archive;
	TSharedPtr<FS47RoofSteps> RoofSteps;
	TSharedPtr<FS47OfficePhone> OfficePhone;
	TSharedPtr<FS47FieldBook> FieldBook;
	TSharedPtr<FS47SameThread> SameThread;

	// ---------- the hooks into systems ported elsewhere ----------
	/** story/reel047.ts on the signal processor's deck (the archive's `deck`). */
	FS47ArcDeck Reel047;
	/** game.decoder.playing and stop() (story/Decoder.ts: tape B on the processor). */
	TFunction<bool()> DecoderPlaying;
	TFunction<void()> DecoderStop;
	/** Prologue.dark: SARO without power (story/saroDark.ts powerOut). */
	TFunction<bool()> Dark;
	/** cb.busy (core/cb.ts). */
	TFunction<bool()> CbBusy;
	/** abduction.data.count: the times away tonight (story/abduction.ts; story/sameThread.ts reads it). */
	TFunction<int32()> AbductionCount;
	/** Dish.rogue of the dishes that follow in a warning (story/dishesFollow.ts), and Dish.back
	 *  (story/arrayBack.ts), by the dish's index. */
	TFunction<bool(int32)> DishRogue, DishBack;
	/** ext.dishArray (world/Dish.ts DishArray, S47SaroNight's): the dishes' phases and their red lamps'
	 *  blink, for the array tipped by the archive's phone (story/archiveDishes.ts). */
	TFunction<FS47DishArray*()> DishArray;
	/** ch3.s.f3 and ch3.s.f4: FRAME 03's and FRAME 04's capture clocks (story/Chapter3.ts). */
	TFunction<TPair<TOptional<double>, TOptional<double>>()> Frames;
	/** ch3.s.fb: the field book's state in chapter three's (Ch3State.fb). Until chapter three's port
	 *  sets these, the book keeps it here. */
	TFunction<TSharedPtr<FJsonValue>()> FbGet;
	TFunction<void(const TSharedPtr<FJsonValue>&)> FbSet;
	/** world.site.setDrawer(open, now) (world/Station01.ts, the station's port). Unset: the exported
	 *  drawer is put where it stands at once. */
	TFunction<void(bool, bool)> StationDrawer;
	/** fieldBook's foretell: pasture.now() while the herd grazes, lineLights.anyWay (main.ts). */
	TFunction<void()> Foretell;
	/** starHole.examine(holeIn(false)): what a closer look up at the patch with no stars says. */
	TFunction<FString()> StarHoleExamine;

	// ---------- main.ts ----------
	FString ArcWhere() const;
	FS47ArcIn ArcIn() const;
	FS47RoofIn RoofIn(bool bOk) const;
	FS47OfficePhoneIn OfficeIn(bool bModal) const;
	/** The closer look: the lens narrows while right mouse is held, on foot (step(), after game.update). */
	void LookStep(double Dt);
	/** ui.examine at the end of step(): what the thing under the crosshair is, once the zoom is most of the way in. */
	void ExamineStep();
	/** fieldBook, roofSteps and the archive (after the night's events), bNightOk as main.ts has it. */
	void StepNight(double Dt, bool bNightOk);
	/** sameThread and the office's telephone (later in step()), the corner clock, the board's books. */
	void StepLate(double Dt, bool bModal);
	/** seenStep: the roof, the office's telephone, the archive, the array tipped, reel 047. */
	void SeenStep();
	void SeenSync();
	/** startGame, once townSeen is back: roofSteps, officePhone and sameThread reset. */
	void Restore();
	void RestoreThread(); // sameThread.reset comes after the ninth and morning in main.ts
	/** startGame, after game.start: archive.restore(), fieldBook.restore(). */
	void Restored();
	/** leavePlay (archive.halt, arrayTip.reset, roofSteps.reset, officePhone.reset) and toTitle (the first two). */
	void Leave(bool bTitle);
	/** saveBlock: the archive's moment (its ring to the dishes down, or reel 047 on). Empty: may save. */
	FString SaveBlock() const;
	/** game.openDoc: T. Vega's field book opens in its own view. */
	bool OpenDoc(const FS47Doc& Doc);
	/** The case board's books from these (game.extraBooks): fieldBook, officePhone, the archive, sameThread. */
	void SyncBooks();

	/** main.ts ch2.onEnd: the steps on the roof still going (the call to the key holder having come in
	 *  the middle of them), the chapter waits for them to finish, then Ch2End (chapter two's own end,
	 *  the card and chapter three) */
	TFunction<void()> Ch2End;
	void Ch2EndWhenQuiet();

	static bool SelfTest(TArray<FString>& Lines);

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	TSharedPtr<FJsonValue> FbOwn;
	double DrawerZ = 0;
	bool bForceZoom = false;
	FString ExamineLogged;
	bool bClockShown = false;
	TArray<class FS47RuleBook*> Mine;
	// seenStep's was, and the entries it ends
	struct FWas { bool bRoof = false; FString Office = TEXT("idle"), Arc = TEXT("idle"), Tip = TEXT("idle"); bool bR47 = false; } Was;
	FS47SeenRef OfficeEntry, ArcEntry, R47Entry;
	// the printer's paper (main.ts paper()): the solve's own sheet as the export has it, and the blank one
	struct FPaperMap { TWeakObjectPtr<UTexture> Map; FLinearColor U, V; float Use = 1.f; bool bHave = false; };
	TArray<FPaperMap> SolvePaper;
	UPROPERTY() TObjectPtr<UTexture> BlankPaper;
	bool bPaperBlank = false;
	FString LoggedArc, LoggedRoof, LoggedOffice, LoggedTip;

	bool Modal() const;
	bool Cinematic() const;
	bool Driving() const;
	bool InSaro(const FString& Zone) const;
	FString Phase() const;
	bool At(const FString& Chapter) const;
	double Clock() const;
	void RequestAutosave() const;
	void Paper(const FString& Kind, double K);
	void SetDrawerFallback(bool bOpen, bool bNow);
	void TestFlags();
};
