#pragma once

#include "CoreMinimal.h"
#include "S47Seen.h"

class AActor;
class USpotLightComponent;

// web/src/core/Flashlight.ts. The flashlight (Tom 15 October: a flashlight, no battery). It lies on
// the truck's passenger seat and is taken the first time the player gets in; F switches it. It is
// a real spot light from the player's hand: the only light that goes everywhere, so it is not one
// of an area's fake floods. It stays in the scene with intensity 0 when it is off. On the PC tier
// (Ultra) it casts a shadow, so it does not shine through walls (main.ts passes the tier). Near the
// strange events it may flicker (Flicker()).
//
// The web version's beam is a cheap flashlight's, drawn by its shaders (core/cookies.ts: a hot core
// a little off the middle, the filament's dark spot, a ragged rim, a dim corona) over three's spot
// light with a linear fall-off (decay 1). Unreal's spot light here has the same colour, cone, edge
// and range; its fall-off is the inverse square, so its intensity is the web version's times 4 (the
// two meet 4 m out, where the web version was tuned: "about as bright at 4 m"), and the cookie's
// pattern is not drawn (its gain, FLASH_GAIN, is kept).
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
	void Update(double Dt, double T, const FS47Cam& Cam, bool bOff, bool bShadows = false);

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
	TMap<FString, double> Flicks;
};
