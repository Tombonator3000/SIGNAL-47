#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "Tickable.h"
#include "UObject/Object.h"
#include "S47Station.generated.h"

class US47Main;
class UMaterialBillboardComponent;

// world/Station01.ts, the behaviour: chapter three's location, STATION 01, the survey station from
// 1947 (Station01's own terms: x east, z south, the group at (0, 0, 8000) in the one scene). The
// geometry is the web version's own (Tools/webexport); this holds its parts by their paths
// (world.site.proxies.*, world.site.objs.*, world.site.anchors.*, world.site.occluders[i]) and does
// what the class's switches do: the tin hood over the field lamp, the receiver's dial, the camera
// flash, the hut's light and door, the lamps outside, the walk gap by the vehicle gate and the
// drawer under the bench. The fake lights are the export's flood sets 'field' and 'hut'.
UCLASS()
class SIGNAL47_API US47Station : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);
	// whether the export has the station (World.prepare('station01') has built it in the web version)
	bool IsBuilt() const;

	// proxies: transit, footing, markerA, markerB, stakesC, diagram, lamp, cable, hutDoor, receiver,
	// timingLog, fieldPhone, gate, truckSpot, walkGate, drawer, ninthHead, ninthBody, ninthFrost
	FS47Obj Proxy(const FString& Key) const;
	// objs: fieldPhoneHandset, lampHead, lampHood, hutDoorLeaf, drawer, drawerInside, fx
	FS47Obj Obj(const FString& Key) const;
	// Invisible boxes for FieldCamera.clearLine(): the hut, the shed, the pier.
	TArray<FS47Obj> Occluders;

	// anchors (StationAnchors), in world terms
	FS47Pose Arrive() const;                        // anchors.arrive
	FVector MarkerTarget() const;                   // anchors.markerTarget
	FVector CableTarget() const;                    // anchors.cableTarget
	FS47Pose PhotoSpot(const FString& Which) const; // anchors.photoSpots.marker | cable (its pitch is towards the target)
	FVector Point(const FString& Key) const;        // anchors.points.<key>
	FS47Area HutBounds() const;

	bool bLampCovered = false;
	bool bReceiverOn = true;
	bool bHutLightOn = true;
	bool bOutLampsOn = true;
	bool bDrawerOpen = false;

	// P07: the tin hood over the field lamp. Covered, the lamp gives no light at all.
	void SetLampCovered(bool bOn);
	// The field camera's flash for one exposure: a white flood at the camera, in slot 0 (kept for
	// the truck's headlights, which are off while the truck is parked here). Unset: off.
	void SetFlash(const TOptional<FVector>& P, double W = 16);
	void SetReceiver(bool bOn);
	void SetHutLight(bool bOn);
	// The hut's door (story/stationTaken.ts): 0 open into the room as it has stood for years, 1 shut
	// in its frame. Shut, the doorway is closed to walking (a collider in the opening).
	void SetHutDoor(double K);
	// The lamps outside (story/stationTaken.ts): the yard light on its pole, the bulb over the hut
	// door, the caged one on the shed and the field lamp (when its hood is up), all out or all back.
	void SetOutLamps(bool bOn);
	// The walk gap in the fence beside the vehicle gate: chained until chapter three, open from then
	// on (main.ts). The truck any time may bring the player here before that.
	void SetWalkGate(bool bOpen);
	// The drawer under the bench out (with what is in it to be seen) or in. bNow: at once (a save).
	void SetDrawer(bool bOpen, bool bNow = false);

	// Station01.update: the drawer slides, the bulb over the door dips in its socket
	void Update(double Dt, double T);

	// main.ts every frame: world.setStoryPlaces({ stationOpen: game.at('ch3'), ... }); the walk gap
	// follows it (set by chapter three's registration)
	TFunction<bool()> StationOpen;

	// FTickableGameObject: World.update steps the site while the player is at the station, and
	// main.ts's step sets the story's places every frame
	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(US47Station, STATGROUP_Tickables); }
	virtual UWorld* GetTickableGameObjectWorld() const override;

	// Station01's private slots, by the order its constructor adds them (fieldFlood.count starts at
	// 2: the truck has 0 and 1; hutFlood at 0), and its BASE intensities
	struct FSlots { int32 Wall = 3, Field = 6, Yard = 2, Spill = 4, Shed = 5, Ceil = 2, Desk = 1, Dial = 0, Door = 3; };
	static const FSlots SLOT;
	struct FBase { double Wall = 6.0, Field = 9.0, Yard = 12.0, Spill = 3.0, Shed = 2.4, Ceil = 5.0, Desk = 2.6, Dial = 1.1, Door = 1.4; };
	static const FBase BASE;
	static constexpr double HOOD_OPEN = 2.1;

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	int32 FieldFlood = -1, HutFlood = -1; // the export's flood sets 'field' and 'hut'
	bool bBuilt = false;
	// the glow points (glow.add, gx): found by their place under world.site.glow.points
	struct FGlows { TWeakObjectPtr<UMaterialBillboardComponent> Wall, Field, Yard, Shed, WinS, WinE, Ceil, Desk, Dial; };
	FGlows Gx;
	TOptional<bool> WalkOpen;
	bool bDoorCol = false;
	double WallK = 1;
	double DrawerK = 0, DrawerZ = 0;
	double T = 0;

	void FindGlows();
	void SetGlow(const TWeakObjectPtr<UMaterialBillboardComponent>& G, uint32 Color, double K = 1) const;
	void SetFloodIn(int32 Set, int32 Slot, double Intensity) const;
	void SetMatColor(const TCHAR* Name, uint32 Color) const;
	void PlaceDrawer();
	FS47Pose PoseAt(const FString& Path) const;
};
