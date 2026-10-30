#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Original SC13 art. Chapter6 supplies all visual time; this never advances the story
// or moves its camera. Missing/compiling art leaves the existing ending available.
class SIGNAL47_API FS47FinaleSky
{
public:
	explicit FS47FinaleSky(US47Main* InMain);
	~FS47FinaleSky();
	bool Prepare();
	void Step(double Mile, const FString& Stage, double EventSeconds, double PullSeconds);
	void Hide();
	bool Ready() const { return bReady; }
	S47J::FObj Readback() const;

private:
	TWeakObjectPtr<US47Main> Main;
	TWeakObjectPtr<UStaticMeshComponent> Craft;
	TWeakObjectPtr<UMaterialInstanceDynamic> CraftFillMids[2];
	TWeakObjectPtr<UStaticMeshComponent> Banks[2];
	TWeakObjectPtr<UMaterialInstanceDynamic> BankMids[2];
	bool bReady = false, bShown = false, bLow = false;
#if WITH_EDITOR
	bool bCloudShaderDiagnosed = false, bFinaleShadersRequested = false;
#endif
	double NextPrepareTime = 0, Phase = -80, Growth = 0, Reveal = 0;
	struct FTuning
	{
		double CraftForward = 1100, CraftRight = 120, CraftHeight = 315, CraftScale = 680;
		double CraftFill = .018;
		double BankForward = 920, BankSpread = 495, BankRevealSpread = 235, BankHeight = 351, Glow = 1, Shade = .82;
		double BankRevealRise = 300;
		double BankDepth = 420, BankWidth = 560, BankThickness = 255, Density = 1;
		int32 Samples = 64;
	} Tuning; // Applied values: metres, uniform mesh scale, and glow multiplier.
	double GlowLevel = .12, GroundShade = 0;
	int32 AppliedSamples = 64;
	FVector Crossing = FVector::ZeroVector;
	FVector ForwardUE = FVector::ForwardVector, RightUE = FVector::RightVector;
	FString Failure, LastStage;
	int32 LastPhaseBand = -1;
	bool WaitFor(const FString& Reason);
	void Drop();
};
