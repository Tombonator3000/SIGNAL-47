#pragma once

#include "CoreMinimal.h"
#include "S47Drive.h"
#include "S47Obj.h"

class US47Main;
class UMaterialInstanceDynamic;

// drive/herd.ts: the exported Herefords. Geometry, frost and eye sprites come from the web scene;
// this keeps their instanced pose, grazing clock, headlight beam, glints and missing cow in step.
namespace S47Herd
{
	extern SIGNAL47_API const TCHAR* TAG_RANCH;
	SIGNAL47_API const TArray<FString>& Tags();
	inline constexpr int32 MISSING = 4;
	inline constexpr double COW_R = 1.15;
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
	FS47Obstacle Obstacle(const FVector2D& Origin);
	void Update(double T, const TOptional<FS47HerdHeadlights>& Headlights = {});
	bool CowAt(int32 I, FVector& Out) const;
	double LitAt(int32 I) const;
	int32 Count() const { return 9; }
	FVector Middle() const;
	static bool SelfTest(TArray<FString>& Lines);

private:
	US47Main* M = nullptr;
	FS47Obj Bodies, Heads, Ring;
	FVector4 BeamPos = FVector4(0, -999, 0, 0);
	FVector BeamDir = FVector(0, 0, -1);
	TArray<FVector> EyeAt;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Eyes;
	bool bEyesLit = false;
	TArray<double> Ys;
	bool bDirty = true;
	double LastT = 0;
	FVector BodyP, HeadP;
	FQuat BodyQ, HeadQ;
	void Pose(int32 I, double T);
	void Place(double T);
	double BeamAt(const FVector& P) const;
	void UpdateObstacles();
};
