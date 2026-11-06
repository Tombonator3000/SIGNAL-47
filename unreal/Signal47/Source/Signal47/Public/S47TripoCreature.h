#pragma once
#include "CoreMinimal.h"
#include "S47PaleMotion.h"
#include "S47CrabMotion.h"

class AS47Web;
class USceneComponent;
class UPoseableMeshComponent;
class US47Main;

namespace S47CreaturePreview { void Start(US47Main* M); }

namespace S47Roswell
{
 constexpr double HeightM=1.20, HeadM=1.08, ProbeM=1.12, DuckM=.42;
}

// Visual adapter for the user's skinned Tripo creatures. The existing
// threat controllers keep pathfinding, senses, collision and encounter state.
struct FS47TripoCreature
{
 TWeakObjectPtr<UPoseableMeshComponent> Mesh;
 TArray<TWeakObjectPtr<class UStaticMeshComponent>> Glints;
 static constexpr double EyeSide=4.9,EyeFront=12.3,EyeUp=3.9,EyeSize=.0042; // Component centimetres from the head bone; sphere scale.
 void AddEyeGlints(class AS47Web* W, UPoseableMeshComponent* C);
 static void StepEyeGlints(const FVector& EyeUE, double Daylight);
 static int32 ShownEyeGlints();
 TArray<FTransform> Rest;
 TArray<int32> Parents;
 TArray<FName> Names;
 TSharedPtr<FS47PaleMotion> PaleMotion;
 FS47PaleContacts Contacts;
 void ApplyPale(const FS47PalePose& Pose);
 TSharedPtr<FS47CrabMotion> CrabMotion;
 FS47CrabContacts CrabContacts;
 void ApplyCrab(const FS47CrabPose& Pose);
 bool Build(AS47Web* W, USceneComponent* Parent, const FString& Name,
  const TCHAR* AssetPath = TEXT("/Game/Licensed/TripoNPC/Animated/SK_Tripo_Pale.SK_Tripo_Pale"));
 void Apply(bool bShow, double HeadTurn, double Gait, double Phase, double Clock, bool bGrab, bool bListen);
 // A small, curious Roswell body. Feet stay planted as it ducks below a sill.
 void Peek(bool bShow, double Exposure, double HeadTurn, double Clock);
 bool Rendered() const;
 bool Ready() const { return Mesh.IsValid(); }
};
