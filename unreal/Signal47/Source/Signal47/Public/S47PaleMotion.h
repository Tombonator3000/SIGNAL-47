#pragma once
#include "CoreMinimal.h"

struct FS47TripoCreature;
enum class ES47PaleMotion : uint8 { Idle, Listen, Walk, Run, Grab };
// Hunter is the SC02 pale. Tall stoops and goes on long slow strides with its arms hanging and its head out in front.
enum class ES47PaleStyle : uint8 { Hunter, Tall };
struct FS47PalePose
{
 bool bVisible=true;
 ES47PaleMotion Mode=ES47PaleMotion::Idle;
 ES47PaleStyle Style=ES47PaleStyle::Hunter;
 // Existing controller clock holds with menus/cinematics. No separate wall clock.
 double Clock=0, HeadYaw=0, GrabProgress=-1;
 // Actual player chest target in Unreal cm, supplied only by a live controller/explicit preview.
 TOptional<FVector> GrabTargetUE;
 double LegacyGait=0, LegacyPhase=0; // unchanged fallback when the actual rig is unsupported
};
struct FS47PaleContacts
{
 bool bValid=false,bLeft=false,bRight=false;
 double MaxPlantErrorCm=0,MaxPlantAngleDeg=0,MaxSoleProbeErrorCm=0;
 FVector AtUE[2]={FVector::ZeroVector,FVector::ZeroVector};
 FQuat RotationUE[2]={FQuat::Identity,FQuat::Identity};
};

// Visual-only per-creature state. Original rest pose, skin, root and collision remain owned elsewhere.
class FS47PaleMotion
{
public:
 FS47PaleContacts Apply(FS47TripoCreature& Creature,const FS47PalePose& In);
private:
 struct FFoot
 {
  FVector Anchor=FVector::ZeroVector,Start=FVector::ZeroVector,Target=FVector::ZeroVector,Current=FVector::ZeroVector;
  FQuat AnchorQ=FQuat::Identity,StartQ=FQuat::Identity,TargetQ=FQuat::Identity,CurrentQ=FQuat::Identity;
  bool bSwing=false,bCorrection=false,bCorrectionLift=false;
  double CorrectionTime=0,SwingStart=0,SwingEnd=1;
 } Feet[2];
 bool bInit=false;
 double LastClock=0,Phase=0,Motion=0,Run=0,HeadYaw=0,Reach=0,HeadTilt=0;
 FVector LastRoot=FVector::ZeroVector;
 TOptional<FVector> LastGrabTargetUE;
};
