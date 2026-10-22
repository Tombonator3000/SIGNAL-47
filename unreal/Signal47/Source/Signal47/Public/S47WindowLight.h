#pragma once

#include "CoreMinimal.h"

class AS47Web;
class FJsonObject;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;

// world/windowLight.ts: a lit window's light on the ground in front of it, the panes, the bars, the
// blinds or the curtain laid out the way the room's lamp throws them through the glass, and a faint
// glow of the whole glass at the foot of the wall; off with the window, and held back where the
// area's other lamps already light the ground. One mesh for all the windows of an area (the
// export's, with its corners' picture coordinates in its UV channels), its shader ported as
// M_WebWindowLight; each corner's colour (aCol) is worked out here as update() does and written to
// a small texture the material reads by the corner's index.
class SIGNAL47_API FS47WindowLights
{
public:
	FString Name;
	bool Build(AS47Web* W, const TSharedPtr<FJsonObject>& J);
	/** setLevel(id, k): how bright a window without a flood slot of its own is now. */
	void SetLevel(const FString& Id, double K);
	/** update(): the windows' lamps now, and the street's light where it changed. */
	void Update();
	int32 Windows() const { return Wins.Num(); }
	/** The windows by id now, 0 (dark) to about 1 (as made) (tests). */
	TMap<FString, double> Levels() const;
	/** How much the street's own light holds the windows back, on average (1 none) (tests). */
	double Darkness() const;

private:
	struct FWin { FString Id; int32 Slot = -1; double K = 0, W0 = 1, Level = 1; FLinearColor Col; int32 At = 0, N = 0; TArray<double> Fall; };
	TArray<FWin> Wins;
	TArray<FVector> Pos;      // the corners, the mesh's terms
	TArray<FLinearColor> Tints;
	TArray<double> Dark;
	TArray<int32> Own;
	double FallOff = 0.03;
	FVector Origin = FVector::ZeroVector;
	int32 Set = -1;
	FString Lit;
	TMap<int32, float> LastW;
	bool bAgain = false;
	TWeakObjectPtr<AS47Web> Web;
	TWeakObjectPtr<UStaticMeshComponent> Mesh;
	TWeakObjectPtr<UMaterialInstanceDynamic> Mid;
	TWeakObjectPtr<UTexture2D> Cols;
	int32 ColsW = 64, ColsH = 1;
	void Street();
	void Upload(const TArray<FLinearColor>& C);
};
