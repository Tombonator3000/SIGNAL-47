#pragma once

#include "CoreMinimal.h"
#include "S47WebAudio.h"

class US47Main;
class AS47Web;
class UMaterialInstanceDynamic;

// A short authored beat, never a random event system. The caller owns its saved once flag.
// Kind 3 deliberately blinks the closed studio's ON AIR sign after 04:00; other kinds
// briefly dip one existing street lamp. All levels and audio routing are restored in Reset.
class SIGNAL47_API FS47AnomalyDread
{
public:
	explicit FS47AnomalyDread(US47Main* InM);
	~FS47AnomalyDread();
	// True only when an existing physical light/sign channel was found and written.
	bool Pulse(FVector At, int32 Kind);
	// Must run after Town::Update so its temporary ON AIR overlay reaches the frame.
	void Step(double Dt);
	void Reset();
	bool Busy() const { return bActive; }
	bool ProtectedScene() const { return Protected(false); }
	static bool SelfTest(TArray<FString>& Lines);
private:
	TWeakObjectPtr<US47Main> M;
	TWeakObjectPtr<AS47Web> Web;
	struct FPaint
	{
		TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
		FLinearColor Original, On, Applied;
	};
	TArray<FPaint> Paint;
	int32 ActiveKind = 0, FloodSet = -1, FloodSlot = -1;
	float OriginalWidth = 0, AppliedWidth = 0;
	bool bActive = false, bFirstFrame = false;
	double Started = 0, Elapsed = 0;
	FString Area;
	TSharedPtr<FS47AudioContext> Context;
	TSharedPtr<FS47GainNode> AmbientBus, MasterBus, Duck, Cue;
	TArray<TSharedPtr<FS47ScheduledSourceNode>> Sources;
	TArray<TSharedPtr<FS47AudioNode>> CueNodes;
	bool Protected(bool bIncludeOneAtATime) const;
	bool SelectLight(const FVector& At, int32 Kind);
	void CaptureColor(UMaterialInstanceDynamic* Mid, TOptional<FLinearColor> On = {});
	void ApplyLight(double T);
	void StartAudio(const FVector& At);
	static double LightLevel(int32 Kind, double T);
	static bool MayBegin(double Clock, bool bProtected, bool bHere);
};
