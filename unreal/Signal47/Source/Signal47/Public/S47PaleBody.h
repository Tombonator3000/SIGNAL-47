#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
#include "S47TripoCreature.h"

class AS47Web;
class UStaticMeshComponent;
class UBoxComponent;

// The SC02 pale body (Content/Signal47/HorrorSC02, six original meshes, ten parts) for a meeting
// outside the Caliche road group: the same parts and the same stop-motion pose as FS47HorrorThreat,
// hung under another scene group. Positions are three.js metres (x, feet y, z); yaws are UE yaws.
// FS47HorrorThreat keeps its own copy for SC02 until that meeting is moved onto this one.
struct FS47PaleBody
{
 enum class EPose : uint8 { Walk, Listen, Chase, Grab };
 FS47Obj Root;
 TWeakObjectPtr<UBoxComponent> RootComp;
 TWeakObjectPtr<UStaticMeshComponent> Torso, Head, ArmL, ForeL, ArmR, ForeR, ThighL, ShinL, ThighR, ShinR;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> All;
 FS47TripoCreature Creature;
 bool bBuilt = false, bFailed = false, bCollide = true;
 static constexpr int32 JointCount = 10;
 double JointPose[JointCount] = {};
 int32 JointTick[JointCount] = {};

 // Group: a web handle such as room.group. Prefix names the components and adopted nodes.
 bool Build(AS47Web* W, const TCHAR* Group, const TCHAR* Prefix);
 // Gait 0 standing to 1.4 striding, Phase the stride angle, Clock the held-pose clock.
 void Apply(bool bShow, const FVector& Feet, double BodyYaw, double HeadYaw, double Gait, double Phase, double Clock, EPose Pose);
 bool Owns(const class UPrimitiveComponent* C) const;
 bool Rendered() const;
};
