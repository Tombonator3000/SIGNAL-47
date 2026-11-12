#pragma once

#include "CoreMinimal.h"

class US47Main;
class UNiagaraComponent;
class UStaticMeshComponent;

// GAS-STATION-LIFT-01 (Tom, 11 November: lift the Caliche gas station, Niagara among it): moths and gnats swarming under the lit
// canopy tubes, and a few slow motes of dust drifting through the light when the wind blows. Seen from the forecourt and from the
// road at night it should be noticed without being a show. One CPU Niagara system, NS_S47_Insects (Tools/fx/import_station.py),
// made with S47Fx::Make (so -S47FxOff leaves it out), hung just under the tubes with the canopy's own rotation.
//
// C++ owns when and how many; the system only draws. On only in Freeform, in Caliche (area "diner"), with the camera within FarM, and
// while the tubes are really lit (AS47Props::LitLevel: the town flood's slot 8 has power and the photocell says night): a power cut
// (feeder_fault) or the dawn takes the swarm away at once. How many: the night life's moths by the hour and the cooling
// (FS47NightLife::Activity, S47LifeActivity), fewer in wind and none in a strong one or in rain, half on Low. Weave, not new: when
// the night goes quiet (FS47Soundscape::Hushed, the crickets stopping before something comes, or for a gust) no more come to the
// light, and those there are gone within a few seconds; they come back slowly after.
//
// The web export's moths at the old canopy's two lamps (the road area's lamps 8 and 9, whose glow never follows the power) are
// handed over (FS47NightLife::Cede) once the swarm exists, so the canopy is not swarmed twice; without the asset they stay.
class SIGNAL47_API FS47StationInsects
{
public:
	explicit FS47StationInsects(US47Main* Main) : M(Main) {}

	/** Every frame (game time). Builds the component the first time the canopy is there in Freeform. */
	void Step(double Dt);
	/** Off now (out of Freeform, the title). The component stays for the next time. */
	void Reset();

	/** What the population rule reads. Lit: the tubes, 0 to 1. Moth: S47LifeActivity's moths, 0 to 1. Wind, Rain, Dust: S47Weather's
	 *  live values, 0 to 1. Quiet: the night's hush, eased, 0 to 1. */
	struct FIn { double Lit = 1, Moth = 1, Wind = 0, Rain = 0, Dust = 0, Quiet = 0; bool bLow = false; };
	/** Insects spawned per second (User.Count): MaxRate on a warm, calm, dry night under lit tubes, none dark, by day, in rain, in a
	 *  strong wind or in the hush; half on Low. */
	static double InsectRate(const FIn& In);
	/** Motes spawned per second (User.Motes): only when the wind blows, more with sand in it, none in rain or dark; half on Low. */
	static double MoteRate(const FIn& In);
	static constexpr double MaxRate = 48, MaxMotes = 3, FarM = 120, WindCmS = 120;
	static bool SelfTest(TArray<FString>& Lines);

	bool IsOn() const { return bOn; }

private:
	US47Main* M = nullptr;
	TWeakObjectPtr<UNiagaraComponent> Fx;
	TWeakObjectPtr<UStaticMeshComponent> BuiltFor;
	bool bOn = false, bCeded = false;
	double Quiet = 0;
	int32 Builds = 0;
	void Build(UStaticMeshComponent* Canopy);
	void SetOn(bool bNow, const TCHAR* Why);
};
