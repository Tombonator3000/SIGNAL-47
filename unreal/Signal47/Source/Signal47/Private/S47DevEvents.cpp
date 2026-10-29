#include "S47DevEvents.h"
#include "S47Main.h"
#include "S47Flow.h"
#include "S47Startup.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Night.h"
#include "S47Glimpses.h"
#include "S47Sky.h"
#include "S47SkyVisitors.h"
#include "S47WorldLife.h"
#include "S47Three.h"
#include "SS47Dev.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DevEvents, Log, All);

float GS47DevFogDensity = -1.f;

namespace
{
 constexpr double ShowerTime = 40, PaleStand = 7;
 bool DevPng(const FString& Path,S47J::FObj& Out)
 {
  TArray<uint8> Bytes; if (!FFileHelper::LoadFileToArray(Bytes,*Path)) return false;
  const uint8 Signature[]={137,80,78,71,13,10,26,10}; if (Bytes.Num()<33 || FMemory::Memcmp(Bytes.GetData(),Signature,8)) return false;
  const auto U32=[&](int64 At){return (uint32(Bytes[At])<<24)|(uint32(Bytes[At+1])<<16)|(uint32(Bytes[At+2])<<8)|Bytes[At+3];};
  int64 At=8,Data=0; int32 Chunks=0; uint32 Width=0,Height=0; bool End=false;
  while (At+12<=Bytes.Num())
  {
   const uint32 Size=U32(At); if (int64(Size)>Bytes.Num()-At-12) return false; const uint8* Type=Bytes.GetData()+At+4;
   if (FCrc::MemCrc32(Type,Size+4)!=U32(At+8+Size)) return false;
   if (!Chunks) { if (FMemory::Memcmp(Type,"IHDR",4)||Size!=13) return false; Width=U32(At+8); Height=U32(At+12); }
   if (!FMemory::Memcmp(Type,"IDAT",4)) Data+=Size; ++Chunks; At+=int64(Size)+12;
   if (!FMemory::Memcmp(Type,"IEND",4)) { End=Size==0; break; }
  }
  if (!End || At!=Bytes.Num() || !Data || Width<320 || Height<200) return false;
  FSHAHash Hash; FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num(),Hash.Hash); Out=S47J::New();
  S47J::Set(Out,TEXT("path"),Path); S47J::Set(Out,TEXT("bytes"),Bytes.Num()); S47J::Set(Out,TEXT("sha1"),Hash.ToString());
  S47J::Set(Out,TEXT("width"),double(Width)); S47J::Set(Out,TEXT("height"),double(Height)); S47J::Set(Out,TEXT("crc_iend_eof"),true); return true;
 }
}

FS47DevEvents::FS47DevEvents(US47Main* InM) : M(InM) { AddBuiltIn(); }

void FS47DevEvents::Add(const FString& Group, const FString& Label, const FString& Note, TFunction<void()> Fn)
{
 List.Add({ Group, Label, Note, [this,Action=MoveTemp(Fn)]
 {
  LastActionOK=false; LastActionReport=S47J::New();
  if (!Allowed()) return;
  if (Action) Action();
 } });
}

bool FS47DevEvents::Allowed() const
{
 return M && M->Flow && M->Mode==US47Main::EMode::Play && M->Flow->bDevRun && M->Flow->CaseId==0 && !M->Flow->bStarting;
}

void FS47DevEvents::AddBuiltIn()
{
 const auto Say = [this](const FString& What) { if (M && M->UI) M->UI->Toast(TEXT("Developer event: ") + What, 3); UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer event %s"), *What); };
 // ---- aliens ----
 Add(TEXT("Aliens"), TEXT("Pale body, here"), TEXT("stands ahead of you, then goes"), [this, Say] { LastActionOK=SpawnPale(); Say(LastActionOK?TEXT("pale body"):TEXT("pale body: no measured free floor in sight")); });
 Add(TEXT("Aliens"), TEXT("Glimpse"), TEXT("a figure at the edge of sight"), [this, Say]
 {
  if (!M->Night || !M->Night->Glimpses) return;
  const bool bOk = M->Night->Glimpses->Now(M->Night->GlimpseIn(true), FString()); LastActionOK=bOk && M->Night->Glimpses->Visible();
  Say(bOk ? TEXT("glimpse") : TEXT("glimpse: no place for one here"));
 });
 // ---- sky ----
 Add(TEXT("Sky"), TEXT("Meteor"), TEXT("one, now"), [this, Say]
 {
  if (!M->Life || !M->Life->Sky) return;
  const int32 Before=M->Life->Sky->Meteors; M->Life->Sky->MeteorNow(); LastActionOK=M->Life->Sky->Meteors>Before;
  S47J::Set(LastActionReport,TEXT("meteors_before"),Before); S47J::Set(LastActionReport,TEXT("meteors_after"),M->Life->Sky->Meteors); Say(TEXT("meteor"));
 });
 Add(TEXT("Sky"), TEXT("Meteor shower"), TEXT("40 seconds"), [this, Say] { ShowerLeft = ShowerTime; ShowerNext = 0; LastActionOK=M->Life && M->Life->Sky; Say(TEXT("meteor shower")); });
 for (int32 S = 1; S <= 5; ++S)
  Add(TEXT("Sky"), FString::Printf(TEXT("Sky lights, stage %d"), S), S == 1 ? TEXT("one pale light") : S == 5 ? TEXT("the last") : TEXT("more of them"),
   [this, Say, S] { if (M->SkyVisitors) { M->SkyVisitors->DevStage(S); LastActionOK=M->SkyVisitors->DevActive(); } Say(FString::Printf(TEXT("sky lights stage %d"), S)); });
 Add(TEXT("Sky"), TEXT("Sky lights, by the clock"), TEXT("back to the schedule"), [this, Say]
 {
  if (!M->SkyVisitors) return;
  const auto Before=M->SkyVisitors->Save(); M->SkyVisitors->DevStage(0);
  LastActionOK=!M->SkyVisitors->DevActive() && FJsonValue::CompareEqual(FJsonValueObject(Before),FJsonValueObject(M->SkyVisitors->Save()));
  S47J::Set(LastActionReport,TEXT("saved_sky_before"),Before); S47J::Set(LastActionReport,TEXT("restored_sky"),M->SkyVisitors->Save()); Say(TEXT("sky lights by the clock"));
 });
 // ---- weather ----
 const auto Fog=[this,Say](float Density,const TCHAR* Name)
 {
  GS47DevFogDensity=Density;
  if (M->Web) M->Web->RefreshFog();
  const auto* Actual=M->Web?M->Web->LightingFog.Get():nullptr;
  LastActionOK=Actual && (Density<0 ? Actual->FogDensity>0 : FMath::IsNearlyEqual(Actual->FogDensity,Density,1e-6f));
  S47J::Set(LastActionReport,TEXT("actual_native_fog_density"),Actual?double(Actual->FogDensity):-1.); Say(Name);
 };
 Add(TEXT("Weather"), TEXT("Fog: the night's own"), TEXT("as the scene has it"), [Fog] { Fog(-1.f,TEXT("fog as the night has it")); });
 Add(TEXT("Weather"), TEXT("Fog: light"), TEXT("haze"), [Fog] { Fog(.006f,TEXT("light fog")); });
 Add(TEXT("Weather"), TEXT("Fog: medium"), TEXT("the far dishes go"), [Fog] { Fog(.02f,TEXT("medium fog")); });
 Add(TEXT("Weather"), TEXT("Fog: heavy"), TEXT("a few metres"), [Fog] { Fog(.06f,TEXT("heavy fog")); });
 // ---- time ----
 Add(TEXT("Time"), TEXT("Clock +15 minutes"), TEXT("the developer case's clock only"), [this, Say]
 {
  if (!M->Story) return; const double Before=M->Story->Clock(); M->Story->SetClock(Before+15*60);
  LastActionOK=FMath::Abs(M->Story->Clock()-Before-900)<.001;
  S47J::Set(LastActionReport,TEXT("clock_before"),Before); S47J::Set(LastActionReport,TEXT("clock_after"),M->Story->Clock()); Say(TEXT("clock +15 min"));
 });
}

void FS47DevEvents::Open()
{
 if (!M || !M->UI || M->Mode != US47Main::EMode::Play || M->UI->Modal()) return;
 if (!Allowed()) { M->UI->Toast(TEXT("Events are available in a developer start from the title. Your current case is unchanged."),4); return; }
 TArray<FS47DevEntry> E;
 for (const FEvent& Ev : List)
 {
  const TFunction<void()> Fn = Ev.Fn;
  E.Add({ Ev.Group, Ev.Label, Ev.Note, [this, Fn] { if (M && M->UI) M->UI->Close(true); if (Fn) Fn(); } });
 }
 TSharedRef<SS47Dev> El = SNew(SS47Dev).Entries(E).Title(TEXT("DEVELOPER / EVENTS"))
  .Note(TEXT("Make something happen here, now. Nothing is saved. Escape: back to the night."))
  .OnBack([this] { if (M && M->UI) M->UI->Close(true); });
 M->UI->Open(El);
 FSlateApplication::Get().SetKeyboardFocus(El);
}

bool FS47DevEvents::SpawnPale()
{
 if (!Allowed() || !M->Web || !M->Player || !M->Web->GetWorld()) return false;
 if (!Pale.bBuilt && !Pale.Build(M->Web.Get(), TEXT("#root"), TEXT("DevPale"))) return false;
 // Four metres ahead where the walker could stand, else as near as there is room; facing you.
 const FVector P = M->Player->Pos;
 // Ahead first, then turning further round, where the walker could stand and you can see it.
 const FVector Eye=M->Player->EyeUE(); const double Floor=M->Player->FloorY;
 TOptional<FVector> Found; TArray<TSharedPtr<FJsonValue>> Queries;
 for (const double Turn : {0.0, .5, -.5, 1.0, -1.0, 1.6, -1.6, double(PI)})
  for (const double D : {4.0, 3.0, 5.5, 2.3})
  {
   if (Found) break;
   const double Yaw = M->Player->Yaw + Turn;
   const FVector2D C(P.X - FMath::Sin(Yaw) * D, P.Z - FMath::Cos(Yaw) * D);
   if (!M->Player->Walkable(C.X, C.Y)) continue;
   bool Free=true; const double Radius=M->Player->Radius;
   for (const auto& R:M->Web->Colliders()) if (R.bEnabled && C.X+Radius>R.MinX && C.X-Radius<R.MaxX && C.Y+Radius>R.MinZ && C.Y-Radius<R.MaxZ) { Free=false; break; }
   if (!Free) continue;
   FCollisionQueryParams Ground(SCENE_QUERY_STAT(S47DevPaleFloor),true);
   for (const auto& Part:Pale.All) if (Part.IsValid()) Ground.AddIgnoredComponent(Part.Get());
   FHitResult Hit; bool FloorHit=false;
   for (int32 Probe=0;Probe<32;++Probe)
   {
    if (!M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(C.X,Floor+2,C.Y),S47Three::ToUE(C.X,Floor-4,C.Y),ECC_Visibility,Ground)) break;
    const auto* Component=Hit.GetComponent(); if (!Component) break;
    const int32 Index=M->Web->NodeOf(Component); const auto* Node=M->Web->Node(Index);
    const bool Shown=Component->IsVisible() && !Component->bHiddenInGame && !M->Web->IsHidden() && (Index<0 || M->Web->ObjAt(Index).Shown());
    const bool Proxy=Node && Node->Name.StartsWith(TEXT("proxy"));
    auto Q=S47J::New(); S47J::Set(Q,TEXT("candidate_xz"),TArray<double>{C.X,C.Y}); S47J::Set(Q,TEXT("component"),Component->GetPathName());
    S47J::Set(Q,TEXT("node"),Index); S47J::Set(Q,TEXT("shown"),Shown); S47J::Set(Q,TEXT("proxy"),Proxy);
    S47J::Set(Q,TEXT("impact_ue_cm"),TArray<double>{Hit.ImpactPoint.X,Hit.ImpactPoint.Y,Hit.ImpactPoint.Z}); S47J::Set(Q,TEXT("normal_z"),Hit.ImpactNormal.Z);
    Queries.Add(MakeShared<FJsonValueObject>(Q)); LastActionReport->SetArrayField(TEXT("floor_queries"),Queries);
    if (!Shown || Proxy) { Ground.AddIgnoredComponent(Component); continue; }
    FloorHit=Hit.bBlockingHit && Hit.ImpactNormal.Z>.7; break;
   }
   if (!FloorHit) continue;
   const double Y=S47Three::ToThree(Hit.ImpactPoint).Y;
   FHitResult Block; FCollisionQueryParams Sight(SCENE_QUERY_STAT(S47DevPaleSight), true);
   for (const auto& Part:Pale.All) if (Part.IsValid()) Sight.AddIgnoredComponent(Part.Get());
   const FVector Target = S47Three::ToUE(C.X,Y+1.4,C.Y);
   if (M->Web->GetWorld()->LineTraceSingleByChannel(Block,Eye,Target,ECC_Visibility,Sight) && Block.Distance<FVector::Dist(Eye,Target)-20) continue;
   Found = FVector(C.X,Y,C.Y);
  }
 if (!Found) return false;
 PaleAt = *Found;
 PaleYaw = FMath::Atan2(P.Z - PaleAt.Z, P.X - PaleAt.X);
 PaleLeft = PaleStand; PaleClock = 0;
 S47J::Set(LastActionReport,TEXT("actual_eye_ue_cm"),TArray<double>{Eye.X,Eye.Y,Eye.Z}); S47J::Set(LastActionReport,TEXT("player_floor_y"),Floor);
 S47J::Set(LastActionReport,TEXT("feet_three_m"),TArray<double>{PaleAt.X,PaleAt.Y,PaleAt.Z});
 if (M->Audio) M->Audio->Gasp(M->Audio->Spatial(PaleAt + FVector(0, 1.8, 0), 2.f));
 return true;
}

void FS47DevEvents::Step(double Dt)
{
 if (!M) return;
 if (!Allowed())
 {
  if (PaleLeft>0 || ShowerLeft>0 || GS47DevFogDensity>=0 || (M->SkyVisitors && M->SkyVisitors->DevActive())) Stop();
  return;
 }
 const bool bPlay = Allowed() && !M->bPausedByMenu && !M->ModalOpen() && M->Story && !M->Story->IsCinematic();
 if (ShowerLeft > 0 && bPlay)
 {
  ShowerLeft -= Dt; ShowerNext -= Dt;
  if (ShowerNext <= 0 && M->Life && M->Life->Sky) { M->Life->Sky->MeteorNow(); ShowerNext = .35 + FMath::FRand() * .9; }
 }
 if (PaleLeft > 0)
 {
  if (bPlay) { PaleLeft -= Dt; PaleClock += Dt; }
  // Still for a second, then the head follows you; gone when its time is up.
  double Head = PaleYaw;
  if (PaleClock > 1 && M->Player) Head = FMath::Atan2(M->Player->Pos.Z - PaleAt.Z, M->Player->Pos.X - PaleAt.X);
  Pale.Apply(PaleLeft > 0, PaleAt, PaleYaw, Head, 0, 0, PaleClock, FS47PaleBody::EPose::Listen);
 }
}

S47J::FObj FS47DevEvents::Readback() const
{
 auto O=S47J::New(); S47J::Set(O,TEXT("allowed"),Allowed());
 S47J::Set(O,TEXT("clock"),M && M->Story?M->Story->Clock():0.);
 S47J::Set(O,TEXT("meteors"),M && M->Life && M->Life->Sky?M->Life->Sky->Meteors:-1);
 S47J::Set(O,TEXT("shower_left"),ShowerLeft); S47J::Set(O,TEXT("pale_left"),PaleLeft);
 S47J::Set(O,TEXT("actual_body_rendered"),Pale.Rendered()); S47J::Set(O,TEXT("pale_feet_three_m"),TArray<double>{PaleAt.X,PaleAt.Y,PaleAt.Z});
 S47J::Set(O,TEXT("fog_override"),double(GS47DevFogDensity));
 const auto* Fog=M && M->Web?M->Web->LightingFog.Get():nullptr;
 S47J::Set(O,TEXT("native_fog_density"),Fog?double(Fog->FogDensity):-1.);
 if (M && M->SkyVisitors)
 {
  S47J::Set(O,TEXT("sky_preview"),M->SkyVisitors->DevActive()); S47J::Set(O,TEXT("actual_sky"),M->SkyVisitors->Data().Save());
  S47J::Set(O,TEXT("persisted_sky"),M->SkyVisitors->Save()); int32 Visible=0;
  for (const auto& Point:M->SkyVisitors->Points) if (const auto* C=Point.Get())
   if (C->IsRegistered() && C->IsVisible() && !C->bHiddenInGame && !C->Elements.IsEmpty() && C->Elements[0].Material) ++Visible;
  S47J::Set(O,TEXT("actual_visible_sky_components"),Visible);
 }
 return O;
}

void FS47DevEvents::FinishTest(bool Good,const FString& Reason)
{
 if (bTestDone) return; bTestDone=true;
 auto O=S47J::New(); S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL")); S47J::Set(O,TEXT("reason"),Reason);
 S47J::Set(O,TEXT("events"),TestStep); S47J::Set(O,TEXT("required_events"),List.Num()); S47J::Set(O,TEXT("actual"),Readback());
 O->SetArrayField(TEXT("actions"),TestEvidence); TArray<TSharedPtr<FJsonValue>> Pictures;
 for (const auto& File:TestPictures)
 {
  S47J::FObj P; if (!DevPng(File,P)) { Good=false; S47J::Set(O,TEXT("png_failure"),File); break; }
  Pictures.Add(MakeShared<FJsonValueObject>(P));
 }
 O->SetArrayField(TEXT("pictures"),Pictures); S47J::Set(O,TEXT("limits"),TEXT("Isolated developer-case API callbacks and actual delayed native component/clock readback. One valid visible floor and current pale torso render; six PNG CRC/IEND/SHA receipts. This is not F10/Slate keyboard-input, human visual/listening/FPS or story-progress acceptance."));
 Stop(); S47J::Set(O,TEXT("after_stop"),Readback()); S47J::Set(O,TEXT("status"),Good?TEXT("PASS"):TEXT("FAIL"));
 const bool Written=!TestDir.IsEmpty() && FFileHelper::SaveStringToFile(S47J::Write(O,true),*(TestDir/TEXT("result.json")));
 if (Good && Written) UE_LOG(LogS47DevEvents,Display,TEXT("S47: developer events contract PASS COMPLETE: %d actual actions evidence=%s"),TestStep,*TestDir);
 else UE_LOG(LogS47DevEvents,Error,TEXT("S47: developer events contract FAIL: %s (result written %d)"),*Reason,Written);
 FPlatformMisc::RequestExitWithStatus(false,Good && Written?0:1);
}

// Isolated opt-in API contract. Every callback must produce its actual effect, observed after
// ordinary production frames. A built-but-unplaced body or an unwritten image is never a pass.
void FS47DevEvents::ControlTest(double Dt)
{
 if (!bTestInit) { bTestInit = true; FParse::Value(FCommandLine::Get(), TEXT("S47DevEventsContract="), TestDir); }
 if (TestDir.IsEmpty() || bTestDone || !M || !M->Flow || !M->Story || !M->UI || !M->Web) return;
 if (!TestWallBegan)
 {
  if (M->Mode!=US47Main::EMode::Play || M->Flow->bStarting || S47Startup::Blocking() || M->UI->Modal() || M->Story->IsCinematic()) return;
  FString UserDir; const bool Explicit=FParse::Value(FCommandLine::Get(),TEXT("UserDir="),UserDir);
  const bool Plain=!UserDir.Contains(TEXT("..")) && !TestDir.Contains(TEXT(".."));
  UserDir=FPaths::ConvertRelativePathToFull(UserDir); FPaths::NormalizeDirectoryName(UserDir);
  TestDir=FPaths::ConvertRelativePathToFull(TestDir);
  if (!Explicit || !Plain || !Allowed() || !UserDir.StartsWith(TEXT("/tmp/"))
   || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()),UserDir)
   || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(M->Flow->Saves.Dir),UserDir)
   || !FPaths::IsUnderDirectory(TestDir,UserDir) || !M->Flow->Saves.List().IsEmpty())
  { TestDir.Reset(); FinishTest(false,TEXT("explicit fresh /tmp developer profile with Saved/SaveStore/evidence inside it is required")); return; }
  TestDir/=FGuid::NewGuid().ToString(EGuidFormats::Digits);
  if (!IFileManager::Get().MakeDirectory(*TestDir,true)) { FinishTest(false,TEXT("fresh evidence directory could not be created")); return; }
  TestWallBegan=FPlatformTime::Seconds();
 }
 if (!Allowed() || FPlatformTime::Seconds()-TestWallBegan>120) { FinishTest(false,TEXT("developer case lost or 120s timeout")); return; }
 if (M->UI->Modal() || M->Story->IsCinematic() || M->bPausedByMenu) return;
 TestT += Dt;
 if (TestT<TestNext) return;
 if (bTestAwaiting)
 {
  const FEvent& Ev=List[TestStep]; const auto After=Readback(); bool Good=LastActionOK;
  if (Ev.Label.StartsWith(TEXT("Pale"))) Good &= PaleLeft>0 && Pale.Rendered() && !S47J::Arr(LastActionReport,TEXT("floor_queries")).IsEmpty();
  if (Ev.Label==TEXT("Meteor shower")) Good &= S47J::Int(After,TEXT("meteors"))>S47J::Int(LastActionReport,TEXT("meteors_before")) && ShowerLeft>0;
  if (Ev.Label.StartsWith(TEXT("Sky lights, stage ")))
  {
   const int32 Stage=FCString::Atoi(*Ev.Label.Right(1)); const auto Sky=S47J::Obj(After,TEXT("actual_sky"));
   Good &= S47J::Bool(After,TEXT("sky_preview")) && S47J::Int(Sky,TEXT("stage"))==Stage && S47J::Num(Sky,TEXT("elapsed"))>0
    && S47J::Int(After,TEXT("actual_visible_sky_components"))==FS47SkyVisitorState::Count(Stage);
  }
  if (Ev.Label==TEXT("Sky lights, by the clock")) Good &= !S47J::Bool(After,TEXT("sky_preview"));
  if (Ev.Label.StartsWith(TEXT("Fog"))) Good &= S47J::Num(After,TEXT("native_fog_density"))>0
   && (GS47DevFogDensity<0 || FMath::Abs(S47J::Num(After,TEXT("native_fog_density"))-GS47DevFogDensity)<1e-6);
  S47J::Set(LastActionReport,TEXT("group"),Ev.Group); S47J::Set(LastActionReport,TEXT("label"),Ev.Label);
  S47J::Set(LastActionReport,TEXT("delayed_actual"),After); S47J::Set(LastActionReport,TEXT("pass"),Good);
  TestEvidence.Add(MakeShared<FJsonValueObject>(LastActionReport));
  if (!Good) { FinishTest(false,TEXT("actual action/readback failed: ")+Ev.Label); return; }
  if (Ev.Label.StartsWith(TEXT("Pale")) || Ev.Label.StartsWith(TEXT("Fog")) || Ev.Label==TEXT("Meteor shower"))
  {
   const FString File=TestDir/FString::Printf(TEXT("event-%02d.png"),TestStep); TestPictures.Add(File); FScreenshotRequest::RequestScreenshot(File,false,false);
  }
  ++TestStep; bTestAwaiting=false; TestNext=TestT+1; return;
 }
 if (TestStep < List.Num())
 {
  const FEvent& Ev = List[TestStep];
  UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer events contract %d/%d: %s / %s"), TestStep + 1, List.Num(), *Ev.Group, *Ev.Label);
  const auto Before=Readback();
  if (Ev.Fn) Ev.Fn();
  if (!LastActionReport) LastActionReport=S47J::New(); S47J::Set(LastActionReport,TEXT("before"),Before);
  if (Ev.Label==TEXT("Meteor shower")) S47J::Set(LastActionReport,TEXT("meteors_before"),S47J::Int(Before,TEXT("meteors")));
  bTestAwaiting=true; TestNext=TestT+1.5; return;
 }
 FinishTest(TestStep==List.Num() && TestEvidence.Num()==List.Num() && TestPictures.Num()==6,TEXT("all actual event actions, delayed effects and complete PNGs"));
}

void FS47DevEvents::Stop()
{
 ShowerLeft = 0; PaleLeft = 0; GS47DevFogDensity = -1.f;
 Pale.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk);
 if (M && M->SkyVisitors) M->SkyVisitors->DevStage(0);
 if (M && M->Web) M->Web->RefreshFog();
}
