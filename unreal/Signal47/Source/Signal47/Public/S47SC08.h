#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47PaleBody.h"

class US47Main;
class UStaticMeshComponent;
class UTextRenderComponent;
struct FS47Task;
struct FS47Doc;

// SC08 "They point at you", 04:00 to 04:20 (Docs/Design/Story/TIDSLINJE_SC01_SC13_2026-10-29.md,
// HISTORIE_SC02_SC13_REVISJON.md). Chapter two's end waits for it (FS47Spine::HoldChapterEnd).
//
//   the reel     Earl leaves 1420 AM at four (S47Timeline::EarlOnDuty), so at 04:00 his dated copy of
//                the September 1981 reel is in the lobby: isolated tape, and the old voice says the
//                same words whatever is asked. His note: 1420 AM is not 1420.405 MHz.
//   the sheet    the array service sheet in the control room: A / ROW 1 WEST, B / ROW 1 EAST, REF / R1.
//   the test     stakes at A and B, the fixed red reference lamp at R1. The player stands at one, goes
//                to the other and a few steps back. The dishes follow the player, not the lamp: the
//                existing FS47DishesFollow, started through US47SaroNight's warning without a time away.
//   the meeting  the pale body comes in between the footings at the far end of row one. The field's
//                only way out is the array gate up the spine: the short way is to read its search and
//                cross to the spine, the long way round by the highway track and row two while it
//                searches row one. Out through the gate it stops at the fence and goes; the dishes hold
//                the place a moment and go home to 026. A capture starts the meeting again where it began.
//
// The field (walks.saro: fSpine x -2.6..0.6, fRow1 z -75.6..-72.4, fRow2 z -139.6..-136.4, fHighway
// x -28.6..-19.4; the gate x -1.7..-0.3 at z -20), three.js metres. Every node and edge is clear of the
// walks' colliders by 0.27 m (measured from the manifest).
class FS47SC08
{
public:
 enum class EStep : uint8 { Reel, Sheet, Field, Hunt, Done };
 enum class EBody : uint8 { Away, Still, Search, Chase, Grab, Fence, Gone };
 struct FFacts { bool bReel = false, bSheet = false, bFirst = false, bSecond = false, bBack = false, bEscaped = false; FString First; int32 Captures = 0; };
 struct FNode { const TCHAR* Id; FVector2D At; };

 explicit FS47SC08(US47Main* InM) : M(InM) {}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 S47J::FObj Save() const;
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
 int32 Following() const; // dishes on the player now (FS47DishesFollow)
 void TestComplete();
 static const TArray<FNode>& Nodes();
 static const TArray<TPair<int32, int32>>& Edges();
 static const TCHAR* BodyName(EBody In);
 static FVector2D PointA();
 static FVector2D PointB();
 static FVector2D Ref();
 static bool OutOfField(const FVector2D& At);
 static FS47Doc ReelDoc();
 static FS47Doc SheetDoc();
 static bool SelfTest(TArray<FString>& Lines);

private:
 US47Main* M = nullptr;
 EStep P = EStep::Reel;
 FFacts F;
 EBody B = EBody::Away;
 FS47PaleBody Pale;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Stakes;
 TArray<TWeakObjectPtr<UTextRenderComponent>> Signs;
 bool bBuilt = false, bShown = false, bHavePlayer = false, bRestart = false;
 FVector2D Pos = FVector2D::ZeroVector, LastKnown = FVector2D::ZeroVector, LastPlayer = FVector2D::ZeroVector, HuntFrom = FVector2D::ZeroVector, Turn = FVector2D::ZeroVector;
 double Y = 0, BodyYaw = 0, HeadYaw = 0, Gait = 0, Stride = 0, Clock = 0, T = 0, StateT = 0, SeeAcc = 0, LostT = 0, NoiseCool = 0, FadeT = -1;
 int32 Patrol = 0;
 TArray<int32> Path;
 void SetStep(EStep Next);
 void Enter(EBody Next);
 void BuildStakes();
 void ShowStakes(bool bOn);
 void StepField();
 void StartHunt(bool bRetry);
 void StepHunt(double Dt);
 void Think(double Dt);
 bool Walk(double Speed, double Dt);
 void RouteTo(int32 Node);
 int32 Nearest(const FVector2D& At) const;
 bool SeesPlayer() const;
 TOptional<FVector2D> Heard(double Dt);
 double Ground(const FVector2D& At) const;
 void Capture();
 void Finish();
 void Follow(bool bOn);
 FVector Eye() const { return FVector(Pos.X, Y + 1.86, Pos.Y); }
};
