#include "S47DevEvents.h"
#include "S47Freeform.h"
#include "S47FarmSite.h"
#include "S47IndoorDread.h"
#include "S47HighwayUfo.h"
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
#include "S47Flow.h"
#include "S47WorldLife.h"
#include "S47Three.h"
#include "SS47Dev.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

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
 Add(TEXT("Aliens"), TEXT("Tall one: walks up from afar"), TEXT("on today's body until the new Pale; 30 seconds"), [this, Say] { SpawnGait(false); Say(TEXT("tall one")); });
 Add(TEXT("Aliens"), TEXT("Crab: folds down and crawls up"), TEXT("then sideways round you and away; 30 seconds"), [this, Say] { SpawnGait(true); Say(TEXT("crab")); });
 for(int32 Kind=0;Kind<3;++Kind)
 {
  static const TCHAR* const Names[3]={TEXT("Visit and taken: small Roswell"),TEXT("Visit and taken: crab"),TEXT("Visit and taken: tall one")};
  static const TCHAR* const Notes[3]={TEXT("roof steps indoors, flood; about an hour gone, where you stood"),TEXT("along the ground; hours gone, far from here"),TEXT("stands far off, comes on, its light first; most of a night gone, at SARO")};
  Add(TEXT("Aliens"),Names[Kind],Notes[Kind],[this,Say,Kind]
  {
   if(!M||!M->Freeform||!M->Freeform->FarmSite||!M->Player||!M->Web)return;
   auto& Site=*M->Freeform->FarmSite;
   Site.Arm(M->Freeform->Clock(),TEXT("dev"),TEXT("here"),M->Web->Area(),FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z));
   Site.TestVisit(Kind);Say(TEXT("visit"));
  });
 }
 Add(TEXT("Aliens"),TEXT("At the window: small Roswell"),TEXT("indoors at night: rises at a window or door you can see; finds none, says so"),[this,Say]
 {
  const bool bStarted=M&&M->Freeform&&M->Freeform->IndoorDread&&M->Freeform->IndoorDread->DevPeek();
  Say(bStarted?TEXT("looking for a window"):TEXT("no indoors Freeform night here"));
 });
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
 Add(TEXT("Sky"), TEXT("Meteor shower"), TEXT("40 seconds, several at once"), [this, Say]
 {
  ShowerLeft = ShowerTime; ShowerNext = FS47Sky::SHOWER_GAP_MIN;
  // a fresh radiant, and the first few already on their way so it begins as a shower
  if (M->Life && M->Life->Sky) { FS47Sky& Sky = *M->Life->Sky; Sky.ShowerBegin(); Sky.ShowerMeteor(.3); Sky.ShowerMeteor(.15); Sky.ShowerMeteor(0); }
  Say(TEXT("meteor shower"));
 });
 for (int32 S = 1; S <= 5; ++S)
  Add(TEXT("Sky"), FString::Printf(TEXT("Preview sky lights, stage %d"), S), TEXT("visual preview; no natural observation credit"),
   [this, Say, S] { if (M->SkyVisitors) M->SkyVisitors->DevStage(S); Say(FString::Printf(TEXT("sky preview stage %d"), S)); });
 Add(TEXT("Sky"), TEXT("Stop sky preview"), TEXT("return to the natural sky"), [this, Say] { if (M->SkyVisitors) M->SkyVisitors->DevStage(0); Say(TEXT("sky preview stopped")); });
 Add(TEXT("Tripo craft preview"),TEXT("Triangular craft: source model"),TEXT("outdoor model preview ahead; story/finale unchanged"),[this]{SpawnCraft(false);});
 Add(TEXT("Tripo craft preview"),TEXT("Flying saucer: source model"),TEXT("outdoor model preview ahead; story/finale unchanged"),[this]{SpawnCraft(true);});
 Add(TEXT("Tripo craft preview"),TEXT("Remove craft preview"),TEXT("remove the temporary model completely"),[this]{if(auto* C=CraftPreview.Get())C->DestroyComponent();CraftPreview.Reset();});
 for(const bool Follow:{false,true})
  Add(TEXT("UFO encounters"),Follow?TEXT("Highway: light follows you"):TEXT("Highway: sheriff follows light"),TEXT("live saved encounter; drive south on a clear highway"),[this,Follow]
  {
   const bool Started=M&&M->HighwayUfo&&M->HighwayUfo->Force(Follow);
   if(M&&M->UI)M->UI->Toast(Started?TEXT("Highway encounter started. Your steering and brakes remain in control."):TEXT("Drive south on the open highway, clear of junctions and other encounters, then try again."),5);
  });
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
  .Note(TEXT("Developer previews and live encounter controls. Check each entry before starting. Escape: back to the night."))
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

void FS47DevEvents::SpawnGait(bool bCrab)
{
 if (!M || !M->Web || !M->Player) return;
 FS47PaleBody& Body=bCrab?CrabBody:TallBody;
 if (!Body.bBuilt){Body.CreatureAsset=bCrab?S47KindBody::Crab:S47KindBody::Tall;if(!bCrab)Body.Style=ES47PaleStyle::Tall;if(!Body.Build(M->Web.Get(),TEXT("#root"),bCrab?TEXT("DevCrab"):TEXT("DevTall")))return;}
 if(!Body.Creature.Ready()){if(M->UI)M->UI->Toast(TEXT("Developer event: this body needs the private skinned model."),3);return;}
 const FVector P=M->Player->Pos;
 const FVector Eye(P.X,P.Y+1.62,P.Z);
 for(const double Turn:{0.,.5,-.5,1.,-1.,1.6,-1.6,double(PI)})for(const double D:{16.,11.,7.})
 {
  const double Yaw=M->Player->Yaw+Turn;
  const FVector2D C(P.X-FMath::Sin(Yaw)*D,P.Z-FMath::Cos(Yaw)*D);
  FHitResult Block;FCollisionQueryParams Sight(SCENE_QUERY_STAT(S47DevGaitSight),true);
  const FVector A=S47Three::ToUE(Eye),B=S47Three::ToUE(C.X,P.Y+1.,C.Y);
  if(M->Web->GetWorld()->LineTraceSingleByChannel(Block,A,B,ECC_Visibility,Sight))continue;
  FGaitTest& T=bCrab?CrabTest:TallTest;
  T.At=FVector(C.X,P.Y,C.Y);T.Yaw=FMath::Atan2(P.Z-C.Y,P.X-C.X);T.Left=30;T.Clock=0;
  return;
 }
 if(M->UI)M->UI->Toast(TEXT("Developer event: no free ground in sight; try outdoors."),3);
}

void FS47DevEvents::StepGait(bool bCrab,double Dt)
{
 FGaitTest& T=bCrab?CrabTest:TallTest;FS47PaleBody& Body=bCrab?CrabBody:TallBody;
 if(T.Left<=0||!Body.bBuilt||!M||!M->Player||!M->Web)return;
 T.Left-=Dt;T.Clock+=Dt;
 const bool bShow=T.Left>0;
 const FVector2D To(M->Player->Pos.X-T.At.X,M->Player->Pos.Z-T.At.Z);
 const double Dist=To.Size();const double Face=FMath::Atan2(To.Y,To.X);
 const double Fold=bCrab?FMath::Clamp(T.Clock/2.5,0.,1.):0.;
 double Ahead=0,Across=0;
 const bool bAway=T.Left<7;
 if(bAway){T.Yaw=FMath::Lerp(T.Yaw,T.Yaw+FMath::UnwindRadians(Face+PI-T.Yaw),FMath::Min(1.,Dt*3.));Ahead=bCrab?3.:1.9;}
 else
 {
  T.Yaw=T.Yaw+FMath::UnwindRadians(Face-T.Yaw)*FMath::Min(1.,Dt*1.2);
  if(!bCrab||Fold>=1.)
  {
   const double Stop=bCrab?5.:4.;
   if(Dist>Stop)Ahead=bCrab?1.2:.95;else if(bCrab)Across=.9*(FMath::Fmod(T.Clock,10.)<5.?1.:-1.);
  }
 }
 T.At.X+=(FMath::Cos(T.Yaw)*Ahead-FMath::Sin(T.Yaw)*Across)*Dt;
 T.At.Z+=(FMath::Sin(T.Yaw)*Ahead+FMath::Cos(T.Yaw)*Across)*Dt;
 // The ground under it, ignoring its own stand-in parts.
 FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevGaitFloor),true);
 for(const auto& Part:Body.All)if(auto* C=Part.Get())Under.AddIgnoredComponent(C);
 FHitResult Floor;
 if(M->Web->GetWorld()->LineTraceSingleByChannel(Floor,S47Three::ToUE(T.At.X,T.At.Y+2.,T.At.Z),S47Three::ToUE(T.At.X,T.At.Y-3.,T.At.Z),ECC_Visibility,Under))
  T.At.Y=FMath::FInterpTo(T.At.Y,S47Three::ToThree(Floor.ImpactPoint).Y,Dt,12.);
 if(bCrab)Body.ApplyCrab(bShow,T.At,T.Yaw,Face,T.Clock,Fold,bAway);
 else Body.Apply(bShow,T.At,T.Yaw,Face,Ahead>1.5?1.4:Ahead>0?.85:0.,T.Clock*3,T.Clock,bAway?FS47PaleBody::EPose::Chase:FS47PaleBody::EPose::Walk);
}

void FS47DevEvents::Step(double Dt)
{
 if (!M) return;
 if (M->Night && M->Night->Glimpses) M->Night->Glimpses->DevStep(Dt,M->Night->GlimpseIn(true));
 const bool bPlay = M->Mode == US47Main::EMode::Play && !M->bPausedByMenu;
 // The shower's own free-play guard: a menu, an open panel or a held story freezes it (time and image).
 const bool bShowerFree = bPlay && !M->ModalOpen() && !(M->Story && M->Story->IsPaused());
 if (M->Life && M->Life->Sky) M->Life->Sky->bShowerHeld = !bShowerFree;
 if (ShowerLeft > 0 && bShowerFree && Dt > 0)
 {
  ShowerLeft -= Dt; ShowerNext -= Dt;
  // each on a free strip of its own (FS47Sky::ShowerMeteor), so several cross at once; a long frame adds no backlog
  for (int32 N = 0; ShowerNext <= 0 && N < 3; ++N)
  {
   if (M->Life && M->Life->Sky) M->Life->Sky->ShowerMeteor(FMath::Max(0.0, -ShowerNext));
   ShowerNext += FS47Sky::SHOWER_GAP_MIN + FMath::FRand() * (FS47Sky::SHOWER_GAP_MAX - FS47Sky::SHOWER_GAP_MIN);
  }
  if (ShowerNext <= 0) ShowerNext = FS47Sky::SHOWER_GAP_MIN;
 }
 if (bPlay) { StepGait(false,Dt); StepGait(true,Dt); }
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
 // Opt-in extension of this contract: real menu actions and registered billboard
 // readback across ordinary world ticks, independent of the natural Data() getter.
 if (OnlyGroup == TEXT("Sky preview"))
 {
  if (!M->SkyVisitors || !M->World || !M->Player || !M->Story || !M->Web || !M->Flow) return;
  static S47J::FObj Original;
  static FString Natural;
  static TArray<FString> Notes;
  static TArray<FVector> HeldPositions;
  static bool OldPaused = false, OldStoryPaused = false;
  auto Finish = [&](bool Good)
  {
   if (Original)
   {
    M->SkyVisitors->Begin(Original, false);
    M->bPausedByMenu = OldPaused; M->Story->SetPaused(OldStoryPaused);
    Original.Reset();
   }
   bTestDone = true;
   UE_LOG(LogS47DevEvents, Display, TEXT("S47: sky preview contract %s COMPLETE"), Good ? TEXT("PASS") : TEXT("FAIL"));
   FPlatformMisc::RequestExitWithStatus(false, Good ? 0 : 1);
  };
  auto Check = [&](bool Good, const TCHAR* What)
  {
   UE_LOG(LogS47DevEvents, Display, TEXT("S47: sky preview contract %s: %s"), Good ? TEXT("PASS") : TEXT("FAIL"), What);
   if (!Good) Finish(false);
   return Good;
  };
  auto Select = [&](int32 Stage)
  {
   const FString Label = FString::Printf(TEXT("Preview sky lights, stage %d"), Stage);
   const FEvent* Event = List.FindByPredicate([&](const FEvent& E) { return E.Label == Label; });
   if (!Event || !Event->Fn) return false;
   Event->Fn();
   return true;
  };
  auto Rendered = [&](int32 Stage, TArray<FVector>* Positions = nullptr)
  {
   TArray<UMaterialBillboardComponent*> All; M->Web->GetComponents(All);
   int32 Visible = 0;
   for (int32 I = 0; I < 9; ++I)
   {
    const FString Name = FString::Printf(TEXT("SkyVisitor%d"), I);
    const auto* Found = All.FindByPredicate([&](const auto* B) { return B->GetName() == Name; });
    if (!Found || !(*Found)->IsRegistered()) return false;
    const auto* B = *Found;
    const bool Expected = I < FS47SkyVisitorState::Count(Stage);
    if (B->IsVisible() != Expected) return false;
    if (!Expected) continue;
    ++Visible;
    if (B->Elements.IsEmpty() || B->Elements[0].BaseSizeX <= 0 || !B->Elements[0].Material) return false;
    FLinearColor Color;
    if (!B->Elements[0].Material->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Color")), Color) || FMath::Max3(Color.R, Color.G, Color.B) <= 0) return false;
    const FVector At = S47Three::ToThree(B->GetComponentLocation());
    // These tested stages have straight tracks. Actual components must be on
    // the selected stage's first six seconds, not the natural completed SKY04.
    const FVector A = FS47SkyVisitors::WorldPosition(FS47SkyVisitorState::MapPosition(Stage, I, 0), M->World->Area);
    const FVector Z = FS47SkyVisitors::WorldPosition(FS47SkyVisitorState::MapPosition(Stage, I, 6), M->World->Area);
    if (FMath::PointDistToSegment(At, A, Z) > .01) return false;
    if (Positions) Positions->Add(B->GetComponentLocation());
   }
   return Visible == FS47SkyVisitorState::Count(Stage);
  };
  TestT += Dt;
  if (TestT < 4 + TestStep * 2.5) return;
  if (TestStep == 0)
  {
   FString User; int32 Override = 0;
   FParse::Value(FCommandLine::Get(), TEXT("UserDir="), User);
   if (!Check(TestDir.StartsWith(TEXT("/tmp/")) && User.StartsWith(TEXT("/tmp/")) && M->Flow->CaseId == 0 && !M->IsFreeform() && M->Story->ChapterKey == TEXT("ch1")
    && !FParse::Value(FCommandLine::Get(), TEXT("S47SkyVisit="), Override)
    && !FParse::Param(FCommandLine::Get(), TEXT("S47SkyVisitContract")), TEXT("isolated chapter-one Story case zero without conflicting sky contract"))) return;
   Original = S47J::Clone(M->SkyVisitors->Save()); Notes = M->Story->Notes();
   OldPaused = M->bPausedByMenu; OldStoryPaused = M->Story->IsPaused();
   M->bPausedByMenu = false; M->Story->SetPaused(true);
   FS47SkyVisitorState Fixture; Fixture.Begin(nullptr, true);
   Fixture.Advance(4, 2, false); Fixture.Observe(3, 1); Fixture.Advance(4, FS47SkyVisitorState::Duration(4), false);
   M->SkyVisitors->Begin(Fixture.Save(), false); Natural = S47J::Write(M->SkyVisitors->Save());
   if (!Check(Select(2), TEXT("actual F10 stage-two preview action exists"))) return;
  }
  else
  {
   if (!Check(S47J::Write(M->SkyVisitors->Save()) == Natural && M->Story->Notes() == Notes,
    TEXT("preview preserves completed natural stage, observation flags and existing notes"))) return;
   if (TestStep == 1)
   {
    HeldPositions.Reset();
    if (!Check(Rendered(2, &HeldPositions) && M->SkyVisitors->Data().Stage == 4,
     TEXT("two registered luminous billboards use preview tracks while natural Data remains SKY04"))) return;
    M->bPausedByMenu = true;
   }
   else if (TestStep == 2)
   {
    TArray<FVector> Actual;
    if (!Check(Rendered(2, &Actual) && Actual == HeldPositions, TEXT("menu hold freezes actual preview positions and saved natural time"))) return;
    if (!Check(Select(2), TEXT("same F10 stage can be selected again"))) return;
    M->SkyVisitors->Step(0);
    if (!Check(Rendered(0), TEXT("repeated stage restarts its own zero-time fade rather than continuing the old pass"))) return;
    M->bPausedByMenu = false;
   }
   else if (TestStep == 3)
   {
    if (!Check(Rendered(2) && Select(5), TEXT("repeated stage two renders before switching to stage five"))) return;
   }
   else if (TestStep == 4)
   {
    if (!Check(Rendered(5) && Select(2), TEXT("nine actual stage-five billboards can be replaced by lower-stage preview"))) return;
   }
   else if (TestStep == 5)
   {
    if (!Check(Rendered(2), TEXT("five-to-two selection renders only the two correct preview tracks"))) return;
    Stop(); M->SkyVisitors->Step(0);
    if (!Check(Rendered(0) && S47J::Write(M->SkyVisitors->Save()) == Natural,
     TEXT("Stop removes preview components from view without replaying or regressing completed natural sky"))) return;
    Finish(true); return;
   }
  }
  ++TestStep; return;
 }
 TArray<const FEvent*> Events;
 for (const FEvent& Event:List) if (OnlyGroup.IsEmpty() || Event.Group==OnlyGroup) Events.Add(&Event);
 TestT += Dt;
 // Measure simultaneous actual components over the short random shower window rather
 // than demand a particular density on one arbitrarily selected frame.
 static int32 ShowerPeak = 0;
 if (OnlyGroup == TEXT("Sky") && ShotName == TEXT("meteor-shower") && M->Life && M->Life->Sky)
  ShowerPeak = FMath::Max(ShowerPeak, FMath::Min(M->Life->Sky->ShowerLive(), M->Life->Sky->ShowerSlotsVisible()));
 else ShowerPeak = 0;
 // a picture a moment after its event, once the scene has taken it
 if (!ShotName.IsEmpty() && TestT >= ShotAt)
 {
  if (OnlyGroup == TEXT("Sky") && ShotName == TEXT("meteor-shower"))
  {
   // Read the actual pool after ordinary world ticks from the real F10 action.
   const FS47Sky* Sky = M->Life && M->Life->Sky ? M->Life->Sky.Get() : nullptr;
   const int32 Live = Sky ? Sky->ShowerLive() : 0, Visible = Sky ? Sky->ShowerSlotsVisible() : 0;
   const bool Concurrent = ShowerPeak >= 3;
   UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer meteor shower contract %s: first 1.5 s peak simultaneous components %d (>= 3); now live %d, visible %d"),
    Concurrent ? TEXT("PASS") : TEXT("FAIL"), ShowerPeak, Live, Visible);
   if (!Concurrent) { bTestDone = true; FPlatformMisc::RequestExitWithStatus(false, 1); return; }
  }
  FScreenshotRequest::RequestScreenshot(TestDir / (ShotName + TEXT(".png")), false, false); ShotName.Reset();
 }
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
 if (M && M->SkyVisitors) M->SkyVisitors->DevStage(0);
 if(auto* C=CraftPreview.Get())C->DestroyComponent();CraftPreview.Reset();
 if (M && M->Night && M->Night->Glimpses) M->Night->Glimpses->DevClear();
 if (M && M->Life && M->Life->Sky) { M->Life->Sky->ShowerClear(); M->Life->Sky->bShowerHeld = false; }
 ShowerLeft = 0; PaleLeft = 0; GS47DevFogDensity = -1.f;
 Pale.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk);
}
