#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// ANSWER-20261108 (Tom, 8 November: "antennen lokker. Peker spilleren arrayen mot et punkt på himmelen, kan noe svare der ute neste
// natt"). On the TRACKING terminal the array can be held on any point of the sky, off schedule. Held there a quarter of an hour or
// more, it has listened there; the next night, when the same point of the sky comes round to the same place (four minutes earlier
// by the clock), something may answer: the monitor channel prints by itself, a short run of pulses from that point, as many as the
// minutes the array was held there. Louder when the array is on the point again, faint in the sidelobes when it is not. It is not
// every time, and nothing says what it is. The printout waits in the control room's printer tray until the player is at SARO; then
// it is a field record (the case board, the night log, the weekly report). Freeform only, saved under "answer".
class FS47Answer
{
public:
	explicit FS47Answer(US47Main* InMain) : M(InMain) {}
	void Begin(const S47J::FObj& Saved);
	void Stop();
	void Step(double Dt);
	S47J::FObj Save() const;
	// TRACKING: hold the array on this point, off schedule. False: no power, or a point too low.
	bool Hold(double Az, double El);
	bool Holding() const { return bHolding; }

	struct FCall { double Az = 0, El = 0, From = 0, Held = 0; int32 Night = 0; bool bDone = false, bAnswered = false, bWaiting = false; double At = 0, Level = 0; };
	const TArray<FCall>& Calls() const { return Kept; }

	// ---------- the pure rules (tested) ----------
	// The same point of the sky, at the same place, comes round a sidereal day later: 23 h 56 min 4 s.
	static constexpr double SiderealDay = 86164.09;
	static constexpr double MinHeld = 15 * 60, Window = 4 * 60, MinEl = 10;
	// How many pulses: the minutes it was held (at least 15, at most 90).
	static int32 PulsesFor(double Held) { return FMath::Clamp(FMath::FloorToInt(Held / 60.0), 15, 90); }
	// Whether it answers: a seeded draw, likelier the longer the array was held there; never sure.
	static bool Answers(int32 Seed, int32 Index, double Held);
	static FString Printout(const FCall& C, const FString& Date, bool bOnPoint);
	static bool SelfTest(TArray<FString>& Lines);
	// F10: a call held this long, as if made one sidereal day ago; and the answer now.
	void DevCall(double Az, double El, double Held);
	bool DevAnswerNow();
private:
	US47Main* M = nullptr;
	TArray<FCall> Kept;
	bool bHolding = false;
	double HoldAz = 0, HoldEl = 0, HoldFrom = 0, Look = 0, OnSince = -1; // OnSince: when the dishes reached the held point
	FVector2D ArrayAzEl() const;
	void Answer(FCall& C, bool bForce);
	void Deliver(FCall& C);
};
