#pragma once

#include "CoreMinimal.h"
#include "S47Obj.h"
#include "Tickable.h"
#include "UObject/Object.h"
#include "S47Motel.generated.h"

class US47Main;

// world/MotelFront.ts and world/Crossing.ts, the behaviour chapter four needs: Sierra Motor Court
// from the outside, 650 m down the highway (the court, CourtSite in Crossing.ts; MotelFront is the
// one the world builds), with room 6's hinged door leaf and its light (the porch lamp, the window
// and the light it lays on the walk), the sign's flicker, and the brick that props the fire exit
// open once it is (Crossing.ts). The geometry is the web version's (world.motel.*, world.crossing.*).
//
// World.update steps the court while the player is at SARO (the motel's front is part of SARO's
// area). The world's step is not in this version yet, so the court ticks itself on the same rule.
UCLASS()
class SIGNAL47_API US47Motel : public UObject, public FTickableGameObject
{
	GENERATED_BODY()

public:
	void Init(US47Main* InM);

	// proxies: room6Door, officeDoor, register, keyBoard, officePhone, message, envelope, otherDoors,
	// iceMachine, car, pool, officeTv; objs: room6DoorLeaf, envelope, message, key6Hook, officePhoneHandset
	FS47Obj Proxy(const FString& Id) const;
	FS47Obj Obj(const FString& Id) const;
	/** anchors: entry, officeInside, room6Outside, fromRoom6 */
	FS47Pose Anchor(const FString& Id) const;
	/** Crossing.brick: the brick on the step outside the fire exit */
	FS47Obj Brick() const;

	bool bRoom6LightOn = true;
	void SetRoom6Light(bool bOn);
	void Update(double Dt, double T);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return bInit; }
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(US47Motel, STATGROUP_Tickables); }
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional; }

private:
	UPROPERTY() TObjectPtr<US47Main> M;
	bool bInit = false;
	// courtFlood: floodSet(16, 'court', 0.1), and the slots the constructor adds in this order: the office
	// 0 and its spill 1, the rooms r1 2, r2 3, r34 4, r5 5, room6 6, r78 7, the sign red 8 and cyan 9
	int32 Flood = -1;
	static constexpr int32 SlotR34 = 4, SlotRoom6 = 6, SlotRed = 8, SlotCyan = 9;
	TWeakObjectPtr<class UTexture> LitWindow, DarkWindow;
};
