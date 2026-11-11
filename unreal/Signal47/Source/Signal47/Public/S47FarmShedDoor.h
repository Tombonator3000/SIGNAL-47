#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47ShedSound.h"

class US47Main;
class AS47Web;
class USceneComponent;
class UStaticMeshComponent;

// WEEKLY-CASE-20261109, patch 2: the feed shed's door at the Alderete place (S47FarmBuildings site "shedB"), its own leaf on a
// hinge, as FS47FarmHouseDoors' are. Freeform only; FS47WeekCase drives it (its phases give the state and the mood).
//
//   Open    as it always stood: out on its hinge, held by a block (the baked door of SM_FarmShedB before patch 2).
//   Locked  shut, a chain through the hasp and a new padlock, locked on this side. E: "Try the door" (the chain rattles), then
//           "Knock". Shut, it stops the walker (a collider of its own, door:shed:feed) and closes the gap in the shed's wall
//           (S47FarmBuildings::SetShedDoorShut).
//   Broken  burst outward from inside: the leaf in pieces in the yard, the chain hanging from the staple, the padlock still locked.
//           Kept by the case's save (FS47WeekCase "door"), the rest of the week.
// The moods, while it is shut: Still (nothing, as by day), Rattle (phase one: uneven rattling inside; a knock is answered in the
// same rhythm after a long silence), Kicks (phase two: kicks at uneven pauses, the leaf bulging against the chain).
//
// The mesh is /Game/FarmBuildings/SM_FarmShedDoor (Tools/farmbuildings/sheds.py build_shed_door, its pivot at the hinge). Until it
// is imported, a plain leaf of boxes is drawn in the doorway over the baked one.
class FS47FarmShedDoor
{
public:
 enum class EState : uint8 { Open, Locked, Broken };
 enum class EMood : uint8 { Still, Rattle, Kicks };
 static const TCHAR* StateName(EState S);
 static const TCHAR* MoodName(EMood S);

 // ---- the pure rules (self-tested) ----
 // The hinge in the shed's own metres (sheds.py: the south wall's door, u 1.0 to 1.98, its hinge .01 in), and the leaf.
 static FVector2D HingeLocal(){return FVector2D(-1.49,-2.);}
 static constexpr double LeafWidth=.96,LeafHeight=1.9,LeafBottom=.05,OpenDegrees=68;
 // A knock is closed this long after its last rap.
 static constexpr double KnockClose=1.4;
 static bool ShutFor(EState S){return S==EState::Locked;}
 // The player's knock: the plan's rhythm from the seed (FS47WeekCase::FPlan::Rhythm, 0 to 3), rap times in seconds from the first.
 static TArray<double> Pattern(int32 Rhythm);
 // What answers from inside: the very same rhythm, from its own first rap.
 static TArray<double> AnswerFor(const TArray<double>& Raps);
 // The long silence before the first answer (the second comes in half of it), from the seed and the night.
 static double AnswerWaitFor(int32 Seed,int32 Night);
 // Seconds from one kick to the next (uneven), and when the first comes in the mood.
 static double KickGap(int32 Seed,int32 Night,int32 Index);
 static constexpr double FirstKick=3.6;
 // The doorway shut, three-space in the SARO area (the walker's collider).
 static FBox2D ClosedBox();
 static bool SelfTest(TArray<FString>& Lines);

 explicit FS47FarmShedDoor(US47Main* Main):M(Main){}
 ~FS47FarmShedDoor();
 void Plan(int32 Seed,int32 Night,int32 Rhythm);
 // What it should be now. bAnimate: a change to Broken is the breach as it happens (pieces fly, the splinter, the push back);
 // otherwise it is simply so (a load).
 void Set(EState State,EMood Mood,bool bAnimate);
 void Step(double Dt);
 // Quiet at once: whatever was due to sound is dropped, the knocks forgotten, the leaf still. The state stays.
 void Calm();
 // Gone, components and all.
 void Stop();
 EState State() const{return Now;}
 EMood Mood() const{return MoodNow;}
 bool Knocked() const{return bKnocked;}
 int32 Answers() const{return AnswerCount;}
 bool Waiting() const{return AnswerAt>=0;}
 bool Drawn() const;
 bool UsesModel() const{return bModel;}
 // The middle of the leaf at a metre up, three-space in the area the player is in.
 FVector Middle() const;
 FString StateText() const;
 // F10: a knock as if E were pressed with "Knock" (tried first if it was not).
 bool DevKnock();
private:
 struct FPend{double At=0;S47ShedSound::EKind Kind=S47ShedSound::EKind::Knock;float Gain=1,Rate=1,Lp=0;bool bInside=false;int32 Variant=0;};
 struct FPiece{TWeakObjectPtr<UStaticMeshComponent> C;FVector Rest=FVector::ZeroVector;double Yaw=0;FVector Size=FVector::OneVector;};
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<USceneComponent> Hinge,Lock;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> LeafParts,Links;
 TWeakObjectPtr<UStaticMeshComponent> Padlock,Shackle,Staple;
 TArray<FPiece> Pieces;
 FS47Obj Object;
 bool bBuilt=false,bModel=false,bColliderAdded=false,bInterAdded=false;
 EState Now=EState::Open;EMood MoodNow=EMood::Still;
 int32 Seed=0,Night=0,Rhythm=0;
 double T=0,MoodSince=0,Bulge=0,BulgeV=0,Jiggle=0,Swing=1,BrokeAt=-1,PushLeft=0;
 FVector2D PushDir=FVector2D::ZeroVector;
 // the knock
 bool bTried=false,bKnocked=false;
 double CaptureFrom=-1,LastRap=-1,AnswerAt=-1,HushUntil=0,NextRattle=0;
 TArray<double> Raps,Answer;
 int32 AnswerCount=0,KickIndex=0,Var=0;
 double NextKick=-1;
 TArray<FPend> Pending;
 FRandomStream Rand;
 bool Build();
 void Pose();
 void SetCollider();
 void Use();
 void Knock();
 void Breach();
 void Due(double After,S47ShedSound::EKind Kind,float Gain,float Rate=1.f,bool bInside=false,float Lp=0.f);
 FVector Inside() const;
 FVector RoadOf(double LocalX,double LocalY) const;
 FVector AreaOf(const FVector& Road) const;
 void StepMood(double Dt);
 void StepSounds();
};
