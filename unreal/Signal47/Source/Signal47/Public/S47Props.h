#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "S47Props.generated.h"

class AS47Web;
class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;
class UPrimitiveComponent;
class FJsonObject;

// Unreal only (departs from the web version): the free models (Tools/freemodels, /Game/Free) and
// Tom's own Blender props placed in the exported world, from one data file per area under
// Config/Props/<area>.json. Positions are the web version's (three.js metres, y up); a yaw turns a
// model about y as three's rotation.y does. A prop can replace exported objects: their meshes are
// hidden from view but keep their collision, so the story's things to use stay where they were and
// answer as before, and the prop hangs under the first of them (it shows, moves and is used with
// it). Props that block get a collider in the walk (and the drive's obstacles where the leg reads
// them from the export). Exported meshes merged with others (a room's static parts) cannot be hidden
// one by one: a prop stands beside those, or over them where it covers them.
//
// The file's fields: area (whose ground and visibility), walk (the walk's key, default the area),
// leg (the drive leg whose obstacles get the colliders, if any), parent (a node path the props hang
// under; default the area's first root), props [ { id, model, at [x, y, z], yaw (degrees), scale,
// ground (true: y found by a trace down from 'at' + lift; 'at' y null: from high above), lift,
// collide, pad, replaces [paths], parent, when (a node path: shown only while it is), paper,
// local (true: 'at' and 'yaw' in the parent's own terms, no trace: the prop moves with a node the
// story moves, the deputy's car), swing { open (a node path: open while it is shown), hinge [x, z]
// (metres in the model: x forward, z to its right), deg }, beacon { on (a node path: lit while it
// is shown), bar [x0, x1, y0, y1, z0, z1] (the light bar's glass in the model, metres) } } ].
//
// The beacon (Tom, 23.10: the patrol car on the shoulder, its roof lights on and flashing): the bar's
// two halves as lenses of their own, red and blue, and one small spot light on each that turns as an
// old rotating beacon does (1.5 turns a second, the two halves half a turn apart) with a quick double
// flash on the lens as its beam comes round; no shadows, a short reach, two lights only. Unreal only.
UCLASS()
class SIGNAL47_API AS47Props : public AActor
{
	GENERATED_BODY()

public:
	AS47Props();
	virtual void Tick(float DeltaSeconds) override;

	/** Reads every Config/Props/<area>.json and places what they hold (AS47Web::BeginPlay, once). */
	void Build(AS47Web* InWeb);
	int32 PlacedCount() const { return Placed.Num(); }

	/** The data files read and checked without the world: each prop names a model the manifest
	 *  knows (when Content/Free is there), a place, and the files agree on their fields. */
	static bool SelfTest(TArray<FString>& Lines);

	/** -S47PropsAt=<area>:x;z;yaw;pitch (the yaw and pitch in radians, three's): 1.5 s after the
	 *  world is up the player stands there in that area (for before and after pictures). */
	static void TestPlace(class US47Main* M);

	/** The beacon at time T (seconds): each half's lens (0..1: its glow, 1 in the double flash) and
	 *  its light's turn (degrees about the bar's up axis). Half 0 is red, half 1 blue. */
	static void BeaconAt(double T, float Lens[2], float Turn[2]);

private:
	struct FPlaced
	{
		FString Id;
		TWeakObjectPtr<UStaticMeshComponent> Comp;
		int32 Node = -1;  // its node (AdoptNode)
		int32 When = -1;  // shown only while this node is
		bool bPaper = false; // the printer's paper: follows room.printerPaper (ControlRoom.ts printerPaper)
		FTransform Rest;  // its own transform as placed (relative)
	};
	TArray<FPlaced> Placed;
	struct FSwing
	{
		TWeakObjectPtr<UStaticMeshComponent> Comp;
		int32 Open = -1;
		FVector Hinge = FVector::ZeroVector; // in the model's own Unreal terms (cm)
		float Deg = 55.f, K = 0.f;
		FTransform Rest;
	};
	TArray<FSwing> Swings;
	struct FBeacon
	{
		TWeakObjectPtr<UStaticMeshComponent> Car;
		int32 On = -1;
		TWeakObjectPtr<UStaticMeshComponent> Lens[2];
		TWeakObjectPtr<class UMaterialInstanceDynamic> Mid[2];
		TWeakObjectPtr<class USpotLightComponent> Light[2];
		bool bLit = false;
		double T = 0;
	};
	TArray<FBeacon> Beacons;
	// -S47PropsTour=<dir>[@name,name]: every file's views (or those named), one picture each, then the
	// game ends; with -S47PropsOff the same views without the props (the before pictures)
	struct FView { FString Name, Area, Car; double X = 0, Z = 0, Yaw = 0, Pitch = 0, Clock = -1; };
	TArray<FView> Views;
	FString TourDir;
	int32 TourStep = -1;
	bool bTourShot = false;
	double TourT = 0;
	void ReadViews(const TSharedPtr<FJsonObject>& File, const TArray<FString>& Only);
	// the walk's collider for a footprint (and the drive leg's obstacle); 1 when added
	int32 AddCollider(const FString& Walk, const FString& Leg, const FBox& UE, double Pad, const FString& Id);
	// rows (fences, poles with their wires) and scattered things (rocks, brush), instanced
	int32 BuildRows(const TSharedPtr<FJsonObject>& File, const FString& Name, const FString& Walk, const FString& Leg, const TArray<FString>& Areas, int32 Parent, int32& Count);
	int32 BuildScatter(const TSharedPtr<FJsonObject>& File, const FString& Name, const FString& Walk, const FString& Leg, const TArray<FString>& Areas, int32 Parent, int32& Count);
	class UInstancedStaticMeshComponent* Instanced(UStaticMesh* Mesh, int32 Parent, const FString& Id, bool bShadow);
	void Tour(float Dt);
	void AddSwing(UStaticMeshComponent* C, const TSharedPtr<FJsonObject>& Swing);
	void AddBeacon(UStaticMeshComponent* Car, const TSharedPtr<FJsonObject>& Beacon);
	void UpdateBeacon(FBeacon& B, float Dt);
	bool Shown(int32 Node) const;
	TWeakObjectPtr<AS47Web> Web;
	TMap<FString, FString> Assets; // model name and asset name -> asset path (Content/Free/free_models.json)
	TMap<FString, TSet<int32>> AreaNodes; // the manifest's areas: their nodes
	int32 PaperNode = -1;

	void LoadManifests();
	void BuildFile(const TSharedPtr<FJsonObject>& File, const FString& Name);
	UStaticMesh* MeshOf(const FString& Model) const;
	bool InArea(int32 Node, const TArray<FString>& Areas) const;
	bool Ground(const FVector& Three, double From, double To, const TArray<FString>& Areas, const TArray<UPrimitiveComponent*>& Ignore, double& OutY) const;
	void HideMeshes(int32 Node, TArray<UPrimitiveComponent*>& Hidden);
};
