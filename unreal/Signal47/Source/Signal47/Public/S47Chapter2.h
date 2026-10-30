#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Chapter.h"
#include "S47Docs.h"
#include "S47Json.h"
#include "S47Chapter2.generated.h"

class US47Annex;

// story/Chapter2.ts Ch2State: the saved state of chapter two, written and read with the
// TypeScript's own field names ({ v: 1, stage, answered, door, entered, read: { original,
// amended, lineage, index }, p04, p05, finding, destination, surveyId, marker, wrong04, wrong05,
// called, links, arrayAsk? }).
struct SIGNAL47_API FS47Ch2State
{
	int32 SequenceVersion = 0;
	bool bBinderCompared = false;
	TArray<FS47Doc> Sources;
	FString Stage = TEXT("ward-call");   // Stage2: 'ward-call' | 'records' | 'call-nora' | 'complete'
	bool bAnswered = false, bDoor = false, bEntered = false;
	struct FRead { bool bOriginal = false, bAmended = false, bLineage = false, bIndex = false; };
	FRead Read;                           // Record<P.SourceKey, boolean>
	bool bP04 = false, bP05 = false;
	FString Finding, Destination, SurveyId, Marker;
	int32 Wrong04 = 0, Wrong05 = 0;
	bool bCalled = false;
	TArray<FString> Links;                // this chapter's threads on the case board, 'a|b' sorted, in the order made
	TOptional<bool> ArrayAsk;             // Ward called back and asked for the array on the schedule (older saves do not have it)
	S47J::FObj Extra;                     // a saved state as it came, so fields this version does not know stay in it

	S47J::FObj Save() const;
	// { ...fresh(), ...saved, read: { ...fresh().read, ...saved.read } }
	static FS47Ch2State Load(const S47J::FObj& Saved);
};

// story/Chapter2.ts, chapter two, "The Amended Record" (K2 in the design bible, "Den strøkne
// protokollen"). Ward reads the B-12 report and holds the morning series (her line V2 from the
// design bible). The records room behind the south corridor holds three collections: the 1947
// field records (E07 and the archive sleeve E08), the service copies (E06) and the maintenance
// lineage card for B-12. P04 and P05 follow the Unity dossier (WorldCase22) word for word: what
// changed in 1947, and which field site the reference leads to. The field access sheet in the
// matching sleeve names the key holder, N. Vega at Sierra Motor Court. Her two lines on the phone
// are new text, written for this port.
//
// The host side (Prologue.ts beginChapter2 and jumpChapter2, main.ts ch1.onEnd and ch2.onEnd) is
// here too, as BeginChapter2, JumpChapter2, AfterChapterOne and the default OnEnd; Register puts
// "chapter2" into the host's jumps.
UCLASS()
class SIGNAL47_API US47Chapter2 : public US47Chapter
{
	GENERATED_BODY()

public:
	// the constructor's work (the interactables, the rule book, the archive wing) and the host's
	// side: Host->Jumps["chapter2"], OnEnd. S47Chapters.cpp: US47Chapter2::Register(Make<US47Chapter2>(Host, M))
	static US47Chapter2* Register(US47Chapter2* C);

	// ---------- lifecycle ----------
	virtual void Begin(const S47J::FObj& Saved) override;
	virtual void Reset() override;
	virtual S47J::FObj SaveState() const override; // this.s once begun (Prologue.composeCase: ch2), else null
	virtual void Update(double Dt, double T) override;
	virtual FString SaveBlock() const override;
	bool ReadyForDeparture() const;
	bool CompleteOfficeRoute();
	virtual FString Objective() const override;
	virtual TArray<FS47Task> Tasks() const override;
	virtual void DoorChanged(const FString& Id, bool bOpen) override;
	virtual FString Key() const override { return TEXT("ch2"); }
	virtual void BeginDestroy() override;

	bool bStarted = false;          // part of the saved case once begun
	bool bRinging = false;
	FS47Ch2State S;
	TFunction<void()> OnEnd;        // main.ts ch2.onEnd (Register sets the default)
	// Chapter2Deps.save: the host's saveCase(composeCase(...)). Set by the saves' port; unset, a
	// change of state is simply not written.
	TFunction<void()> OnSave;
	// The archive's phone (story/archivePhone.ts, set by main.ts): while it rings, the phones answer
	// it; while it is still to come, lifting the wall phone finds its line open. Use() false leaves
	// the phone the chapter's own. Label empty: null.
	struct FPhoneHook { TFunction<FString()> Label; TFunction<bool(const FString& Where)> Use; };
	FPhoneHook PhoneHook;
	// Nothing else of the night's going (main.ts, oneAtATime): Ward's second call waits for it.
	TFunction<bool()> Quiet;
	// The array has been put back on the schedule at least once tonight (story/arrayBack.ts).
	TFunction<bool()> ArrayTried;
	// The ringing is Ward calling back about the array.
	bool bRingBack = false;
	// P04 and P05 on the case board: the records' lines, the questions and the rules.
	TUniquePtr<FS47RuleBook> Book;
	UPROPERTY() TObjectPtr<US47Annex> Annex;

	// ---------- Ward ----------
	// The card before chapter two holds the ring back (main.ts, chapterBreak; FORSLAG.md, 20
	// October, small change 6): the phone is ringing, but nothing is heard of it until the card's
	// title is up and RingOut() lets it go. Then the card ends on the ring, and the ring is this
	// phone. (The web version returns the loop's handle; here whether it rang.)
	bool bHoldRing = false;
	bool RingOut();
	bool RingAgain();   // the ring again from its start (the card cuts away on it)
	void RingShown();   // the card has cut away on the ring: what the player is told of it, a moment after
	FString PhoneLabel() const;
	void UsePhone();
	// The case board, laid out on the archive table once a record is on it.
	void OpenBoard();

	// ---------- the host (Prologue.ts, main.ts) ----------
	void BeginChapter2(const S47J::FObj& SavedCase);   // Prologue.beginChapter2
	void JumpChapter2();                               // Prologue.jumpChapter2
	void AfterChapterOne(const FString& Method);       // main.ts ch1.onEnd: the card, then chapter two

	// The rules, the replies, the saved state and the drawings, without the world.
	static bool SelfTest(TArray<FString>& Lines);

private:
	double BackT = -1;   // seconds until Ward calls back (-1: not to come)
	double BackRing = 0; // and how long it has rung
	struct FImages { FString Original, Amended, Index; TMap<FString, FString> Icons; };
	FImages Images;

	void BuildBook();
	void Interactables();
	void ApplyWorld();
	void KeepSources();
	bool HasBinderSeed() const;
	void File(const FS47Doc& Doc);
	void Save();
	FString Time() const;
	void Toast(const FString& Text, float Secs = 3.2f) const;
	void Note(const FString& Text) const;
	void Click(const TOptional<FVector>& At = {}) const;
	FVector PhonePos() const;

	void Ring();
	bool RingOn();
	void RingSaid();
	void CallBack(bool bHere);
	void AnswerBack();
	bool AnyRead() const;
	void TakeFieldBox();
	void TakeBinder();
	void TakeLineage();
	void OpenTable();
	TArray<FS47BoardCard> BoardCards();
	FS47Result Compare(const FString& Finding);
	FS47Result Route(const FString& Destination, const FString& SurveyId, const FString& Marker);
	void UseRecPhone();

	FS47Doc DocOriginal();
	FS47Doc DocAmended() ;
	FS47Doc DocLineage() const;
	FS47Doc DocIndex();
	FS47Doc DocFinding04() const;
	FS47Doc DocFinding05() const;
	FS47Doc DocAccess() const;
	FS47Doc DocMap() const;
};
