#include "S47Taken.h"
#include "S47CalicheAccess.h"
#include "S47FarmSite.h"
#include "S47FarmHouse.h"
#include "S47IndoorDread.h"
#include "S47Ambience.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Audio.h"
#include "S47Album.h"
#include "S47FieldCamera.h"
#include "S47UI.h"
#include "S47Three.h"
#include "S47Story.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FieldRadio.h"
#include "S47Night.h"
#include "S47Flashlight.h"
#include "S47UfoPresentation.h"
#include "SS47AwayOverlay.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/LocalFogVolumeComponent.h"
#include "S47Flow.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

namespace
{
 constexpr double DreadSeconds=13,FloodSeconds=20,TakenAfter=5,LiftSeconds=6.5;
 const FS47TakenTerms RoswellTerms{35,70,true,0,0,false,FLinearColor(1.f,.60f,.24f)};
 const FS47TakenTerms CrabTerms{120,240,false,150,300,false,FLinearColor(1.f,.36f,.12f)};
 const FS47TakenTerms TallTerms{300,420,true,0,0,true,FLinearColor(.80f,.84f,1.f)};
 FString ClockText(double Seconds)
 {
  const int32 S=int32(FMath::Fmod(Seconds,86400.));
  return FString::Printf(TEXT("%02d:%02d"),S/3600,(S/60)%60);
 }
}

const FS47TakenTerms& FS47Taken::Terms(ES47Taker Who){return Who==ES47Taker::Crab?CrabTerms:Who==ES47Taker::Tall?TallTerms:RoswellTerms;}

FS47Taken::~FS47Taken(){Stop();}

FVector FS47Taken::Feet() const{return M&&M->Player?FVector(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z):FVector::ZeroVector;}

bool FS47Taken::Indoors() const
{
 if(!M||!M->Player||!M->Web||!M->Web->GetWorld())return false;
 const FVector Eye=Feet()+FVector(0,1.6,0);FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47TakenRoof),true);
 return M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(Eye),S47Three::ToUE(Eye+FVector(0,9,0)),ECC_Visibility,Q);
}

bool FS47Taken::Begin(ES47Taker Who,const FString& Title,bool bOnlyDread)
{
 if(Busy()||!M||!M->Player||!M->Freeform||(M->World&&M->World->bDriving))return false;
 Taker=Who;Place=Title;Age=0;InLight=0;SafeClock=M->Freeform->Clock();bDoorDone=false;
 bOutdoors=!Indoors();
 UE_LOG(LogTemp,Display,TEXT("S47: taken begins, %s, %s"),bOutdoors?TEXT("in the open"):TEXT("under a roof"),*Title);
 if(!bOutdoors||bOnlyDread)
 {
  Now=EStage::Dread;StepIn=.6;TalkIn=3.5;StepAt=Feet()+FVector(-3,3.4,1);
  return true;
 }
 return StartFlood();
}

bool FS47Taken::TakeNow(ES47Taker Who,const FString& Title)
{
 if(!M||!M->Player||!M->Freeform)return false;
 if(Now==EStage::Lift||Now==EStage::Away)return false;
 Taker=Who;Place=Title;
 if(Now==EStage::None)SafeClock=M->Freeform->Clock();
 bOutdoors=!Indoors();
 if(!Terms(Who).bLift){Now=EStage::Away;Age=0;HideLight();White(TOptional<double>());if(M->UI)M->UI->Fade(true);M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=.9f});return true;}
 // In a shut room, where no light can reach: it comes straight down on them all the same, once they are held.
 if(Now!=EStage::Flood&&!ChooseLight()){From=Feet()+FVector(0,14,0);Aim=Feet()+FVector(0,1.25,0);}
 Lift();return true;
}

bool FS47Taken::ChooseLight()
{
 if(!M||!M->Player||!M->Web||!M->Web->GetWorld())return false;
 const FVector Chest=Feet()+FVector(0,1.25,0);
 bSpill=false;
 if(bOutdoors){From=Feet()+FVector(0,24,0);Aim=Chest;return true;}
 // The way the light can actually get in: a window or a door, seen from out there. Walls give it its shape.
 for(const double Dist:{16.,24.,10.})for(int32 I=0;I<16;++I)
 {
  const double A=I*UE_DOUBLE_PI/8+.2;
  for(const double Up:{7.,4.,2.})
  {
   const FVector Try=Chest+FVector(FMath::Cos(A)*Dist,Up,FMath::Sin(A)*Dist);
   const FVector Save=From;From=Try;
   if(LightReaches(Chest)){Aim=Chest;return true;}
   From=Save;
  }
 }
 // Nothing reaches them where they stand: then it fills the room next door, seen through the doorway, and they are
 // safe only as long as they stay out of it.
 const FVector Eye=Feet()+FVector(0,1.6,0);
 for(const double Near:{4.,6.,8.,2.5})for(int32 J=0;J<12;++J)
 {
  const double B=J*UE_DOUBLE_PI/6;
  const FVector Spot2=Chest+FVector(FMath::Cos(B)*Near,0,FMath::Sin(B)*Near);
  FHitResult See;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47TakenSpill),true);
  if(M->Web->GetWorld()->LineTraceSingleByChannel(See,S47Three::ToUE(Eye),S47Three::ToUE(Spot2),ECC_Visibility,Q))continue;
  for(int32 I=0;I<16;++I)
  {
   const double A=I*UE_DOUBLE_PI/8+.2;
   for(const double Up:{4.,2.})
   {
    From=Spot2+FVector(FMath::Cos(A)*16,Up,FMath::Sin(A)*16);
    if(LightReaches(Spot2)){Aim=Spot2;bSpill=true;return true;}
   }
  }
 }
 return false;
}

bool FS47Taken::LightReaches(const FVector& Chest) const
{
 if(!M||!M->Web||!M->Web->GetWorld())return false;
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47TakenLight),true);
 const FVector A=S47Three::ToUE(From),B=S47Three::ToUE(Chest);
 if(!M->Web->GetWorld()->LineTraceSingleByChannel(Hit,A,B,ECC_Visibility,Q))return true;
 return Hit.Distance>FVector::Dist(A,B)-30.;
}

bool FS47Taken::StartFlood()
{
 if(!ChooseLight())
 {
  // No way in for it, wherever it looks: the walls hold, this time.
  UE_LOG(LogTemp,Display,TEXT("S47: taken: no window for the light to come through"));
  Now=EStage::None;return false;
 }
 Now=EStage::Flood;Age=0;InLight=0;FloodFeet=Feet();FloodArea=M->Web?M->Web->Area():FString();
 if(M->Audio)
 {
  M->Audio->Sfx(TEXT("thudSoft"),FS47Sound{.Gain=1.f,.Rate=.5f});
  M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.75f,.Rate=.7f,.At=From});
 }
 UE_LOG(LogTemp,Display,TEXT("S47: taken: the light comes on from %s"),*From.ToCompactString());
 return true;
}

FLinearColor FS47Taken::LightColor(double Time) const
{
 // Never quite one colour: it drifts through its own range, each kind in its own.
 static const FLinearColor Warm[3]={FLinearColor(1.f,.55f,.18f),FLinearColor(1.f,.82f,.45f),FLinearColor(1.f,.28f,.22f)};
 static const FLinearColor Cold[3]={FLinearColor(.82f,.86f,1.f),FLinearColor(.62f,.46f,1.f),FLinearColor(.45f,.95f,.95f)};
 static const FLinearColor Sick[3]={FLinearColor(1.f,.32f,.10f),FLinearColor(.72f,1.f,.30f),FLinearColor(1.f,.70f,.2f)};
 const FLinearColor* P=Taker==ES47Taker::Tall?Cold:Taker==ES47Taker::Crab?Sick:Warm;
 const double U=FMath::Fmod(Time/2.6,3.);const int32 A=FMath::FloorToInt(U);const double F=U-A;
 return FLinearColor::LerpUsingHSV(P[A%3],P[(A+1)%3],float(F*F*(3-2*F)));
}

double FS47Taken::Pulse(double Time) const
{
 // A steady glare broken by bursts of strobe. With calm flashes chosen, it only breathes.
 const bool bCalm=M&&M->Flow&&M->Flow->bCalmFlash;
 const double Breath=.86+.14*FMath::Sin(Time*2.3);
 if(bCalm||Time<2.2)return Breath;
 const double Into=FMath::Fmod(Time-2.2,4.3);
 if(Into<1.15)return FMath::Fmod(Into*11.,1.)<.5?1.25:.06;
 return Breath;
}

void FS47Taken::StepMist(double Dt)
{
 if(!M||!M->Web||!M->Player)return;
 const bool bWant=Now==EStage::Dread||Now==EStage::Flood||Now==EStage::Lift;
 MistK=FMath::FInterpConstantTo(MistK,bWant?1.:0.,Dt,bWant?.16:.3);
 auto* F=Mist.Get();
 if(!F)
 {
  if(MistK<=0)return;
  F=NewObject<ULocalFogVolumeComponent>(M->Web.Get(),TEXT("S47TakenMist"));
  F->SetupAttachment(M->Web->GetRootComponent());F->RegisterComponent();
  F->SetWorldScale3D(FVector(1.5,1.5,.5));F->SetFogPhaseG(.5f);F->SetHeightFogFalloff(320.f);
  Mist=F;
 }
 // A mist comes in round them, low at first, and it takes the colour of whatever is shining through it.
 F->SetWorldLocation(S47Three::ToUE(Feet()+FVector(0,1.1,0)));
 F->SetRadialFogExtinction(float(MistK*1.1));F->SetHeightFogExtinction(float(MistK*.7));
 F->SetFogAlbedo(Now==EStage::Flood||Now==EStage::Lift?LightColor(Age):FLinearColor(.82f,.8f,.76f));
 F->SetVisibility(MistK>.002);
}

void FS47Taken::ShowLight(double K)
{
 if(!M||!M->Web)return;
 auto* L=Spot.Get();
 if(!L)
 {
  L=NewObject<USpotLightComponent>(M->Web.Get(),TEXT("S47TakenFlood"));
  L->SetupAttachment(M->Web->GetRootComponent());L->SetMobility(EComponentMobility::Movable);L->RegisterComponent();
  L->SetIntensityUnits(ELightUnits::Candelas);L->SetCastShadows(true);L->SetVolumetricScatteringIntensity(5.f);
  L->ShadowResolutionScale=4.f;L->SetSourceRadius(55.f);L->SetSoftSourceRadius(90.f);L->MarkRenderStateDirty();L->SetInnerConeAngle(13.f);L->SetOuterConeAngle(24.f);L->SetAttenuationRadius(9000.f);L->SetIndirectLightingIntensity(1.f);
  Spot=L;
 }
 const FLinearColor Col=LightColor(Age);
 L->SetLightColor(Col);
 L->SetWorldLocation(S47Three::ToUE(From));
 L->SetWorldRotation((S47Three::ToUE(Aim)-S47Three::ToUE(From)).Rotation());
 L->SetIntensity(float(9000.*K));L->SetVisibility(K>.001);
 // What the beam throws back: the whole room comes up in it, not just the patch on the wall.
 auto* B=Fill.Get();
 if(!B)
 {
  B=NewObject<UPointLightComponent>(M->Web.Get(),TEXT("S47TakenFill"));
  B->SetupAttachment(M->Web->GetRootComponent());B->SetMobility(EComponentMobility::Movable);B->RegisterComponent();
  B->SetIntensityUnits(ELightUnits::Candelas);B->SetCastShadows(false);B->SetAttenuationRadius(950.f);B->SetVolumetricScatteringIntensity(1.f);
  Fill=B;
 }
 const FVector Toward=(From-Aim).GetSafeNormal();
 B->SetWorldLocation(S47Three::ToUE(Aim+Toward*1.6+FVector(0,.3,0)));
 B->SetLightColor(Col);B->SetIntensity(float((bOutdoors?30.:75.)*K));B->SetVisibility(K>.001);
 if(bOutdoors)
 {
  if(!Ufo)Ufo=MakeUnique<FS47UfoPresentation>(M->Web.Get());
  if(Ufo->Prepare())
  {
   FS47UfoPresentation::FFrame F;F.Position=From;F.GroundTarget=Feet();F.Seconds=Age;F.Width=9;F.Opacity=FMath::Min(1.,K);F.Beam=K;F.NearWhite=0;F.bSlowBeam=true;
   Ufo->Show(F);
  }
 }
}

void FS47Taken::HideLight()
{
 if(auto* L=Spot.Get()){L->SetIntensity(0);L->SetVisibility(false);}
 if(auto* B=Fill.Get()){B->SetIntensity(0);B->SetVisibility(false);}
 if(M&&M->Player)M->Player->MoveScale=1;
 if(M&&M->Audio)M->Audio->HoldRoomTone(1.f,1.2f);
 S47CalicheAccess::SetLampLevel(1);
 if(M&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,TEXT("taken"));
 if(Ufo)Ufo->Hide();
}

void FS47Taken::White(TOptional<double> K)
{
 if(!bOverlay&&K.IsSet()&&GEngine&&GEngine->GameViewport)
 {
  Overlay=SNew(SS47AwayOverlay);GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(),25);bOverlay=true;
 }
 if(Overlay)Overlay->SetLight(K);
}

void FS47Taken::Lift()
{
 Now=EStage::Lift;Age=0;
 if(!M||!M->Player)return;
 auto* W=M->Player.Get();
 StartEye=Feet()+FVector(0,W->Eye,0);StartYaw=W->Yaw;StartPitch=W->Pitch;
 BeforeCamera=W->CameraOverride;Hooked=W;bHooked=true;
 const TWeakPtr<bool> Guard=Alive;
 auto Prev=BeforeCamera;
 W->CameraOverride=[this,Guard,Prev](double Dt,const FVector2D& Move,const FVector2D& Look,FS47View& Out)
 {
  if(!Guard.IsValid())return Prev?Prev(Dt,Move,Look,Out):false;
  // Up off the floor, turning over slowly, the head falling back.
  const double T=FMath::Clamp(Age/LiftSeconds,0.,1.),E=T*T*(3-2*T);
  Out.Eye=StartEye+FVector(0,E*(bOutdoors?5.:.9),0);Out.Yaw=StartYaw+E*.5;Out.Pitch=FMath::Lerp(StartPitch,1.2,E);Out.Roll=E*.32;
  return true;
 };
 if(M->Audio)M->Audio->Sfx(TEXT("alienRadio"),FS47Sound{.Gain=.9f,.Rate=.55f});
 UE_LOG(LogTemp,Display,TEXT("S47: taken: lifted"));
}

void FS47Taken::Unhook()
{
 if(bHooked)if(auto* W=Hooked.Get())W->CameraOverride=BeforeCamera;
 BeforeCamera=nullptr;Hooked.Reset();bHooked=false;
}

void FS47Taken::Apply()
{
 if(!M||!M->Freeform||!M->Player)return;
 const FS47TakenTerms& T=Terms(Taker);
 const double Minutes=FMath::FRandRange(float(T.MinMinutes),float(T.MaxMinutes));
 Seconds=Minutes*60;
 // Once, and whole: the clock, the place and the note together, before anything can be saved in between.
 M->Freeform->TakenAway(Seconds);
 const double WakeClock=M->Freeform->Clock();
 if(M->Freeform->FarmSite)M->Freeform->FarmSite->SetCalm(WakeClock+3600);
 if(M->Freeform->IndoorDread)M->Freeform->IndoorDread->AfterTaken();
 const FVector Before=Feet();
 FString Where=TEXT("where I had been standing");
 if(T.bHome&&M->World)
 {
  M->World->PlaceTruck(TEXT("saro"));
  if(M->Web->Area()!=TEXT("saro"))M->World->Enter(TEXT("saro"));
  M->Player->Place(9.2,1.2,0);M->Player->Pitch=-.1;
  Where=TEXT("in the yard at SARO. The jeep is on its pad and I did not drive it there");
 }
 else if(T.WakeMax>0)
 {
  if(M->Night&&M->Night->Flashlight)M->Night->Flashlight->bOn=false;
  for(int32 I=0;I<40;++I)
  {
   const double A=FMath::FRandRange(0.f,6.283f),D=FMath::FRandRange(float(T.WakeMin),float(T.WakeMax));
   const double X=Before.X+FMath::Cos(A)*D,Z=Before.Z+FMath::Sin(A)*D;
   // Open ground only: not inside a room, a wall or the truck.
   if(!M->Player->OpenLand||!M->Player->OpenLand(X,Z)||!M->Player->Walkable(X,Z))continue;
   if(M->World&&M->World->Indoors(FVector(X,Before.Y+1,Z)))continue;
   bool bClear=true;
   for(const FS47Rect& R:M->Web->Colliders())if(R.bEnabled&&X>R.MinX-.5&&X<R.MaxX+.5&&Z>R.MinZ-.5&&Z<R.MaxZ+.5){bClear=false;break;}
   if(bClear){M->Player->Place(X,Z,A);Where=FString::Printf(TEXT("about %d metres from there, on the ground"),FMath::RoundToInt(D/10)*10);break;}
  }
 }
 // A frame on the roll that was not taken: no word about it. It is only there in the album, like the others.
 const bool bFrame=M->FieldCamera&&M->FieldCamera->bHave&&M->Night&&M->Night->Album&&M->Night->Album->AddUnbidden(FMath::FRand()<.5f);
 Note=FString::Printf(TEXT("%s. Last time I am sure of: %s. When I next looked: %s, a gap of about %d minutes I cannot account for. I came to %s. %s %s are as I left them. This records what I can check, not what happened."),*Place,*ClockText(SafeClock),*ClockText(WakeClock),FMath::RoundToInt((WakeClock-SafeClock)/60.),*Where,Taker==ES47Taker::Crab?TEXT("There is dirt down the front of my jacket and my palms are scraped."):Taker==ES47Taker::Tall?TEXT("My boots are on and tied, tighter than I tie them."):TEXT("There is dried blood under my nose, and it is not sore."),bFrame?TEXT("My papers"):TEXT("My film and papers"));
 Note[0]=FChar::ToUpper(Note[0]);
 M->Freeform->RecordEvidence(FString::Printf(TEXT("taken-%d-%d"),M->Freeform->Day(),int32(SafeClock)),Note);
 if(M->Freeform->Director)M->Freeform->Director->RestFor(1800);
 UE_LOG(LogTemp,Display,TEXT("S47: taken away for %.0f minutes, %s"),Minutes,*Where);
}

void FS47Taken::Finish()
{
 Now=EStage::None;Age=0;InLight=0;HideLight();White(TOptional<double>());Unhook();
 if(M&&M->UI)
 {
  M->UI->Fade(false);
  if(Seconds>0&&M->Freeform)M->UI->Toast(FString::Printf(TEXT("%s. Your watch has moved on. It is in your field records (TAB)."),*ClockText(M->Freeform->Clock())),7);
 }
 Seconds=0;
 if(M)M->RequestAutosave();
}

void FS47Taken::GiveUp()
{
 if(Now==EStage::Dread||Now==EStage::Flood)
 {
  // Out of it with the roof still over them: that room will know them again, later.
  if(Now==EStage::Flood&&!bOutdoors&&M&&M->Freeform&&M->Freeform->IndoorDread&&!FloodArea.IsEmpty())M->Freeform->IndoorDread->SurvivedFlood(FloodFeet,FloodArea,int32(Taker),Place);
  Now=EStage::None;Age=0;InLight=0;HideLight();UE_LOG(LogTemp,Display,TEXT("S47: taken: the light goes out"));
 }
}

void FS47Taken::Stop()
{
 const bool bLive=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);
 Alive=MakeShared<bool>(true);
 Now=EStage::None;Age=0;Seconds=0;
 if(bLive)
 {
  Unhook();HideLight();White(TOptional<double>());
  if(auto* L=Spot.Get())L->DestroyComponent();
  if(auto* B=Fill.Get())B->DestroyComponent();
  if(auto* F=Mist.Get())F->DestroyComponent();
  if(Overlay&&GEngine&&GEngine->GameViewport)GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
  if(Ufo)Ufo->Hide();
 }
 Ufo.Reset();Overlay.Reset();bOverlay=false;Spot.Reset();Fill.Reset();Mist.Reset();MistK=0;
}

void FS47Taken::Step(double Dt)
{
 if(!M||!M->Player||!M->Freeform||!M->Audio||Dt<=0)return;
 StepMist(Dt);
 if(Now==EStage::None)return;
 if(M->Freeform->Dead()){GiveUp();return;}
 // Held with the game: a menu or a panel is no time spent in the light.
 if(M->bPausedByMenu||M->ModalOpen())return;
 Age+=Dt;
 if(M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(Now==EStage::Dread?.3+.45*FMath::Abs(FMath::Sin(Age*1.7)):Now==EStage::Flood?.85:0.,TEXT("taken"));
 if(Now==EStage::Dread)M->Audio->HoldRoomTone(.15f,.8f);
 // The night outside stops too, wherever it began from; a stretch at a time, so it comes back by itself.
 if(Now==EStage::Dread||Now==EStage::Flood)
 {
  HushIn-=Dt;
  if(HushIn<=0&&M->Audio->Night.IsValid()){HushIn=3;M->Audio->Night->Hush(5,.05,.8,3);}
 }
 else HushIn=0;
 if(Now==EStage::Dread)
 {
  // The room's own lamp is failing.
  S47CalicheAccess::SetLampLevel(.55+.45*FMath::Sin(Age*7.3)*FMath::Sin(Age*2.1));
  // Someone on the roof, going over and back. Then something said, not quite language.
  StepIn-=Dt;
  if(StepIn<=0)
  {
   StepIn=FMath::FRandRange(.55f,1.3f);
   const FVector Over=Feet()+FVector(0,3.4,0);
   StepAt+=(Over+FVector(FMath::Sin(Age*.6)*4,0,FMath::Cos(Age*.43)*4)-StepAt)*.3;
   M->Audio->Sfx(TEXT("stepConcrete0"),FS47Sound{.Gain=.8f,.Rate=FMath::FRandRange(.72f,.92f),.At=StepAt});
  }
  TalkIn-=Dt;
  if(TalkIn<=0)
  {
   TalkIn=FMath::FRandRange(5.f,8.f);
   if(M->FieldRadio)M->FieldRadio->Talk(StepAt+FVector(1.5,-1,1));
  }
  // A door in the house moves, with nobody at it.
  if(!bDoorDone&&Age>=6.5)
  {
   bDoorDone=true;
   if(M->Freeform->HouseDoors&&M->Web->Area()==TEXT("saro"))
   {
    double Best=11;int32 Door=INDEX_NONE;
    for(const auto& Plan:S47FarmHouse::DoorPlans())
    {
     const int32 I=M->Freeform->HouseDoors->IndexOf(Plan.Name);if(I==INDEX_NONE)continue;
     const FVector P=Plan.From+FVector(-24,0,300);
     const double D=FVector2D::Distance(FVector2D(P.X,P.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
     if(D<Best&&D>1.4){Best=D;Door=I;}
    }
    if(Door!=INDEX_NONE)M->Freeform->HouseDoors->Use(Door);
   }
   else if(M->Web->Area()==TEXT("diner")||M->Web->Area()==TEXT("road"))S47CalicheAccess::MoveNearestDoor(Feet(),11);
  }
  if(Age>=DreadSeconds&&!StartFlood())GiveUp();
  return;
 }
 if(Now==EStage::Flood)
 {
  const double K=Pulse(Age)*FMath::Clamp(Age/.18,0.,1.)*(1-FMath::SmoothStep(FloodSeconds-1.2,FloodSeconds,Age));
  const FVector Chest=Feet()+FVector(0,1.25,0);
  // It looks for them at walking pace, so a shadow or a wall is a real way out.
  S47CalicheAccess::SetLampLevel(0);
  const bool bReach=LightReaches(Chest);
  if(bReach||!bSpill)Aim=FMath::VInterpConstantTo(Aim,Chest,Dt,1.5);
  ShowLight(K);
  const bool bCaught=FVector::Dist(Aim,Chest)<1.6&&bReach;
  InLight=bCaught?InLight+Dt:FMath::Max(0.,InLight-Dt*.6);
  // The legs go: the longer in it, the harder to leave it.
  M->Player->MoveScale=FMath::Lerp(1.,.22,FMath::Clamp(InLight/TakenAfter,0.,1.));
  if(InLight>=TakenAfter){M->Player->MoveScale=1;Lift();return;}
  if(Age>=FloodSeconds||(M->World&&M->World->bDriving))GiveUp();
  return;
 }
 if(Now==EStage::Lift)
 {
  ShowLight(1+Age*.4);
  White(FMath::SmoothStep(LiftSeconds*.45,LiftSeconds,Age));
  if(Age>=LiftSeconds)
  {
   Now=EStage::Away;Age=0;HideLight();
   if(M->UI)M->UI->Fade(true);
  }
  return;
 }
 if(Now==EStage::Away)
 {
  // Sound and picture gone; the time passes out here, once. Nothing to walk with till it is back.
  M->Player->MoveScale=0;
  if(Seconds<=0&&Age>=.5)
  {
   White(TOptional<double>());Unhook();Apply();
  }
  if(Age>=3.)Finish();
 }
}

bool FS47Taken::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s taken: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 const auto& R=Terms(ES47Taker::Roswell);const auto& C=Terms(ES47Taker::Crab);const auto& T=Terms(ES47Taker::Tall);
 Check(R.MaxMinutes<=C.MinMinutes&&C.MaxMinutes<=T.MinMinutes,TEXT("each kind of taker costs more time than the one before"));
 Check(R.bLift&&!C.bLift&&T.bLift,TEXT("the crab takes in the dark, the others in the light"));
 Check(R.WakeMax==0&&!R.bHome&&C.WakeMin>=100&&!C.bHome&&T.bHome,TEXT("each comes to in its own place"));
 // Who comes where, from the visit's own dice: each kind can be drawn at each place, only in its hours.
 {
  int32 Farm[3]={0,0,0},Town[3]={0,0,0},Early[3]={0,0,0};
  for(int32 I=0;I<1000;++I)
  {
   const float Roll=(I+.5f)/1000.f;
   ++Farm[FS47FarmSite::PickWho(TEXT("farm"),4.,Roll)];++Town[FS47FarmSite::PickWho(TEXT("cinema"),4.,Roll)];++Early[FS47FarmSite::PickWho(TEXT("farm"),2.5,Roll)];
  }
  Check(Farm[0]>0&&Farm[1]>0&&Farm[2]>0&&Town[0]>0&&Town[1]>0&&Town[2]>0,TEXT("all three kinds can come, at the farm and in town"));
  Check(Farm[1]>Town[1]&&Town[0]>Farm[0],TEXT("the crab keeps to open ground, town is mostly the small ones"));
  Check(Early[2]==0&&Early[0]>0&&Early[1]>0,TEXT("the tall one keeps to the last of the night"));
 }
 const double Night2=86400.+19*3600.;
 Check(!FS47FarmSite::TheirHours(Night2)&&!FS47FarmSite::TheirHours(Night2+4.5*3600.)&&FS47FarmSite::TheirHours(Night2+7.5*3600.)&&!FS47FarmSite::TheirHours(Night2+12*3600.),TEXT("no body before about two in the morning, none after six"));
 OK&=FS47IndoorDread::SelfTest(Lines);
 return OK;
}
