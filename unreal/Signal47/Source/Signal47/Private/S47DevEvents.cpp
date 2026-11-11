#include "S47DevEvents.h"
#include "S47DirectorCam.h"
#include "S47SaroWallWatch.h"
#include "S47DevFreeform.h"
#include "S47FloodProbe.h"
#include "S47Freeform.h"
#include "S47FarmSite.h"
#include "S47IndoorDread.h"
#include "S47HighwayUfo.h"
#include "S47FreeformDirector.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47StoryDirector.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47World.h"
#include "S47Night.h"
#include "S47Glimpses.h"
#include "S47Shot.h"
#include "S47Sky.h"
#include "S47SkyVisitors.h"
#include "S47Flow.h"
#include "S47WorldLife.h"
#include "S47Three.h"
#include "SS47Dev.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkinnedAsset.h"
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

void FS47DevEvents::AddFreeform(const FString& Group, const FString& Label, const FString& Note, TFunction<void()> Fn)
{
 List.Add({ Group, Label, Note, MoveTemp(Fn), true });
}

void FS47DevEvents::AddStory(const FString& Group, const FString& Label, const FString& Note, TFunction<void()> Fn)
{
 List.Add({ Group, Label, Note, MoveTemp(Fn), true, true });
}

void FS47DevEvents::Relabel(const FString& OldLabel, const FString& NewLabel)
{
 for (FEvent& E : List) if (E.Label == OldLabel) E.Label = NewLabel;
}

void FS47DevEvents::AddBuiltIn()
{
 // Freeform's first: in a Freeform night those are the lines asked for, in the order the night has them.
 FreeformLines = MakeShared<FS47DevFreeform>(M);
 FreeformLines->AddEntries(*this);
 const auto Say = [this](const FString& What) { if (M && M->UI) M->UI->Toast(TEXT("Developer event: ") + What, 3); UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer event %s"), *What); };
 Add(TEXT("Station"), TEXT("SARO wall check"), TEXT("WORLD-04 (N1110): are the office walls drawing? Logs the room pieces; the same is logged around every Rest"), [this, Say] { Say(S47SaroWallWatch::Check(M)); });
 // ---- aliens ----
 // Pale: the label reads as it did so far (and Label.StartsWith "Pale" keeps its picture in the contract below).
 Add(TEXT("Aliens"), TEXT("Pale One, here"), TEXT("stands ahead of you and looks at you, then goes"), [this, Say] { SpawnPale(); Say(TEXT("Pale One")); });
 Add(TEXT("Aliens"), TEXT("Pale One: walks up from afar"), TEXT("on its own walk; 30 seconds"), [this, Say] { SpawnGait(GaitPale); Say(TEXT("Pale One walking")); });
 Add(TEXT("Aliens"), TEXT("Tall one: walks up from afar"), TEXT("on its own model and clips; 30 seconds"), [this, Say] { SpawnGait(GaitTall); Say(TEXT("tall one")); });
 Add(TEXT("Aliens"), TEXT("Crab: folds down and crawls up"), TEXT("then sideways round you and away; 30 seconds"), [this, Say] { SpawnGait(GaitCrab); Say(TEXT("crab")); });
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
  Add(TEXT("Sky"), FString::Printf(TEXT("Preview sky lights, stage %d"), S), TEXT("high up ahead of you, outdoors; no natural observation credit"),
   [this, Say, S] { if (M->SkyVisitors) M->SkyVisitors->DevStage(S); Say(FString::Printf(TEXT("sky preview stage %d"), S)); });
 Add(TEXT("Sky"), TEXT("Stop sky preview"), TEXT("return to the natural sky"), [this, Say] { if (M->SkyVisitors) M->SkyVisitors->DevStage(0); Say(TEXT("sky preview stopped")); });
 // Lights in formation (FS47SkyFormationState): the same ones the night sends, across the view from where you stand.
 for (int32 F = 0; F < int32(ES47SkyForm::Num); ++F)
  Add(TEXT("UFO formations"), FString::Printf(TEXT("Formation: %s"), FS47SkyFormationState::Name(ES47SkyForm(F))), TEXT("high lights crossing ahead of you, outdoors; nothing saved"), [this, Say, F]
  {
   const bool bShown = M->SkyVisitors && M->SkyVisitors->DevFormation(ES47SkyForm(F), FormationPace);
   Say(bShown ? FString::Printf(TEXT("%s formation: look up ahead"), FS47SkyFormationState::Name(ES47SkyForm(F))) : FString(TEXT("formations need an outdoor place")));
  });
 Add(TEXT("UFO formations"), TEXT("Pace: slow"), TEXT("cycle slow, fast, or slow and going out where they are"), [this]
 {
  FormationPace = (FormationPace + 1) % 3;
  static const TCHAR* const Paces[3] = {TEXT("Pace: slow"), TEXT("Pace: fast"), TEXT("Pace: goes out")};
  for (auto& E : List) if (E.Group == TEXT("UFO formations") && E.Label.StartsWith(TEXT("Pace:"))) E.Label = Paces[FormationPace];
  Open();
 });
 Add(TEXT("UFO formations"), TEXT("Stop formation"), TEXT("return to the night's own sky"), [this, Say] { if (M->SkyVisitors) M->SkyVisitors->DevFormationStop(); Say(TEXT("formation stopped")); });
 Add(TEXT("Tripo craft preview"),TEXT("Triangular craft: source model"),TEXT("outdoor model preview ahead; story/finale unchanged"),[this]{SpawnCraft(false);});
 Add(TEXT("Tripo craft preview"),TEXT("Flying saucer: source model"),TEXT("outdoor model preview ahead; story/finale unchanged"),[this]{SpawnCraft(true);});
 Add(TEXT("Tripo craft preview"),TEXT("Remove craft preview"),TEXT("remove the temporary model completely"),[this]{if(auto* C=CraftPreview.Get())C->DestroyComponent();CraftPreview.Reset();});
 for(const bool Follow:{false,true})
  Add(TEXT("UFO encounters"),Follow?TEXT("Highway: light follows you"):TEXT("Highway: sheriff follows light"),TEXT("live saved encounter; drive north or south on a clear highway"),[this,Follow]
  {
   const bool Started=M&&M->HighwayUfo&&M->HighwayUfo->Force(Follow);
   if(M&&M->UI)M->UI->Toast(Started?TEXT("Highway encounter started. Your steering and brakes remain in control."):TEXT("Keep driving north or south on the open highway, between the motel and Caliche with road ahead, clear of other encounters, then try again."),5);
  });
 // ---- the massive light outside (S47FloodProbe.h) ----
 {
  const auto Massive=[this,Say](const FString& Room)
  {
   FString Why;
   Say(S47FloodProbe::BeginLive(M,Room,Why)?FString(TEXT("the light is outside, 40 seconds")):Why);
  };
  Add(TEXT("Massive light outside"),TEXT("Here, where you stand"),TEXT("through whatever window or door reaches you; finds none, says so"),[Massive]{Massive(FString());});
  Add(TEXT("Massive light outside"),TEXT("SARO control room"),TEXT("takes you there; walk about in it"),[Massive]{Massive(TEXT("control"));});
  Add(TEXT("Massive light outside"),TEXT("The old house"),TEXT("takes you there; walk about in it"),[Massive]{Massive(TEXT("house"));});
  Add(TEXT("Massive light outside"),TEXT("The garage"),TEXT("takes you there; walk about in it"),[Massive]{Massive(TEXT("garage"));});
  Add(TEXT("Massive light outside"),TEXT("The Plaza cinema"),TEXT("takes you there; walk about in it"),[Massive]{Massive(TEXT("cinema"));});
  Add(TEXT("Massive light outside"),TEXT("Put it out"),TEXT("back to the night"),[this,Say]{S47FloodProbe::StopLive(M);Say(TEXT("the light is out"));});
 }
 // ---- weather ----
 Add(TEXT("Weather"), TEXT("Fog: the night's own"), TEXT("as the scene has it"), [this, Say] { GS47DevFogDensity = -1.f; if (M->Web) UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer fog %.4f, native fog now %.4f"), GS47DevFogDensity, M->Web->RefreshFog()); Say(TEXT("fog as the night has it")); });
 Add(TEXT("Weather"), TEXT("Fog: light"), TEXT("about 100 m"), [this, Say] { GS47DevFogDensity = .04f; if (M->Web) UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer fog %.4f, native fog now %.4f"), GS47DevFogDensity, M->Web->RefreshFog()); Say(TEXT("light fog")); });
 Add(TEXT("Weather"), TEXT("Fog: medium"), TEXT("about 40 m"), [this, Say] { GS47DevFogDensity = .18f; if (M->Web) UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer fog %.4f, native fog now %.4f"), GS47DevFogDensity, M->Web->RefreshFog()); Say(TEXT("medium fog")); });
 Add(TEXT("Weather"), TEXT("Fog: heavy"), TEXT("about 15 m"), [this, Say] { GS47DevFogDensity = 1.5f; if (M->Web) UE_LOG(LogS47DevEvents, Display, TEXT("S47: developer fog %.4f, native fog now %.4f"), GS47DevFogDensity, M->Web->RefreshFog()); Say(TEXT("heavy fog")); });
 // ---- time ----
 Add(TEXT("Time"), TEXT("Clock +15 minutes"), TEXT("the night's clock only"), [this, Say]
 {
  // Freeform's clock is the session's; Story's is overwritten from it on the next step.
  if (M->IsFreeform() && M->Freeform) M->Freeform->TestAdvance(15 * 60);
  else if (M->Story) M->Story->SetClock(M->Story->Clock() + 15 * 60);
  Say(TEXT("clock +15 min"));
 });
}

void FS47DevEvents::Open()
{
 if (!M || !M->UI || M->Mode != US47Main::EMode::Play || M->UI->Modal()) return;
 const bool bFree = M->IsFreeform();
 TArray<FS47DevEntry> E;
 for (const FEvent& Ev : List)
 {
  if (Ev.bFreeform && !bFree) continue;
  if (Ev.bStory && !FS47StoryDirector::Of(M)) continue; // STORY-ON-FREEFORM-01 (step 4): a Story night's lines only
  const TFunction<void()> Fn = Ev.Fn;
  const FString Group = Ev.Group;
  // A line that opens this menu again (a setting being cycled) comes back to its own group, not the front page.
  E.Add({ Ev.Group, Ev.Label, Ev.Note, [this, Fn, Group] { if (M && M->UI) M->UI->Close(true); ReopenGroup = Group; if (Fn) Fn(); ReopenGroup.Empty(); } });
 }
 const FString Line = FreeformLines && bFree ? FreeformLines->MenuNote() : FString(TEXT("Developer previews and live encounter controls. Check each entry before starting."));
 TSharedRef<SS47Dev> El = SNew(SS47Dev).Entries(E).Title(TEXT("DEVELOPER / EVENTS")).bCategories(true).InitialGroup(ReopenGroup)
  .Note(Line)
  .OnBack([this] { if (M && M->UI) M->UI->Close(true); });
 M->UI->Open(El);MenuEl=El;
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
 const FVector P=PlayerFeet();const double Yaw=M->Player->Yaw;
 const FVector At(P.X-FMath::Sin(Yaw)*18,P.Y+7.5,P.Z-FMath::Cos(Yaw)*18);
 C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldRotation(FRotator::ZeroRotator);CraftPreview=C;
 if(M->UI)M->UI->Toast(TEXT("Tripo craft preview ahead. View outdoors; F10 removes it. Nothing is saved."),5);
 UE_LOG(LogS47DevEvents,Display,TEXT("S47: temporary Tripo craft preview %s at%s; no story/finale replacement"),Name,*At.ToString());
}

FVector FS47DevEvents::PlayerFeet() const
{
 return M && M->Player ? FVector(M->Player->Pos.X, M->Player->FloorY, M->Player->Pos.Z) : FVector::ZeroVector;
}

bool FS47DevEvents::BuildPale()
{
 // Tom's Pale One, which wears the old pale body by itself in a build without it (FS47TripoCreature::Build). This used to ask
 // for nothing and so got the old body, and on the Freeform land it looked for its floor at height zero.
 if (!Pale.bBuilt) { Pale.CreatureAsset = S47KindBody::Pale; Pale.Build(M->Web.Get(), TEXT("#root"), TEXT("DevPale")); }
 return Pale.bBuilt;
}

void FS47DevEvents::SpawnPale()
{
 if (!M || !M->Web || !M->Player) return;
 if (!BuildPale()) { if (M->UI) M->UI->Toast(TEXT("Developer event: the pale body could not be built here."), 3); return; }
 Gaits[GaitPale].Left = 0;
 // Four metres ahead where the walker could stand, else as near as there is room; facing you.
 const FVector P = PlayerFeet();
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
   // On rolling land its ground can be well above or below yours at this distance, and the trace has to find it.
   const double Reach = .45 + D * .3;
   FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevPaleFloor), true);
   for (const auto& Part : Pale.All) if (const auto* Own = Part.Get()) Under.AddIgnoredComponent(Own);
   FHitResult Floor;
   if (!M->Web->GetWorld()->LineTraceSingleByChannel(Floor, S47Three::ToUE(C.X, P.Y + Reach + 2.2, C.Y), S47Three::ToUE(C.X, P.Y - Reach - 3, C.Y), ECC_Visibility, Under)) continue;
   const double G = S47Three::ToThree(Floor.ImpactPoint).Y;
   if (FMath::Abs(G - P.Y) > Reach) continue;
   FHitResult Block; FCollisionQueryParams Sight(SCENE_QUERY_STAT(S47DevPaleSight), true);
   for (const auto& Part : Pale.All) if (const auto* Own = Part.Get()) Sight.AddIgnoredComponent(Own);
   const FVector A = S47Three::ToUE(Eye), B = S47Three::ToUE(C.X, G + 1.4, C.Y);
   if (M->Web->GetWorld()->LineTraceSingleByChannel(Block, A, B, ECC_Visibility, Sight) && Block.Distance < FVector::Dist(A, B) - 20) continue;
   Found = C; FoundY = G;
  }
 if (!Found) { if (M->UI) M->UI->Toast(TEXT("Developer event: no free floor in sight for the body here."), 3); return; }
 const FVector2D At = *Found;
 PaleAt = FVector(At.X, FoundY, At.Y);
 PaleYaw = FMath::Atan2(P.Z - At.Y, P.X - At.X);
 PaleLeft = PaleStand; PaleClock = 0;
 UE_LOG(LogS47DevEvents, Display, TEXT("S47: Pale One stands at %s, feet of the player at %.2f, wearing %s"), *PaleAt.ToCompactString(), P.Y, Pale.Creature.Mesh.IsValid() && Pale.Creature.Mesh->GetSkinnedAsset() ? *Pale.Creature.Mesh->GetSkinnedAsset()->GetName() : TEXT("the old parts"));
 if (M->Audio) M->Audio->Gasp(M->Audio->Spatial(PaleAt + FVector(0, 1.8, 0), 2.f));
}

void FS47DevEvents::SpawnGait(int32 Kind)
{
 if (!M || !M->Web || !M->Player) return;
 const bool bCrab=Kind==GaitCrab;
 FS47PaleBody& Body=GaitBody(Kind);
 if(Kind==GaitPale){if(!BuildPale()){if(M->UI)M->UI->Toast(TEXT("Developer event: the pale body could not be built here."),3);return;}PaleLeft=0;}
 else if(!Body.bBuilt){Body.CreatureAsset=bCrab?S47KindBody::Crab:S47KindBody::Tall;if(!bCrab)Body.Style=ES47PaleStyle::Tall;if(!Body.Build(M->Web.Get(),TEXT("#root"),bCrab?TEXT("DevCrab"):TEXT("DevTall"))){if(M->UI)M->UI->Toast(TEXT("Developer event: this body could not be built here."),3);return;}}
 if(!Body.Creature.Ready()){if(M->UI)M->UI->Toast(TEXT("Developer event: this body needs the private skinned model."),3);return;}
 const FVector P=PlayerFeet();
 const FVector Eye(P.X,P.Y+1.62,P.Z);
 for(const double Turn:{0.,.5,-.5,1.,-1.,1.6,-1.6,double(PI)})for(const double D:{16.,11.,7.})
 {
  const double Yaw=M->Player->Yaw+Turn;
  const FVector2D C(P.X-FMath::Sin(Yaw)*D,P.Z-FMath::Cos(Yaw)*D);
  // The ground out there first: rolling land is metres above or below yours this far off, and the sight is to where it would stand.
  FCollisionQueryParams Under(SCENE_QUERY_STAT(S47DevGaitGround),true);
  for(const auto& Part:Body.All)if(const auto* Own=Part.Get())Under.AddIgnoredComponent(Own);
  FHitResult Ground;
  if(!M->Web->GetWorld()->LineTraceSingleByChannel(Ground,S47Three::ToUE(C.X,P.Y+12.,C.Y),S47Three::ToUE(C.X,P.Y-12.,C.Y),ECC_Visibility,Under))continue;
  const double G=S47Three::ToThree(Ground.ImpactPoint).Y;
  FHitResult Block;FCollisionQueryParams Sight(SCENE_QUERY_STAT(S47DevGaitSight),true);
  for(const auto& Part:Body.All)if(const auto* Own=Part.Get())Sight.AddIgnoredComponent(Own);
  const FVector A=S47Three::ToUE(Eye),B=S47Three::ToUE(C.X,G+1.,C.Y);
  if(M->Web->GetWorld()->LineTraceSingleByChannel(Block,A,B,ECC_Visibility,Sight))continue;
  FGaitTest& T=Gaits[Kind];
  T.At=FVector(C.X,G,C.Y);T.Yaw=FMath::Atan2(P.Z-C.Y,P.X-C.X);T.Left=30;T.Clock=0;
  UE_LOG(LogS47DevEvents,Display,TEXT("S47: gait test %d starts %.0f m off at %s, feet of the player at %.2f"),Kind,D,*T.At.ToCompactString(),P.Y);
  return;
 }
 if(M->UI)M->UI->Toast(TEXT("Developer event: no free ground in sight; try outdoors."),3);
}

void FS47DevEvents::StepGait(int32 Kind,double Dt)
{
 const bool bCrab=Kind==GaitCrab;
 FGaitTest& T=Gaits[Kind];FS47PaleBody& Body=GaitBody(Kind);
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
 if (FreeformLines) FreeformLines->Step(Dt);
 if (M->Night && M->Night->Glimpses) M->Night->Glimpses->DevStep(Dt,M->Night->GlimpseIn(true));
 if (S47FloodProbe::LiveActive()) S47FloodProbe::Step(M,float(Dt));
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
 if (bPlay) for (int32 Kind = 0; Kind < GaitCount; ++Kind) StepGait(Kind,Dt);
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
 if (ProofTest(Dt)) return;
 if (!bTestInit) { bTestInit = true; FParse::Value(FCommandLine::Get(), TEXT("S47DevEventsContract="), TestDir); }
 if (TestDir.IsEmpty() || bTestDone || !M || M->Mode != US47Main::EMode::Play) return;
 FString OnlyGroup; FParse::Value(FCommandLine::Get(),TEXT("S47DevEventsGroup="),OnlyGroup);
 // SHOT-BEAT-01: the director shot's own check, through its F10 line and the script player (S47Shot::Contract).
 if (OnlyGroup == TEXT("Director shot")) { if (S47Shot::Contract(M, Dt)) bTestDone = true; return; }
 // DIRECTOR-MODE-01: the director's free camera (S47DirectorCam::Contract).
 if (OnlyGroup == TEXT("Director mode")) { if (S47DirectorCam::Contract(M, Dt)) bTestDone = true; return; }
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
 for (const FEvent& Event:List) if (OnlyGroup.IsEmpty() ? !Event.bFreeform : Event.Group==OnlyGroup) Events.Add(&Event);
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
 // -S47DevEventsHold=seconds: the last event's own sequence (a drive, a stop) runs out before the contract ends
 double Hold = 0; FParse::Value(FCommandLine::Get(), TEXT("S47DevEventsHold="), Hold);
 if (TestT < 4 + TestStep * 2.5 + (TestStep >= Events.Num() ? FMath::Clamp(Hold, 0.0, 120.0) : 0.0)) return;
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
 if (FreeformLines) FreeformLines->Stop();
 if (M && M->SkyVisitors) M->SkyVisitors->DevStage(0);
 S47FloodProbe::StopLive(M);
 if(auto* C=CraftPreview.Get())C->DestroyComponent();CraftPreview.Reset();
 if (M && M->Night && M->Night->Glimpses) M->Night->Glimpses->DevClear();
 if (M && M->Life && M->Life->Sky) { M->Life->Sky->ShowerClear(); M->Life->Sky->bShowerHeld = false; }
 ShowerLeft = 0; PaleLeft = 0; GS47DevFogDensity = -1.f;
 if (M && M->SkyVisitors) M->SkyVisitors->DevFormationStop();
 for (FGaitTest& T : Gaits) T.Left = 0;
 // Hidden as a whole, whatever each was last doing: a walk left half done must not stand in the next night.
 Pale.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk);
 TallBody.Apply(false, FVector::ZeroVector, 0, 0, 0, 0, 0, FS47PaleBody::EPose::Walk);
 CrabBody.ApplyCrab(false, FVector::ZeroVector, 0, 0, 0, 0, false);
}

bool FS47DevEvents::ProofCheck(bool bGood, const FString& What)
{
 ++ProofChecks;
 UE_LOG(LogS47DevEvents, Display, TEXT("S47: dev proof %s: %s"), bGood ? TEXT("PASS") : TEXT("FAIL"), *What);
 if (!bGood) ProofFinish(false, What);
 return bGood;
}
void FS47DevEvents::ProofFinish(bool bGood, const FString& Why)
{
 if (bProofDone) return; bProofDone = true;
 UE_LOG(LogS47DevEvents, Display, TEXT("S47: dev proof %s COMPLETE after %d checks: %s"), bGood ? TEXT("PASS") : TEXT("FAIL"), ProofChecks, *Why);
 FPlatformMisc::RequestExitWithStatus(false, bGood ? 0 : 1);
}
bool FS47DevEvents::ProofUse(const FString& Label)
{
 // The menu's own entry, found by the label the player reads.
 const FEvent* Event = List.FindByPredicate([&](const FEvent& E) { return E.Label == Label; });
 UE_LOG(LogS47DevEvents, Display, TEXT("S47: dev proof menu: %s"), *Label);
 if (!ProofCheck(Event && Event->Fn, FString::Printf(TEXT("the menu has \"%s\""), *Label))) return false;
 Event->Fn(); return true;
}
void FS47DevEvents::ProofAim(const FVector& At)
{
 const FVector D = At - FVector(M->Player->Pos.X, M->Player->FloorY + M->Player->Eye, M->Player->Pos.Z);
 M->Player->Yaw = FMath::Atan2(-D.X, -D.Z); M->Player->Pitch = FMath::Atan2(D.Y, FVector2D(D.X, D.Z).Size());
}
// Run in Freeform on a /tmp profile, never on Tom's: the night's own schedule first, the collision with another event, then each
// entry through the menu's own function from where a player stands outdoors, reading back what is drawn. How it looks is for the pictures.
bool FS47DevEvents::ProofTest(double Dt)
{
 if (!bProofInit)
 {
  bProofInit = true; FParse::Value(FCommandLine::Get(), TEXT("S47DevProof="), ProofDir);
  FString User; FParse::Value(FCommandLine::Get(), TEXT("UserDir="), User);
  if (!ProofDir.IsEmpty() && (!ProofDir.StartsWith(TEXT("/tmp/")) || !User.StartsWith(TEXT("/tmp/")))) { UE_LOG(LogS47DevEvents, Error, TEXT("S47: dev proof needs a /tmp directory and profile")); ProofDir.Empty(); FPlatformMisc::RequestExitWithStatus(false, 1); }
 }
 if (ProofDir.IsEmpty()) return false;
 if (bProofDone || !M || M->Mode != US47Main::EMode::Play || !M->Freeform || !M->Freeform->Active() || !M->SkyVisitors || !M->Player || !M->World || !M->Web) return true;
 auto& F = *M->Freeform; auto& Sky = *M->SkyVisitors;
 ProofT += Dt;
 if (ProofT > 420) { ProofFinish(false, TEXT("timed out")); return true; }
 if (!ProofShot.IsEmpty()) { FScreenshotRequest::RequestScreenshot(ProofDir / (ProofShot + TEXT(".png")), false, false); UE_LOG(LogS47DevEvents, Display, TEXT("S47: dev proof picture %s"), *ProofShot); ProofShot.Empty(); return true; }
 const double Since = ProofT - ProofAt;
 auto Go = [&](int32 Next) { ProofStage = Next; ProofAt = ProofT; };
 auto Hour = [&] { return FMath::Fmod(F.Clock(), 86400.) / 3600.; };
 auto ToHour = [&](double Want) { return F.TestAdvance(FMath::Fmod(Want * 3600. - FMath::Fmod(F.Clock(), 86400.) + 86400., 86400.)); };
 auto AimAtShown = [&]() -> int32 { const auto Shown = Sky.ShownPoints(); FVector Mean = FVector::ZeroVector; for (const FVector& P : Shown) Mean += P; if (Shown.Num()) { ProofMean = Mean / Shown.Num(); ProofAim(ProofMean); } return Shown.Num(); };
 static const TCHAR* const Forms[5] = {TEXT("V"), TEXT("line"), TEXT("triangle"), TEXT("circle"), TEXT("scattered")};
 switch (ProofStage)
 {
 case 0:
  if (Since < 4) return true;
  if (!ProofCheck(M->Flow && M->Flow->bDevRun && M->Flow->CaseId == 0, TEXT("isolated developer Freeform case"))) return true;
  M->UI->Close(true); F.TestEndFirstShift(); M->World->Enter(TEXT("saro")); M->Player->Place(7, 19, UE_DOUBLE_PI); M->Player->Pitch = 0;
  // Night two, a little before the first pass the schedule offers. Nothing is asked of the sky: it comes or it does not.
  if (!ProofCheck(F.TestAdvance(86400. + 21.3 * 3600. - FMath::Fmod(F.Clock(), 86400.)), TEXT("clock moved to night two"))) return true;
  if (F.Director) F.Director->CancelForTest();
  Go(1); return true;
 case 1:
 {
  const auto& Form = Sky.FormationData();
  if (Form.bActive && !Form.bDev)
  {
   if (F.Director && !ProofCheck(!F.Director->Busy() || Form.LeaveAt >= 0, TEXT("the natural pass did not start on top of another event"))) return true;
   if (!ProofCheck(Hour() >= 21.5 && (Form.Form == ES47SkyForm::Line || Form.Form == ES47SkyForm::Scatter), FString::Printf(TEXT("first pass of the evening from the schedule at %.2f h: %s, %d lights, %s"), Hour(), FS47SkyFormationState::Name(Form.Form), Form.Lights, Form.bFast ? TEXT("fast") : TEXT("slow")))) return true;
   Go(2); return true;
  }
  // Waiting at the clock's own rate would take minutes: up to its hour once, and from there the schedule's own trigger decides.
  if (Form.NextClock >= 0 && F.Clock() < Form.NextClock - 30) F.TestAdvance(Form.NextClock - 30 - F.Clock());
  if (F.Director && F.Director->Busy()) F.Director->CancelForTest();
  if (Since > 40) { ProofFinish(false, FString::Printf(TEXT("no natural pass; next %.0f, clock %.0f"), Form.NextClock, F.Clock())); return true; }
  return true;
 }
 case 2:
  if (Since < 6) return true;
  {
   // The night's own stay behind the weather, a developer's do not.
   const int32 N = AimAtShown(); const auto& Form = Sky.FormationData();
   const bool bCloud = !M->Web->CloudSightClear(M->Player->EyeUE(), S47Three::ToUE(FS47SkyVisitors::WorldPosition(Form.MapPosition(0), M->World->Area)));
   if (!ProofCheck(N == Form.Lights || bCloud, FString::Printf(TEXT("%d of the natural pass's %d lights are drawn, high at %s, cloud %d"), N, Form.Lights, *ProofMean.ToCompactString(), bCloud ? 1 : 0))) return true;
  }
  Go(3); return true;
 case 3:
  if (Since < .4) return true;
  ProofShot = TEXT("natural-first-pass"); Go(4); return true;
 case 4:
  if (Since < .3) return true;
  // Something else begins: they go, and do not stand over it.
  if (!ProofCheck(F.Director && F.Director->Force(TEXT("response_lights")), TEXT("another event starts under the pass"))) return true;
  Go(5); return true;
 case 5:
  if (Since < 2.2) return true;
  if (!ProofCheck(!Sky.FormationData().bActive && Sky.ShownPoints().IsEmpty(), TEXT("the pass left within two seconds of it"))) return true;
  // And at its next hour, while that event is still going, none comes.
  F.TestAdvance(FMath::Max(0., Sky.FormationData().NextClock + 5 - F.Clock()));
  Go(6); return true;
 case 6:
  if (Since < 1.5) return true;
  if (!ProofCheck(F.Director->Busy() && !Sky.FormationData().bActive && Sky.FormationData().NextClock >= 0, FString::Printf(TEXT("%.2f h, its hour: held back while the event is on"), Hour()))) return true;
  F.Director->CancelForTest(); Go(7); return true;
 case 7:
  if (Sky.FormationData().bActive) { if (!ProofCheck(!Sky.FormationData().bDev, FString::Printf(TEXT("once it is over the waiting pass comes, %s"), FS47SkyFormationState::Name(Sky.FormationData().Form)))) return true; Go(8); return true; }
  if (F.Director->Busy()) F.Director->CancelForTest();
  if (Since > 20) { ProofFinish(false, TEXT("the waiting pass never came")); return true; }
  return true;
 case 8:
  // Each menu entry from a standing start, day or night: first the stage previews that showed nothing in Freeform.
  ToHour(23.); Sky.TestFormationQuiet(F.Clock()); ProofIndex = 1; M->Player->Place(7, 19, UE_DOUBLE_PI); M->Player->Pitch = .25;
  if (F.Director && F.Director->Busy()) F.Director->CancelForTest();
  Go(9); return true;
 case 9:
  if (Since < .3) return true;
  if (F.Director && F.Director->Busy()) F.Director->CancelForTest();
  if (!ProofUse(FString::Printf(TEXT("Preview sky lights, stage %d"), ProofIndex))) return true;
  Go(10); return true;
 case 10:
  if (Since < 3.5) return true;
  {
   const int32 N = Sky.ShownPoints().Num(), Want = FS47SkyVisitorState::Count(ProofIndex);
   if (!ProofCheck(N == Want, FString::Printf(TEXT("preview stage %d draws %d of %d lights in Freeform"), ProofIndex, N, Want))) return true;
   AimAtShown();
   // In the sky, not under it, and in front of whoever asked when the camera has not moved.
   const FS47Cam Cam = FS47Cam::Of(M->Player, 30000); const FVector P = Cam.Project(ProofMean);
   if (!ProofCheck(ProofMean.Y > M->Player->FloorY + 100 && P.Z < 1, FString::Printf(TEXT("stage %d is high up, mean at %s"), ProofIndex, *ProofMean.ToCompactString()))) return true;
  }
  Go(11); return true;
 case 11:
  if (Since < .4) return true;
  ProofShot = FString::Printf(TEXT("preview-stage-%d"), ProofIndex);
  if (++ProofIndex > 5) { ProofIndex = 0; Go(12); } else Go(9);
  return true;
 case 12:
  if (Since < .2) return true;
  if (!ProofUse(TEXT("Stop sky preview"))) return true;
  Go(13); return true;
 case 13:
  if (Since < .4) return true;
  if (!ProofCheck(Sky.ShownPoints().IsEmpty(), TEXT("stop leaves nothing in the sky"))) return true;
  M->Player->Pitch = .1; Go(14); return true;
 case 14:
  if (F.Director && F.Director->Busy()) F.Director->CancelForTest();
  if (!ProofUse(FString::Printf(TEXT("Formation: %s"), Forms[ProofIndex]))) return true;
  Go(15); return true;
 case 15:
  if (Since < 4) return true;
  {
   const auto& Form = Sky.FormationData(); const int32 N = Sky.ShownPoints().Num();
   if (!ProofCheck(Form.bActive && Form.bDev && int32(Form.Form) == ProofIndex && N == Form.Lights, FString::Printf(TEXT("formation %s draws %d of %d lights"), Forms[ProofIndex], N, Form.Lights))) return true;
   // Asked for with the camera where it was: the shape's middle is in the picture before anything turns to it.
   FVector Mean = FVector::ZeroVector; for (const FVector& P : Sky.ShownPoints()) Mean += P; Mean /= FMath::Max(1, N);
   const FVector Seen = FS47Cam::Of(M->Player, 30000).Project(Mean);
   if (!ProofCheck(Seen.Z < 1 && FMath::Abs(Seen.X) < 1 && FMath::Abs(Seen.Y) < 1, FString::Printf(TEXT("%s is in view where the player was looking, at %.2f %.2f"), Forms[ProofIndex], Seen.X, Seen.Y))) return true;
  }
  ProofShot = FString::Printf(TEXT("formation-%s"), Forms[ProofIndex]); Go(16); return true;
 case 16:
  if (Since < .3) return true;
  if (++ProofIndex < 5) { Go(14); return true; }
  ProofIndex = 0; Go(17); return true;
 case 17:
  // The pace entry, as a person cycles it, and a fast pass that really crosses.
  if (!ProofUse(TEXT("Pace: slow")) ) return true;
  M->UI->Close(true);
  if (!ProofUse(TEXT("Formation: V"))) return true;
  ProofMean = FS47SkyVisitors::WorldPosition(Sky.FormationData().MapPosition(0), M->World->Area); Go(18); return true;
 case 18:
  if (Since < 4) return true;
  {
   const auto& Form = Sky.FormationData(); const auto Shown = Sky.ShownPoints();
   const double Moved = Shown.Num() ? FVector::Dist(Shown[0], ProofMean) : 0;
   if (!ProofCheck(Form.bFast && Shown.Num() == Form.Lights && Moved > 1000, FString::Printf(TEXT("fast V: %.0f m in the first four seconds"), Moved))) return true;
  }
  if (!ProofUse(TEXT("Pace: fast"))) return true;
  M->UI->Close(true);
  if (!ProofUse(TEXT("Formation: line"))) return true;
  Sky.TestFormationAge(Sky.FormationData().Seconds() * .6 - .5); Go(19); return true;
 case 19:
  if (Since < 2.2) return true;
  {
   const auto& Form = Sky.FormationData(); const int32 N = Sky.ShownPoints().Num();
   if (!ProofCheck(Form.bVanish && N < Form.Lights, FString::Printf(TEXT("going out: %d of %d left, one after another"), N, Form.Lights))) return true;
  }
  Go(20); return true;
 case 20:
  if (Since < 3.) return true;
  if (!ProofCheck(!Sky.FormationData().bActive && Sky.ShownPoints().IsEmpty(), TEXT("and the pass is over, nothing left in the sky"))) return true;
  // Bodies, by day so the pictures say something. First where the ground is flat, then out on the land: the relief is what broke them.
  Stop(); FormationPace = 0; ToHour(12.); M->World->Enter(TEXT("saro")); M->Player->Place(7, 19, UE_DOUBLE_PI); M->Player->Pitch = -.05;
  ProofIndex = 0; Go(21); return true;
 case 21:
 {
  if (Since < 1.5) return true;
  // Round two: out on the pasture, the highest ground difference from zero near the base.
  if (ProofIndex == 4) { const FVector C = F.FarmSite ? F.FarmSite->Centre() : FVector(60, 0, 240); M->Player->Place(C.X + 30, C.Z + 30, 0); M->Player->Pitch = -.05; }
  UE_LOG(LogS47DevEvents, Display, TEXT("S47: dev proof ground: player at %.1f %.1f, Pos.Y %.2f, FloorY %.2f"), M->Player->Pos.X, M->Player->Pos.Z, M->Player->Pos.Y, M->Player->FloorY);
  static const TCHAR* const Bodies[4] = {TEXT("Pale One, here"), TEXT("Pale One: walks up from afar"), TEXT("Tall one: walks up from afar"), TEXT("Crab: folds down and crawls up")};
  if (ProofIndex < 8)
  {
   if (F.Director && F.Director->Busy()) F.Director->CancelForTest();
   if (!ProofUse(Bodies[ProofIndex % 4])) return true;
  }
  else if (!ProofUse(TEXT("Stand for 8 seconds"))) return true;
  Go(22); return true;
 }
 case 22:
  {
   const int32 Kind = ProofIndex % 4;
   FString Name; FVector Feet = FVector::ZeroVector; bool bDrawn = false; FString Worn;
   const auto WornBy = [](const FS47PaleBody& B) { const auto* C = B.Creature.Mesh.Get(); return C && C->GetSkinnedAsset() ? C->GetSkinnedAsset()->GetName() : FString(TEXT("none")); };
   if (ProofIndex >= 8)
   {
    const auto Views = M->Night && M->Night->Glimpses ? M->Night->Glimpses->Individuals() : TArray<FS47RoswellGroup::FView>();
    Name = TEXT("Roswell"); Worn = TEXT("SK_Tripo_Roswell2");
    if (!Views.IsEmpty()) { Feet = Views[0].Pos; bDrawn = Views[0].bRendered; }
   }
   else if (Kind == 0) { Name = TEXT("Pale One"); Feet = PaleAt; bDrawn = PaleLeft > 0 && Pale.Rendered(); Worn = WornBy(Pale); }
   else
   {
    const int32 Gait = Kind == 1 ? int32(GaitPale) : Kind == 2 ? int32(GaitTall) : int32(GaitCrab);
    Name = Kind == 1 ? TEXT("Pale One walking") : Kind == 2 ? TEXT("Tall one") : TEXT("Crab");
    Feet = Gaits[Gait].At; bDrawn = Gaits[Gait].Left > 0 && GaitBody(Gait).Rendered(); Worn = WornBy(GaitBody(Gait));
   }
   // As anyone would, turned to it; drawn is what the renderer says it has drawn, which needs it in view.
   if (!Feet.IsZero()) ProofAim(Feet + FVector(0, 1.1, 0));
   if (Since < 3.) return true;
   if (!ProofCheck(bDrawn, FString::Printf(TEXT("%s drawn at %s, wearing %s, player's feet at %.2f"), *Name, *Feet.ToCompactString(), *Worn, M->Player->FloorY))) return true;
   if (ProofIndex < 8 && !ProofCheck(Kind == 2 ? Worn == TEXT("SK_Tripo_TallOne") : Worn == TEXT("SK_Tripo_PaleOne"), FString::Printf(TEXT("%s wears its own model"), *Name))) return true;
   FHitResult Under; FCollisionQueryParams Q(SCENE_QUERY_STAT(S47DevProofFeet), true);
   const bool bHit = M->Web->GetWorld()->LineTraceSingleByChannel(Under, S47Three::ToUE(Feet.X, Feet.Y + 1., Feet.Z), S47Three::ToUE(Feet.X, Feet.Y - 3., Feet.Z), ECC_Visibility, Q);
   UE_LOG(LogS47DevEvents, Display, TEXT("S47: dev proof feet: %s %.2f m above what the trace finds under it"), *Name, bHit ? Feet.Y - S47Three::ToThree(Under.ImpactPoint).Y : -999.);
  }
  ProofShot = FString::Printf(TEXT("body-%02d"), ProofIndex); Go(23); return true;
 case 23:
  if (Since < .3) return true;
  Stop();
  if (++ProofIndex <= 8) { Go(21); return true; }
  ProofFinish(true, TEXT("natural schedule, collision, previews, every formation and pace, every body outdoors; how they look is for the pictures"));
  return true;
 default: return true;
 }
}
