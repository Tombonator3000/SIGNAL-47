#pragma once

#include "CoreMinimal.h"
#include "S47BoardRules.h"
#include "S47Docs.h"
#include "S47Seen.h"

class US47Audio;
class US47Annex;
class US47Inter;
class FS47ArrayTip;
class FS47AudioNode;
class FS47GainNode;
class FS47BufferSourceNode;
class FS47ScheduledSourceNode;
struct FS47Rect;

// web/src/story/archivePhone.ts. Chapter two gets one peak in the archive: the phone from the future, a
// second time (FORSLAG.md, 20 October, item 5 with Valg for Tom 1, which takes its place; Tom:
// "Spenning og skummelt slår faste regler", so the scarier one is built, with what of item 5 makes it
// stronger).
//
//   the build     the light steps on the roof (story/roofSteps.ts) come first; this never comes while
//                 they go, nor within BUILD seconds after them (one thing at a time, kept apart).
//   the call      in the records room, at its last quiet moment (P05 recorded, the key holder next;
//                 or while still reading, a while after the steps): the call recorder on the
//                 switchboard puts down a line by itself, IN TRK1 EXT 201, and the wall phone rings,
//                 and through the wall the empty control room's, on the same line. Or, if the player
//                 goes to the wall phone first to call the key holder: no dial tone; the line is
//                 already open. Either way, on it: this room, before it happens. The room's hum and
//                 the tired tube ticking, a thump through concrete, a drawer rolling out on its
//                 runners and slamming, something heavy falling, paper. Then the line is dead.
//   the 47        from the line's death, as at 02:14 (the prologue's grammar, Prologue.startCountdown):
//                 the corner clock in seconds; at once K3 through the wall (it trips before, not after:
//                 bank 3 is out, and there are 47 seconds to put it back); at :16 the beep; at :31 the
//                 call recorder prints again by itself, the night's first call once more, now with
//                 where it came from: EXT 201, the control room, to EXT 201 (R5: an imprint of it);
//                 at :40 the tube that has flickered all night goes out.
//   :47           it happens. A thump up through the floor: every drive in the array starts at once
//                 (story/archiveDishes.ts), and they all tip down from 32 to the horizon along 026.
//                 The drawer from the phone rolls out of the wall behind the STATION 01 box and throws
//                 the box on the floor. The printer in the control room runs by itself: with bank 3
//                 back in, the same solve as at 02:14, its old time on it (ns.solveAt); without, a
//                 blank sheet. The console log has the solve again at its old time.
//   after         the drawer stands out of the wall with reel 047 in it, NOT FOR CATALOGUE (the tape
//                 catalogue on the terminal goes 001-046, 048-1311). On the signal processor's deck
//                 (story/reel047.ts): the pulse group, a crank, a cut, 47 s of nothing, a relay, and
//                 -32 LY on the display. When the relay goes on the tape, the array goes back up to
//                 026 / 32, all at once ("reference motion observed", E10 will say of 02:17:47). Not
//                 played before the player leaves SARO, the dishes go back up unseen.
//
// The prologue's call stays the first; the note says this one has been heard before. Once in the
// night, chapter two's one 47-second event: the curve keeps the chapter's 47 for it (main.ts, keep)
// and it lands the peak (story/curve.ts, FELT 'archive'). No save from the ring until the dishes are
// down; a save after has the drawer out, the box on the floor and the dishes where they are, at rest.
// Kept in the town's part of the case (`arc:` keys). No voices: captions and code-made or recorded
// sounds (the bought ones through Audio.or, the game's own without them).

struct FS47ArcIn
{
	/** Chapter two at the records stage or ready for the key holder, not stood aside (tests). */
	bool bOk = false;
	/** Stage2 ('records', 'call-nora' ...), empty: null. */
	FString Stage;
	/** A record has been read. */
	bool bRead = false;
	/** Nothing open, no cinematic, no call on a line. */
	bool bQuiet = false;
	/** Nothing else of the night's is going (one at a time). */
	bool bFree = false;
	/** Something else of the night's is going now (main.ts, oneAtATime; the tape on the processor and
	 *  the CB not counted): a handset lifted then finds the line dead, it does not open this. */
	bool bOthers = false;
	/** The steps on the roof are going / have been. */
	bool bRoofBusy = false, bRoofDone = false;
	/** 'records' | 'annex' | 'room' | 'out' | 'away' */
	FString Where;
	/** A paper or the notebook open, a cinematic: the 47 holds, as the clock does. */
	bool bHold = false;
	double Clock = 0;
	FS47Cam Cam;
	/** In SARO's area. */
	bool bSaro = false;
	/** Somewhere the array could be seen from: out at SARO, or in the control room (its windows). */
	bool bView = false;
	/** Bank 3 is in (powered, K3 closed). */
	bool bBankIn = false;
	/** Chapter two has called the key holder (the recorder puts the call down). */
	bool bCalled = false;
};

/** story/reel047.ts as the archive's phone uses it (deck: Reel047), by its port (set by main.ts's
 *  wiring); unset: no deck yet, nothing plays. A cue's kind: 'thread' | 'start' | 'pulse' | 'crank' |
 *  'cut' | 'hole' | 'relay' | 'back' | 'range' | 'leader' | 'end'. */
struct FS47ArcDeck
{
	TFunction<bool()> Playing;                                    // deck.playing
	TFunction<bool()> Finished;                                   // deck.finished
	TFunction<void(double)> Update;                               // deck.update(dt)
	TFunction<void(TFunction<void(const FString& Kind)>)> Play;   // deck.play(cue)
	TFunction<void()> Reset;                                      // deck.reset()
};

struct FS47ArcDeps
{
	TWeakObjectPtr<US47Audio> Audio;
	TWeakObjectPtr<US47Annex> Annex;
	TWeakObjectPtr<US47Inter> Inter;
	FS47TownSeen* Seen = nullptr;
	FS47ArrayTip* Dishes = nullptr;
	FS47ArcDeck Deck;
	/** SARO's colliders (the box on the floor is in the way). */
	TFunction<TArray<FS47Rect>*()> Colliders;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, float)> Toast;
	TFunction<void()> Save;
	/** A line in the console log at a clock (Prologue.consoleLog). */
	TFunction<void(const FString&, double)> Log;
	/** K3 goes (Prologue.relayClack): true if it tripped, false if it had nothing left to trip. */
	TFunction<bool()> Relay;
	struct FAt { TFunction<FVector()> Rack, Desk, Printer, RecPhone; FVector Tube = FVector::ZeroVector; };
	FAt At;
	TFunction<void(bool)> Cinematic;
	TFunction<void(double)> Shake;
	/** Settings, Flashing images: slow fades. */
	TFunction<bool()> Calm;
	TFunction<void(const FS47Doc&)> File;
	TFunction<void(const FS47Doc&, TFunction<void()>)> Open;
	/** The night's solve (ns.solveAt), the prologue's call (from the console log), Ward's call (notes). */
	TFunction<TOptional<double>()> SolveAt, FirstCall, WardCall;
	/** The control room's printer: the paper feeding out (k 0..1) with the solve ('same') or blank, or
	 *  none (empty: torn off; with k below 0 only the printer's own paper put back, nothing hidden). */
	TFunction<void(const FString& Kind, double K)> Paper;
	/** The case board, drawn again. */
	TFunction<void()> Refresh;
	/** The signal processor can play (power, a tape locked tonight): why not, or empty. */
	TFunction<FString()> DeckReady;
	/** The steps on the roof now (story/roofSteps.ts, now): false if they cannot come. */
	TFunction<bool()> RoofNow;
};

class SIGNAL47_API FS47ArchivePhone
{
public:
	explicit FS47ArchivePhone(FS47ArcDeps InD);

	/** 'idle' | 'ringing' | 'line' | 'count' | 'moment' | 'done' */
	FString Phase = TEXT("idle");
	/** Stand aside: never comes (the tests of other things; the chapter goes as it did before it). */
	bool bOff = false;
	/** Seconds into the phase (the 47's own seconds in 'count'). */
	double T = 0;
	/** How it began: 'ring' | 'lift' (empty: null). */
	FString How;
	/** Which phone it was taken on: 'room' | 'desk' (empty: null). */
	FString Via;
	/** Where the player was at :47 (empty: null). */
	FString At47;
	/** The captions given since it began (tests). */
	TArray<FString> Shown;
	/** The beats as they came, with the 47's second they came at (tests). */
	struct FBeat { FString What; double T = 0; };
	TArray<FBeat> Beats;
	/** The corner clock with seconds while the 47 counts (main.ts, UI.clockSeconds). */
	struct FClockShow { FString Hm, Ss; };
	TOptional<FClockShow> ClockShow;
	TUniquePtr<FS47RuleBook> Book;

	// ---------- what main.ts asks ----------
	/** Not had yet tonight: chapter two's 47 is kept for it (story/curve.ts, keep). */
	bool Pending() const { return !bOff && !Has(TEXT("arc:call")); }
	/** Going: nothing else of the night's starts (one at a time), no save, the music holds. */
	bool Busy() const;
	/** The peak's own thing (the curve's rest counts from when it is over). */
	bool Peaking() const { return Busy(); }
	/** No save now: the 47 is a moment, and the dishes are moving or the reel is on. */
	bool Racing() const;
	/** A few seconds after it, nothing else starts either (the steps on the roof). */
	bool Resting() const { return Tail > 0; }
	/** The drawer stands out of the wall. */
	bool DrawerOut() const { return Has(TEXT("arc:done")); }
	/** The reel is in the kit: 'none' | 'kit' | 'heard'. */
	FString Reel() const { return Has(TEXT("arc:heard")) ? TEXT("heard") : Has(TEXT("arc:reel")) ? TEXT("kit") : TEXT("none"); }

	void Update(double Dt, const FS47ArcIn& S);

	/** The wall phone (and the control room's): what main.ts and story/Chapter2.ts ask first. */
	FString PhoneLabel() const { return Phase == TEXT("ringing") ? TEXT("Answer the phone") : FString(); }
	/** Used on a phone ('room' | 'desk'): answer it, or lift the wall phone to dial and find the line
	 *  open. False: the chapter's own use of it. */
	bool PhoneUse(const FString& Where, const FS47ArcIn& S);

	/** The strip's lines, as the night has them (the newest at the bottom). */
	TArray<FString> Lines() const;
	/** The strip as a paper, read off the recorder. */
	FS47Doc StripDoc() const;

	/** The control room's printer: a sheet hangs out of it ('same' | 'blank', empty: none). */
	FString Sheet() const;
	FString PrinterLabel() const { return !Sheet().IsEmpty() && PrintT < 0 ? TEXT("Tear off the printout") : FString(); }
	bool PrinterUse();

	/** What the processor offers while the reel is in the kit (story/Decoder.ts asks first). */
	FString DeckLabel() const;
	bool DeckUse();

	TArray<FS47RuleBook*> Books() { return Has(TEXT("arc:heard")) ? TArray<FS47RuleBook*>{ Book.Get() } : TArray<FS47RuleBook*>(); }

	// ---------- a night opened, the developer menu, the tests ----------
	/** Stop at once, everything as the night has it (a new night, another save, the title). */
	void Halt();
	/** A night put back from the town's part of the case. */
	void Restore();
	/** The recorder puts down the call to the key holder (main.ts, when chapter two has made it). */
	void CalledOut(double Clock);
	/** Now, whatever the waiting (the developer menu, the tests): 'ring' or 'lift'. */
	bool Now(const FString& InHow, const FS47ArcIn& S);

	// the papers and the sounds
	static FS47Doc ReelDoc();
	static FS47Doc ReprintDoc(const FString& Kind, TOptional<double> Solve, double Printed);
	static void DrawerRoll(US47Audio* A, const TSharedPtr<FS47AudioNode>& Dest, double When, double K = 1, double Dur = 0.55);
	static void BoxFall(US47Audio* A, const TSharedPtr<FS47AudioNode>& Dest, double When, double K = 1);

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47ArcDeps D;
	double Q = 0;               // quiet in the room, seconds
	double Since = -1;          // seconds since the steps on the roof were over (-1: not yet)
	double InRoom = 0;          // seconds in the room since the steps
	bool bWasRoof = false;
	int32 Rings = 0;
	double Lamp = 0;
	struct FRingHandle { TSharedPtr<FS47BufferSourceNode> Src; TSharedPtr<FS47GainNode> Gain; };
	TArray<FRingHandle> RingHandles;
	TArray<TSharedPtr<FS47AudioNode>> LineNodes;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> LineSrcs;
	struct FCue { double At = 0; TFunction<void()> Fn; };
	TArray<FCue> Cues;
	double CueT = 0;
	double DeadAt = 0;          // the night's clock when the line went dead
	double MomentAt = 0;        // and at :47
	double Tail = 0;
	double FallT = -1;
	double PrintT = -1;
	FString PrintKind = TEXT("blank");
	TArray<FString> Links;
	bool bWatchUp = false;
	bool bShownOk = false;
	double LineEnd = 0;
	TSet<FString> Beats47;
	double NowClock = 0;

	bool Has(const FString& K) const;
	void Add(const FString& K) const;
	void ArcLamp(const FString& Line, double K) const; // annex.arc.lamp

	void Found(const FS47ArcIn& S);
	void Idle(double Dt, const FS47ArcIn& S);
	void Ring(const FS47ArcIn& S);
	void Ringing(double Dt, const FS47ArcIn& S);
	void StopRing();
	void Answer(const FString& Where);
	void Lift(const FS47ArcIn& S);
	void Call(double Lead);
	void LineSound(double Lead);
	void ReleaseLine(double Fade = 0.05);
	void Dead(const FS47ArcIn& S);
	void Count(const FS47ArcIn& S);
	void Beat(const FString& What);
	bool HasBeat(const FString& What) const { return Beats47.Contains(What); }
	void Clock();
	void At47Now(const FS47ArcIn& S);
	void Moment(double Dt, const FS47ArcIn& S);
	void Finish(const FS47ArcIn& S);
	void FallStep(double Dt);
	void TipPose(double A);
	FVector2D TipAt(double A) const;
	void FloorPose(double Lift = 0);
	void BoxDown();
	void BoxBack();
	void UseDrawer();
	void PrintIn(double Clock);
	void PrintAgain(const FS47ArcIn& S);
	void Chatter(TFunction<void()> Then);
	void Strip();
	void ReadStrip();
	void PrintStart();
	void PrintStep(double Dt);
	double At47Clock() const;
	void ArrayStep(double Dt, const FS47ArcIn& S);
	bool FacingArray(const FS47Cam& Cam) const;
	TArray<FS47BoardCard> Cards() const;
	void KeepLinks();
	void Say(const FString& Text, float Secs = 3.2f);
	void Cue(double After, TFunction<void()> Fn);
};
