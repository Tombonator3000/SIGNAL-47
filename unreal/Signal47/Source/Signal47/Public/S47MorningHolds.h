#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Main;
class US47Audio;

// web/src/story/morningHolds.ts: the cooler and the morning's held breath.
namespace S47Morning
{
	inline constexpr TCHAR KEY[] = TEXT("morning:held");
	inline constexpr double FROM = 5 * 3600 + 13 * 60, TO = 5 * 3600 + 20 * 60;
	inline constexpr TCHAR LINE[] = TEXT("There. Some mornings about now.");
	struct FCues { double Caption = 2.4, Line = 10.6, Cooler = 15.2, Radio = 20.5, Bird = 25.4, End = 29.5; };
	inline constexpr FCues CUES;
	inline constexpr double SETTLE = 14, PANEL_MAX = 75;
}

class SIGNAL47_API FS47DinerCooler
{
public:
	explicit FS47DinerCooler(US47Audio* InAudio) : Audio(InAudio) {}
	void Update(TOptional<FVector> At, bool bInside);
	void Off();
	void On();
	bool bRunning = true, bHeard = false;
	double Level = 0.09;
private:
	struct FNodes
	{
		TSharedPtr<FS47GainNode> Out;
		TArray<TSharedPtr<FS47OscillatorNode>> F0;
		TSharedPtr<FS47BiquadFilterNode> Lp;
		TArray<TSharedPtr<FS47ScheduledSourceNode>> Sources;
		void Stop();
	};
	TSharedPtr<FNodes> Build();
	US47Audio* Audio = nullptr;
	TSharedPtr<FNodes> N;
	FVector Pos = FVector::ZeroVector;
};

struct FS47MorningIn
{
	bool bHere = false, bArea = false, bFree = false, bPanel = false;
	double Clock = 0;
};

class SIGNAL47_API FS47MorningHolds
{
public:
	FS47MorningHolds(US47Main* InM, FS47DinerCooler* InCooler) : M(InM), Cooler(InCooler) {}
	FString Phase = TEXT("idle");
	double T = 0;
	TOptional<double> At;
	TFunction<bool()> RadioHeard;
	TFunction<void(double)> Radio;
	TFunction<void()> Saw;
	bool Busy() const { return Phase == TEXT("held"); }
	bool Had() const;
	void Update(double Dt, const FS47MorningIn& S);
	bool Now(const FS47MorningIn& S);
	void Reset();
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	FS47DinerCooler* Cooler = nullptr;
	double Cue = 0, Inside = 0, PanelT = 0;
	bool bByPanel = false, bRadioWas = false;
	void Begin(double Clock);
	void LetGo(bool bCut);
	void Save();
};
