#pragma once

#include "CoreMinimal.h"
#include "S47Audio.h"
#include "S47Seen.h"

class US47Main;
class FS47Town;

struct FS47TownEventsIn
{
	bool bOk = false, bInTown = false, bLater = false;
	FS47Cam Cam;
	double Clock = 0, T = 0;
	bool bMay = true, bMayLong = true;
	double Unrest = 0;
};
struct FS47TownMark { FString Kind; double Clock = 0; };
SIGNAL47_API TArray<FS47TownMark> S47TownMarks(const FS47TownSeen& Seen);

// web/src/story/townEvents.ts: Caliche's four events, once in the saved night.
class SIGNAL47_API FS47TownEvents
{
public:
	explicit FS47TownEvents(US47Main* InM) : M(InM) {}
	FString Now = TEXT("none");
	bool bWaiting = false;
	int32 Tinks = 0;
	void Bind(FS47Town* InTown);
	void Reset();
	void Update(double Dt, const FS47TownEventsIn& S);
	void Answer();
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	FS47Town* Town = nullptr;
	double T = 0, Near = 0, Street = 0, RingT = 0, LineT = 0;
	int32 Call = 1;
	double WaveT = 0;
	TArray<int32> Dogs;
	double NotBefore = 0;
	bool bCutoutSeen = false, bCarSeen = false;
	double LineCheck = 0, TinkT = 3;
	TArray<FS47Play> Recordings;
	TSharedPtr<FS47AudioNode> LineSpeaker, LineBand;
	double Last() const;
	bool Free(double Clock) const;
	void Mark(const FString& Kind, double Clock);
	FVector Pos(const FVector& Local) const;
	void Tink(const FVector& At, double U);
	void HangUp(double Clock);
	void StopRing();
	void StopAll();
	void StopRecording();
	void Note(const FString& Text);
	void Toast(const FString& Text, double Secs);
	void Save();
};
