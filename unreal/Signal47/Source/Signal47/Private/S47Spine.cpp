#include "S47Spine.h"
#include "S47SC04Contract.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Horror.h"
#include "S47HorrorSC03.h"
#include "S47Chapter.h"
#include "S47Docs.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Web.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Chapter2.h"
#include "S47Chapter4.h"
#include "Misc/CommandLine.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

// The approved timeline, 29 October 2026. Windows are on the night clock; jumps fill the drives.
const TArray<FS47Spine::FScenario>& FS47Spine::Scenarios()
{
 static const TArray<FScenario> List = {
  { TEXT("SC01"), TEXT("The evening round"), TEXT("18:00-23:40"), TEXT("SARO") },
  { TEXT("SC02"), TEXT("Back to the truck"), TEXT("02:00"), TEXT("Caliche Junction") },
  { TEXT("SC03"), TEXT("The help that came"), TEXT("02:15-02:45"), TEXT("Caliche fuel stop") },
  { TEXT("SC04"), TEXT("Secure the originals"), TEXT("02:55-03:15"), TEXT("SARO and the annex") },
  { TEXT("SC05"), TEXT("The call that lies ahead"), TEXT("03:15-03:25"), TEXT("SARO control room") },
  { TEXT("SC06"), TEXT("The second exposure"), TEXT("03:25-03:40"), TEXT("SARO and B-12") },
  { TEXT("SC07"), TEXT("What Dale did not file"), TEXT("03:40-03:55"), TEXT("Records room") },
  { TEXT("SC08"), TEXT("They point at you"), TEXT("04:00-04:20"), TEXT("1420 AM and the array") },
  { TEXT("SC09"), TEXT("The interrupted survey"), TEXT("04:25-04:45"), TEXT("STATION 01") },
  { TEXT("SC10"), TEXT("Room 6"), TEXT("04:50-05:00"), TEXT("Sierra Motor Court") },
  { TEXT("SC11"), TEXT("SARO is searched"), TEXT("05:00-05:10"), TEXT("SARO") },
  { TEXT("SC12"), TEXT("What you manage to send"), TEXT("05:12-05:20"), TEXT("Mesa Diner") },
  { TEXT("SC13"), TEXT("Last road to Roswell"), TEXT("05:20-05:29:47"), TEXT("The old road north") },
 };
 return List;
}

double FS47Spine::GateDone(EGate G)
{
 return G == SC04 ? 3 * 3600. + 15 * 60 : G == SC08 ? 4 * 3600. + 20 * 60 : 5 * 3600. + 10 * 60;
}

// three.js, SARO area: a free-standing post on the open floor east of the consoles, and one inside
// the array gate where the 21:40 round stood.
FVector FS47Spine::GatePoint(EGate G) { return G == SC08 ? FVector(-.2, 1.0, -18.4) : FVector(4.2, 1.0, .3); }
FVector FS47Spine::StandPoint(EGate G) { return G == SC08 ? FVector(-1.0, 0, -17.6) : FVector(3.0, 0, .3); }
const TCHAR* FS47Spine::GateItem(EGate G) { return G == SC04 ? TEXT("spine:sc04") : G == SC08 ? TEXT("spine:sc08") : TEXT("spine:sc11"); }

FS47Doc FS47Spine::Card(EGate G)
{
 FS47Doc D; D.Kind = TEXT("typed");
 if (G == SC04)
 {
  D.Id = TEXT("spine_sc04"); D.Title = TEXT("Placeholder: SC04"); D.Stamp = TEXT("SC04");
  D.Page = TEXT("PLACEHOLDER / SC04 SECURE THE ORIGINALS / 02:55-03:15\n\nNot built yet. What will happen here:\n\n"
   "The deputy follows Reyes to the gate and is called away to Ortiz's wagon. Reyes goes in alone to collect the receiver log and strip chart from 02:15, the shift journal and the T01 journal for the missing-person report, and locks the automatic schedule.\n\n"
   "In the annex a Dale fragment comes from the wrong door. The pale body comes in and searches. Close a known door and take the long way, or start an ordinary sound from a safe place and take the short way.\n\n"
   "Afterwards Reyes locks the control room. The receiver stays on, passive.");
 }
 else if (G == SC08)
 {
  D.Id = TEXT("spine_sc08"); D.Title = TEXT("Placeholder: SC08"); D.Stamp = TEXT("SC08");
  D.Page = TEXT("PLACEHOLDER / SC08 THEY POINT AT YOU / 04:00-04:20\n\nNot built yet. What will happen here:\n\n"
   "Earl at 1420 AM plays his marked reel from 1981 on isolated tape. The old voice says the same thing whatever is asked.\n\n"
   "On the array, with the service sheet A / ROW 1 WEST, B / ROW 1 EAST, REF / R1: the reference lamp stays put. Reyes moves from A to B and back. The antennas follow Reyes, not the lamp. The command log shows no command.\n\n"
   "Then the pale body comes in between the footings. Long way behind opaque footings, or a short crossing after reading its search. After the escape the antennas go to 026.");
 }
 else
 {
  D.Id = TEXT("spine_sc11"); D.Title = TEXT("Placeholder: SC11"); D.Stamp = TEXT("SC11");
  D.Page = TEXT("PLACEHOLDER / SC11 SARO IS SEARCHED / 05:00-05:10\n\nNot built yet. What will happen here:\n\n"
   "The receiver original, the started event sheet and the remote sheets from Hondo and Site 11, ready at 05:04. Lights search the building from outside while the pale body searches inside. Light through openings, doors for time, the long way or the short. A local lamp can be switched off; the light outside goes on.\n\n"
   "Reyes gets out with the records. The escape fact comes from the actual escape.");
 }
 D.Transcript = TEXT("A placeholder card for a scenario that is not built yet. It says what will happen here.");
 return D;
}

void FS47Spine::Begin(const S47J::FObj& Saved, bool bNewCase)
{
 Stop();
 bEnabled = bTestSC03 = false; for (int32& S : State) S = 0;
 Act04.Begin(nullptr);
 if (Saved)
 {
  if (S47J::Int(Saved, TEXT("v")) != Version) return;
  bEnabled = true;
  State[SC04] = FMath::Clamp(S47J::Int(Saved, TEXT("sc04")), 0, 2);
  State[SC08] = FMath::Clamp(S47J::Int(Saved, TEXT("sc08")), 0, 2);
  State[SC11] = FMath::Clamp(S47J::Int(Saved, TEXT("sc11")), 0, 2);
  Act04.Begin(S47J::Obj(Saved, TEXT("sc04d")));
  return;
 }
 bEnabled = bNewCase && M && M->Story && M->Story->bEveningTimeline;
}

void FS47Spine::Stop()
{
 Act04.Stop();
 for (auto& P : Posts) if (P.IsValid()) P->SetVisibility(false);
 for (auto& S : Signs) if (S.IsValid()) S->SetVisibility(false);
}

S47J::FObj FS47Spine::Save() const
{
 if (!bEnabled) return nullptr;
 auto O = S47J::New(); S47J::Set(O, TEXT("v"), Version);
 S47J::Set(O, TEXT("sc04"), State[SC04]); S47J::Set(O, TEXT("sc08"), State[SC08]); S47J::Set(O, TEXT("sc11"), State[SC11]);
 if (State[SC04] >= 1) S47J::Set(O, TEXT("sc04d"), Act04.Save());
 return O;
}

FS47Spine::EGate FS47Spine::Pending() const
{
 if (!bEnabled) return GateCount;
 for (int32 G = 0; G < GateCount; ++G) if (State[G] == 1) return EGate(G);
 return GateCount;
}

bool FS47Spine::HoldsResidual() const { return bEnabled && State[SC04] == 1; }

bool FS47Spine::HoldChapterEnd(EGate G)
{
 if (!bEnabled || State[G] == 2) return false;
 State[G] = 1;
 if (M && M->UI) M->UI->Toast(FString::Printf(TEXT("%s. Next: %s. This part of the night is a marked placeholder."), M->Story ? *M->Story->ClockText(false) : TEXT(""), Scenarios()[G == SC08 ? 7 : 10].Title), 5);
 UE_LOG(LogTemp, Display, TEXT("S47: spine holds %s placeholder"), GateItem(G));
 if (M) M->RequestAutosave(false);
 return true;
}

int32 FS47Spine::Current() const
{
 if (!M || !M->Story) return 0;
 const AS47Story& S = *M->Story;
 const auto Has = [&](const TCHAR* K) { return S.Begun.Contains(K); };
 if (Has(TEXT("ch6"))) return 12;
 if (Has(TEXT("ch5"))) return 11;
 if (State[SC11] >= 1) return 10;
 if (Has(TEXT("ch4"))) return 9;
 if (Has(TEXT("ch3"))) return 8;
 if (State[SC08] >= 1) return 7;
 if (Has(TEXT("ch2"))) return 6;
 if (Has(TEXT("ch1"))) return 5;
 if (State[SC04] == 2) return 4;
 if (State[SC04] == 1) return 3;
 if (M->Horror && M->Horror->SC03Live()) return 2;
 if (M->Horror && M->Horror->Complete()) return 1;
 return 0;
}

FString FS47Spine::Label(const FString& Id) const
{
 if (bEnabled && State[SC04] == 1 && Id != GateItem(SC04)) return Act04.Label(Id);
 for (int32 G = 0; G < GateCount; ++G)
  if (Id == GateItem(EGate(G))) return State[G] == 1 ? FString::Printf(TEXT("%s placeholder: read the card"), Scenarios()[G == SC04 ? 3 : G == SC08 ? 7 : 10].Id) : FString();
 return FString();
}

bool FS47Spine::Use(const FString& Id)
{
 if (bEnabled && State[SC04] == 1 && Id != GateItem(SC04)) return Act04.Use(Id);
 for (int32 G = 0; G < GateCount; ++G)
  if (Id == GateItem(EGate(G)) && State[G] == 1) { Finish(EGate(G)); return true; }
 return false;
}

void FS47Spine::Finish(EGate G)
{
 AS47Story& S = *M->Story;
 State[G] = 2;
 S.SetClock(FMath::Max(S.Clock(), GateDone(G)));
 const FS47Doc Doc = Card(G);
 S.AddDoc(Doc);
 const FScenario& Sc = Scenarios()[G == SC04 ? 3 : G == SC08 ? 7 : 10];
 S.Note(FString::Printf(TEXT("%s %s: placeholder read. The scenario is not built yet."), Sc.Id, Sc.Title));
 if (M->Audio) M->Audio->Play(TEXT("paper0"), .5f);
 ShowPosts();
 UE_LOG(LogTemp, Display, TEXT("S47: spine %s placeholder done at clock %.0f"), Sc.Id, S.Clock());
 M->UI->Document(Doc);
 if (G == SC08 && AfterSC08) AfterSC08();
 else if (G == SC11 && AfterSC11) AfterSC11();
 else M->UI->Toast(FString::Printf(TEXT("%s. Next: %s on the control-room console."), *S.ClockText(false), Scenarios()[4].Title), 5);
 M->RequestAutosave(false);
}

FString FS47Spine::Objective() const
{
 switch (Pending())
 {
 case SC04: return Act04.Objective();
 case SC08: return TEXT("ARRAY GATE // SC08 PLACEHOLDER: THEY POINT AT YOU. READ THE YELLOW PLACEHOLDER POST INSIDE THE ARRAY GATE");
 case SC11: return TEXT("SARO // SC11 PLACEHOLDER: SARO IS SEARCHED. READ THE YELLOW PLACEHOLDER POST IN THE CONTROL ROOM");
 default: return FString();
 }
}

TArray<FS47Task> FS47Spine::Tasks() const
{
 TArray<FS47Task> Out;
 const EGate G = Pending();
 if (G == GateCount) return Out;
 if (G == SC04) return Act04.Tasks();
 const FScenario& Sc = Scenarios()[G == SC08 ? 7 : 10];
 Out.Add({ FString::Printf(TEXT("%s %s (%s) - placeholder"), Sc.Id, Sc.Title, Sc.Window), false });
 Out.Add({ FString::Printf(TEXT("Read the marked card: %s"), Sc.Place), false });
 return Out;
}

void FS47Spine::Build()
{
 if (bBuilt || !M || !M->Web || !M->Inter || !M->Web->IsLoaded()) return;
 AS47Web* W = M->Web;
 const int32 Parent = W->Handle(TEXT("room.group"));
 const auto* ParentNode = W->Node(Parent);
 USceneComponent* ParentComp = ParentNode ? ParentNode->Comp.Get() : nullptr;
 auto* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if (!ParentComp || !Cube || !Base) return;
 bBuilt = true;
 auto* Mat = UMaterialInstanceDynamic::Create(Base, W);
 // Placeholder yellow, so nobody takes it for finished content.
 Mat->SetVectorParameterValue(TEXT("Color"), S47Three::Hex(0xd8b21c)); Mat->SetScalarParameterValue(TEXT("Roughness"), .5f);
 Mat->SetScalarParameterValue(TEXT("NativeLighting"), 1); Mat->SetScalarParameterValue(TEXT("EmissivePower"), .4f);
 for (int32 G = 0; G < GateCount; ++G)
 {
  const EGate E = EGate(G);
  const FString Name = FString::Printf(TEXT("SpinePost%d"), G);
  auto* C = NewObject<UStaticMeshComponent>(W, *Name);
  C->SetMobility(EComponentMobility::Movable); C->SetupAttachment(ParentComp); C->SetStaticMesh(Cube); C->SetMaterial(0, Mat);
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly); C->SetCollisionResponseToAllChannels(ECR_Ignore);
  C->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); C->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
  C->RegisterComponent(); C->SetWorldLocation(S47Three::ToUE(GatePoint(E)));
  const FVector Scale(.3, .5, .3);
  C->SetWorldScale3D(S47Three::DirToUE(Scale));
  FS47Obj Obj = W->ObjAt(W->AdoptNode(C, Parent, FString::Printf(TEXT("spine:post%d"), G))); Obj.SetScale(Scale);
  auto* T = NewObject<UTextRenderComponent>(W, *(Name + TEXT("Text")));
  T->SetMobility(EComponentMobility::Movable); T->SetupAttachment(ParentComp);
  T->SetText(FText::FromString(FString::Printf(TEXT("%s\nPLACEHOLDER"), Scenarios()[E == SC04 ? 3 : E == SC08 ? 7 : 10].Id)));
  T->SetWorldSize(3.f); T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter);
  T->SetTextRenderColor(FColor(30, 24, 8)); T->SetCollisionEnabled(ECollisionEnabled::NoCollision); T->RegisterComponent();
  // The sign faces the stand point.
  const FVector Face = (StandPoint(E) - GatePoint(E)).GetSafeNormal2D();
  T->SetWorldLocation(S47Three::ToUE(GatePoint(E) + FVector(Face.X * .17, .32, Face.Z * .17)));
  T->SetWorldRotation(FRotator(0, FMath::RadiansToDegrees(FMath::Atan2(Face.X, Face.Z)), 0));
  W->AdoptNode(T, Parent, FString::Printf(TEXT("spine:sign%d"), G));
  Posts.Add(C); Signs.Add(T);
  FS47Interactable It; It.Id = GateItem(E); It.Object = Obj; It.Range = 2.4;
  It.Label = [this, E]() { return Label(GateItem(E)); };
  It.Use = [this, E] { Use(GateItem(E)); };
  It.Examine = [this, E] { return State[E] == 1 ? FString(TEXT("A yellow placeholder post. This part of the night is not built yet; the card says what will happen.")) : FString(); };
  M->Inter->Add(It);
 }
 ShowPosts();
}

void FS47Spine::ShowPosts()
{
 for (int32 G = 0; G < Posts.Num() && G < GateCount; ++G)
 {
  const bool bShow = bEnabled && State[G] == 1 && G != SC04;
  if (Posts[G].IsValid()) Posts[G]->SetVisibility(bShow);
  if (Signs.IsValidIndex(G) && Signs[G].IsValid()) Signs[G]->SetVisibility(bShow);
  if (Posts[G].IsValid()) Posts[G]->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
 }
}

void FS47Spine::Step(double Dt)
{
 if (!bEnabled || !M || !M->Story) return;
 Build();
 // SC03's end opens SC04: the control room holds until the originals are secured.
 if (State[SC04] == 0 && (bTestSC03 || (M->Horror && M->Horror->SC03 && M->Horror->SC03->Complete())))
 {
  State[SC04] = 1;
  UE_LOG(LogTemp, Display, TEXT("S47: spine opens SC04 after SC03"));
  if (M->UI) M->UI->Toast(TEXT("Next: get back to SARO. The deputy needs the receiver log for the missing-person report."), 5);
  M->RequestAutosave(false);
 }
 if (State[SC04] == 1 && Act04.Step(Dt))
 {
  State[SC04] = 2;
  M->Story->SetClock(FMath::Max(M->Story->Clock(), GateDone(SC04)));
  UE_LOG(LogTemp, Display, TEXT("S47: spine SC04 done at clock %.0f"), M->Story->Clock());
  M->RequestAutosave(false);
 }
 ShowPosts();
 if (M->Mode == US47Main::EMode::Play && M->UI && Pending() != GateCount) M->UI->Objective(Objective());
}

bool FS47Spine::SelfTest(TArray<FString>& Lines)
{
 const auto& L = Scenarios();
 const bool Thirteen = L.Num() == 13 && FString(L[0].Id) == TEXT("SC01") && FString(L[12].Id) == TEXT("SC13");
 const bool Ordered = GateDone(SC04) > S47Timeline::TownReport && GateDone(SC04) < GateDone(SC08) && GateDone(SC08) < GateDone(SC11);
 const bool Chapters = GateDone(SC08) < S47Timeline::Chapter3(true) && S47Timeline::Chapter3(true) < S47Timeline::Chapter4(true) &&
  S47Timeline::Chapter4(true) < GateDone(SC11) && GateDone(SC11) < S47Timeline::Chapter5(true) && S47Timeline::Chapter5(true) < S47Timeline::Departure;
 bool Cards = true; for (int32 G = 0; G < GateCount; ++G) Cards &= Card(EGate(G)).Page.StartsWith(TEXT("PLACEHOLDER"));
 const bool Ok = Thirteen && Ordered && Chapters && Cards;
 Lines.Add(FString::Printf(TEXT("%s spine: 13 scenarios, placeholders ordered SC04 %.0f < SC08 %.0f < ch3 %.0f < ch4 %.0f < SC11 %.0f < ch5 %.0f < departure"),
  Ok ? TEXT("PASS") : TEXT("FAIL"), GateDone(SC04), GateDone(SC08), S47Timeline::Chapter3(true), S47Timeline::Chapter4(true), GateDone(SC11), S47Timeline::Chapter5(true)));
 return Ok;
}

void FS47Spine::ControlTest(double Dt)
{
 if (!Test.bInit) { Test.bInit = true; FParse::Value(FCommandLine::Get(), TEXT("S47SpineContract="), Test.Mode); }
 if (Test.Mode == TEXT("sc04") || Test.Mode == TEXT("sc04hide") || Test.Mode == TEXT("sc04long")) { FS47SC04Contract::Step(M, Dt); return; }
 if (Test.Mode.IsEmpty() || Test.bDone || !M || !M->Story || M->Mode != US47Main::EMode::Play) return;
 Test.T += Dt;
 const auto Fail = [this](const FString& Why)
 {
  Test.bDone = true;
  UE_LOG(LogTemp, Error, TEXT("S47: spine contract FAIL %s stage %d: %s"), *Test.Mode, Test.Stage, *Why);
  FPlatformMisc::RequestExit(false);
 };
 const auto Check = [this, &Fail](bool bGood, const FString& What)
 {
  if (!bGood) { Fail(What); return false; }
  ++Test.Checks; UE_LOG(LogTemp, Display, TEXT("S47: spine contract PASS: %s"), *What); return true;
 };
 const auto Next = [this](int32 S, double Wait = .3) { Test.Stage = S; Test.Since = Test.T + Wait; };
 if (Test.T < Test.Since) return;
 // An actual ray from the stand point when the player is at SARO; otherwise a labelled API fixture.
 const auto Read = [this, &Fail](EGate G)
 {
  const FString Id = GateItem(G);
  if (M->World && M->World->Area == TEXT("saro") && M->Player && M->Inter)
  {
   const FVector Eye = M->Player->EyeUE(), At = S47Three::ToUE(GatePoint(G));
   const FString Picked = M->Inter->Pick(Eye, (At - Eye).GetSafeNormal());
   if (Picked != Id || !M->Player->OnUse)
   {
    FHitResult Hit; FString What = TEXT("nothing");
    if (M->Web && M->Web->GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + (At - Eye).GetSafeNormal() * 400, ECC_Visibility))
     What = FString::Printf(TEXT("%s at %.0f cm"), Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("?"), Hit.Distance);
    const bool bVisible = Posts.IsValidIndex(G) && Posts[G].IsValid() && Posts[G]->IsVisible();
    Fail(FString::Printf(TEXT("ray to %s picked '%s'; trace hit %s; eye %s target %s; post visible %d; area %s"), *Id, *Picked, *What, *Eye.ToString(), *At.ToString(), bVisible, *M->World->Area));
    return false;
   }
   M->Player->OnUse(Picked);
   UE_LOG(LogTemp, Display, TEXT("S47: spine contract actual ray and production use: %s"), *Id);
  }
  else
  {
   UE_LOG(LogTemp, Display, TEXT("S47: spine contract API FIXTURE: player in area %s, %s used through Spine::Use"), M->World ? *M->World->Area : TEXT("?"), *Id);
   Use(Id);
  }
  if (M->UI) M->UI->Close(true);
  return true;
 };
 // The walker takes its new pose on the next tick: place, wait, then read.
 const auto Stand = [this](EGate G) { const FVector S = StandPoint(G); if (M->Player && M->World && M->World->Area == TEXT("saro")) { M->Player->Place(S.X, S.Z, 0); M->Player->Pitch = 0; } };
 const auto RoundTrip = [this](EGate G, int32 Want)
 {
  FS47Spine Fresh(nullptr); Fresh.Begin(S47J::Read(S47J::Write(Save())), false);
  return Fresh.Enabled() && Fresh.Gate(G) == Want;
 };
 if (Test.T > 240) { Fail(TEXT("timeout")); return; }
 const bool bSC08 = Test.Mode == TEXT("sc08");
 if (Test.Stage == 0)
 {
  if (Test.T < 3) return;
  if (!Check(bEnabled && M->Story->bEveningTimeline && M->Story->Begun.Contains(bSC08 ? TEXT("ch2") : TEXT("ch4")), TEXT("evening jump starts the spine inside its chapter"))) return;
  if (!bSC08) { Next(20); return; }
  TestOpenSC04(); Next(1, .4); return;
 }
 if (Test.Stage == 1)
 {
  if (!Check(Gate(SC04) == 1 && HoldsResidual() && Pending() == SC04 && Act04.Phase() == FS47SC04::EStep::Collect && Tasks().Num() == 5 && !(Posts.IsValidIndex(SC04) && Posts[SC04].IsValid() && Posts[SC04]->IsVisible()),
   TEXT("SC03's end opens the built SC04 at SARO: no placeholder post, five tasks, residual console held"))) return;
  const double Before = M->Story->Clock(); const int32 Stage = M->Story->Rx.Stage;
  M->Story->ConsoleAction();
  if (!Check(M->Story->Rx.Stage == Stage && M->Story->Clock() == Before, TEXT("the console refuses residual work while SC04 holds"))) return;
  if (!Check(RoundTrip(SC04, 1), TEXT("saved spine reloads with SC04 pending"))) return;
  // SC04 has its own contract (-S47SpineContract=sc04); here it is counted done by fixture.
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract FIXTURE: SC04 counted done without playing it"));
  Act04.TestComplete(); Next(5); return;
 }
 if (Test.Stage == 5)
 {
  if (!Check(Gate(SC04) == 2 && !HoldsResidual() && M->Story->Clock() >= GateDone(SC04), TEXT("SC04 done jumps to 03:15 and releases the console"))) return;
  Next(2); return;
 }
 if (Test.Stage == 2)
 {
  US47Chapter2* C2 = Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2")));
  if (!Check(C2 && C2->OnEnd, TEXT("chapter two's end is wired"))) return;
  C2->OnEnd();
  if (!Check(Gate(SC08) == 1 && Pending() == SC08 && !M->Story->Begun.Contains(TEXT("ch3")) && Current() == 7, TEXT("chapter two's end holds for the SC08 placeholder; chapter three waits"))) return;
  if (!Check(RoundTrip(SC08, 1), TEXT("saved spine reloads with SC08 pending"))) return;
  Next(3, 1.5); return;
 }
 if (Test.Stage == 3)
 {
  if (!Check(!M->Story->Begun.Contains(TEXT("ch3")), TEXT("chapter three still waits after a second and a half"))) return;
  Stand(SC08); Next(6); return;
 }
 if (Test.Stage == 6)
 {
  if (!Read(SC08)) return;
  if (!Check(Gate(SC08) == 2 && M->Story->Clock() >= GateDone(SC08), TEXT("reading the SC08 card jumps to 04:20 and continues chapter two's end"))) return;
  Next(4, 2.5); return;
 }
 if (Test.Stage == 4)
 {
  if (!M->Story->Begun.Contains(TEXT("ch3"))) { if (Test.T - Test.Since > 10) Fail(TEXT("chapter three did not begin")); return; }
  if (!Check(M->Story->Clock() >= S47Timeline::Chapter3(true) && Current() == 8, TEXT("chapter three begins as SC09 at 04:25"))) return;
  Next(99); return;
 }
 if (Test.Stage == 20)
 {
  US47Chapter4* C4 = Cast<US47Chapter4>(M->Story->Chapter(TEXT("ch4")));
  if (!Check(C4 && C4->OnEnd, TEXT("chapter four's end is wired"))) return;
  if (!Check(M->Story->Clock() >= S47Timeline::Chapter4(true) - 3 * 60, TEXT("chapter four runs at the SC10 time"))) return;
  C4->OnEnd();
  if (!Check(Gate(SC11) == 1 && Pending() == SC11 && !M->Story->Begun.Contains(TEXT("ch5")) && Current() == 10, TEXT("chapter four's end holds for the SC11 placeholder; chapter five waits"))) return;
  if (!Check(RoundTrip(SC11, 1), TEXT("saved spine reloads with SC11 pending"))) return;
  Next(21, 1.5); return;
 }
 if (Test.Stage == 21)
 {
  if (!Check(!M->Story->Begun.Contains(TEXT("ch5")), TEXT("chapter five still waits after a second and a half"))) return;
  Stand(SC11); Next(23); return;
 }
 if (Test.Stage == 23)
 {
  if (!Read(SC11)) return;
  if (!Check(Gate(SC11) == 2 && M->Story->Clock() >= GateDone(SC11), TEXT("reading the SC11 card jumps to 05:10 and continues chapter four's end"))) return;
  Next(22, 2.5); return;
 }
 if (Test.Stage == 22)
 {
  if (!M->Story->Begun.Contains(TEXT("ch5"))) { if (Test.T - Test.Since > 10) Fail(TEXT("chapter five did not begin")); return; }
  if (!Check(M->Story->Clock() >= S47Timeline::Chapter5(true) && M->Story->Clock() < S47Timeline::Departure && Current() == 11, TEXT("chapter five begins as SC12 at 05:12, before the 05:20 departure"))) return;
  Next(99); return;
 }
 if (Test.Stage == 99)
 {
  Test.bDone = true;
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract %s PASS COMPLETE: %d checks"), *Test.Mode, Test.Checks);
  FPlatformMisc::RequestExit(false);
 }
}
