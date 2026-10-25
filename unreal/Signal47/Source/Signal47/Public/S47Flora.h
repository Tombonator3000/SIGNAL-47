#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"

class AS47Web;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

// world/flora.ts: the desert ground cover's streaming. The land is cut into 8 m cells; a cell's
// plants are drawn from its own random numbers (the same seeds as the web version: hash4, mulberry,
// vnoise) the first time the camera comes near it, so the same land draws the very same plants;
// only the cells near the camera, in front of it, are in the meshes, copied in again each time the
// camera has moved 8 m or turned. The shapes, their pictures and the materials are the export's (one
// instanced mesh per kind: the pair's second kind on a mesh of its own, export.js pairGeo); the
// distance fade and the sway are the flora materials' (import_web.py FLORA_WPO). Each outdoor area
// has one patch (S47GroundCover.h says where it grows and how thick).
enum class ES47FloraKind : uint8 { Grass, Sage, Creosote, Stalks, Succulent, Stones, Stain };

struct FS47FloraRect { double MinX = 0, MaxX = 0, MinZ = 0, MaxZ = 0; };

/** FloraSpec: what a patch needs to know about its land, in the patch's own terms. */
struct SIGNAL47_API FS47FloraSpec
{
	FString Name;                 // for the tests
	int32 Seed = 1986;            // the same seed and the same cover give the same plants in two patches
	TFunction<double(double, double)> Ground;   // the ground's height at a point
	TFunction<double(double, double)> Cover;    // how thick: 0 none, 1 the open desert, up to about 2.5
	TArray<FS47FloraRect> Blocks;              // things that stand: nothing inside, thick round their feet
	TArray<FS47FloraRect> Walks;               // where the player walks: only the low kinds, and small
	TMap<ES47FloraKind, double> Mix;            // more or less of a kind here
	TFunction<FString(double, double)> Ways;     // tests: a way the story uses at a point
	TFunction<double(double, double)> Seam;     // in a way's seam: tiny grass and pebbles
	TFunction<double(double, double)> Spill;    // the gravel spilt onto the dirt just off a way
};

namespace S47Flora
{
	/** The random numbers (flora.ts hash4, mulberry, vnoise) and a way's ragged edge. */
	SIGNAL47_API uint32 Hash4(int32 A, int32 B, int32 C, int32 D);
	struct FMulberry { uint32 S; explicit FMulberry(uint32 Seed) : S(Seed) {} double operator()(); };
	SIGNAL47_API double VNoise(double X, double Z, double Size, int32 Seed);
	SIGNAL47_API double Ragged(double X, double Z);
}

/** One patch of ground cover (flora.ts Flora), filling the export's meshes under its group. */
class SIGNAL47_API FS47Flora
{
public:
	struct FKind;
	/** A kind's copies made in a cell: matrices (three's, sixteen floats each), colours, ranks, seams. */
	struct FKindCell { TArray<float> M, C, Rank; TArray<uint8> Seam; int32 N = 0; };

	FS47FloraSpec Spec;
	FS47Flora(const FS47FloraSpec& InSpec);
	/** More things that stand, or more ground walked (an area built later): the cells round them are made again. */
	void Add(const TArray<FS47FloraRect>& Blocks, const TArray<FS47FloraRect>& Walks);

	/** The patch's group in the export ("flora:<name>") and its meshes, drawn from here. */
	bool Bind(AS47Web* W, int32 GroupNode);
	/** follow(cam): fill the meshes round the camera when it has moved or turned far enough (cam in
	 *  world terms, three's; bLow: the Low tier). */
	void Follow(const FVector& CamPos, const FVector& CamDir, double FovDeg, double Aspect, bool bLow);
	/** The wind's way in this patch's terms, on its materials (world/wind.ts, turned with the group). */
	void SetWind(const FVector2D& Dir);
	/** The second kind of a pair is drawn when the first is (its mesh is shown and hidden with the area's). */
	void SyncVisibility();

	/** The copies in the meshes now, by kind (tests). */
	TMap<ES47FloraKind, int32> Drawn() const;
	/** Opt-in highway audit: last fill candidates, omitted invisible copies and actual GPU upload. */
	void AuditDrawCounts(TArray<FString>& Lines) const;
	/** A cell's copies of a kind (tests): made as the web version makes them. */
	const FKindCell& CellOf(int32 I, int32 J, int32 Ki) { return Cell(I, J, Ki); }
	bool Shown() const;

private:
	TMap<int64, TArray<TOptional<FKindCell>>> Cells;
	FVector2D At = FVector2D(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
	int32 Q = -1; // the tier the meshes were filled for (0 high, 1 low)
	bool bDirty = true;
	struct FBucket { TArray<FS47FloraRect> B, W; };
	TMap<int64, FBucket> Bucket;
	bool bWalked = false, bSeamed = false, bSpilt = false;
	double Yaw = 0, Half = UE_DOUBLE_PI;
	struct FNext { int32 Kind = 0, At = 0; } Next;
	bool bPending = false;

	int64 Key(int32 I, int32 J) const { return int64(I + 40000) * 80000 + int64(J + 40000); }
	double NoiseAt(double X, double Z, int32 Ki) const;
	double CoverAt(double X, double Z, int32 Ki, double Noise);
	const FKindCell& Cell(int32 I, int32 J, int32 Ki);
	TArray<float> Nurses(int32 I, int32 J, const TArray<int32>& Kinds);
	FKindCell Make(int32 I, int32 J, int32 Ki);
	bool Ahead(double Px, double Pz);
	void Fill(double Px, double Pz, bool bLow, bool bAll = false);

	// ---------- Unreal ----------
	TWeakObjectPtr<AS47Web> Web;
	int32 Group = -1;
	/** a kind's mesh: the export's for the first of a pair, one of its own for the second */
	struct FLayer { TWeakObjectPtr<UInstancedStaticMeshComponent> Mesh; TWeakObjectPtr<UMaterialInstanceDynamic> Mid; double Reach = 0; int32 N = 0, Candidates = 0, Omitted = 0, EndCull = 0; };
	TArray<FLayer> Layers;
	void Put(int32 Ki, const TArray<float>& M, const TArray<float>& C, int32 N);
};
