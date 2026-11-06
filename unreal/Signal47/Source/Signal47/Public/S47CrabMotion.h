#pragma once
#include "CoreMinimal.h"

struct FS47TripoCreature;

enum class ES47CrabMode : uint8 { Idle, Walk, Run };

struct FS47CrabPose
{
 bool bVisible=true;
 ES47CrabMode Mode=ES47CrabMode::Idle;
 // The owner's held clock. HeadYaw is relative to the body.
 double Clock=0, HeadYaw=0;
 // 0 stands upright, 1 is folded down on hands and feet. Limbs only step when fully folded.
 double Fold=1;
};

struct FS47CrabContacts
{
 bool bValid=false;
 // Hand left, hand right, foot left, foot right.
 bool bStep[4]={false,false,false,false};
 FVector AtUE[4]={FVector::ZeroVector,FVector::ZeroVector,FVector::ZeroVector,FVector::ZeroVector};
 double MaxPlantErrorCm=0;
};

// A tall body folded down on all fours, chest to the ground with elbows and knees above its back. It goes wherever
// its root is moved, also sideways: the limbs are planted on the ground and step after it, one at a time. Visual
// only; position, collision and senses belong to whoever moves the root. Needs the UE5 Mannequin bone names.
class FS47CrabMotion
{
public:
 FS47CrabContacts Apply(FS47TripoCreature& Creature,const FS47CrabPose& In);
private:
 struct FLimb
 {
  FVector Anchor=FVector::ZeroVector,Start=FVector::ZeroVector,Target=FVector::ZeroVector,Current=FVector::ZeroVector;
  bool bSwing=false,bCorrection=false;
  double SwingStart=0,SwingEnd=1,CorrectionTime=0;
 } Limbs[4];
 bool bInit=false;
 double LastClock=0,Phase=0,Motion=0,Run=0,HeadYaw=0,HeadTilt=0;
 FVector LastRoot=FVector::ZeroVector;
};
