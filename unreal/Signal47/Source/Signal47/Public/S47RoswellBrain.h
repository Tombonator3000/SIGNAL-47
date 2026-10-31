#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"

// Tom, 31 October 2026: a curious, shy Roswell is an actual moving observer,
// not a silhouette that pops away. This controller owns no world objects or saves.
// Positions/directions use the game's metre-based three.js coordinates, Y up.
class SIGNAL47_API FS47RoswellBrain
{
public:
 enum class EMode : uint8 { Stand, Curious, Hunt };
 enum class EState : uint8 { Idle, Listen, Stalk, Peek, Hide, Flee, Hunt, Search, Contact };
 struct FPoint
 {
  FString Id;
  FVector Pos = FVector::ZeroVector; // planted feet, on the real ground or roof
  bool bCover = false, bRoof = false, bWindow = false;
 };
 struct FNoise { FVector At = FVector::ZeroVector; double Loudness = 1.; };
 struct FInput
 {
  bool bHeld = false, bPlayerValid = true, bFlashlight = false;
  bool bCanContact = true; // a visible driver is protected by the cab even if a sight ray passes glass
  FVector PlayerFeet = FVector::ZeroVector, PlayerEye = FVector::ZeroVector;
  FVector PlayerForward = FVector(0,0,-1);
  // Measured movement, not a key held while the player is blocked by a wall.
  double PlayerSpeed = 0.;
  TOptional<FNoise> Noise;
 };
 struct FWorld
 {
  TArray<FPoint> Points;
  // A complete route, omitting From and ending at To. Empty means no route;
  // no direct fallback is attempted when this callback rejects a route.
  TFunction<TArray<FVector>(const FVector&,const FVector&)> FindPath;
  // False means deferred by the shared frame budget, not an unreachable goal.
  TFunction<bool()> PlanningReady;
  // Return the physically supported foot position for this short movement.
  // Empty rejects the step; never bypass CanStand/CanMove after grounding it.
  TFunction<TOptional<FVector>(const FVector&,const FVector&)> GroundedStep;
  TFunction<bool(const FVector&,const FVector&)> CanMove;
  TFunction<bool(const FVector&)> CanStand;
  // Opaque world geometry blocks a ray; glass may be clear. Ignore all the
  // small aliens themselves in this adapter, never the walls/closed doors.
  TFunction<bool(const FVector&,const FVector&)> Sight;
  // 0 is free. Values >=1 reserve the point for another nearby individual.
  TFunction<double(const FVector&)> Occupancy;
 };

 void Start(EMode InMode, const FVector& At, double Facing, int32 Seed);
 void SetMode(EMode InMode);
 S47J::FObj Save() const;
 bool Restore(const S47J::FObj& Saved);
 void Tick(double Dt, const FInput& In, const FWorld& World);

 EMode Mode = EMode::Curious;
 EState State = EState::Idle;
 FVector Pos = FVector::ZeroVector, Velocity = FVector::ZeroVector;
 FVector LastKnownPlayer = FVector::ZeroVector, LookTarget = FVector::ZeroVector;
 double Yaw = 0, HeadYaw = 0, Crouch = 0, DistanceTravelled = 0;
 double Age = 0, StateAge = 0, MemoryAge = 0, Noticed = 0;
 bool bHasPlayerMemory = false, bSeesPlayer = false, bPlayerSeesMe = false;
 bool bContact = false, bStandDone = false, bAtGoal = true;
 bool bPlanningDeferred = false;
 FString TargetId;

 FVector Eye() const { return Pos+FVector(0,1.08-.42*Crouch,0); }
 const TArray<FVector>& Route() const { return Path; }
 static const TCHAR* ModeName(EMode Value);
 static const TCHAR* StateName(EState Value);
 static bool SelfTest(TArray<FString>& Lines);

private:
 FRandomStream Random;
 TArray<FVector> Path;
 int32 NextPoint = 0, ChoiceCursor = 0;
 double DecisionAt = 0, RepathAt = 0, ContactCool = 0, HeardAge = 1e6, LastListenAt = -10.;
 double DesiredCrouch = 0, DesiredSpeed = 0;
 bool bTargetCover = false, bTargetRoof = false, bTargetWindow = false;
 FVector Goal = FVector::ZeroVector, LastPlayerEye = FVector::ZeroVector;

 void Step(double Dt, const FInput& In, const FWorld& World);
 void Enter(EState NewState);
 bool Plan(const FVector& To, const FString& Id, const FWorld& World);
 bool Choose(const FInput& In, const FWorld& World, bool bFlee, bool bPreferCover);
 void Move(double Dt, const FWorld& World);
 bool Seen(const FInput& In, const FWorld& World) const;
 bool Watched(const FInput& In, const FWorld& World) const;
 TOptional<FVector> Heard(const FInput& In, const FWorld& World) const;
};
