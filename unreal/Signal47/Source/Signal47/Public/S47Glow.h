#pragma once

#include "CoreMinimal.h"

class UMaterialBillboardComponent;
class UMaterialInstanceDynamic;

// world/glow.ts GlowPoints: many glowing lamps, each a soft additive disc that keeps its size in
// metres and can blink with its own phase. AS47Web makes a billboard for each exported point (its
// own copy of M_WebGlow: Color, Brightness); this is the GlowPoints the TypeScript holds, over
// them: the points' places, colours, sizes and blinks (aBlink, export.js), uTime, uHold, uLevel.
// The shader's blink is done here, on the billboards' Brightness: aCol * mix(b, uLevel, uHold).
struct SIGNAL47_API FS47GlowPoints
{
	int32 Node = -1;
	TArray<TWeakObjectPtr<UMaterialBillboardComponent>> Bill;
	TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Mid;
	TArray<FVector> Pos;       // position, in the points' own terms
	TArray<FLinearColor> Col;  // aCol
	TArray<float> Size;        // aSize, the glow's diameter in metres
	TArray<FVector2D> Blink;   // aBlink: rate (0 steady), phase
	double Time = 0;           // uTime
	float Hold = 0, Level = 1; // uHold, uLevel

	int32 Num() const { return Pos.Num(); }
	/** setPosition(i, p) */
	void SetPosition(int32 I, const FVector& P);
	/** setPhase(i, phase): where in its blink a point is (radians); its rate stays */
	void SetPhase(int32 I, double Phase);
	/** the aCol attribute's setXYZ (FarLights, Town's lamps going out) */
	void SetColor(int32 I, const FLinearColor& C);
	/** the aSize attribute's setX */
	void SetSize(int32 I, float S);
	/** hold(level): every point at one brightness (0 dark, 1 its full colour, more for a flash), or
	 *  blinking again (unset) */
	void HoldAt(TOptional<float> InLevel);
	TOptional<float> Held() const { return Hold > 0 ? TOptional<float>(Level) : TOptional<float>(); }
	/** update(t) */
	void Update(double T);

	/** The vertex shader's colour factor: mix(b, uLevel, uHold), b the blink at uTime. */
	static float Factor(double T, const FVector2D& B, float Hold, float Level);

private:
	TArray<float> Brightness; // what each billboard has now (set only when it changes)
	void Apply(int32 I);
};

class AS47Web;

/** A GlowPoints by its path as the night's events use it (world/saroPower.ts, story/dishArray.ts: the
 *  points' colours and places in the world, a point out of sight): the one AS47Web keeps and blinks,
 *  so the blink and the events' colours are the same points. Positions in three's world terms. */
struct SIGNAL47_API FS47GlowView
{
	FS47GlowView() = default;
	FS47GlowView(AS47Web* InWeb, const FString& Path);

	bool IsValid() const { return Num() > 0; }
	int32 Num() const;
	FLinearColor Col(int32 I) const;
	void SetCol(int32 I, const FLinearColor& C) const;
	FVector Pos(int32 I) const;
	void SetPos(int32 I, const FVector& P) const;
	void SetVisible(int32 I, bool bOn) const;

private:
	TWeakObjectPtr<AS47Web> Web;
	int32 Node = -1;
	FS47GlowPoints* Get() const;
};

