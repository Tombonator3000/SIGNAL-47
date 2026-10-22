#pragma once

#include "CoreMinimal.h"
#include <limits>
#include "S47Audio.h"
#include "S47Chapter.h"
#include "S47Drive.h"
#include "S47FlashFrames.h"
#include "S47Glow.h"
#include "S47OldRoad.h"
#include "S47Walker.h"
#include "S47Chapter6.generated.h"

class SS47Ending;
class FS47ThirdFigure;
class UTexture2D;

// story/Chapter6.ts: "Roswell Road". Nothing here is saved: Continue returns to the diner.
UCLASS()
class SIGNAL47_API US47Chapter6 : public US47Chapter
{
	GENERATED_BODY()
public:
	US47Chapter6* Register();
	virtual void Begin(const S47J::FObj& Saved = nullptr) override;
	virtual void Reset() override;
	virtual void Update(double Dt, double T) override;
	virtual FString Objective() const override;
	virtual TArray<FS47Task> Tasks() const override;
	virtual FString SaveBlock() const override { return TEXT("Not on the road."); }
	virtual FString Key() const override { return TEXT("ch6"); }
	virtual void BeginDestroy() override;

	bool bStarted = false;
	FString Stage = TEXT("drive"); // Stage6: drive | event | pullout | end
	TOptional<double> SkyLift, SunElev;
	TSharedPtr<SS47Ending> Ending;
	TSharedPtr<FS47ThirdFigure> Third;
	TOptional<FVector> ThirdAt() const;

	struct FLastMile
	{
		TOptional<double> Cb;
		bool bSkipped = false;
		double D0 = 0, Clock = 0;
		TOptional<double> OpenAt, ClosedAt;
		double Vs = -1;
		bool bApproach = false;
		double Eta = std::numeric_limits<double>::infinity();
	} LastMile;
	struct FTries { int32 Key = 0, Door = 0; double KeyFree = 0, DoorFree = 0; } Tries;
	bool CbDue() const;
	FString ClockShow() const; // UI's seconds clock is a string (hm:ss), empty is null
	bool WantsKey() const;
	bool DoorOffered() const;
	bool TurnKey();
	bool TryDoor();
	UTexture2D* Photo() const;

	// Prologue.ts jumpChapter6 / beginChapter6; chapter five's integrator calls BeginChapter6(nullptr).
	void BeginChapter6(const S47J::FObj& SavedCase = nullptr);
	void JumpChapter6(const FString& At = FString());
	S47J::FObj BeganCase;
	static bool SelfTest(TArray<FString>& Lines);

private:
	double Et = 0, Pt = 0;
	TSet<FString> Done;
	FS47Pressure Bed;
	TOptional<TArray<S47FlashFrames::FFrame>> Frames;
	UPROPERTY() TArray<TObjectPtr<UTexture2D>> FrameTextures;
	bool bStill = false, bCalm = false;
	double Idle = 0, BackT = 0;
	struct FFrom { FVector Pos = FVector::ZeroVector; FQuat Q = FQuat::Identity; } From;
	struct FTruckAt { FVector Pos = FVector::ZeroVector; double Heading = 0; } TruckAt;
	bool bSteady = false;
	TArray<FVector> BandPts;
	FLinearColor HeadNow = FLinearColor::Black;
	TOptional<double> StopS;
	TOptional<TArray<double>> KeyLamps;
	struct FCue { double At; TFunction<void()> Fn; };
	TArray<FCue> Cues;
	FS47GlowView ThirdGlow;
	TArray<int32> ThirdIds;
	bool bThirdShown = false;
	FString JumpAt;
	bool bJumpPlaced = false;
	double TestClock = 0; // a hostless chapter for the self test
	double Clock() const;
	void SetClock(double C);
	FString Time() const;
	void Toast(const FString& Text, float Seconds) const;
	void Note(const FString& Text) const;
	void DrawFrames();
	void DropThird();
	void ThirdGlints(const TArray<FVector>* Pts, const FLinearColor& Col = FLinearColor::Black);
	void Drive(double Dt);
	bool LastMileStep(double Dt, const FS47OldRoad::FWhere& At, double Speed);
	void OpenCB(double Rem);
	void CloseCB();
	void Cue(double At, TFunction<void()> Fn) { Cues.Add({ At, MoveTemp(Fn) }); }
	bool OnWords() const;
	void StartEvent();
	FS47DriveIn Coast(double Dt) const;
	void Once(const FString& K, TFunctionRef<void()> Fn);
	void Event(double Dt);
	void ThirdStep(double E);
	void Flash(double T);
	void StartPullout();
	struct FKeyframes { FVector Front, AtTruck, High, Westward, SunSide, Sunward, Top, Stars; };
	FKeyframes Keyframes() const;
	void Shoot(FS47View& Out);
	void Pullout(double Dt);
	void PlaceJump();
};
