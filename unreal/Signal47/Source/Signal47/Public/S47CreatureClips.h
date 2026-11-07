#pragma once
#include "CoreMinimal.h"
#include "UObject/StrongObjectPtr.h"

class UAnimSequence;
class USkeletalMesh;
class UPoseableMeshComponent;
struct FS47TripoCreature;
struct FS47PaleContacts;

// The clips Tom's Tripo bodies came with; each body has its own selection (S47KindBody). Observe, LookAround, Relax and Shh
// are the Tall One's, Angry and Frustrated the Pale One's, Afraid the small one's. Num: none.
enum class ES47Clip : uint8 { Idle, Walk, Climb, JumpDown, Angry, Frustrated, Sneak, Listen, Run, Fall, Afraid, Observe, LookAround, Relax, Shh, Sit, Jump, Freeze, Num };

// One clip-wearing body's request for this frame. Where the root is moved stays with whoever moves it, as for the
// procedural motions: a gait's phase is read from how far the root has really gone, so its feet cannot glide.
struct FS47ClipPose
{
 bool bVisible=true;
 // The owner's held clock. HeadYaw and HeadPitch are relative to the body.
 double Clock=0,HeadYaw=0,HeadPitch=0;
 // What it plays standing and moving. Num: the bank's own idle (or listen), and its walk.
 ES47Clip Stand=ES47Clip::Num,Move=ES47Clip::Num;
 // Plays over either.
 ES47Clip Act=ES47Clip::Num;
};

// A body's clips, shared by every creature wearing that mesh, read out frame by frame as they are first needed so that
// no one frame pays for a whole clip. Works cooked and in the editor; one line in the log if a clip is missing.
class FS47ClipBank
{
public:
 static constexpr double Rate=24;
 // Null if this mesh has no walk and nothing to stand in under the prefix: the body then keeps its procedural motions.
 static TSharedPtr<FS47ClipBank> For(const USkeletalMesh* Mesh,const FS47TripoCreature& Creature,const TCHAR* Prefix);
 bool Has(ES47Clip Clip) const;
 double Seconds(ES47Clip Clip) const;
 // What this bank stands in when nothing else is asked.
 ES47Clip Standing() const { return Has(ES47Clip::Idle)?ES47Clip::Idle:ES47Clip::Listen; }
 // Local transforms of all bones.
 void Sample(ES47Clip Clip,double Time,TArray<FTransform>& Out);
 // The clip's own travel over its length in component space: the root's speed that fits it is this over Seconds.
 FVector Travel(ES47Clip Clip);
 // Where its hips begin, off their place in the rest pose, along the ground.
 FVector StartOffset(ES47Clip Clip);
 // A clip that walks: read for its feet and how far one round of it goes.
 bool IsGait(ES47Clip Clip) const { return Clip==ES47Clip::Walk||Clip==ES47Clip::Sneak||Clip==ES47Clip::Run; }
 double GaitCycleCm(ES47Clip Clip) const;
 // The speed its steps were made for, in metres a second.
 double GaitSpeedMps(ES47Clip Clip) const;
 // Is this foot (0 left, 1 right) on the ground at this point (0..1) of the gait?
 bool GaitStance(ES47Clip Clip,int32 Foot,double U) const;
private:
 struct FRow { FQuat4f Q; FVector3f T; };
 struct FClip
 {
  TStrongObjectPtr<UAnimSequence> Seq;
  double Length=0,Cycle=0;
  int32 Frames=0;
  TArray<FRow> Rows;
  TBitArray<> Filled;
  TArray<bool> Stance[2];
 };
 FClip Clips[int32(ES47Clip::Num)];
 TArray<int32> SkeletonIndex,Parents;
 TArray<FTransform> Rest;
 TArray<FName> Names;
 int32 PelvisBone=INDEX_NONE;
 FVector RestPelvis=FVector::ZeroVector;
 const FRow* RowsAt(FClip& Clip,int32 Frame);
 FVector BoneAt(FClip& Clip,int32 Frame,int32 Bone);
 void ReadGait(ES47Clip Which);
};

// Plays a bank on a poseable body, in place: the clip's travel is taken out of the pose and stands for the root's.
// Standing is one clip, moving another, an act plays over either. A change of clip is faded.
class FS47ClipMotion
{
public:
 void Apply(FS47TripoCreature& Creature,FS47ClipBank& Bank,const FS47ClipPose& In,FS47PaleContacts& Out);
 // Fade the next frame from what the mesh shows now, whoever put it there.
 void FadeFromShown(const UPoseableMeshComponent* Mesh);
 ES47Clip Playing() const { return Now; }
 double PlayTime() const { return ClipTime; }
 // The current act has come to its end, and holds its last frame until the owner moves on. Loops never end.
 bool ActDone() const { return bActDone; }
 // Component space cm/s the owner should move the root at to make this act's travel true; zero if it stays on the spot.
 FVector ActRootVelocity(FS47ClipBank& Bank) const;
 // These go somewhere, and only make sense if the root goes with them. Any other act stays on the spot.
 static bool CarriesRoot(ES47Clip Clip) { return Clip==ES47Clip::Climb||Clip==ES47Clip::JumpDown||Clip==ES47Clip::Fall; }
private:
 struct FPlant
 {
  FVector Anchor=FVector::ZeroVector,Slip=FVector::ZeroVector,StepFrom=FVector::ZeroVector;
  FQuat AnchorQ=FQuat::Identity,StepFromQ=FQuat::Identity;
  double Ramp=0,StepTime=-1;
  bool bPlanted=false,bHasTurn=false;
 } Plants[2];
 ES47Clip Now=ES47Clip::Num;
 bool bInit=false,bActDone=false;
 double ClipTime=0,GaitU=0,FadeLeft=0,LastClock=0,LookYaw=0,LookPitch=0,LookWeight=1,LookTilt=0,StillFor=1;
 FVector LastRoot=FVector::ZeroVector;
 TArray<FTransform> FadeFrom,Local;
};
