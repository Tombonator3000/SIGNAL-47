#pragma once
#include "CoreMinimal.h"

class US47Main;
class USpotLightComponent;
class ULightComponent;
class ULocalFogVolumeComponent;
class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// What the massive light asks of the night's fog (S47Lighting), or at or below zero: nothing. Slices kept to the rooms
// instead of 140 m out, and light thrown forward through the air, while it lasts.
extern SIGNAL47_API float GS47FloodFogDistance;
extern SIGNAL47_API float GS47FloodFogPhase;

// An enormous light outside a building, seen from inside it (Tom's T16-14): one real shadowed source out in the open, so it
// can only come in by the windows and doors, the walls and the furniture throwing real shadows that sweep as it moves. The air
// indoors catches it in the volumetric fog: the same froxels that take the room's local fog, lit through the same shadow.
// Nothing that lights a room here casts no shadow. The room comes up by Lumen's bounce off the lit patches, not by a fill.
// Every value is a console variable (s47.flood.*), so it can be tried with -ExecCmds, without a build.
class SIGNAL47_API FS47OutsideLight
{
public:
 explicit FS47OutsideLight(US47Main* Main):M(Main){}
 ~FS47OutsideLight();
 struct FFrame
 {
  FVector From=FVector::ZeroVector,Aim=FVector::ZeroVector; // three's: metres, Y up
  FLinearColor Color=FLinearColor(.82f,.86f,1.f);
  double K=1;             // 0: out. Above 1 at the very end of being taken.
  double Seconds=0;       // drives the dust
  bool bThroughRoof=false; // a shut room, once they are held: it comes down on them all the same
 };
 void Show(const FFrame& Frame);
 // Out, and the lamps and the renderer as they were. The air has its own level: see SetAir.
 void Hide();
 // Gone, components and all.
 void Stop();
 // The air. bRoom: a thin haze that a shaft shows in; otherwise the low thick mist of the open ground.
 void SetAir(double K,const FVector& At,const FLinearColor& Albedo,bool bRoom);
 bool Shown() const{return bShown;}
 // One is up somewhere and holds the renderer's values.
 static bool AnyShown(){return ShownNow>0;}

 // From, drifting slowly side to side and up and down: the patches on the floor and the window bars' shadows travel, while
 // the place it stands in the game does not change.
 static FVector Swept(const FVector& From,const FVector& Aim,double Seconds);
 // A place out in the open from which a straight line reaches Chest, through a window or a door. False: nothing does.
 static bool FindWayIn(US47Main* Main,const FVector& Chest,FVector& OutFrom);
private:
 static int32 ShownNow;
 US47Main* M=nullptr;
 TWeakObjectPtr<USpotLightComponent> Spot;
 TWeakObjectPtr<ULocalFogVolumeComponent> Air;
 TWeakObjectPtr<UStaticMeshComponent> Core,Halo;
 TWeakObjectPtr<UMaterialInstanceDynamic> CoreMid,HaloMid,DustMid;
 TWeakObjectPtr<UInstancedStaticMeshComponent> Dust;
 TArray<TWeakObjectPtr<ULightComponent>> Doused;
 TArray<FVector> DustSeed;
 TArray<FTransform> DustMoved;
 TArray<float> DustLit;
 int32 DustNext=0;
 struct FHeld{FString Name;float Was=0;};
 TArray<FHeld> HeldCvars;
 bool bShown=false,bPowerCut=false,bFogAsked=false;
 void Hold(const TCHAR* Name,float Want);
 void Acquire(const FVector& Aim);
 void Release();
 void StepDust(const FFrame& Frame);
 void StepCore(const FFrame& Frame);
 bool EnsureParts();
 bool bPartsTried=false;
};
