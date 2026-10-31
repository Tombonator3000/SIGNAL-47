#pragma once
#include "CoreMinimal.h"

struct FS47TripoCreature;

enum class ES47RoswellMotion : uint8 { Idle, Listen, Walk, Stalk, Run, Peek, Grab };

struct FS47RoswellPose
{
 ES47RoswellMotion Mode=ES47RoswellMotion::Idle;
 bool bVisible=true, bHeld=false;
 // Distance is the controller's cumulative, collision-resolved travel in metres.
 double SpeedMps=0, DistanceTravelled=0, Crouch=0, HeadYaw=0, HeadPitch=0;
};

struct FS47RoswellContacts
{
 bool bLeft=false, bRight=false;
 double Energy=0;
 // Actual sole contacts in Unreal world centimetres, not Three metres.
 FVector AtUE[2]={FVector::ZeroVector,FVector::ZeroVector};
};

// One instance per small Roswell. Place its parent root before Apply().
// Never shared with the tall Pale threat or between different actors.
class FS47RoswellMotion
{
public:
 FS47RoswellContacts Apply(FS47TripoCreature& Creature,const FS47RoswellPose& In,double Dt);
 void Reset();
private:
 struct FFoot
 {
  FVector Anchor=FVector::ZeroVector, Start=FVector::ZeroVector, Target=FVector::ZeroVector, Current=FVector::ZeroVector;
  double Previous=0;
  bool bSwing=false;
 } Feet[2];
 bool bInit=false;
 double Distance=0, Phase=0, Clock=0, Duck=0, HeadYaw=0, HeadPitch=0, Motion=0, Reach=0;
 FVector LastRoot=FVector::ZeroVector;
};
