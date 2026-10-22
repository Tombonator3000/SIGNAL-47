#pragma once

#include "CoreMinimal.h"

class AS47Web;
class FJsonObject;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

// world/airlight.ts: light in the air, faint cones of sodium light under the lamps and a low haze
// lying on the open ground at night (High and Ultra only). The cones of an area are one mesh, drawn
// additively; the haze is two big flat layers that follow the camera. Their shaders are ported as
// M_WebAirCone and M_WebHaze (import_web.py); the meshes and the lamps (each read off its glow
// point, so a lamp that goes out takes its cone with it) are the export's (export.js lifeOf: air).

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
	TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
	TWeakObjectPtr<UTexture2D> KTex; // uK: the lamps' strengths, a row
	TArray<float> K;
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
	TWeakObjectPtr<AS47Web> Web;
	int32 Node = -1;
	TWeakObjectPtr<UStaticMeshComponent> Mesh;
	TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
	double Base = 0;
	bool bBase = false; // (base NaN: none yet)
};
