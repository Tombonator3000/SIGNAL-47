#pragma once

#include "CoreMinimal.h"
#include "S47Airlight.h"
#include "S47Backdrop.h"
#include "UObject/Object.h"
#include "S47WorldLife.generated.h"

class US47Main;
class FS47Sky;
class FS47Flora;
class FS47WindowLights;
class FS47Ultra;
class FS47NightLife;

// The world's own life: what the web version's world modules change every frame, stepped in
// main.ts's order (main-orchestration.md step 13: the sky, the PC tier's lamps, the shared lights,
// the fog's colour, the painted horizon, the areas' glows and window lights; and from World.update,
// the light in the air and the life in the night; and before the picture, the ground cover round
// the camera with the wind). Each module is its own file: S47Glow (world/glow.ts), S47Flora and
// S47GroundCover (flora.ts, groundCover.ts), S47Wind, S47Sky, S47Backdrop, S47WindowLight,
// S47Airlight, S47NightLife, S47Ultra (core/ultra.ts). World.ts's own parts of it (airStep,
// lifeStep, shareLights, skyPlaces, groundUnder) and main.ts's (ultraLamps, the fog, the dawn lift,
// the star hole's hour) are here.
UCLASS()
class SIGNAL47_API US47WorldLife : public UObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);
	/** main.ts step(): the world's own life this frame (after the story, the radio and the night's things). */
	void Step(double Dt);
	static bool SelfTest(TArray<FString>& Lines);

	TSharedPtr<FS47Sky> Sky;
	TSharedPtr<FS47Backdrop> Backdrop;
	TArray<TSharedPtr<FS47Flora>> Flora;
	TArray<TSharedPtr<FS47LightCones>> Cones;
	TSharedPtr<FS47GroundHaze> Haze;
	TArray<TSharedPtr<FS47WindowLights>> Windows;
	TSharedPtr<FS47Ultra> Ultra;
	TSharedPtr<FS47NightLife> Life;

	/** What other systems hold of it, set when they are ported: chapter six's sky (game.ch6.skyLift,
	 *  sunElev), SARO dark's adaptation (saroDark.adapt), the morning's held breath (morning.busy). */
	struct FHooks
	{
		TFunction<TOptional<double>()> SkyLift, SunElev;
		TFunction<double()> SaroDarkAdapt;
		TFunction<bool()> MorningBusy;
		TFunction<bool()> LifeQuiet;   // World.lifeQuiet: one of the night's strange things has it
	};
	FHooks Hooks;
	/** Tests: the night without the light in the air (World.airOff). */
	bool bAirOff = false;

	// ---------- World.ts ----------
	/** airStep: the light in the air, every frame (High and Ultra only, at night). */
	void AirStep(double Dt, double T);
	/** shareLights: each area lights its own materials from its own lamps. */
	void ShareLights();
	/** skyPlaces: the lit places whose glow shows on the sky from the area shown, world terms. */
	TArray<FS47SkyPlace> SkyPlaces() const;
	/** groundUnder: the height of the land under a point in the area shown. */
	double GroundUnder(const FVector& P) const;
	/** main.ts ultraLamps: which flood sets the PC tier may turn into real lamps where p is, and
	 *  whether the control room's own lamp shadows are worth drawing. */
	TArray<FString> UltraLamps(const FVector& P, bool& bRoomLamps) const;
	/** The sky's dawn now (sky.uniforms.uDawn). */
	double Dawn() const;

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	TArray<FS47HazeLamp> HazeSaro, HazeRoad;
	int32 TownCones = -1;
	double FlashReady = 0;
	FString ClockTest;
	bool bClockSet = false;
	double MeteorAt = -1, FloraLogT = 0;
	bool bQuality = false;
	FString LastQuality;
	void BuildFlora(bool bBind);
	FVector CamPos() const;
	FVector CamDir() const;
	void TestFlags(double Dt);
};
