#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "UObject/StrongObjectPtr.h"

class US47Main;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;

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
	struct FGroundMaterial
	{
		TWeakObjectPtr<UStaticMeshComponent> Component;
		TStrongObjectPtr<UMaterialInterface> Original;
		TStrongObjectPtr<UMaterialInstanceDynamic> Replacement;
	};
	TArray<FGroundMaterial> GroundMaterials;
	bool bGroundMaterialLookupTried = false;
	bool bReady = false, bShown = false, bLow = false;
#if WITH_EDITOR
	bool bCloudShaderDiagnosed = false, bFinaleShadersRequested = false;
#endif
	double NextPrepareTime = 0, Phase = -80, Growth = 0, Reveal = 0;
	struct FTuning
	{
		double CraftForward = 2100, CraftRight = 100, CraftHeight = 680, CraftScale = 1000;
		double CraftFill = .065;
		double BankForward = 1100, BankSpread = 280, BankRevealSpread = 520, BankHeight = 300, Glow = 1.1, Shade = .6;
		double BankRevealRise = 450;
		double BankDepth = 950, BankWidth = 1200, BankThickness = 1000, Density = 1.7;
		int32 Samples = 96;
	} Tuning; // Applied values: metres, uniform mesh scale, and glow multiplier.
	double GlowLevel = .12, GroundShade = 0;
	int32 AppliedSamples = 96;
	FVector Crossing = FVector::ZeroVector;
	FVector ForwardUE = FVector::ForwardVector, RightUE = FVector::RightVector;
	FString Failure, LastStage;
	int32 LastPhaseBand = -1;
	bool WaitFor(const FString& Reason);
	void SetGroundMaterialOverride(bool bActive);
	void Drop();
};
