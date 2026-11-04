#pragma once
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
 explicit FS47EastWing(US47Main* InM);
 ~FS47EastWing();
 void Init();
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
 void Light(const FString& Name,const FVector& At,float Lumens,const FLinearColor& Colour);
 void Walls();
 bool Step(float Dt);
 void DriveTest();
 int32 DriveStage=0; double DriveStarted=0; bool bDriveRampSeen=false;
 TArray<FString> DriveChecks;
};
