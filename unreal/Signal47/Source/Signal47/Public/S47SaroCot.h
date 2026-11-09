#pragma once

#include "CoreMinimal.h"

class US47Main;
class UStaticMeshComponent;

// SARO-COT (Tom, 9 November): a bed at the station. Until now sleep at SARO was only "REST UNTIL MORNING" on the OPS terminal. An
// army folding cot (canvas on two rails, X legs, a wool blanket folded at the foot, a pillow), built of plain shapes in the game's own
// materials, stands in the first of a few sensible places that is really free: inside a room's walk zone, clear of every walk
// collider, and with nothing of the world (wall, desk, shelf) in the space it takes, measured by traces through that space when it is
// built. It has its own walk collider, so it is walked round and not through.
//
// Lying down on it is the same rest as OPS: FS47FreeformSession::WaitUntil at the station (refused with the same words when something
// is close; woken by the night's own; FS47Fatigue counts it as a night at the station; the standby charge as at OPS), with the screen
// going dark and coming back. Saving is untouched: S47SaroSave saves at SARO as it does after a rest at OPS. Freeform only; nothing of
// the cot itself is saved, it is always there.
class SIGNAL47_API FS47SaroCot
{
public:
	explicit FS47SaroCot(US47Main* Main) : M(Main) {}
	~FS47SaroCot();
	void Begin();
	void Stop();
	void Step(double Dt);

	bool Built() const { return bBuilt && Spot >= 0; }
	/** Where it stands (three-space: the middle of it on the floor) and which way its length runs. */
	FVector Center() const { return At; }
	bool AlongZ() const { return bAlongZ; }
	FString Place() const;
	/** What the placement measured, place by place. */
	const TArray<FString>& Checks() const { return Notes; }
	/** Lie down: until 08:00, or for this many hours. False with the reason on the session's feedback, as at OPS. */
	bool Sleep(double Hours);
	void OpenMenu();
	/** Measure a place again now (F10): empty when it is clear, else what is in the way. */
	FString Measure(double X, double Z, bool bAlong) const;

	static constexpr double Length = 1.90, Width = 0.68, Height = 0.42;
	/** The next 08:00 after this world clock (seconds), as OPS's REST UNTIL MORNING has it. */
	static double MorningAfter(double Clock);
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	TSharedPtr<uint8> Life = MakeShared<uint8>(0);
	TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;
	TArray<FString> Notes;
	FVector At = FVector::ZeroVector;
	bool bBuilt = false, bAlongZ = true, bShown = false;
	int32 Spot = -1, Tries = 0;
	double Wait = 0, FadeLeft = -1;
	FString After;
	bool Live() const;
	void Build();
	void Show(bool bOn);
	void SetCollider(bool bOn);
};
