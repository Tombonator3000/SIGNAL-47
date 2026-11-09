#pragma once
#include "CoreMinimal.h"
#include "S47PaleMotion.h"
#include "S47CrabMotion.h"
#include "S47CreatureClips.h"

class AS47Web;
class USceneComponent;
class UPoseableMeshComponent;
class US47Main;

namespace S47CreaturePreview { void Start(US47Main* M); }

// Tom's Tripo bodies of 7 November and what they replace. Licensed lies outside Git: FS47TripoCreature::Build wears the old
// one in a build without the new, with one line in the log. Where each body's eyes are: S47EyeGlint, measured per body with
// Tools/triponpcs/measure_eyes.py. Story's pale is none of these.
namespace S47KindBody
{
 inline constexpr const TCHAR* Old=TEXT("/Game/Licensed/TripoNPC/Animated/SK_Tripo_Pale.SK_Tripo_Pale");
 inline constexpr const TCHAR* Pale=TEXT("/Game/Licensed/TripoNPC/Animated/SK_Tripo_PaleOne.SK_Tripo_PaleOne");
 inline constexpr const TCHAR* Crab=Pale; // until it has crab clips of its own
 inline constexpr const TCHAR* PaleOneClips=TEXT("/Game/Licensed/TripoNPC/Animated/A_PaleOne_");
 inline constexpr const TCHAR* Tall=TEXT("/Game/Licensed/TripoNPC/Animated/SK_Tripo_TallOne.SK_Tripo_TallOne");
 inline constexpr const TCHAR* TallOneClips=TEXT("/Game/Licensed/TripoNPC/Animated/A_TallOne_");
 inline constexpr const TCHAR* OldRoswell=TEXT("/Game/Licensed/TripoNPC/Roswell/SK_Tripo_Roswell.SK_Tripo_Roswell");
 inline constexpr const TCHAR* Roswell=TEXT("/Game/Licensed/TripoNPC/Roswell/SK_Tripo_Roswell2.SK_Tripo_Roswell2");
 inline constexpr const TCHAR* RoswellClips=TEXT("/Game/Licensed/TripoNPC/Roswell/A_Roswell2_");
}

namespace S47Roswell
{
 constexpr double HeightM=1.20, HeadM=1.08, ProbeM=1.12, DuckM=.42;
 // Speed over time while sneaking: a few steps, then near still. Averages about .7.
 inline double StalkPulse(double Age)
 {
  const double U=FMath::Frac(Age/1.9);
  return U<.62?1.5*FMath::Sqrt(FMath::Sin(UE_DOUBLE_PI*U/.62)):.04;
 }
}

// Visual adapter for the user's skinned Tripo creatures. The existing
// threat controllers keep pathfinding, senses, collision and encounter state.
struct FS47TripoCreature
{
 TWeakObjectPtr<UPoseableMeshComponent> Mesh;
 // The dot in each eye (S47EyeGlint), children of the head bone: they turn with it and hide with the body.
 TArray<TWeakObjectPtr<class UStaticMeshComponent>> Glints;
 TArray<FTransform> Rest;
 TArray<int32> Parents;
 TArray<FName> Names;
 TSharedPtr<FS47PaleMotion> PaleMotion;
 FS47PaleContacts Contacts;
 void ApplyPale(const FS47PalePose& Pose);
 TSharedPtr<FS47CrabMotion> CrabMotion;
 FS47CrabContacts CrabContacts;
 void ApplyCrab(const FS47CrabPose& Pose);
 // A body with its own clips (FS47ClipBank): standing, walking and acts come from them, running and seizing stay procedural.
 TSharedPtr<FS47ClipBank> Clips;
 TSharedPtr<FS47ClipMotion> ClipMotion;
 bool bOnProcedural=false;
 double ProcFadeLeft=0,ProcFadeClock=0;
 TArray<FTransform> ProcFadeFrom;
 void PlayClips(const FS47ClipPose& Pose);
 // A procedural motion takes over from a clip: from the pose it was in. Call the step after it has written its pose.
 void BeginProcFade(double Clock);
 void StepProcFade(double Clock);
 bool Build(AS47Web* W, USceneComponent* Parent, const FString& Name,
  const TCHAR* AssetPath = TEXT("/Game/Licensed/TripoNPC/Animated/SK_Tripo_Pale.SK_Tripo_Pale"));
 void Apply(bool bShow, double HeadTurn, double Gait, double Phase, double Clock, bool bGrab, bool bListen);
 // A small, curious Roswell body. Feet stay planted as it ducks below a sill.
 void Peek(bool bShow, double Exposure, double HeadTurn, double Clock);
 bool Rendered() const;
 bool Ready() const { return Mesh.IsValid(); }
};
