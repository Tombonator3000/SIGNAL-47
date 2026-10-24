#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class AActor;
class USpotLightComponent;

// web/src/core/Flashlight.ts. The flashlight has no battery and is taken from the truck's
// passenger seat. F switches it; story events may make its electrical contact flicker. Its warm
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
	/** Every frame: held a little right of and below the eyes, pointing where they look. Off: in the
	 *  truck, in a cinematic, on the title screen (it goes out, and comes back on after). */
	void Update(double Dt, double T, const FS47Cam& Cam, bool bOff);

	/** three's intensity now (the tests): 0 when off. */
	double Intensity() const { return ThreeIntensity; }
	USpotLightComponent* Light() const { return Spot.Get(); }

	static bool SelfTest(TArray<FString>& Lines);

private:
	TWeakObjectPtr<USpotLightComponent> Spot;
	double Level = 0;
	double Flick = 0;
	double Dark = 0;
	double ThreeIntensity = 0;
	double NativeNearGain = 1, NextNearLog = 0;
	bool bNativeNearReady = false, bLogNear = false;
	TMap<FString, double> Flicks;
};
