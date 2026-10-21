#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "Tickable.h"
#include "UObject/Object.h"
#include "S47Annex.generated.h"

class US47Main;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;

// world/Annex.ts, the behaviour of the chapter two location: the south corridor behind the
// control room and the SARO records room off it. The rooms themselves come from the web
// version's scene (Tools/webexport: annex.group, annex.interior and everything under them);
// this is what the code does with them: the zones and the objects by their names, the tired
// tube that flickers (and is held by the steps on the roof and the archive's phone), the dust
// shaken out of the ceiling, the frost on the roof, the tubes without power, and the archive's
// phone fittings (world/archive047.ts, annex.arc).
//
//   z 4.8 to 6.6   corridor, x -6 to 2. The control room door is at x -2.9 to -1.9.
//   z 6.8 to 10.5  records room, x -6 to -1. Its doorway is at x -4.4 to -3.4.
//                  East of it (x -1 to 2): restrooms and the operations office, closed.
//
// The doors (setRecordsDoor, setExitDoor) are moved by AS47Web's doors, from the exported leaf
// positions. main.ts calls annex.update(t) every frame; until its step is ported into
// US47Main::Step, the annex ticks itself with main.ts's clock (US47Main::T).

// world/archive047.ts Archive047: the fire drawer in the west wall, the 1947 box's papers on the
// floor, the switchboard and its call recorder (story/archivePhone.ts moves them)
struct SIGNAL47_API FS47Arc
{
	US47Main* M = nullptr;
	FS47Obj Group, Drawer, Reel, Papers;
	struct FHit { FS47Obj Drawer, Ksu, Recorder; };
	FHit Hit;                                        // the hit areas (invisible)
	double Shut = -5.994, Out = -5.994 + 0.3;      // where the drawer's front is (room x)
	FVector RecorderAt = FVector::ZeroVector, DrawerAt = FVector::ZeroVector; // sounds, three.js terms
	void SetDrawer(double K) const;                  // 0 shut, 1 all the way out
	void Lamp(const FString& Line, double K) const;  // 'trk1' | 'trk2': 0 dark, 1 lit
	void SetStrip(const TArray<FString>& Lines) const; // the strip out of the recorder
};

UCLASS()
class SIGNAL47_API US47Annex : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// the one in play (made by chapter two's registration, S47Chapter2.cpp)
	static US47Annex* Get();
	void Init(US47Main* InM);

	// zone: Record<string, Zone> ('southDoor', 'corridor', 'recordsDoor', 'records', 'exitDoor')
	FS47Area Zone(const FString& Id) const;
	// objs: Record<string, Object3D> ('fieldBox', 'binder', 'lineage', 'workTable', 'recPhone',
	// 'vending', 'officeDoor', 'restroomDoor', 'exitDoor', 'recordsDoor', ...)
	FS47Obj Obj(const FString& Name) const;
	FS47Obj Interior() const; // everything inside, drawn while the player is in it or looking in
	FS47Arc Arc;              // the archive's phone (story/archivePhone.ts)

	// The tubes while something stands on the roof (story/roofSteps.ts): 'steady' holds the tired
	// one lit, without its flicker; 'stutter' lets it go with a run of flickers; 'out' puts it out
	// (the archive's phone, story/archivePhone.ts: seven seconds before); '' (null) as it was.
	void HoldTubes(const FString& Mode);
	FString TubeMode() const { return TubeHold; } // how the tired tube is held now (tests)

	// Dust shaken out of the ceiling at (x, z), falling to the floor (story/roofSteps.ts): a few
	// motes under each step that comes over the room (n small, quick), and a thin trickle where
	// they stop, right over the player (the default: 56 motes over some two seconds). One pool of
	// motes, drawn with the rooms; each mote is gone when it reaches the floor, the pool hidden
	// when all are.
	void Sift(double X, double Z, int32 N = 56, double Spread = 0.12, double Delay = 2.2);
	int32 DustLive() const; // motes in the air now (tests)

	// The frost on the roof unit and on the lip at one corner (story/roofSteps.ts), or none ('').
	void SetRoofFrost(const FString& Corner);
	FString FrostAt;

	// The tubes with the power (1) or without it (0: world/saroPower.ts), as glass.
	void SetPower(double K);

	// A tired tube flickers now and then.
	void Update(double T);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(US47Annex, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }
	virtual bool IsTickable() const override;

	// the flood set's index of the tired tube's lamp: addFlood's order in Annex.ts (corridor
	// -3.7 and 0.2, the vending machine, the records room -4.6 and -2.4, the desk lamp)
	static constexpr int32 FlickerFlood = 4;

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	FString TubeHold;
	double StutterLeft = 0;
	double LastT = -1;
	double Power = 1;
	int32 LastOn = -1;
	int32 FloodSet = -1;

	// the dust: positions in three.js terms, fall speeds, when each lets go, which are in the air
	struct FDust { TArray<FVector> Pos; TArray<float> V, At; TArray<uint8> Live; double T = 0; int32 Next = 0; bool bVisible = false; };
	TOptional<FDust> Dust;
	UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> DustMesh;
	UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> DustMat;
	void DustStep(double Dt);
	void DrawDust();
	FLinearColor TubeOnColor() const;
	FLinearColor TubeDimColor() const;
};
