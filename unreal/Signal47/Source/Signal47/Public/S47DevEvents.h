#pragma once
#include "CoreMinimal.h"
#include "S47PaleBody.h"

class US47Main;
class FS47DevFreeform;

// The fog density a developer event holds over the scene's own (S47Lighting), or below zero: none.
extern SIGNAL47_API float GS47DevFogDensity;

// The in-game developer events (Tom 29 October 2026): F10 in a night opens a list of things to
// make happen now, for trying them where the player stands. Any system adds its own with Add
// (aliens, sky, weather, light ...); the list is built when the menu opens, in the order added.
// Nothing here is saved, and nothing is offered on the title.
class SIGNAL47_API FS47DevEvents
{
public:
 // bFreeform: only offered in a Freeform night, and left out of the Story contract below.
 struct FEvent { FString Group, Label, Note; TFunction<void()> Fn; bool bFreeform=false; };
 explicit FS47DevEvents(US47Main* InM);
 void Add(const FString& Group, const FString& Label, const FString& Note, TFunction<void()> Fn);
 void AddFreeform(const FString& Group, const FString& Label, const FString& Note, TFunction<void()> Fn);
 // Renames a line in place, for a line that says what it is set to now.
 void Relabel(const FString& OldLabel, const FString& NewLabel);
 void Open();
 // The F10 screen while it is open (SS47Dev.h), for -S47DevMenuTest.
 TSharedPtr<class SS47Dev> Menu() const { return MenuEl.Pin(); }
 void Step(double Dt);
 void Stop();
 void ControlTest(double Dt);
 const TArray<FEvent>& Events() const { return List; }
 FS47DevFreeform* FreeformDev() const { return FreeformLines.Get(); }
 TOptional<FVector> PalePresence() const { return PaleLeft>0 && Pale.Rendered()?TOptional<FVector>(PaleAt+FVector(0,1.8,0)):TOptional<FVector>(); }
private:
 US47Main* M = nullptr;
 TArray<FEvent> List;
 // Freeform's own lines and the night run at speed (S47DevFreeform.h).
 TSharedPtr<class FS47DevFreeform> FreeformLines;
 FString ReopenGroup;
 TWeakPtr<class SS47Dev> MenuEl;
 double ShowerLeft = 0, ShowerNext = 0;
 FS47PaleBody Pale;
 // Tall, crab and Pale One, coming from a distance so their gait can be judged. The Pale One is Pale above, one body.
 enum EGait : int32 { GaitTall, GaitCrab, GaitPale, GaitCount };
 FS47PaleBody TallBody, CrabBody;
 struct FGaitTest { double Left=0,Clock=0,Yaw=0,Fold=0;FVector At=FVector::ZeroVector; } Gaits[GaitCount];
 FS47PaleBody& GaitBody(int32 Kind){return Kind==GaitCrab?CrabBody:Kind==GaitPale?Pale:TallBody;}
 bool BuildPale();
 void SpawnGait(int32 Kind);
 void StepGait(int32 Kind,double Dt);
 // The feet, not Pos: the walker's Pos.Y stays 0 whatever the ground does, and its FloorY is the ground.
 FVector PlayerFeet() const;
 int32 FormationPace=0;
 // -S47DevProof=/tmp/dir: what this menu offers, tried outdoors and from the natural schedule, with what is really drawn read back.
 FString ProofDir; int32 ProofStage=0,ProofChecks=0,ProofIndex=0; double ProofT=0,ProofAt=0; bool bProofInit=false,bProofDone=false; FString ProofShot; FVector ProofMean=FVector::ZeroVector;
 bool ProofTest(double Dt);
 bool ProofCheck(bool bGood,const FString& What);
 void ProofFinish(bool bGood,const FString& Why);
 bool ProofUse(const FString& Label);
 void ProofAim(const FVector& At);
 double PaleLeft = 0, PaleClock = 0;
 FVector PaleAt = FVector::ZeroVector;
 double PaleYaw = 0;
 FString TestDir, ShotName; double TestT = 0, ShotAt = 0; int32 TestStep = 0; bool bTestInit = false, bTestDone = false;
 int32 RoswellNumber=1;
 void SpawnRoswell(int32 Mode,bool bRoof=false);
 void AddBuiltIn();
 void SpawnPale();
 void SpawnCraft(bool bSaucer);
 TWeakObjectPtr<class UStaticMeshComponent> CraftPreview;
};
