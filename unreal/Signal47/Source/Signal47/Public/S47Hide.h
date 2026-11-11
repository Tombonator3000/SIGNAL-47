#pragma once

#include "CoreMinimal.h"

class US47Main;
class UStaticMeshComponent;

// N1110 (HORROR-IDEAS-20261106-10 point 6, Tom): hiding under a table while something goes past. At the control-room desk and the
// archive table at SARO the player may look at the dark space under the top and press E: they creep in and stay there, still and
// low, the eye some 1 m below standing eye height with the top edge of the table closing in on the view (the same lids as tiredness
// draws, from the top), their breathing a little heavier (FS47Stress) and a lamp in the hand still a lamp. Hidden, S47Sneak's numbers
// run to their hidden end (AS47Walker::Sneak() 1 to 2): what looks for the player sees them from three metres, hears their feet from
// one, loses interest much slower and finds a chest that is nearly on the floor late. E again comes out, to the place they went in
// from. Not saved, not a new danger: it gives no event of its own, and the night's one danger is counted by whatever it is.
//
// Never while the door loop runs (its chase included: the loop's doors are the way out), never while being taken, in the truck or
// with a menu or paper open; any of them while hidden brings the player out at once. The kitchen table of the old farmhouse is not
// a place yet: the house has no furniture in code to hide under (add a row to Spots() when it has one).
class SIGNAL47_API FS47Hide
{
public:
	explicit FS47Hide(US47Main* Main) : M(Main) {}
	~FS47Hide();
	void Begin();
	void Stop();
	void Step(double Dt);

	bool Hidden() const { return bIn; }
	/** 0 to 1, how far the table's edge has come down over the view (for the screen's lids). */
	double Frame() const;
	/** Where under which table, for F10 and the self-test. */
	static int32 SpotCount();
	/** Why not (empty: may), from the world as it is. */
	FString WhyNot() const;

	// ---- the rule, pure (tested) ----
	static constexpr double Reach = 2.4;        // looking at it from this far (m)
	static constexpr double LeaveLock = 0.7;    // seconds before E counts as "come out" after going in
	static constexpr double Strain = 0.3;       // the stress held at least this high while hidden: heavier breath
	static constexpr double LidShare = 0.38;    // the view's top lid at full cover
	static bool MayHide(bool bFreeformOn, bool bHeld, bool bDriving, bool bDoorLoop, bool bTaken, bool bModal);
	static double LidOf(double HideK) { return FMath::IsFinite(HideK) ? LidShare * FMath::Clamp(HideK, 0.0, 1.0) : 0.0; }
	/** Whether (X, Z) is near enough a spot's centre to be put back at: a table half-width and a bit. */
	static bool InsideSpot(int32 Spot, double X, double Z, FString& OutArea);
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TSharedPtr<uint8> Life = MakeShared<uint8>(0);
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Targets;
	bool bBuilt = false, bIn = false;
	int32 At = -1;
	double Age = 0, Tries = 0;
	FVector2D Back = FVector2D::ZeroVector;
	double BackYaw = 0;
	bool Live() const;
	void Build();
	void Enter(int32 Spot);
	void Leave(const TCHAR* Why);
};
