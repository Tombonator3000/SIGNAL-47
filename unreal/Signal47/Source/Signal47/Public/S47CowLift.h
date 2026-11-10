#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class FS47UfoPresentation;
class UPrimitiveComponent;

// PHOTO-02 (N1110, Tom's cow example). The cow FS47Aftermath already plans to be gone from the pasture can, on that same night, be
// seen to go: a pale light stands over the Alderete pasture, the beam comes down on one cow (always the same one, S47Herd::MISSING,
// tag 25), she rises slowly, turning, and the light leaves. Only when the player is out there to see it, in the dark hours after
// 02:00 and with nothing else of the Director's going on. Nothing new is invented: the plan, the cow, the craft and beam
// (FS47UfoPresentation) and the field record are the ones that were already there. If the player walks away mid-lift the cow is
// let down again, step by step, the same way she went up: the lift is reversible until it is finished, and its pose is saved by
// the seconds into it. A cow gone from an empty pasture, with nobody there, stays the separate silent track it always was.
// Freeform only; saved inside "aftermath" as "lift", and absent when nothing is under way.
class FS47CowLift
{
public:
	explicit FS47CowLift(US47Main* InMain);
	~FS47CowLift();
	void Begin(const S47J::FObj& Saved);
	void Stop();
	S47J::FObj Save() const;
	enum class EStep : uint8 { Idle, Lifting, Done, LetDown };
	// One step. bMayStart: the plan, the hour and the player's place allow a lift to begin; bWatching: the player is where it can be seen
	// from. Done is returned once, the moment she is gone from the pasture; LetDown once, when she is back on the grass unlifted.
	EStep Step(double Dt, bool bMayStart, bool bWatching);
	bool Active() const { return Cow >= 0; }
	int32 CowId() const { return Cow; }
	double Seconds() const { return T; }
	// Visible lift: well off the ground with the craft and beam really drawn. Photo credit asks for exactly this, nothing less.
	bool Visible() const;
	FVector CraftAt() const { return CraftPos; }
	FVector CowNow() const { return CowPos; }
	const UPrimitiveComponent* Hull() const;

	// ---------- the pure rules (tested) ----------
	static constexpr double Arrive = 10, Rise = 34, Hold = 4, Leave = 8, TopHeight = 14, CraftHigh = 26, MinHour = 2, MaxHour = 5;
	static constexpr double GoneAt = Arrive + Rise + Hold, Total = GoneAt + Leave;
	static constexpr double MinDistance = 30, MaxDistance = 85, VisibleHeight = 2.5;
	static double HeightAt(double Sec);
	static double SpinAt(double Sec);
	static double CraftAltitude(double Sec);
	static double BeamAt(double Sec);
	static double OpacityAt(double Sec);
	// The hour (0..24 of the clock), how long until the planned morning, how far the player stands from her, and whether the Director
	// has something of its own going: may it begin?
	static bool MayBegin(double Hour, double SecondsToMorning, double Distance, bool bPlanned, bool bDirectorBusy);
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	int32 Cow = -1;
	double T = 0, Tail = -1; // Tail: seconds into the craft's own leaving, after she is gone (never saved)
	FVector CraftPos = FVector::ZeroVector, CowPos = FVector::ZeroVector;
	TUniquePtr<FS47UfoPresentation> Ufo;
	void Apply(double Sec, int32 LiftedCow);
	void Clear();
};
