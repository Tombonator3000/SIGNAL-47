#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class AActor;
class USpotLightComponent;
class UTextureLightProfile;

// web/src/core/Flashlight.ts. In the Story the flashlight has no battery; in Freeform it has one (bDrains,
// set by the night layer): it runs down while lit, is filled again at SARO and by a dry cell from the store.
// It is taken from the truck's passenger seat. F switches it; story events may make its electrical contact flicker. Its warm
// reflector beam is a movable Unreal spot with a finite source, shadows and volumetric scattering
// on every quality tier. The light is kept at zero intensity when off. The story-facing intensity
// retains its original range so saves and deterministic behavioural tests remain compatible.
// The native spot alone compensates close surfaces for the fixed-exposure picture.
class SIGNAL47_API FS47Flashlight
{
public:
	/** The light, in the scene from the start (Owner: the actor that holds it, the web world). */
	explicit FS47Flashlight(AActor* Owner);

	/** Taken off the truck's seat (saved with the case). */
	bool bHave = false;
	bool bOn = false;

	/** F (and on a phone the Light button). False when there is none yet. */
	bool Toggle();
	/** How much it flickers, 0 to 1, near the strange events. Each event says so for itself (From: the
	 *  sounds out in the dark and the silence, a figure at the edge of the view, the town's events,
	 *  the time away, the curve's sign), and the strongest of them counts. */
	void Flicker(double K, const FString& From = TEXT("event"));
	/** While it flickers (Flicker above zero): the contact gives way altogether for this long. No battery in it. */
	void FlickerOut(double Seconds) { Dark = FMath::Max(Dark, Seconds); }
	/** GEAR-FLICKER-20261108: the share of the beam lost near something (0 to 1), by source as Flicker, the strongest counting.
	 *  A steady weakening that comes and goes with distance; it spends no battery (FS47GearFlicker). */
	void Sag(double Share, const FString& From = TEXT("event"));
	double SagNow() const { return SagK; }
	double FlickerNow() const { return Flick; }
	/** Every frame: held a little right of and below the eyes, pointing where they look. Off: in the
	 *  truck, in a cinematic, on the title screen (it goes out, and comes back on after). bHeld: the game
	 *  is held (the pause menu, a panel open): the lamp stays as it is, lit or not, and its battery is not used. */
	void Update(double Dt, double T, const FS47Cam& Cam, bool bOff, bool bHeld = false);

	// ---------- the battery (Freeform only; Unreal addition, HUD-20261108) ----------
	/** Freeform: the lamp runs down. Story: it never does (the web version's lamp has no battery). */
	bool bDrains = false;
	/** Lit time from full to empty, and from empty to full when charged at SARO (seconds). M7 (C): about half a night lit (the
	 *  night is FS47FreeformSession's 2700 s), and a quarter of an hour on the charger, which needs station power (S47Night). */
	static constexpr double RunSeconds = 1350, ChargeSeconds = 900;
	/** Below this the beam is weak and the HUD shows the last stroke. */
	static constexpr double LowLevel = 0.2;
	/** 0 (empty) to 1 (full). */
	double BatteryLevel() const { return Battery; }
	bool Dead() const { return bDrains && Battery <= 0; }
	bool Low() const { return bDrains && Battery < LowLevel; }
	/** A fresh dry cell. */
	void Refill() { Battery = 1; bSaidLow = bSaidDead = false; }
	/** SARO's charger: K is a share of a full charge. */
	void Charge(double K) { Battery = FMath::Clamp(Battery + K, 0.0, 1.0); if (Battery >= LowLevel) bSaidLow = false; if (Battery > 0) bSaidDead = false; }
	void SetBattery(double Full) { Battery = FMath::Clamp(Full, 0.0, 1.0); bSaidLow = Battery < LowLevel; bSaidDead = Battery <= 0; }
	/** A weak cell (Freeform, M2): now and then the bulb drops out for a moment, oftener the lower it is, like a loose contact or
	 *  something near (FS47GearFlicker). Times a second; 0 for a good cell. */
	static double CellDropRate(double Full);
	/** How many times the lit lamp has gone out for a moment, for whatever reason (a contact, a weak cell, something near). */
	int32 CutOuts() const { return Cuts; }
	/** 0 nothing new, 1 it has just grown weak, 2 it has just gone out: each once (for the caption). */
	int32 TakeNotice();
	/** The beam's share of full for this battery level: 1 down to LowLevel, then falling to a faint glow. */
	static double BatteryGain(double Full);

	/** three's intensity now (the tests): 0 when off. */
	double Intensity() const { return ThreeIntensity; }
	USpotLightComponent* Light() const { return Spot.Get(); }

	static bool SelfTest(TArray<FString>& Lines);

private:
	/** The beam's shape from the s47.flash.* settings (profile, cone, reach), set again only when they change. */
	void ApplyBeam();
	TWeakObjectPtr<USpotLightComponent> Spot;
	TWeakObjectPtr<UTextureLightProfile> Profile;   // held by the light while it is used
	bool bBeamProfile = false;
	float BeamCone = 0, BeamRange = 0;
	double Level = 0;
	double Flick = 0;
	double Dark = 0;
	double ThreeIntensity = 0;
	double NativeNearGain = 1, NextNearLog = 0;
	bool bNativeNearReady = false, bLogNear = false;
	TMap<FString, double> Flicks, Sags;
	double SagK = 0;
	double Battery = 1, CellDark = 0;
	int32 Cuts = 0;
	bool bSaidLow = false, bSaidDead = false;
};
