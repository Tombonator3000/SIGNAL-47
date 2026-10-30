// Opt-in SC12 fixture and production-action contract. No SC11 escape, human input or full-night claim.
#include "S47Chapter5.h"
#include "S47Chapter6.h"
#include "S47Diner.h"
#include "S47Drawings.h"
#include "S47Flow.h"
#include "S47Horror.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47SC11.h"
#include "S47Spine.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47Timeline.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

class FS47SC12Contract : public TSharedFromThis<FS47SC12Contract>
{
 using FJ = S47J::FObj;
 TWeakObjectPtr<US47Main> Main;
 bool Done = false, Armed = false;
 int32 Stage = 0, Checks = 0, Restores = 0, RayUses = 0, Waypoint = 0;
 double Elapsed = 0, Wait = 0, WalkMetres = 0, Stalled = 0;
 FString Root;
 FVector LastFoot = FVector::ZeroVector;
 TArray<FVector> Path;
 TArray<TSharedPtr<FJsonValue>> Evidence, Rays;
 FS47Doc Third;
 TArray<FS47Doc> AcquiredSources;
 US47Main* M() const { return Main.Get(); }
 US47Chapter5* C() const { return M() && M()->Story ? Cast<US47Chapter5>(M()->Story->Chapter(TEXT("ch5"))) : nullptr; }
 bool Write(const FString& Name, const FJ& O) const { return FFileHelper::SaveStringToFile(S47J::Write(O, true), *(Root / Name)); }
 static bool Same(const FJ& A, const FJ& B) { return A && B && FJsonValue::CompareEqual(FJsonValueObject(A), FJsonValueObject(B)); }
 static FJ DocFields(const FS47Doc& D)
 {
  auto O = S47J::New(); S47J::Set(O,TEXT("id"),D.Id); S47J::Set(O,TEXT("title"),D.Title); S47J::Set(O,TEXT("kind"),D.Kind);
  S47J::Set(O,TEXT("stamp"),D.Stamp); S47J::Set(O,TEXT("page"),D.Page); S47J::Set(O,TEXT("transcript"),D.Transcript);
  S47J::Set(O,TEXT("image"),D.Image); S47J::Set(O,TEXT("paper"),D.Paper); S47J::Set(O,TEXT("paperRatio"),D.PaperRatio);
  TArray<TSharedPtr<FJsonValue>> Sheets;
  for (const auto& Sheet : D.Sheets)
  {
   auto Row = S47J::New(); S47J::Set(Row,TEXT("page"),Sheet.Page); S47J::Set(Row,TEXT("paper"),Sheet.Paper);
   S47J::Set(Row,TEXT("strip"),Sheet.Strip); S47J::Set(Row,TEXT("term"),Sheet.Term); Sheets.Add(MakeShared<FJsonValueObject>(Row));
  }
  O->SetArrayField(TEXT("sheets"),Sheets); return O;
 }
 TArray<TSharedPtr<FJsonValue>> CurrentSources() const
 {
  TArray<TSharedPtr<FJsonValue>> Rows;
  if (M() && M()->Story) for (const auto& Original : AcquiredSources)
   if (const auto* Actual = M()->Story->Docs().FindByPredicate([&](const FS47Doc& D) { return D.Id == Original.Id; })) Rows.Add(MakeShared<FJsonValueObject>(DocFields(*Actual)));
  return Rows;
 }
 void Result(bool Pass, const FString& Why)
 {
  const auto KeepAlive = AsShared(); if (Done) return; Done = true;
  if (M() && M()->Player) M()->Player->TestMove.Reset();
  auto O = S47J::New(); S47J::Set(O, TEXT("status"), Pass ? TEXT("PASS") : TEXT("FAIL")); S47J::Set(O, TEXT("reason"), Why);
  S47J::Set(O, TEXT("stage"), Stage); S47J::Set(O, TEXT("checks"), Checks); S47J::Set(O, TEXT("diskFlowRestores"), Restores);
  S47J::Set(O, TEXT("ordinaryRayUses"), RayUses); S47J::Set(O, TEXT("walkMetres"), WalkMetres); S47J::Set(O, TEXT("seconds"), Elapsed);
  S47J::Set(O, TEXT("lastReadback"), C() ? C()->SC12Readback() : FJ()); O->SetArrayField(TEXT("checksExecuted"), Evidence); O->SetArrayField(TEXT("rays"), Rays);
  O->SetArrayField(TEXT("actualHostSourcePapers"),CurrentSources());
  S47J::Set(O, TEXT("scope"), TEXT("Explicit isolated chapter5/SC11-secured-source fixture using the actual FS47SC11 document factories, not a played SC11 escape. One declared diner/foot start. Ordinary Walker between booth, phone and dated patrol sheet; actual camera/focus/ray and Player.OnUse. Conversation choices and tracing motion use the exact production callbacks, not human Slate input. Incomplete sources, early acknowledgements, interrupted calls, map errors and full disk/title/Continue at each stable boundary. All current Host.Docs source fields are independently compared with acquired fixture pages before and after Continue. Final service-truck choice is a labelled production API action, not a measured road drive. No screenshots, listening, full-night, packaging or installation acceptance."));
  const bool Written = Root.IsEmpty() || Write(TEXT("result.json"), O);
  if (Pass && Written) { UE_LOG(LogTemp, Display, TEXT("S47: spine contract sc12 PASS COMPLETE: %d checks, %d restores, %d rays, %.3fm"), Checks, Restores, RayUses, WalkMetres); FPlatformMisc::RequestExitWithStatus(false, 0); }
  else { UE_LOG(LogTemp, Error, TEXT("S47: spine contract sc12 FAIL stage %d: %s"), Stage, *Why); FPlatformMisc::RequestExitWithStatus(false, 1); }
 }
 bool Check(bool Good, const FString& Why)
 {
  ++Checks; auto O = S47J::New(); S47J::Set(O, TEXT("pass"), Good); S47J::Set(O, TEXT("detail"), Why); S47J::Set(O, TEXT("stage"), Stage); Evidence.Add(MakeShared<FJsonValueObject>(O));
  if (!Good) { Result(false, Why); return false; } return true;
 }
 void Next(int32 S, double Delay = .25) { Stage = S; Wait = Elapsed + Delay; }
 bool Snapshot(const FString& Name)
 {
  const auto R = C()->SC12Readback();
  S47J::Set(R, TEXT("name"), Name); S47J::Set(R, TEXT("area"), M()->World->Area);
  S47J::Set(R, TEXT("case"), M()->Flow->ComposeCase());
  R->SetArrayField(TEXT("actualHostSourcePapers"),CurrentSources());
  if (!Write(Name + TEXT(".json"), R)) { Result(false, TEXT("readback write failed: ") + Name); return false; } return true;
 }
 bool Choice(const FString& Id)
 {
  const auto View = C()->View();
  if (!Check(View.Choices.ContainsByPredicate([&](const S47Panels::FTalkChoice& X) { return X.Id == Id; }), TEXT("production panel offers choice: ") + Id)) return false;
  return Check(C()->SC12Choice(Id), TEXT("same guarded production choice callback: ") + Id);
 }
 void Close() { if (M()->UI) M()->UI->Close(true); }
 bool Restore(const FString& Name)
 {
  // Called only after closing the modal; this is the complete production case, never a hand-made ch5 restore.
  if (!Check(!M()->UI->Modal() && M()->Flow->GameSaveBlock().IsEmpty(), TEXT("normal stable save boundary: ") + Name)) return false;
  const auto Before = M()->Flow->GameState(), State5 = S47J::Obj(S47J::Obj(Before, TEXT("case")), TEXT("ch5"));
  const double Clock = M()->Story->Clock(); const FVector Foot = M()->Player->Pos;
  TArray<FS47Doc> BeforePapers;
  for (const auto& Original : AcquiredSources)
  {
   const auto* Actual = M()->Story->Docs().FindByPredicate([&](const FS47Doc& D) { return D.Id == Original.Id; });
   if (!Actual) { if (!Check(Stage == 1 && Original.Id == Third.Id, TEXT("only the declared missing-third-source cohort may omit a paper: ") + Original.Id)) return false; continue; }
   if (!Check(Same(DocFields(*Actual),DocFields(Original)), TEXT("actual acquired Host.Docs payload before disk: ") + Original.Id + TEXT(" / ") + Name)) return false;
   BeforePapers.Add(*Actual);
  }
  if (!Check(Before && State5 && S47J::Str(Before, TEXT("checkpoint")) == TEXT("chapter5"), TEXT("actual complete chapter5 snapshot before disk: ") + Name)) return false;
  FS47SaveStore Writer(Root / Name / TEXT("case1")); Writer.Init(); FS47SaveCard Card; Card.Chapter = M()->Story->ChapterTitle(); Card.Place = M()->Flow->PlaceName(); Card.Clock = M()->Story->ClockText(false); Card.Playtime = M()->Playtime;
  FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, Before, Card, Error);
  if (!Check(Meta.IsSet(), TEXT("isolated complete disk save: ") + Name + TEXT(" ") + Error)) return false;
  FS47SaveStore Reader(Writer.Dir); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
  if (!Check(Same(Before, Disk), TEXT("fresh reader equals complete written state: ") + Name)) return false;
  if (!Write(Name + TEXT(".disk.json"), Disk)) { Result(false, TEXT("disk evidence failed")); return false; }
  M()->Flow->Saves = FS47SaveStore(Writer.Dir); M()->Flow->Saves.Init(); M()->Flow->ToTitle();
  if (!Check(M()->Mode == US47Main::EMode::Title, TEXT("ordinary title reached: ") + Name)) return false;
  M()->Flow->OpenSave(*Meta); ++Restores;
  if (!Check(C() && Same(C()->S.Save(), State5) && FMath::Abs(M()->Story->Clock() - Clock) < .001 && M()->World->Area == TEXT("diner") && M()->Player->Pos.Equals(Foot, .015) && C()->PendingReadback == 0,
   TEXT("full Flow Continue keeps complete ch5/clock/area/pose; no live pending acknowledgement survives: ") + Name)) return false;
  for (const auto& Original : AcquiredSources)
  {
   const auto* Had = BeforePapers.FindByPredicate([&](const FS47Doc& D) { return D.Id == Original.Id; });
   const auto* Actual = M()->Story->Docs().FindByPredicate([&](const FS47Doc& D) { return D.Id == Original.Id; });
   if (!Check((Had && Actual && Same(DocFields(*Actual),DocFields(*Had))) || (!Had && !Actual),
    TEXT("full Flow keeps actual Host.Docs fields, not replacement templates: ") + Original.Id + TEXT(" / ") + Name)) return false;
  }
  return Snapshot(Name + TEXT("-continued"));
 }
 void WalkTo(const TArray<FVector>& Local)
 {
  Path.Reset(); for (const FVector& P : Local) Path.Add(C()->Diner()->Group.LocalToWorld(P));
  Waypoint = 0; LastFoot = M()->Player->Pos; Stalled = Elapsed;
 }
 bool Walk(double Dt)
 {
  auto* Player = M()->Player.Get(); const double Moved = FVector2D(Player->Pos.X - LastFoot.X, Player->Pos.Z - LastFoot.Z).Size();
  if (Moved > FMath::Max(.5, Dt * 4 + .1)) { Result(false, TEXT("Walker discontinuity; no placement recovery")); return false; }
  WalkMetres += Moved; LastFoot = Player->Pos; if (Moved > .002) Stalled = Elapsed;
  if (Elapsed - Stalled > 12) { Result(false, TEXT("ordinary diner Walker stalled")); return false; }
  if (Waypoint >= Path.Num()) { Player->TestMove.Reset(); return true; }
  const FVector D = Path[Waypoint] - Player->Pos; const double Distance = FVector2D(D.X, D.Z).Size();
  if (Distance < .085) { ++Waypoint; Player->TestMove.Reset(); return false; }
  Player->Yaw = FMath::Atan2(-D.X, -D.Z); Player->Pitch = -.08; Player->bTestRun = false; Player->TestMove = FVector2D(0, -FMath::Min(1., Distance / .5)); return false;
 }
 FVector Target(const FString& Id) const
 {
  if (Id == TEXT("sc12:patrol")) return C()->SC12Object(TEXT("patrol")).WorldPosition();
  return C()->Diner()->Proxy(Id.RightChop(6)).WorldPosition();
 }
 void Look(const FString& Id)
 {
  const FVector D = Target(Id) - S47Three::ToThree(M()->Player->EyeUE());
  M()->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M()->Player->Pitch = FMath::Atan2(D.Y, FVector2D(D.X, D.Z).Size());
 }
 bool Use(const FString& Id)
 {
  const FVector Eye = M()->Player->EyeUE(), Dir = M()->Player->ForwardUE(); const FString Picked = M()->Inter->Update(Eye, Dir);
  auto R = S47J::New(); S47J::Set(R, TEXT("expected"), Id); S47J::Set(R, TEXT("picked"), Picked); S47J::Set(R, TEXT("focus"), M()->Player->FocusedId());
  S47J::Set(R, TEXT("eyeUE"), TArray<double>{Eye.X, Eye.Y, Eye.Z}); S47J::Set(R, TEXT("clock"), M()->Story->Clock()); Rays.Add(MakeShared<FJsonValueObject>(R));
  if (!Check(Picked == Id && M()->Player->FocusedId() == Id && bool(M()->Player->OnUse) && !M()->UI->Modal(), TEXT("actual camera/focus/ray then ordinary Player.OnUse: ") + Id)) return false;
  M()->Player->OnUse(Picked); ++RayUses; return true;
 }
public:
 explicit FS47SC12Contract(US47Main* InM) : Main(InM) {}
 void Tick(double Dt)
 {
  if (Done) return;
  if (!M() || !M()->Story || !M()->Flow || !M()->World || !M()->Player || !M()->Web || !M()->UI || !M()->Inter || !M()->Horror || !M()->Spine || !C()) { Result(false, TEXT("missing actual SC12 runtime dependency")); return; }
  Elapsed += Dt;
  if (Elapsed > 300) { Result(false, TEXT("300 second SC12 contract timeout")); return; }
  if (M()->Flow->bStarting || M()->World->bBusy || Elapsed < Wait) return;
  if (Stage == 0)
  {
   if (Elapsed < 3 || !C()->Diner()) return;
   FString UserDir; const bool HasUserDir = FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir);
   const bool NoParentTraversal = !UserDir.Contains(TEXT("..")); UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::NormalizeDirectoryName(UserDir);
   if (!Check(HasUserDir && NoParentTraversal && UserDir.StartsWith(TEXT("/tmp/"))
    && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir)
    && FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M()->Flow->Saves.Dir),UserDir)
    && M()->Flow->Saves.List().IsEmpty() && M()->Flow->bDevRun && M()->Flow->CaseId == 0 && M()->Story->bEveningTimeline && M()->Story->ChapterKey == TEXT("ch5"),
    TEXT("fresh isolated /tmp developer chapter5 fixture; actual Saved and SaveStore lie inside that profile"))) return;
   Root = FPaths::ProjectSavedDir() / TEXT("SpineSC12Contract") / FGuid::NewGuid().ToString(EGuidFormats::Digits);
   if (!IFileManager::Get().MakeDirectory(*Root, true)) { Result(false, TEXT("fresh evidence directory unavailable")); return; }
   Armed = true;
   // Prior scenarios and their secured papers are explicitly fixture data here, never a new escape assertion.
   M()->Spine->Begin(S47J::Read(TEXT("{\"v\":1,\"sc04\":2,\"sc08\":2,\"sc11\":2,\"sc12Route\":1}")), false);
   C()->Begin(nullptr); M()->World->PlaceTruck(TEXT("diner")); M()->World->GoDiner(TEXT("stool"));
   Next(100,.7); return;
  }
  if (Stage == 100)
  {
   if (!Check(M()->World->Area == TEXT("diner"), TEXT("declared diner transition actually completed before any foot placement"))) return;
   C()->ArrivedDiner();
   // These are the same factories the actual SC11 uses. The complete local source is an explicit
   // prerequisite fixture; calling this factory is never credited as playing its escape.
   const auto Local = FS47SC11::ReceiverDoc(true,TEXT("05:10:00 / SC12 PREREQUISITE FIXTURE"));
   const auto Hondo = FS47SC11::HondoDoc(); Third = FS47SC11::Site11Doc(); AcquiredSources = {Local,Hondo,Third};
   if (!Check(Local.Page.Contains(TEXT("ctx_hr_h07_escape_complete / WORLD ESCAPE RECORDED"))
    && !Hondo.Page.IsEmpty() && !Third.Page.IsEmpty(), TEXT("actual SC11 document factories supply the explicit secured-source fixture; no new escape credit"))) return;
   M()->Story->AddDoc(Local); M()->Story->AddDoc(Hondo);
   M()->Story->DocsMutable().RemoveAll([](const FS47Doc& D) { return D.Id == TEXT("HR_SITE11_REMOTE"); });
   const FVector At = C()->Diner()->Group.LocalToWorld(FVector(-.8, 0, 3.05));
   if (!Check(M()->Player->Walkable(At.X, At.Z), TEXT("one declared diner-aisle foot start is actually walkable"))) return;
   M()->Player->Place(At.X, At.Z, 0); Look(TEXT("diner:booth"));
   if (!Check(C()->RevisedDiner() && M()->Story->Clock() >= S47Timeline::Chapter5(true) && M()->Story->Clock() < 5 * 3600 + 15 * 60 && !C()->S.SC12.bDeputyHere && !C()->ReadyForRoad(), TEXT("new SC12 starts05:12 before sheriff05:15 and cannot depart"))) return;
   Next(1, .5); return;
  }
  if (!Armed) return;
  if (Stage == 1)
  {
   Look(TEXT("diner:booth")); if (!Use(TEXT("diner:booth"))) return;
   C()->Choose(TEXT("compare")); C()->Who = TEXT("ward"); C()->Choose(TEXT("ack-warning")); C()->Choose(TEXT("ack-route")); C()->Finish();
   if (!Check(!C()->SourcesVerified() && !C()->S.SC12.bWarningSent && !C()->S.SC12.bCourier && !M()->Story->Begun.Contains(TEXT("ch6")), TEXT("one missing source and out-of-order compare/ack/departure produce no conclusion or progress"))) return;
   Close(); if (!Restore(TEXT("incomplete-sources"))) return; Next(2); return;
  }
  if (Stage == 2)
  {
   M()->Story->AddDoc(Third);
   for (int32 I = 0; I < 3; ++I) { C()->OpenSources(); if (!Choice(FString::Printf(TEXT("read%d"), I))) return; Close(); }
   C()->OpenSources(); if (!Choice(TEXT("origins")) || !Choice(TEXT("clocks")) || !Choice(TEXT("feeds"))) return;
   if (!Check(C()->S.SC12.ReadMask == 7 && C()->S.SC12.CheckMask == 7 && !C()->S.SC12.bCompared && !C()->S.SC12.bWarningSent, TEXT("reading and all three explicit checks still require the comparison action"))) return;
   Close(); if (!Restore(TEXT("source-checks-before-conclusion"))) return; Next(3); return;
  }
  if (Stage == 3)
  {
   C()->OpenSources(); if (!Choice(TEXT("compare"))) return; Close();
   if (!Check(C()->SourcesVerified() && !C()->S.SC12.bWarningSent && M()->Story->Clock() >= 5 * 3600 + 15 * 60 && M()->Story->Clock() < S47Timeline::Departure, TEXT("actual completed comparison advances to05:15 but sends no warning"))) return;
   if (!Restore(TEXT("sources-compared"))) return; Next(4); return;
  }
  if (Stage == 4)
  {
   const auto R = C()->SC12Readback();
   if (!Check(C()->S.SC12.bDeputyHere && !S47J::Bool(R, TEXT("deputyAtOldOffice")) && !S47J::Bool(R, TEXT("oldPatrolCarOut")) && !S47J::Bool(R, TEXT("oldSC03DeputyVisible")) && C()->SC12Object(TEXT("deputyBody")).Shown() && C()->SC12Object(TEXT("patrol")).Shown(), TEXT("05:15 actual diner deputy and patrol paper; retained SC03 body and old office/car are not simultaneously visible"))) return;
   WalkTo({FVector(-1.4,0,3.05), FVector(-1.4,0,7.1), FVector(-.8,0,7.25)}); Next(5,0); return;
  }
  if (Stage == 5) { if (!Walk(Dt)) return; Look(TEXT("diner:payphone")); Next(6,.5); return; }
  if (Stage == 6)
  {
   Look(TEXT("diner:payphone")); if (!Use(TEXT("diner:payphone")) || !Choice(TEXT("send-warning"))) return;
   if (!Check(C()->PendingReadback == 1 && !C()->S.SC12.bWarningSent && !M()->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("HR_WARNING_RECEIPT"); })
    && C()->Lines.ContainsByPredicate([](const S47Panels::FTalkLine& L) { return L.Text.Contains(TEXT("Hondo local clock / 04:56:08")) && L.Text.Contains(TEXT("04:56:09")); }),
    TEXT("three IDs, Hondo04:56:08 and separate SARO04:56:09 displayed in readback, not yet acknowledged"))) return;
   Close(); if (!Check(C()->PendingReadback == 0 && !C()->S.SC12.bWarningSent, TEXT("hanging up before confirmation gives no receipt")) || !Restore(TEXT("interrupted-warning"))) return;
   Next(7,.5); return;
  }
  if (Stage == 7)
  {
   Look(TEXT("diner:payphone")); if (!Use(TEXT("diner:payphone")) || !Choice(TEXT("send-warning")) || !Choice(TEXT("ack-warning"))) return;
   if (!Check(C()->S.SC12.bWarningSent && !C()->S.SC12.WarningStamp.IsEmpty() && !C()->S.SC12.bCourier && C()->S.Stage == TEXT("diner"), TEXT("explicit readback confirmation writes dated warning; no courier or road credit"))) return;
   Close(); if (!Restore(TEXT("warning-receipted"))) return;
   WalkTo({FVector(-2,0,7.25), FVector(-2,0,1.25), FVector(-2.8,0,1.25)}); Next(8,0); return;
  }
  if (Stage == 8) { if (!Walk(Dt)) return; Look(TEXT("sc12:patrol")); Next(9,.5); return; }
  if (Stage == 9)
  {
   Look(TEXT("sc12:patrol")); if (!Use(TEXT("sc12:patrol"))) return;
   if (!Check(C()->S.SC12.bPatrolRead && C()->PatrolDoc().Page.Contains(TEXT("04:15 MST")) && C()->PatrolDoc().Page.Contains(TEXT("05:15")) && !C()->S.SC12.bCourier, TEXT("actual dated counter paper read, one04:15 drive and05:15 arrival, no guaranteed safe road or courier credit"))) return;
   Close(); if (!Restore(TEXT("patrol-read"))) return;
   WalkTo({FVector(-2,0,1.25), FVector(-2,0,3.05), FVector(-.8,0,3.05)}); Next(10,0); return;
  }
  if (Stage == 10) { if (!Walk(Dt)) return; Look(TEXT("diner:booth")); Next(11,.5); return; }
  if (Stage == 11)
  {
   Look(TEXT("diner:booth")); if (!Use(TEXT("diner:booth")) || !Choice(TEXT("map"))) return;
   C()->MoveTracing(300,690); const auto Early = C()->Answer(TEXT("bm8"));
   if (!Check(!Early.bOk && !C()->S.bP13 && C()->S.Wrong13 == 0, TEXT("correct guessed crossing before actual tracing alignment records nothing"))) return;
   const auto Station = S47Drawings::SERVICE_MAP().Station; C()->MoveTracing(Station.X,Station.Y);
   if (!Check(!C()->Answer(TEXT("bm6")).bOk && C()->S.Wrong13 == 1 && !C()->S.bP13, TEXT("aligned wrong bolt remains wrong, counted independently of warning"))) return;
   Close(); if (!Restore(TEXT("aligned-map-wrong-answer"))) return; Next(12); return;
  }
  if (Stage == 12)
  {
   if (!Check(C()->Aligned() && C()->S.Wrong13 == 1 && !C()->S.bP13, TEXT("aligned tracing and wrong count restore before correct answer"))) return;
   C()->OpenMap(); if (!Check(C()->Answer(TEXT("bm8")).bOk && C()->S.bP13 && C()->S.Stage == TEXT("diner") && C()->DocFinding().Page.Contains(TEXT("8.15")) && C()->DocFinding().Page.Contains(TEXT("10.4")) && !C()->TruckTrip(TEXT("diner")), TEXT("P13 separates C8.15 and proposed courier10.4, but cannot release old road without Ward's route readback"))) return;
   Close(); if (!Restore(TEXT("p13-before-courier"))) return;
   WalkTo({FVector(-1.4,0,3.05), FVector(-1.4,0,7.1), FVector(-.8,0,7.25)}); Next(13,0); return;
  }
  if (Stage == 13) { if (!Walk(Dt)) return; Look(TEXT("diner:payphone")); Next(14,.5); return; }
  if (Stage == 14)
  {
   Look(TEXT("diner:payphone")); if (!Use(TEXT("diner:payphone"))) return;
   C()->Choose(TEXT("ack-route")); if (!Check(!C()->S.SC12.bCourier && !C()->ReadyForRoad(), TEXT("unsolicited route acknowledgement does not confirm a courier")) || !Choice(TEXT("request-courier"))) return;
   if (!Check(C()->PendingReadback == 2 && !C()->S.SC12.bCourier && C()->Lines.ContainsByPredicate([](const S47Panels::FTalkLine& L) { return L.Text.Contains(TEXT("not a map of everywhere")); }), TEXT("Ward's actual route readback includes patrol limits and a choice to remain in refuge"))) return;
   Close(); if (!Restore(TEXT("interrupted-courier-readback"))) return; Next(15,.5); return;
  }
  if (Stage == 15)
  {
   Look(TEXT("diner:payphone")); if (!Use(TEXT("diner:payphone")) || !Choice(TEXT("request-courier")) || !Choice(TEXT("ack-route"))) return;
   if (!Check(C()->ReadyForRoad() && C()->S.Stage == TEXT("road") && C()->TruckTrip(TEXT("diner")).IsSet() && !M()->Story->Begun.Contains(TEXT("ch6")), TEXT("only complete warning/patrol/P13/courier readback releases departure choice"))) return;
   Close(); if (!Restore(TEXT("complete-route-before-departure"))) return; Next(16); return;
  }
  if (Stage == 16)
  {
   if (!Check(C()->ReadyForRoad() && RayUses >= 6 && Restores >= 8 && WalkMetres > 12, TEXT("complete source/readback/action-order/disk journey and physical diner legs executed"))) return;
   const auto Trip = C()->TruckTrip(TEXT("diner")); if (!Check(Trip && bool(Trip->Use), TEXT("production service-truck departure choice exists"))) return;
   Trip->Use();
   if (!Check(FMath::Abs(M()->Story->Clock() - S47Timeline::Departure) < .001 && C()->S.Stage == TEXT("road"), TEXT("normal chosen departure reaches05:20 while stable saved checkpoint still says road"))) return;
   M()->Flow->FlushAutosave(); const auto Meta = M()->Flow->Saves.Latest(1); FS47SaveStore Reader(M()->Flow->Saves.Dir); Reader.Init(); const auto Disk = Meta ? Reader.Load(Meta->Id) : FJ();
   const auto Ch5 = S47J::Obj(S47J::Obj(Disk,TEXT("case")),TEXT("ch5"));
   if (!Check(Disk && S47J::Str(Disk,TEXT("checkpoint")) == TEXT("chapter5") && S47J::Str(Ch5,TEXT("stage")) == TEXT("road") && S47J::Bool(S47J::Obj(Ch5,TEXT("sc12")),TEXT("routeReadback")) && S47J::Num(S47J::Obj(Disk,TEXT("case")),TEXT("clock")) == S47Timeline::Departure, TEXT("fresh disk reader sees ordinary safe05:20 departure checkpoint before chapter6"))) return;
   if (!Write(TEXT("departure-checkpoint.disk.json"), Disk)) { Result(false,TEXT("departure disk evidence write failed")); return; }
   Next(17,1); return;
  }
  if (Stage == 17)
  {
   if (!Check(M()->Story->Begun.Contains(TEXT("ch6")) && !C()->bActive && C()->S.Stage == TEXT("complete") && M()->Story->Clock() >= S47Timeline::Departure && M()->Story->Clock() < S47Timeline::Finale, TEXT("normal chapter6 continuation begins only after complete SC12; finale time remains reserved"))) return;
   if (!Snapshot(TEXT("departed-to-sc13"))) return;
   Result(true,TEXT("SC12 source comparison, text-only Ward receipts, physical dated patrol sheet, P13, chosen courier route and stable full Flow restores"));
  }
 }
};

void US47Chapter5::ControlSC12Test(double Dt)
{
 if (!SC12Contract) SC12Contract = MakeShared<FS47SC12Contract>(M.Get());
 const auto KeepAlive = SC12Contract; // Full Flow resets this chapter's state, not the in-flight test's lifetime.
 KeepAlive->Tick(Dt);
}
