#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class AS47Web;
class US47Main;
class FJsonObject;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;
struct FS47GlowPoints;

// world/nightlife.ts: life in the night. Moths round the sodium lamps and the porch lights (and a
// few resting on the wall by a porch light), bats at the lamps the player walks under in the early
// night, dust in the flashlight's beam and in the headlights on the survey track, lights far out on
// the land (a car on a county road now and then, ranch and trailer windows that go out and come on, a
// siding's lamps), tumbleweeds caught where the wind left them, and now and then one across the
// highway or the lot in a gust. None of it is the story's. Its own random numbers, so the rest of the
// night goes the same with it or without it.
//
// The moths and bats are one cloud per area that moves in its vertex shader, the dust one cloud per
// beam: here instanced quads with those shaders ported (M_WebMoths, M_WebMotes, import_web.py), each
// copy's attributes as its custom data (the export's: export.js lifeOf). The far lights are a glow
// points node of the export's (S47Glow), the tumbleweeds the export's meshes.

/** activity(clock, dawn): how much is about at this hour: the moths, the bats, the hour. */
struct FS47LifeActivity { double Moth = 1, Bat = 0, Hour = 2; };
SIGNAL47_API FS47LifeActivity S47LifeActivity(double Clock, double Dawn);

class SIGNAL47_API FS47NightLife
{
public:
	/** Tests: the night without its life. */
	bool bOff = false;
	void Build(US47Main* M);
	/** update(dt, t, o) (World.lifeStep): the camera, the sky's dawn, and quiet: one of the night's
	 *  strange things has it, or a cinematic (World.lifeQuiet). */
	void Update(double Dt, double InT, const FS47Cam& Cam, double Dawn, bool bQuiet);
	/** carNow(near): a car on the county road coming past (the tests, the developer menu). */
	bool CarNow(bool bNear = true);
	/** tumbleNow(anywhere): a tumbleweed now, crossing in front of the camera or the highway ahead of the truck. */
	bool TumbleNow(bool bAnywhere = false);
	/** What is drawn now (tests): moths, bats, far lights' car, a tumbleweed rolling, the dust. */
	FString Stats() const;
	/** The lamps' strengths as the moths' material reads them (uK): a row of floats, made with them. */
	static UTexture2D* LampRow(const TArray<float>& K);
	/** The lamps' row, the hour's activity (-S47SelfTest). */
	static bool SelfTest(TArray<FString>& Lines);

private:
	struct FLamp { FVector P, W; FLinearColor Col; bool bBig = false; double H = 0; int32 GlowNode = -1, GlowIndex = -1; double K0 = 1; };
	struct FMoths
	{
		int32 Node = -1;
		TArray<FLamp> Lamps;
		int32 Low = 0, Moths = 0, Bats = 0, Resting = 0;
		TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
		TWeakObjectPtr<UTexture2D> KTex;
		TArray<float> K;
		FBox Reach = FBox(ForceInit);
		int32 DrawnMoths = 0, DrawnBats = 0;
	};
	struct FWin { int32 I = -1; double Ph = 0; FVector P; double X = 0, Z = 0, Y = 0, Size = 1; FLinearColor Col; double On0 = 0, On1 = 0, K = 1; bool bTv = false; };
	struct FFar { int32 Node = -1; TArray<FWin> Win; int32 Car[3] = { -1, -1, -1 }; bool bGone = false; };
	struct FWeed
	{
		int32 Node = -1, BaseMat = -1;
		TArray<int32> FloodMats;
		bool bOn = false;
		FVector From = FVector::ZeroVector, Dir = FVector::ZeroVector, Axis = FVector::ZeroVector;
		double S = 0, Len = 0, Top = 5, Rest = 0, Spin = 0;
		struct FHop { double H = 0, Phase = 0, Next = 3; } Hop;
		FQuat Q = FQuat::Identity;
		FVector At = FVector::ZeroVector;
	};
	struct FResting { int32 Node = -1; FVector Centre; double Radius = 0; };
	struct FArea
	{
		FString Id;
		TOptional<FMoths> Moths;
		TOptional<FFar> Far;
		TOptional<FWeed> Weed;
		TArray<FResting> Resting;
		TArray<FVector> LampsW;
	};
	TArray<FArea> Areas;
	FArea* Find(const FString& Id);

	// the dust in a beam: its cloud (a quad each) and its own values
	struct FMotes
	{
		TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
		int32 N = 0;
		FVector Box = FVector::OneVector, Carried = FVector::ZeroVector;
		void Drift(double Dt, const FVector& V);
	};
	FMotes Flash, Head;

	TWeakObjectPtr<AS47Web> Web;
	US47Main* M = nullptr;
	FS47LifeActivity Act;
	double Specks = 0, Cloud = 0, LastSpeed = 0;
	double WeedT = 0, CarT = 0;
	struct FCar { double Z = 0, Dir = 1, End = 0, Speed = 20; };
	TOptional<FCar> Car;
	FString WeedArea, Here;
	double T = 0;
	FS47Cam Last;
	bool bHaveLast = false;
	double Lines = 720;

	// the life's own random numbers (core/textures.ts rng, seeded from the clock)
	uint32 Seed = 0;
	double Rand();
	double Rnd(double A, double B) { return A + Rand() * (B - A); }

	void MothsUpdate(FMoths& Mo, double InT, bool bOn, bool bLow, const FVector& Cam);
	void FarUpdate(FFar& F, const FString& Area, bool bOn, double Hour, double Night, const FVector& Cam);
	void CarStep(double Dt, bool bQuiet, bool bCan);
	double CarK() const;
	void WeedStep(double Dt, const FS47Cam& Cam, bool bQuiet, bool bCan, bool bFoot, bool bOut);
	void WeedUpdate(FWeed& W, const FString& Area, double Dt, bool bSeen);
	void WeedPlace(FWeed& W, const FString& Area);
	void WeedStop(FWeed& W);
	void Beams(double Dt, const FS47Cam& Cam, bool bOn, bool bLow, bool bOut);
	bool Blocked(const FString& Area, double X, double Z) const;
	bool OnHighway(const FString& Area, double X, double Z) const;
	double Ground(const FString& Area, double X, double Z) const;
	bool MotesBuild(FMotes& Mo, int32 N, uint32 InSeed, const FVector& Box, bool bBottom);
};
