#pragma once
#include "CoreMinimal.h"

class AS47Web;
class USceneComponent;
class UPoseableMeshComponent;
class US47Main;

namespace S47CreaturePreview { void Start(US47Main* M); }

// Visual adapter for the user's original skinned Tripo Tall One. The existing
// threat controllers keep pathfinding, senses, collision and encounter state.
struct FS47TripoCreature
{
 TWeakObjectPtr<UPoseableMeshComponent> Mesh;
 TArray<FTransform> Rest;
 TArray<int32> Parents;
 TArray<FName> Names;
 bool Build(AS47Web* W, USceneComponent* Parent, const FString& Name);
 void Apply(bool bShow, double HeadTurn, double Gait, double Phase, double Clock, bool bGrab, bool bListen);
 bool Rendered() const;
 bool Ready() const { return Mesh.IsValid(); }
};
