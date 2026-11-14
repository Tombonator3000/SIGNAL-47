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
  // SNEAK-20261108: how low the player keeps, 0 standing to 1 crouched (AS47Walker::Sneak). Seen and heard from less far (S47Sneak.h).
  double PlayerQuiet = 0.;
  TOptional<FNoise> Noise;
  // WEATHER-20261108: how far sounds carry now, 1 on a still night; a strong wind covers steps and breath (down to about half).
  double HearScale = 1.;
  // M5 (13 November): how far it sees the player now, 1 at a half moon; a full moon further, no moon or blowing sand less far, a lit
  // lamp never less (S47Weather::SightScale, from FS47RoswellGroup in a live Freeform game). Times S47Sneak's reach, never touching it.
  double SightScale = 1.;
  // A visit's attention (FS47VisitAttention): stop and face this, a lamp or a sound, easing back from it at HoldBack m/s.
  // Comes from outside on each tick, so nothing of it is in a save and Story never sets it. Never changes the mode.
  TOptional<FVector> HoldLook;
  double HoldBack = 0.;
  bool bHoldHeard = false; // a sound, not a lamp: it now also knows where to look next
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
 /** A hunt's pace (m/s): once it sees the player, and searching where it last did. FS47Fatigue keeps a tired run above the first. */
 static constexpr double HuntSeenSpeed=2.8,HuntSearchSpeed=1.15;
 static const TCHAR* ModeName(EMode Value);
 static const TCHAR* StateName(EState Value);
 static bool SelfTest(TArray<FString>& Lines);

 // The body's sneak is a clip with its own rhythm of steps, set by the owner every tick: it goes steadily at the same
 // pace as the steps and stops, and stops now and then to listen, in place of StalkPulse.
 bool bSteadySneak = false;
private:
 FRandomStream Random;
 TArray<FVector> Path;
 int32 NextPoint = 0, ChoiceCursor = 0;
 double DecisionAt = 0, RepathAt = 0, ContactCool = 0, HeardAge = 1e6, LastListenAt = -10.;
 double DesiredCrouch = 0, DesiredSpeed = 0;
 bool bTargetCover = false, bTargetRoof = false, bTargetWindow = false;
 FVector Goal = FVector::ZeroVector, LastPlayerEye = FVector::ZeroVector;
 // PHYSICS-20261108: the last sound it heard from somewhere other than where the player is (a thrown stone, a can), and how long
 // ago. Heard while it could see the player, it is taken for them as soon as it no longer does (bLurePending); a curious one also
 // goes to look there (bGoLook). Not saved: a noise lasts a moment.
 FVector NoiseAt = FVector::ZeroVector;
 double NoiseAge = 1e6;
 bool bLurePending = false, bGoLook = false;
 // STONE-LURE-20261109: which of GoLook's places (the spot, then a little short of it) is tried next. Kept across ticks, as
 // ChoiceCursor is: the group grants one route a tick, so a spot with no route (a stone by a wall) has to give way to the next one
 // on the following tick, not be asked for again each tick until the sound is too old.
 int32 LookTry = 0;

 void Step(double Dt, const FInput& In, const FWorld& World);
 void Enter(EState NewState);
 bool Plan(const FVector& To, const FString& Id, const FWorld& World);
 bool Choose(const FInput& In, const FWorld& World, bool bFlee, bool bPreferCover);
 bool GoLook(const FVector& At, const FWorld& World);
 void Move(double Dt, const FWorld& World);
 bool Seen(const FInput& In, const FWorld& World) const;
 bool Watched(const FInput& In, const FWorld& World) const;
 TOptional<FVector> Heard(const FInput& In, const FWorld& World) const;
};
