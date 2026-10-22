#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Seen.h"
#include "S47Story.h"

class US47Audio;
class FS47DishArray;

// web/src/story/arrayBack.ts. Put the array back. It goes back (FORSLAG.md, 20 October, item 12; Tom:
// "Spenning og skummelt slår faste regler"). The night watchman fails at the real job, with his own
// hands (R1). Since 02:15 every dish has stood at az 026, el 32, with no command (canon). In chapter two
// Ward calls back: "Put the array back on the schedule and tell me it stays there." (story/Chapter2.ts).
// At the operations terminal on the west desk (story/nightshift.ts) ARRAY RESUME puts it back:
//
//   slew     the contactors go one after another out across the field, the console's motor hum of
//            02:15 (AudioSys.startMotors), and all 27 turn to the night's schedule, az 042, el 48
//            (story/field.ts, SCHED; the status screen's SCHEDULE line). It obeys. The brakes set,
//            the hum stops, ON SCHEDULE in the console log, and on the status screen a count.
//   hold     47 seconds on the schedule. Nothing happens, except that the red lamps on the feeds,
//            each blinking in its own time, slide into one blink across the whole field; towards
//            the end the crickets stop.
//   signal   at 47 every light out in the field goes out at once, the lamps on the posts, their
//            floods and the red lamps on the feeds; then four flashes of the red lamps, all of them
//            together, a gap, and seven (the pulse group of 02:14, slowed so it can be counted),
//            each flaring round its dish and lighting the near ones red. In the control room the
//            status console beeps with them; out of doors each comes with the signal's deep thump.
//            Then the field's lamps are back.
//   back     and all 27 turn back to 026 / 32 at the pace of 02:15, in perfect step and without a
//            sound: no contactor, no motor, nothing on the drive bus (DRIVE CURRENT 0.0 A in the
//            log), the red lamps steady, not blinking. Home, every brake at once.
//
// Every time the player tries. Each try is kept with Night Shift's part of the case
// (NightShiftState.array): the console log has it, S-03's motor bus has it in its own log (the S-03
// history, Chapter1.docS03), the notebook has it, and in chapter five Ward has the count
// (Chapter5.ts). Before Ward has asked (chapter one, chapter two before her second call) the array is
// held for the supervisor.
//
// One thing at a time (main.ts, oneAtATime): it does not start while one of the night's things is
// going (the console says ARRAY LINK BUSY), nor while the dishes move by themselves (DRIVES IN
// MOTION), and while it runs nothing else starts. The night's curve lands its peak on it
// (story/curve.ts, FELT) and does not start its rest over when it comes in one (AGAIN). If the
// warning of the time away comes anyway (the developer menu), the array leaves the schedule for it:
// the dishes the warning takes (story/dishesFollow.ts) are the warning's, the rest turn back to 026 at
// once. Saves wait for it (main.ts, saveBlock); a case kept in the middle of it comes back with the
// array at 026, the try counted and the log written (restore). Like everything in the night it waits
// while a panel is open (main.ts gives it no time then). Main.ts calls update() every frame of play,
// before the array is drawn.

namespace S47ArrayBack
{
	inline constexpr double HOME_AZ = 26, HOME_EL = 32, SCHED_AZ = 42, SCHED_EL = 48;
	/** Seconds on the schedule before it goes. */
	inline constexpr double HOLD = 47;
	/** How long the lamps' four and seven take, from the 47th second to the turn. */
	SIGNAL47_API double SIGNAL();
	/** How many times, in words: once, twice, three times. */
	SIGNAL47_API FString Times(int32 N);
	/** Where the lamps are in their four and seven, seconds after the 47th: on, and which flash. */
	struct FLamps { bool bOn = false; int32 N = -1; };
	SIGNAL47_API FLamps LampsAt(double Ts);
}

/** One try, by the night's clock (whole seconds): the command, on the schedule, leaving it (47 s
 *  after: the lamps' four and seven), turning back, home at 026. */
struct SIGNAL47_API FS47ArrayTry
{
	double Cmd = 0;
	TOptional<double> On, Lost, Turn, Home;
	S47J::FObj ToJson() const;
	/** copyTry: a copy of a kept one, or unset for anything that is not one. */
	static TOptional<FS47ArrayTry> Copy(const TSharedPtr<FJsonValue>& V);
};
/** What is kept with the case (NightShiftState.array). */
struct SIGNAL47_API FS47ArraySave
{
	TArray<FS47ArrayTry> Runs;
	/** A try going on as the case was kept, as far as it had come (restore finishes it). */
	TOptional<FS47ArrayTry> Run;
	S47J::FObj ToJson() const;
	/** copyArraySave: a copy of a kept one, or unset for anything that is not one. */
	static TOptional<FS47ArraySave> Copy(const S47J::FObj& O);
};

struct FS47ArrayIn
{
	/** In SARO's area, where the array is. */
	bool bSaro = false;
	/** In the control room (the status console, the windows), and out of doors at SARO. */
	bool bRoom = false;
	bool bOutside = false;
	/** In the truck at SARO: the field through the windscreen (nothing heard of it over the engine). */
	bool bCab = false;
	FS47Cam Cam;
	FVector Eye = FVector::ZeroVector;
	/** The warning of the time away has begun (story/abduction.ts): the array leaves the schedule. */
	bool bWarning = false;
	/** The dishes the warning's dishes have (story/dishesFollow.ts), while they have any. */
	TOptional<TSet<FString>> Taken;
	/** The dishes that follow are not going (their lamps are this one's again). */
	bool bFollowIdle = true;
	/** The calm flashes setting: the lamps' four and seven softer. */
	bool bCalm = false;
};

/** Why the console will not do it now (empty: it will): 'early' | 'release' | 'moving' | 'tipped' | 'busy'. */
using FS47ArrayGate = FString;

struct FS47ArrayDeps
{
	FS47DishArray* Dishes = nullptr;
	TWeakObjectPtr<US47Audio> Audio;
	/** Kept with Night Shift's part of the case. */
	TFunction<TOptional<FS47ArraySave>()> Get;
	TFunction<void(const FS47ArraySave&)> Set;
	TFunction<double()> Clock;
	/** A line in the console log at its own time (Prologue.consoleLog). */
	TFunction<void(const FString&, double)> Log;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, double)> Toast;
	TFunction<void()> Save;
	/** The crickets (Ambience.hush). */
	TFunction<void(double, double, double, double)> Hush;
	/** S-03's motor bus has more in its log (Chapter1 files it again). */
	TFunction<void()> S03;
	/** The floods out among the near dishes (Exterior.fieldLights): as built (unset), out (0), or red. */
	TFunction<void(TOptional<double>, bool)> Field;
};

class SIGNAL47_API FS47ArrayBack
{
public:
	explicit FS47ArrayBack(FS47ArrayDeps InD);

	/** 'idle' | 'armed' | 'slew' | 'hold' | 'signal' | 'back' | 'settle' */
	FString Stage = TEXT("idle");
	/** Seconds into the stage. */
	double T = 0;
	/** What the player got of this try (and the last): the lamps seen, the console's beeps heard,
	 *  the dishes seen turning back. */
	struct FGot { bool bLamps = false, bBeeps = false, bBack = false; } Got;
	/** The lamps' flashes, as they come (tests): how many came, and whether they are lit now. */
	int32 Flashes = 0;
	bool bLit = false;

	bool Busy() const { return Stage != TEXT("idle"); }
	/** How many times it was put back and went back. */
	int32 Count() const;
	/** Every try kept (tests, the S-03 history). */
	TArray<FS47ArrayTry> Runs() const;
	/** Where to look to see it: the middle of the array's near rows, a lamp's height. */
	static FVector Centre() { return FVector(-12, 14, -120); }

	/** The status screen in the control room (Prologue.drawStatus), while a try runs. */
	TOptional<FS47ArrayStatus> Status() const;
	/** What the status terminal says when used during a try (Prologue's crtLeft); empty: null. */
	FString StatusText() const;

	// ---------- the console (story/nightshift.ts) ----------
	/** The operations terminal's answer to one of the array's commands, or unset when it is not one. */
	TOptional<FString> Command(const FString& Input, const FS47ArrayGate& Gate);
	/** S-03's own log since 02:15 (Chapter1.docS03): each command, on the schedule, and the turn back
	 *  with no command. The last four tries. */
	struct FHistory { int32 Cmds = 0; FString Text; };
	TOptional<FHistory> History() const;

	void Update(double Dt, const FS47ArrayIn& S);

	// ---------- the case ----------
	/** Everything stopped (a new night, the title, a jump): nothing of a try runs on. */
	void Reset();
	/** After a case is opened (Night Shift's part back, the dishes at 026): a try kept in the middle
	 *  of it is over, counted, and the log has the rest. */
	void Restore();

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47ArrayDeps D;
	struct FRig
	{
		int32 I = 0;
		double Az = 0, El = 0, VAz = 0, VEl = 0;
		/** When its contactor goes (seconds into the slew); started; one of the few heard close by. */
		double At = 0; bool bOn = false, bLoud = false;
		/** Still this one's (the warning's dishes have not taken it). */
		bool bMine = true;
	};
	TArray<FRig> Rigs;
	/** This try's clocks. */
	FS47ArrayTry Run;
	/** Each lamp's place in its blink (the array's lamps, by index), and the one they slide to. */
	TArray<double> Ph;
	double Common = 0;
	bool bHushed = false, bCut = false, bDrifting = false, bMotors = false;
	struct FSaid { bool bSync = false, bFlash = false, bTurn = false; } Said;

	FS47ArraySave Kept() const;
	void Begin();
	void Lamps();
	void Keep();
	void Loud(const FVector& Eye);
	void Slew(double Dt, const FS47ArrayIn& S);
	bool Drive(FRig& R, double ToAz, double ToEl, double Dt);
	void OnSchedule(const FS47ArrayIn& S);
	void Hold(double Dt, const FS47ArrayIn& S);
	void Signal(const FS47ArrayIn& S);
	void Turn(const FS47ArrayIn& S);
	void Back(double Dt, const FS47ArrayIn& S);
	void Home(const FS47ArrayIn& S);
	void End(bool bSeen);
	void Leave();
	void Away();
	void Drift(double Dt, const FS47ArrayIn& S);
	bool Sees(const FS47ArrayIn& S) const;
	bool InView(const FS47Cam& Cam) const;
	void Thump(double Level);
	void Clunk(int32 Dish, bool bStart, bool bCalm, double When = 0);
	void Log(const FString& Text, double At) const { if (D.Log) D.Log(Text, At); }
	void Toast(const FString& Text, double Secs) const { if (D.Toast) D.Toast(Text, Secs); }
	void Note(const FString& Text) const { if (D.Note) D.Note(Text); }
	double Clock() const { return D.Clock ? D.Clock() : 0; }
	void Field(TOptional<double> K, bool bRed = false) const { if (D.Field) D.Field(K, bRed); }
	void HoldBeacons(TOptional<double> Level, double Flare = 0) const;
	void BeaconPhase(int32 I, double P) const;
	const struct FS47Dish* Dish(int32 I) const;
};
