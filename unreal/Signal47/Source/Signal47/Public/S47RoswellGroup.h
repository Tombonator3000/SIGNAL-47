#pragma once
#include "CoreMinimal.h"
#include "S47RoswellBrain.h"

class US47Main;
class USceneComponent;
struct FS47TripoCreature;
struct FS47GlimpsesIn;

// One native skin, brain, gait and Foley source per individual. Freeform can persist controlled groups.
class FS47RoswellGroup
{
public:
 struct FView
 {
  int32 Id=0; FS47RoswellBrain::EMode Mode=FS47RoswellBrain::EMode::Curious;
  FS47RoswellBrain::EState State=FS47RoswellBrain::EState::Idle;
  FVector Pos=FVector::ZeroVector, Eye=FVector::ZeroVector;
  double Distance=0, Age=0, Speed=0, Crouch=0;
  bool bRoof=false,bRendered=false; int32 Contacts=0, Sounds=0;
 };
 FS47RoswellGroup(US47Main* Main,USceneComponent* FirstRoot,FS47TripoCreature* FirstBody);
 ~FS47RoswellGroup();
 void Update(double Dt,const FS47GlimpsesIn& Input);
 int32 Spawn(int32 Count,FS47RoswellBrain::EMode Mode,bool bRoof=false,bool bNatural=false,bool bControlled=false);
 S47J::FObj Save() const;
 bool Restore(const S47J::FObj& Saved);
 void ReleaseControlled();
 void SetMode(FS47RoswellBrain::EMode Mode);
 bool Developer() const;
 void Clear();
 TArray<FView> Views() const;
 // UE-space closed doorway bounds against actual active native body bounds.
 bool DoorOccupied(const FBox& DoorwayUE) const;
 int32 NightCount=0;
private:
 struct FImpl;
 TUniquePtr<FImpl> P;
};
