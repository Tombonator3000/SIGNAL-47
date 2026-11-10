#pragma once

#include "CoreMinimal.h"
#include "S47Drive.h"
#include "S47Obj.h"

class US47Main;
class UMaterialInstanceDynamic;
class UMaterialBillboardComponent;

// drive/herd.ts: the Herefords. Authored body/head meshes reuse the original instance pools;
// frost, eye sprites, grazing clock, headlight beam and missing cow remain in step.
namespace S47Herd
{
	extern SIGNAL47_API const TCHAR* TAG_RANCH;
	SIGNAL47_API const TArray<FString>& Tags();
	inline constexpr int32 MISSING = 4;
	inline constexpr double COW_R = 1.15;
	// ANIMALS-20261108: an alarm below this is unease, not fright: the herd walks off together, all the same way, heads up. At it
	// and above they run, each its own way.
	inline constexpr double SCATTER = 0.5;
	inline const FVector NECK(0, 1.12, -0.74);
	inline const FVector EYE[2] = { FVector(-0.17, 0.11, -0.66), FVector(0.17, 0.11, -0.66) };
}

// The first and third pools of the area's headlight flood set, in three.js world terms.
struct FS47HerdHeadlights
{
	FVector4 F0 = FVector4(0, 0, 0, 0), F2 = FVector4(0, 0, 0, 0);
	double ColorR = 1;
};

class SIGNAL47_API FS47Herd
{
public:
	FS47Herd(US47Main* InM, const FString& InPath, const FString& InArea);
	FS47Obj Group;
	FString Area;
	bool bStill = false;
	int32 Missing = -1;
	TArray<FS47Obj> LookAreas;
	void Set(bool bInStill, int32 InMissing, bool bRing);
 bool ReconcileMode();
 FVector AppliedOffset() const { return ModeOffset; }
	FS47Obstacle Obstacle(const FVector2D& Origin);
	void Update(double T, const TOptional<FS47HerdHeadlights>& Headlights = {});
	bool CowAt(int32 I, FVector& Out) const;
	double LitAt(int32 I) const;
	int32 Count() const { return 9; }
	FVector Middle() const;
	// Something near that animals can feel: 0 calm, 1 run from this place.
	void SetAlarm(double K, const FVector& FromWorld);
	double AlarmLevel() const { return Alarm; }
	/** Uneasy: all walking the same way (S47Herd::SCATTER). The way, flat, herd-local, while it lasts (tests). */
	bool Together() const { return Alarm > .05 && Alarm < S47Herd::SCATTER; }
	FVector TogetherWay() const { return Way; }
	// One cow (or -1 for none) stands quite still, head up, facing this place, instead of grazing or wandering.
	void SetStare(int32 Cow, const FVector& TowardWorld);
	int32 Starer() const { return Stare; }
	// Freeform: one cow away for now (S47Herd::MISSING, the one with the machinery for it), -1 for all nine. Story's own
	// count is never touched, and without this call Freeform's herd is whole, as it always was.
	void SetAway(int32 Cow);
	// PHOTO-02 (N1110): one cow (by her stable index, S47Herd::MISSING) held up in the air for a lift: Height metres above the grass, Spin
	// radians about her own axis. -1 or 0 puts her down exactly where she stood. Not saved here: S47CowLift owns the pose and its save.
	void SetLift(int32 Cow, double Height, double Spin);
	double LiftHeight() const { return LiftCow >= 0 ? LiftH : 0; }
	// Where she stands on the grass, in the herd's world terms (Y is the ground), for the craft and the beam above her.
	FVector LiftGround() const;
	static bool SelfTest(TArray<FString>& Lines);

private:
	int32 LiftCow = -1;
	double LiftH = 0, LiftSpin = 0;
	US47Main* M = nullptr;
	FS47Obj Bodies, Heads, Ring;
	FVector4 BeamPos = FVector4(0, -999, 0, 0);
	FVector BeamDir = FVector(0, 0, -1);
	TArray<FVector> EyeAt;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Eyes;
 TArray<TWeakObjectPtr<UMaterialBillboardComponent>> EyeSprites;
 TArray<FVector> LookBases;
 FVector GroupBase=FVector::ZeroVector,RingBase=FVector::ZeroVector,ModeOffset=FVector::ZeroVector;
 bool bModeKnown=false,bStoryStill=false,bStoryRing=false;
 int32 StoryMissing=-1,Away=-1;
 void MoveObstacle(const FVector& Old,const FVector& New,double Radius);
	bool bEyesLit = false;
	bool bNativeModels = false;
	TArray<double> Ys;
	bool bDirty = true;
	double LastT = 0;
	// Freeform: where each has wandered to, what fear has added to that, and which way it faces.
	double Alarm = 0;
	FVector AlarmFrom = FVector::ZeroVector; // herd-local
	FVector Way = FVector(0, 0, 1);         // herd-local: the one way they all walk while uneasy
	double StepDt = 0;
	int32 Stare = -1;
	FVector StareAt = FVector::ZeroVector, StareHold = FVector::ZeroVector; // herd-local
	TArray<double> Yaws;
	TArray<FVector> ColliderAt; // where its collider and handle stand, which follows it in steps
	void FollowColliders();
	void ClearLife();
	TArray<FVector> Fled;
	TArray<FVector> Offsets;
	FVector WanderAt(int32 I, double T, bool& bWalking, FVector& Heading) const;
	FVector BodyP, HeadP;
	FQuat BodyQ, HeadQ;
	void Pose(int32 I, double T);
	void Place(double T);
	double BeamAt(const FVector& P) const;
	void UpdateObstacles();
};
