#include "S47SC11.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Docs.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Web.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Seen.h"
#include "S47Flow.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SC11, Log, All);

namespace
{
 constexpr double Arrive = 5 * 3600., TelexAt = 5 * 3600. + 4 * 60, TelexTime = 36;
 constexpr double WalkSpeed = 1.2, ChaseSpeed = 2.1, SeeTime = .35, LoseTime = 2.6, Reach = .9, GrabHold = .7;
 constexpr double SightRange = 16, SightCos = .342, LureCos = .87, WalkHear = 9, RunHear = 14, FadeTime = 1.2;
 constexpr double DoorTry = 1.2, LureTime = 14, ComeBack = 15, LightPeriod = 12, LightCone = 9, LitTime = .8;
 enum : int32 { NLabIn, NLabDoor, NLabPath, NWalk, NLanding, NStairTop, NStairFoot, NPadWest, NTruck, NEastIn, NCrEast, NCrNE, NCrCentre, NCrNW, NCrWest,
  NCrSouthW, NControlDoor, NCorrSouth, NCorrRec, NOffice, NRecIn, NRecEast, NRecW };
 const FVector OfficePhone(.45, 1.1, 6.6), TelexAtPos(-4.2, .9, 4.13), LightFrom(0, 6, -18);
 const FVector2D TelexStand(-4.2, 3.2);
 double YawTo(const FVector2D& D) { return FMath::Atan2(D.Y, D.X); }
 struct FGap { const TCHAR* Door; double X0, X1, Z0, Z1; };
 // The doors' zones (walks.saro) and the control room's north windows, always open to sight.
 const FGap Gaps[] = {{TEXT("exitDoor"), -6.9, -5.0, 5.25, 6.15}, {TEXT("recordsDoor"), -4.38, -3.42, 5.6, 7.7}, {TEXT("doorSouth"), -2.88, -1.92, 3.6, 5.8},
  {TEXT("doorEast"), 5.0, 7.1, 1.12, 2.08}, {TEXT("labDoor"), 12.0, 13.6, -1.7, -0.7}, {nullptr, -5.75, 5.75, -4.5, -4.0}};
 const double GapReach[] = {14, 7, 7, 14, 14, 0};
 bool Through(const FVector2D& A, const FVector2D& B, const FGap& R)
 {
  double T0 = 0, T1 = 1; const FVector2D D = B - A;
  auto Clip = [&T0, &T1](double P, double Q)
  {
   if (FMath::Abs(P) < 1e-12) return Q >= 0;
   const double Rt = Q / P;
   if (P < 0) { if (Rt > T1) return false; if (Rt > T0) T0 = Rt; }
   else { if (Rt < T0) return false; if (Rt < T1) T1 = Rt; }
   return true;
  };
  return Clip(-D.X, A.X - R.X0) && Clip(D.X, R.X1 - A.X) && Clip(-D.Y, A.Y - R.Z0) && Clip(D.Y, R.Z1 - A.Y);
 }
}

const TArray<FS47SC11::FNode>& FS47SC11::Nodes()
{
 static const TArray<FNode> N = {
  {TEXT("labIn"), FVector2D(14, -1.2)}, {TEXT("labDoor"), FVector2D(12.8, -1.2)}, {TEXT("labPath"), FVector2D(11.0, -1.2)}, {TEXT("walk"), FVector2D(9.5, -.2)},
  {TEXT("landing"), FVector2D(7.5, 1.6)}, {TEXT("stairTop"), FVector2D(10.2, 1.6)}, {TEXT("stairFoot"), FVector2D(10.2, 4.5)}, {TEXT("padWest"), FVector2D(7.2, 4.8)},
  {TEXT("truckDoor"), FVector2D(7.2, 7.5)}, {TEXT("eastDoorIn"), FVector2D(4.6, 1.6)}, {TEXT("crEast"), FVector2D(3.4, .3)}, {TEXT("crNE"), FVector2D(3.5, -1.4)},
  {TEXT("crCentre"), FVector2D(0, .3)}, {TEXT("crNW"), FVector2D(-3.5, -1.4)}, {TEXT("crWest"), FVector2D(-4.2, .3)}, {TEXT("crSouthW"), FVector2D(-2.4, 2.9)},
  {TEXT("controlDoor"), FVector2D(-2.4, 3.6)}, {TEXT("corridorSouth"), FVector2D(-2.4, 5.7)}, {TEXT("corridorRecords"), FVector2D(-3.9, 5.7)},
  {TEXT("office"), FVector2D(.4, 5.75)}, {TEXT("recordsIn"), FVector2D(-3.9, 7.5)}, {TEXT("recordsEast"), FVector2D(-2.3, 7.6)}, {TEXT("recordsWest"), FVector2D(-5.0, 7.7)}};
 return N;
}

const TArray<TPair<int32, int32>>& FS47SC11::Edges()
{
 static const TArray<TPair<int32, int32>> E = {{NLabIn, NLabDoor}, {NLabDoor, NLabPath}, {NLabPath, NWalk}, {NWalk, NLanding}, {NWalk, NStairTop}, {NLanding, NStairTop},
  {NStairTop, NStairFoot}, {NStairFoot, NPadWest}, {NPadWest, NTruck}, {NLanding, NEastIn}, {NEastIn, NCrEast}, {NCrEast, NCrNE}, {NCrEast, NCrCentre},
  {NCrNE, NCrNW}, {NCrCentre, NCrWest}, {NCrNW, NCrWest}, {NCrCentre, NCrSouthW}, {NCrWest, NCrSouthW}, {NCrSouthW, NControlDoor}, {NControlDoor, NCorrSouth},
  {NCorrSouth, NCorrRec}, {NCorrSouth, NOffice}, {NCorrRec, NRecIn}, {NRecIn, NRecEast}, {NRecIn, NRecW}};
 return E;
}

const TCHAR* FS47SC11::BodyName(EBody In)
{
 switch (In)
 {
 case EBody::Away: return TEXT("away"); case EBody::Search: return TEXT("search"); case EBody::Listen: return TEXT("listen");
 case EBody::Lured: return TEXT("lured"); case EBody::Chase: return TEXT("chase"); case EBody::Grab: return TEXT("grab"); default: return TEXT("leave");
 }
}

int32 FS47SC11::Room(const FVector2D& At)
{
 if (At.Y < 4.6 && At.Y > -4.4 && At.X > -5.9 && At.X < 5.9) return 0;
 if (At.Y < 6.6 && At.Y >= 4.6 && At.X > -6.0 && At.X < 2.0) return 1;
 if (At.Y < 10.5 && At.Y >= 6.6 && At.X > -5.9 && At.X < -1.2) return 2;
 return 3;
}

// ---------- saving ----------

void FS47SC11::Begin(const S47J::FObj& Saved)
{
 Stop();
 P = EStep::Drive; F = FFacts(); TelexT = 0; TelexLine = 0; bRestart = false;
 if (!Saved) return;
 P = EStep(FMath::Clamp(S47J::Int(Saved, TEXT("step")), 0, int32(EStep::Done)));
 F.bOriginal = S47J::Bool(Saved, TEXT("original")); F.bSheet = S47J::Bool(Saved, TEXT("sheet")); F.bRemote = S47J::Bool(Saved, TEXT("remote"));
 F.bEscaped = S47J::Bool(Saved, TEXT("escaped")); F.bLure = S47J::Bool(Saved, TEXT("lure")); F.Captures = FMath::Max(0, S47J::Int(Saved, TEXT("captures")));
 // Restart the printing, with both originals kept. Older restores could save Collect
 // with both flags set, leaving no usable collection action; recover those cases too.
 if (P == EStep::Telex || (P == EStep::Collect && F.bSheet && F.bOriginal))
 { P = EStep::Telex; F.bSheet = F.bOriginal = true; }
 if (P == EStep::Siege) bRestart = true;
}

S47J::FObj FS47SC11::Save() const
{
 auto O = S47J::New();
 S47J::Set(O, TEXT("step"), int32(P));
 S47J::Set(O, TEXT("original"), F.bOriginal); S47J::Set(O, TEXT("sheet"), F.bSheet); S47J::Set(O, TEXT("remote"), F.bRemote);
 S47J::Set(O, TEXT("escaped"), F.bEscaped); S47J::Set(O, TEXT("lure"), F.bLure); S47J::Set(O, TEXT("captures"), F.Captures);
 return O;
}

void FS47SC11::Stop()
{
 B = EBody::Away; bShown = false; Path.Reset(); FadeT = -1; bLureRinging = false; bLit = false;
 Pale.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk);
 if (USpotLightComponent* L = Search.Get()) L->SetVisibility(false);
}

void FS47SC11::SetStep(EStep Next)
{
 if (P == Next) return;
 UE_LOG(LogS47SC11, Display, TEXT("S47: SC11 step %d -> %d"), int32(P), int32(Next));
 P = Next;
 if (M) M->RequestAutosave(false);
}

void FS47SC11::TestComplete() { P = EStep::Done; Stop(); }

// ---------- the originals and the telex ----------

FS47Doc FS47SC11::RemoteDoc()
{
 FS47Doc D; D.Id = TEXT("sc11_remote"); D.Title = TEXT("Remote sheets, Hondo and Site 11"); D.Kind = TEXT("typed"); D.Stamp = TEXT("TELEX 05:04");
 D.Page = TEXT("TELEX 05:04  HONDO VALLEY RADIO STATION\nSOURCE H-1 ....... RANGE NEGATIVE\nFIRST LIMIT ...... 05:29:47\nREQUEST PROCEDURE\n\nTELEX 05:05  SITE 11\nSOURCE S-11 ...... RANGE NEGATIVE\nFIRST LIMIT ...... 05:29:47\nSAME DIRECTION AS SARO\n\n(Three stations, three sources, one time. Nobody asked for a procedure for this.)");
 D.Transcript = TEXT("The remote sheets off the telex at 05:04: Hondo's source H-1 and Site 11's source S-11, both with a negative range and the same first limit, 05:29:47, the same as SARO's.");
 return D;
}

FString FS47SC11::Label(const FString& Id) const
{
 if (P == EStep::Collect)
 {
  if (Id == TEXT("printer") && !F.bOriginal) return TEXT("Tear off the receiver original from the event log");
  if (Id == TEXT("logbook") && !F.bSheet) return TEXT("Take the event sheet you started");
 }
 if (P == EStep::Telex && Id == TEXT("telex") && TelexT >= TelexTime) return TEXT("Tear off the remote sheets");
 if (P == EStep::Siege)
 {
  if (Id == TEXT("phone") && !bLureRinging && B != EBody::Chase && B != EBody::Grab) return TEXT("Ring the operations office");
  if (Id == TEXT("truck")) return TEXT("Drive away with the originals");
 }
 return FString();
}

bool FS47SC11::Use(const FString& Id)
{
 if (Label(Id).IsEmpty() || !M || !M->Story || !M->UI) return false;
 AS47Story& S = *M->Story;
 if (Id == TEXT("printer"))
 {
  F.bOriginal = true; if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
  M->UI->Toast(TEXT("The receiver's event log, torn off at the last line. The original, not a copy."), 4);
 }
 else if (Id == TEXT("logbook"))
 {
  F.bSheet = true; if (M->Audio) M->Audio->Play(TEXT("paper0"), .45f);
  M->UI->Toast(TEXT("The event sheet you started at three. Your hand, your times."), 4);
 }
 else if (Id == TEXT("telex"))
 {
  F.bRemote = true; const FS47Doc D = RemoteDoc(); S.AddDoc(D);
  if (M->Audio) M->Audio->Play(TEXT("paper0"), .55f);
  M->UI->Document(D);
  S.Note(FString::Printf(TEXT("%s. Hondo and Site 11 on the telex. Three sources, the same first limit: 05:29:47."), *S.ClockText(false)));
  StartSiege(false);
 }
 else if (Id == TEXT("phone"))
 {
  F.bLure = true; bLureRinging = true; LureT = 0;
  if (M->Audio)
  {
   M->Audio->Play(TEXT("handset_up"), .5f);
   for (int32 I = 0; I < 4; ++I) { FS47Sound O; O.Gain = .9f; O.Lp = 1800.f; O.At = OfficePhone; O.When = 1.f + I * 3.2f; O.Dur = 2.4f; M->Audio->Sfx(TEXT("phoneRing"), O); }
  }
  M->UI->Toast(TEXT("You dial the operations office. Down the corridor, behind its door, the phone starts ringing."), 4);
  UE_LOG(LogS47SC11, Display, TEXT("S47: SC11 lure: the operations office rings"));
 }
 else if (Id == TEXT("truck")) { Finish(); return true; }
 if (P == EStep::Collect && F.bOriginal && F.bSheet)
 {
  S.SetClock(FMath::Max(S.Clock(), TelexAt)); TelexT = 0; TelexLine = 0;
  SetStep(EStep::Telex);
 }
 M->RequestAutosave(false);
 return true;
}

FString FS47SC11::Objective() const
{
 switch (P)
 {
 case EStep::Drive: return TEXT("SC11 // SARO: THE REMOTE SHEETS COME IN AT 05:04. GET THE ORIGINALS OUT");
 case EStep::Collect: return TEXT("SARO // THE ORIGINALS: THE RECEIVER'S EVENT LOG AND THE SHEET YOU STARTED");
 case EStep::Telex: return TelexT >= TelexTime ? TEXT("SARO // TEAR OFF THE REMOTE SHEETS AT THE TELEX") : TEXT("SARO // 05:04. THE TELEX IS PRINTING: HONDO, THEN SITE 11");
 case EStep::Siege: return TEXT("SARO // SOMETHING IS SEARCHING THE BUILDING. GET THE ORIGINALS TO THE TRUCK");
 default: return FString();
 }
}

TArray<FS47Task> FS47SC11::Tasks() const
{
 TArray<FS47Task> Out;
 Out.Add({ TEXT("Drive back to SARO"), P != EStep::Drive });
 if (P == EStep::Drive) return Out;
 Out.Add({ TEXT("Tear off the receiver original"), F.bOriginal });
 Out.Add({ TEXT("Take the event sheet you started"), F.bSheet });
 Out.Add({ TEXT("The remote sheets off the telex, 05:04"), F.bRemote });
 if (F.bRemote) Out.Add({ TEXT("Get the originals to the truck"), F.bEscaped });
 return Out;
}

// ---------- the step ----------

bool FS47SC11::Step(double Dt)
{
 if (!M || !M->Story || !M->World) return false;
 AS47Story& S = *M->Story;
 const bool bPlay = M->Mode == US47Main::EMode::Play && !M->bPausedByMenu && !M->ModalOpen();
 if (FadeT >= 0 && bPlay) { FadeT += Dt; if (FadeT >= FadeTime) { FadeT = -1; if (M->UI) M->UI->Fade(false); } }
 if (bRestart && bPlay && M->World->Area == TEXT("saro")) { bRestart = false; StartSiege(true); }
 switch (P)
 {
 case EStep::Drive:
  if (bPlay && M->World->Area == TEXT("saro") && !M->World->bDriving)
  {
   S.SetClock(FMath::Max(S.Clock(), Arrive));
   M->UI->Toast(TEXT("SARO, five in the morning. Every window dark but yours."), 5);
   SetStep(EStep::Collect);
  }
  break;
 case EStep::Telex:
  if (!bPlay) break;
  TelexT += Dt;
  {
   static const TCHAR* Lines[] = { TEXT("The telex wakes: TELEX 05:04 HONDO VALLEY RADIO STATION."), TEXT("SOURCE H-1. RANGE NEGATIVE. FIRST LIMIT 05:29:47."),
    TEXT("TELEX 05:05 SITE 11. SOURCE S-11. RANGE NEGATIVE."), TEXT("FIRST LIMIT 05:29:47. SAME DIRECTION AS SARO. The telex stops.") };
   if (TelexLine < 4 && TelexT >= TelexLine * (TelexTime / 4))
   {
    M->UI->Toast(Lines[TelexLine], 6);
    if (M->Audio) M->Audio->Play(TEXT("printer"), .5f, S47Three::ToUE(TelexAtPos));
    ++TelexLine;
   }
  }
  break;
 case EStep::Siege: StepSiege(Dt); break;
 case EStep::Done: return true;
 default: break;
 }
 return P == EStep::Done;
}

void FS47SC11::Finish()
{
 AS47Story& S = *M->Story;
 F.bEscaped = true;
 if (M->UI) M->UI->Fade(true);
 FadeT = 0;
 S.Note(FString::Printf(TEXT("%s. Out of SARO with the originals. Behind me the light stayed on the building."), *S.ClockText(false)));
 Pale.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk); bShown = false;
 if (USpotLightComponent* L = Search.Get()) L->SetVisibility(false);
 SetStep(EStep::Done);
 UE_LOG(LogS47SC11, Display, TEXT("S47: SC11 complete; captures %d, lure %d"), F.Captures, F.bLure ? 1 : 0);
}

// ---------- the siege ----------

void FS47SC11::StartSiege(bool bRetry)
{
 SetStep(EStep::Siege);
 T = bRetry ? 2.0 : 0; SeeAcc = LostT = 0; Patrol = 0; Gait = 0; Path.Reset(); bHavePlayer = false; bLureRinging = false; LastDoors.Reset(); LitFor = 0;
 Pos = Nodes()[NLabIn].At; Enter(EBody::Away);
 if (!Search.IsValid() && M->Web && M->Web->GetRootComponent())
 {
  USpotLightComponent* L = NewObject<USpotLightComponent>(M->Web.Get());
  L->SetMobility(EComponentMobility::Movable); L->SetupAttachment(M->Web->GetRootComponent()); L->RegisterComponent();
  L->SetIntensity(120000.f); L->SetLightColor(FLinearColor(.86f, .9f, 1.f)); L->SetAttenuationRadius(6000.f);
  L->SetInnerConeAngle(LightCone * .6f); L->SetOuterConeAngle(LightCone); L->SetCastShadows(true);
  L->SetWorldLocation(S47Three::ToUE(LightFrom));
  Search = L;
 }
 if (USpotLightComponent* L = Search.Get()) L->SetVisibility(true);
 if (!bRetry) M->UI->Toast(TEXT("Light comes across the control-room windows from outside. Not a car. It moves like someone looking."), 6);
 UE_LOG(LogS47SC11, Display, TEXT("S47: SC11 siege %s"), bRetry ? TEXT("starts again at the telex") : TEXT("starts"));
}

void FS47SC11::Enter(EBody Next)
{
 if (B != Next) UE_LOG(LogS47SC11, Display, TEXT("S47: SC11 body %s -> %s at %.2f %.2f"), BodyName(B), BodyName(Next), Pos.X, Pos.Y);
 B = Next; StateT = 0; Path.Reset();
}

bool FS47SC11::EdgeOpen(int32 A, int32 Bn) const
{
 const auto Is = [A, Bn](int32 X, int32 Z) { return (A == X && Bn == Z) || (A == Z && Bn == X); };
 if (!M || !M->Web) return true;
 if (Is(NLabIn, NLabDoor)) return M->Web->IsDoorOpen(TEXT("labDoor"));
 if (Is(NLanding, NEastIn)) return M->Web->IsDoorOpen(TEXT("doorEast"));
 if (Is(NControlDoor, NCorrSouth)) return M->Web->IsDoorOpen(TEXT("doorSouth"));
 if (Is(NCorrRec, NRecIn)) return M->Web->IsDoorOpen(TEXT("recordsDoor"));
 return true;
}

int32 FS47SC11::Nearest(const FVector2D& At) const
{
 int32 Best = NCrCentre; double D = 1e9;
 const int32 R = Room(At);
 for (int32 I = 0; I < Nodes().Num(); ++I)
 {
  const double Dist = FVector2D::Distance(Nodes()[I].At, At) + (Room(Nodes()[I].At) == R ? 0 : 50);
  if (Dist < D) { D = Dist; Best = I; }
 }
 return Best;
}

void FS47SC11::RouteTo(int32 Goal)
{
 // Doors do not stop it in SC11 (they buy time): the route ignores them and Walk tries each one.
 Path.Reset();
 const int32 Start = Nearest(Pos);
 TArray<int32> Prev; Prev.Init(-2, Nodes().Num());
 TArray<int32> Q = {Start}; Prev[Start] = -1;
 for (int32 H = 0; H < Q.Num(); ++H)
  for (const auto& E : Edges())
  {
   const int32 A = Q[H], N = E.Key == A ? E.Value : E.Value == A ? E.Key : -1;
   if (N < 0 || Prev[N] != -2) continue;
   Prev[N] = A; Q.Add(N);
  }
 if (Prev[Goal] == -2) return;
 for (int32 N = Goal; N >= 0; N = Prev[N]) Path.Insert(N, 0);
 // Already past the first node towards the second (a replan mid-edge): go on, not back.
 if (Path.Num() > 1 && (FVector2D::Distance(Pos, Nodes()[Path[0]].At) < .2 || FVector2D::Distance(Pos, Nodes()[Path[1]].At) < FVector2D::Distance(Nodes()[Path[0]].At, Nodes()[Path[1]].At))) Path.RemoveAt(0);
}

bool FS47SC11::Walk(double Speed, double Dt)
{
 if (Path.IsEmpty()) { Gait = FMath::Max(0.0, Gait - Dt * 3); return true; }
 // A shut door on the way: a moment at the handle, then it opens.
 const int32 From = Nearest(Pos);
 if (FVector2D::Distance(Pos, Nodes()[From].At) < .25 && From != Path[0] && !EdgeOpen(From, Path[0]))
 {
  Gait = FMath::Max(0.0, Gait - Dt * 3);
  DoorT += Dt;
  if (DoorT < DoorTry) return false;
  DoorT = 0;
  const auto Is = [From, To = Path[0]](int32 X, int32 Z) { return (From == X && To == Z) || (From == Z && To == X); };
  const TCHAR* Door = Is(NLabIn, NLabDoor) ? TEXT("labDoor") : Is(NLanding, NEastIn) ? TEXT("doorEast") : Is(NControlDoor, NCorrSouth) ? TEXT("doorSouth") : TEXT("recordsDoor");
  M->Web->SetDoorOpen(Door, true, false); LastDoors.Add(Door, true);
  if (M->Audio) M->Audio->Play(TEXT("door_latch"), .6f, S47Three::ToUE(FVector(Pos.X, 1.0, Pos.Y)));
  UE_LOG(LogS47SC11, Display, TEXT("S47: SC11 body opens %s"), Door);
  return false;
 }
 const FVector2D To = Nodes()[Path[0]].At, D = To - Pos;
 const double Dist = D.Size(), StepLen = Speed * Dt;
 BodyYaw = HeadYaw = YawTo(D);
 Gait = FMath::Min(Speed > 1.5 ? 1.4 : 1.0, Gait + Dt * 3); Stride += StepLen * 2.1;
 DoorT = 0;
 if (Dist <= StepLen) { Pos = To; Path.RemoveAt(0); return Path.IsEmpty(); }
 Pos += D / Dist * StepLen;
 return false;
}

bool FS47SC11::SeesPlayer() const
{
 if (!M || !M->Player || !M->Web || !M->Web->GetWorld() || !bShown) return false;
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 const FVector E = Eye();
 const FVector2D D(Cam.Pos.X - E.X, Cam.Pos.Z - E.Z);
 const double Dist = D.Size();
 if (Dist > SightRange) return false;
 // Between rooms only through an open door or the north windows, as SC04 rules it.
 const FVector2D From(E.X, E.Z), To(Cam.Pos.X, Cam.Pos.Z);
 const int32 RA = Room(From), RB = Room(To);
 if (RA != RB)
 {
  bool bOk = false;
  for (const FGap& G : Gaps) if ((!G.Door || M->Web->IsDoorOpen(G.Door)) && Through(From, To, G)) bOk = true;
  if (!bOk) return false;
 }
 const double Cone = B == EBody::Lured ? LureCos : SightCos;
 if (Dist > .4 && FVector2D::DotProduct(FVector2D(FMath::Cos(HeadYaw), FMath::Sin(HeadYaw)), D / Dist) < Cone) return false;
 AS47Web* W = M->Web.Get();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC11Sight), true);
 for (const auto& C : Pale.All) if (const UStaticMeshComponent* Part = C.Get()) Q.AddIgnoredComponent(Part);
 const FVector A = S47Three::ToUE(E), Bp = S47Three::ToUE(Cam.Pos);
 for (int32 I = 0; I < 16; ++I)
 {
  FHitResult H;
  if (!W->GetWorld()->LineTraceSingleByChannel(H, A, Bp, ECC_Visibility, Q)) return true;
  const UPrimitiveComponent* C = H.GetComponent(); if (!C) return false;
  bool Solid = C->IsVisible();
  const auto* Node = W->Node(W->NodeOf(C));
  if (Node && Node->Name.StartsWith(TEXT("proxy"))) Solid = false;
  if (const auto* Mat = C->GetMaterial(FMath::Max<int32>(0, H.ElementIndex)))
  {
   float Opacity = 1; Mat->GetScalarParameterValue(TEXT("Opacity"), Opacity);
   if (Mat->GetBlendMode() == BLEND_Additive || (Mat->GetBlendMode() == BLEND_Translucent && Opacity < .5f)) Solid = false;
  }
  if (Solid) return false;
  Q.AddIgnoredComponent(C);
 }
 return false;
}

TOptional<FVector2D> FS47SC11::Heard(double Dt)
{
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 const double Speed = bHavePlayer ? FVector2D::Distance(Pl, LastPlayer) / FMath::Max(Dt, 1e-3) : 0;
 LastPlayer = Pl; bHavePlayer = true; NoiseCool -= Dt;
 if (Speed < .4 || Speed > 12 || NoiseCool > 0) return {};
 const int32 A = Room(Pos), Bp = Room(Pl);
 const auto Closed = [this](const TCHAR* Door) { return M->Web && !M->Web->IsDoorOpen(Door); };
 const auto Between = [A, Bp](int32 X, int32 Z) { return (A == X && Bp == Z) || (A == Z && Bp == X); };
 int32 Shut = 0;
 if (Between(1, 2) && Closed(TEXT("recordsDoor"))) ++Shut;
 if (Between(0, 1) && Closed(TEXT("doorSouth"))) ++Shut;
 if (Between(0, 2)) Shut += int32(Closed(TEXT("recordsDoor"))) + int32(Closed(TEXT("doorSouth")));
 if (Between(0, 3)) ++Shut; // walls and glass between the control room and outside
 if (Between(1, 3) && Closed(TEXT("exitDoor"))) ++Shut;
 const double Range = (Speed > 2.5 ? RunHear : WalkHear) / FMath::Pow(3.0, Shut);
 if (FVector2D::Distance(Pl, Pos) > Range) return {};
 NoiseCool = .5;
 return Pl;
}

double FS47SC11::Ground(const FVector2D& At) const
{
 if (Room(At) != 3 || !M || !M->Web || !M->Web->GetWorld()) return 0;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC11Ground), true);
 for (const auto& C : Pale.All) if (const UStaticMeshComponent* Part = C.Get()) Q.AddIgnoredComponent(Part);
 FHitResult H;
 if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(At.X, Y + 1.5, At.Y), S47Three::ToUE(At.X, Y - 4, At.Y), ECC_Visibility, Q)) return Y;
 return S47Three::ToThree(H.ImpactPoint).Y;
}

void FS47SC11::Capture()
{
 ++F.Captures;
 if (M->UI) M->UI->Fade(true);
 FadeT = 0;
 if (M->Audio) M->Audio->Gasp();
 if (M->Player) { M->Player->Place(TelexStand.X, TelexStand.Y, 0); M->Player->Pitch = -.06; }
 UE_LOG(LogS47SC11, Display, TEXT("S47: SC11 capture %d; the siege starts again at the telex, originals kept, clock unchanged"), F.Captures);
 StartSiege(true);
 M->RequestAutosave(false);
}

void FS47SC11::StepLight(double Dt)
{
 USpotLightComponent* L = Search.Get();
 if (!L || !M->Player) return;
 LightT += Dt;
 // A sweep across the north windows and the control room, there and back in LightPeriod seconds.
 const double K = .5 - .5 * FMath::Cos(2 * PI * LightT / LightPeriod);
 const FVector Aim(FMath::Lerp(-6.0, 6.0, K), 1.2, FMath::Lerp(1.5, -1.0, FMath::Abs(K - .5) * 2));
 const FVector Dir = (Aim - LightFrom).GetSafeNormal();
 L->SetWorldRotation(S47Three::DirToUE(Dir).Rotation());
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 const FVector ToP = Cam.Pos - LightFrom;
 bLit = false;
 if (FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Dir, ToP.GetSafeNormal()))) < LightCone)
 {
  FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC11Light), true);
  const FVector A = S47Three::ToUE(LightFrom), Bp = S47Three::ToUE(Cam.Pos);
  bLit = true;
  for (int32 I = 0; I < 12 && bLit; ++I)
  {
   if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, A, Bp, ECC_Visibility, Q)) break;
   const UPrimitiveComponent* C = H.GetComponent();
   const auto* Mat = C ? C->GetMaterial(FMath::Max<int32>(0, H.ElementIndex)) : nullptr;
   const bool bGlass = Mat && Mat->GetBlendMode() == BLEND_Translucent;
   if (C && (!C->IsVisible() || bGlass)) { Q.AddIgnoredComponent(C); continue; }
   bLit = false;
  }
 }
 LitFor = bLit ? LitFor + Dt : 0;
}

void FS47SC11::StepSiege(double Dt)
{
 const bool bHeld = !M->Player || M->Mode != US47Main::EMode::Play || M->bPausedByMenu || M->ModalOpen() || M->Story->IsCinematic() || FadeT >= 0 ||
  M->World->Area != TEXT("saro") || (M->Flow && M->Flow->bStarting);
 if (!Pale.bBuilt && !Pale.Build(M->Web.Get(), TEXT("#root"), TEXT("SC11Pale"))) return; // the light still searches; the truck still drives
 if (!bHeld)
 {
  T += Dt; Clock += Dt;
  StepLight(Dt);
  const double Before = Stride;
  Think(Dt);
  if (bShown)
  {
   Y = Ground(Pos);
   if (M->Audio && FMath::FloorToDouble(Stride / PI) != FMath::FloorToDouble(Before / PI))
   {
    const auto Pick = M->Audio->StepPick(Room(Pos) == 3 ? TEXT("concrete") : TEXT("tile"));
    FS47Sound O; O.Gain = Pick.Gain * (B == EBody::Chase ? 1.f : .75f); O.At = FVector(Pos.X, Y + .05, Pos.Y);
    M->Audio->Sfx(Pick.Name, O);
   }
  }
 }
 Pale.Apply(bShown, FVector(Pos.X, Y, Pos.Y), BodyYaw, HeadYaw, Gait, Stride, Clock,
  B == EBody::Chase ? FS47PaleBody::EPose::Chase : B == EBody::Grab ? FS47PaleBody::EPose::Grab : B == EBody::Listen || B == EBody::Lured ? FS47PaleBody::EPose::Listen : FS47PaleBody::EPose::Walk);
}

void FS47SC11::Think(double Dt)
{
 StateT += Dt;
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 if (bLureRinging) { LureT += Dt; if (LureT >= LureTime) bLureRinging = false; }
 // A door the player moves is heard at its opening; the searchlight holding on the player tells it where.
 TOptional<FVector2D> Noise;
 for (int32 G = 0; G < UE_ARRAY_COUNT(Gaps); ++G)
 {
  if (!Gaps[G].Door) continue;
  const bool bOpen = M->Web->IsDoorOpen(Gaps[G].Door);
  const bool* Was = LastDoors.Find(Gaps[G].Door);
  const FVector2D At((Gaps[G].X0 + Gaps[G].X1) * .5, (Gaps[G].Z0 + Gaps[G].Z1) * .5);
  if (Was && *Was != bOpen && bShown && FVector2D::Distance(At, Pos) <= GapReach[G]) Noise = At;
  LastDoors.Add(Gaps[G].Door, bOpen);
 }
 if (LitFor >= LitTime) { Noise = Pl; if (LitFor - Dt < LitTime) M->UI->Toast(TEXT("The light holds on you."), 2.5); }
 const bool bSees = bShown && SeesPlayer();
 SeeAcc = bSees ? SeeAcc + Dt : FMath::Max(0.0, SeeAcc - Dt);
 TOptional<FVector2D> Sound = bShown ? Heard(Dt) : TOptional<FVector2D>();
 if (Noise) Sound = Noise;
 if (bSees) { LastKnown = Pl; LostT = 0; }
 // In by the lab, across the landing, through the control room, the corridor and the records room, back out.
 static const int32 Round[] = {NWalk, NLanding, NEastIn, NCrEast, NCrNE, NCrNW, NCrWest, NCrSouthW, NControlDoor, NCorrSouth, NCorrRec, NRecIn, NRecEast, NRecW, NRecIn,
  NCorrRec, NCorrSouth, NOffice, NCorrSouth, NControlDoor, NCrSouthW, NCrCentre, NCrEast, NEastIn, NLanding, NWalk, NLabPath, NLabDoor, NLabIn};
 const auto NextPatrol = [this]()
 {
  while (Patrol < UE_ARRAY_COUNT(Round)) { RouteTo(Round[Patrol++]); if (!Path.IsEmpty()) return true; }
  return false;
 };
 const auto Investigate = [this](const FVector2D& At)
 {
  LastKnown = At; const int32 N = Nearest(At); RouteTo(N);
  for (int32 I = 0; I < UE_ARRAY_COUNT(Round); ++I) if (Round[I] == N) { Patrol = I + 1; break; }
 };
 switch (B)
 {
 case EBody::Away:
  if (T >= 6.0)
  {
   Pos = Nodes()[NLabIn].At; BodyYaw = HeadYaw = PI; Y = Ground(Pos); bShown = true;
   if (M->Audio) M->Audio->Play(TEXT("door_latch"), .5f, S47Three::ToUE(FVector(12.8, 1.0, -1.2)));
   Enter(EBody::Search); Patrol = 0; NextPatrol();
  }
  break;
 case EBody::Search:
 case EBody::Leave:
  if (B == EBody::Search && SeeAcc >= SeeTime) { Enter(EBody::Chase); break; }
  if (B == EBody::Search && bLureRinging) { Enter(EBody::Lured); RouteTo(NOffice); break; }
  if (B == EBody::Search && Sound) Investigate(*Sound);
  if (Walk(B == EBody::Leave ? WalkSpeed * 1.2 : WalkSpeed, Dt) && (B == EBody::Leave || !NextPatrol()))
  {
   if (FVector2D::Distance(Pos, Nodes()[NLabIn].At) < .3) { bShown = false; Enter(EBody::Away); T = -ComeBack + 6; } // out by the lab; back in a while
   else { Enter(EBody::Leave); RouteTo(NLabIn); }
  }
  break;
 case EBody::Listen:
  if (SeeAcc >= SeeTime) { Enter(EBody::Chase); break; }
  if (Sound) { Enter(EBody::Search); Investigate(*Sound); break; }
  if (StateT > 4) { Enter(EBody::Search); NextPatrol(); }
  break;
 case EBody::Lured:
  if (SeeAcc >= SeeTime) { Enter(EBody::Chase); break; }
  if (Walk(WalkSpeed * 1.3, Dt)) BodyYaw = HeadYaw = PI / 2; // at the office door, facing it
  if (Path.IsEmpty() && !bLureRinging && StateT > LureTime) { Enter(EBody::Search); Investigate(Nodes()[NCorrSouth].At); }
  break;
 case EBody::Chase:
 {
  if (bSees) LostT = 0; else LostT += Dt;
  if (FMath::FloorToInt(StateT * 2) != FMath::FloorToInt((StateT - Dt) * 2))
   UE_LOG(LogS47SC11, Verbose, TEXT("S47: SC11 chase at %s to %s sees %d lost %.1f path %d next %d east %d"), *Pos.ToString(), *LastKnown.ToString(), bSees, LostT, Path.Num(), Path.Num() ? Path[0] : -1, M->Web->IsDoorOpen(TEXT("doorEast")));
  if (LostT > LoseTime) { Enter(EBody::Search); Investigate(LastKnown); break; }
  const FVector2D Goal = LastKnown;
  if (Room(Goal) == Room(Pos) && M->Player->Walkable(Pos.X, Pos.Y))
  {
   const FVector2D D = Goal - Pos; const double Dist = D.Size(), StepLen = ChaseSpeed * Dt;
   BodyYaw = HeadYaw = YawTo(D); Gait = FMath::Min(1.4, Gait + Dt * 4); Stride += StepLen * 2.1;
   const FVector2D Next = Dist > StepLen ? Pos + D / Dist * StepLen : Goal;
   if (M->Player->Walkable(Next.X, Next.Y)) Pos = Next;
   Path.Reset();
  }
  else { if (Path.IsEmpty() || StateT > .5) { RouteTo(Nearest(Goal)); StateT = 0; } Walk(ChaseSpeed, Dt); }
  if (FVector2D::Distance(Pos, Pl) < Reach && bSees) { Enter(EBody::Grab); BodyYaw = HeadYaw = YawTo(Pl - Pos); }
  break;
 }
 case EBody::Grab:
  if (StateT >= GrabHold) { bShown = false; Capture(); }
  break;
 }
}

// ---------- self-test ----------

bool FS47SC11::SelfTest(TArray<FString>& Lines)
{
 bool Ok = true;
 const auto Check = [&](bool Good, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s SC11: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), What)); Ok &= Good; };
 bool Rooms = Room(Nodes()[NLabIn].At) == 3 && Room(Nodes()[NLanding].At) == 3 && Room(Nodes()[NTruck].At) == 3 && Room(Nodes()[NEastIn].At) == 0 &&
  Room(Nodes()[NCrSouthW].At) == 0 && Room(Nodes()[NCorrSouth].At) == 1 && Room(Nodes()[NRecIn].At) == 2;
 Check(Rooms, TEXT("the lab path, landing and truck are outside; the service door leads into the control room"));
 bool Short = true; for (const auto& E : Edges()) Short &= FVector2D::Distance(Nodes()[E.Key].At, Nodes()[E.Value].At) < 8;
 Check(Nodes().Num() == 23 && Edges().Num() == 25 && Short, TEXT("twenty-three nodes, twenty-five short edges"));
 FS47SC11 A(nullptr); A.P = EStep::Telex; A.F.bOriginal = A.F.bSheet = true; A.F.Captures = 1;
 FS47SC11 Bk(nullptr); Bk.Begin(S47J::Read(S47J::Write(A.Save())));
 Check(Bk.P == EStep::Telex && Bk.F.bOriginal && Bk.F.bSheet && !Bk.F.bRemote && Bk.F.Captures == 1 && Bk.TelexT == 0 && Bk.TelexLine == 0,
  TEXT("a saved telex restarts printing, with the originals kept"));
 Check(Bk.Label(TEXT("telex")).IsEmpty(), TEXT("restored printing cannot be torn off before it finishes"));
 Bk.TelexT = TelexTime;
 Check(!Bk.Label(TEXT("telex")).IsEmpty(), TEXT("the restarted telex offers the next world action after printing"));
 A.P = EStep::Collect;
 Bk.Begin(S47J::Read(S47J::Write(A.Save())));
 Check(Bk.P == EStep::Telex && Bk.TelexT == 0 && Bk.F.bOriginal && Bk.F.bSheet,
  TEXT("a legacy stranded collection save recovers without recollecting the originals"));
 A.F.bSheet = false;
 Bk.Begin(S47J::Read(S47J::Write(A.Save())));
 Check(Bk.P == EStep::Collect && !Bk.Label(TEXT("logbook")).IsEmpty() && Bk.Label(TEXT("printer")).IsEmpty(),
  TEXT("a genuinely incomplete collection keeps its remaining action"));
 FS47SC11 Fresh(nullptr); Fresh.Begin(nullptr);
 Check(Fresh.P == EStep::Drive && Fresh.Label(TEXT("printer")).IsEmpty() && Fresh.Label(TEXT("truck")).IsEmpty(), TEXT("a new SC11 starts with the drive"));
 Check(RemoteDoc().Page.Contains(TEXT("05:29:47")) && RemoteDoc().Page.Contains(TEXT("S-11")), TEXT("the remote sheets name both sources and the first limit"));
 return Ok;
}
