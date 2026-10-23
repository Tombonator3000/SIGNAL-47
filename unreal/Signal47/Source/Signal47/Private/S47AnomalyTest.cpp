#include "S47AnomalyTest.h"

#include "S47Album.h"
#include "Containers/Ticker.h"
#include "S47Anomaly.h"
#include "S47AnomalyWorld.h"
#include "S47Chapter1.h"
#include "S47Evening.h"
#include "S47Flow.h"
#include "S47Inter.h"
#include "S47Keys.h"
#include "S47Image.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Story.h"
#include "S47Timeline.h"
#include "S47Three.h"
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
#include "UnrealClient.h"

namespace
{
 bool SameAlbum(const FS47AlbumSave& A, const FS47AlbumSave& B)
 {
  if (A.Shots.Num() != B.Shots.Num()) return false;
  for (int32 I=0; I<A.Shots.Num(); ++I)
  {
   const auto& X=A.Shots[I]; const auto& Y=B.Shots[I];
   if (X.Id!=Y.Id || X.Url!=Y.Url || X.Clock!=Y.Clock || X.Place!=Y.Place || X.Text!=Y.Text || X.bOdd!=Y.bOdd || X.bUnavailable!=Y.bUnavailable || X.Targets.Num()!=Y.Targets.Num()) return false;
   for (const auto& T:X.Targets)
   {
    const auto* Other=Y.Targets.FindByPredicate([&T](const TPair<FString,FVector2D>& P){return P.Key==T.Key;});
    if (!Other || Other->Value!=T.Value) return false;
   }
  }
  return true;
 }
}

void FS47AnomalyTest::Fail(const FString& Why)
{
 bDone=true;
 if (M && M->Player) M->Player->TestMove.Reset();
 Event(TEXT("FAIL: ")+Why);
 UE_LOG(LogTemp,Error,TEXT("S47: anomaly route %s FAIL stage %d after %.1fs: %s"),*Route,Stage,Elapsed,*Why);
 if (M && M->Web && M->Web->GetWorld())
 {
  // Leave at least one rendered frame for the requested failure HUD evidence.
  Shot(TEXT("failure"));
  FTimerHandle ExitTimer;
  M->Web->GetWorldTimerManager().SetTimer(ExitTimer,[] {FPlatformMisc::RequestExitWithStatus(false,1);},1.5f,false);
 }
 else FPlatformMisc::RequestExitWithStatus(false,1);
}

void FS47AnomalyTest::Event(const FString& Action)
{
 const auto E=S47J::New();
 S47J::Set(E,TEXT("seconds"),Elapsed); S47J::Set(E,TEXT("action"),Action);
 S47J::Set(E,TEXT("clock"),M && M->Story?M->Story->Clock():0);
 S47J::Set(E,TEXT("clock_text"),M && M->Story?M->Story->ClockText():FString(TEXT("unknown")));
 Events.Add(E);
 if (!Shots.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> Values;
  for (const auto& V:Events) Values.Add(MakeShared<FJsonValueObject>(V));
  FString Text; auto Writer=TJsonWriterFactory<>::Create(&Text); FJsonSerializer::Serialize(Values,Writer);
  FFileHelper::SaveStringToFile(Text,*FPaths::Combine(Shots,TEXT("events.json")));
 }
 UE_LOG(LogTemp,Display,TEXT("S47: anomaly route %s [%06.1fs / %s] %s"),*Route,Elapsed,M && M->Story?*M->Story->ClockText():TEXT("?"),*Action);
}

void FS47AnomalyTest::Next(int32 To,const FString& Action,double Wait)
{
 Stage=To; StageElapsed=0; Hold=Wait; Event(Action);
 const TMap<FString,int32> Stops={{TEXT("routine"),2},{TEXT("n02"),6},{TEXT("reference"),8},{TEXT("records"),9},{TEXT("anomaly"),13},{TEXT("comparison"),15},{TEXT("p01"),18},{TEXT("notebook"),19}};
 if (!InspectState.IsEmpty() && Stops.FindRef(InspectState)==To)
 {
  bDone=true; Shot(InspectState);
  UE_LOG(LogTemp,Display,TEXT("S47: anomaly state %s READY; isolated control fixture held for inspection, no human-input claim"),*InspectState);
 }
}

void FS47AnomalyTest::Shot(const FString& Name)
{
 if (Shots.IsEmpty()) return;
 Requested.AddUnique(Name);
 const FString Path=FPaths::Combine(Shots,Name+TEXT(".png"));
 // ShutterFlash paints black/white in this frame. Capture the settled HUD after
 // it clears, while the scenario's two-second hold keeps its viewpoint/panel.
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Path](float)
 {
  FScreenshotRequest::RequestScreenshot(Path,true,false); return false;
 }),.6f);
 Event(TEXT("Rendered HUD screenshot queued after shutter/fade settles: ")+Name);
}

void FS47AnomalyTest::Pose(const FVector& Feet,const FVector& LookAt)
{
 // Test pose establishes a controlled viewpoint. It does not claim physical travel.
 M->UI->Close(true);
 M->Player->Place(Feet.X,Feet.Z,0);
 const FVector Delta=LookAt-FVector(Feet.X,M->Player->FloorY+M->Player->Eye,Feet.Z);
 M->Player->Yaw=FMath::Atan2(-Delta.X,-Delta.Z);
 M->Player->Pitch=FMath::Atan2(Delta.Y,FMath::Sqrt(Delta.X*Delta.X+Delta.Z*Delta.Z));
}

bool FS47AnomalyTest::Photograph(const FString& ExpectedTarget)
{
 auto* C1=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")));
 if (!C1 || !M->FieldCamera->bHave) {Fail(TEXT("ordinary field camera unavailable"));return false;}
 M->UI->Close(true);
 if (!M->FieldCamera->bRaised) C1->ToggleCamera();
 const int32 N=M->Night->Album->Shots().Num();
 const double Clock=M->Story->Clock();
 C1->Shutter();
 if (M->Night->Album->Shots().Num()!=N+1) {Fail(TEXT("Chapter1.Shutter did not render exactly one album exposure"));return false;}
 const auto& P=M->Night->Album->Shots().Last();
 // Preserve the actual failed frame before checking its evidence tag. A failure
 // must remain inspectable; dumping a frame never changes the verdict.
 if (!Shots.IsEmpty())
 {
  const FString Stem=P.Id.Replace(TEXT("@"),TEXT("-"));
  int32 Comma=INDEX_NONE; TArray<uint8> Bytes;
  if (P.Url.FindChar(TCHAR(','),Comma) && FBase64::Decode(P.Url.Mid(Comma+1),Bytes))
  {
   FFileHelper::SaveArrayToFile(Bytes,*FPaths::Combine(Shots,Stem+TEXT(".jpg")));
   if (ExpectedTarget==TEXT("anomaly:reference"))
    FFileHelper::SaveArrayToFile(Bytes,*FPaths::Combine(Shots,TEXT("reference-photo.jpg")));
  }
  const auto Info=S47J::New();
  S47J::Set(Info,TEXT("id"),P.Id); S47J::Set(Info,TEXT("expected_target"),ExpectedTarget);
  S47J::Set(Info,TEXT("clock"),P.Clock); S47J::Set(Info,TEXT("place"),P.Place); S47J::Set(Info,TEXT("caption"),P.Text);
  S47J::Set(Info,TEXT("url_length"),P.Url.Len());
  const auto Player=S47J::New();
  S47J::Set(Player,TEXT("pos"),TArray<double>{M->Player->Pos.X,M->Player->Pos.Y,M->Player->Pos.Z});
  S47J::Set(Player,TEXT("floor_y"),M->Player->FloorY); S47J::Set(Player,TEXT("eye"),M->Player->Eye);
  S47J::Set(Player,TEXT("yaw"),M->Player->Yaw); S47J::Set(Player,TEXT("pitch"),M->Player->Pitch);
  S47J::Set(Player,TEXT("area"),M->World->Area); S47J::Set(Info,TEXT("player"),Player);
  S47J::Set(Info,TEXT("photo_loc_ue"),TArray<double>{M->FieldCamera->PhotoLoc.X,M->FieldCamera->PhotoLoc.Y,M->FieldCamera->PhotoLoc.Z});
  S47J::Set(Info,TEXT("photo_rot_ue"),TArray<double>{M->FieldCamera->PhotoRot.Pitch,M->FieldCamera->PhotoRot.Yaw,M->FieldCamera->PhotoRot.Roll});
  const auto Targets=S47J::New();
  for (const auto& T:P.Targets) S47J::Set(Targets,*T.Key,TArray<double>{T.Value.X,T.Value.Y});
  S47J::Set(Info,TEXT("targets"),Targets);
  const auto Projected=S47J::New();
  for (const auto& T:TArray<TPair<FString,FVector>>{{TEXT("lamp"),M->AnomalyWorld->LampAt()},{TEXT("reference"),M->AnomalyWorld->ReferenceAt()},{TEXT("changed"),M->AnomalyWorld->ChangedAt()}})
  {
   const auto Q=M->FieldCamera->Project(T.Value); const auto V=S47J::New();
   S47J::Set(V,TEXT("world"),TArray<double>{T.Value.X,T.Value.Y,T.Value.Z});
   S47J::Set(V,TEXT("uv"),TArray<double>{Q.U,Q.V}); S47J::Set(V,TEXT("front"),Q.bFront);
   S47J::Set(Projected,*T.Key,V);
  }
  S47J::Set(Info,TEXT("world_projections"),Projected);
  int32 Width=0,Height=0; TArray<FColor> Pixels;
  const bool Decoded=S47Image::DecodePixels(P.Url,Pixels,Width,Height);
  S47J::Set(Info,TEXT("decoded"),Decoded); S47J::Set(Info,TEXT("width"),Width); S47J::Set(Info,TEXT("height"),Height);
  if (const auto& Last=M->Night->Album->Last;Last)
  {
   S47J::Set(Info,TEXT("render_mean"),Last->Mean); S47J::Set(Info,TEXT("render_ms"),Last->Ms);
   S47J::Set(Info,TEXT("named"),Last->Names); S47J::Set(Info,TEXT("odd"),Last->Odd);
  }
  FFileHelper::SaveStringToFile(S47J::Write(Info,true),*FPaths::Combine(Shots,Stem+TEXT(".photo.json")));
 }
 if (P.Url.Len()<1000 || !P.Url.StartsWith(TEXT("data:image/")) || P.Clock!=Clock || !P.Targets.ContainsByPredicate([&ExpectedTarget](const TPair<FString,FVector2D>& T){return T.Key==ExpectedTarget && T.Value.X>=0 && T.Value.X<=1 && T.Value.Y>=0 && T.Value.Y<=1;}))
 {Fail(TEXT("actual exposure lacks content, original clock or framed target ")+ExpectedTarget);return false;}
 if (M->FieldCamera->bRaised) C1->ToggleCamera();
 Event(TEXT("Real Chapter1 camera exposure: ")+P.Id+TEXT(" target ")+ExpectedTarget);
 return true;
}

bool FS47AnomalyTest::RoundTrip(const FString& Name,const S47J::FObj& Override)
{
 const auto Before=Override?S47J::Clone(Override):M->Flow->GameState();
 if (!Override && S47J::Int(Before,TEXT("v"))!=3) {Fail(TEXT("new anomaly state not written in save v3"));return false;}
 const auto Album=M->Night->Album->Data;
 const auto Anomaly=M->Anomaly->Save();
 const double Clock=M->Story->Clock(); const FString Area=M->World->Area;
 FS47SaveStore Writer(FPaths::Combine(Profile,TEXT("AnomalyControls"),Route,Name)); Writer.Init();
 FS47SaveCard Card; Card.Chapter=TEXT("Anomaly pilot controls"); Card.Place=Area; Card.Clock=M->Story->ClockText(); Card.Playtime=M->Playtime;
 FString Error; const auto Meta=Writer.Save(1,TEXT("manual"),0,Before,Card,Error);
 if (!Meta) {Fail(TEXT("isolated FS47SaveStore.Save failed: ")+Error);return false;}
 FS47SaveStore Reader(Writer.Dir); Reader.Init(); const auto Loaded=Reader.Load(Meta->Id);
 if (!Loaded || S47J::Int(Loaded,TEXT("v"))!=S47J::Int(Before,TEXT("v"))) {Fail(TEXT("fresh disk store failed to read state/version"));return false;}
 if (!Shots.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(Loaded,true),*FPaths::Combine(Shots,Name+TEXT(".disk-save.json")));
 US47Flow::FStart Start; Start.State=Loaded; Start.CaseId=0; Start.bDev=true; Start.Playtime=M->Playtime;
 M->Flow->StartGame(Start);
 if (!M->Flow->bDevRun || M->Flow->CaseId!=0 || M->Story->Clock()!=Clock || M->World->Area!=Area || !SameAlbum(Album,M->Night->Album->Data))
 {Fail(TEXT("full Flow.StartGame disk restore changed photos (bytes/clock/targets), clock, area or isolated dev case"));return false;}
 if (!Override && S47J::Write(Anomaly)!=S47J::Write(M->Anomaly->Save())) {Fail(TEXT("full disk restore changed investigation states"));return false;}
 Event(TEXT("PASS disk save + new FS47SaveStore.Load + full Flow.StartGame restore: ")+Name+TEXT("; exact photo bytes/clock/targets retained"));
 return true;
}

void FS47AnomalyTest::Cut(const FString& Chapter)
{
 const auto Docs=M->Story->Docs(); const auto Notes=M->Story->Notes();
 const double Before=M->Story->Clock();
 M->UI->Close(true); M->Story->Jump(Chapter);
 // The fixture jumps over old chapters only. The actual player's pilot material is carried.
 for (const auto& D:Docs) M->Story->AddDoc(D);
 for (const auto& N:Notes) M->Story->Note(N);
 M->Anomaly->Changed();
 if (M->Story->Clock()<Before) {Fail(TEXT("marked fixture cut moved clock backward"));return;}
 Event(TEXT("MARKED TEST CUT: ")+Chapter+TEXT(". Intermediate story/travel skipped; actual pilot photographs and documents carried unchanged."));
}

void FS47AnomalyTest::Finish()
{
 for (const auto& Name:Requested)
  if (IFileManager::Get().FileSize(*FPaths::Combine(Shots,Name+TEXT(".png")))<1000) {Fail(TEXT("requested screenshot absent or empty: ")+Name);return;}
 Event(TEXT("PASS COMPLETE. API actions and test poses; keyboard input, audio listening, human unease and full-night continuity UNVERIFIED."));
 UE_LOG(LogTemp,Display,TEXT("S47: anomaly route %s PASS COMPLETE %.1fs; disk/world restore, exact stored exposures, requested HUD screenshots checked; human input/audio/unease UNVERIFIED"),*Route,Elapsed);
 bDone=true; FPlatformMisc::RequestExit(false);
}

void FS47AnomalyTest::Step(double Dt)
{
 if (bDone) return;
 if (!bInitialized)
 {
  bInitialized=true;
  FParse::Value(FCommandLine::Get(),TEXT("S47AnomalyState="),InspectState);
  if (!FParse::Value(FCommandLine::Get(),TEXT("S47AnomalyRoute="),Route)) {if(InspectState.IsEmpty()) {bDone=true;return;} Route=TEXT("pilot");}
  if (!InspectState.IsEmpty() && !TArray<FString>{TEXT("routine"),TEXT("n02"),TEXT("reference"),TEXT("records"),TEXT("anomaly"),TEXT("comparison"),TEXT("p01"),TEXT("notebook")}.Contains(InspectState)) {Fail(TEXT("unknown anomaly inspection state"));return;}
  if (!TArray<FString>{TEXT("pilot"),TEXT("late"),TEXT("legacy"),TEXT("already-seen"),TEXT("missing-phone"),TEXT("archive"),TEXT("missing-photo")}.Contains(Route)) {Fail(TEXT("unknown anomaly route"));return;}
  FParse::Value(FCommandLine::Get(),TEXT("S47AnomalyShots="),Shots);
  if (!Shots.IsEmpty()) IFileManager::Get().MakeDirectory(*Shots,true);
  if (!FParse::Value(FCommandLine::Get(),TEXT("UserDir="),Profile)) {Fail(TEXT("explicit isolated -UserDir required"));return;}
  Profile=FPaths::ConvertRelativePathToFull(Profile); FPaths::CollapseRelativeDirectories(Profile);
  if (!(Profile.StartsWith(TEXT("/tmp/")) || Profile.StartsWith(TEXT("/var/tmp/"))) || !M || !M->Flow || !M->Flow->bDevRun || M->Flow->CaseId!=0)
  {Fail(TEXT("test requires /tmp or /var/tmp UserDir, devRun and case0; no installed saves touched"));return;}
  Next(1,TEXT("START isolated API controls; marked test poses/time cuts, rendered exposures; real input UNVERIFIED"),2);
  return;
 }
 if (!M || !M->Story || !M->World || !M->Anomaly || !M->AnomalyWorld || !M->Night || !M->Night->Album || !M->FieldCamera || !M->Evening || !M->Player || !M->UI || !M->Inter)
 {Fail(TEXT("runtime dependency missing"));return;}
 Elapsed+=Dt; StageElapsed+=Dt;
 if (Elapsed>560 || StageElapsed>120) {Fail(TEXT("timeout waiting for state or physical control"));return;}
 if (Hold>0) {Hold-=Dt;return;}
 auto& D=M->Anomaly->Data;
 const bool Full=Route==TEXT("pilot") || Route==TEXT("missing-phone") || Route==TEXT("missing-photo");
 if (Stage==1)
 {
  M->World->StopDriving(); M->World->Enter(TEXT("saro"));
  M->Story->bEveningTimeline=true; M->Story->Reset(); M->Story->SetClock(S47Timeline::ShiftStart); M->Story->BeginEveningShift(); M->Evening->Begin(nullptr,true);
  M->Anomaly->Begin(); M->AnomalyWorld->Begin(); M->UI->Fade(false);
  if (Route==TEXT("archive"))
  {
   // Authored source capture, not the night player's earlier visit or photograph.
   M->Story->SetClock(-24600); D.bEarlyCamera=true;
   if (auto* C1=Cast<US47Chapter1>(M->Story->Chapter(TEXT("ch1")))) C1->RestoreEarlyCamera();
   M->World->Enter(TEXT("diner")); Pose(FVector(8017.6,0,2062.3),M->AnomalyWorld->ReferenceAt());
   Next(92,TEXT("DAY OPERATOR ARCHIVE CAPTURE FIXTURE / 13 APR 1986 17:10; authored source, no player-route claim"),2); return;
  }
  if (!Full)
  {
   Cut(TEXT("chapter5"));
   if (bDone) return;
   M->Story->SetClock(4*3600+20*60);
   if (Route==TEXT("already-seen")) {M->TownSeen.Add(TEXT("ev:cutout"));M->TownSeen.Add(TEXT("ev:wave"));}
   if (Route==TEXT("legacy")) M->Story->bEveningTimeline=false;
   auto Old=M->Flow->GameState(); Old->RemoveField(TEXT("anomaly")); Old->RemoveField(TEXT("evening"));
   S47J::Set(Old,TEXT("v"),2); S47J::Set(Old,TEXT("timelineVersion"),Route==TEXT("legacy")?1:2);
   if (!RoundTrip(TEXT("old-without-anomaly"),Old)) return;
   Next(90,TEXT("Late/old fixture loaded from disk without anomaly state; no completed investigation manufactured"),2); return;
  }
  if (M->Story->Clock()!=S47Timeline::ShiftStart || M->Evening->Opening()) {Fail(TEXT("routine fixture did not start at 18:00 after handover"));return;}
  // D01 transport/voice are covered separately by the evening routes.
  Pose(FVector(4.1,0,1.0),M->Web->Obj(TEXT("room.objs.fieldCamera")).WorldPosition());
  M->Inter->UseId(TEXT("fieldCamera"));
  if (!D.bEarlyCamera || !M->FieldCamera->bHave) {Fail(TEXT("early camera pickup did not preserve ordinary B12 state"));return;}
  if (!M->Anomaly->Use(TEXT("anomaly:routine")) || !D.N01Routine || D.N01Compared || D.N01Followed || !D.N01Assessment.IsEmpty()) {Fail(TEXT("18:00 N01 routine missing or future event assessed prematurely"));return;}
  M->UI->Close(true);
  Next(2,TEXT("18:00 SARO routine and early field-camera pickup; no B12/story completion")); return;
 }
 if (Stage==2)
 {
  if (!RoundTrip(TEXT("routine"))) return;
  if (!D.N01Routine || D.RoutineClock>=0 || M->Story->Begun.Contains(TEXT("ch5"))) {Fail(TEXT("routine disk checkpoint lost early state or enabled ch5"));return;}
  M->Story->SetClock(-5*3600-30*60);
  M->World->Enter(TEXT("diner"));
  Pose(FVector(7990,0,2133.1),M->AnomalyWorld->LampAt());
  Next(3,TEXT("MARKED TEST TRANSFER to Caliche at 18:30, first N02 viewpoint; travel unmeasured"),2); return;
 }
 if (Stage==3)
 {
  if (!Photograph(TEXT("anomaly:n02reference"))) return;
  Pose(FVector(7987,0,2138),M->AnomalyWorld->LampAt());
  Next(4,TEXT("N02 changed standing position for a controlled second view"),2); return;
 }
 if (Stage==4)
 {
  M->Inter->UseId(TEXT("anomaly:lamp"));
  if (!D.N02Observed || !D.N02Control) {Fail(TEXT("physical side-position inspection did not establish N02 control"));return;}
  if (!Photograph(TEXT("anomaly:n02control"))) return;
  if (D.N02Photo.IsEmpty() || D.ControlPhoto.IsEmpty() || D.N02Photo==D.ControlPhoto) {Fail(TEXT("N02 did not retain two different actual frames"));return;}
  Next(5,TEXT("N02 control exposed from second position; preserve before assessment"));return;
 }
 if (Stage==5)
 {
  if (!RoundTrip(TEXT("n02-documented"))) return;
  M->Story->SetClock(M->Story->Clock()+91);
  M->Anomaly->Open(TEXT("n02"));
  Next(6,TEXT("MARKED TEST DEVELOPMENT +91s; active N02 comparison opened"),1);return;
 }
 if (Stage==6)
 {
  if (!M->Anomaly->Assess(TEXT("n02"),TEXT("reflection")) || !D.N02Compared) {Fail(TEXT("N02 active comparison did not explain ordinary reflection"));return;}
  M->UI->Close(true);
  Pose(FVector(8017.6,0,2062.3),M->AnomalyWorld->ReferenceAt());
  Next(7,TEXT("N02 ordinary lighting explanation established; move to real Caliche window baseline"),2);return;
 }
 if (Stage==7)
 {
  if (!Photograph(TEXT("anomaly:reference"))) return;
  if (D.ReferencePhoto.IsEmpty() || D.ReferenceClock>=0 || D.A02Moved || D.A02Compared) {Fail(TEXT("window reference absent or future anomaly triggered during baseline"));return;}
  Shot(TEXT("reference"));
  Next(8,TEXT("Evening baseline photograph contains real price tag/damage reference"),2);return;
 }
 if (Stage==8)
 {
  if (!RoundTrip(TEXT("evening-reference"))) return;
  EarlyState=M->Flow->GameState();
  M->World->Enter(TEXT("saro")); Cut(TEXT("printed"));
  if (bDone) return;
  M->Story->Use(TEXT("printer")); M->UI->Close(true);
  if (!M->Story->Docs().ContainsByPredicate([](const FS47Doc& P){return P.Id==TEXT("printout");})) {Fail(TEXT("actual printer tear did not file the residual paper"));return;}
  Cut(TEXT("chapter1")); if (bDone) return;
  M->Anomaly->Open(TEXT("n01"));
  Next(9,TEXT("00:13 SARO actual printout acquired; active journal/schedule comparison opened"),1);return;
 }
 if (Stage==9)
 {
  if (M->Anomaly->Assess(TEXT("n01"),TEXT("cause")) || D.N01Compared || !D.N01Assessment.StartsWith(TEXT("correction needed"))) {Fail(TEXT("N01 unjustified causal claim was accepted"));return;}
  if (!RoundTrip(TEXT("n01-correction-pending"))) return;
  if (D.N01Compared || !D.N01Assessment.StartsWith(TEXT("correction needed"))) {Fail(TEXT("N01 correction choice lost across disk/world reload"));return;}
  if (!M->Anomaly->Assess(TEXT("n01"),TEXT("revise")) || !D.N01Compared) {Fail(TEXT("N01 material never obtained promised function against actual printout after revision"));return;}
  const FVector Folder=S47Three::ToThree(M->Web->PropLocation(TEXT("logbook")));
  Pose(Folder+FVector(0,0,1.3),Folder);
  M->Anomaly->Use(TEXT("anomaly:n01follow")); M->UI->Close(true);
  if (!D.N01Followed) {Fail(TEXT("N01 physical handover-folder follow-up missing"));return;}
  Next(10,TEXT("N01 printout order compared; physical records followed up"));return;
 }
 if (Stage==10)
 {
  if (!RoundTrip(TEXT("n01-compared"))) return;
  Cut(TEXT("chapter5")); if (bDone) return;
  Next(11,TEXT("MARKED TEST CUT to All Night 04:10 with same real evidence"),2);return;
 }
 if (Stage==11)
 {
  if (!D.A02Moved || D.A02Compared || D.A02Followed) {Fail(TEXT("A02 physical variant did not move away from view after midnight, or silently completed"));return;}
  M->World->Enter(TEXT("diner"));
  Pose(FVector(8014,0,2044),M->AnomalyWorld->ChangedAt());
  Next(12,TEXT("Return to relocated cardboard figure; independent scene variant"),2);return;
 }
 if (Stage==12)
 {
  M->Inter->UseId(TEXT("anomaly:figure")); M->UI->Close(true);
  if (!Photograph(TEXT("anomaly:changed"))) return;
  if (!D.A02Observed || D.ChangedPhoto.IsEmpty() || D.ChangedPhoto==D.ReferencePhoto || D.ChangedClock<D.ReferenceClock) {Fail(TEXT("A02 change not documented with new real exposure"));return;}
  Shot(TEXT("anomaly"));
  Next(13,TEXT("New photograph documents same figure in changed location"),2);return;
 }
 if (Stage==13)
 {
  if (!RoundTrip(TEXT("a02-documented"))) return;
  M->Story->SetClock(M->Story->Clock()+91); M->Anomaly->Open(TEXT("a02"));
  Next(14,TEXT("MARKED TEST DEVELOPMENT +91s; actual stored photographs side by side"),1);return;
 }
 if (Stage==14)
 {
  if (!M->UI->Modal()) {Fail(TEXT("A02 comparison panel not open"));return;}
  Shot(TEXT("comparison"));
  Next(15,TEXT("A02 comparison view captured before active assessment"),2);return;
 }
 if (Stage==15)
 {
  if (Route==TEXT("missing-photo"))
  {
   const auto Before=M->Flow->GameState();
   auto ExpectedAlbum=M->Night->Album->Data;
   const auto* Original=ExpectedAlbum.Shots.FindByPredicate([&D](const FS47AlbumShot& P){return P.Id==D.ChangedPhoto;});
   if (!Original || Original->Url.Len()<1000) {Fail(TEXT("missing-photo fixture lacks actual changed exposure"));return;}
   const FS47AlbumShot OriginalPhoto=*Original;
   const FString ArchiveBefore=S47J::Str(S47J::Obj(S47J::Obj(Before,TEXT("anomaly")),TEXT("archive")),TEXT("url"));
   const FString StoreDir=FPaths::Combine(Profile,TEXT("AnomalyControls"),TEXT("missing-photo"),TEXT("corrupt-fixture"));
   FS47SaveStore Writer(StoreDir); Writer.Init();
   FS47SaveCard Card; Card.Chapter=TEXT("Missing-photo negative control"); Card.Place=M->World->Area; Card.Clock=M->Story->ClockText();
   FString Error; const auto Meta=Writer.Save(1,TEXT("manual"),0,Before,Card,Error);
   if (!Meta) {Fail(TEXT("missing-photo isolated save failed: ")+Error);return;}
   const FString Key=S47Saves::PhotoKey(OriginalPhoto.Url);
   const FString PhotoPath=FPaths::Combine(StoreDir,TEXT("photos"),Key+TEXT(".txt"));
   // The only deletion is one photograph in the store this fixture just created.
   // Never use the active game's store or delete the preserved archive/reference.
   if (!Meta->Photos.Contains(Key) || IFileManager::Get().FileSize(*PhotoPath)<1000 || !IFileManager::Get().Delete(*PhotoPath,false,true,true))
   {Fail(TEXT("could not remove exactly the newly-created changed-photo file"));return;}
   FS47SaveStore Reader(StoreDir); Reader.Init(); const auto Loaded=Reader.Load(Meta->Id);
   if (!Loaded) {Fail(TEXT("fresh store could not load deliberately incomplete photo transaction"));return;}
   S47J::FObj Missing;
   for (const auto& V:S47J::Arr(S47J::Obj(Loaded,TEXT("album")),TEXT("shots")))
   {
    const S47J::FObj* O=nullptr;
    if (V && V->TryGetObject(O) && O && S47J::Str(*O,TEXT("id"))==OriginalPhoto.Id) Missing=*O;
   }
   if (!Missing || !S47J::Bool(Missing,TEXT("unavailable")) || S47J::Str(Missing,TEXT("url"))==OriginalPhoto.Url)
   {Fail(TEXT("missing original was not explicitly marked unavailable with replacement URL"));return;}
   if (!Shots.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(Loaded,true),*FPaths::Combine(Shots,TEXT("missing-photo.disk-save.json")));
   US47Flow::FStart Start; Start.State=Loaded; Start.CaseId=0; Start.bDev=true; Start.Playtime=M->Playtime;
   M->Flow->StartGame(Start);
   auto* ExpectedChanged=ExpectedAlbum.Shots.FindByPredicate([&OriginalPhoto](const FS47AlbumShot& P){return P.Id==OriginalPhoto.Id;});
   ExpectedChanged->Url=S47J::Str(Missing,TEXT("url")); ExpectedChanged->bUnavailable=true;
   bool CarryUnavailable=false;
   for (const auto& V:S47J::Arr(S47J::Obj(M->Flow->Carry,TEXT("album")),TEXT("shots")))
   {
    const S47J::FObj* O=nullptr;
    if (V && V->TryGetObject(O) && O && S47J::Str(*O,TEXT("id"))==OriginalPhoto.Id) CarryUnavailable=S47J::Bool(*O,TEXT("unavailable"));
   }
   const auto ArchiveAfter=S47J::Obj(M->Anomaly->Save(),TEXT("archive"));
   if (!M->Flow->bDevRun || M->Flow->CaseId!=0 || !SameAlbum(ExpectedAlbum,M->Night->Album->Data) || !CarryUnavailable || ArchiveBefore!=S47J::Str(ArchiveAfter,TEXT("url")) || S47J::Bool(ArchiveAfter,TEXT("unavailable")))
   {Fail(TEXT("missing-photo restore changed id/clock/targets/other photographs/archive or lost unavailable marker"));return;}
   if (M->Anomaly->Assess(TEXT("a02"),TEXT("moved")) || D.A02Compared)
   {Fail(TEXT("replacement card accepted as proof of moved figure"));return;}
   // A normal snapshot after loading must keep the missing-image marker in the
   // album itself. Reopening Carry alone would miss snapshot/transition loss.
   const auto Snapshot=M->Flow->GameState();
   auto MissingIn=[&OriginalPhoto](const S47J::FObj& State)
   {
    for (const auto& V:S47J::Arr(S47J::Obj(State,TEXT("album")),TEXT("shots")))
    {
     const S47J::FObj* O=nullptr;
     if (V && V->TryGetObject(O) && O && S47J::Str(*O,TEXT("id"))==OriginalPhoto.Id)
      return S47J::Bool(*O,TEXT("unavailable"));
    }
    return false;
   };
   if (!MissingIn(Snapshot)) {Fail(TEXT("normal GameState album snapshot discarded unavailable marker"));return;}
   if (!Shots.IsEmpty()) FFileHelper::SaveStringToFile(S47J::Write(Snapshot,true),*FPaths::Combine(Shots,TEXT("missing-photo.snapshot.json")));
   US47Flow::FStart Again; Again.State=Snapshot; Again.CaseId=0; Again.bDev=true; Again.Playtime=M->Playtime;
   M->Flow->StartGame(Again);
   const auto ArchiveAgain=S47J::Obj(M->Anomaly->Save(),TEXT("archive"));
   if (!M->Flow->bDevRun || M->Flow->CaseId!=0 || !MissingIn(M->Flow->Carry) || !MissingIn(M->Flow->GameState()) || !SameAlbum(ExpectedAlbum,M->Night->Album->Data) || ArchiveBefore!=S47J::Str(ArchiveAgain,TEXT("url")) || S47J::Bool(ArchiveAgain,TEXT("unavailable")))
   {Fail(TEXT("second full snapshot restore lost marker, metadata or independent archive image"));return;}
   if (M->Anomaly->Assess(TEXT("a02"),TEXT("moved")) || D.A02Compared)
   {Fail(TEXT("snapshot/transition converted missing-photo replacement into valid evidence"));return;}
   if (!M->Anomaly->Assess(TEXT("a02"),TEXT("insufficient")) || !D.A02Compared || D.A02Assessment!=TEXT("insufficient evidence"))
   {Fail(TEXT("unavailable image prevents honest insufficient-evidence assessment"));return;}
   M->Anomaly->Open(TEXT("a02")); Shot(TEXT("missing-photo"));
   Next(95,TEXT("PASS missing film: only fixture changed-photo file removed; fresh disk store + full restore + normal GameState snapshot + second full restore retain id/clock/targets and unavailable=true, archive intact, placeholder rejected before/after transition, insufficient accepted; no foreign-store original-ref preservation claim"),2);return;
  }
  if (!M->Anomaly->Assess(TEXT("a02"),TEXT("moved")) || !D.A02Compared || !D.P01Pending || D.P01Heard) {Fail(TEXT("A02 assessment or deferred P01 after modal invalid"));return;}
  if (!RoundTrip(TEXT("a02-compared"))) return;
  M->Anomaly->Open(TEXT("a02"));
  Next(16,TEXT("A02 active identification saved; P01 must wait while comparison panel is open"),3);return;
 }
 if (Stage==16)
 {
  if (D.P01Heard) {Fail(TEXT("P01 began over active comparison modal"));return;}
  M->UI->Close(true); M->Inter->UseId(TEXT("anomaly:figure")); M->UI->Close(true);
  if (!D.A02Followed) {Fail(TEXT("A02 figure physical follow-up missing"));return;}
  Pose(M->AnomalyWorld->StudioAt()+FVector(-3,0,0),M->AnomalyWorld->StudioAt());
  Next(17,TEXT("Close comparison, inspect actual figure, approach closed studio"),1);return;
 }
 if (Stage==17)
 {
  if (!D.P01Heard) return;
  if (M->Story->Clock()>=4*3600+55*60) {Fail(TEXT("pilot missed retained 04:55 light-wave boundary"));return;}
  Next(18,TEXT("P01 printer rhythm triggered only after comparison, at closed studio; listening UNVERIFIED"),8);return;
 }
 if (Stage==18)
 {
  M->Inter->UseId(TEXT("anomaly:studio")); M->UI->Close(true);
  if (!D.P01Followed || !D.P01Heard || D.P01Pending) {Fail(TEXT("P01 facade follow-up/state incomplete"));return;}
  if (Route==TEXT("missing-phone") && M->TownSeen.Contains(TEXT("ev:phone"))) {Fail(TEXT("optional telephone 1 fabricated or started by pilot"));return;}
  if (!RoundTrip(TEXT("p01-followed"))) return;
  M->Flow->OpenTestView(TEXT("notebook:findings"));
  // Exercise the notebook's ordinary selection keys; the new P01 entry is last.
  TArray<FString> Down; for(int32 I=0;I<32;++I) Down.Add(TEXT("Down"));
  S47Keys::TestPresses(FString::Join(Down,TEXT(",")),.02f);
  FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float){Shot(TEXT("notebook"));return false;}),.8f);
  Next(19,TEXT("Notebook findings after reload; no anomaly quota required for P13"),2);return;
 }
 if (Stage==19)
 {
  if (!D.N01Routine || !D.N01Compared || !D.N01Followed || !D.N02Compared || !D.A02Compared || !D.A02Followed || !D.P01Followed || D.ReferencePhoto.IsEmpty() || D.ChangedPhoto.IsEmpty()) {Fail(TEXT("completed pilot missing required investigation state"));return;}
  Finish();return;
 }
 if (Stage==90)
 {
  if (D.N01Compared || D.N02Compared || D.A02Compared || D.P01Heard || D.P01Followed) {Fail(TEXT("old/late migration invented completed investigations"));return;}
  if (Route==TEXT("legacy") && M->Story->bEveningTimeline) {Fail(TEXT("old case silently changed historical timeline"));return;}
  if (Route==TEXT("already-seen") && (!M->TownSeen.Contains(TEXT("ev:cutout")) || !M->TownSeen.Contains(TEXT("ev:wave")) || !D.bLegacyCutout || D.A02Moved)) {Fail(TEXT("existing town event history lost"));return;}
  M->Anomaly->Use(TEXT("anomaly:dated")); M->UI->Close(true);
  if (!M->Anomaly->Assess(TEXT("a02"),TEXT("insufficient")) || !D.A02Compared || D.P01Heard) {Fail(TEXT("late/missing baseline cannot close as insufficient without random event or Earl"));return;}
  if (!RoundTrip(TEXT("late-insufficient"))) return;
  Next(91,TEXT("PASS late/migration/previous-event fallback: insufficient evidence accepted, Earl not summoned, old history retained"),1);return;
 }
 if (Stage==91 || Stage==95) Finish();
 if (Stage==92)
 {
  M->Story->SetClock(-24600); // The authored exposure's real clock, not a rewritten caption.
  if (!Photograph(TEXT("anomaly:reference"))) return;
  if (D.ReferenceClock!=-24600 || M->Night->Album->Shots().Num()!=1) {Fail(TEXT("archive capture did not retain its actual 17:10 exposure"));return;}
  Shot(TEXT("archive")); Next(93,TEXT("Actual day-operator 17:10 source rendered and reference-photo.jpg flushed"),2);return;
 }
 if (Stage==93)
 {
  if (Shots.IsEmpty() || IFileManager::Get().FileSize(*FPaths::Combine(Shots,TEXT("reference-photo.jpg")))<1000) {Fail(TEXT("archive JPEG absent or empty"));return;}
  Finish();
 }
}
