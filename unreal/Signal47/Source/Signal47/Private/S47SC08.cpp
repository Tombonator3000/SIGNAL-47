#include "S47SC08.h"
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
#include "S47SaroNight.h"
#include "S47DishesFollow.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47SC08, Log, All);

namespace
{
 constexpr double ReelAt = 4 * 3600.;            // Earl has gone home
 constexpr double WalkSpeed = 1.2, ChaseSpeed = 2.1, SeeTime = .4, LoseTime = 3.0, Reach = .9, GrabHold = .7;
 constexpr double SightRange = 26, SightCos = .342, WalkHear = 9, RunHear = 14, FadeTime = 1.2;
 constexpr double AtStake = 2.2, BackSteps = 4;
 // FS47DishesFollow starts a breath after :16 of a 47-second warning and goes on until its end.
 constexpr double WarnStart = 17.6, WarnHold = 40;
 enum : int32 { NGateIn, NSpine50, NR1, NW12, NHwy1, NWest, NE12, NEast, NHwy106, NHwy2, NR2, NSpine106 };
 double YawTo(const FVector2D& D) { return FMath::Atan2(D.Y, D.X); }
}

const TArray<FS47SC08::FNode>& FS47SC08::Nodes()
{
 static const TArray<FNode> N = {
  {TEXT("gateIn"), FVector2D(-1, -24)}, {TEXT("spine50"), FVector2D(-1, -50)}, {TEXT("r1"), FVector2D(-1, -74)},
  {TEXT("row1West12"), FVector2D(-12, -74)}, {TEXT("highwayRow1"), FVector2D(-24, -74)}, {TEXT("row1West40"), FVector2D(-40, -74)},
  {TEXT("row1East12"), FVector2D(12, -74)}, {TEXT("row1East30"), FVector2D(30, -74)},
  {TEXT("highway106"), FVector2D(-24, -106)}, {TEXT("highwayRow2"), FVector2D(-24, -138)}, {TEXT("r2"), FVector2D(-1, -138)}, {TEXT("spine106"), FVector2D(-1, -106)}};
 return N;
}

const TArray<TPair<int32, int32>>& FS47SC08::Edges()
{
 static const TArray<TPair<int32, int32>> E = {{NGateIn, NSpine50}, {NSpine50, NR1}, {NR1, NW12}, {NW12, NHwy1}, {NHwy1, NWest}, {NR1, NE12}, {NE12, NEast},
  {NHwy1, NHwy106}, {NHwy106, NHwy2}, {NHwy2, NR2}, {NR2, NSpine106}, {NSpine106, NR1}};
 return E;
}

const TCHAR* FS47SC08::BodyName(EBody In)
{
 switch (In)
 {
 case EBody::Away: return TEXT("away"); case EBody::Still: return TEXT("still"); case EBody::Search: return TEXT("search");
 case EBody::Chase: return TEXT("chase"); case EBody::Grab: return TEXT("grab"); case EBody::Fence: return TEXT("fence"); default: return TEXT("gone");
 }
}

// The service sheet's stakes: twelve metres either side of the spine on row one, the lamp at the junction.
FVector2D FS47SC08::PointA() { return FVector2D(-12, -74); }
FVector2D FS47SC08::PointB() { return FVector2D(12, -74); }
FVector2D FS47SC08::Ref() { return FVector2D(-1.9, -72.9); }
// Out through the array gate: gNorth begins at z -19.55.
bool FS47SC08::OutOfField(const FVector2D& At) { return At.Y > -19.4; }

// ---------- saving ----------

void FS47SC08::Begin(const S47J::FObj& Saved)
{
 Stop();
 P = EStep::Reel; F = FFacts();
 if (!Saved) return;
 P = EStep(FMath::Clamp(S47J::Int(Saved, TEXT("step")), 0, int32(EStep::Done)));
 F.bReel = S47J::Bool(Saved, TEXT("reel")); F.bSheet = S47J::Bool(Saved, TEXT("sheet"));
 F.bFirst = S47J::Bool(Saved, TEXT("first")); F.bSecond = S47J::Bool(Saved, TEXT("second")); F.bBack = S47J::Bool(Saved, TEXT("back"));
 F.bEscaped = S47J::Bool(Saved, TEXT("escaped")); F.First = S47J::Str(Saved, TEXT("firstStake")); F.Captures = FMath::Max(0, S47J::Int(Saved, TEXT("captures")));
 // The test and the meeting are never saved half run: a load starts the field test again.
 if (P == EStep::Field || P == EStep::Hunt) { P = EStep::Field; F.bFirst = F.bSecond = F.bBack = false; F.First.Reset(); }
}

S47J::FObj FS47SC08::Save() const
{
 auto O = S47J::New();
 S47J::Set(O, TEXT("step"), int32(P));
 S47J::Set(O, TEXT("reel"), F.bReel); S47J::Set(O, TEXT("sheet"), F.bSheet); S47J::Set(O, TEXT("first"), F.bFirst);
 S47J::Set(O, TEXT("second"), F.bSecond); S47J::Set(O, TEXT("back"), F.bBack); S47J::Set(O, TEXT("escaped"), F.bEscaped);
 S47J::Set(O, TEXT("firstStake"), F.First); S47J::Set(O, TEXT("captures"), F.Captures);
 return O;
}

void FS47SC08::Stop()
{
 B = EBody::Away; bShown = false; Path.Reset(); FadeT = -1;
 Pale.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk);
 ShowStakes(false);
 if (M && M->SaroNight && M->SaroNight->StoryFollowAt()) M->SaroNight->StoryFollow({});
}

void FS47SC08::SetStep(EStep Next)
{
 if (P == Next) return;
 UE_LOG(LogS47SC08, Display, TEXT("S47: SC08 step %d -> %d"), int32(P), int32(Next));
 P = Next;
 if (M) M->RequestAutosave(false);
}

void FS47SC08::TestComplete() { P = EStep::Done; Stop(); }

// ---------- the reel and the sheet ----------

FS47Doc FS47SC08::ReelDoc()
{
 FS47Doc D; D.Id = TEXT("sc08_reel"); D.Title = TEXT("Earl's note, with the 1981 reel"); D.Kind = TEXT("hand"); D.Stamp = TEXT("1420 AM / LOBBY");
 D.Page = TEXT("Reyes -\n\nThe '81 reel you asked about. A copy, on isolated tape, not the monitor.\nThe station was off the air that night. The meter says so. The reel says otherwise.\n\n1420 AM is a radio station. 1420.405 megahertz is yours. Don't mix them up.\n\nGone home at four.\n- Earl\n\n(Played it twice. The boy: \"It's low in the south. It has a little tail.\" I asked the tape what it was. It said it again, the same breath.)");
 D.Transcript = TEXT("Earl's note left with a copy of the September 1981 reel. On isolated tape the boy says the same words whatever is asked: low in the south, a little tail. Earl's warning: 1420 AM is not 1420.405 megahertz.");
 return D;
}

FS47Doc FS47SC08::SheetDoc()
{
 FS47Doc D; D.Id = TEXT("sc08_sheet"); D.Title = TEXT("Array service sheet, alignment check"); D.Kind = TEXT("typed"); D.Stamp = TEXT("ARRAY / SERVICE");
 D.Page = TEXT("SARO ARRAY / SERVICE SHEET / ALIGNMENT CHECK\n\nA / ROW 1 WEST ........ stake, 12 m west of R1\nB / ROW 1 EAST ........ stake, 12 m east of R1\nREF / R1 .............. red lamp at the spine junction, fixed\n\nThe dishes are commanded to REF and hold.\nStand at A or B, cross to the other, return part way.\nNote any drive that moves. Every movement must show in the command log.");
 D.Transcript = TEXT("The array service sheet: stake A twelve metres west of the spine on row one, stake B twelve metres east, the fixed red reference lamp at R1. The dishes hold on REF; any drive that moves must show in the command log.");
 return D;
}

FString FS47SC08::Label(const FString& Id) const
{
 if (Id == TEXT("amDoor") && P == EStep::Reel) return TEXT("Earl's reel copy, left in the lobby");
 if (Id == TEXT("serviceRecord") && P == EStep::Sheet) return TEXT("Take the array service sheet");
 return FString();
}

bool FS47SC08::Use(const FString& Id)
{
 if (Label(Id).IsEmpty() || !M || !M->Story || !M->UI) return false;
 AS47Story& S = *M->Story;
 if (Id == TEXT("amDoor"))
 {
  F.bReel = true;
  S.SetClock(FMath::Max(S.Clock(), ReelAt));
  const FS47Doc D = ReelDoc(); S.AddDoc(D);
  if (M->Audio) M->Audio->Play(TEXT("click"), .5f);
  M->UI->Document(D);
  S.Note(FString::Printf(TEXT("%s. 1420 AM, Earl gone home. His copy of the 1981 reel in the lobby, on isolated tape: the boy says the same words whatever I ask. 1420 AM is not 1420.405 MHz; he says so."), *S.ClockText(false)));
  SetStep(EStep::Sheet);
 }
 else if (Id == TEXT("serviceRecord"))
 {
  F.bSheet = true;
  const FS47Doc D = SheetDoc(); S.AddDoc(D);
  if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
  M->UI->Document(D);
  S.Note(FString::Printf(TEXT("%s. The array service sheet: A and B on row one, the reference lamp at R1. Going out to see what the dishes hold on."), *S.ClockText(false)));
  SetStep(EStep::Field);
 }
 M->RequestAutosave(false);
 return true;
}

FString FS47SC08::Objective() const
{
 switch (P)
 {
 case EStep::Reel: return TEXT("SC08 // 1420 AM: EARL WENT HOME AT FOUR. HIS COPY OF THE 1981 REEL IS IN THE LOBBY");
 case EStep::Sheet: return TEXT("SARO // TAKE THE ARRAY SERVICE SHEET IN THE CONTROL ROOM");
 case EStep::Field:
  if (!F.bFirst) return TEXT("ARRAY // ROW ONE: STAND AT STAKE A OR B, THE RED LAMP IS THE REFERENCE");
  if (!F.bSecond) return FString::Printf(TEXT("ARRAY // NOW CROSS TO STAKE %s"), F.First == TEXT("A") ? TEXT("B") : TEXT("A"));
  return TEXT("ARRAY // A FEW STEPS BACK. WATCH WHAT THE DISHES HOLD ON");
 case EStep::Hunt: return TEXT("ARRAY // SOMETHING IS IN THE FIELD. OUT THROUGH THE ARRAY GATE");
 default: return FString();
 }
}

TArray<FS47Task> FS47SC08::Tasks() const
{
 TArray<FS47Task> Out;
 Out.Add({ TEXT("Earl's 1981 reel at 1420 AM"), F.bReel });
 if (!F.bReel) return Out;
 Out.Add({ TEXT("The array service sheet"), F.bSheet });
 if (!F.bSheet) return Out;
 Out.Add({ TEXT("Stand at stake A or B"), F.bFirst });
 Out.Add({ TEXT("Cross to the other stake"), F.bSecond });
 Out.Add({ TEXT("A few steps back"), F.bBack });
 if (F.bBack) Out.Add({ TEXT("Out through the array gate"), F.bEscaped });
 return Out;
}

int32 FS47SC08::Following() const
{
 int32 N = 0;
 if (M && M->SaroNight && M->SaroNight->Follow) for (const FS47DishesFollow::FState& S : M->SaroNight->Follow->State()) N += S.bOn ? 1 : 0;
 return N;
}

// ---------- the stakes ----------

void FS47SC08::BuildStakes()
{
 if (bBuilt || !M || !M->Web || !M->Web->IsLoaded()) return;
 AS47Web* W = M->Web.Get();
 auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 USceneComponent* Root = W->GetRootComponent();
 if (!Cube || !Base || !Root) return;
 bBuilt = true;
 const auto Mat = [W, Base](uint32 Hex, float Glow)
 {
  auto* D = UMaterialInstanceDynamic::Create(Base, W);
  D->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(Hex)); D->SetScalarParameterValue(TEXT("Roughness"), .6f);
  D->SetScalarParameterValue(TEXT("NativeLighting"), 1); D->SetScalarParameterValue(TEXT("EmissivePower"), Glow);
  return D;
 };
 struct FStake { FVector2D At; uint32 Color; float Glow; double H; const TCHAR* Text; };
 const FStake List[] = {{PointA(), 0xe8e2d0, .5f, 1.1, TEXT("A")}, {PointB(), 0xe8e2d0, .5f, 1.1, TEXT("B")}, {Ref(), 0xff2a18, 6.f, 1.5, TEXT("REF")}};
 for (const FStake& S : List)
 {
  const double G = Ground(S.At);
  auto* C = NewObject<UStaticMeshComponent>(W);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(Root); C->SetStaticMesh(Cube); C->SetMaterial(0, Mat(S.Color, S.Glow));
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetCastShadow(false); C->RegisterComponent();
  C->SetWorldLocation(S47Three::ToUE(S.At.X, G + S.H * .5, S.At.Y)); C->SetWorldScale3D(S47Three::DirToUE(FVector(.06, S.H, .06)));
  Stakes.Add(C);
  auto* Sign = NewObject<UTextRenderComponent>(W);
  Sign->SetMobility(EComponentMobility::Movable); Sign->SetupAttachment(Root); Sign->SetText(FText::FromString(S.Text));
  Sign->SetWorldSize(28.f); Sign->SetHorizontalAlignment(EHTA_Center); Sign->SetTextRenderColor(FColor(235, 226, 205));
  Sign->SetCollisionEnabled(ECollisionEnabled::NoCollision); Sign->RegisterComponent();
  Sign->SetWorldLocation(S47Three::ToUE(S.At.X, G + S.H + .25, S.At.Y)); Sign->SetWorldRotation(FRotator(0, 90, 0));
  Signs.Add(Sign);
 }
 ShowStakes(false);
}

void FS47SC08::ShowStakes(bool bOn)
{
 for (auto& S : Stakes) if (S.IsValid()) S->SetVisibility(bOn);
 for (auto& S : Signs) if (S.IsValid()) S->SetVisibility(bOn);
}

double FS47SC08::Ground(const FVector2D& At) const
{
 if (!M || !M->Web || !M->Web->GetWorld()) return 0;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC08Ground), true);
 for (const auto& C : Pale.All) if (const UStaticMeshComponent* Part = C.Get()) Q.AddIgnoredComponent(Part);
 FHitResult H;
 if (!M->Web->GetWorld()->LineTraceSingleByChannel(H, S47Three::ToUE(At.X, 8, At.Y), S47Three::ToUE(At.X, -8, At.Y), ECC_Visibility, Q)) return 0;
 return S47Three::ToThree(H.ImpactPoint).Y;
}

// ---------- the step ----------

void FS47SC08::Follow(bool bOn)
{
 if (!M || !M->SaroNight) return;
 if (bOn) { if (!M->SaroNight->StoryFollowAt()) M->SaroNight->StoryFollow(WarnStart); }
 else if (M->SaroNight->StoryFollowAt()) M->SaroNight->StoryFollow({});
}

bool FS47SC08::Step(double Dt)
{
 if (!M || !M->Story || !M->World) return false;
 const bool bPlay = M->Mode == US47Main::EMode::Play && !M->bPausedByMenu;
 if (FadeT >= 0 && bPlay) { FadeT += Dt; if (FadeT >= FadeTime) { FadeT = -1; if (M->UI) M->UI->Fade(false); } }
 BuildStakes();
 ShowStakes(P == EStep::Field || P == EStep::Hunt);
 // The follow runs through the test and the meeting, held short of its end until the player is out.
 if (P == EStep::Hunt || (P == EStep::Field && F.bFirst))
  if (const TOptional<double> W = M->SaroNight ? M->SaroNight->StoryFollowAt() : TOptional<double>(); W && *W > WarnHold) M->SaroNight->StoryFollow(WarnHold - 6);
 if (bRestart && bPlay && M->World->Area == TEXT("saro")) { bRestart = false; StartHunt(true); }
 if (P == EStep::Field && bPlay) StepField();
 else if (P == EStep::Hunt) StepHunt(Dt);
 return P == EStep::Done;
}

void FS47SC08::StepField()
{
 if (!M->Player || M->World->Area != TEXT("saro") || M->World->bDriving) return;
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 const bool bA = FVector2D::Distance(Pl, PointA()) < AtStake, bB = FVector2D::Distance(Pl, PointB()) < AtStake;
 if (!F.bFirst && (bA || bB))
 {
  F.bFirst = true; F.First = bA ? TEXT("A") : TEXT("B");
  Follow(true);
  M->UI->Toast(FString::Printf(TEXT("Stake %s. The red lamp holds at R1. Somewhere down the row a brake lets go."), *F.First), 4);
  UE_LOG(LogS47SC08, Display, TEXT("S47: SC08 at stake %s; the dishes' follow starts"), *F.First);
 }
 else if (F.bFirst && !F.bSecond && (F.First == TEXT("A") ? bB : bA))
 {
  F.bSecond = true; Turn = F.First == TEXT("A") ? PointB() : PointA(); // back steps count from the stake itself
  M->UI->Toast(TEXT("The other stake. Every dish you passed has come round after you. None of them is on the lamp."), 4);
  UE_LOG(LogS47SC08, Display, TEXT("S47: SC08 at the second stake; %d dishes on the player"), Following());
 }
 else if (F.bSecond && !F.bBack && FVector2D::Distance(Pl, Turn) > BackSteps)
 {
  F.bBack = true;
  M->Story->Note(FString::Printf(TEXT("%s. Row one. I went from %s to %s and back. The dishes followed me, not the reference lamp. The command log: no command."),
   *M->Story->ClockText(false), *F.First, F.First == TEXT("A") ? TEXT("B") : TEXT("A")));
  UE_LOG(LogS47SC08, Display, TEXT("S47: SC08 back steps; %d dishes on the player; the meeting starts"), Following());
  HuntFrom = Pl;
  StartHunt(false);
 }
}

void FS47SC08::Finish()
{
 AS47Story& S = *M->Story;
 Follow(false);
 M->UI->Toast(TEXT("Behind the fence the dishes hold where you were, one breath, then go home to 026, one drive after another. The command log has nothing."), 7);
 S.Note(FString::Printf(TEXT("%s. Out through the array gate. The dishes held the place I left, then went back to 026, the state they have had since 02:15. Nothing in the command log."), *S.ClockText(false)));
 ShowStakes(false);
 SetStep(EStep::Done);
 UE_LOG(LogS47SC08, Display, TEXT("S47: SC08 complete; captures %d"), F.Captures);
}

// ---------- the meeting ----------

void FS47SC08::StartHunt(bool bRetry)
{
 SetStep(EStep::Hunt);
 T = bRetry ? 1.0 : 0; SeeAcc = LostT = 0; Patrol = 0; Gait = 0; Path.Reset(); bHavePlayer = false;
 Enter(EBody::Away);
 UE_LOG(LogS47SC08, Display, TEXT("S47: SC08 meeting %s"), bRetry ? TEXT("starts again") : TEXT("starts"));
}

void FS47SC08::Enter(EBody Next)
{
 if (B != Next) UE_LOG(LogS47SC08, Display, TEXT("S47: SC08 body %s -> %s at %.2f %.2f"), BodyName(B), BodyName(Next), Pos.X, Pos.Y);
 B = Next; StateT = 0; Path.Reset();
}

int32 FS47SC08::Nearest(const FVector2D& At) const
{
 int32 Best = NR1; double D = 1e9;
 for (int32 I = 0; I < Nodes().Num(); ++I) { const double Dist = FVector2D::Distance(Nodes()[I].At, At); if (Dist < D) { D = Dist; Best = I; } }
 return Best;
}

void FS47SC08::RouteTo(int32 Goal)
{
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
 if (Path.Num() > 1 && FVector2D::Distance(Pos, Nodes()[Path[0]].At) < .2) Path.RemoveAt(0);
}

bool FS47SC08::Walk(double Speed, double Dt)
{
 if (Path.IsEmpty()) { Gait = FMath::Max(0.0, Gait - Dt * 3); return true; }
 const FVector2D To = Nodes()[Path[0]].At, D = To - Pos;
 const double Dist = D.Size(), StepLen = Speed * Dt;
 BodyYaw = HeadYaw = YawTo(D);
 Gait = FMath::Min(Speed > 1.5 ? 1.4 : 1.0, Gait + Dt * 3); Stride += StepLen * 2.1;
 if (Dist <= StepLen) { Pos = To; Path.RemoveAt(0); return Path.IsEmpty(); }
 Pos += D / Dist * StepLen;
 return false;
}

bool FS47SC08::SeesPlayer() const
{
 if (!M || !M->Player || !M->Web || !M->Web->GetWorld() || !bShown) return false;
 const FS47Cam Cam = FS47Cam::Of(M->Player);
 const FVector E = Eye();
 const FVector2D D(Cam.Pos.X - E.X, Cam.Pos.Z - E.Z);
 const double Dist = D.Size();
 if (Dist > SightRange || OutOfField(FVector2D(Cam.Pos.X, Cam.Pos.Z))) return false;
 if (Dist > .4 && FVector2D::DotProduct(FVector2D(FMath::Cos(HeadYaw), FMath::Sin(HeadYaw)), D / Dist) < SightCos) return false;
 // The footings, cabinets and dishes stop sight (FS47HorrorThreat::ClearLine's rule).
 AS47Web* W = M->Web.Get();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(S47SC08Sight), true);
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

TOptional<FVector2D> FS47SC08::Heard(double Dt)
{
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 const double Speed = bHavePlayer ? FVector2D::Distance(Pl, LastPlayer) / FMath::Max(Dt, 1e-3) : 0;
 LastPlayer = Pl; bHavePlayer = true; NoiseCool -= Dt;
 if (Speed < .4 || Speed > 12 || NoiseCool > 0 || OutOfField(Pl)) return {};
 if (FVector2D::Distance(Pl, Pos) > (Speed > 2.5 ? RunHear : WalkHear)) return {};
 NoiseCool = .5;
 return Pl;
}

void FS47SC08::Capture()
{
 ++F.Captures;
 if (M->UI) M->UI->Fade(true);
 FadeT = 0;
 if (M->Audio) M->Audio->Gasp();
 if (M->Player) { M->Player->Place(HuntFrom.X, HuntFrom.Y, 0); M->Player->Pitch = -.06; }
 UE_LOG(LogS47SC08, Display, TEXT("S47: SC08 capture %d; the meeting starts again where it began, clock unchanged"), F.Captures);
 StartHunt(true);
 M->RequestAutosave(false);
}

void FS47SC08::StepHunt(double Dt)
{
 const bool bHeld = !M->Player || M->Mode != US47Main::EMode::Play || M->bPausedByMenu || M->ModalOpen() || M->Story->IsCinematic() || FadeT >= 0 ||
  M->World->Area != TEXT("saro") || (M->Flow && M->Flow->bStarting);
 if (!Pale.bBuilt && !Pale.Build(M->Web.Get(), TEXT("#root"), TEXT("SC08Pale")))
 {
  // Without the body the meeting cannot be played honestly: the dishes go home and the step goes on.
  F.bEscaped = true; Finish(); return;
 }
 if (!bHeld)
 {
  T += Dt; Clock += Dt;
  const double Before = Stride;
  Think(Dt);
  if (bShown)
  {
   Y = Ground(Pos);
   if (M->Audio && FMath::FloorToDouble(Stride / PI) != FMath::FloorToDouble(Before / PI))
   {
    const auto Pick = M->Audio->StepPick(TEXT("dirt"));
    FS47Sound O; O.Gain = Pick.Gain * (B == EBody::Chase ? 1.f : .75f); O.At = FVector(Pos.X, Y + .05, Pos.Y);
    M->Audio->Sfx(Pick.Name, O);
   }
  }
 }
 Pale.Apply(bShown, FVector(Pos.X, Y, Pos.Y), BodyYaw, HeadYaw, Gait, Stride, Clock,
  B == EBody::Chase ? FS47PaleBody::EPose::Chase : B == EBody::Grab ? FS47PaleBody::EPose::Grab : B == EBody::Still || B == EBody::Fence ? FS47PaleBody::EPose::Listen : FS47PaleBody::EPose::Walk);
}

void FS47SC08::Think(double Dt)
{
 StateT += Dt;
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 const bool bOut = OutOfField(Pl);
 const bool bSees = bShown && SeesPlayer();
 SeeAcc = bSees ? SeeAcc + Dt : FMath::Max(0.0, SeeAcc - Dt);
 const TOptional<FVector2D> Sound = bShown ? Heard(Dt) : TOptional<FVector2D>();
 if (bSees) { LastKnown = Pl; LostT = 0; }
 // Its search: the row the player is on, the spine, then round by row two and the highway track.
 static const int32 Round[] = {NW12, NR1, NE12, NEast, NE12, NR1, NSpine50, NR1, NSpine106, NR2, NHwy2, NHwy106, NHwy1, NWest, NHwy1, NW12, NR1, NSpine50, NGateIn};
 const auto NextPatrol = [this]()
 {
  while (Patrol < UE_ARRAY_COUNT(Round)) { RouteTo(Round[Patrol++]); if (!Path.IsEmpty()) return true; }
  Patrol = 0; return false;
 };
 const auto Investigate = [this](const FVector2D& At)
 {
  LastKnown = At; const int32 N = Nearest(At); RouteTo(N);
  for (int32 I = 0; I < UE_ARRAY_COUNT(Round); ++I) if (Round[I] == N) { Patrol = I + 1; break; }
 };
 // Out through the gate: it comes to the fence, stands, and goes in among the footings.
 if (bOut && bShown && B != EBody::Fence && B != EBody::Grab) { Enter(EBody::Fence); RouteTo(NGateIn); }
 switch (B)
 {
 case EBody::Away:
  if (bOut) { F.bEscaped = true; Finish(); break; } // out before it came
  if (T >= 4.0)
  {
   // In at the end of row one away from the player, between the footings.
   const bool bEastSide = Pl.X > -1;
   Pos = Nodes()[bEastSide ? NHwy1 : NEast].At; BodyYaw = HeadYaw = bEastSide ? 0 : PI;
   Y = Ground(Pos); bShown = true; Enter(EBody::Still);
   if (M->Audio) M->Audio->Gasp(M->Audio->Spatial(FVector(Pos.X, Y + 1.8, Pos.Y), 3.f));
  }
  break;
 case EBody::Still:
  HeadYaw = StateT > 1.0 ? YawTo(Pl - Pos) : BodyYaw;
  if (SeeAcc >= SeeTime && StateT > 1.0) { Enter(EBody::Chase); break; }
  if (StateT > 2.2) { Enter(EBody::Search); Patrol = 0; if (Sound) Investigate(*Sound); else NextPatrol(); }
  break;
 case EBody::Search:
  if (SeeAcc >= SeeTime) { Enter(EBody::Chase); break; }
  if (Sound) Investigate(*Sound);
  if (Walk(WalkSpeed, Dt)) NextPatrol();
  break;
 case EBody::Chase:
 {
  if (bSees) LostT = 0; else LostT += Dt;
  if (LostT > LoseTime) { Enter(EBody::Search); Investigate(LastKnown); break; }
  const FVector2D D = LastKnown - Pos; const double Dist = D.Size(), StepLen = ChaseSpeed * Dt;
  const FVector2D Next = Dist > StepLen ? Pos + D / Dist * StepLen : LastKnown;
  if (M->Player->Walkable(Next.X, Next.Y))
  {
   BodyYaw = HeadYaw = YawTo(D); Gait = FMath::Min(1.4, Gait + Dt * 4); Stride += StepLen * 2.1; Pos = Next; Path.Reset();
  }
  else { if (Path.IsEmpty() || StateT > .6) { RouteTo(Nearest(LastKnown)); StateT = 0; } Walk(ChaseSpeed, Dt); }
  if (FVector2D::Distance(Pos, Pl) < Reach && bSees) { Enter(EBody::Grab); BodyYaw = HeadYaw = YawTo(Pl - Pos); }
  break;
 }
 case EBody::Grab:
  if (StateT >= GrabHold) { bShown = false; Capture(); }
  break;
 case EBody::Fence:
  if (Walk(ChaseSpeed * .8, Dt)) { HeadYaw = YawTo(Pl - Pos); if (StateT > 4) { Enter(EBody::Gone); bShown = false; } }
  else if (StateT > 14) { Enter(EBody::Gone); bShown = false; }
  break;
 case EBody::Gone:
  F.bEscaped = true; Finish();
  break;
 }
}

// ---------- self-test ----------

bool FS47SC08::SelfTest(TArray<FString>& Lines)
{
 bool Ok = true;
 const auto Check = [&](bool Good, const TCHAR* What) { Lines.Add(FString::Printf(TEXT("%s SC08: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), What)); Ok &= Good; };
 bool InField = true; for (const FNode& N : Nodes()) InField &= !OutOfField(N.At) && N.At.Y < -20;
 Check(Nodes().Num() == 12 && Edges().Num() == 12 && InField, TEXT("twelve nodes inside the field, twelve edges; the loop by row two"));
 Check(FVector2D::Distance(PointA(), Nodes()[NW12].At) < .01 && FVector2D::Distance(PointB(), Nodes()[NE12].At) < .01 && FVector2D::Distance(PointA(), PointB()) == 24, TEXT("the stakes are on row one, 24 m apart, either side of R1"));
 Check(OutOfField(FVector2D(-1, -17)) && !OutOfField(FVector2D(-1, -24)), TEXT("out of the field is through the array gate"));
 FS47SC08 A(nullptr); A.P = EStep::Hunt; A.F.bReel = A.F.bSheet = A.F.bFirst = A.F.bSecond = A.F.bBack = true; A.F.First = TEXT("B"); A.F.Captures = 1;
 FS47SC08 Bk(nullptr); Bk.Begin(S47J::Read(S47J::Write(A.Save())));
 Check(Bk.P == EStep::Field && Bk.F.bReel && Bk.F.bSheet && !Bk.F.bFirst && !Bk.F.bBack && Bk.F.First.IsEmpty() && Bk.F.Captures == 1, TEXT("a saved meeting reloads as the field test from the start, reel and sheet kept"));
 FS47SC08 Fresh(nullptr); Fresh.Begin(nullptr);
 Check(Fresh.P == EStep::Reel && !Fresh.Label(TEXT("amDoor")).IsEmpty() && Fresh.Label(TEXT("serviceRecord")).IsEmpty(), TEXT("a new SC08 starts at Earl's lobby"));
 Check(ReelDoc().Page.Contains(TEXT("1420.405")) && SheetDoc().Page.Contains(TEXT("REF / R1")), TEXT("Earl's note and the service sheet say what they must"));
 return Ok;
}
