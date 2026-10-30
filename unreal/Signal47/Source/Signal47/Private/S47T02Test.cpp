#include "S47T02Test.h"

#include "S47Album.h"
#include "S47Audio.h"
#include "S47Chapter1.h"
#include "S47Chapter2.h"
#include "S47Chapter3.h"
#include "S47Evening.h"
#include "S47FieldCamera.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Office.h"
#include "S47OfficeOwned.h"
#include "S47Phones.h"
#include "S47SaroInterior.h"
#include "S47SaroNight.h"
#include "S47Saves.h"
#include "S47Story.h"
#include "S47Substation.h"
#include "S47T02Photo.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Base64.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47T02Test, Log, All);

struct FS47T02Test::FState
{
 bool bEnabled = false, bDone = false, bNative = false, bNativeStarted = false, bReturn = false, bUnresolved = false, bEarlyOffice = false, bArrivalOffice = false;
 bool bOutSaro = false, bOutRoad = false, bBackRoad = false, bBackSaro = false;
 bool bSaroInterior = false;
 int32 Stage = 0, Checks = 0, Loads = 0, StoreReads = 0, WalkIndex = 0, InteriorContinues = 0;
 double Elapsed = 0, Since = 0, Hold = 3, WalkDistance = 0, LastProgress = 0;
 double LastClock = 0, OdoStart = 0, OutMetres = 0, BackMetres = 0, NativeWalkMetres = 0;
 FVector LastFoot = FVector::ZeroVector;
 FVector LastTruck = FVector::ZeroVector, LastNativeFoot = FVector::ZeroVector;
 FVector FieldTruck = FVector::ZeroVector, FieldEgress = FVector::ZeroVector;
 FString LastLeg, LastFootArea;
 TArray<FVector> WalkPoints;
 FString Root, Baseline, FirstPhoto, LatestPhoto, FieldFilmBefore, PendingFailure, OfficePhase;
 int32 AlbumCount = 0, AlbumLeft = 0, FilingNotes = 0, FilingHistory = 0;
 S47J::FObj GoodOffice, GoodPhoto, PendingVerified, GoodFiled, FiledCard;
 S47J::FObj InteriorLocked, InteriorLockedDocs, InteriorEarnedDocs;
 FString FiledHistoryBytes, OfficeIndexTitle, OfficeIndexPage;
};

FS47T02Test::FS47T02Test(US47Main* InM) : State(MakeUnique<FState>()), M(InM)
{
 State->bArrivalOffice = FParse::Param(FCommandLine::Get(), TEXT("S47OfficeArrival"));
 State->bSaroInterior = FParse::Param(FCommandLine::Get(), TEXT("S47SaroInteriorContract"));
 State->bEarlyOffice = State->bArrivalOffice || FParse::Param(FCommandLine::Get(), TEXT("S47OfficeEarly"));
 State->bNative = FParse::Param(FCommandLine::Get(), TEXT("S47T02DriveContract"));
 State->bUnresolved = FParse::Param(FCommandLine::Get(), TEXT("S47T02UnresolvedContract"));
 State->bEnabled = State->bSaroInterior || State->bEarlyOffice || State->bNative || State->bUnresolved || FParse::Param(FCommandLine::Get(), TEXT("S47T02Contract"));
}
FS47T02Test::~FS47T02Test() = default;

void FS47T02Test::Fail(const FString& Why)
{
 if (State->bDone) return;
 State->bDone = true;
 if (M && M->Player) M->Player->TestMove.Reset();
 if (M && M->World && State->bNative) { M->World->Autopilot.Reset(); M->World->TestInput = nullptr; }
 UE_LOG(LogS47T02Test, Error, TEXT("S47: T02 contract FAIL stage %d at %.2f s: %s"), State->Stage, State->Elapsed, *Why);
 if (!State->Root.IsEmpty())
 {
  auto O = S47J::New(); S47J::Set(O, TEXT("status"), TEXT("FAIL")); S47J::Set(O, TEXT("stage"), State->Stage);
  S47J::Set(O, TEXT("reason"), Why); S47J::Set(O, TEXT("checks"), State->Checks);
  FFileHelper::SaveStringToFile(S47J::Write(O, true), *(State->Root / TEXT("result.json")));
 }
 FPlatformMisc::RequestExitWithStatus(false, 1);
}
bool FS47T02Test::Check(bool Good, const FString& What)
{
 ++State->Checks;
 if (!Good) { Fail(What); return false; }
 UE_LOG(LogS47T02Test, Display, TEXT("S47: T02 contract PASS: %s"), *What); return true;
}
void FS47T02Test::Stage(int32 Next, double Hold)
{
 State->Stage = Next; State->Since = State->Elapsed; State->Hold = Hold;
 UE_LOG(LogS47T02Test, Display, TEXT("S47: T02 contract stage %d at %.2f s"), Next, State->Elapsed);
}
FS47Obj FS47T02Test::Object(const FString& Id) const
{
 if (Id.StartsWith(TEXT("office:")) && M->Office) return M->Office->Object(Id);
 if (const auto* Item = M->Inter->Get(Id)) return Item->Object;
 if (const auto* Item = M->Web->Item(Id)) return M->Web->ObjAt(Item->Node);
 return {};
}
FVector FS47T02Test::Target(const FString& Id) const
{
 const FS47Obj Obj = Object(Id); const FBox Bounds = Obj ? Obj.WorldBox() : FBox(ForceInit);
 return Bounds.IsValid ? Bounds.GetCenter() : Obj ? Obj.WorldPosition() : FVector::ZeroVector;
}
void FS47T02Test::Aim(const FVector& Point)
{
 FVector Eye = S47Three::ToThree(M->Player->EyeUE());
 // Place updates feet/floor immediately, but the ordinary camera follows on the next
 // Walker tick. Aim from the new standing eye for that one fixture frame; never move it.
 const FVector StandingEye(M->Player->Pos.X, M->Player->FloorY + M->Player->Eye, M->Player->Pos.Z);
 if (FVector2D(Eye.X - StandingEye.X, Eye.Z - StandingEye.Z).Size() > .08 || FMath::Abs(Eye.Y - StandingEye.Y) > .35) Eye = StandingEye;
 const FVector D = Point - Eye;
 M->Player->Yaw = FMath::Atan2(-D.X, -D.Z);
 M->Player->Pitch = FMath::Atan2(D.Y, FVector2D(D.X, D.Z).Size());
}
bool FS47T02Test::Pick(const FString& Id, const FString& Alternate, bool bUse)
{
 const FS47Obj Obj = Object(Id);
 if (!Obj) { Fail(TEXT("missing physical target: ") + Id); return false; }
 const FVector Eye = M->Player->EyeUE(), Direction = (S47Three::ToUE(Target(Id)) - Eye).GetSafeNormal();
 const FString Actual = M->Inter->Pick(Eye, Direction);
 if ((Actual != Id && (Alternate.IsEmpty() || Actual != Alternate)) || (bUse && !M->Player->OnUse) ||
  FVector::DotProduct(Direction, M->Player->ForwardUE()) < .94)
 { Fail(FString::Printf(TEXT("actual aimed ray for %s picked '%s' (view dot %.3f)"), *Id, *Actual, FVector::DotProduct(Direction, M->Player->ForwardUE()))); return false; }
 if (bUse) M->Player->OnUse(Actual);
 UE_LOG(LogS47T02Test, Display, TEXT("S47: T02 contract physical %s: %s"), bUse ? TEXT("use") : TEXT("ray"), *Actual); return true;
}
void FS47T02Test::Path(const TArray<FVector>& Points)
{
 State->WalkPoints = Points; State->WalkIndex = 0; State->LastFoot = M->Player->Pos; State->LastProgress = State->Elapsed;
}
bool FS47T02Test::Walk()
{
 const FVector Now = M->Player->Pos;
 const double Moved = FVector2D(Now.X - State->LastFoot.X, Now.Z - State->LastFoot.Z).Size();
 if (Moved > .5) { Fail(TEXT("continuous waypoint walk jumped farther than a real walking step")); return false; }
 State->WalkDistance += Moved; State->LastFoot = Now;
 if (Moved > .002) State->LastProgress = State->Elapsed;
 if (State->WalkIndex >= State->WalkPoints.Num()) { M->Player->TestMove.Reset(); return true; }
 const FVector To = State->WalkPoints[State->WalkIndex], D = To - Now;
 const double Distance = FVector2D(D.X, D.Z).Size();
 if (Distance < .11) { ++State->WalkIndex; M->Player->TestMove.Reset(); State->LastProgress = State->Elapsed; return false; }
 if (State->Elapsed - State->LastProgress > 4) { Fail(TEXT("actual Walker/Resolve path stalled against geometry")); return false; }
 M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = -.06;
 M->Player->TestMove = FVector2D(0, -FMath::Min(1.0, Distance * 2)); return false;
}
bool FS47T02Test::AuditNative(double Dt)
{
 auto& T = *State;
 if (!T.bNativeStarted) return true;
 if (M->Story->Clock() + .001 < T.LastClock) { Fail(TEXT("clock moved backwards during continuous native route")); return false; }
 T.LastClock = M->Story->Clock();
 if (M->World->bDriving)
 {
  const auto* Truck = M->World->CabTruck();
  if (!Truck) { Fail(TEXT("native trip has no actual cab truck")); return false; }
  if (M->World->Leg == T.LastLeg)
  {
   const double Moved = FVector::Dist(Truck->Position, T.LastTruck);
   if (Moved > FMath::Max(6.0, FMath::Abs(M->World->DrivingSpeed()) * Dt + 3))
   { Fail(TEXT("non-native truck discontinuity within one leg")); return false; }
   (T.bReturn ? T.BackMetres : T.OutMetres) += Moved;
  }
  T.LastLeg = M->World->Leg; T.LastTruck = Truck->Position; T.LastFootArea.Empty();
  if (T.bReturn) { T.bBackRoad |= T.LastLeg == TEXT("road"); T.bBackSaro |= T.LastLeg == TEXT("saro"); }
  else { T.bOutSaro |= T.LastLeg == TEXT("saro"); T.bOutRoad |= T.LastLeg == TEXT("road"); }
 }
 else
 {
  T.LastLeg.Empty();
  if (!M->World->bBusy && M->World->Area == T.LastFootArea)
  {
   const double Moved = FVector2D(M->Player->Pos.X - T.LastNativeFoot.X, M->Player->Pos.Z - T.LastNativeFoot.Z).Size();
   if (Moved > FMath::Max(3.0, Dt * 12)) { Fail(TEXT("walker discontinuity during native route")); return false; }
   T.NativeWalkMetres += Moved;
  }
  T.LastFootArea = M->World->Area; T.LastNativeFoot = M->Player->Pos;
 }
 return true;
}
void FS47T02Test::Shot(const FString& Name)
{
 FScreenshotRequest::RequestScreenshot(State->Root / (Name + TEXT(".png")), true, false);
}

bool FS47T02Test::SaveLoad(const FString& Name, const S47J::FObj& Source, bool WantVerified, bool RemovePhotoFile)
{
 if (!Source) { Fail(TEXT("missing source state: ") + Name); return false; }
 const auto Input = S47J::Clone(Source);
 const FString Area = S47J::Str(Input, TEXT("area")); const auto Pose = S47J::Obj(Input, TEXT("pose"));
 // The opening snapshot intentionally has no chapter case. Its authoritative
 // clock is evening.clock; later chapter saves retain case.clock.
 const bool OpeningSave = S47J::Str(Input, TEXT("checkpoint")) == TEXT("evening");
 const double Clock = S47J::Num(S47J::Obj(Input, OpeningSave ? TEXT("evening") : TEXT("case")), TEXT("clock"));
 const auto Office = S47J::Obj(Input, TEXT("office"));
 const auto Followup = S47J::Obj(S47J::Obj(Input, TEXT("substation")), TEXT("t02"));
 const auto Photo = S47J::Obj(Followup, TEXT("photo"));
 const FString PhotoId = S47J::Str(Photo, TEXT("id")), Url = S47J::Str(Photo, TEXT("url"));
 const FString StoreName = State->bNative ? FString(TEXT("native-")) + Name : Name;
 if (State->bNative && RemovePhotoFile) { Fail(TEXT("negative disk mutations are forbidden in continuous native mode")); return false; }
 FS47SaveStore Writer(State->Root / StoreName); Writer.Init();
 FS47SaveCard Card; Card.Chapter = TEXT("T02 live API/disk contract"); Card.Place = Area; Card.Clock = M->Story->ClockText(true); Card.Playtime = M->Playtime;
 FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, Input, Card, Error);
 if (!Meta) { Fail(TEXT("actual isolated store write: ") + Name + TEXT(": ") + Error); return false; }
 if (RemovePhotoFile)
 {
  const FString File = State->Root / StoreName / TEXT("photos") / (S47Saves::PhotoKey(Url) + TEXT(".txt"));
  if (!Check(!Url.IsEmpty() && IFileManager::Get().FileExists(*File) && IFileManager::Get().Delete(*File), TEXT("removed only the actual current T02 photo file in isolated store"))) return false;
 }
 FS47SaveStore Reader(State->Root / StoreName); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
 if (!Disk) { Fail(TEXT("fresh store read failed: ") + Name); return false; }
 const auto DiskPhoto = S47J::Obj(S47J::Obj(S47J::Obj(Disk, TEXT("substation")), TEXT("t02")), TEXT("photo"));
 if (!PhotoId.IsEmpty() && !Check(S47J::Str(DiskPhoto, TEXT("id")) == PhotoId &&
  (RemovePhotoFile ? S47J::Bool(DiskPhoto, TEXT("unavailable")) : S47J::Str(DiskPhoto, TEXT("url")) == Url), TEXT("fresh store restores original photo or explicitly marks the missing file unavailable: ") + Name)) return false;
 ++State->StoreReads;
 FFileHelper::SaveStringToFile(S47J::Write(Disk, true), *(State->Root / (StoreName + TEXT(".loaded.json"))));
 if (State->bNative)
 {
  // A fresh disk reader verifies bytes without interrupting the actual journey.
  // In particular, no StartGame, Player.Place or world relocation is permitted here.
  return Check(M->Substation->FollowupVerified() == WantVerified &&
   S47J::Write(S47J::Obj(Disk,TEXT("office"))) == S47J::Write(Office) &&
   S47J::Str(S47J::Obj(Disk,TEXT("substation")),TEXT("completedStamp")) == S47J::Str(S47J::Obj(Input,TEXT("substation")),TEXT("completedStamp")),
   TEXT("native writer/fresh-reader store without live world reload: ") + StoreName);
 }
 US47Flow::FStart Start; Start.State = Disk; Start.CaseId = 0; Start.bDev = true; Start.Playtime = M->Playtime;
 M->Flow->StartGame(Start); ++State->Loads;
 const bool PoseOk = Pose && FMath::Abs(M->Player->Pos.X - S47J::Num(Pose,TEXT("x"))) < .035 &&
  FMath::Abs(M->Player->Pos.Z - S47J::Num(Pose,TEXT("z"))) < .035;
 UE_LOG(LogS47T02Test, Display, TEXT("S47: restore audit %s dev=%d case=%d area=%s/%s pose=%d clock=%.9f/%.9f key=%d/%d sheet=%d/%d verified=%d/%d filed=%d/%d"),
  *Name, M->Flow->bDevRun, M->Flow->CaseId, *M->World->Area, *Area, PoseOk, M->Story->Clock(), Clock,
  M->Office->HasKey(), S47J::Bool(Office,TEXT("key")), M->Office->HasFieldSheet(), S47J::Bool(Office,TEXT("hasFieldSheet")),
  M->Substation->FollowupVerified(), WantVerified, M->Substation->FollowupFiled(), !RemovePhotoFile && S47J::Bool(Followup,TEXT("filed")));
 return Check(M->Flow->bDevRun && M->Flow->CaseId == 0 && M->World->Area == Area && PoseOk && M->Story->Clock() == Clock &&
  M->Office->HasKey() == S47J::Bool(Office,TEXT("key")) && M->Office->HasFieldSheet() == S47J::Bool(Office,TEXT("hasFieldSheet")) &&
  M->Substation->FollowupVerified() == WantVerified &&
  M->Substation->FollowupFiled() == (!RemovePhotoFile && S47J::Bool(Followup,TEXT("filed"))),
  TEXT("actual writer/fresh-reader/full-Flow restore preserves pose/time/Office and expected T02 proof: ") + Name);
}

bool FS47T02Test::AdvanceDialogue(const FString& Choice)
{
 auto& Pack = M->Evening->Pack;
 if (Pack.CurrentLine())
 {
  M->UI->Close(true); Pack.Advance(); M->Evening->ShowDialogue(); State->Hold = .08; return false;
 }
 if (Pack.CurrentScene())
 {
  if (Choice.IsEmpty()) { Fail(TEXT("unexpected authored choice in UI-only test scene")); return false; }
  if (!Pack.Select(Choice)) { Fail(TEXT("real authored choice was unavailable: ") + Choice); return false; }
  M->UI->Close(true); M->Evening->ShowDialogue(); State->Hold = .08; return false;
 }
 M->UI->Close(true); return true;
}

bool FS47T02Test::NoTransformerContext() const
{
 for (const TCHAR* Id : {TEXT("ctx_trafo_isolation_verified"),TEXT("ctx_trafo_backup_excluded"),TEXT("ctx_trafo_indicator_checked"),TEXT("ctx_trafo_lamp_persists"),TEXT("ctx_trafo_photo_available")})
  if (M->Evening->Pack.Flag(Id)) return false;
 return true;
}

bool FS47T02Test::CheckFiledPaper(const S47J::FObj& OriginalCard, const FString& Context)
{
 if(!OriginalCard){Fail(TEXT("missing original physically filed history card: ")+Context);return false;}
 const FString Title=S47J::Str(OriginalCard,TEXT("cardTitle")),Stamp=S47J::Str(OriginalCard,TEXT("cardStamp")),Page=S47J::Str(OriginalCard,TEXT("cardPage"));
 const FS47Doc* Found=nullptr;int32 Count=0;
 for(const auto& D:M->Story->Docs()) if(D.Id==TEXT("EV_TRAFO_FOLLOWUP")){++Count;Found=&D;}
 const auto History=S47J::Arr(M->Substation->FollowupSnapshot(),TEXT("filedHistory"));
 const auto CurrentCard=!History.IsEmpty()&&History.Last().IsValid()&&History.Last()->Type==EJson::Object?History.Last()->AsObject():S47J::FObj();
 const FS47Doc Default=M->Substation->FiledFollowupDocument();
 if(!Check(!Title.IsEmpty()&&!Stamp.IsEmpty()&&!Page.IsEmpty() && Count==1 && Found &&
  Found->Title==Title && Found->Stamp==Stamp && Found->Page==Page && Found->Image.IsEmpty() &&
  Default.Id==TEXT("EV_TRAFO_FOLLOWUP") && Default.Title==Title && Default.Stamp==Stamp && Default.Page==Page && Default.Image.IsEmpty() &&
  CurrentCard && S47J::Str(CurrentCard,TEXT("cardTitle"))==Title && S47J::Str(CurrentCard,TEXT("cardStamp"))==Stamp && S47J::Str(CurrentCard,TEXT("cardPage"))==Page,
  TEXT("exactly one permanent T02 paper matches original immutable filing title/stamp/page without redundant image: ")+Context))return false;
 auto Evidence=S47J::New();S47J::Set(Evidence,TEXT("context"),Context);S47J::Set(Evidence,TEXT("documentId"),Found->Id);
 S47J::Set(Evidence,TEXT("title"),Found->Title);S47J::Set(Evidence,TEXT("stamp"),Found->Stamp);S47J::Set(Evidence,TEXT("page"),Found->Page);
 S47J::Set(Evidence,TEXT("matchingDocumentCount"),Count);S47J::Set(Evidence,TEXT("permanentImageEmpty"),Found->Image.IsEmpty());
 S47J::Set(Evidence,TEXT("fullFlowReloadBeforeObservation"),!State->bNative);
 S47J::Set(Evidence,TEXT("limitation"),State->bNative?TEXT("Actual live Story.Docs observed after fresh disk reader; native journey deliberately has no world/memory reload"):TEXT("Actual live Story.Docs observed after isolated full Flow restoration"));
 FFileHelper::SaveStringToFile(S47J::Write(Evidence,true),*(State->Root/(Context+TEXT(".paper.json"))));
 return true;
}

bool FS47T02Test::PlaceAtSaroFolder()
{
 // API-only return fixture. The native driver must use the reverse road and Walker.
 if (State->bNative) { Fail(TEXT("return placement fixture is forbidden during the native journey")); return false; }
 M->World->PlaceTruck(TEXT("saro")); M->World->Enter(TEXT("saro"));
 const FVector Book=Target(TEXT("logbook")); bool Placed=false;
 for(double Radius:{.9,1.2,1.5}) for(int32 I=0;I<24&&!Placed;++I)
 {
  const double A=I*PI/12; const FVector P(Book.X+FMath::Cos(A)*Radius,0,Book.Z+FMath::Sin(A)*Radius);
  if(!M->Web->Rect(TEXT("room.bounds")).Contains(P)||!M->Player->Walkable(P.X,P.Z))continue;
  bool Blocked=false;
  for(const auto& C:M->Web->Colliders()) if(C.bEnabled&&P.X+M->Player->Radius>C.MinX&&P.X-M->Player->Radius<C.MaxX&&P.Z+M->Player->Radius>C.MinZ&&P.Z-M->Player->Radius<C.MaxZ) {Blocked=true;break;}
  if(!Blocked){M->Player->Place(P.X,P.Z,0);Placed=true;}
 }
 if(!Placed){Fail(TEXT("no collider-clear physical SARO folder return fixture"));return false;}
 UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract fixture: SARO truck/player return placement; actual folder ray and one filing remain required; no native-travel claim"));
 Aim(Book); return true;
}

void FS47T02Test::Step(double Dt)
{
 auto& T = *State;
 if (!T.bEnabled || T.bDone) return;
 if (!M || !M->Story || !M->Flow || !M->World || !M->Web || !M->Player || !M->Inter || !M->UI || !M->Office || !M->Substation || !M->Evening || !M->Night || !M->Night->Album)
 { Fail(TEXT("runtime dependencies missing")); return; }
 T.Elapsed += Dt;
 if (T.Elapsed > (T.bNative ? 900 : 300)) { Fail(T.bNative ? TEXT("900 s native route timeout") : TEXT("300 s contract timeout")); return; }
 if (!M->Flow->bDevRun || M->Flow->CaseId != 0) { Fail(TEXT("requires developer case zero; never operate on a player case")); return; }
 if (!AuditNative(Dt)) return;
 if (M->Flow->bStarting || M->World->bBusy) return;
 if (T.Hold > 0) { T.Hold -= Dt; return; }
 auto* C1 = Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));
 auto* C2 = Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2")));
 auto* C3 = Cast<US47Chapter3>(M->Story->Chapter(TEXT("ch3")));
 if (!C1 || !C2 || !C3 || !M->FieldCamera) { Fail(TEXT("native chapter/camera dependencies missing")); return; }
 auto Snapshot = [this] { return M->Substation->FollowupSnapshot(); };
 auto Lower = [this, C1] { if (M->FieldCamera->bRaised) C1->ToggleCamera(); };
 auto RefAim = [this]
 { M->Player->Yaw = FMath::DegreesToRadians(205.0); M->Player->Pitch = FMath::DegreesToRadians(2.0); };
 auto InteriorDocs = [this]
 {
  auto O = S47J::New();
  for (const FS47Doc& Doc : M->Story->Docs()) if (Doc.Id.StartsWith(TEXT("EV_DALE_")))
  {
   auto D = S47J::New(); S47J::Set(D, TEXT("title"), Doc.Title); S47J::Set(D, TEXT("page"), Doc.Page);
   S47J::Set(D, TEXT("transcript"), Doc.Transcript); S47J::Set(D, TEXT("image"), Doc.Image); S47J::Set(O, *Doc.Id, D);
  }
  return O;
 };
 if (T.Stage == 917) { Fail(T.PendingFailure); return; }

 if (T.Stage == 0)
 {
  if (T.bSaroInterior && !T.bEarlyOffice) { Fail(TEXT("S47SaroInteriorContract is an opt-in addition to OfficeEarly or OfficeArrival only")); return; }
  const int32 DriverCount=int32(T.bEarlyOffice)+int32(T.bNative)+int32(T.bUnresolved)+int32(FParse::Param(FCommandLine::Get(),TEXT("S47T02Contract")));
  if (DriverCount!=1) { Fail(TEXT("choose exactly one API/disk, unresolved or native T02 driver")); return; }
  FString UserDir, Jump, EveningMode; FParse::Value(FCommandLine::Get(), TEXT("S47Evening="), EveningMode); FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir); FParse::Value(FCommandLine::Get(), TEXT("S47Jump="), Jump);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  if (!((UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) &&
   FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir)) || (T.bArrivalOffice ? (!Jump.IsEmpty() || EveningMode != TEXT("parking")) : Jump != (T.bEarlyOffice ? TEXT("chapter1") : TEXT("chapter2"))))
  { Fail(TEXT("requires isolated /tmp UserDir and chapter2; OfficeEarly chapter1; OfficeArrival S47Evening=parking without Jump")); return; }
  for (const TCHAR* Flag : {TEXT("S47T01Contract"),TEXT("S47T01DriveContract"),TEXT("S47SmokeTest"),TEXT("S47SubstationTest"),TEXT("S47SubstationTestLoad")})
   if (FParse::Param(FCommandLine::Get(), Flag)) { Fail(TEXT("do not combine independent live control drivers")); return; }
  T.Root = FPaths::ProjectSavedDir() / (T.bEarlyOffice ? TEXT("OfficeEarly") : T.bNative ? TEXT("T02NativeRoute") : T.bUnresolved ? TEXT("T02UnresolvedContract") : TEXT("T02Contract")) / FGuid::NewGuid().ToString(EGuidFormats::Digits);
  IFileManager::Get().MakeDirectory(*T.Root, true);
  if (T.bArrivalOffice) { Stage(-4,.5); return; }

  // Isolated old-chapter fixture BEFORE physical Office/native START. StartGame's
  // explicit nonempty Jump selects timelineVersion1 and does not begin the opening.
  US47Flow::FStart Legacy; Legacy.Jump=TEXT("chapter1"); Legacy.bDev=true; Legacy.CaseId=0;
  M->Flow->StartGame(Legacy);
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract fixture: isolated legacy chapter-one StartGame before physical Office/native START; no Office/T02 facts assigned"));
  Stage(-3,.5); return;
 }
 if (T.Stage == -4)
 {
  if (!Check(M->Office->Ready() && M->Evening->Opening() && M->Story->bEveningTimeline && !M->Evening->Pack.Flag(TEXT("handover_done")) && !M->Story->Begun.Contains(TEXT("ch2")) && !M->Office->HasKey(), TEXT("actual arrival boot retains active opening, unfinished D01 and locked office"))) return;
  T.OfficePhase = M->Evening->Pack.GetPhase();
  M->UI->Close(true); M->Story->SetCinematic(false); M->Story->ClearTimers();
  M->Player->Place(.45,5.75,0); Aim(Target(TEXT("office:door")));
  Stage(1,.35); return;
 }
 if (T.Stage == -3)
 {
  if(!Check(M->Story->ChapterKey==TEXT("ch1") && !M->Story->bEveningTimeline && !M->Evening->Enabled() &&
   !M->Office->RouteRequired() && !M->Office->HasKey() && !M->Office->HasFieldSheet(),TEXT("actual chapter-one StartGame retains legacy timeline with investigation disabled and no new Office route requirement")))return;
  if (T.bEarlyOffice)
  {
   // Real chapter-one baseline: no archive, Ward or evening-pack prerequisites.
   // Only the initial corridor pose is a fixture; keys, opening, walking and papers
   // below use the same production interaction/ray route as the full T02 test.
   M->UI->Close(true); M->Story->SetCinematic(false); M->Story->ClearTimers();
   if (!Check(M->Office->Ready() && !M->Story->Begun.Contains(TEXT("ch2")), TEXT("early office is built before any archive chapter"))) return;
   M->Player->Place(.45,5.75,0); Aim(Target(TEXT("office:door")));
   Stage(1,.35); return;
  }
  C2->BeginChapter2(nullptr); Stage(-2,.5); return;
 }
 if (T.Stage == -2)
 {
  if(!Check(M->Story->ChapterKey==TEXT("ch2") && !M->Story->bEveningTimeline && M->Evening->Enabled() &&
   !M->Office->RouteRequired() && !M->Evening->Pack.Flag(TEXT("handover_done")),TEXT("ordinary C2 BeginChapter2 activates investigation from an older chapter-one case without new-night or D01 requirements")))return;
  // End the isolated legacy-hook fixture before starting the physical contract.
  // The established chapter-two jump supplies the existing completed chapter-one
  // camera kit; never manufacture camera ownership or any new Office/T02 fact.
  US47Flow::FStart Existing;Existing.Jump=TEXT("chapter2");Existing.bDev=true;Existing.CaseId=0;
  M->Flow->StartGame(Existing);
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract fixture: legacy-hook test complete; reset to established chapter-two fixture and existing chapter-one kit before physical/native START"));
  Stage(-1,.5);return;
 }
 if (T.Stage == -1)
 {
  if (!Check(M->Story->ChapterKey == TEXT("ch2") && M->Office->Ready() && M->Evening->Enabled() && !M->Office->HasKey() && !M->Office->HasFieldSheet(), TEXT("actual ch2 legacy fixture starts with physical Office still locked and empty"))) return;
  // The existing Ward/archive prerequisites are fixtures, never new Office/T02 credit.
  M->UI->Close(true); M->Story->SetCinematic(false); M->Story->ClearTimers();
  C2->bRinging = false; C2->S.bAnswered = true; C2->S.bDoor = true; C2->S.bEntered = true; C2->S.Stage = TEXT("records");
  M->Player->Place(.45,5.75,0); Aim(Target(TEXT("office:door")));
  UE_LOG(LogS47T02Test, Display, TEXT("S47: T02 contract START: ch2/archive start and corridor placement are fixtures; physical key/door/threshold/papers and all T02 facts use production APIs; human input/full native travel UNVERIFIED"));
  if (T.bNative)
  {
   T.bNativeStarted = true; T.LastClock = M->Story->Clock(); T.OdoStart = M->World->Odometer();
   UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 native route START: legacy ch2/Ward/initial corridor and later P04/P05/Nora/native trip target are fixtures; no test placement or intermediate Flow load after START; physical Office, native driving and Walker thereafter; manual Roam/human input UNVERIFIED"));
  }
  Stage(1,.35); return;
 }
 if (T.Stage == 1)
 {
  if (T.bSaroInterior)
  {
   // Establish the ordinary production restore baseline first. Legacy Flow adds
   // its existing explanatory timeline note on Continue; that is independent of
   // interior migration and must already be present in the comparison source.
   if (!SaveLoad(TEXT("interior-locked-normal-baseline"),M->Flow->GameState(),false)) return;
   T.InteriorLocked = S47J::Clone(M->Flow->GameState()); T.InteriorLockedDocs = InteriorDocs();
   if (!Check(!M->Office->HasKey() && !M->Office->WasUnlocked() && !S47J::Bool(M->Office->Save(),TEXT("entered")), TEXT("interior locked cohort captured before actual spare acquisition, without fact setters"))) return;
  }
  if (T.bEarlyOffice) Shot(T.bArrivalOffice ? TEXT("arrival-office-locked") : TEXT("early-office-locked"));
  Stage(1001,.35); return;
 }
 if (T.Stage == 1001)
 {
  if (!Pick(TEXT("office:door"),TEXT("officeDoor")) || !Check(!M->Office->WasUnlocked(),TEXT("physical locked door directs the player to its own service drawer"))) return;
  Aim(Target(TEXT("office:drawer"))); Stage(2); return;
 }
 if (T.Stage == 2) { if (!Pick(TEXT("office:drawer"))) return; Stage(3,.55); return; }
 if (T.Stage == 3) { Aim(Target(TEXT("office:key"))); Stage(4); return; }
 if (T.Stage == 4)
 {
  if (T.bEarlyOffice) Shot(TEXT("office-open-service-drawer"));
  Stage(41,.35); return;
 }
 if (T.Stage == 41)
 {
  if (!Pick(TEXT("office:key")) || !Check(M->Office->HasKey(),TEXT("actual opened drawer key ray retains the independent Operations key"))) return;
  Aim(Target(TEXT("office:door"))); Stage(5); return;
 }
 if (T.Stage == 5) { if (!Pick(TEXT("office:door"),TEXT("officeDoor"))) return; Stage(6,.8); return; }
 if (T.Stage == 6)
 {
  if (!Check(M->Office->DoorOpen() && M->Office->WasUnlocked(),TEXT("actual door leaf finishes opening before the threshold walk"))) return;
  Path({FVector(.45,0,6.2),FVector(.45,0,6.6),FVector(.45,0,7.0),FVector(.45,0,7.4),FVector(.40,0,8.1),FVector(.35,0,8.8)}); Stage(7,0); return;
 }
 if (T.Stage == 7)
 {
  if (!Walk()) return;
  if (!Check(M->Office->Contains(M->Player->Pos) && S47J::Bool(M->Office->Save(),TEXT("entered")) && T.WalkDistance > 2.5,TEXT("real Walker/Resolve movement crosses the opened threshold into Operations"))) return;
  Aim(FVector(.45,1.2,9.65)); Stage(71,.35); return;
 }
 if (T.Stage == 71)
 {
  Shot(TEXT("office-entered")); Stage(72,.35); return;
 }
 if (T.Stage == 72)
 {
  Aim(Target(TEXT("office:folder"))); Stage(8); return;
 }
 if (T.Stage == 8)
 {
  if (!Pick(TEXT("office:folder"))) return; M->UI->Close(true); Stage(9); return;
 }
 if (T.Stage == 9)
 {
  if (T.bEarlyOffice)
  {
   if (!Check((T.bArrivalOffice ? (M->Evening->Opening() && !M->Evening->Pack.Flag(TEXT("handover_done")) && M->Evening->Pack.GetPhase() == T.OfficePhase) : M->Story->ChapterKey == TEXT("ch1")) && M->Office->HasKey() && M->Office->DoorOpen() && M->Office->HasFieldSheet() && !M->Office->WantsDeparture() && !M->Office->DepartureReleased(), TEXT("early physical key, doorway and paper route works without chapter-two gate or premature departure"))) return;
   const FS47Doc* Index = M->Story->Docs().FindByPredicate([](const FS47Doc& D) { return D.Id == TEXT("EV_DALE_INDEX"); });
   if (!Check(Index && !Index->Page.IsEmpty(), TEXT("actual early field-sheet reading retained its authored document before save"))) return;
   T.OfficeIndexTitle = Index->Title; T.OfficeIndexPage = Index->Page;
   T.GoodOffice = S47J::Clone(M->Flow->GameState());
   if (!SaveLoad(TEXT("early-office-field-sheet"), T.GoodOffice, false)) return;
   Stage(94,.5); return;
  }
  if (!Check(M->Office->HasFieldSheet() && M->Evening->Pack.Flag(TEXT("office_evidence_taken")) && !M->Evening->Pack.Flag(TEXT("vhs_seen")),TEXT("physical field folder commits real papers without VHS completion"))) return;
  T.GoodOffice=S47J::Clone(M->Flow->GameState());
  if (!SaveLoad(TEXT("office-field-sheet"),T.GoodOffice,false)) return;
  if (!T.bNative) {Stage(90,.4);return;}
  Stage(10,.7); return;
 }
 if (T.Stage == 94)
 {
  if (!Check((T.bArrivalOffice ? (M->Evening->Opening() && !M->Evening->Pack.Flag(TEXT("handover_done")) && M->Evening->Pack.GetPhase() == T.OfficePhase) : M->Story->ChapterKey == TEXT("ch1")) && M->Office->HasFieldSheet() && M->Office->DoorOpen() && M->Office->Contains(M->Player->Pos) && !M->Office->WantsDeparture() && !M->Evening->Pack.Flag(TEXT("vhs_seen")), TEXT("fresh disk and full Flow restore keeps early access, original field sheet and no video/departure credit"))) return;
  const FS47Doc* Index = M->Story->Docs().FindByPredicate([](const FS47Doc& D) { return D.Id == TEXT("EV_DALE_INDEX"); });
  const bool UnreadGranted = M->Story->Docs().ContainsByPredicate([](const FS47Doc& D) { return D.Id == TEXT("EV_DALE_CLIPPINGS") || D.Id == TEXT("EV_DALE_NOTE") || D.Id == TEXT("EV_DALE_CONTACT"); });
  if (!Check(Index && Index->Title == T.OfficeIndexTitle && Index->Page == T.OfficeIndexPage && !UnreadGranted && !M->ModalOpen(), TEXT("fresh early restore refiles the exact acquired field index nonmodally without granting unread office papers"))) return;
  if (T.bSaroInterior)
  {
   // GoodOffice was captured before the normal field-sheet restore. Compare
   // cohorts against its genuinely restored state, including existing legacy
   // timeline annotation, rather than manufacturing or ignoring story notes.
   T.GoodOffice = S47J::Clone(M->Flow->GameState());
   T.InteriorEarnedDocs = InteriorDocs();
   Path({FVector(.40,0,8.1),FVector(.45,0,7.4),FVector(.47,0,6.8)}); Stage(800,0); return;
  }
  Aim(FVector(.45, 1.2, 9.65)); Stage(95,.35); return;
 }
 if (T.Stage == 800)
 {
  if (!Walk()) return;
  Stage(801,.25); return;
 }
 if (T.Stage == 801)
 {
  TArray<FString> OwnedLines;
  const bool bOwnedReady = S47OfficeOwned::Audit(M->Web,OwnedLines);
  for (const FString& Line:OwnedLines) UE_LOG(LogTemp,Display,TEXT("SARO owned placement: %s"),*Line);
  if (!Check(bOwnedReady,TEXT("all four native owned props are placed and preserve eight live paper bounds before physical corridor/save cohorts"))) return;
  const bool bBay = M->Phones && M->SaroNight && S47SaroInterior::ContainsPassingBay(M->Web,M->Player->Pos) &&
   M->Web->Obj(TEXT("annex.interior")).Shown() && M->Space == TEXT("room") && M->Phones->ArcWhere() == TEXT("annex") &&
   M->SaroNight->DarkWhere() == TEXT("away") && !M->Office->Contains(M->Player->Pos);
  if (!Check(bBay && S47J::Bool(M->Office->Save(),TEXT("entered")) == S47J::Bool(S47J::Obj(T.GoodOffice,TEXT("office")),TEXT("entered")),
   TEXT("physical return walk reaches clear passing bay with annex visible, room audio, annex phone/away night and unchanged actual entry fact"))) return;
  Shot(TEXT("interior-passing-bay-physical"));
  Path({FVector(.47,0,6.2),FVector(-1.2,0,6.2),FVector(-5.4,0,6.2)}); Stage(810,0); return;
 }
 if (T.Stage == 810)
 {
  if (!Walk()) return;
  if (!Check(M->Web->Obj(TEXT("annex.interior")).Shown() && M->Space==TEXT("room") &&
   M->Phones->ArcWhere()==TEXT("annex") && !M->Office->Contains(M->Player->Pos),
   TEXT("actual Walker reaches west corridor beyond control-room door without losing annex/audio"))) return;
  Path({FVector(-1.2,0,6.2),FVector(-.8,0,5.4),FVector(1.4,0,5.4)}); Stage(811,0); return;
 }
 if (T.Stage == 811)
 {
  if (!Walk()) return;
  if (!Check(M->Web->Obj(TEXT("annex.interior")).Shown() && M->Space==TEXT("room") &&
   M->Phones->ArcWhere()==TEXT("annex") && !M->Office->Contains(M->Player->Pos),
   TEXT("actual Walker reaches east corridor clear of retained vending collision and control door"))) return;
  Shot(TEXT("interior-corridor-east-physical"));
  Path({FVector(.47,0,5.4),FVector(.47,0,6.8),FVector(.47,0,7.8)}); Stage(812,0); return;
 }
 if (T.Stage == 812)
 {
  if (!Walk()) return;
  if (!Check(M->Office->DoorOpen() && M->Office->Use(TEXT("office:door")) && S47J::Bool(M->Office->Save(),TEXT("doorOpen")),
   TEXT("nearby real office close action is refused while player occupies inward door sweep"))) return;
  Stage(802,.35); return;
 }
 if (T.Stage == 802)
 {
  // Only pose and the missing layout annotation are fixtures. Office, reads and
  // story come from the two snapshots physically earned/captured above.
  auto RunCohorts = [&]() -> bool
  {
   FS47SaveStore PreviousStore = M->Flow->Saves;
   ON_SCOPE_EXIT { M->Flow->Saves = MoveTemp(PreviousStore); };
   auto Equal = [](const S47J::FObj& A, const S47J::FObj& B)
   {
    if (!A || !B) return A.IsValid() == B.IsValid();
    const FJsonValueObject Left(A), Right(B); return FJsonValue::CompareEqual(Left,Right);
   };
   auto Clear = [&](const FVector& P)
   {
    const double R = M->Player->Radius;
    if (!M->Player->Walkable(P.X,P.Z)) return false;
    for (const FS47Rect& C : M->Web->Colliders()) if (C.bEnabled && P.X+R>C.MinX && P.X-R<C.MaxX && P.Z+R>C.MinZ && P.Z-R<C.MaxZ) return false;
    return true;
   };
   struct FCohort { const TCHAR* Name; double X,Z; bool bMoved,bLocked; };
   const bool bOwnedBox = M->Web->Colliders().ContainsByPredicate([](const FS47Rect& R)
    { return R.Id==TEXT("office:ownedOpenBox") && R.bEnabled; });
   const FCohort Cohorts[] = {
    {TEXT("legacy-valid-front"),1.2,7.2,true,false},
    {TEXT("legacy-valid-rear"),.35,8.8,false,false},
    {TEXT("legacy-valid-door-sweep"),.1,8.4,true,false},
    {TEXT("legacy-valid-private-shelf"),-.2,8.7,bOwnedBox,false},
    {TEXT("legacy-valid-desk-side"),1.49,9.8,true,false},
    {TEXT("legacy-invalid-clipped"),1.2,6.9,true,false},
    {TEXT("legacy-locked-corridor"),.45,5.8,false,true}
   };
   for (const FCohort& C : Cohorts)
   {
    auto Input = S47J::Clone(C.bLocked ? T.InteriorLocked : T.GoodOffice);
    auto Office = S47J::Obj(Input,TEXT("office")), Pose = S47J::Obj(Input,TEXT("pose"));
    if (!Check(Office && Pose,TEXT("interior cohort retains actual snapshot objects: ")+FString(C.Name))) return false;
    Office->RemoveField(TEXT("interiorLayout"));
    S47J::Set(Pose,TEXT("x"),C.X); S47J::Set(Pose,TEXT("z"),C.Z); S47J::Set(Pose,TEXT("yaw"),.37); S47J::Set(Pose,TEXT("pitch"),-.18);
    UE_LOG(LogS47T02Test,Display,TEXT("S47: interior restore snapshot fixture %s at %.3f %.3f; pose/layout only, not player-input evidence; clipped cohort was not historically walkable"),C.Name,C.X,C.Z);
    FVector FirstPosition = FVector::ZeroVector;
    for (int32 Round=0;Round<2;++Round)
    {
     const FString Name = FString::Printf(TEXT("interior-%s-round%d"),C.Name,Round+1), Dir = T.Root/Name;
     const auto SourceOffice = S47J::Clone(S47J::Obj(Input,TEXT("office"))), SourcePose = S47J::Obj(Input,TEXT("pose"));
     const auto SourceCase = S47J::Clone(S47J::Obj(Input,TEXT("case")));
     FS47SaveStore Writer(Dir); Writer.Init(); FS47SaveCard Card; Card.Chapter=TEXT("SARO interior isolated restore"); Card.Place=TEXT("Operations/corridor"); Card.Clock=M->Story->ClockText(true);
     FString Error; const auto Meta=Writer.Save(1,TEXT("manual"),0,Input,Card,Error);
     if (!Check(Meta.IsSet(),TEXT("actual isolated interior Writer: ")+Name+TEXT(" ")+Error)) return false;
     FS47SaveStore Reader(Dir); Reader.Init(); const auto Disk=Reader.Load(Meta->Id); ++T.StoreReads;
     if (!Check(Disk && Equal(S47J::Obj(Disk,TEXT("office")),SourceOffice),TEXT("fresh interior Reader preserves source facts: ")+Name)) return false;
     FFileHelper::SaveStringToFile(S47J::Write(Disk,true),*(T.Root/(Name+TEXT(".loaded.json"))));
     // Use the real title/Continue path with its own case1, only in this isolated
     // store. No engine tick or frame returns before developer case0 is restored.
     M->Flow->Saves=MoveTemp(Reader); M->Flow->ToTitle();
     const auto Continue=M->Flow->Saves.ContinueSave();
     if (!Check(M->Mode==US47Main::EMode::Title && Continue && Continue->Id==Meta->Id && M->Flow->Saves.Dir==Dir,
      TEXT("actual title Continue selects only isolated cohort: ")+Name)) return false;
     if (!Check(M->Flow->OpenTestView(TEXT("continue")) && M->Mode==US47Main::EMode::Play && M->Flow->CaseId==1 && !M->Flow->bStarting,
      TEXT("actual title/Continue production OpenSave completed: ")+Name)) return false;
     ++T.Loads; ++T.InteriorContinues;
     const FVector Actual=M->Player->Pos;
     const auto After=M->Flow->GameState(), AfterOffice=S47J::Clone(S47J::Obj(After,TEXT("office")));
     auto ExpectedOffice=S47J::Clone(SourceOffice); ExpectedOffice->RemoveField(TEXT("interiorLayout")); AfterOffice->RemoveField(TEXT("interiorLayout"));
     const bool bSameView=FMath::IsNearlyEqual(M->Player->Yaw,S47J::Num(SourcePose,TEXT("yaw")),1e-6) && FMath::IsNearlyEqual(M->Player->Pitch,S47J::Num(SourcePose,TEXT("pitch")),1e-6);
     const bool bSamePosition=FMath::Abs(Actual.X-S47J::Num(SourcePose,TEXT("x")))<1e-6 && FMath::Abs(Actual.Z-S47J::Num(SourcePose,TEXT("z")))<1e-6;
     const bool bExpectedPose=Round==0 ? (C.bMoved ? !bSamePosition && M->Office->Contains(Actual) : bSamePosition) : Actual.Equals(FirstPosition,1e-6);
     const auto ActualDocs=InteriorDocs(), ExpectedDocs=C.bLocked?T.InteriorLockedDocs:T.InteriorEarnedDocs;
     const bool bClear=Clear(Actual), bOffice=Equal(AfterOffice,ExpectedOffice), bCase=Equal(S47J::Obj(After,TEXT("case")),SourceCase), bDocs=Equal(ActualDocs,ExpectedDocs);
     const bool bLayout=S47J::Num(S47J::Obj(After,TEXT("office")),TEXT("interiorLayout"))==2;
     auto Receipt=S47J::New(); S47J::Set(Receipt,TEXT("cohort"),Name); S47J::Set(Receipt,TEXT("fixturePose"),SourcePose);
     S47J::Set(Receipt,TEXT("actualPose"),S47J::Obj(After,TEXT("pose"))); S47J::Set(Receipt,TEXT("sourceOffice"),SourceOffice); S47J::Set(Receipt,TEXT("restoredOffice"),S47J::Obj(After,TEXT("office")));
     S47J::Set(Receipt,TEXT("sourceCase"),SourceCase); S47J::Set(Receipt,TEXT("restoredCase"),S47J::Obj(After,TEXT("case")));
     S47J::Set(Receipt,TEXT("sourceDocs"),ExpectedDocs); S47J::Set(Receipt,TEXT("restoredDocs"),ActualDocs);
     auto Checks=S47J::New(); S47J::Set(Checks,TEXT("colliderClear"),bClear); S47J::Set(Checks,TEXT("sameView"),bSameView); S47J::Set(Checks,TEXT("expectedPose"),bExpectedPose);
     S47J::Set(Checks,TEXT("sameOffice"),bOffice); S47J::Set(Checks,TEXT("sameCase"),bCase); S47J::Set(Checks,TEXT("sameDocs"),bDocs); S47J::Set(Checks,TEXT("layout2"),bLayout); S47J::Set(Receipt,TEXT("checks"),Checks);
     S47J::Set(Receipt,TEXT("status"),bClear&&bSameView&&bExpectedPose&&bOffice&&bCase&&bDocs&&bLayout?TEXT("PASS_DISK_TITLE_CONTINUE_FIXTURE"):TEXT("FAIL_DISK_TITLE_CONTINUE_FIXTURE"));
     FFileHelper::SaveStringToFile(S47J::Write(Receipt,true),*(T.Root/(Name+TEXT(".restore.json"))));
     UE_LOG(LogS47T02Test,Display,TEXT("S47: interior restore subchecks %s clear=%d view=%d pose=%d office=%d case=%d docs=%d layout=%d actual=(%.9f,%.9f) yaw=%.9f/%.9f pitch=%.9f/%.9f"),
      *Name,bClear,bSameView,bExpectedPose,bOffice,bCase,bDocs,bLayout,Actual.X,Actual.Z,M->Player->Yaw,S47J::Num(SourcePose,TEXT("yaw")),M->Player->Pitch,S47J::Num(SourcePose,TEXT("pitch")));
     if (!Check(bClear,TEXT("interior restored capsule is walkable and clear of every enabled collider: ")+Name) ||
      !Check(bSameView,TEXT("interior restored yaw/pitch unchanged: ")+Name) || !Check(bExpectedPose,TEXT("interior restore uses expected local migration or unchanged/idempotent pose: ")+Name) ||
      !Check(bOffice,TEXT("interior preserves all Office facts except layout annotation: ")+Name) || !Check(bCase,TEXT("interior preserves full chapter/night case: ")+Name) ||
      !Check(bDocs,TEXT("interior preserves actual acquired Dale documents: ")+Name) || !Check(bLayout,TEXT("interior migration annotation is layout2: ")+Name)) return false;
     if (Round==0) FirstPosition=Actual;
     Input=S47J::Clone(After);
    }
   }
   return true;
  };
  if (!RunCohorts()) return;
  US47Flow::FStart Original; Original.State=S47J::Clone(T.GoodOffice); Original.CaseId=0; Original.bDev=true; Original.Playtime=M->Playtime;
  M->Flow->StartGame(Original);
  if (!Check(M->Flow->bDevRun && M->Flow->CaseId==0 && M->Office->HasFieldSheet(),TEXT("interior cohort series restores original developer route and store"))) return;
  UE_LOG(LogS47T02Test,Display,TEXT("S47: SARO interior contract PASS COMPLETE: physical bay and west/east corridor return, closing sweep action, seven pose/layout fixtures, fourteen actual isolated Writer/freshReader/title/Continue restores; developer route resumed; human input UNVERIFIED"));
  Aim(FVector(.45,1.2,9.65)); Stage(95,.35); return;
 }
 if (T.Stage == 95)
 {
  Shot(TEXT("early-office-restored")); Stage(96,.35); return;
 }
 if (T.Stage == 96)
 {
  // Screenshot-only camera fixture after the verified physical route and restore.
  // This position is not counted as Walker movement or player-input evidence.
  M->Player->Place(.35,7.35,0); Aim(FVector(.45,1.2,9.65));
  UE_LOG(LogS47T02Test, Display, TEXT("S47: office overview camera fixture after completed physical/disk test, not extra walking evidence"));
  Stage(97,.35); return;
 }
 if (T.Stage == 97)
 {
  Shot(TEXT("office-room-overview")); Stage(98,.5); return;
 }
 if (T.Stage == 98)
 {
  auto Result = S47J::New(); S47J::Set(Result, TEXT("status"), T.bArrivalOffice ? TEXT("PASS_ARRIVAL_OFFICE_PHYSICAL_AND_DISK") : TEXT("PASS_EARLY_OFFICE_PHYSICAL_AND_DISK")); S47J::Set(Result, TEXT("checks"), T.Checks); S47J::Set(Result, TEXT("walkedMetres"), T.WalkDistance); S47J::Set(Result, TEXT("freshStoreFullFlowRestores"), T.Loads);
  S47J::Set(Result,TEXT("saroInteriorContract"),T.bSaroInterior); S47J::Set(Result,TEXT("interiorTitleContinueCohorts"),T.InteriorContinues);
  FFileHelper::SaveStringToFile(S47J::Write(Result,true), *(T.Root/TEXT("result.json")));
  T.bDone = true; M->Player->TestMove.Reset();
  UE_LOG(LogS47T02Test, Display, TEXT("S47: early office PASS COMPLETE: %d checks, real key/drawer/door/paper interaction, %.2f m Walker, fresh disk/full Flow restore, no premature departure or VHS credit; artifacts %s"), T.Checks, T.WalkDistance, *T.Root);
  FPlatformMisc::RequestExit(false); return;
 }
 if (T.Stage == 90)
 {
  auto Bad=S47J::Clone(T.GoodOffice);auto Office=S47J::Obj(Bad,TEXT("office"));
  // A bool is deliberately not a timestamp; keep its causal fieldSheet bit true.
  S47J::Set(Office,TEXT("fieldSheetAt"),true);S47J::Set(Office,TEXT("hasFieldSheet"),false);
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract negative snapshot: actual acquired field sheet with bool acquisition timestamp; expected derived annotation false"));
  if(!SaveLoad(TEXT("bad-office-sheet-time"),Bad,false))return;
  if(!Check(!M->Office->HasFieldSheet() && !M->Evening->Pack.Flag(TEXT("office_evidence_taken")) && NoTransformerContext(),TEXT("full Flow refuses boolean field-sheet time and all dependent Office/T02 credit")))return;
  Stage(91,.3);return;
 }
 if (T.Stage == 91)
 {
  if(!SaveLoad(TEXT("office-restored-after-bad-time"),T.GoodOffice,false))return;
  if(!Check(M->Office->HasFieldSheet() && M->Evening->Pack.Flag(TEXT("office_evidence_taken")),TEXT("original genuine Office acquisition restores after rejected timestamp fixture")))return;
  Stage(92,.3);return;
 }
 if (T.Stage == 92)
 {
  auto Threshold=S47J::Clone(T.GoodOffice);auto Office=S47J::Obj(Threshold,TEXT("office"));auto Pose=S47J::Obj(Threshold,TEXT("pose"));
  S47J::Set(Office,TEXT("doorOpen"),false);S47J::Set(Pose,TEXT("x"),.45);S47J::Set(Pose,TEXT("z"),6.6);
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract negative snapshot: already unlocked closed Office threshold at (.45,6.6); restore may reopen doorway only"));
  if(!SaveLoad(TEXT("closed-unlocked-office-threshold"),Threshold,false))return;
  const auto Restored=M->Office->Save(),Original=S47J::Obj(T.GoodOffice,TEXT("office"));bool SameFacts=true;
  for(const TCHAR* Key:{TEXT("key"),TEXT("unlocked"),TEXT("everOpened"),TEXT("entered"),TEXT("deskSeen"),TEXT("indexRead"),TEXT("clippingsRead"),TEXT("noteRead"),TEXT("contactRead"),TEXT("fieldSheet"),TEXT("hasFieldSheet"),TEXT("departed")})
   SameFacts &= S47J::Bool(Restored,Key)==S47J::Bool(Original,Key);
  if(!Check(M->Office->DoorOpen() && SameFacts && S47J::Num(Restored,TEXT("fieldSheetAt"))==S47J::Num(Original,TEXT("fieldSheetAt")),TEXT("closed unlocked threshold restores exact pose by reopening only, retaining original acquisition and departure facts")))return;
  Stage(93,.3);return;
 }
 if (T.Stage == 93)
 {
  if(!SaveLoad(TEXT("office-restored-after-threshold"),T.GoodOffice,false))return;
  Stage(10,.7);return;
 }
 if (T.Stage == 10)
 {
  // P04/P05/Nora and chapter-three start are explicit existing-story fixtures.
  C2->S.bP04 = C2->S.bP05 = C2->S.bCalled = true; C2->S.Stage = TEXT("complete");
  if (T.bNative)
  {
   UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 native route fixture: existing P04/P05/Nora prerequisites only; physical Office departure card and production ChapterBreak remain required"));
   Path({FVector(.40,0,8.1),FVector(.45,0,7.4),FVector(.45,0,7.3)}); Stage(101,0); return;
  }
  C3->BeginChapter3(nullptr);
  UE_LOG(LogS47T02Test, Display, TEXT("S47: T02 contract fixture: existing P04/P05/Nora prerequisites and BeginChapter3; no claim of playing their story or driving the highway"));
  M->World->PlaceTruck(TEXT("diner")); M->World->Enter(TEXT("diner")); M->Player->Place(7885,525.8,PI); Aim(Target(TEXT("substation:journal")));
  UE_LOG(LogS47T02Test, Display, TEXT("S47: T02 contract fixture: field/vehicle placement at the actual control front; field controls and walks are physical thereafter"));
  Stage(11,.7); return;
 }
 if (T.Stage == 11)
 {
  if (!Check(M->Substation->FollowupAvailable() && !M->Substation->HasNormalResult(),TEXT("field follow-up is available while old case has no fabricated normal baseline"))) return;
  if (!Pick(TEXT("substation:journal"))) return; M->UI->Close(true); Aim(Target(TEXT("substation:service"))); Stage(12); return;
 }
 if (T.Stage == 12) { if (!Pick(TEXT("substation:service"))) return; Stage(13,3.6); return; }
 if (T.Stage == 13) { Aim(Target(TEXT("substation:lamp"))); Stage(14); return; }
 if (T.Stage == 14)
 {
  if (!Pick(TEXT("substation:lamp"))) return; M->UI->Close(true);
  if (!Check(M->Substation->HasNormalResult(),TEXT("actual journal plus three-second normal control and actual lamp create a dated baseline"))) return;
  T.Baseline = M->Substation->NormalCompletionStamp();
  const FVector Ref = M->Substation->FollowupReferencePosition();
  Path({FVector(7885,0,520.8),FVector(Ref.X,0,Ref.Z)}); Stage(15,0); return;
 }
 if (T.Stage == 15) { if (!Walk()) return; Aim(Target(TEXT("substation:t02:reference"))); Stage(16); return; }
 if (T.Stage == 16)
 {
  if (!Pick(TEXT("substation:t02:reference"))) return;
  RefAim(); Stage(17,.6); return;
 }
 if (T.Stage == 17)
 {
  FString Why;
  if (!M->Substation->ReferenceMatched())
  {
   if (T.Elapsed - T.Since > 3)
   {
    const bool ViewOk = FS47T02Photo::CheckView(M,Why);
    T.PendingFailure = TEXT("actual eight-feature framing/LOS did not match: ") + (ViewOk ? FString(TEXT("world record rejected")) : Why);
    M->FieldCamera->Sync();
    const FVector Pos=M->Player->Pos, Eye=S47Three::ToThree(M->Player->EyeUE());
    const FRotator Rot=M->FieldCamera->PhotoRot;
    UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 reference diagnostic actual pose=(%.6f,%.6f,%.6f) floor=%.6f eye=(%.6f,%.6f,%.6f) yaw=%.6f pitch=%.6f photoRot=(%.6f,%.6f,%.6f)"),
     Pos.X,Pos.Y,Pos.Z,M->Player->FloorY,Eye.X,Eye.Y,Eye.Z,M->Player->Yaw,M->Player->Pitch,Rot.Pitch,Rot.Yaw,Rot.Roll);
    auto Diag=S47J::New(); S47J::Set(Diag,TEXT("status"),TEXT("REFERENCE_VIEW_FAILED_ACTUAL_DIAGNOSTIC"));
    S47J::Set(Diag,TEXT("pose"),TArray<double>{Pos.X,Pos.Y,Pos.Z});S47J::Set(Diag,TEXT("floorY"),M->Player->FloorY);
    S47J::Set(Diag,TEXT("eye"),TArray<double>{Eye.X,Eye.Y,Eye.Z});S47J::Set(Diag,TEXT("yaw"),M->Player->Yaw);S47J::Set(Diag,TEXT("pitch"),M->Player->Pitch);
    S47J::Set(Diag,TEXT("photoRotation"),TArray<double>{Rot.Pitch,Rot.Yaw,Rot.Roll});S47J::Set(Diag,TEXT("reason"),T.PendingFailure);
    TArray<TSharedPtr<FJsonValue>> Rows;
    const auto Occluders=M->Substation->FollowupOccluders();
    for(const auto& Feature:M->Substation->FollowupFeatures())
    {
     const auto P=M->FieldCamera->Project(Feature.Value);TArray<FString> Blockers;TArray<double> Nodes;
     for(const auto& Occluder:Occluders)
      if(!M->FieldCamera->ClearLine(Feature.Value,TArray<FS47Obj>{Occluder})) {Blockers.Add(Occluder.Name());Nodes.Add(Occluder.I);}
     UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 reference feature %s world=(%.6f,%.6f,%.6f) uv=(%.6f,%.6f) front=%d blockers=[%s]"),
      *Feature.Key,Feature.Value.X,Feature.Value.Y,Feature.Value.Z,P.U,P.V,int32(P.bFront),*FString::Join(Blockers,TEXT(" | ")));
     auto Row=S47J::New();S47J::Set(Row,TEXT("id"),Feature.Key);S47J::Set(Row,TEXT("world"),TArray<double>{Feature.Value.X,Feature.Value.Y,Feature.Value.Z});
     S47J::Set(Row,TEXT("uv"),TArray<double>{P.U,P.V});S47J::Set(Row,TEXT("front"),P.bFront);S47J::Set(Row,TEXT("blockingOccluders"),Blockers);S47J::Set(Row,TEXT("blockingNodeIndices"),Nodes);
     Rows.Add(MakeShared<FJsonValueObject>(Row));
    }
    Diag->SetArrayField(TEXT("features"),Rows);S47J::Set(Diag,TEXT("occluderCount"),Occluders.Num());
    FFileHelper::SaveStringToFile(S47J::Write(Diag,true),*(T.Root/TEXT("reference-failed.json")));
    Shot(TEXT("reference-failed"));Stage(917,.8);
   }
   return;
  }
  if (!Check(!M->Substation->FollowupAttemptId().IsEmpty() && S47J::Obj(Snapshot(),TEXT("referenceView")),TEXT("actual reference view records eight framed physical features, not paper-open credit"))) return;
  Path({FVector(7880,0,520.5),FVector(7886.15,0,525.0)}); Stage(18,0); return;
 }
 if (T.Stage == 18) { if (!Walk()) return; Aim(Target(TEXT("substation:t02:record"))); Stage(19); return; }
 if (T.Stage == 19) { if (!Pick(TEXT("substation:t02:record"))) return; Stage(190); return; }
 if (T.Stage == 190)
 {
  const auto* Scene=M->Evening->Pack.CurrentScene();
  if(!Check(M->UI->Modal() && Scene && Scene->Id==TEXT("T02_CHECKS"),TEXT("physical report plate opens the actual T02 checks modal")))return;
  M->UI->Close();
  if(!Check(!M->UI->Modal() && M->Evening->Pack.CurrentScene() && M->Evening->Pack.CurrentScene()->Id==TEXT("T02_CHECKS"),TEXT("ordinary close retains the current T02 review for later physical reopening")))return;
  Aim(Target(TEXT("substation:t02:record")));Stage(191);return;
 }
 if (T.Stage == 191)
 {
  if(!Pick(TEXT("substation:t02:record")) || !Check(M->UI->Modal(),TEXT("actual report-plate use reopens the closed T02 review modal")))return;
  Stage(20);return;
 }
 if (T.Stage == 20)
 {
  auto& P = M->Evening->Pack;
  if (P.CurrentLine()) { AdvanceDialogue(); return; }
  if (!Check(P.CurrentScene() && P.CurrentScene()->Id == TEXT("T02_CHECKS") && !P.Select(TEXT("T02_CH_VERIFIED")),TEXT("real UI-only checks refuse verified choice before physical controls and photo"))) return;
  if (!P.Select(TEXT("T02_CH_UNRESOLVED"))) { Fail(TEXT("ordinary unresolved UI choice missing")); return; }
  M->UI->Close(true); M->Evening->ShowDialogue(); Stage(21); return;
 }
 if (T.Stage == 21)
 {
  if (!AdvanceDialogue()) return;
  if (!Check(M->Evening->Pack.Flag(TEXT("field_followup_unresolved")) && !M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")),TEXT("actual unresolved choice retains limits without confirmed-anomaly credit"))) return;
  if(T.bUnresolved)
  {
   if(!Check(M->Substation->ReferenceMatched() && !S47J::Bool(Snapshot(),TEXT("isolation")) && !S47J::Bool(Snapshot(),TEXT("backup")) && !S47J::Bool(Snapshot(),TEXT("indicator")) && !S47J::Bool(Snapshot(),TEXT("lamp")) && !S47J::Obj(Snapshot(),TEXT("photo")) && !M->Substation->FollowupVerified(),TEXT("unresolved variant retains actual baseline/reference but has no later control or photograph credit")))return;
   if(!PlaceAtSaroFolder())return;Stage(37,.4);return;
  }
  Aim(Target(TEXT("substation:t02:isolation"))); Stage(22); return;
 }
 if (T.Stage == 22) { if (!Pick(TEXT("substation:t02:isolation"))) return; Aim(FVector(7886.05,.38,527.10)); Stage(23,.2); return; }
 if (T.Stage == 23)
 {
  if (!S47J::Bool(Snapshot(),TEXT("isolation"))) { if (T.Elapsed-T.Since>8) Fail(TEXT("visible isolation test did not complete with independent OPEN response")); return; }
  if (!Check(S47J::Bool(Snapshot(),TEXT("isolationResponseSeen")) && !S47J::Bool(Snapshot(),TEXT("backup")),TEXT("separate three-second isolation completes only with its actual visible response"))) return;
  if (!SaveLoad(TEXT("field-isolation"),M->Flow->GameState(),false)) return;
  Aim(Target(TEXT("substation:t02:auxiliary"))); Stage(24,.5); return;
 }
 if (T.Stage == 24) { if (!Pick(TEXT("substation:t02:auxiliary"))) return; Aim(FVector(7886.05,.38,527.10)); Stage(25,.2); return; }
 if (T.Stage == 25)
 {
  if (!S47J::Bool(Snapshot(),TEXT("backup"))) { if(T.Elapsed-T.Since>8) Fail(TEXT("independent auxiliary pulse and dark return did not complete")); return; }
  if (!Check(S47J::Bool(Snapshot(),TEXT("auxiliaryPulseSeen")),TEXT("independent auxiliary pulse was visibly observed before its dark result"))) return;
  Aim(Target(TEXT("substation:t02:indicator"))); Stage(26); return;
 }
 if (T.Stage == 26)
 {
  if (!Pick(TEXT("substation:t02:indicator")) || !Check(S47J::Bool(Snapshot(),TEXT("indicator")),TEXT("real panel ray compares the three current native responses"))) return;
  const FVector Ref = M->Substation->FollowupReferencePosition(); Path({FVector(7880,0,520.5),FVector(Ref.X,0,Ref.Z)}); Stage(27,0); return;
 }
 if (T.Stage == 27) { if(!Walk())return; Aim(Target(TEXT("substation:lamp"))); Stage(28,.2); return; }
 if (T.Stage == 28)
 {
  if (!S47J::Bool(Snapshot(),TEXT("lamp"))) { if(T.Elapsed-T.Since>10) Fail(TEXT("actual full bright/dim lamp sequence was not observed from reference")); return; }
  if (!Check(S47J::Num(Snapshot(),TEXT("lampViewSeconds"))>=3.5 && S47J::Num(Snapshot(),TEXT("lampMax"))-S47J::Num(Snapshot(),TEXT("lampMin"))>=.25 && !M->Substation->FollowupVerified(),TEXT("actual 3.5-second visible lamp sequence still needs its own photograph"))) return;
  T.AlbumCount = M->Night->Album->Data.Shots.Num(); T.AlbumLeft = M->Night->Album->Left(); T.FieldFilmBefore = S47J::Write(C3->S.Save());
  RefAim(); if(!M->FieldCamera->bRaised) C1->ToggleCamera(); Stage(29,.4); return;
 }
 if (T.Stage == 29)
 {
  C1->Shutter(); const auto P = S47J::Obj(Snapshot(),TEXT("photo")); T.FirstPhoto = S47J::Str(P,TEXT("id"));
  if (!Check(M->Substation->FollowupVerified() && T.FirstPhoto.StartsWith(TEXT("t02-")) && S47J::Str(P,TEXT("url")).StartsWith(TEXT("data:image/jpeg;base64,")) &&
   M->Night->Album->Data.Shots.Num()==T.AlbumCount && M->Night->Album->Left()==T.AlbumLeft && S47J::Write(C3->S.Save())==T.FieldFilmBefore,TEXT("actual C1 camera shutter retains genuine reserved T02 photograph without using album or STATION01 film"))) return;
  TArray<uint8> Jpeg; const FString Url = S47J::Str(P,TEXT("url"));
  if (!FBase64::Decode(Url.Mid(Url.Find(TEXT(","))+1),Jpeg) || !FFileHelper::SaveArrayToFile(Jpeg,*(T.Root/TEXT("photo-first.jpg")))) { Fail(TEXT("could not export actual photographed JPEG bytes")); return; }
  if (T.bNative)
  {
   T.LatestPhoto = T.FirstPhoto; Lower(); T.GoodPhoto = S47J::Clone(M->Flow->GameState());
   if (!SaveLoad(TEXT("observed-reference"),T.GoodPhoto,true)) return;
   const FVector Ref=M->Substation->FollowupReferencePosition();
   Path({FVector(Ref.X,0,Ref.Z),FVector(7880,0,520.5),FVector(7886.15,0,525.0)}); Stage(32,0); return;
  }
  // Only the old free-album quota is a fixture. These are 36 references to THIS actual
  // rendered JPEG, not a claim of 36 exposures, and never Office/T02 fact setters.
  M->Night->Album->Data.Shots.Reset();
  for(int32 I=0;I<FS47Album::ROLL;++I) { FS47AlbumShot S; S.Id=FString::Printf(TEXT("quota-fixture-%d"),I); S.Url=Url; S.Clock=M->Story->Clock(); S.Place=TEXT("T02 quota fixture"); S.Text=TEXT("Explicit quota-only fixture using actual T02 exposure pixels"); M->Night->Album->Data.Shots.Add(MoveTemp(S)); }
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract fixture: free album set to 36 references to genuine first T02 JPEG; no claim of 36 captures"));
  Stage(30,.35); return;
 }
 if (T.Stage == 30)
 {
  C1->Shutter(); Lower(); const auto P = S47J::Obj(Snapshot(),TEXT("photo")); T.LatestPhoto = S47J::Str(P,TEXT("id"));
  if(!Check(M->Substation->FollowupVerified() && T.LatestPhoto!=T.FirstPhoto && M->Night->Album->Left()==0 && M->Night->Album->Data.Shots.Num()==36 && S47J::Write(C3->S.Save())==T.FieldFilmBefore,TEXT("genuine retake gets its own ID with full free album and unchanged STATION01 film")))return;
  T.GoodPhoto = S47J::Clone(M->Flow->GameState());
  if(!SaveLoad(TEXT("field-real-photo"),T.GoodPhoto,true))return;
  Stage(31,.5); return;
 }
 if (T.Stage == 31)
 {
  if(!SaveLoad(TEXT("missing-photo-file"),T.GoodPhoto,false,true))return;
  if(!Check(!M->Evening->Pack.Flag(TEXT("ctx_trafo_photo_available")) && !M->Substation->FollowupVerified(),TEXT("actual missing photo file and stand-in cannot grant verified evidence")))return;
  if(!SaveLoad(TEXT("photo-restored-after-missing"),T.GoodPhoto,true))return;
  const FVector Ref=M->Substation->FollowupReferencePosition(); Path({FVector(Ref.X,0,Ref.Z),FVector(7880,0,520.5),FVector(7886.15,0,525.0)}); Stage(32,0); return;
 }
 if (T.Stage == 32) { if(!Walk())return; Aim(Target(TEXT("substation:t02:record"))); Stage(33); return; }
 if (T.Stage == 33) { if(!Pick(TEXT("substation:t02:record")))return; Stage(34); return; }
 if (T.Stage == 34)
 {
  auto& P=M->Evening->Pack; if(P.CurrentLine()){AdvanceDialogue();return;}
  if(!Check(P.CurrentScene() && P.CurrentScene()->Id==TEXT("T02_CHECKS") && P.Select(TEXT("T02_CH_VERIFIED")),TEXT("actual verified UI choice appears only after real controls and independent photograph")))return;
  M->UI->Close(true); M->Evening->ShowDialogue(); T.PendingVerified=S47J::Clone(M->Flow->GameState());
  if (T.bNative) { Stage(36); return; }
  auto Bad=S47J::Clone(T.PendingVerified); auto Local=S47J::Obj(S47J::Obj(Bad,TEXT("substation")),TEXT("t02")); S47J::Set(Local,TEXT("auxiliaryPulseSeen"),false);
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract negative snapshot: pending VERIFIED with actual auxiliary world response removed; not production fact assignment"));
  if(!SaveLoad(TEXT("bad-world-pending-verified"),Bad,false))return;
  Stage(35,.4); return;
 }
 if (T.Stage == 35)
 {
  if(M->Evening->Pack.CurrentLine()){AdvanceDialogue();return;}
  if(!Check(!M->Substation->FollowupVerified() && !M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")),TEXT("restored pending VERIFIED cannot complete after its world proof is invalidated")))return;
  M->UI->Close(true);
  auto NoSheet=S47J::Clone(T.PendingVerified);auto Office=S47J::Obj(NoSheet,TEXT("office"));
  S47J::Set(Office,TEXT("fieldSheet"),false);S47J::Set(Office,TEXT("hasFieldSheet"),false);
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract negative snapshot: real pending VERIFIED with Office field-sheet possession removed; all external transformer contexts must clear"));
  if(!SaveLoad(TEXT("no-office-pending-verified"),NoSheet,false))return;
  Stage(351,.4);return;
 }
 if (T.Stage == 351)
 {
  if(!Check(!M->Office->HasFieldSheet() && !M->Evening->Pack.Flag(TEXT("office_evidence_taken")) && NoTransformerContext() &&
   !M->Evening->Pack.CurrentScene() && M->Evening->Pack.GetPendingChoice().IsEmpty() && !M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")),TEXT("missing Office possession cancels pending VERIFIED and clears all five ctx flags without anomaly credit")))return;
  if(!SaveLoad(TEXT("valid-pending-verified"),T.PendingVerified,true))return;
  Stage(36,.4); return;
 }
 if (T.Stage == 36)
 {
  if(!AdvanceDialogue())return;
  if(!Check(M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")) && M->Substation->FollowupVerified() && M->Substation->NormalCompletionStamp()==T.Baseline,TEXT("actual verified scene completes with retained physical proof and unchanged historical baseline")))return;
  Aim(Target(TEXT("substation:t02:record")));Stage(360);return;
 }
 if(T.Stage==360)
 {
  if(!Pick(TEXT("substation:t02:record")) || !Check(M->UI->Modal() && !M->Evening->Pack.CurrentScene() && M->Substation->FollowupDocument(true).Image.StartsWith(TEXT("data:image/jpeg;base64,")),TEXT("physical report plate reads the permanent verified record with its genuine photograph after the authored scene")))return;
  Shot(TEXT("verified-read-back"));Stage(361,.5);return;
 }
 if(T.Stage==361)
 {
  M->UI->Close();
  if (T.bNative)
  {
   Path({FVector(7885,0,520.8),T.FieldEgress}); Stage(201,0); return;
  }
  if(!PlaceAtSaroFolder())return; Stage(37,.4); return;
 }
 if (T.Stage == 37)
 {
  T.FilingNotes=M->Story->Notes().Num(); T.FilingHistory=S47J::Arr(Snapshot(),TEXT("filedHistory")).Num();
  if(!Pick(TEXT("logbook")))return;M->UI->Close(true);
  const FString Resolution=T.bUnresolved?TEXT("unresolved"):TEXT("verified");
  if(!Check(M->Substation->FollowupFiledResolution()==Resolution && M->Story->Notes().Num()==T.FilingNotes+1 && S47J::Arr(Snapshot(),TEXT("filedHistory")).Num()==T.FilingHistory+1,
   TEXT("one real SARO folder ray files exactly one local record with resolution ")+Resolution))return;
  if(!Pick(TEXT("logbook")))return;M->UI->Close(true);
  if(!Check(M->Story->Notes().Num()==T.FilingNotes+1 && S47J::Arr(Snapshot(),TEXT("filedHistory")).Num()==T.FilingHistory+1,TEXT("reopening folder does not duplicate filing note or historical card")))return;
  const auto History=S47J::Arr(Snapshot(),TEXT("filedHistory"));
  if(History.IsEmpty()||!History.Last().IsValid()||History.Last()->Type!=EJson::Object){Fail(TEXT("physical filing has no actual historical paper record"));return;}
  T.FiledCard=S47J::Clone(History.Last()->AsObject());T.GoodFiled=S47J::Clone(M->Flow->GameState());
  auto HistoryState=S47J::New();HistoryState->SetArrayField(TEXT("history"),History);T.FiledHistoryBytes=S47J::Write(HistoryState);
  if(!SaveLoad(T.bUnresolved?TEXT("filed-unresolved-at-saro"):TEXT("filed-at-saro"),T.GoodFiled,!T.bUnresolved))return;
  if(!CheckFiledPaper(T.FiledCard,T.bNative?TEXT("native-filed-after-fresh-reader-no-world-load"):T.bUnresolved?TEXT("unresolved-filed-after-full-flow"):TEXT("verified-filed-after-full-flow")))return;
  if(T.bUnresolved)
  {
   const auto Trip=C3->TruckTrip(TEXT("saro"));
   if(!Check(!M->Substation->FollowupVerified() && !M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")) &&
    M->Substation->FollowupFiledResolution()==TEXT("unresolved") && !S47J::Obj(Snapshot(),TEXT("photo")) &&
    Trip && Trip->Label==TEXT("Drive to STATION 01") && !M->Inter->LabelOf(TEXT("truck")).IsEmpty(),TEXT("full Flow retains unresolved filing without photograph/anomaly and preserves the ordinary STATION 01 trip")))return;
  }
  Shot(T.bUnresolved?TEXT("filed-unresolved-at-saro"):TEXT("filed-at-saro"));
  if(!T.bNative&&!T.bUnresolved){Stage(370,.5);return;}
  Stage(38,1); return;
 }
 if(T.Stage==370)
 {
  // Preserve the original real filed source/card; remove only its actual image file.
  T.FilingNotes=M->Story->Notes().Num();T.FilingHistory=S47J::Arr(Snapshot(),TEXT("filedHistory")).Num();
  if(!SaveLoad(TEXT("filed-missing-photo-file"),T.GoodFiled,false,true))return;
  if(!CheckFiledPaper(T.FiledCard,TEXT("filed-missing-photo-after-full-flow")))return;
  const auto Photo=S47J::Obj(Snapshot(),TEXT("photo"));
  auto HistoryState=S47J::New();HistoryState->SetArrayField(TEXT("history"),S47J::Arr(Snapshot(),TEXT("filedHistory")));
  if(!Check(!M->Substation->FollowupVerified() && !M->Substation->FollowupFiled() &&
   !M->Evening->Pack.Flag(TEXT("ctx_trafo_photo_available")) && !M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked")) &&
   Photo && S47J::Bool(Photo,TEXT("unavailable")) && S47J::Str(Photo,TEXT("id"))==T.LatestPhoto &&
   M->Substation->FiledFollowupDocument(true).Image.IsEmpty() &&
   M->Story->Notes().Num()==T.FilingNotes && S47J::Write(HistoryState)==T.FiledHistoryBytes,
   TEXT("missing actual filed image invalidates current proof while retaining original paper/history and adding no filing note")))return;
  Aim(Target(TEXT("logbook")));Stage(371);return;
 }
 if(T.Stage==371)
 {
  if(!Pick(TEXT("logbook")))return;M->UI->Close();
  auto HistoryState=S47J::New();HistoryState->SetArrayField(TEXT("history"),S47J::Arr(Snapshot(),TEXT("filedHistory")));
  if(!Check(M->Story->Notes().Num()==T.FilingNotes && S47J::Arr(Snapshot(),TEXT("filedHistory")).Num()==T.FilingHistory &&
   S47J::Write(HistoryState)==T.FiledHistoryBytes && !M->Substation->FollowupVerified(),TEXT("actual folder use after missing filed photo cannot create a replacement filing, note or history")))return;
  if(!CheckFiledPaper(T.FiledCard,TEXT("filed-missing-photo-after-folder-use")))return;
  Shot(TEXT("filed-missing-photo-paper-retained"));Stage(372,.5);return;
 }
 if(T.Stage==372)
 {
  if(!SaveLoad(TEXT("filed-original-photo-restored"),T.GoodFiled,true))return;
  if(!CheckFiledPaper(T.FiledCard,TEXT("filed-original-photo-restored-full-flow")))return;
  Stage(38,.5);return;
 }
 if (T.Stage == 38)
 {
  if (T.bNative && !Check(T.Loads==0 && T.bOutSaro && T.bOutRoad && T.bBackRoad && T.bBackSaro &&
   T.OutMetres>500 && T.BackMetres>500 && T.NativeWalkMetres>30,
   TEXT("native Office/Caliche/return journey has measured legs and walking without intermediate Flow restore"))) return;
  auto Result=S47J::New();S47J::Set(Result,TEXT("status"),TEXT("PASS_API_DISK_SCOPED"));S47J::Set(Result,TEXT("checks"),T.Checks);S47J::Set(Result,TEXT("freshStoreFullFlowRestores"),T.Loads);S47J::Set(Result,TEXT("walkedMetres"),T.WalkDistance);S47J::Set(Result,TEXT("photoId"),T.LatestPhoto);S47J::Set(Result,TEXT("humanInputVerified"),false);S47J::Set(Result,TEXT("fullNativeTravelVerified"),false);
  if (T.bNative)
  {
   S47J::Set(Result,TEXT("status"),TEXT("PASS_NATIVE_T02_CALICHE_ROUTE_SCOPED"));
   S47J::Set(Result,TEXT("nativeOutboundMetres"),T.OutMetres);S47J::Set(Result,TEXT("nativeReturnMetres"),T.BackMetres);
   S47J::Set(Result,TEXT("nativeWalkMetres"),T.NativeWalkMetres);S47J::Set(Result,TEXT("freshStoreReadsWithoutWorldLoad"),T.StoreReads);
   S47J::Set(Result,TEXT("continuousNativeCalicheJourney"),true);
  }
  if(T.bUnresolved)
  {
   S47J::Set(Result,TEXT("status"),TEXT("PASS_UNRESOLVED_API_DISK_SCOPED"));S47J::Set(Result,TEXT("resolution"),TEXT("unresolved"));
   S47J::Set(Result,TEXT("photographExposed"),false);S47J::Set(Result,TEXT("anomalyCredit"),false);
  }
  FFileHelper::SaveStringToFile(S47J::Write(Result,true),*(T.Root/TEXT("result.json")));
  T.bDone=true; M->Player->TestMove.Reset();
  if (T.bNative)
  {
   M->World->Autopilot.Reset();
   UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 native route PASS COMPLETE: %d checks, %d genuine writer/fresh-reader stores, %.1f outbound m, %.1f return m, %.1f walked m; no test placement or intermediate Flow load after START; ch2/P04/P05/Nora/initial corridor/target are fixtures; STATION01 chapter completion, manual Roam and human input UNVERIFIED; artifacts %s"),T.Checks,T.StoreReads,T.OutMetres,T.BackMetres,T.NativeWalkMetres,*T.Root);
   FPlatformMisc::RequestExit(false); return;
  }
  if(T.bUnresolved)
  {
   UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 unresolved contract PASS COMPLETE: %d API checks, %d actual writer/fresh-reader/full-Flow disk restores, %.2f m actual waypoint walking; real Office key/folder, normal baseline, matched reference, authored unresolved choice and deduplicated SARO filing; no later controls/photo/anomaly; chapter prerequisites and field/return placement are fixtures; native travel and human input UNVERIFIED; artifacts %s"),T.Checks,T.Loads,T.WalkDistance,*T.Root);
   FPlatformMisc::RequestExit(false);return;
  }
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 contract PASS COMPLETE: %d API checks, %d actual writer/fresh-reader/full-Flow disk restores, %.2f m actual waypoint walking; ch2/P04/P05/Nora/ch3 and field/return positions are fixtures; quota fixture reuses genuine photographed pixels; full native travel and human input UNVERIFIED; artifacts %s"),T.Checks,T.Loads,T.WalkDistance,*T.Root);
  FPlatformMisc::RequestExit(false);
  return;
 }

 // Native-only departure/travel branch. Source-controlled API fixtures above stay separate.
 if (T.Stage == 101) { if(!Walk())return; Aim(Target(TEXT("office:depart"))); Stage(102); return; }
 if (T.Stage == 102)
 {
  if(!Check(M->Office->WantsDeparture(),TEXT("actual Office departure card is available with real retained field sheet")) || !Pick(TEXT("office:depart")))return;
  Stage(103,.4); return;
 }
 if (T.Stage == 103)
 {
  if(M->Story->ChapterKey!=TEXT("ch3")||M->Story->IsCinematic()||M->ModalOpen())
  {if(T.Elapsed-T.Since>90)Fail(TEXT("production Office departure/roof wrapper/ChapterBreak did not finish"));return;}
  if(!Check(M->Office->DepartureReleased() && C3->bStarted,TEXT("actual departure card retained release before production chapter-three transition")))return;
  Path({FVector(.45,0,7.0),FVector(.45,0,6.2),FVector(.45,0,5.75),FVector(-2.4,0,5.75)}); Stage(104,0); return;
 }
 if (T.Stage == 104) {if(!Walk())return;Aim(Target(TEXT("doorSouth")));Stage(105);return;}
 if (T.Stage == 105)
 {
  if(!M->Web->IsDoorOpen(TEXT("doorSouth"))&&!Pick(TEXT("doorSouth")))return;
  Path({FVector(-2.4,0,5.3),FVector(-2.4,0,4.2),FVector(-2.4,0,3.5),FVector(-2.4,0,1.6),FVector(4.8,0,1.6)});Stage(106,.8);return;
 }
 if (T.Stage == 106) {if(!Walk())return;Aim(Target(TEXT("doorEast")));Stage(107);return;}
 if (T.Stage == 107)
 {
  if(!M->Web->IsDoorOpen(TEXT("doorEast"))&&!Pick(TEXT("doorEast")))return;
  Path({FVector(5.2,0,1.6),FVector(7.5,0,1.6),FVector(9.8,0,1.6),FVector(9.8,0,4),FVector(11.5,0,4),FVector(11.5,0,8),FVector(10.8,0,8)});Stage(108,.8);return;
 }
 if (T.Stage == 108) {if(!Walk())return;Aim(Target(TEXT("truck")));Stage(109);return;}
 if (T.Stage == 109)
 {
  if(!Check(M->World->TruckAt==TEXT("saro")&&!M->World->bCommuteVehicle&&!M->Inter->LabelOf(TEXT("truck")).IsEmpty(),TEXT("actual service truck and physical use range available after Office walk")) || !Pick(TEXT("truck"),FString(),false))return;
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 native route fixture: choose existing STATION01 trip target and stop along real Caliche track; steering remains native Autopilot"));
  M->World->Autopilot=10;M->World->DriveOut(false);Stage(110,.6);return;
 }
 if (T.Stage == 110)
 {
  if(!M->World->bDriving){Fail(TEXT("native outbound trip ended before actual Caliche approach"));return;}
  const auto* Truck=M->World->CabTruck();
  if(M->World->Leg==TEXT("road")&&Truck->Position.X<7950)M->World->Autopilot=4;
  if(M->World->Leg!=TEXT("road")||Truck->Position.X>7900)return;
  if(!Check(Truck->Position.Z>495&&Truck->Position.Z<530&&T.bOutSaro&&T.bOutRoad&&T.OutMetres>500,TEXT("native SARO/highway/track physically reaches actual Caliche front approach")))return;
  M->World->Autopilot=0;Stage(111,.1);return;
 }
 if (T.Stage == 111)
 {
  if(!M->World->bDriving||T.Elapsed-T.Since>15){Fail(TEXT("actual Caliche approach braking failed"));return;}
  if(FMath::Abs(M->World->DrivingSpeed())>.1||!M->World->CanGetOut())return;
  T.FieldTruck=M->World->CabTruck()->Position;
  if(!Check(FVector2D(T.FieldTruck.X-7885,T.FieldTruck.Z-525.8).Size()<28&&M->World->GetOutHere(),TEXT("normal stopped truck permits actual field-door egress near control front")))return;
  Stage(112,.65);return;
 }
 if (T.Stage == 112)
 {
  const auto Truck=M->World->TruckSave(),Pose=S47J::Obj(Truck,TEXT("pose"));
  if(!Check(!M->World->bDriving&&M->World->TruckAt==TEXT("diner")&&M->World->Area==TEXT("diner")&&Pose&&FMath::Abs(S47J::Num(Pose,TEXT("x"))-T.FieldTruck.X)<.02&&FMath::Abs(S47J::Num(Pose,TEXT("z"))-T.FieldTruck.Z)<.02,TEXT("normal Park retains physical truck pose and publishes actual Caliche foot area")))return;
  T.FieldEgress=M->Player->Pos;Path({FVector(7885,0,520.8),FVector(7885,0,525.8)});Stage(113,0);return;
 }
 if (T.Stage == 113) {if(!Walk())return;Aim(Target(TEXT("substation:journal")));Stage(11);return;}

 if (T.Stage == 201) {if(!Walk())return;Aim(Target(TEXT("dinerTruck")));Stage(202);return;}
 if (T.Stage == 202)
 {
  if(!Pick(TEXT("dinerTruck"),FString(),false))return;
  T.bReturn=true;M->World->Autopilot=10;M->World->DriveBack();Stage(203,.6);return;
 }
 if (T.Stage == 203) {if(M->World->bDriving)return;Stage(204,.65);return;}
 if (T.Stage == 204)
 {
  if(!Check(M->World->TruckAt==TEXT("saro")&&M->World->Area==TEXT("saro")&&M->World->TruckOnLot()&&T.bBackRoad&&T.bBackSaro&&T.BackMetres>500&&
   M->World->Odometer()>T.OdoStart+(T.OutMetres+T.BackMetres)/1609.344-.05,TEXT("actual reverse track/highway/pad trip parks normally with measured odometer gain")))return;
  const auto& Body=M->Player->TruckBox;if(!Body||Body->Area!=TEXT("saro")){Fail(TEXT("actual return truck collider was not published"));return;}
  const bool West=M->Player->Pos.X<Body->X;const double Lane=West?7.35:11.5;
  const TArray<FVector> Exit={FVector(Lane,0,M->Player->Pos.Z),FVector(Lane,0,4),FVector(9.8,0,4),FVector(9.8,0,1.6),FVector(7.5,0,1.6)};
  FVector From=M->Player->Pos;
  for(const auto& To:Exit)
  {const int32 Samples=FMath::Max(1,FMath::CeilToInt(FVector2D(To.X-From.X,To.Z-From.Z).Size()/.15));for(int32 I=0;I<=Samples;++I){const FVector At=FMath::Lerp(From,To,double(I)/Samples);if(!M->Player->Walkable(At.X,At.Z)||M->Player->InTruck(At.X,At.Z)){Fail(TEXT("actual return egress side has no sampled walkable path clear of parked truck"));return;}}From=To;}
  UE_LOG(LogS47T02Test,Display,TEXT("S47: T02 native route actual %s return egress, lane %.2f"),West?TEXT("west"):TEXT("east"),Lane);
  Path(Exit);Stage(205,0);return;
 }
 if (T.Stage == 205) {if(!Walk())return;Aim(Target(TEXT("doorEast")));Stage(206);return;}
 if (T.Stage == 206)
 {
  if(!M->Web->IsDoorOpen(TEXT("doorEast"))&&!Pick(TEXT("doorEast")))return;
  Path({FVector(5.2,0,1.6),FVector(4.1,0,1),FVector(3.6,0,1.3)});Stage(207,.8);return;
 }
 if (T.Stage == 207) {if(!Walk())return;Aim(Target(TEXT("logbook")));Stage(37);return;}
}
