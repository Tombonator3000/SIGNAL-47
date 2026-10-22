#pragma once

#include "CoreMinimal.h"

class AS47Web;
class FJsonObject;
class UStaticMeshComponent;

// Legacy airlight metadata remains available to WorldLife. Native volumetric fog now owns
// scattering; exported additive cone meshes and flat haze sheets stay hidden on every tier.

/** A lamp that tints the haze: world place, how wide its pool, its colour, how lit (World.ts HazeLamp). */
struct FS47HazeLamp { FVector P = FVector::ZeroVector; double R = 7; FLinearColor Col = FLinearColor::Black; double K = 1; };

/** LightCones: the cones under an area's lamps, one mesh. */
class SIGNAL47_API FS47LightCones
{
public:
	struct FLamp { FVector P = FVector::ZeroVector; double Ground = 0, R = 0; FLinearColor Col = FLinearColor::White; int32 GlowNode = -1, GlowIndex = -1; double K0 = 1; };
	TArray<FLamp> Lamps;
	bool Build(AS47Web* W, const TSharedPtr<FJsonObject>& J);
	/** L.k(): the lamp's glow point's red over its red as built (Town.ts darkens a lamp's glow when it goes off). */
	double LampK(int32 I) const;
	/** update(on, gain) */
	void Update(bool bOn, double Gain);
	bool Visible() const;

private:
	TWeakObjectPtr<AS47Web> Web;
	TWeakObjectPtr<UStaticMeshComponent> Mesh;
};

/** GroundHaze: two flat layers of haze a little above the open ground round the camera. */
class SIGNAL47_API FS47GroundHaze
{
public:
	static constexpr double R = 320;
	bool Build(AS47Web* W, int32 Node);
	/** update(dt, t, cam, ground, k, fog, lamps): ground the open land's height under the camera, or
	 *  unset for none here (indoors, another area, day); lamps those that colour it, world terms. */
	void Update(double Dt, double T, const FVector& Cam, TOptional<double> Ground, double K, const FLinearColor& Fog, const TArray<FS47HazeLamp>& Lamps);
	bool Visible() const;

private:
	TWeakObjectPtr<UStaticMeshComponent> Mesh;
};
