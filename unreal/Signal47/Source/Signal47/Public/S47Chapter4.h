#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47CB.h"
#include "S47Chapter.h"
#include "S47Docs.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47Panels.h"
#include "S47Chapter4.generated.h"

class US47Room6;
class US47Motel;
class US47Chapter1;
class US47Chapter2;
class FS47WalkieSound;
class FS47SC10Contract;

// story/Chapter4.ts Ch4State: the saved state of chapter four, written and read with the
// TypeScript's own field names ({ v: 1, stage, exitOpen, arrived, greeted, placed, shown, p10,
// wrong10, topics: { record, tomas, cable }, tone, p11, wrong11, line, p12, wrong12, told, read:
// { card, letter, correction }, seen: { photo, window }, cb?, links?, mark?, down3? }).
struct SIGNAL47_API FS47Ch4State
{
	FString Stage = TEXT("to-motel");   // Stage4: 'to-motel' | 'room' | 'leave' | 'complete'
	bool bExitOpen = false;
	bool bArrived = false, bGreeted = false;
	TArray<FString> Placed;
	bool bShown = false;                // the evidence is on the table; her question about the control follows
	bool bP10 = false; int32 Wrong10 = 0;
	struct FTopics { bool bRecord = false, bTomas = false, bCable = false; };
	FTopics Topics;                     // Record<Topic, boolean>
	FString Tone;                       // 'mild' | 'critical' | '' (null)
	bool bP11 = false; int32 Wrong11 = 0;
	bool bLine = false, bP12 = false; int32 Wrong12 = 0;
	bool bTold = false;
	struct FRead { bool bCard = false, bLetter = false, bCorrection = false; };
	FRead Read;                         // Record<Paper, boolean>
	struct FSeen { bool bPhoto = false, bWindow = false; };
	FSeen Seen;
	TOptional<bool> Cb;                 // Nora has called the truck on the CB (saves from before 16 October have none)
	TOptional<TArray<FString>> Links;   // this chapter's threads on the case board, 'a|b' sorted
	TOptional<bool> Mark;               // she has seen it on the player's face, after a time away (story/abduction.ts): said once
	TOptional<bool> Down3;              // she has turned PHOTO 03 face down (the band on it: story/theThird.ts), without a word: once
	S47J::FObj Extra;                   // a saved state as it came, so fields this version does not know stay in it

	S47J::FObj Save() const;
	// { ...fresh(), ...saved, topics: { ...fresh().topics, ...saved.topics }, read: ..., seen: ... }
	static FS47Ch4State Load(const S47J::FObj& Saved);
};

// story/Chapter4.ts, chapter four, "Room 6" (K4 in the design bible). N. Vega is waiting in room 6
// at Sierra Motor Court, 650 m down the highway south of SARO (Tom, 15 and 16 October: further from
// the stations). The player drives there in the truck, or walks: out of the fire exit at the end of
// the south corridor and down the highway's shoulder (world/Crossing.ts). Then the evidence.
//   P10  Do tonight's prints carry the same mark as 1947? The two B-12 prints and the original
//        record on the table, and which source the player removed for the control.
//   P11  Why did she rewrite the record? Three parts in any order, each with something she puts on
//        the table: the amended record (her signed correction, E12), T. Vega (his letter, E13) and
//        the cable (his last field card, E11). Then the motive.
//   P12  Is the return line a live conversation? The phone in the room is on the old field line.
//        She asks it a new question and gets the 1947 fragment back.
// P10 is answered in the talk, with the evidence on her table. P11 and P12 are worked out on the
// case board from what she puts on the table (SPILLDESIGN.md: answers are given with evidence; the
// three sentences with one right one are gone). The texts follow the bible's story material (A5 to
// A7, V3 to V5). New in the web version: at the end she says where C crossed the highway, which
// leads to the road south.
//
// The host side (Prologue.ts beginChapter4 and jumpChapter4, main.ts's motel deps, ch4.onEnd and
// the world's room 6 hooks) is here too, as BeginChapter4, JumpChapter4 and Register. Chapter
// three is read from its saved state (its JSON, `ch3` in the case), not through its class.
UCLASS()
class SIGNAL47_API US47Chapter4 : public US47Chapter
{
	GENERATED_BODY()

public:
	// the constructor's work (the rule book) and main.ts's for chapter four: the room and the court,
	// the world's room 6 hooks, the truck's choice, the CB, Nora's voice, Host->Jumps["chapter4"],
	// OnEnd. S47Chapters.cpp: Make<US47Chapter4>(Host, M)->Register()
	US47Chapter4* Register();
	virtual void BeginDestroy() override;

	// ---------- lifecycle ----------
	virtual void Begin(const S47J::FObj& Saved) override;
	virtual void Reset() override;
	virtual S47J::FObj SaveState() const override; // this.s once begun (Prologue.composeCase: ch4), else null
	virtual void Update(double Dt, double T) override;
	virtual FString Objective() const override;
	virtual TArray<FS47Task> Tasks() const override;
	virtual FString SaveBlock() const override { return FString(); }
	virtual void DoorChanged(const FString& Id, bool bOpen) override;
	virtual FString Key() const override { return TEXT("ch4"); }

	bool bStarted = false;
	FS47Ch4State S;
	TFunction<void()> OnEnd;          // main.ts ch4.onEnd (Register sets the default)
	TFunction<void()> OnSave;         // Chapter4Deps.save: the host composes the case and main.ts autosaves
	/** The player has been taken tonight and lost the time (main.ts; story/abduction.ts): it shows on
	 *  one side of the face, and she sees it once P10 is on the table. Unset: never (not in this version). */
	TFunction<bool()> Taken;
	/** The crank telephone in the office (story/officePhone.ts, set by main.ts): its label and its use
	 *  while it has something of its own (ringing, off the hook); Use() false leaves it the prop.
	 *  Label empty: null. Unset until officePhone.ts is ported. */
	struct FOfficePhone { TFunction<FString()> Label; TFunction<bool()> Use; };
	TOptional<FOfficePhone> OfficePhone;
	/** Motel.cb: a call on the truck's CB (core/cb.ts); false if it could not be made now (main.ts). */
	TFunction<bool(const TArray<FS47CBLine>&, TFunction<void()>)> Cb;
	/** P10 to P12 on the case board: her papers, the return line, the questions and the rules. */
	TUniquePtr<FS47RuleBook> Book;
	/** The places (World.ts room6 and court; Crossing.ts brick): made here, as chapter one makes the yard. */
	UPROPERTY() TObjectPtr<US47Room6> Room;
	UPROPERTY() TObjectPtr<US47Motel> Court;

	// ---------- the truck ----------
	/** This chapter's drive (main.ts asks it, World.truckChoice): down the highway to the motel, until
	 *  the player has been to room 6, from wherever the truck stands but the motel's lot. */
	struct FTrip { FString Label; TFunction<void()> Use; };
	TOptional<FTrip> TruckTrip(const FString& At) const;

	// ---------- room 6 (World's hooks) ----------
	void BindRoom();
	void EnteredRoom();   // World.onEnterRoom6: the player comes in through the door
	void LeftRoom();      // World.onLeaveRoom6: the player has stepped out of room 6
	void Choose(const FString& Id);

	// ---------- the host (Prologue.ts, main.ts) ----------
	void BeginChapter4(const S47J::FObj& SavedCase);  // Prologue.beginChapter4
	void JumpChapter4();                              // Prologue.jumpChapter4
	/** The case chapter four began with (a save, or the done states jumpChapter4 writes): what is read
	 *  of the chapters before it when their classes are not there (chapter three's, while it is not in
	 *  this version). */
	S47J::FObj BeganCase;

	// Tomás' field radio: 'off' | 'playing' | 'on' | 'sending' (tests read it)
	FString WalkieState = TEXT("off");

	// The rules, the talk, the saved state and the sounds, without the world.
	static bool SelfTest(TArray<FString>& Out);
	// -S47Ch4Drive=<stop>: plays the chapter by itself up to a stop and holds there for the
	// screenshot (room, tv, walkie, talk, table, p10, line, end, leave), as tools/chapter4.py plays it.
	void TestDrive(const FString& Stop);

private:
	friend class FS47SC10Contract;
	TArray<S47Panels::FTalkLine> Lines;
	FString Mode = TEXT("menu");      // 'menu' | 'table' | 'control' | 'tone' | 'mark'
	TOptional<FS47Result> Reply;
	TOptional<S47Panels::FTalkHandle> Panel;
	bool bRoomBound = false, bCourtBound = false;
	double HeadYaw = 0;
	// The set is on when you come in: snow, and its blue light jumping on the walls. Nora keeps it
	// on for the noise. It reacts to nothing (R1); it is just a television at 4 am.
	bool bTvSeen = false;
	bool bTvTouched = false;          // until the player switches it, the set is on whenever they come in
	TSharedPtr<FS47WalkieSound> Walkie;
	bool bWalkiePressed = false;      // pressed to talk since it was switched on
	bool bWalkieHeard = false;        // Nora's lines come once each
	bool bWalkieSent = false;
	FString AfterText;                // this.after: told under her lines a moment after them ('' none)
	TArray<FString> TestNotes, TestDocs; // without a host (the self test)

	void BuildBook();
	TArray<FS47BoardCard> BoardCards() const;
	void ApplyWorld();
	void Save();
	FString Time() const;
	bool File(const FS47Doc& Doc);
	void Note(const FString& Text);
	void Toast(const FString& Text, float Secs) const;
	void Milestone() const;
	bool TextOnlyNora() const;
	void RestoreTalk();
	void ContinueAfterRoom();
	FVector Pos(const FS47Obj& O) const { return O.WorldPosition(); }
	FString Area() const;
	bool InRoom() const;
	bool ExitOpen() const;
	void SetLeaf(double K);
	void BindCourt();

	// the chapters before (their classes, or the case it began with)
	const US47Chapter1* Ch1() const;
	const US47Chapter2* Ch2() const;
	S47J::FObj Ch3() const;

	void UseTv();
	void TvSound();
	FString WalkieLabel() const;
	void UseWalkie();
	void WalkiePress();
	void WalkieOff();
	void UseDoor();
	void Look(const FString& What);
	void ReadPaper(const FString& Which);
	void UsePhone();

	void NoraToast(const FString& Text, float Seconds);
	void OpenTalk();
	bool Have(const FString& Id) const;
	FString Image(const FString& Id) const;
	S47Panels::FTalkView View() const;
	void ToggleChip(const FString& Id);
	void ShowEvidence();
	void FaceDown(const FString& How);
	void AnswerControl(const FString& Id);
	void AnswerTone(const FString& Tone);
	void MarkTalk(bool bLater);
	void TopicTomas();
	void TopicCable();
	void Give(const FString& Which);
	FS47Result RecordP11();
	void TopicLine();
	FS47Result RecordP12();
	void Ending();
	void Finish();

	FS47Doc DocCard() const;
	FS47Doc DocLetter() const;
	FS47Doc DocCorrection() const;
	FS47Doc DocFinding(const FString& K) const;
};
