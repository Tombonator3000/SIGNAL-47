#include "S47DevEvents.h"
#include "S47Main.h"
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
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47DevEvents, Log, All);

float GS47DevFogDensity = -1.f;

namespace
{
 constexpr double ShowerTime = 40, PaleStand = 7;
}

FS47DevEvents::FS47DevEvents(US47Main* InM) : M(InM) { AddBuiltIn(); }

void FS47DevEvents::Add(const FString& Group, const FString& Label, const FString& Note, TFunction<void()> Fn)
{
 List.Add({ Group, Label, Note, MoveTemp(Fn) });
}

void FS47DevEvents::AddBuiltIn()
{
 const auto Say = [this](const FString& What) { if (M && M->UI) M->UI->Toast(TEXT("Developer event: ") + What, 3); UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer event %s"), *What); };
 // ---- aliens ----
 Add(TEXT("Aliens"), TEXT("Pale body, here"), TEXT("stands ahead of you, then goes"), [this, Say] { SpawnPale(); Say(TEXT("pale body")); });
 Add(TEXT("Roswell aliens"),TEXT("Number: 1"),TEXT("cycle 1, 2, 3 or 6 individuals"),[this]
 {
  RoswellNumber=RoswellNumber==1?2:RoswellNumber==2?3:RoswellNumber==3?6:1;
  for (auto& E:List) if (E.Group==TEXT("Roswell aliens") && E.Label.StartsWith(TEXT("Number:"))) E.Label=FString::Printf(TEXT("Number: %d"),RoswellNumber);
  Open();
 });
 Add(TEXT("Roswell aliens"),TEXT("Stand for 8 seconds"),TEXT("spawn the selected number, then remove them"),[this]{SpawnRoswell(0);});
 Add(TEXT("Roswell aliens"),TEXT("Curious: stalk and hide"),TEXT("spawn shy observers; listen, peek and flee when noticed"),[this]{SpawnRoswell(1);});
 Add(TEXT("Roswell aliens"),TEXT("Hunt: chase the player"),TEXT("dev pursuit and grab pose; no story damage"),[this]{SpawnRoswell(2);});
 Add(TEXT("Roswell aliens"),TEXT("Change active group to curious"),TEXT("keep their positions; stop the test hunt"),[this]
 { if (M->Night && M->Night->Glimpses) M->Night->Glimpses->DevMode(FS47RoswellBrain::EMode::Curious); });
 Add(TEXT("Roswell aliens"),TEXT("Change active group to hunt"),TEXT("keep their positions; follow sight and heard footsteps"),[this]
 { if (M->Night && M->Night->Glimpses) M->Night->Glimpses->DevMode(FS47RoswellBrain::EMode::Hunt); });
 Add(TEXT("Roswell aliens"),TEXT("Roof: curious footsteps"),TEXT("actual bodies on the Annex roof, with muffled overhead Foley"),[this]{SpawnRoswell(1,true);});
 Add(TEXT("Roswell aliens"),TEXT("Clear Roswell group"),TEXT("remove bodies and their sounds; return to natural encounters"),[this]
 { if (M->Night && M->Night->Glimpses) M->Night->Glimpses->DevClear(); });
 // ---- sky ----
 Add(TEXT("Sky"), TEXT("Meteor"), TEXT("one, now"), [this, Say] { if (M->Life && M->Life->Sky) M->Life->Sky->MeteorNow(); Say(TEXT("meteor")); });
 Add(TEXT("Sky"), TEXT("Meteor shower"), TEXT("40 seconds"), [this, Say] { ShowerLeft = ShowerTime; ShowerNext = 0; Say(TEXT("meteor shower")); });
 for (int32 S = 1; S <= 5; ++S)
  Add(TEXT("Sky"), FString::Printf(TEXT("Sky lights, stage %d"), S), S == 1 ? TEXT("one pale light") : S == 5 ? TEXT("the last") : TEXT("more of them"),
   [this, Say, S] { if (M->SkyVisitors) M->SkyVisitors->DevStage(S); Say(FString::Printf(TEXT("sky lights stage %d"), S)); });
 Add(TEXT("Sky"), TEXT("Sky lights, by the clock"), TEXT("back to the schedule"), [this, Say] { if (M->SkyVisitors) M->SkyVisitors->DevStage(0); Say(TEXT("sky lights by the clock")); });
 Add(TEXT("Tripo craft preview"),TEXT("Triangular craft: source model"),TEXT("outdoor model preview ahead; story/finale unchanged"),[this]{SpawnCraft(false);});
 Add(TEXT("Tripo craft preview"),TEXT("Flying saucer: source model"),TEXT("outdoor model preview ahead; story/finale unchanged"),[this]{SpawnCraft(true);});
 Add(TEXT("Tripo craft preview"),TEXT("Remove craft preview"),TEXT("remove the temporary model completely"),[this]{if(auto* C=CraftPreview.Get())C->DestroyComponent();CraftPreview.Reset();});
 // ---- weather ----
 Add(TEXT("Weather"), TEXT("Fog: the night's own"), TEXT("as the scene has it"), [this, Say] { GS47DevFogDensity = -1.f; if (M->Web) UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer fog %.4f, native fog now %.4f"), GS47DevFogDensity, M->Web->RefreshFog()); Say(TEXT("fog as the night has it")); });
 Add(TEXT("Weather"), TEXT("Fog: light"), TEXT("about 100 m"), [this, Say] { GS47DevFogDensity = .04f; if (M->Web) UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer fog %.4f, native fog now %.4f"), GS47DevFogDensity, M->Web->RefreshFog()); Say(TEXT("light fog")); });
 Add(TEXT("Weather"), TEXT("Fog: medium"), TEXT("about 40 m"), [this, Say] { GS47DevFogDensity = .18f; if (M->Web) UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer fog %.4f, native fog now %.4f"), GS47DevFogDensity, M->Web->RefreshFog()); Say(TEXT("medium fog")); });
 Add(TEXT("Weather"), TEXT("Fog: heavy"), TEXT("about 15 m"), [this, Say] { GS47DevFogDensity = 1.5f; if (M->Web) UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer fog %.4f, native fog now %.4f"), GS47DevFogDensity, M->Web->RefreshFog()); Say(TEXT("heavy fog")); });
 // ---- time ----
 Add(TEXT("Time"), TEXT("Clock +15 minutes"), TEXT("the night's clock only"), [this, Say] { if (M->Story) M->Story->SetClock(M->Story->Clock() + 15 * 60); Say(TEXT("clock +15 min")); });
}

void FS47DevEvents::Open()
{
 if (!M || !M->UI || M->Mode != US47Main::EMode::Play || M->UI->Modal()) return;
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

void FS47DevEvents::SpawnRoswell(int32 Mode,bool bRoof)
{
 if (!M || !M->Night || !M->Night->Glimpses) return;
 const auto Behaviour=Mode==0?FS47RoswellBrain::EMode::Stand:Mode==2?FS47RoswellBrain::EMode::Hunt:FS47RoswellBrain::EMode::Curious;
 const int32 Made=M->Night->Glimpses->DevSpawn(RoswellNumber,Behaviour,bRoof);
 if (M->UI) M->UI->Toast(FString::Printf(TEXT("Roswell: %d of %d placed on free %s. Behaviour: %s."),Made,RoswellNumber,bRoof?TEXT("roof"):TEXT("floor"),FS47RoswellBrain::ModeName(Behaviour)),4);
 UE_LOG(LogS47DevEvents,Display,TEXT("S47: Roswell developer spawn count=%d requested=%d mode=%s roof=%d"),Made,RoswellNumber,FS47RoswellBrain::ModeName(Behaviour),bRoof?1:0);
}

void FS47DevEvents::SpawnCraft(bool bSaucer)
{
 if(!M||!M->Web||!M->Player)return;
 const TCHAR* Id=bSaucer?TEXT("ufo_saucer"):TEXT("ufo_triangle");
 const TCHAR* Name=bSaucer?TEXT("SM_TripoFleet_Saucer"):TEXT("SM_TripoFleet_Triangle");
 const FString Path=FString::Printf(TEXT("/Game/Licensed/TripoFleet/%s/%s.%s"),Id,Name,Name);
 UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
 if(!Mesh){if(M->UI)M->UI->Toast(TEXT("Tripo source model is not installed in this build."),4);return;}
 if(auto* Previous=CraftPreview.Get())Previous->DestroyComponent();
 auto* C=NewObject<UStaticMeshComponent>(M->Web.Get());C->SetStaticMesh(Mesh);C->SetMobility(EComponentMobility::Movable);
 C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(true);C->RegisterComponent();
 const FVector P=M->Player->Pos;const double Yaw=M->Player->Yaw;
 const FVector At(P.X-FMath::Sin(Yaw)*18,P.Y+7.5,P.Z-FMath::Cos(Yaw)*18);
 C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldRotation(FRotator::ZeroRotator);CraftPreview=C;
 if(M->UI)M->UI->Toast(TEXT("Tripo craft preview ahead. View outdoors; F10 removes it. Nothing is saved."),5);
 UE_LOG(LogS47DevEvents,Display,TEXT("S47: temporary Tripo craft preview %s at%s; no story/finale replacement"),Name,*At.ToString());
}

void FS47DevEvents::SpawnPale()
{
 if (!M || !M->Web || !M->Player) return;
 if (!Pale.bBuilt && !Pale.Build(M->Web.Get(), TEXT("#root"), TEXT("DevPale"))) return;
 // Four metres ahead where the walker could stand, else as near as there is room; facing you.
 const FVector P = M->Player->Pos;
 // Ahead first, then turning further round, where the walker could stand and you can see it.
 const FVector Eye(P.X, P.Y + 1.62, P.Z);
 TOptional<FVector2D> Found; double FoundY = P.Y;
 for (const double Turn : {0.0, .5, -.5, 1.0, -1.0, 1.6, -1.6, double(PI)})
  for (const double D : {4.0, 3.0, 5.5, 2.3})
  {
   if (Found) break;
   const double Yaw = M->Player->Yaw + Turn;
   const FVector2D C(P.X - FMath::Sin(Yaw) * D, P.Z - FMath::Cos(Yaw) * D);
   if (!M->Player->Walkable(C.X, C.Y)) continue;
   FHitResult Block; FCollisionQueryParams Sight(SCENE_QUERY_STAT(S47DevPaleSight), true);
   const FVector A = S47Three::ToUE(Eye), B = S47Three::ToUE(C.X, P.Y + 1.4, C.Y);
   if (M->Web->GetWorld()->LineTraceSingleByChannel(Block, A, B, ECC_Visibility, Sight) && Block.Distance < FVector::Dist(A, B) - 20) continue;
   // its floor, not a desk or a cabinet: the ground under it within a step of yours
   FHitResult Floor; FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevPaleFloor), true);
   if (!M->Web->GetWorld()->LineTraceSingleByChannel(Floor, S47Three::ToUE(C.X, P.Y + 2.2, C.Y), S47Three::ToUE(C.X, P.Y - 3, C.Y), ECC_Visibility, Under)) continue;
   const double G = S47Three::ToThree(Floor.ImpactPoint).Y;
   if (FMath::Abs(G - P.Y) > .45) continue;
   Found = C; FoundY = G;
  }
 if (!Found) { if (M->UI) M->UI->Toast(TEXT("Developer event: no free floor in sight for the body here."), 3); return; }
 const FVector2D At = *Found;
 PaleAt = FVector(At.X, FoundY, At.Y);
 PaleYaw = FMath::Atan2(P.Z - At.Y, P.X - At.X);
 PaleLeft = PaleStand; PaleClock = 0;
 if (M->Audio) M->Audio->Gasp(M->Audio->Spatial(PaleAt + FVector(0, 1.8, 0), 2.f));
}

void FS47DevEvents::Step(double Dt)
{
 if (!M) return;
 if (M->Night && M->Night->Glimpses) M->Night->Glimpses->DevStep(Dt,M->Night->GlimpseIn(true));
 const bool bPlay = M->Mode == US47Main::EMode::Play && !M->bPausedByMenu;
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

// -S47DevEventsContract=/tmp/dir: every event in turn through the menu's own functions, with a
// picture of the body and of each fog, then exit. Not a judgement of how they look.
void FS47DevEvents::ControlTest(double Dt)
{
 if (!bTestInit) { bTestInit = true; FParse::Value(FCommandLine::Get(), TEXT("S47DevEventsContract="), TestDir); }
 if (TestDir.IsEmpty() || bTestDone || !M || M->Mode != US47Main::EMode::Play) return;
 FString OnlyGroup; FParse::Value(FCommandLine::Get(),TEXT("S47DevEventsGroup="),OnlyGroup);
 TArray<const FEvent*> Events;
 for (const FEvent& Event:List) if (OnlyGroup.IsEmpty() || Event.Group==OnlyGroup) Events.Add(&Event);
 TestT += Dt;
 // a picture a moment after its event, once the scene has taken it
 if (!ShotName.IsEmpty() && TestT >= ShotAt) { FScreenshotRequest::RequestScreenshot(TestDir / (ShotName + TEXT(".png")), false, false); ShotName.Reset(); }
 if (TestT < 4 + TestStep * 2.5) return;
 if (TestStep < Events.Num())
 {
  const FEvent& Ev = *Events[TestStep];
  if (TestStep==0 && OnlyGroup==TEXT("Tripo craft preview"))
  {
   // These large exterior assets cannot be judged through the control-room roof.
   // The opt-in, isolated test uses the same ordinary spawn functions outdoors.
   M->World->Enter(TEXT("saro")); M->Player->Place(7,19,UE_DOUBLE_PI); M->Player->Pitch=0;
   M->Story->SetClock(12*3600); M->Story->SetPaused(true); M->Story->SetCinematic(true);
  }
  UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer events contract %d/%d: %s / %s"), TestStep + 1, Events.Num(), *Ev.Group, *Ev.Label);
  if (Ev.Fn) Ev.Fn();
  const bool Craft=Ev.Group==TEXT("Tripo craft preview"),Remove=Ev.Label==TEXT("Remove craft preview");
  if (Craft && (CraftPreview.IsValid()==Remove))
  { bTestDone=true; UE_LOG(LogS47DevEvents,Error,TEXT("S47: developer craft contract FAIL: native preview creation/removal")); FPlatformMisc::RequestExit(false); return; }
  if (Craft && !Remove && M->Player)
  { const FVector D=S47Three::ToThree(CraftPreview->GetComponentLocation())-M->Player->Pos-FVector(0,M->Player->Eye,0); M->Player->Yaw=FMath::Atan2(-D.X,-D.Z); M->Player->Pitch=FMath::Atan2(D.Y,FVector2D(D.X,D.Z).Size()); }
  if (Ev.Label.StartsWith(TEXT("Pale")) || Ev.Label.StartsWith(TEXT("Fog")) || Ev.Label == TEXT("Meteor shower") || (Craft && !Remove))
  {
   // the body: the camera turned to it for its picture
   if (Ev.Label.StartsWith(TEXT("Pale")) && M->Player && PaleLeft > 0)
   { const FVector D = PaleAt - M->Player->Pos; M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = 0; }
   ShotName = Ev.Label.Replace(TEXT(" "), TEXT("-")).Replace(TEXT(":"), TEXT("")).Replace(TEXT(","), TEXT("")).Replace(TEXT("'"), TEXT("")).ToLower(), ShotAt = TestT + 1.5;
  }
  ++TestStep; return;
 }
 bTestDone = true;
 const bool bPale = Pale.bBuilt,bPassed=OnlyGroup.IsEmpty()?bPale:Events.Num()>0;
 UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer events contract %s COMPLETE: %d events run, pale body %s, fog %.3f"), bPassed ? TEXT("PASS") : TEXT("FAIL"), TestStep, bPale ? TEXT("built") : TEXT("missing"), GS47DevFogDensity);
 FPlatformMisc::RequestExit(false);
}

void FS47DevEvents::Stop()
{
 if(auto* C=CraftPreview.Get())C->DestroyComponent();CraftPreview.Reset();
 if (M && M->Night && M->Night->Glimpses) M->Night->Glimpses->DevClear();
 ShowerLeft = 0; PaleLeft = 0; GS47DevFogDensity = -1.f;
 Pale.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk);
}
