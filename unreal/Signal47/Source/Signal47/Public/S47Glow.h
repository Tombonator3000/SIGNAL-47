#pragma once

#include "CoreMinimal.h"

class AS47Web;
class UMaterialBillboardComponent;

// world/glow.ts GlowPoints as the export left them: AS47Web builds one billboard per point under the
// points' node, in the order the points were added, each with its own copy of M_WebGlow ("Color").
// This reaches them by that node's path ("yard.glow.points", "ext.dishArray.beacons.points") so the
// story can do what the web version does to the points' attributes: read and write a point's colour
// (aCol) and move it (setPosition). Positions are three.js world terms. The blink of a GlowPoints
// shader (aBlink) is not in the export: the antennas' red lamps blink in FS47DishArray.
struct SIGNAL47_API FS47GlowPoints
{
	FS47GlowPoints() = default;
	FS47GlowPoints(AS47Web* InWeb, const FString& Path);

	bool IsValid() const { return Points.Num() > 0; }
	int32 Num() const { return Points.Num(); }
	/** aCol as it is now (the material's Color). */
	FLinearColor Col(int32 I) const;
	void SetCol(int32 I, const FLinearColor& C) const;
	/** position (three's world terms). */
	FVector Pos(int32 I) const;
	void SetPos(int32 I, const FVector& P) const;
	void SetVisible(int32 I, bool bOn) const;

	TArray<TWeakObjectPtr<UMaterialBillboardComponent>> Points;
};
