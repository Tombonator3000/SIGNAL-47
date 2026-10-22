#pragma once

#include "CoreMinimal.h"
#include "S47Docs.h"
#include "S47Obj.h"
#include "S47Seen.h"
#include "S47TvProgramme.h"
#include "UObject/StrongObjectPtr.h"

class US47Main;
class UTexture2D;
class UTextureRenderTarget2D;
class ASceneCapture2D;
class UPointLightComponent;
class UMaterialInstanceDynamic;
class FS47GainNode;

// story/motelTv.ts: the office television's 47 seconds of carrier and actual view of tonight's lot.
struct FS47MotelTvIn
{
	bool bOk = false, bSaro = false, bHere = false, bTruckOnLot = false;
	FS47Cam Cam;
	double Clock = 0;
	TOptional<bool> May;
	double Unrest = 0;
};
struct FS47MotelTvDeps
{
	TFunction<FS47Obj()> Truck;
	TFunction<void(const FS47Doc&)> File;
	TFunction<TOptional<FS47Doc>()> Document;
	TFunction<void(const FS47Doc&)> Open;
	TFunction<void(const FString&)> Note;
	TFunction<void(const FString&, double)> Toast;
	TFunction<void()> Save;
};

class SIGNAL47_API FS47MotelTv
{
public:
	FS47MotelTv(US47Main* InM, FS47MotelTvDeps InD);
	~FS47MotelTv();
	FString Mode = TEXT("card");
	/** The actual lot picture, as a JPEG data URL (canvas.toDataURL). */
	FString FrameUrl;
	bool Busy() const { return Mode != TEXT("card"); }
	bool bWaiting = false;
	int32 Slips = 0;
	bool Heard() const { return Busy() && Dist < 30; }
	void Update(double Dt, const FS47MotelTvIn& S);
	void Silence();
	void Reset();
	void Refile(const FString& Url = FString());
	void Now();
	static bool SelfTest(TArray<FString>& Lines);
private:
	US47Main* M = nullptr;
	FS47MotelTvDeps D;
	double T = 0, Dwell = 0, NotBefore = 0, PaintT = 0;
	TArray<FColor> Card, Shot;
	FS47TvProgramme Programme;
	double ProgrammeTime = 0;
	FVector TvAt = FVector::ZeroVector;
	double FrameClock = 0;
	bool bCanWitness = false;
	struct FSnd { TSharedPtr<FS47GainNode> Tone, Hum; double HumK = 1; TSharedPtr<FS47GainNode> Hiss, Whine; TFunction<void()> Stop; };
	TOptional<FSnd> Snd;
	FLinearColor Light;
	TWeakObjectPtr<ASceneCapture2D> Capture;
	TWeakObjectPtr<UPointLightComponent> Top;
	TStrongObjectPtr<UTextureRenderTarget2D> Target;
	TStrongObjectPtr<UTexture2D> Tube;
	TWeakObjectPtr<UMaterialInstanceDynamic> ScreenMat;
	bool bWarmed = false;
	double Unrest = 0;
	struct FRoll { double Y = 0, V = 0; };
	TOptional<FRoll> Roll;
	double Dist = TNumericLimits<double>::Max();
	FS47Obj Tv;
	FVector CaptureAt = FVector::ZeroVector;

	double Last() const;
	void Carrier();
	void Missed(double Clock);
	void ShowFrame(const FS47MotelTvIn& S, double InDist);
	void Witness(const FS47MotelTvIn& S, double InDist);
	bool InView(const FS47Cam& Cam) const;
	void Aim();
	void Warm();
	bool Shoot();
	void Paint(double InT);
	void Sound(double InDist);
	void Click();
	void BindScreen();
	bool Seen(const FString& Key) const;
	void Toast(const FString& Text, double Secs) const;
	static FS47Doc FrameDocument(const FString& Image, const FString& Time);
	static TArray<FColor> TestCard();
};
