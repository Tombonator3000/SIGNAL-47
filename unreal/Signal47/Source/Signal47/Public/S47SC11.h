#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47PaleBody.h"

class US47Main;
class USpotLightComponent;
struct FS47Doc;
struct FS47Task;

// New evening cases, SC11: collect the independent records, distinguish the local
// lamp from the search outside, and physically leave SARO. The original source
// becomes complete only after that escape. Legacy cases have no Spine gate.
class FS47SC11
{
public:
 enum class EStep : uint8 { Return, Collect, Hunt, Done };
 enum class EBody : uint8 { Approach, Search, Listen, Chase, Grab };
 struct FFacts
 {
  bool Receiver = false, Event = false, Hondo = false, Site11 = false;
  bool CheckedLocalLight = false, ObservedRoom = false, Escaped = false;
  int32 Captures = 0;
 };
 struct FNode { const TCHAR* Id; FVector2D At; };
 explicit FS47SC11(US47Main* InM) : M(InM) {}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 S47J::FObj Save() const;
 bool Step(double Dt);
 FString Label(const FString& Id) const;
 bool Use(const FString& Id);
 FString Objective() const;
 TArray<FS47Task> Tasks() const;
 EStep Phase() const { return P; }
 EBody Body() const { return B; }
 const FFacts& Facts() const { return F; }
 FVector2D BodyAt() const { return Pos; }
 bool BodyShown() const { return bShown && Pale.Rendered(); }
 bool HuntLive() const { return P == EStep::Hunt; }
 bool SourcesSecured() const { return F.Receiver && F.Event && F.Hondo && F.Site11; }
 bool OutsideLightStillOn() const;
 bool LocalLightOff() const;
 static constexpr double Arrive = 5 * 3600., RemoteReady = 5 * 3600. + 4 * 60;
 static FS47Doc ReceiverDoc(bool bComplete, const FString& EscapedAt = FString());
 static FS47Doc EventDoc();
 static FS47Doc HondoDoc();
 static FS47Doc Site11Doc();
 static const TArray<FNode>& Nodes();
 static const TArray<TPair<int32, int32>>& Edges();
 static int32 Room(const FVector2D& At);
 static bool SelfTest(TArray<FString>& Lines);
 // Explicit fixture for subsequent scenario contracts, never called by gameplay.
 void TestComplete();
private:
 friend class FS47SC11Contract;
 US47Main* M = nullptr;
 EStep P = EStep::Return;
 EBody B = EBody::Approach;
 FFacts F;
 FS47PaleBody Pale;
 FS47Light LocalLamp;
 TArray<TWeakObjectPtr<USpotLightComponent>> Beams;
 FVector2D Pos = FVector2D::ZeroVector, LastKnown = FVector2D::ZeroVector, LastPlayer = FVector2D::ZeroVector;
 FVector DriveStart = FVector::ZeroVector;
 TArray<int32> Path;
 TMap<FString, bool> LastDoors;
 bool bShown = false, bHavePlayer = false, bRestoreHunt = false, bLampCaptured = false, bDriveStarted = false, bSavedLampOff = false;
 float LampBefore = 0;
 double T = 0, StateT = 0, Clock = 0, Y = 0, Yaw = 0, HeadYaw = 0, Gait = 0, Stride = 0;
 double SeeT = 0, LostT = 0, ObserveT = 0, HeardCooldown = 0, DoorWait = 0, FadeT = -1;
 int32 Patrol = 0;
 FString WaitingDoor;
 FString EscapedAt;
 int32 RuntimeRoom(const FVector2D& At) const;
 void Refile();
 void SetStep(EStep Next);
 void BuildLights();
 void StepLights(double Dt);
 void ObserveRoom(double Dt);
 void StartHunt(bool bRetry);
 void StepHunt(double Dt);
 void Enter(EBody Next);
 void Capture();
 void Finish();
 void RouteTo(int32 Goal);
 int32 Nearest(const FVector2D& At) const;
 bool Walk(double Speed, double Dt);
 FString ClosedDoorTo(const FVector2D& Target) const;
 bool ClearSight(const FVector& From, const FVector& To) const;
 bool SeesPlayer() const;
 bool BeamSeesPlayer(int32 Index) const;
 TOptional<FVector2D> Heard(double Dt);
 bool AtEscape() const;
 bool ClearWalk(const FVector2D& From,const FVector2D& To) const;
 double Ground() const;
};
