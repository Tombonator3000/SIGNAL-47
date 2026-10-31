#pragma once
#include "CoreMinimal.h"

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
 TArray<FTransform> Rest;
 TArray<int32> Parents;
 TArray<FName> Names;
 bool Build(AS47Web* W, USceneComponent* Parent, const FString& Name,
  const TCHAR* AssetPath = TEXT("/Game/Licensed/TripoNPC/Animated/SK_Tripo_Pale.SK_Tripo_Pale"));
 void Apply(bool bShow, double HeadTurn, double Gait, double Phase, double Clock, bool bGrab, bool bListen);
 // A small, curious Roswell body. Feet stay planted as it ducks below a sill.
 void Peek(bool bShow, double Exposure, double HeadTurn, double Clock);
 bool Rendered() const;
 bool Ready() const { return Mesh.IsValid(); }
};
