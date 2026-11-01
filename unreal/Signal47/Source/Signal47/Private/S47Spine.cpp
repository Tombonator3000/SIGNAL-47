#include "S47Spine.h"
#include "S47SaroNight.h"
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
#include "S47Chapter3.h"
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

void FS47Spine::Begin(const S47J::FObj& Saved, bool bNewCase, bool bSavedFieldPresent)
{
 Stop();
 bEnabled = bTestSC03 = false; for (int32& S : State) S = 0;
 bMissingSavedSpine = !Saved.IsValid() && !bSavedFieldPresent;
 bUnrecognizedSavedSpine = !bMissingSavedSpine;
 Act04.Begin(nullptr); Act08.Begin(nullptr); Act11.Begin(nullptr);
 if (Saved)
 {
  if (S47J::Int(Saved, TEXT("v")) != Version) return;
  bUnrecognizedSavedSpine = false;
  bEnabled = true;
  State[SC04] = FMath::Clamp(S47J::Int(Saved, TEXT("sc04")), 0, 2);
  State[SC08] = FMath::Clamp(S47J::Int(Saved, TEXT("sc08")), 0, 2);
  State[SC11] = FMath::Clamp(S47J::Int(Saved, TEXT("sc11")), 0, 2);
  Act04.Begin(S47J::Obj(Saved, TEXT("sc04d")));
  Act08.Begin(S47J::Obj(Saved, TEXT("sc08d")));
  Act11.Begin(S47J::Obj(Saved, TEXT("sc11d")));
  return;
 }
 bEnabled = bMissingSavedSpine && bNewCase && M && M->Story && M->Story->bEveningTimeline;
}

bool FS47Spine::ResumeAfterSC03()
{
 // Old route-2 nights reached the deputy before Spine was shipped. Resume only this
 // proven boundary, never retrofit chapter gates or invent a sighting in other saves.
 if (bEnabled || !bMissingSavedSpine || !M || !M->Story || !M->Story->NightSequence() || M->Story->ChapterKey != TEXT("prologue") ||
     M->Story->Phase() != ES47Phase::Residual || !M->Horror || !M->Horror->SC03 || !M->Horror->SC03->Complete()) return false;
 const auto& F = M->Horror->SC03->Facts();
 if (!F.bObserved || !F.bMissing || !F.bLocalOrder || F.MissingStamp.IsEmpty() || F.OrderStamp.IsEmpty()) return false;
 Begin(nullptr, true);
 if (!bEnabled) return false;
 State[SC04] = 1;
 if (M->UI) M->UI->Objective(Act04.Objective());
 UE_LOG(LogTemp, Display, TEXT("S47: recovered pre-Spine night after completed deputy report; SC04 available, originals retained"));
 M->RequestAutosave(false);
 return true;
}

void FS47Spine::Stop()
{
 Act04.Stop(); Act08.Stop(); Act11.Stop();
 for (auto& P : Posts) if (P.IsValid()) P->SetVisibility(false);
 for (auto& S : Signs) if (S.IsValid()) S->SetVisibility(false);
}

S47J::FObj FS47Spine::Save() const
{
 if (!bEnabled) return nullptr;
 auto O = S47J::New(); S47J::Set(O, TEXT("v"), Version);
 S47J::Set(O, TEXT("sc04"), State[SC04]); S47J::Set(O, TEXT("sc08"), State[SC08]); S47J::Set(O, TEXT("sc11"), State[SC11]);
 if (State[SC04] >= 1) S47J::Set(O, TEXT("sc04d"), Act04.Save());
 if (State[SC08] >= 1) S47J::Set(O, TEXT("sc08d"), Act08.Save());
 if (State[SC11] >= 1) S47J::Set(O, TEXT("sc11d"), Act11.Save());
 return O;
}

FS47Spine::EGate FS47Spine::Pending() const
{
 if (!bEnabled) return GateCount;
 for (int32 G = 0; G < GateCount; ++G) if (State[G] == 1) return EGate(G);
 return GateCount;
}

// From the 00:13 outage until SC04 is done the console is passive: the residual belongs to SC05 (03:15).
bool FS47Spine::HoldsResidual() const { return bEnabled && State[SC04] != 2 && M && M->Story && M->Story->Phase() == ES47Phase::Residual; }

bool FS47Spine::HoldChapterEnd(EGate G)
{
 if (!bEnabled || State[G] == 2) return false;
 State[G] = 1;
 if (M && M->UI) M->UI->Toast(FString::Printf(TEXT("%s. Next: %s.%s"), M->Story ? *M->Story->ClockText(false) : TEXT(""), Scenarios()[G == SC08 ? 7 : 10].Title, G == SC08 ? TEXT(" Earl's night line ends at four.") : TEXT(" Hondo and Site 11 are sending their sheets to SARO.")), 5);
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
 if (bEnabled && State[SC08] == 1 && Id != GateItem(SC08)) return Act08.Label(Id);
 if (bEnabled && State[SC11] == 1 && Id != GateItem(SC11)) return Act11.Label(Id);
 for (int32 G = 0; G < GateCount; ++G)
  if (Id == GateItem(EGate(G))) return State[G] == 1 ? FString::Printf(TEXT("%s placeholder: read the card"), Scenarios()[G == SC04 ? 3 : G == SC08 ? 7 : 10].Id) : FString();
 return FString();
}

bool FS47Spine::Use(const FString& Id)
{
 if (bEnabled && State[SC04] == 1 && Id != GateItem(SC04)) return Act04.Use(Id);
 if (bEnabled && State[SC08] == 1 && Id != GateItem(SC08)) return Act08.Use(Id);
 if (bEnabled && State[SC11] == 1 && Id != GateItem(SC11)) return Act11.Use(Id);
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
 case SC08: return Act08.Objective();
 case SC11: return Act11.Objective();
 default: return FString();
 }
}

TArray<FS47Task> FS47Spine::Tasks() const
{
 TArray<FS47Task> Out;
 const EGate G = Pending();
 if (G == GateCount) return Out;
 if (G == SC04) return Act04.Tasks();
 if (G == SC08) return Act08.Tasks();
 if (G == SC11) return Act11.Tasks();
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
  const bool bShow = false; // every gate is built now (SC04, SC08, SC11); the posts stay for older saves' code paths
  if (Posts[G].IsValid()) Posts[G]->SetVisibility(bShow);
  if (Signs.IsValidIndex(G) && Signs[G].IsValid()) Signs[G]->SetVisibility(bShow);
  if (Posts[G].IsValid()) Posts[G]->SetCollisionEnabled(bShow ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
 }
}

void FS47Spine::Step(double Dt)
{
 if (!bEnabled) ResumeAfterSC03();
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
  M->Story->After(8.0, [S = TWeakObjectPtr<AS47Story>(M->Story.Get())] { if (S.IsValid()) S->Toast(TEXT("03:15. The residual is still there under the hydrogen line. The console."), 5); });
  M->RequestAutosave(false);
 }
 if (State[SC08] == 1 && Act08.Step(Dt))
 {
  State[SC08] = 2;
  M->Story->SetClock(FMath::Max(M->Story->Clock(), GateDone(SC08)));
  UE_LOG(LogTemp, Display, TEXT("S47: spine SC08 done at clock %.0f; chapter two's end continues"), M->Story->Clock());
  M->RequestAutosave(false);
  if (AfterSC08) AfterSC08();
 }
 if (State[SC11] == 1 && Act11.Step(Dt))
 {
  State[SC11] = 2;
  M->Story->SetClock(FMath::Max(M->Story->Clock(), GateDone(SC11)));
  UE_LOG(LogTemp, Display, TEXT("S47: spine SC11 done at clock %.0f; chapter four's end continues"), M->Story->Clock());
  M->RequestAutosave(false);
  if (AfterSC11) AfterSC11();
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
 bool Protects = true;
 FS47Spine Guard(nullptr);
 auto Saved = S47J::New(); S47J::Set(Saved, TEXT("v"), Version);
 for (const TCHAR* Key : {TEXT("sc04"), TEXT("sc08"), TEXT("sc11")})
 {
  S47J::Set(Saved, Key, 1); Guard.Begin(Saved, false); Protects &= Guard.Protected();
  S47J::Set(Saved, Key, 2); Guard.Begin(Saved, false); Protects &= !Guard.Protected();
 }
 auto Future=S47J::New();S47J::Set(Future,TEXT("v"),Version+1);Guard.Begin(Future,false,true);
 const bool FutureKept=!Guard.Enabled() && Guard.PreserveUnknownSave() && !Guard.bMissingSavedSpine;
 Guard.Begin(nullptr,false,true); // The raw field existed but was not an object (including explicit null).
 const bool MalformedKept=!Guard.Enabled() && Guard.PreserveUnknownSave() && !Guard.bMissingSavedSpine;
 Guard.Begin(nullptr,false);
 const bool MissingRecoverable=Guard.bMissingSavedSpine && !Guard.PreserveUnknownSave();
 const bool SaveBoundary=FutureKept && MalformedKept && MissingRecoverable;
 const bool Ok = Thirteen && Ordered && Chapters && Cards && Protects && SaveBoundary;
 Lines.Add(FString::Printf(TEXT("%s spine: 13 scenarios, placeholders ordered SC04 %.0f < SC08 %.0f < ch3 %.0f < ch4 %.0f < SC11 %.0f < ch5 %.0f < departure"),
  Ok ? TEXT("PASS") : TEXT("FAIL"), GateDone(SC04), GateDone(SC08), S47Timeline::Chapter3(true), S47Timeline::Chapter4(true), GateDone(SC11), S47Timeline::Chapter5(true)));
 Lines.Add(FString::Printf(TEXT("%s spine: every active authored gate is protected until complete"), Protects ? TEXT("PASS") : TEXT("FAIL")));
 Lines.Add(FString::Printf(TEXT("%s spine: only absent legacy state is recoverable; unknown and malformed present fields are retained"), SaveBoundary ? TEXT("PASS") : TEXT("FAIL")));
 return Ok;
}

void FS47Spine::ControlTest(double Dt)
{
 if (!Test.bInit) { Test.bInit = true; FParse::Value(FCommandLine::Get(), TEXT("S47SpineContract="), Test.Mode); }
 if (Test.Mode.IsEmpty() || Test.bDone || !M || !M->Story || M->Mode != US47Main::EMode::Play) return;
 if (Test.Mode == TEXT("sc09"))
 {
  if (auto* C3 = Cast<US47Chapter3>(M->Story->Chapter(TEXT("ch3")))) C3->ControlSC09Test(Dt);
  return;
 }
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
 if (Test.Mode.StartsWith(TEXT("sc04"))) { SC04Test(Dt, Fail, Check); return; }
 if (Test.Mode.StartsWith(TEXT("sc08play")) || Test.Mode == TEXT("sc08caught")) { SC08Test(Dt, Fail, Check); return; }
 if (Test.Mode == TEXT("sc11play") || Test.Mode == TEXT("sc11caught")) { SC11Test(Dt, Fail, Check); return; }
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
  // (The console hold belongs to the residual phase after 00:13; the sc04 contract checks it.)
  if (!Check(Gate(SC04) == 1 && Pending() == SC04 && Act04.Phase() == FS47SC04::EStep::Collect && Tasks().Num() == 5 && !(Posts.IsValidIndex(SC04) && Posts[SC04].IsValid() && Posts[SC04]->IsVisible()),
   TEXT("SC03's end opens the built SC04 at SARO: no placeholder post, five tasks"))) return;
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
  // SC08 has its own contract (-S47SpineContract=sc08play); here it is counted done by fixture.
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract FIXTURE: SC08 counted done without playing it"));
  Act08.TestComplete(); Next(6); return;
 }
 if (Test.Stage == 6)
 {
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
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract FIXTURE: SC11 counted done without playing it (-S47SpineContract=sc11play plays it)"));
  Act11.TestComplete(); Next(23); return;
 }
 if (Test.Stage == 23)
 {
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

// The SC04 contract from the residual jump, with SC03's end by fixture. Every original, the wall
// phone and the doors are used by an actual ray from a walkable stand point and the production use
// path; the player crosses the annex in small walker steps at walking speed, so the body hears and
// sees what a player would give it. Door opening into the annex before the binder is a fixture.
void FS47Spine::SC04Test(double Dt, TFunctionRef<void(const FString&)> Fail, TFunctionRef<bool(bool, const FString&)> Check)
{
 const FS47SC04& A = Act04;
 const bool bDoors = Test.Mode == TEXT("sc04doors"), bHide = Test.Mode == TEXT("sc04hide"), bLong = Test.Mode == TEXT("sc04long"), bShort = !bHide && !bLong;
 const auto Next = [this](int32 S, double Wait = .3) { Test.Stage = S; Test.Since = Test.T + Wait; Test.Mark = Test.T; Test.Cand = 0; };
 if (Test.T < Test.Since) return;
 if (Test.T > 300) { Fail(FString::Printf(TEXT("timeout; SC04 step %d body %s at %s"), int32(A.Phase()), FS47SC04::BodyName(A.Body()), *A.BodyAt().ToString())); return; }
 // FS47HorrorTest::Target: the item's current world box, so an open door is aimed at where its leaf is now.
 const auto ItemAt = [this](const TCHAR* Id)
 {
  if (const auto* Item = M->Web->Item(Id)) { const FBox Box = M->Web->ObjAt(Item->Node).WorldBox(); if (Box.IsValid) return Box.GetCenter(); }
  return S47Three::ToThree(M->Web->PropLocation(Id));
 };
 // Stand candidates on rings round the item; the next one is tried when the ray picks something else.
 const auto Stand = [this, &ItemAt](const TCHAR* Id, TOptional<FVector2D> Fixed)
 {
  const FVector I = ItemAt(Id);
  FVector2D S = Fixed ? *Fixed : FVector2D::ZeroVector;
  if (!Fixed)
  {
   int32 Seen = 0; bool bFound = false;
   for (const double R : {1.1, 1.5, 1.9}) for (int32 K = 0; K < 16 && !bFound; ++K)
   {
    const FVector2D C(I.X + R * FMath::Cos(K * PI / 8), I.Z + R * FMath::Sin(K * PI / 8));
    if (!M->Player->Walkable(C.X, C.Y)) continue;
    {
     // A stand point must see the item: the use ray answers through walls, a player does not.
     FHitResult Hit; const FVector From = S47Three::ToUE(FVector(C.X, 1.62, C.Y)), To = S47Three::ToUE(I);
     if (M->Web->GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility) && Hit.Distance < FVector::Dist(From, To) - 35) continue;
    }
    if (FString(Id) == TEXT("recordsDoor") && (C.Y < 7.75 || C.X < -5.4)) continue; // from inside the records room, clear of the door zone
    if (FString(Id) == TEXT("doorSouth") && C.Y > 3.5) continue;                     // from the control room, clear of the door zone
    if (Seen++ == Test.Cand) { S = C; bFound = true; }
   }
   if (!bFound) return false;
  }
  M->Player->Place(S.X, S.Y, FMath::Atan2(-(I.X - S.X), -(I.Z - S.Y))); M->Player->Pitch = 0;
  return true;
 };
 const auto Use = [this, &ItemAt](const TCHAR* Id)
 {
  const FVector Eye = M->Player->EyeUE(), At = S47Three::ToUE(ItemAt(Id));
  const FString Picked = M->Inter->Pick(Eye, (At - Eye).GetSafeNormal());
  if (Picked != Id) return Picked.IsEmpty() ? FString(TEXT("nothing")) : Picked;
  M->Player->OnUse(Picked);
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract actual ray and production use: %s"), Id);
  if (M->UI) M->UI->Close(true);
  return FString();
 };
 // Stage pairs: stand (even), then ray and use (odd), retrying stand points.
 const auto StandUse = [&](int32 Stage, const TCHAR* Id, TOptional<FVector2D> Fixed, TFunctionRef<void()> Done)
 {
  if (Test.Stage == Stage) { if (!Stand(Id, Fixed)) { Fail(FString::Printf(TEXT("no walkable stand point for %s after %d; last ray: %s"), Id, Test.Cand, *Test.Miss)); return; } Test.Stage = Stage + 1; Test.Since = Test.T + .4; return; }
  const FString Miss = Use(Id);
  if (Miss.IsEmpty()) { Done(); return; }
  Test.Miss = FString::Printf(TEXT("%s from %s to %s (door open %d)"), *Miss, *FVector2D(M->Player->Pos.X, M->Player->Pos.Z).ToString(), *ItemAt(Id).ToString(), M->Web->IsDoorOpen(Id));
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract ray miss for %s: %s"), Id, *Test.Miss);
  if (Fixed || ++Test.Cand > 40) { Fail(FString::Printf(TEXT("ray to %s picked %s from %s"), Id, *Miss, *FVector2D(M->Player->Pos.X, M->Player->Pos.Z).ToString())); return; }
  Test.Stage = Stage;
 };
 // Walker steps along a route (1.9 m/s walking, 3.0 running); true when the last point is reached.
 const auto Glide = [this, Dt](double Speed = 1.9)
 {
  if (Test.Route.IsEmpty()) return true;
  const FVector2D P(M->Player->Pos.X, M->Player->Pos.Z), D = Test.Route[0] - P;
  if (Test.T - Test.Mark > 40) UE_LOG(LogTemp, Warning, TEXT("S47: spine contract glide slow at %s to %s"), *P.ToString(), *Test.Route[0].ToString());
  const double Step = Speed * Dt;
  const FVector2D N = D.Size() <= Step ? Test.Route[0] : P + D.GetSafeNormal() * Step;
  M->Player->Place(N.X, N.Y, FMath::Atan2(-D.X, -D.Y));
  if (D.Size() <= Step) Test.Route.RemoveAt(0);
  return Test.Route.IsEmpty();
 };
 const auto Outside = [](const FVector2D& P) { return FS47SC04::Room(P) == 3; };
 switch (Test.Stage)
 {
 case 0:
  if (Test.T < 3) return;
  if (!Check(bEnabled && M->Story->bEveningTimeline && M->World->Area == TEXT("saro"), TEXT("residual jump starts the spine at SARO"))) return;
  Test.ClockMark = M->Story->Clock(); TestOpenSC04(); Next(1, .5); return;
 case 1:
 {
  if (!Check(Gate(SC04) == 1 && HoldsResidual() && A.Phase() == FS47SC04::EStep::Collect && M->Story->Clock() >= 2 * 3600 + 55 * 60 && Tasks().Num() == 5 && Objective().Contains(TEXT("OPS TERMINAL")),
   TEXT("SC03's end opens SC04; at SARO the drive is over, 02:55, five tasks"))) return;
  const int32 Stage = M->Story->Rx.Stage; M->Story->ConsoleAction();
  if (!Check(M->Story->Rx.Stage == Stage, TEXT("the residual console waits while SC04 runs"))) return;
  if (!Check(A.Label(TEXT("binder")) == TEXT("Take the run binder for the report") && A.Label(TEXT("recPhone")).IsEmpty(), TEXT("the binder is offered, the wall phone not before the meeting"))) return;
  Next(10); return;
 }
 case 10: case 11: StandUse(10, TEXT("opsTerminal"), {}, [&] { if (Check(A.Facts().bSchedule && M->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("sc04_schedule"); }), TEXT("the ops terminal holds the automatic schedule"))) Next(12); }); return;
 case 12: case 13: StandUse(12, TEXT("printer"), {}, [&] { if (Check(A.Facts().bStrip && M->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("sc04_strip"); }), TEXT("the strip chart comes off the printer"))) Next(14); }); return;
 case 14: case 15: StandUse(14, TEXT("logbook"), {}, [&] { if (Check(A.Facts().bJournal && A.Phase() == FS47SC04::EStep::Collect, TEXT("the shift journal is taken; nothing has happened yet"))) Next(16); }); return;
 case 16:
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract FIXTURE: corridor and records doors opened by API before the walk in"));
  M->Web->SetDoorOpen(TEXT("doorSouth"), true, true); M->Web->SetDoorOpen(TEXT("recordsDoor"), true, true);
  Next(17); return;
 case 17: case 18: StandUse(17, TEXT("binder"), FVector2D(-2.3, 9.45), [&] { if (Check(A.Facts().bBinder && A.HuntLive() && A.Body() == FS47SC04::EBody::Away, TEXT("taking the binder starts the annex meeting"))) { Test.ClockMark = M->Story->Clock(); Test.Hop = 0; Next(bDoors ? 80 : bHide ? 40 : bLong ? 60 : 20); } }); return;
 // ---- sc04doors: real body crossing, then an explicitly labelled east-landing pose ----
 case 80:
 {
  const FVector2D Body=A.BodyAt(); const double Dx=Body.X-M->Player->Pos.X,Dz=Body.Y-M->Player->Pos.Z;
  M->Player->Yaw=FMath::Atan2(-Dx,-Dz); M->Player->Pitch=.05;
  if (!A.DoorOccupied(TEXT("recordsDoor")) || !A.TestNativeBodyRendered())
  { if(Test.T-Test.Mark>40)Fail(TEXT("native visible Tripo never naturally reached the open records doorway")); return; }
  if(!Check(M->Web->IsDoorOpen(TEXT("recordsDoor")) && !M->Web->ToggleDoor(TEXT("recordsDoor")) && M->Web->IsDoorOpen(TEXT("recordsDoor")),
   TEXT("normal ToggleDoor refuses closing through the actually rendered Tripo body at the records leaf")))return;
  Test.ClockMark=Body.Y; Next(81,0); return;
 }
 case 81:
  if(A.Facts().Captures){Fail(TEXT("captured before the doorway crossing cleared"));return;}
  if(A.DoorOccupied(TEXT("recordsDoor")))
  {if(Test.T-Test.Mark>8)Fail(FString::Printf(TEXT("body failed to continue through the door after refused closure: %s at %s"),FS47SC04::BodyName(A.Body()),*A.BodyAt().ToString()));return;}
  if(!Check(A.BodyAt().Y>Test.ClockMark+.15 && M->Web->ToggleDoor(TEXT("recordsDoor")) && !M->Web->IsDoorOpen(TEXT("recordsDoor")),
   TEXT("body naturally clears the opening and the same ordinary close now latches")))return;
  // Independent spatial fixture: the production patrol has no route to this landing.
  // Do not award escape or force Calm/Finish; all following movement is ordinary AI.
  UE_LOG(LogTemp,Display,TEXT("S47: SC04 doors explicit pose FIXTURE east landing (8.6,1.6), listening toward player control (4.3,1.6); normal AI, no state completion"));
  M->Web->SetDoorOpen(TEXT("doorSouth"),false,true);M->Web->SetDoorOpen(TEXT("doorEast"),true,true);
  M->Player->Place(4.3,1.6,-PI/2);M->Player->Pitch=.05;
  Act04.TestPlaceBody(FVector2D(8.6,1.6),PI);
  Next(82,.3);return;
 case 82:
  if(!A.TestNativeBodyRendered() || !A.TestSeesPlayer())
  {if(Test.T-Test.Mark>3)Fail(TEXT("east fixture lacks rendered Tripo or actual sight through open service doorway"));return;}
  if(!Check(!A.TestPlayerReachable() && !M->Web->IsDoorOpen(TEXT("doorSouth")) && M->Web->IsDoorOpen(TEXT("doorEast")),
   TEXT("actual service-door sight exists while the closed south door makes the player unreachable")))return;
  Next(83,.75);return;
 case 83:
  if(!Check(A.HuntLive() && A.Body()!=FS47SC04::EBody::Chase && A.Body()!=FS47SC04::EBody::Grab && FS47SC04::Room(A.BodyAt())!=0 && !A.Facts().Captures,
   TEXT("unreachable visible target does not trap AI in Chase, grab through the wall or enter the refuge")))return;
  if(A.DoorOccupied(TEXT("doorEast")))
  {Fail(TEXT("east body reached the leaf before the refuge-close check"));return;}
  if(!Check(M->Web->ToggleDoor(TEXT("doorEast")) && !M->Web->IsDoorOpen(TEXT("doorEast")),TEXT("ordinary service-door ToggleDoor secures the player behind both doors")))return;
  Next(84,0);return;
 case 84:
  if(A.Facts().Captures || FS47SC04::Room(A.BodyAt())==0){Fail(TEXT("body crossed the secured service door or captured the player"));return;}
  if(Gate(SC04)!=2)
  {if(Test.T-Test.Mark>45)Fail(FString::Printf(TEXT("secured east-landing refuge failed physical withdrawal: %s at %s"),FS47SC04::BodyName(A.Body()),*A.BodyAt().ToString()));return;}
  if(!Check(A.Facts().bEscaped && A.Facts().bSchedule && A.Facts().bStrip && A.Facts().bJournal && A.Facts().bBinder,
   TEXT("ordinary AI physically withdraws and releases SC04 with all four originals intact")))return;
  Test.bDone=true;UE_LOG(LogTemp,Display,TEXT("S47: spine contract sc04doors PASS COMPLETE: %d checks"),Test.Checks);FPlatformMisc::RequestExit(false);return;
 // ---- sc04: walk east into the corridor and be taken, then the lure ----
 case 20: Test.Route = {FVector2D(-2.3, 7.7), FVector2D(-3.9, 7.5), FVector2D(-3.9, 5.75), FVector2D(-1.2, 5.75)}; Next(21, 0); return;
 case 21:
  if (A.Facts().Captures == 0) { Glide(); if (Test.T - Test.Mark > 40) Fail(TEXT("no capture in the corridor")); return; }
  if (Test.Hop == 0) { Test.Hop = 1; Test.Since = Test.T + .3; return; } // the walker takes the new place next tick
  Test.Hop = 0;
  {
   const double Away = FVector2D::Distance(FVector2D(M->Player->Pos.X, M->Player->Pos.Z), FVector2D(-2.3, 9.45));
   if (!Check(A.Facts().Captures == 1 && A.HuntLive() && A.Body() == FS47SC04::EBody::Away && Away < .6 && M->Story->Clock() - Test.ClockMark < 60,
    FString::Printf(TEXT("met in the corridor out of the office and taken: the annex starts again at the binder, originals kept, clock unmoved (captures %d body %s, %.2f m from the binder, clock +%.0f s)"),
     A.Facts().Captures, FS47SC04::BodyName(A.Body()), Away, M->Story->Clock() - Test.ClockMark))) return;
  }
  if (!Check(A.Facts().bSchedule && A.Facts().bStrip && A.Facts().bJournal && A.Facts().bBinder, TEXT("all four originals survive the capture"))) return;
  Next(22, .2); return;
 case 22: case 23: StandUse(22, TEXT("recordsDoor"), {}, [&] { if (Check(!M->Web->IsDoorOpen(TEXT("recordsDoor")), TEXT("the records door is shut from inside before it comes"))) Next(24, 0); }); return;
 case 24:
  if (A.Body() == FS47SC04::EBody::Chase || A.Facts().Captures > 1) { Fail(TEXT("found behind the shut records door")); return; }
  if (A.Body() != FS47SC04::EBody::Listen) { if (Test.T - Test.Mark > 40) Fail(FString::Printf(TEXT("the body never listened at the records door: %s at %s"), FS47SC04::BodyName(A.Body()), *A.BodyAt().ToString())); return; }
  Check(true, TEXT("the shut door was heard: the body stops at it and listens"));
  Next(25, 0); return;
 case 25: case 26: StandUse(25, TEXT("recPhone"), FVector2D(-1.75, 9.3), [&] { if (Check(A.Facts().bLure, TEXT("the wall phone rings the operations office while it listens"))) Next(27); }); return;
 case 27:
  if (A.Body() == FS47SC04::EBody::Chase || A.Facts().Captures > 1) { Fail(TEXT("the body found the player instead of going to the ringing")); return; }
  if (A.Body() != FS47SC04::EBody::Lured || FVector2D::Distance(A.BodyAt(), FVector2D(.4, 5.75)) > .3) { if (Test.T - Test.Mark > 30) Fail(FString::Printf(TEXT("the body did not reach the office door: %s at %s"), FS47SC04::BodyName(A.Body()), *A.BodyAt().ToString())); return; }
  Check(true, TEXT("the body walks back to the ringing office door and listens there"));
  Next(28, 0); return;
 case 28: case 29: StandUse(28, TEXT("recordsDoor"), {}, [&] { Test.Route = {FVector2D(-3.9, 7.6), FVector2D(-3.9, 5.75), FVector2D(-2.4, 5.75), FVector2D(-2.4, 2.9)}; Next(30, 0); }); return;
 case 30:
  if (A.Body() == FS47SC04::EBody::Chase || A.Facts().Captures > 1) { Fail(TEXT("seen on the short way past the listening body")); return; }
  if (Glide()) Next(31, .3);
  return;
 case 31: case 32: StandUse(31, TEXT("doorSouth"), {}, [&] { if (Check(!M->Web->IsDoorOpen(TEXT("doorSouth")), TEXT("the corridor door is shut from the control room"))) Next(50); }); return;
 // ---- sc04hide: shut the records door and keep still while it walks the annex ----
 case 40: case 41: StandUse(40, TEXT("recordsDoor"), {}, [&] { if (Check(!M->Web->IsDoorOpen(TEXT("recordsDoor")), TEXT("the records door is shut from inside"))) Next(42, 0); }); return;
 case 42:
  if (A.Body() != FS47SC04::EBody::Away && FS47SC04::Room(A.BodyAt()) == 2) { Fail(TEXT("the body went into the records room behind a shut door")); return; }
  if (A.Body() == FS47SC04::EBody::Chase || A.Facts().Captures > 0) { Fail(TEXT("found behind the shut records door")); return; }
  if (A.Body() == FS47SC04::EBody::Listen) Test.Hop = 1;
  if (A.Phase() != FS47SC04::EStep::Calm) { if (Test.T - Test.Mark > 150) Fail(FString::Printf(TEXT("the body did not leave: %s at %s"), FS47SC04::BodyName(A.Body()), *A.BodyAt().ToString())); return; }
  if (!Check(A.Facts().bEscaped && A.Facts().Captures == 0 && Test.Hop == 1, TEXT("still behind the shut door: it listened there, walked the annex and went back into the office"))) return;
  Test.Hop = 0; Next(43); return;
 case 43: case 44: StandUse(43, TEXT("recordsDoor"), {}, [&] { Test.Route = {FVector2D(-3.9, 7.5), FVector2D(-3.9, 5.75), FVector2D(-2.4, 5.75), FVector2D(-2.4, 2.9)}; Next(45, 0); }); return;
 case 45: if (Glide()) Next(31, .3); return;
 // ---- sc04long: out by the fire exit, round the north side, in by the service door ----
 case 60: Test.Route = {FVector2D(-2.3, 7.7), FVector2D(-3.9, 7.5), FVector2D(-3.9, 5.75), FVector2D(-4.75, 5.7)}; Next(61, 0); return;
 case 61: if (Glide(3.0)) Next(62, 0); return;
 case 62: case 63: StandUse(62, TEXT("exitDoor"), FVector2D(-4.75, 5.7), [&] { if (Check(M->Web->IsDoorOpen(TEXT("exitDoor")), TEXT("the long way: the fire exit opens from the corridor"))) {
   Test.Route = {FVector2D(-7.0, 5.7), FVector2D(-11.1, 5.7), FVector2D(-11.1, 0.0), FVector2D(-9.0, -8.0), FVector2D(5.0, -8.0), FVector2D(5.0, -12.0), FVector2D(9.5, -12.0), FVector2D(9.5, 0.0), FVector2D(9.0, 1.6), FVector2D(7.6, 1.6)};
   Next(64, 0); } }); return;
 case 64:
  if (A.Body() == FS47SC04::EBody::Chase || A.Facts().Captures > 0) { Fail(FString::Printf(TEXT("caught on the long way at %s"), *FVector2D(M->Player->Pos.X, M->Player->Pos.Z).ToString())); return; }
  if (A.Body() != FS47SC04::EBody::Away && Outside(A.BodyAt())) Test.Hop = 1; // it followed the sound of the fire exit out
  if (Glide(3.0)) Next(65, 0);
  return;
 case 65: case 66: StandUse(65, TEXT("doorEast"), FVector2D(7.6, 1.6), [&] { if (Check(M->Web->IsDoorOpen(TEXT("doorEast")), TEXT("round the north side: the service door opens from the landing"))) { Test.Route = {FVector2D(6.5, 1.6), FVector2D(4.3, 1.6)}; Next(67, 0); } }); return;
 case 67: if (Glide(3.0)) Next(68, .3); return;
 case 68: case 69: StandUse(68, TEXT("doorEast"), FVector2D(4.3, 1.6), [&] { if (Check(!M->Web->IsDoorOpen(TEXT("doorEast")), TEXT("the service door is shut behind you"))) Next(31, .3); }); return;
 // ---- both: locked in, Ward, 03:15 ----
 case 50:
  if (bLong && A.Body() != FS47SC04::EBody::Away && Outside(A.BodyAt())) Test.Hop = 1;
  if (Gate(SC04) != 2) { if (Test.T - Test.Mark > 150) Fail(FString::Printf(TEXT("SC04 did not finish: step %d body %s"), int32(A.Phase()), FS47SC04::BodyName(A.Body()))); return; }
  if (bLong && !Check(Test.Hop == 1, TEXT("the fire exit's sound drew it outside along the west wall while the player went round"))) return;
  if (!Check(!HoldsResidual() && M->Story->Clock() >= GateDone(SC04) && A.Facts().bEscaped && A.Facts().Captures == (bShort ? 1 : 0) && A.Facts().bLure == bShort,
   TEXT("locked in the control room behind both doors: Ward on the line, 03:15, the residual console released"))) return;
  {
   FS47Spine Fresh(nullptr); Fresh.Begin(S47J::Read(S47J::Write(Save())), false);
   if (!Check(Fresh.Gate(SC04) == 2 && Fresh.Secure().Phase() == FS47SC04::EStep::Done && Fresh.Secure().Facts().Captures == A.Facts().Captures, TEXT("saved spine reloads with SC04 done and its facts"))) return;
  }
  Test.bDone = true;
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract %s PASS COMPLETE: %d checks"), *Test.Mode, Test.Checks);
  FPlatformMisc::RequestExit(false);
  return;
 default: return;
 }
}

// The SC08 contract from the chapter two jump, chapter two's end called as the chapter calls it.
// The reel at 1420 AM is an API fixture (the drive to Caliche has its own contracts); the sheet is
// an actual ray; the walk to the array gate is a labelled place fixture; on the field the player
// moves in walker steps (1.9 m/s walking, 3.0 running) so the dishes and the body sense a player.
void FS47Spine::SC08Test(double Dt, TFunctionRef<void(const FString&)> Fail, TFunctionRef<bool(bool, const FString&)> Check)
{
 const FS47SC08& A = Act08;
 const bool bCaught = Test.Mode == TEXT("sc08caught");
 const auto Next = [this](int32 S, double Wait = .3) { Test.Stage = S; Test.Since = Test.T + Wait; Test.Mark = Test.T; Test.Cand = 0; };
 if (Test.T < Test.Since) return;
 if (Test.T > 360) { Fail(FString::Printf(TEXT("timeout; SC08 step %d body %s at %s, player %s"), int32(A.Phase()), FS47SC08::BodyName(A.Body()), *A.BodyAt().ToString(), *FVector2D(M->Player->Pos.X, M->Player->Pos.Z).ToString())); return; }
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 const auto Glide = [this, Dt](double Speed)
 {
  if (Test.Route.IsEmpty()) return true;
  const FVector2D P(M->Player->Pos.X, M->Player->Pos.Z), D = Test.Route[0] - P;
  const double Step = Speed * Dt;
  const FVector2D N = D.Size() <= Step ? Test.Route[0] : P + D.GetSafeNormal() * Step;
  M->Player->Place(N.X, N.Y, FMath::Atan2(-D.X, -D.Y));
  if (D.Size() <= Step) Test.Route.RemoveAt(0);
  return Test.Route.IsEmpty();
 };
 switch (Test.Stage)
 {
 case 0:
 {
  if (Test.T < 3) return;
  US47Chapter2* C2 = Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2")));
  if (!Check(bEnabled && C2 && C2->OnEnd && M->World->Area == TEXT("saro"), TEXT("chapter two jump at SARO with the spine and chapter two's end"))) return;
  C2->OnEnd();
  if (!Check(Gate(SC08) == 1 && A.Phase() == FS47SC08::EStep::Reel && Objective().Contains(TEXT("1420 AM")) && !(Posts.IsValidIndex(SC08) && Posts[SC08].IsValid() && Posts[SC08]->IsVisible()) && !M->Story->Begun.Contains(TEXT("ch3")),
   TEXT("chapter two's end holds for the built SC08: Earl's lobby first, no placeholder post, chapter three waits"))) return;
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract API FIXTURE: Earl's lobby used through Spine::Use (the drive to Caliche is not part of this contract)"));
  Use(TEXT("amDoor")); if (M->UI) M->UI->Close(true);
  if (!Check(A.Facts().bReel && M->Story->Clock() >= 4 * 3600 && M->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("sc08_reel"); }) && Objective().Contains(TEXT("SERVICE SHEET")),
   TEXT("Earl's reel copy and note: 04:00, the paper filed, the sheet next"))) return;
  Next(1); return;
 }
 case 1:
 {
  // the sheet by an actual ray from a free stand point with a view of it
  const FVector I = [this] { const auto* It = M->Web->Item(TEXT("serviceRecord")); const FBox B = It ? M->Web->ObjAt(It->Node).WorldBox() : FBox(ForceInit); return B.IsValid ? B.GetCenter() : S47Three::ToThree(M->Web->PropLocation(TEXT("serviceRecord"))); }();
  int32 Seen = 0; bool bFound = false; FVector2D S;
  for (const double R : {1.1, 1.5, 1.9}) for (int32 K = 0; K < 16 && !bFound; ++K)
  {
   const FVector2D C(I.X + R * FMath::Cos(K * PI / 8), I.Z + R * FMath::Sin(K * PI / 8));
   if (!M->Player->Walkable(C.X, C.Y)) continue;
   FHitResult Hit; const FVector From = S47Three::ToUE(FVector(C.X, 1.62, C.Y)), To = S47Three::ToUE(I);
   if (M->Web->GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility) && Hit.Distance < FVector::Dist(From, To) - 35) continue;
   if (Seen++ == Test.Cand) { S = C; bFound = true; }
  }
  if (!bFound) { Fail(TEXT("no stand point sees the service sheet")); return; }
  M->Player->Place(S.X, S.Y, FMath::Atan2(-(I.X - S.X), -(I.Z - S.Y))); M->Player->Pitch = 0;
  Test.Stage = 2; Test.Since = Test.T + .4; Test.Route.Reset(); Test.Route.Add(FVector2D(I.X, I.Z)); return;
 }
 case 2:
 {
  const FVector I(Test.Route[0].X, 0, Test.Route[0].Y);
  const auto* It = M->Web->Item(TEXT("serviceRecord")); const FBox Bx = It ? M->Web->ObjAt(It->Node).WorldBox() : FBox(ForceInit);
  const FVector Eye = M->Player->EyeUE(), At = S47Three::ToUE(Bx.IsValid ? Bx.GetCenter() : I);
  const FString Picked = M->Inter->Pick(Eye, (At - Eye).GetSafeNormal());
  if (Picked != TEXT("serviceRecord")) { if (++Test.Cand > 40) { Fail(TEXT("ray to the service sheet picked ") + Picked); return; } Test.Stage = 1; return; }
  M->Player->OnUse(Picked); if (M->UI) M->UI->Close(true);
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract actual ray and production use: serviceRecord"));
  if (!Check(A.Facts().bSheet && A.Phase() == FS47SC08::EStep::Field && M->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("sc08_sheet"); }), TEXT("the array service sheet is taken; the field test is next"))) return;
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract FIXTURE: the array gate opened and the player placed inside it (the walk from the building is not part of this contract)"));
  M->Web->SetDoorOpen(TEXT("fieldGate"), true, true); M->Player->Place(-1, -24, PI); Test.Route.Reset();
  Next(3, .5); return;
 }
 case 3: Test.Route = {FVector2D(-1, -50), FVector2D(-1, -74), FS47SC08::PointA()}; Next(4, 0); return;
 case 4:
  if (!Glide(1.9)) return;
  Next(5, 1.0); return;
 case 5:
  if (!Check(A.Facts().bFirst && A.Facts().First == TEXT("A") && M->SaroNight && M->SaroNight->StoryFollowAt().IsSet(), TEXT("stake A: the dishes' follow is started"))) return;
  Test.Route = {FS47SC08::PointB()}; Next(6, 0); return;
 case 6:
  if (!Glide(1.9)) return;
  Next(7, 1.0); return;
 case 7:
  if (!Check(A.Facts().bSecond && A.Following() >= 1, FString::Printf(TEXT("stake B: %d dishes are on the player, none on the lamp"), A.Following()))) return;
  Test.Route = {FS47SC08::PointB() + FVector2D(-5, 0)}; Next(8, 0); return;
 case 8:
  if (!Glide(1.9)) return;
  Next(80, .5); return; // the walker takes its last step next tick
 case 80:
  if (!Check(A.Facts().bBack && A.Phase() == FS47SC08::EStep::Hunt, TEXT("a few steps back: noted, the meeting starts"))) return;
  Next(9, 0); return;
 case 9:
  if (A.Body() == FS47SC08::EBody::Away) { if (Test.T - Test.Mark > 15) Fail(TEXT("the body never came into the field")); return; }
  if (!Check(A.BodyShown() && A.BodyAt().X < -1, TEXT("the body comes in at the far end of row one, away from the player"))) return;
  Test.Route = bCaught ? TArray<FVector2D>{FVector2D(-1, -74), FVector2D(-12, -74), FVector2D(-24, -74)} : TArray<FVector2D>{FVector2D(-1, -74), FVector2D(-1, -50), FVector2D(-1, -24), FVector2D(-1, -16)};
  Next(bCaught ? 10 : 12, 0); return;
 case 10:
  if (A.Facts().Captures == 0) { Glide(1.9); if (Test.T - Test.Mark > 40) Fail(TEXT("not caught walking at it")); return; }
  if (Test.Hop == 0) { Test.Hop = 1; Test.Since = Test.T + .3; return; }
  Test.Hop = 0;
  if (!Check(A.Facts().Captures == 1 && A.Phase() == FS47SC08::EStep::Hunt && FVector2D::Distance(Pl, FS47SC08::PointB() + FVector2D(-5, 0)) < 1.0, TEXT("caught on row one: the meeting starts again where it began"))) return;
  Next(11, .2); return;
 case 11:
  if (A.Body() == FS47SC08::EBody::Away) { if (Test.T - Test.Mark > 15) Fail(TEXT("the body did not come back")); return; }
  Test.Route = {FVector2D(-1, -74), FVector2D(-1, -50), FVector2D(-1, -24), FVector2D(-1, -16)}; Next(12, 0); return;
 case 12:
  if (A.Facts().Captures > (bCaught ? 1 : 0)) { Fail(FString::Printf(TEXT("caught on the short way up the spine at %s"), *Pl.ToString())); return; }
  if (!Glide(3.0)) return;
  Next(13, 0); return;
 case 13:
  if (Gate(SC08) != 2) { if (Test.T - Test.Mark > 40) Fail(FString::Printf(TEXT("SC08 did not finish after the gate: body %s"), FS47SC08::BodyName(A.Body()))); return; }
  if (!Check(A.Facts().bEscaped && A.Facts().Captures == (bCaught ? 1 : 0) && M->Story->Clock() >= GateDone(SC08) && !(M->SaroNight && M->SaroNight->StoryFollowAt().IsSet()),
   TEXT("out through the array gate: the body stops at the fence, the dishes go home, 04:20"))) return;
  {
   FS47Spine Fresh(nullptr); Fresh.Begin(S47J::Read(S47J::Write(Save())), false);
   if (!Check(Fresh.Gate(SC08) == 2 && Fresh.Point().Phase() == FS47SC08::EStep::Done && Fresh.Point().Facts().bReel, TEXT("saved spine reloads with SC08 done and its facts"))) return;
  }
  Next(14, 2.5); return;
 case 14:
  if (!M->Story->Begun.Contains(TEXT("ch3"))) { if (Test.T - Test.Mark > 12) Fail(TEXT("chapter three did not begin after SC08")); return; }
  if (!Check(M->Story->Clock() >= S47Timeline::Chapter3(true) && Current() == 8, TEXT("chapter three begins as SC09 at 04:25"))) return;
  Test.bDone = true;
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract %s PASS COMPLETE: %d checks"), *Test.Mode, Test.Checks);
  FPlatformMisc::RequestExit(false);
  return;
 default: return;
 }
}

// The SC11 contract from the chapter four jump, chapter four's end called as the chapter calls it.
// The drive from the motel is a labelled area fixture; the originals, the telex and the truck are
// actual rays; doors on the long way are opened by fixture; the player moves in walker steps.
void FS47Spine::SC11Test(double Dt, TFunctionRef<void(const FString&)> Fail, TFunctionRef<bool(bool, const FString&)> Check)
{
 const FS47SC11& A = Act11;
 const bool bCaught = Test.Mode == TEXT("sc11caught");
 const auto Next = [this](int32 S, double Wait = .3) { Test.Stage = S; Test.Since = Test.T + Wait; Test.Mark = Test.T; Test.Cand = 0; };
 if (Test.T < Test.Since) return;
 if (Test.T > 360) { Fail(FString::Printf(TEXT("timeout; SC11 step %d body %s at %s, player %s"), int32(A.Phase()), FS47SC11::BodyName(A.Body()), *A.BodyAt().ToString(), *FVector2D(M->Player->Pos.X, M->Player->Pos.Z).ToString())); return; }
 const FVector2D Pl(M->Player->Pos.X, M->Player->Pos.Z);
 const auto Glide = [this, Dt](double Speed)
 {
  if (Test.Route.IsEmpty()) return true;
  const FVector2D P(M->Player->Pos.X, M->Player->Pos.Z), D = Test.Route[0] - P;
  const double Step = Speed * Dt;
  const FVector2D N = D.Size() <= Step ? Test.Route[0] : P + D.GetSafeNormal() * Step;
  M->Player->Place(N.X, N.Y, FMath::Atan2(-D.X, -D.Y));
  if (D.Size() <= Step) Test.Route.RemoveAt(0);
  return Test.Route.IsEmpty();
 };
 const auto ItemAt = [this](const TCHAR* Id)
 {
  if (const auto* It = M->Web->Item(Id)) { const FBox Bx = M->Web->ObjAt(It->Node).WorldBox(); if (Bx.IsValid) return Bx.GetCenter(); }
  return S47Three::ToThree(M->Web->PropLocation(Id));
 };
 // Stand (even stage) then an actual ray and the production use (odd stage), trying stand points.
 const auto StandUse = [&](int32 Stage, const TCHAR* Id, TFunctionRef<void()> Done)
 {
  const FVector I = ItemAt(Id);
  if (Test.Stage == Stage)
  {
   int32 Seen = 0; bool bFound = false; FVector2D S;
   for (const double R : {1.1, 1.5, 1.9, 2.4}) for (int32 K = 0; K < 16 && !bFound; ++K)
   {
    const FVector2D C(I.X + R * FMath::Cos(K * PI / 8), I.Z + R * FMath::Sin(K * PI / 8));
    if (!M->Player->Walkable(C.X, C.Y)) continue;
    FHitResult Hit; const FVector From = S47Three::ToUE(FVector(C.X, 1.62, C.Y)), To = S47Three::ToUE(I);
    if (M->Web->GetWorld()->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility) && Hit.Distance < FVector::Dist(From, To) - 35 && M->Web->UsableAt(Hit.GetComponent()) != Id) continue;
    if (Seen++ == Test.Cand) { S = C; bFound = true; }
   }
   if (!bFound) { Fail(FString::Printf(TEXT("no stand point sees %s"), Id)); return; }
   M->Player->Place(S.X, S.Y, FMath::Atan2(-(I.X - S.X), -(I.Z - S.Y))); M->Player->Pitch = 0;
   Test.Stage = Stage + 1; Test.Since = Test.T + .4; return;
  }
  const FVector Eye = M->Player->EyeUE(), At = S47Three::ToUE(I);
  const FString Picked = M->Inter->Pick(Eye, (At - Eye).GetSafeNormal(), 3.2);
  if (Picked != Id) { if (++Test.Cand > 50) { Fail(FString::Printf(TEXT("ray to %s picked %s"), Id, *Picked)); return; } Test.Stage = Stage; return; }
  M->Player->OnUse(Picked); if (M->UI) M->UI->Close(true);
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract actual ray and production use: %s"), Id);
  Done();
 };
 switch (Test.Stage)
 {
 case 0:
 {
  if (Test.T < 3) return;
  US47Chapter4* C4 = Cast<US47Chapter4>(M->Story->Chapter(TEXT("ch4")));
  if (!Check(bEnabled && C4 && C4->OnEnd, TEXT("chapter four jump with the spine and chapter four's end"))) return;
  C4->OnEnd();
  if (!Check(Gate(SC11) == 1 && A.Phase() == FS47SC11::EStep::Drive && !M->Story->Begun.Contains(TEXT("ch5")), TEXT("chapter four's end holds for the built SC11; chapter five waits"))) return;
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract FIXTURE: the drive from the motel to SARO is an area fixture"));
  M->World->PlaceTruck(TEXT("saro")); M->World->Enter(TEXT("saro")); M->Player->Place(0, .3, 0);
  Next(1, 1.0); return;
 }
 case 1:
  if (!Check(A.Phase() == FS47SC11::EStep::Collect && M->Story->Clock() >= 5 * 3600, TEXT("at SARO: 05:00, the originals next"))) return;
  Next(2); return;
 case 2: case 3: StandUse(2, TEXT("printer"), [&] { if (Check(A.Facts().bOriginal, TEXT("the receiver original is torn off"))) Next(4); }); return;
 case 4: case 5: StandUse(4, TEXT("logbook"), [&] { if (Check(A.Facts().bSheet && A.Phase() == FS47SC11::EStep::Telex && M->Story->Clock() >= 5 * 3600 + 4 * 60, TEXT("the event sheet is taken; 05:04, the telex prints"))) Next(6, 0); }); return;
 case 6:
  if (A.Label(TEXT("telex")).IsEmpty()) { if (Test.T - Test.Mark > 60) Fail(TEXT("the telex never finished")); return; }
  Next(7, 0); return;
 case 7: case 8: StandUse(7, TEXT("telex"), [&] { if (Check(A.Facts().bRemote && A.Phase() == FS47SC11::EStep::Siege && M->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("sc11_remote"); }), TEXT("the remote sheets are torn off; the siege starts"))) Next(bCaught ? 9 : 12, 0); }); return;
 // ---- sc11caught: towards the service door, to meet it ----
 case 9: Test.Route = {FVector2D(0, .3), FVector2D(3.4, .3), FVector2D(4.6, 1.6)}; Next(10, 0); return;
 case 10:
  if (A.Facts().Captures == 0) { Glide(1.9); if (Test.T - Test.Mark > 60) Fail(FString::Printf(TEXT("not met at the service door: body %s at %s"), FS47SC11::BodyName(A.Body()), *A.BodyAt().ToString())); return; }
  if (Test.Hop == 0) { Test.Hop = 1; Test.Since = Test.T + .3; return; }
  Test.Hop = 0;
  if (!Check(A.Facts().Captures == 1 && A.Phase() == FS47SC11::EStep::Siege && FVector2D::Distance(Pl, FVector2D(-4.2, 3.2)) < .8, TEXT("taken: the siege starts again at the telex, the originals kept"))) return;
  Next(12, .2); return;
 // ---- both: the long way, out by the annex and the fire exit, round the north side, down to the truck ----
 case 12:
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract FIXTURE: the corridor door and the fire exit opened by API on the long way"));
  M->Web->SetDoorOpen(TEXT("doorSouth"), true, true); M->Web->SetDoorOpen(TEXT("exitDoor"), true, true);
  Test.Route = {FVector2D(-2.4, 2.9), FVector2D(-2.4, 5.7), FVector2D(-4.75, 5.7), FVector2D(-7.0, 5.7), FVector2D(-11.1, 5.7), FVector2D(-11.1, 0), FVector2D(-9, -8)};
  Next(13, 0); return;
 case 13:
  if (A.Facts().Captures > (bCaught ? 1 : 0)) { Fail(FString::Printf(TEXT("caught on the long way at %s"), *Pl.ToString())); return; }
  if (!Glide(3.0)) return;
  Next(18, 0); return;
 case 18:
  // At the north-west corner, out of its sight and hearing, until it has gone into the building (as a player would wait).
  if (A.Facts().Captures > (bCaught ? 1 : 0)) { Fail(FString::Printf(TEXT("caught waiting at the corner at %s"), *Pl.ToString())); return; }
  if (A.Body() == FS47SC11::EBody::Away || FS47SC11::Room(A.BodyAt()) == 3) { if (Test.T - Test.Mark > 90) Fail(FString::Printf(TEXT("it never went inside: %s at %s"), FS47SC11::BodyName(A.Body()), *A.BodyAt().ToString())); return; }
  Check(true, TEXT("waited at the north-west corner until the body was inside"));
  Test.Route = {FVector2D(5, -8), FVector2D(5, -12), FVector2D(9.5, -12), FVector2D(9.5, 0), FVector2D(10.2, 1.6), FVector2D(10.2, 4.5), FVector2D(7.2, 4.8), FVector2D(7.2, 6.6)};
  Next(19, 0); return;
 case 19:
  if (A.Facts().Captures > (bCaught ? 1 : 0)) { Fail(FString::Printf(TEXT("caught on the last stretch at %s"), *Pl.ToString())); return; }
  if (!Glide(3.0)) return;
  Next(14, .3); return;
 case 14: case 15: StandUse(14, TEXT("truck"), [&] { Next(16, 1.5); }); return;
 case 16:
  if (Gate(SC11) != 2) { if (Test.T - Test.Mark > 10) Fail(FString::Printf(TEXT("SC11 did not finish at the truck: step %d"), int32(A.Phase()))); return; }
  if (!Check(A.Facts().bEscaped && A.Facts().Captures == (bCaught ? 1 : 0) && M->Story->Clock() >= GateDone(SC11), TEXT("away in the truck with the originals: 05:10"))) return;
  {
   FS47Spine Fresh(nullptr); Fresh.Begin(S47J::Read(S47J::Write(Save())), false);
   if (!Check(Fresh.Gate(SC11) == 2 && Fresh.Searched().Phase() == FS47SC11::EStep::Done && Fresh.Searched().Facts().bRemote, TEXT("saved spine reloads with SC11 done and its facts"))) return;
  }
  Next(17, 2.5); return;
 case 17:
  if (!M->Story->Begun.Contains(TEXT("ch5"))) { if (Test.T - Test.Mark > 12) Fail(TEXT("chapter five did not begin after SC11")); return; }
  if (!Check(M->Story->Clock() >= S47Timeline::Chapter5(true) && Current() == 11, TEXT("chapter five begins as SC12 at 05:12"))) return;
  Test.bDone = true;
  UE_LOG(LogTemp, Display, TEXT("S47: spine contract %s PASS COMPLETE: %d checks"), *Test.Mode, Test.Checks);
  FPlatformMisc::RequestExit(false);
  return;
 default: return;
 }
}
