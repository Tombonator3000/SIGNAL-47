#pragma once
#include "S47TubeFlicker.h"
#include "S47Json.h"
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
class US47Main;
class UMaterialInterface;

// SARO east wing (Docs/Design/EastWing/EAST_WING_PLAN.md): the exported 1960s wing is
// retired and a native test lab / parts storage / calibration / garage building stands
// on the same footprint. World frame (three.js export): x 20.2..39.0, z -5.9..5.9, roof 3.6.
// Wall planes match the old shell exactly (x 20.6..38.6, z -5.5..5.5) so the kept roof
// dressing, downspouts and east-end meter stay on the walls.
class FS47EastWing
{
public:
 void TestDoorUp(){DoorOpen=1;bDoorUp=true;bDoorMoving=bDoorHand=false;} // S47FloodProbe: the big opening, at once
 explicit FS47EastWing(US47Main* InM);
 ~FS47EastWing();
 void Init();
 void Begin(const S47J::FObj& Saved);
 S47J::FObj Save() const;
 bool Built() const { return bBuilt; }
 static FBox Footprint() { return FBox(FVector(20.2,-.7,-5.9),FVector(39.0,4.6,5.9)); }
 // Rooms (three.js x,z): test lab, parts storage, calibration/repair, garage.
 static FBox2D Room(int32 Index);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 bool bBuilt=false;
 int32 Parent=-1;
 FTSTicker::FDelegateHandle Tick;
 int32 ShotStage=0;double ShotAt=0;
 TArray<TWeakObjectPtr<class USceneComponent>> Parts;bool bShown=true;
 void Dump() const;
 bool Retire();
 void Build();
 bool BuildDetailed();
 bool RelocateAccessDressing();
 TArray<TWeakObjectPtr<class UStaticMeshComponent>> NativeDoors;
 TUniquePtr<class FS47EastWingProps> PropsHelper;
 void Block(const FString& Name,const FVector& Min,const FVector& Max,UMaterialInterface* Mat,bool bShadow=true);
 void Light(const FString& Name,const FVector& At,float Lumens,const FLinearColor& Colour,double YawDegrees=0);
 void Walls();
 void Grime();
 // Motorised roll-up door (Tom 4 Nov): two control stations (inside / outside), 4.5 s travel.
 double DriveSince=0;
 double DoorOpen=1;bool bDoorUp=true,bDoorMoving=false,bDoorHand=false,bDoorControls=false;
 bool bLeafControls=false; bool LeafOpen[2]={true,true}; double LeafAmount[2]={1.,1.};
 bool LeafControls(); void UseLeaf(int32 Index); void DoorTest(); int32 DoorTestStage=0; double DoorTestAt=0;
 bool DoorControls();void StepDoor(float Dt,bool bFree,bool bPowered);
 FString DoorLabel() const;void UseDoor();
 TArray<TWeakObjectPtr<class URectLightComponent>> Tubes; TArray<float> TubeLumens; TArray<FS47TubeFlicker> TubeFlicker; // photo pass: deferred-decal dirt bands, floor dust, jamb scuffs, run-off streaks
 bool Step(float Dt);
 void DriveTest();
 FString DriveStoryState;
 int32 DriveStage=0; double DriveStarted=0,DriveWaitStarted=0; bool bDriveRampSeen=false;
 TArray<FString> DriveChecks;
};
