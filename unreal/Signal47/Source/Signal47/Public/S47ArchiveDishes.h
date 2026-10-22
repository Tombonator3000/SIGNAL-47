#pragma once

#include "CoreMinimal.h"

class AS47Web;
class FS47DishArray;
class US47Audio;
class FS47GainNode;
class FS47BiquadFilterNode;
class FS47OscillatorNode;

// web/src/story/archiveDishes.ts. The array tips down to the horizon along 026, all 27 at once, and
// later goes back up to 32 (story/archivePhone.ts: the archive's phone, chapter two; FORSLAG.md, 20
// October, Valg for Tom 1). Kept small and apart from the rest on purpose: the dishes have one owner
// at a time.
//
// Who has the dishes. Since 02:15 they stand at az 026, el 32 with nobody driving them (the prologue
// snaps them there). A story thing that moves them marks each one `rogue` (world/Dish.ts: "driven by
// nothing on the console") for as long as it has them, and gives them back exactly at 026 / 32 with
// `rogue` off: the dishes that follow the player in a warning (story/dishesFollow.ts) do it so, and
// this does too. So this takes the array only when no dish is rogue (nothing else has them), and
// while it has them `holds` is true: anything else that would drive them (the console's schedule,
// ARRAY RESUME, the warning's dishes) leaves them alone until it is false again.
//
//   Tip()      every drive at once: a ragged clunk of 27 brakes letting go, the motors of 02:15, and
//              every dish down from where it is to el 2 on 026, together, in some twenty seconds; the
//              brakes again all at once when they get there, and the motors stop.
//   Raise()    the same the other way: up to 32, then given back (rogue off, exactly 026 / 32).
//   Home()     at once, unseen (nobody at SARO to see it): back at 026 / 32 and given back.
//   Restore()  a night put back: tipped (as they were saved, at rest), or not.
//
// The dishes are AS47Web's (Exterior.dishes, drawn by the array's instanced parts); this points them
// all at once, as Dish.point() does for each. Their `rogue` flags while this holds them are Holds();
// the flags of the dishes that follow (story/dishesFollow.ts, its own port) come in through Deps.Rogue.
// The red lamps blink as one while they are down (DishArray.beaconPhase: FS47DishArray, which puts
// the phase on AS47Web's glow points).
struct FS47ArrayTipDeps
{
	TWeakObjectPtr<AS47Web> Web;       // ext.dishes, ext.dishArray
	TWeakObjectPtr<US47Audio> Audio;
	/** this.d.array (world/Dish.ts DishArray: FS47DishArray, the dishes' phases and the red lamps'
	 *  blink); unset or null: the lamps keep their own blink. */
	TFunction<FS47DishArray*()> Array;
	/** this.d.dishes.some((x) => x.rogue) for the dishes something else drives (story/dishesFollow.ts,
	 *  set by its port); unset: none is. */
	TFunction<bool()> Rogue;
};

class SIGNAL47_API FS47ArrayTip
{
public:
	static constexpr double TIP_AZ = 26, TIP_EL = 2;
	static constexpr double HOME_AZ = 26, HOME_EL = 32;
	/** How fast (Dish.slew, degrees a second in azimuth; elevation goes at 0.6 of it): 30 degrees of
	 *  elevation in about 22 seconds, the pace of the drives at 02:15. */
	static constexpr double SLEW = 2.3;

	explicit FS47ArrayTip(FS47ArrayTipDeps InD) : D(MoveTemp(InD)) {}

	/** 'idle' | 'tipping' | 'down' | 'raising' */
	FString Mode = TEXT("idle");
	/** Indoors (the annex, the control room): the motors are heard through the walls (main.ts). */
	bool bMuffled = true;

	/** This has the dishes (tipping, down, or on the way up): nothing else may drive them. */
	bool Holds() const { return Mode != TEXT("idle"); }
	/** They are moving now (the motors run; no save meanwhile). */
	bool Moving() const { return Mode == TEXT("tipping") || Mode == TEXT("raising"); }
	/** Every dish where the tipping leaves them, at rest. */
	bool Down() const { return Mode == TEXT("down"); }
	/** Something else has them now (a dish is rogue and this does not hold them). */
	bool Taken() const { return !Holds() && D.Rogue && D.Rogue(); }

	/** Every drive at once, down to the horizon along 026. False if something else has them. */
	bool Tip();
	/** Back up to 32 on 026, all at once, and given back when they are there. */
	bool Raise();
	/** At once, nobody there to see it: home, and given back. */
	void Home();
	/** A night put back: tipped and at rest, or not this one's at all. */
	void Restore(bool bTipped);
	/** A new night, the title: whatever this had, back as the night left it (026 / 32), quiet. */
	void Reset();
	/** Every frame while a night runs: 'down' or 'home' when they have just arrived, else empty. */
	FString Update(double Dt);
	/** Whether the motors are heard now (tests). */
	bool Humming() const { return Bed.IsSet(); }

	static bool SelfTest(TArray<FString>& Lines);

private:
	FS47ArrayTipDeps D;
	struct FBed { TArray<TSharedPtr<FS47OscillatorNode>> Osc; TSharedPtr<FS47BiquadFilterNode> Lp; TSharedPtr<FS47GainNode> G, Whine; };
	TOptional<FBed> Bed;
	/** The phase the red lamps blink at together while this has them (the first dish's). */
	double Common = 0;

	void Release();
	void Beacons(bool bOn);
	TArray<int32> Near(int32 N = 4) const;
	void Brakes(bool bStart);
	void Start();
	void Quiet(double Tau);
};
