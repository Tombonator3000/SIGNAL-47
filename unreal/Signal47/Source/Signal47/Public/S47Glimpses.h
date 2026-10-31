#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47Seen.h"
#include "S47TripoCreature.h"
#include "S47RoswellGroup.h"

class AS47Web;
class US47Main;

// web/src/core/glimpses.ts. The original distant silhouette remains the asset-missing fallback.
// Tom, 31 October: the actual small Roswell body now peeks at nearby windows/corners,
// takes a moment to notice the observer, then ducks or withdraws behind the real cover.
// The quiet opening, encounter guards and maximum eight appearances are preserved.
// Historical fallback: Them, in the background (HENDELSER.md, section 7; Tom, 16 October:
// always there, something that could happen at any time). One figure, about 1.2 m, black with a
// faint rim, stands still somewhere 40 to 300 m off, at the edge of the view. It is there only
// while it is more than 25 degrees from the middle of the view: looked at, zoomed at, lit by the
// flashlight, or come closer than 40 m, and it was not there. No effect, no sound. It never moves
// and never comes nearer (the rules in HENDELSER.md, section 4).
//
// Where: behind the far antennas at SARO, out in the dark round Caliche and the diner, and on the
// highway's shoulder ahead of the truck at the far end of the headlights. When: now and then,
// rarely at first and more often towards morning, at most eight in a night. main.ts decides when
// it may run (not in the prologue, not in a cinematic, not once THE EVENT has the night).
//
// The figure is the web version's Silhouette('glimpse') (core/roadside.ts): a group under the scene
// named 'glimpse' with the body and its rim (the shell, drawn from behind) as built in code. The
// export has it as it was built, hidden; this moves it, turns it and colours its rim. (Round 14's
// painted outlines, which the web version swaps in once their pictures have loaded, are not in the
// export: the figure here is always the one in code, and its poses look the same.)

struct FS47GlimpsesIn
{
	bool bOk = false;
	bool bHeld = false;
	/** 'saro' | 'road' | empty (null). */
	FString Scene;
	FS47Cam Cam;
	double Clock = 0;
	/** The ground's height under a point of this scene. */
	TFunction<double(double, double)> Ground;
	/** Driving the highway: the truck's position and its heading (for the spot ahead). */
	struct FTruck { FVector Pos = FVector::ZeroVector, Dir = FVector::ZeroVector; };
	TOptional<FTruck> Truck;
	bool bFlashlight = false;
	double Zoom = 0;
	/** The night's curve lets one stand now (its peak); unset, it may. */
	TOptional<bool> May;
	/** How much sooner one comes than its own pace (the curve's peak). */
	TOptional<double> Pace;
};

class SIGNAL47_API FS47Glimpses
{
public:
	explicit FS47Glimpses(AS47Web* InWeb);

	/** Where it stands now ('S-11', 'town-south', 'shoulder' ...), empty when it does not. */
	FString Up;
	int32 Count = 0;

	/** Where its head is while it stands somewhere (story/seen.ts), or unset. */
	TOptional<FVector> Where() const;
	/** The figure (the album's frames find it by its name, as the web version's scene does). */
	FS47Obj Figure() const { return Fig; }
	FVector Position() const { return FigPos; }
	bool Visible() const { return bVisible; }
	bool HasNativeBody() const { return bRoswell; }

	void Attach(US47Main* Main);
	int32 DevSpawn(int32 Number, FS47RoswellBrain::EMode Mode, bool bRoof=false);
	void DevMode(FS47RoswellBrain::EMode Mode);
	void DevClear();
	void DevStep(double Dt,const FS47GlimpsesIn& Input);
	TArray<FS47RoswellGroup::FView> Individuals() const;
	void Update(double Dt, const FS47GlimpsesIn& S);
	/** Put it at a named place now if it can stand there (the developer menu, the tests). */
	bool Now(const FS47GlimpsesIn& S, const FString& Id);
	/** A new night, the title, a save opened. */
	void Reset();

	static bool SelfTest(TArray<FString>& Lines);

private:
	void SyncGroup();
	TSharedPtr<FS47RoswellGroup> Group;
	friend struct FS47RoswellPreviewDriver;
	friend struct FS47RoswellPoseDriver;
	bool bPreviewDriver = false;
	TWeakObjectPtr<AS47Web> Web;
	FS47Obj Fig;
	FVector FigPos = FVector::ZeroVector;
	bool bVisible = false;
	bool bRoswell = false;
	TWeakObjectPtr<USceneComponent> BodyRoot;
	FS47TripoCreature Body;
	FString ActiveScene;
	FVector BasePos = FVector::ZeroVector, Cover = FVector::ZeroVector;
	double Facing = 0, Noticed = 0, Withdraw = -1, ExposureNow = 1;
	bool bWindow = false;
	double Wait = 0;
	double T = 0;

	struct FSight { double Off = 0, Dist = 0; bool bInView = false; };
	static FSight Sight(const FS47Cam& Cam, const FVector& At, double HeadY = .6);
	struct FAt
	{
		FString Id; FVector P = FVector::ZeroVector; uint32 Rim = 0;
		FVector Cover = FVector::ZeroVector; bool bWindow = false; FString Scene;
	};
	TOptional<FAt> Pick(const FS47GlimpsesIn& S, const FString& Only = FString()) const;
	bool ClearLine(const FVector& Eye, const FVector& Head, FString* BlockedBy = nullptr) const;
	void UpdateRoswell(double Dt, const FS47GlimpsesIn& S);
	void DrawRoswell(double Exposure, double Clock);
	void Place(const FAt& At, const FVector& Cam);
	void Stand(const FAt& At, const FVector& Cam);
	void SetVisible(bool bOn);
	void Hide();
};
