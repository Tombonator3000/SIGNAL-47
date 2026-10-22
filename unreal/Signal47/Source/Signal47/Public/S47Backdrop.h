#pragma once

#include "CoreMinimal.h"

class FS47Sky;

// world/backdrop.ts: the painted horizon, one band all the way round on a ring in the sky group: the
// night picture and the dawn picture in one shader (M_WebSkyBand, its fragment shader ported),
// crossfaded with the sky's own dawn, the area's air over it, and the warm glow of SARO, Caliche and
// the motel over the skyline where those places are from the camera (Sky.uGlow, set here). What
// changes every frame is set here, as Backdrop.update does; the pictures are the export's.

/** A lit place on the map: world position, how far across its lights are, how bright (World.skyPlaces). */
struct FS47SkyPlace { double X = 0, Z = 0, R = 1, K = 0; };

namespace S47Backdrop
{
	/** Elevations (degrees) at the top and the bottom of the band's pictures; the horizon is at 0. */
	inline constexpr double EL_TOP = 8.4375, EL_BOT = -2.8125;
	/** Places whose lights glow on the sky (Sky.uGlow). */
	inline constexpr int32 GLOWS = 3;
	/** The mesa the sun comes up behind (its middle, half width, top over the band's horizon). */
	struct FSunMesa { double Az, Half, Top; };
	inline constexpr FSunMesa SUN_MESA = { 81, 9, 0.6 };
	/** horizonDip(h): how far (degrees) the horizon dips under a camera h metres over the land. */
	SIGNAL47_API double HorizonDip(double H);
	/** horizonEdge: the colour of the land where it ends as seen from here (linear) and how much fog
	 *  is on it; the old road's far land takes it too (OldRoad.ts). */
	struct FEdge { FLinearColor Land = FLinearColor::Black; double Fog = 1; };
	SIGNAL47_API FEdge& HorizonEdge();
}

class SIGNAL47_API FS47Backdrop
{
public:
	/** Tests: keep it hidden. */
	bool bHold = false;
	// the uniforms of its own (the rest are the sky's)
	double Gain = 1, Rim = 0, BodyK = 0.2, Dip = 0;
	FLinearColor Fog = FLinearColor::Black, Mesa = FLinearColor::Black;

	/** update(fog, density, oldRoad, h, places, cam, yaw): every frame, the area's air, how high the
	 *  camera is over the land (h, m), and the places that glow (world terms) seen from cam; yaw is
	 *  how the sky is turned in this area (World.skyYaw). Writes the sky's uGlow too. */
	void Update(FS47Sky& Sky, const FLinearColor& InFog, double Density, bool bOldRoad, double H, const TArray<FS47SkyPlace>& Places, const FVector& Cam, double Yaw);
};
