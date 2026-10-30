#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47PaleBody.h"

class US47Main;
class USpotLightComponent;
struct FS47Task;
struct FS47Doc;

// SC11 "SARO is searched", 05:00 to 05:10 (Docs/Design/Story/PRODUKSJON_SC01_SC13.md). Chapter four's
// end waits for it (FS47Spine::HoldChapterEnd). Back at SARO Reyes takes the receiver's event log and
// the sheet he started; at 05:04 Hondo and Site 11 come in on the telex. Then a light searches the
// building from outside while the pale body searches inside, in through the lab door, across the
// landing and the service door. Doors buy time; it opens them. The short way out: ring the
// operations office from the control-room phone and go out by the service door and the stairs to the
// truck while it listens there. The long way: the annex, the fire exit and round the north side past
// the lit windows. Standing in the searchlight tells it where you are. Using the truck drives away
// with the originals; a capture starts the siege again at the telex with everything kept.
//
// Rooms (three.js metres): 0 control x -5.9..5.9 z -4.4..4.6; 1 corridor z 4.6..6.6; 2 records;
// 3 outside (the landing, the stairs, the truck pad, the lab path). Every node and edge is clear of
// the walks' colliders by 0.27 m (measured from the manifest, 30 October 2026).
class FS47SC11
{
public:
 enum class EStep : uint8 { Drive, Collect, Telex, Siege, Done };
 enum class EBody : uint8 { Away, Search, Listen, Lured, Chase, Grab, Leave };
 struct FFacts { bool bOriginal = false, bSheet = false, bRemote = false, bEscaped = false, bLure = false; int32 Captures = 0; };
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
 const FFacts& Facts() const { return F; }
 EBody Body() const { return B; }
 FVector2D BodyAt() const { return Pos; }
 bool BodyShown() const { return bShown; }
 bool InLight() const { return bLit; }
 void TestComplete();
 static const TArray<FNode>& Nodes();
 static const TArray<TPair<int32, int32>>& Edges();
 static const TCHAR* BodyName(EBody In);
 static int32 Room(const FVector2D& At);
 static FS47Doc RemoteDoc();
 static bool SelfTest(TArray<FString>& Lines);

private:
 US47Main* M = nullptr;
 EStep P = EStep::Drive;
 FFacts F;
 EBody B = EBody::Away;
 FS47PaleBody Pale;
 TWeakObjectPtr<USpotLightComponent> Search;
 bool bShown = false, bHavePlayer = false, bRestart = false, bLureRinging = false, bLit = false;
 FVector2D Pos = FVector2D::ZeroVector, LastKnown = FVector2D::ZeroVector, LastPlayer = FVector2D::ZeroVector;
 double Y = 0, BodyYaw = 0, HeadYaw = 0, Gait = 0, Stride = 0, Clock = 0, T = 0, StateT = 0, SeeAcc = 0, LostT = 0, NoiseCool = 0, FadeT = -1;
 double TelexT = 0, LightT = 0, LitFor = 0, LureT = 0, DoorT = 0;
 int32 Patrol = 0, TelexLine = 0;
 TArray<int32> Path;
 TMap<FString, bool> LastDoors;
 void SetStep(EStep Next);
 void Enter(EBody Next);
 void StartSiege(bool bRetry);
 void StepSiege(double Dt);
 void StepLight(double Dt);
 void Think(double Dt);
 bool Walk(double Speed, double Dt);
 void RouteTo(int32 Node);
 int32 Nearest(const FVector2D& At) const;
 bool EdgeOpen(int32 A, int32 Bn) const;
 bool SeesPlayer() const;
 TOptional<FVector2D> Heard(double Dt);
 double Ground(const FVector2D& At) const;
 void Capture();
 void Finish();
 FVector Eye() const { return FVector(Pos.X, Y + 1.86, Pos.Y); }
};
