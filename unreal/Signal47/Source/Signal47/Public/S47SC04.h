#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47PaleBody.h"

class US47Main;
struct FS47Task;
struct FS47Doc;

// SC04 "Secure the originals", 02:55 to 03:15 (Docs/Design/Story/TIDSLINJE_SC01_SC13_2026-10-29.md,
// HISTORIE_SC02_SC13_REVISJON.md). After SC03 the deputy follows Reyes to SARO and is called back to
// Ortiz's wagon at the gate. Reyes collects the originals for the missing-person report from the
// ordinary places (the schedule on the ops terminal, the strip off the printer, the shift journal on
// the desk, the run binder he filed at 20:30) and the annex meeting starts when the binder is taken:
// Dale's door fragment from the fire exit, word for word, then the pale body comes in and searches.
//
// The annex (S47Annex.h, Content/WebData/manifest.json walks.saro), three.js metres:
//   corridor x -5.75..1.75, z 5.05..6.35; fire exit at its west end (x -6.3), stoop and ramp outside.
//   records room x -5.75..-1.25, z 7.05..10.25, its door at x -4.4..-3.4; work table x -4.5..-2.8, z 8.2..9.
//   control-room door x -2.9..-1.9, z 4.7. The operations office is the closed door at the east end.
// The corridor has no loop, so the story's two answers are fitted to it: keep the records door shut
// and stay still while it passes (the long way, waiting), or ring the operations office from the
// records-room wall phone and take the short way past its back while it listens at that door.
// The safe place is the control room with its door shut. A capture starts the annex again at the
// binder with everything collected kept; the clock does not move.
class FS47SC04
{
public:
 enum class EStep : uint8 { Drive, Collect, Hunt, Calm, Done };
 struct FFacts { bool bSchedule = false, bStrip = false, bJournal = false, bBinder = false, bEscaped = false, bLure = false; int32 Captures = 0; };
 enum class EBody : uint8 { Away, Door, Enter, Search, Listen, Lured, Chase, Grab, Leave };

 explicit FS47SC04(US47Main* InM) : M(InM) {}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 S47J::FObj Save() const;
 // Runs while the spine holds SC04; returns true once the control room is locked and calm.
 bool Step(double Dt);
 FString Label(const FString& Id) const;
 bool Use(const FString& Id);
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 EStep Phase() const { return P; }
 const FFacts& Facts() const { return F; }
 EBody Body() const { return B; }
 FVector2D BodyAt() const { return Pos; }
 bool BodyShown() const { return bShown; }
 FVector BodyHead() const { return FVector(Pos.X,Y+1.8,Pos.Y); }
 bool HuntLive() const { return P == EStep::Hunt; }
 bool DoorOccupied(const FString& Id) const;
 // Contract fixture for later scenarios' tests: SC04 counted as done without playing it.
 void TestComplete();
 // Explicit spatial fixture and readbacks for the existing opt-in SC04 door contract.
 void TestPlaceBody(const FVector2D& At, double Facing);
 bool TestNativeBodyRendered() const { return Pale.Creature.Ready() && Pale.Rendered(); }
 bool TestSeesPlayer() const { return SeesPlayer(); }
 bool TestPlayerReachable() const;
 static FS47Doc StripDoc(const FString& Taken, const FString& Restored = FString(), const FString& Torn = FString());
 static FS47Doc ScheduleDoc();
 static bool SelfTest(TArray<FString>& Lines);
 // Graph for tests: three.js x/z.
 struct FNode { const TCHAR* Id; FVector2D At; };
 static const TArray<FNode>& Nodes();
 static const TArray<TPair<int32, int32>>& Edges();
 static const TCHAR* BodyName(EBody In);
 static int32 Room(const FVector2D& At); // 0 control, 1 corridor, 2 records, 3 outside

private:
 US47Main* M = nullptr;
 EStep P = EStep::Drive;
 FFacts F;
 EBody B = EBody::Away;
 FS47PaleBody Pale;
 FVector2D Pos = FVector2D::ZeroVector, LastKnown = FVector2D::ZeroVector, LastPlayer = FVector2D::ZeroVector;
 double Y = 0, BodyYaw = 0, HeadYaw = 0, Gait = 0, Stride = 0, Clock = 0, T = 0, StateT = 0, SeeAcc = 0, LostT = 0, CalmT = 0, NoiseCool = 0;
 int32 Patrol = 0;
 TArray<int32> Path;
 bool bShown = false, bHavePlayer = false, bVoice = false, bBanged = false, bLureRinging = false, bRestartHunt = false, bListened = false;
 double LureT = 0, FadeT = -1, RefugeT = 0, BlockedT = 0;
 bool bRefugeRetreat = false;
 int32 RetreatGoal = 0;
 TOptional<FVector2D> PendingNoise; // a door moved before the body was in: it starts there
 TMap<FString, bool> LastDoors;
 double Ground() const;
 void SetStep(EStep Next);
 void Enter(EBody Next);
 void StartHunt(bool bRetry);
 void StepHunt(double Dt);
 void Think(double Dt);
 bool Walk(double Speed, double Dt);
 void RouteTo(int32 Node);
 int32 Nearest(const FVector2D& At) const;
 bool EdgeOpen(int32 A, int32 B) const;
 bool Reachable(int32 Goal) const;
 FVector2D BodyExtent() const;
 bool SeesPlayer() const;
 void LogSight() const;
 TOptional<FVector2D> Heard(double Dt);
 bool PlayerSafe() const;
 bool PhysicalStep(const FVector2D& To) const;
 bool ClearOfClosedDoors() const;
 bool PlayerInControl() const;
 void Capture();
 void Finish();
 void Sounds(double Before);
 FVector Eye() const { return FVector(Pos.X, Y + 1.86, Pos.Y); }
};
