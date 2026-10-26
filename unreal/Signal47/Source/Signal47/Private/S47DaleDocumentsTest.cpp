#include "S47DaleDocumentsTest.h"
#include "S47Chapter2.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47HUD.h"
#include "S47Inter.h"
#include "S47Three.h"
#include "SS47Widgets.h"
#include "S47Main.h"
#include "S47Office.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DaleDocs, Log, All);

FS47DaleDocumentsTest::FS47DaleDocumentsTest(US47Main* InM) : M(InM)
{ bEnabled = FParse::Param(FCommandLine::Get(), TEXT("S47DaleDocumentsContract")); }
void FS47DaleDocumentsTest::Fail(const FString& Why)
{
 if (bDone) return; bDone = true;
 if (M && M->Player) M->Player->TestMove.Reset();
 UE_LOG(LogS47DaleDocs, Error, TEXT("S47: Dale documents FAIL stage=%d checks=%d: %s"), Stage, Checks, *Why);
 if (!Root.IsEmpty()) { auto R = S47J::New(); S47J::Set(R,TEXT("status"),TEXT("FAIL")); S47J::Set(R,TEXT("reason"),Why); S47J::Set(R,TEXT("checks"),Checks); FFileHelper::SaveStringToFile(S47J::Write(R,true), *(Root / TEXT("result.json"))); }
 FPlatformMisc::RequestExitWithStatus(false, 1);
}
bool FS47DaleDocumentsTest::Check(bool Good, const FString& Why)
{
 ++Checks; if (!Good) { Fail(Why); return false; }
 UE_LOG(LogS47DaleDocs, Display, TEXT("S47: Dale documents PASS: %s"), *Why); return true;
}
void FS47DaleDocumentsTest::Next(int32 Value, double Wait) { Stage = Value; Hold = Wait; }
void FS47DaleDocumentsTest::Shot(const FString& Name) { FScreenshotRequest::RequestScreenshot(Root / (Name + TEXT(".png")), true, false); }
bool FS47DaleDocumentsTest::StoreRestore(const FString& Name, const S47J::FObj& Snapshot)
{
 FS47SaveStore Writer(Root / Name); Writer.Init(); FS47SaveCard Card; Card.Chapter = TEXT("Dale documents contract"); Card.Place = TEXT("Operations"); Card.Clock = M->Story->ClockText(true);
 FString Error; const auto Meta = Writer.Save(1, TEXT("manual"), 0, S47J::Clone(Snapshot), Card, Error);
 if (!Meta) { Fail(TEXT("isolated Writer failed: ") + Error); return false; }
 FS47SaveStore Reader(Root / Name); Reader.Init(); const auto Disk = Reader.Load(Meta->Id);
 if (!Check(Disk.IsValid(), TEXT("fresh isolated disk reader: ") + Name)) return false;
 FFileHelper::SaveStringToFile(S47J::Write(Disk,true), *(Root / (Name + TEXT(".loaded.json")))); ++Stores;
 US47Flow::FStart Start; Start.State = Disk; Start.CaseId = 0; Start.bDev = true; Start.Playtime = M->Playtime; M->Flow->StartGame(Start);
 return Check(M->Flow->bDevRun && M->Flow->CaseId == 0 && M->Office->WasUnlocked(), TEXT("full Flow restore keeps actual office access: ") + Name);
}
void FS47DaleDocumentsTest::Step(double Dt)
{
 if (!bEnabled || bDone || !M || !M->Office || !M->Evening || !M->Flow || !M->Story || !M->World || !M->Player || !M->UI) return;
 Elapsed += FMath::Clamp(Dt,0.,.2);
 if (Elapsed > 180) { Fail(TEXT("180 s timeout")); return; }
 if (M->Flow->bStarting || M->World->bBusy) return;
 if (Hold > 0) { Hold -= Dt; return; }
 if (!M->Flow->bDevRun || M->Flow->CaseId != 0) { Fail(TEXT("developer case zero required; no player save may be changed")); return; }
 auto Document = [this]() -> TSharedPtr<SS47Document>
 {
  const TSharedPtr<SWidget> Widget=M->Hud?M->Hud->ModalWidget():nullptr;
  return Widget.IsValid()&&Widget->GetTypeAsString()==TEXT("SS47Document")?StaticCastSharedPtr<SS47Document>(Widget):nullptr;
 };
 auto SendKey = [this](const FKey& Key, bool bRepeat = false)
 {
  if (!FSlateApplication::IsInitialized()) { Fail(TEXT("Slate keyboard routing is unavailable")); return false; }
  return Check(FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(Key,FModifierKeysState(),0,bRepeat,0,0)), TEXT("actual focused Slate document handles key ")+Key.GetFName().ToString());
 };
 if (Stage == 0)
 {
  FString UserDir, Jump; FParse::Value(FCommandLine::Get(), TEXT("UserDir="), UserDir); FParse::Value(FCommandLine::Get(), TEXT("S47Jump="), Jump);
  UserDir = FPaths::ConvertRelativePathToFull(UserDir); FPaths::CollapseRelativeDirectories(UserDir);
  if (!(UserDir.StartsWith(TEXT("/tmp/")) || UserDir.StartsWith(TEXT("/var/tmp/"))) || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()), UserDir) || Jump != TEXT("chapter2"))
  { Fail(TEXT("explicit isolated /tmp UserDir and S47Jump=chapter2 required")); return; }
  for (const TCHAR* Flag : {TEXT("S47T01Contract"),TEXT("S47T02Contract"),TEXT("S47T02DriveContract"),TEXT("S47T02UnresolvedContract"),TEXT("S47SmokeTest")})
   if (FParse::Param(FCommandLine::Get(),Flag)) { Fail(TEXT("do not combine independent live drivers")); return; }
  Root = FPaths::ProjectSavedDir() / TEXT("DaleDocumentsContract") / FGuid::NewGuid().ToString(EGuidFormats::Digits); IFileManager::Get().MakeDirectory(*Root,true);
  auto* C2 = Cast<US47Chapter2>(M->Story->Chapter(TEXT("ch2"))); if (!C2) { Fail(TEXT("chapter-two fixture missing")); return; }
  M->UI->Close(true); M->Story->SetCinematic(false); M->Story->ClearTimers();
  C2->bRinging=false; C2->S.bAnswered=true; C2->S.bDoor=true; C2->S.bEntered=true; C2->S.Stage=TEXT("records");
  M->Player->Place(.45,5.75,0);
  if (!Check(M->Office->Ready() && !M->Office->HasKey() && !M->Office->RouteRequired(),TEXT("legacy chapter fixture has physical locked office, optional new route"))) return;
  if (!Check(!M->Office->ReadLibraryDocument(TEXT("EV_DALE_SKYLOG")),TEXT("locked/outside office cannot read a registered private document"))) return;
  if (!Check(M->Office->Use(TEXT("office:drawer")),TEXT("actual service drawer begins opening"))) return;
  Next(1,.65); return;
 }
 if (Stage == 1)
 {
  if (!Check(M->Office->Use(TEXT("office:key")) && M->Office->HasKey() && M->Office->Use(TEXT("office:door")),TEXT("actual spare and office latch used"))) return;
  Next(2,.8); return;
 }
 if (Stage == 2)
 {
  if (!Check(M->Office->DoorOpen(),TEXT("physical office door finishes opening"))) return;
  Points = {FVector(.45,0,6.2),FVector(.45,0,6.6),FVector(.45,0,7.0),FVector(.45,0,7.4),FVector(.30,0,8.1),FVector(.20,0,8.65)};
  WalkIndex=0; LastWalk=M->Player->Pos; LastProgress=Elapsed; Next(3,0); return;
 }
 if (Stage == 3)
 {
  const FVector At=M->Player->Pos; const double Moved=FVector2D(At.X-LastWalk.X,At.Z-LastWalk.Z).Size();
  if (Moved > .5) { Fail(TEXT("office entry jumped beyond a native walking step")); return; }
  WalkMetres+=Moved; LastWalk=At; if(Moved>.002)LastProgress=Elapsed;
  if(WalkIndex<Points.Num())
  {
   const FVector D=Points[WalkIndex]-At;const double Distance=FVector2D(D.X,D.Z).Size();
   if(Distance<.11){++WalkIndex;M->Player->TestMove.Reset();return;}
   if(Elapsed-LastProgress>4){Fail(TEXT("native office walk blocked"));return;}
   M->Player->Yaw=FMath::Atan2(-D.X,-D.Z);M->Player->Pitch=-.12;M->Player->TestMove=FVector2D(0,-FMath::Min(1.,Distance*2));return;
  }
  M->Player->TestMove.Reset();
  if(!Check(M->Office->Contains(At)&&WalkMetres>2.5,TEXT("native Walker/Resolve enters real room without placement")))return;
  if(!Check(M->Office->Use(TEXT("office:folder"))&&M->Office->HasFieldSheet(),TEXT("short T02 folder route retains real field sheet")))return;
  M->UI->Close(true);
  for(const TCHAR* Id : {TEXT("office:private"),TEXT("office:arrayService")})
  {
   const FS47Obj Obj=M->Office->Object(Id);const FVector Eye=M->Player->EyeUE();const FVector Aim=(S47Three::ToUE(Obj.WorldBox().GetCenter())-Eye).GetSafeNormal();
   if(!Check(M->Inter->Pick(Eye,Aim)==Id,FString(TEXT("separate physical paper is independently ray-pickable: "))+Id))return;
  }
  if(!Check(M->Office->Use(TEXT("office:private"))&&M->UI->Modal(),TEXT("real private folder opens its document list")))return;
  for(const FString& Id:FS47Office::LibraryDocumentIds()) if(!Check(!M->Office->DocumentRead(Id),TEXT("browsing leaves unread: ")+Id))return;
  Shot(TEXT("private-folder-unread"));Next(4,.5);return;
 }
 if(Stage==4)
 {
  M->UI->Close(true);
  if(!StoreRestore(TEXT("unread-menu"),M->Flow->GameState()))return;
  Next(5,.6);return;
 }
 if(Stage==5)
 {
  if(!Check(M->Office->HasFieldSheet()&&!M->Evening->Pack.Flag(TEXT("vhs_seen"))&&!M->Office->DocumentRead(TEXT("EV_DALE_SKYLOG")),TEXT("Continue preserves field route without reading lore or film")))return;
  if(!Check(M->Office->ReadLibraryDocument(TEXT("EV_DALE_SKYLOG"))&&M->UI->Modal()&&M->Evening->Pack.Flag(TEXT("office_skylog_read")),TEXT("opening actual sky log commits just that read")))return;
  Next(6,.6);return;
 }
 if(Stage==6)
 {
  const TSharedPtr<SWidget> Modal=M->Hud?M->Hud->ModalWidget():nullptr;
  if(!Check(Modal.IsValid()&&Modal->GetTypeAsString()==TEXT("SS47Document")&&StaticCastSharedPtr<SS47Document>(Modal)->DocumentTextScale()==1.f,TEXT("actual normal document widget uses scale one")))return;
  Shot(TEXT("skylog-normal"));Next(7,.4);return;
 }
 if(Stage==7)
 {
  M->UI->Close(true); ActualRead=S47J::Clone(M->Flow->GameState());
  if(!StoreRestore(TEXT("partial-one-read"),ActualRead))return;
  Next(8,.6);return;
 }
 if(Stage==8)
 {
  if(!Check(M->Office->DocumentRead(TEXT("EV_DALE_SKYLOG"))&&!M->Office->DocumentRead(TEXT("EV_DALE_WITNESS"))&&M->Evening->Pack.Flag(TEXT("office_skylog_read"))&&!M->Evening->Pack.Flag(TEXT("office_witness_read")),TEXT("full Continue preserves one read and leaves the next page unread")))return;
  FS47Doc Empty;Empty.Id=TEXT("EV_DALE_SKYLOG");Empty.Title=TEXT("Isolated empty retained-copy fixture");M->Story->AddDoc(Empty);
  if(!Check(M->Office->ReadLibraryDocument(TEXT("EV_DALE_SKYLOG")),TEXT("actual private-page opening replaces an isolated blank retained copy")))return;
  const auto* Recovered=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_DALE_SKYLOG");});
  const auto* Source=M->Evening->Pack.FindDocument(TEXT("EV_DALE_SKYLOG"));
  if(!Check(Recovered&&Source&&Recovered->Page==FString::Join(Source->Body,TEXT("\n\n"))&&Recovered->Title==Source->Title,TEXT("read credit follows actual registered text, never the blank notebook-copy fixture")))return;
  M->UI->Close(true);
  auto Legacy=S47J::Clone(ActualRead);S47J::Obj(Legacy,TEXT("office"))->RemoveField(TEXT("readDocuments"));
  if(!StoreRestore(TEXT("legacy-no-read-list"),Legacy))return;
  Next(9,.6);return;
 }
 if(Stage==9)
 {
  if(!Check(!M->Office->DocumentRead(TEXT("EV_DALE_SKYLOG"))&&!M->Evening->Pack.Flag(TEXT("office_skylog_read"))&&M->Office->HasFieldSheet(),TEXT("legacy optional-read omission removes read credit without revoking T02 sheet")))return;
  if(!StoreRestore(TEXT("genuine-partial-restored"),ActualRead))return;
  ReadIndex=0;Next(10,.6);return;
 }
 if(Stage==10)
 {
  const auto& Ids=FS47Office::LibraryDocumentIds();
  if(ReadIndex>=Ids.Num()){Next(14);return;}
  M->UI->Close(true);M->UI->bLargeText=true;
  if(!Check(M->Office->ReadLibraryDocument(Ids[ReadIndex])&&M->UI->Modal(),TEXT("actual scaled document opened: ")+Ids[ReadIndex]))return;
  const TSharedPtr<SWidget> Modal=M->Hud?M->Hud->ModalWidget():nullptr;
  if(!Check(Modal.IsValid()&&Modal->GetTypeAsString()==TEXT("SS47Document")&&FMath::IsNearlyEqual(StaticCastSharedPtr<SS47Document>(Modal)->DocumentTextScale(),1.35f),TEXT("actual document widget carries larger text scale: ")+Ids[ReadIndex]))return;
  const auto* Doc=M->Story->Docs().FindByPredicate([&](const FS47Doc& D){return D.Id==Ids[ReadIndex];});
  if(!Check(Doc&&Doc->Page.Len()>180&&!Doc->Transcript.IsEmpty()&&Doc->Image.IsEmpty(),TEXT("real text page retained, without invented photograph: ")+Ids[ReadIndex]))return;
  Next(11,.6);return;
 }
 if(Stage==11){Shot(FString::Printf(TEXT("document-%02d-large"),ReadIndex));Next(12,.4);return;}
 if(Stage==12){ScrollKeys=0;Next(120);return;}
 if(Stage==120)
 {
  const auto W=Document();if(!Check(W.IsValid(),TEXT("keyboard scrolling targets the actual document widget")))return;
  PreviousScroll=W->DocumentScrollOffset();
  if(++ScrollKeys>24){Fail(TEXT("PageDown failed to reach the actual end within 24 keys"));return;}
  if(!SendKey(EKeys::PageDown))return;++TotalScrollKeys;Next(121,.2);return;
 }
 if(Stage==121)
 {
  const auto W=Document();if(!W.IsValid()){Fail(TEXT("document disappeared while scrolling"));return;}
  const float Offset=W->DocumentScrollOffset(),End=W->DocumentScrollEnd();
  if(!Check(Offset>=PreviousScroll-.5f&&Offset<=End+1.f,TEXT("native scroll stays monotonic and within actual content bounds")))return;
  if(Offset+1.f<End)
  {
   if(Offset<=PreviousScroll+.01f){Fail(TEXT("PageDown was handled without advancing the long paper"));return;}
   Next(120,.05);return;
  }
  if(!Check(FMath::Abs(Offset-End)<=1.f,TEXT("focused keyboard reaches last paragraph at actual scroll end")))return;
  Shot(FString::Printf(TEXT("document-%02d-large-bottom"),ReadIndex));Next(122,.4);return;
 }
 if(Stage==122)
 {
  const auto W=Document();if(!W.IsValid()){Fail(TEXT("document missing before PageUp"));return;}
  PreviousScroll=W->DocumentScrollOffset();
  // Held/repeated PageDown at an already visible end must not store invisible
  // overshoot that would swallow the first reverse key.
  for(int32 Repeat=0;Repeat<3;++Repeat){if(!SendKey(EKeys::PageDown,true))return;++TotalScrollKeys;}
  Next(1221,.2);return;
 }
 if(Stage==1221)
 {
  const auto W=Document();
  if(!Check(W.IsValid()&&FMath::Abs(W->DocumentScrollOffset()-PreviousScroll)<=1.f,TEXT("repeated PageDown remains at the actual last paragraph")))return;
  if(!SendKey(EKeys::PageUp))return;++TotalScrollKeys;Next(123,.2);return;
 }
 if(Stage==123)
 {
  const auto W=Document();if(!Check(W.IsValid()&&(PreviousScroll<=1.f?W->DocumentScrollOffset()<=1.f:W->DocumentScrollOffset()<PreviousScroll-.5f),TEXT("PageUp returns through the same real scroll surface")))return;
  if(!SendKey(EKeys::Escape))return;Next(124,.2);return;
 }
 if(Stage==124)
 {
  if(!Check(!M->UI->Modal(),TEXT("focused Escape closes the document without a mouse")))return;
  if(bLegacyTyped){Next(141);return;}
  ++ReadIndex;Next(10);return;
 }
 if(Stage==14)
 {
  M->UI->Close(true);
  for(const FString& Id:FS47Office::LibraryDocumentIds())if(!Check(M->Office->DocumentRead(Id),TEXT("independent page read retained: ")+Id))return;
  if(!Check(!M->Evening->Pack.Flag(TEXT("vhs_seen"))&&!M->Evening->Pack.Flag(TEXT("trafo_anomaly_checked"))&&M->Office->HasFieldSheet(),TEXT("all optional reading completes neither film nor array/T02 experiment")))return;
  if(!Check(M->Player->Walkable(-8,-74)&&M->Player->Walkable(18,-74)&&M->Player->Walkable(-1,-74),TEXT("ordinary A/B/reference coordinates lie on native walkable Row One")))return;
  const auto* Existing=M->Story->Docs().FindByPredicate([](const FS47Doc& D){return D.Id==TEXT("EV_SERVICE_LOG");});
  if(!Check(Existing&&Existing->Kind==TEXT("typed"),TEXT("existing ordinary typed service-log document is retained")))return;
  M->UI->bLargeText=true;M->UI->Document(*Existing);bLegacyTyped=true;ReadIndex=7;Next(140,.6);return;
 }
 if(Stage==140)
 {
  const auto W=Document();
  if(!Check(W.IsValid()&&FMath::IsNearlyEqual(W->DocumentTextScale(),1.35f),TEXT("ordinary existing typed document uses actual large-text widget scale")))return;
  Shot(TEXT("service-log-large-top"));Next(12,.4);return;
 }
 if(Stage==141)
 {
  if(!StoreRestore(TEXT("all-documents-read"),M->Flow->GameState()))return;
  Next(15,.6);return;
 }
 if(Stage==15)
 {
  for(const FString& Id:FS47Office::LibraryDocumentIds())if(!Check(M->Office->DocumentRead(Id),TEXT("full Flow restores actually read page: ")+Id))return;
  if(!Check(!M->Office->ReadLibraryDocument(TEXT("VHS_DALE_FIELD_01")),TEXT("unproduced film cannot be read as an optional paper")))return;
  auto R=S47J::New();S47J::Set(R,TEXT("status"),TEXT("PASS"));S47J::Set(R,TEXT("checks"),Checks);S47J::Set(R,TEXT("freshDiskFlowRestores"),Stores);S47J::Set(R,TEXT("nativeOfficeWalkMetres"),WalkMetres);S47J::Set(R,TEXT("actualSlateScrollKeys"),TotalScrollKeys);
  S47J::Set(R,TEXT("scope"),TEXT("actual Office APIs, native entry, real document UI and focused Slate PageDown/PageUp/Escape through actual content end, existing typed document at large scale, isolated Writer/fresh reader/full Flow restores; initial chapter prerequisites are fixtures; human input, VHS, photos and ARRAY-01 encounter not tested"));
  FFileHelper::SaveStringToFile(S47J::Write(R,true),*(Root/TEXT("result.json")));bDone=true;
  UE_LOG(LogS47DaleDocs,Display,TEXT("S47: Dale documents contract PASS checks=%d diskRestores=%d root=%s"),Checks,Stores,*Root);FPlatformMisc::RequestExit(false);
 }
}
