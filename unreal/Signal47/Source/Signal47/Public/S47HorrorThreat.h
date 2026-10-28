#pragma once

#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Web.h"

class US47Main;
struct FHitResult;

// SC02 meeting 1: the pale reactive body at Caliche Junction. It owns its rendered parts,
// its senses and a small hand-made route graph outside the closed yard. The story host alone
// turns its measured facts into dialogue gates, a local retry and escape credit.
//
// Route graph (three metres, x/z), measured from the code that builds the site:
//   closed yard   x 7880..7900, z 529..544.2: the fence collider of Config/Props/substation.json
//                 (local [-10,10,-8,7.2] at (7890,536.2), yaw 180); the gates are inside it.
//   front         (7885,525.8) and the track (7885,520.8): S47Substation/S47EveningServiceDriveTest.
//   shed cover    (7878.5,541.1): beside the locked shed, Blender (6.5,4.9) +-1.5/1.2, inside the fence.
//   route A west  front, west apron, SW corner, west fence, shed cover; south to the west track.
//   route B east  shed cover, NW corner, north side, NE corner, east fence, SE corner, east track.
// Nothing in the graph crosses the yard, the control cabinet, the T02 culvert or its band post.
class FS47HorrorThreat
{
public:
 enum class EState : uint8 { Idle, Warn, Investigate, Search, Chase, Capture, Withdrawn };
 // One frame of measured input. The world adapter fills it; the self-test writes it by hand.
 struct FSense
 {
  bool bHeld = false;           // pause, modal panel, cinematic, busy world or restore
  bool bCanAppear = false;      // the post is out of the player's actual view
  bool bSeesPlayer = false;     // a clear sight ray from the body's eye to the player's eye
  bool bPlayerSeesBody = false; // the rendered body is on screen behind no opaque surface
  bool bDriving = false;        // the player is in SARO 07's cab
  TOptional<FVector2D> Sound;   // a real source heard this frame: footsteps or the truck
  FVector2D Player = FVector2D::ZeroVector; // on foot, or the cab while driving
  double TruckDistance = 1e9;   // body to the driven truck, only while driving
 };
 struct FNode { const TCHAR* Id; FVector2D At; };

 explicit FS47HorrorThreat(US47Main* InM);
 ~FS47HorrorThreat();
 bool Build();
 bool Ready() const { return bBuilt; }
 bool Unavailable() const { return bBuildFailed; }
 void Start(bool bInSeen); // a fresh meeting at the post; nothing is shown until the post is unseen
 void Reset();             // after a capture: the same post and checkpoint, no counter
 void Stop();
 void Step(double Dt);
 void Think(const FSense& In, double Dt);
 bool Active() const { return bActive; }
 bool Appeared() const { return bAppeared; }
 bool Gone() const { return bGone; }
 EState State() const { return S; }
 bool Seen() const { return bSeen; }
 bool Withdrawn() const { return bActive && S == EState::Withdrawn; }
 bool ConsumeCapture() { const bool B = bCaptureEvent; bCaptureEvent = false; return B; }
 FVector2D Position() const { return Pos; }
 FVector2D Checkpoint() const;
 FString CheckpointId() const;
 static bool InYard(const FVector2D& P, double Pad = 0);
 static FVector2D SiteCentre() { return FVector2D(7889, 528); }
 static const TArray<FNode>& Nodes();
 static const TArray<TPair<int32, int32>>& Edges();
 static const TCHAR* StateName(EState In);
 static bool SelfTest(TArray<FString>& Lines);
 bool NativeSelfTest(TArray<FString>& Lines); // opt-in isolated native collision/bounds probe

private:
 struct FParts;
 TUniquePtr<FParts> Parts;
 US47Main* M = nullptr;
 bool bBuilt = false, bBuildFailed = false, bActive = false, bAppeared = false, bGone = false;
 bool bSeen = false, bCaptureEvent = false, bChaseAfter = false, bWorld = false, bBreath = false, bGrounded = false;
 EState S = EState::Idle;
 FVector2D Pos = FVector2D::ZeroVector, Goal = FVector2D::ZeroVector, LastKnown = FVector2D::ZeroVector;
 FVector2D Look = FVector2D::ZeroVector, PathGoal = FVector2D::ZeroVector, LastPlayer = FVector2D::ZeroVector, SearchAt = FVector2D::ZeroVector;
 double Y = -1.1, BodyYaw = 0, HeadYaw = 0, StateT = 0, SeeAcc = 0, SeenAcc = 0, LostT = 0, DriveLostT = 0;
 double SinceSeen = 1e9, SinceWarn = 1e9, Replan = 0, Phase = 0, Gait = 0, Clock = 0, NoiseCool = 0, ChaseBreath = 0;
 double BlockedFor = 0;
 int32 PostNode = 2, CheckNode = 6, RetreatNode = 8, SearchStep = 0;
 bool bHavePlayer = false;
 static constexpr int32 JointCount = 10; // torso, head, arms and forearms, thighs and shins
 double JointPose[JointCount] = {};
 int32 JointTick[JointCount] = {};
 TArray<FVector2D> Path;
 TArray<bool> NodeOk, EdgeOk;
 TArray<FS47Rect> Near;
 TArray<FS47Rect> NativeHints; // broad-phase hints only; the actual capsule decides native clearance
 struct FNativeObstacle { TWeakObjectPtr<class UPrimitiveComponent> Part; int32 Instance = INDEX_NONE; };
 TArray<FNativeObstacle> NativeObstacles;
 void Enter(EState Next);
 void Warn(const FVector2D& At, bool bChase);
 bool Move(const FVector2D& Target, double Speed, double Dt, double StopDist, bool bVehicle = false);
 void TurnBody(double Yaw, double Rate, double Dt);
 bool Clear(const FVector2D& A, const FVector2D& B) const;
 bool Sampled(const FVector2D& A, const FVector2D& B) const;
 bool InCollider(const FVector2D& P, double Pad) const;
 TArray<FVector2D> Plan(const FVector2D& From, const FVector2D& To) const;
 static TArray<int32> Route(int32 From, int32 To, const TArray<bool>& Allowed, const TArray<bool>* EdgeMask = nullptr);
 void Validate();
 void RefreshNavigation();
 bool PhysicalSegment(const FVector2D& From, const FVector2D& To, FHitResult* OutHit = nullptr) const;
 bool PhysicalStep(const FVector2D& To);
 bool ContactClear(const FVector2D& To) const;
 bool Held() const;
 bool InSite() const;
 bool SeesPlayer() const;
 bool BodyInView() const;
 bool PostHidden() const;
 TOptional<FVector2D> Heard(const FSense& In, double Dt);
 bool ClearLine(const FVector& From, const FVector& To, bool bToBody) const;
 FVector Head3() const { return FVector(Pos.X, Y + 1.86, Pos.Y); }
 FVector Chest3() const { return FVector(Pos.X, Y + 1.40, Pos.Y); }
 double Ground(const FVector2D& P, bool bFree) const;
 void Apply(bool bShow);
 void PlaySounds(double PhaseBefore);
};
