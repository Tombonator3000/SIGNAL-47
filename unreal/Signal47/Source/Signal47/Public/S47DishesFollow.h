#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"

class US47Audio;
class FS47DishArray;
class FS47AudioContext;
class FS47AudioNode;
class FS47GainNode;
class FS47OscillatorNode;
class FS47BiquadFilterNode;
class FS47PannerNode;

// web/src/story/dishesFollow.ts. The dishes come down and follow you (FORSLAG.md, 20 October, Valg for
// Tom 2, the full version; Tom: "Spenning og skummelt slår faste regler"). Since 02:15 every dish in the
// array has stood at az 026, el 32, with no command. Once in the night, when the warning of the time
// away (story/abduction.ts) comes while the player is out in the antenna field or close to it:
//
//   the start    on the warning's own beats (story/abduction.ts, BEAT; FORSLAG.md, 20 October,
//                item 2): a breath after the beep at :16, when the player's footsteps have gone,
//                a contactor and a brake out in the array, then the drive motor of 02:15, near.
//                The nearest dish lowers out of 026 and turns until it points at the player; the
//                next two after it, a second or so apart, each on the player within some eight
//                seconds however far round it has to come (by about :28). The middle distance
//                after them; the far ones with the pressure at :31. At :40 the flashlight dies and
//                they go on in the dark, by their red lamps, until the light at :47.
//   following    they track the player's eyes, the nearest fast enough to keep up. With the
//                footsteps gone, walking is heard only as their motors; standing still, they stop,
//                one brake after another, and it is quiet but for the warning's own pressure. The
//                one right over the player cannot come down far enough (its final limit, el -15)
//                and pushes against it now and then. Further out, later and slower (one bed of
//                sound, not placed); far out, late or not at all. The red lamps of those that have
//                been on the player slide into one blink.
//   escape       off the line, inside, in the truck: they stay on the place a while, then go back to
//                026 at the pace of 02:15.
//   taken        when the light comes they turn away from the player all at once, to the grass
//                where the player will wake (chosen then, Abduction.backAt). On waking they are
//                pointing down at it, and the view lifts from the stars towards the nearest; a
//                few seconds later they let go and go back.
//   the record   the console log has each drive that moved with no command, the final limit, and
//                the array back at 026 (SHOW LOG); the notebook has what was seen. Kept with the
//                case (AbductSave.dishes): it does not come again, not after Continue either.
//
// Later chapters see the array at 026: every dish ends exactly there (Dish.snap), and a save made
// while they are out comes back with them home and the log line written. Cheap on a phone: the
// array's matrices change only for the dishes that move (DishArray.refresh), three motor voices
// at most, one bed for the rest. Main.ts calls update() every frame of play, before the array is
// drawn; the Missing time setting turns the warning, and so this, off.

namespace S47Follow
{
	inline constexpr double HOME_AZ = 26, HOME_EL = 32;
	/** As low as a drive goes: below its own final limit already, and not low enough for someone under it. */
	inline constexpr double EL_MIN = -15;
	/** story/abduction.ts: the warning's length and its beats (the steps gone, the tone, the dark). */
	inline constexpr double WARN = 47;
	struct FBeat { double Steps, Tone, Dark; };
	inline constexpr FBeat BEAT = { 16, 31, 40 };
}

/** AbductSave.dishes */
struct SIGNAL47_API FS47DishesSave
{
	double Clock = 0;
	TArray<FString> Ids;
	bool bBack = false;
	S47J::FObj ToJson() const;
	static TOptional<FS47DishesSave> Copy(const S47J::FObj& O);
};

struct FS47FollowIn
{
	/** The time away's stage: 'calm' | 'warning' | 'light' | 'taken' | 'black' | 'back'. */
	FString Stage = TEXT("calm");
	/** The warning: seconds into it, and how long it is. */
	struct FWarn { double T = 0, Len = 47; };
	TOptional<FWarn> Warning;
	/** In SARO's area, where the array is; on foot there. */
	bool bSaro = false;
	bool bOnFoot = false;
	/** The player's eyes and which way they look. */
	FVector Eye = FVector::ZeroVector;
	FVector View = FVector(0, 0, -1);
	double Clock = 0;
	/** Where the player will wake, once the light has come. */
	TOptional<FVector> BackAt;
	/** The calm flashes setting: the brakes a little softer. */
	bool bCalm = false;
	/** The clock the player woke at, after the time away (its notes have it). */
	double Woke = 0;
	/** One of them is standing over the player on waking (story/abduction.ts): which way the player
	 *  lies is the abduction's (they against something lighter), and the dishes' watch waits for it. */
	bool bOver = false;
};

struct FS47FollowDeps
{
	FS47DishArray* Dishes = nullptr;
	TWeakObjectPtr<US47Audio> Audio;
	/** Kept with the case (the abduction's save). */
	TFunction<TOptional<FS47DishesSave>()> Get;
	TFunction<void(const FS47DishesSave&)> Set;
	TFunction<double(double, double)> Ground;
	TFunction<void(const FString&, double)> Toast;
	TFunction<void(const FString&)> Note;
	/** A line in the console log at its own time (Prologue.consoleLog). */
	TFunction<void(const FString&, double)> Log;
	TFunction<void()> Save;
	/** Turn the player's head (waking on the back: towards the one that watches). */
	TFunction<void(double)> Face;
	/** nearArray(x, z) (world/arrayLayout.ts with the field's zones from the export). */
	TFunction<bool(double, double)> NearArray;
};

class SIGNAL47_API FS47DishesFollow
{
public:
	explicit FS47DishesFollow(FS47FollowDeps InD);
	~FS47DishesFollow();

	/** 'idle' | 'follow' | 'hold' | 'away' | 'watch' | 'home' */
	FString Mode = TEXT("idle");
	/** Seconds since it began (tests). */
	double T = 0;

	/** Out in the antenna field, or close enough to one of its dishes (world/arrayLayout.ts onField with
	 *  the field's zones, nearestAntenna). */
	static bool NearArray(const TArray<FS47Area>& FieldZones, const FS47DishArray* Dishes, double X, double Z);
	/** Seconds into the warning before the first one starts: a breath after the beep at :16 (the
	 *  footsteps gone), as much earlier in a warning the tests cut short. */
	static double FirstAt(double Len) { return (S47Follow::BEAT.Steps + 1.5) * FMath::Min(1.0, Len / S47Follow::WARN); }

	/** The dishes that are part of it now (tests), with how they stand. */
	struct FState { FString Id; int32 Tier = 0; bool bOn = false; double OnAt = -1; bool bLocked = false, bEver = false, bRun = false; double Az = 0, El = 0; FVector Aim; bool bPush = false; };
	TArray<FState> State() const;
	/** The voices that sound now (tests): the dish each is on and its level. */
	TArray<TPair<FString, double>> Voices() const;
	/** Held still (main.ts: the notebook open in the warning holds the night): nothing turns, so
	 *  no motor is heard; update() takes it up again. */
	void Wait();

	void Update(double Dt, const FS47FollowIn& S);

	// ---------- the case ----------
	/** Everything stopped, the dishes home and their own again (a new night, the title). */
	void Reset();
	/** After a case is opened (the abduction's save restored, the log with it): a save made while
	 *  they were out has them home, and the line that says so. */
	void Restore();

	/** The near ones' fields, also for main.ts's followDish (story/seen.ts). */
	struct FRate { double Az = 0, El = 0, Acc = 0; };
	struct FRig
	{
		int32 I = 0; int32 Tier = 0;
		/** When it starts, in seconds of the event; started. */
		double At = 0; bool bOn = false; double OnAt = -1;
		double Az = 0, El = 0, VAz = 0, VEl = 0;
		/** Where it points: the player's eyes while it follows, then the place. */
		FVector Aim = FVector::ZeroVector;
		bool bLocked = false, bEver = false;
		/** Running (its motor heard, a brake each way), and how long it has been nearly still. */
		bool bRun = false; double Still = 0, ClunkT = -9;
		/** Against the final limit: seconds to the next push, and the push. */
		double Strain = 0, Push = 0; bool bLimit = false;
		/** How fast it may go (the near ones faster the further they have to come round). */
		FRate Rate;
		/** Its red lamp's place in its blink. */
		double Ph = 0;
	};

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47FollowDeps D;
	TArray<TSharedPtr<FRig>> Rigs;   // (a Map by dish id in the web version: in the order they were added)
	TSet<FString> Skip;
	double Clock0 = 0;
	double Len = 30;
	double HoldT = 0;
	double ChooseT = 0;
	TOptional<double> Common;
	struct FSaid { bool bStart = false, bLock = false, bMove = false; } Said;
	/** Where the player stood when it began, and whether they went on from there. */
	FVector From = FVector::ZeroVector;
	bool bMoved = false;
	FVector LockAt = FVector::ZeroVector;
	double FirstFar = -1;
	/** The notebook has what they did (record): it gets their going home too. */
	bool bNoted = false;
	TUniquePtr<class FS47Drives> Sound;

	TSharedPtr<FRig> RigOf(const FString& Id) const;
	const struct FS47Dish& DishOf(const FRig& R) const;
	FString IdOf(const FRig& R) const;
	void Begin(const FS47FollowIn& S);
	void Choose(const FVector& Eye, double W0);
	void Promote(const FVector& Eye);
	TSharedPtr<FRig> MakeRig(int32 I, int32 Tier, double At, const FVector& Eye) const;
	void Follow(double Dt, const FS47FollowIn& S);
	void Start(FRig& R, const FS47FollowIn& S);
	void HoldPlace(const FS47FollowIn& S);
	void Away(const FS47FollowIn& S);
	void Woke(const FS47FollowIn& S);
	void GoHome(const FS47FollowIn& S);
	void End(double Clock, bool bSeen = true);
	void Release(FRig& R);
	void Record(const FString& After);
	void Step(double Dt, const FS47FollowIn& S);
	void Write(FRig& R, double Off);
	FRate RateFor(int32 I, int32 Tier, const FVector& Eye, TOptional<double> FromAz = {}) const;
	void Log(const FString& Text, double At) const { if (D.Log) D.Log(Text, At); }
	void Toast(const FString& Text, double Secs) const { if (D.Toast) D.Toast(Text, Secs); }
	void Note(const FString& Text) const { if (D.Note) D.Note(Text); }
	TOptional<FS47DishesSave> Kept() const { return D.Get ? D.Get() : TOptional<FS47DishesSave>(); }
};
