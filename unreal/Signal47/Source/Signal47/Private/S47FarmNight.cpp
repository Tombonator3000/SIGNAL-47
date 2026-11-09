#include "S47FarmNight.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"
#include "S47Herd.h"
#include "S47Animals.h"
#include "S47TownLayer.h"
#include "S47FarmSite.h"
#include "S47Taken.h"
#include "S47FreeformDirector.h"
#include "S47WebAudio.h"
#include "Components/LocalFogVolumeComponent.h"
#include "HAL/IConsoleManager.h"
#include "S47OutsideLight.h"
#include "S47RoomLights.h"
#include "S47SaroNight.h"
#include "S47SaroPower.h"
#include "S47Audio.h"
#include "S47FarmHouse.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47StreetLamps.h"
#include "Engine/World.h"
#include "Components/SpotLightComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
 const TCHAR* LoopKey=TEXT("farm-wind");
 constexpr double FarmX=84,FarmZ=492,FarmReach=190; // Middle of the farm in the saro frame, and how far its sounds carry.
 FVector Size(double X,double Y,double Z){const FVector V=S47Three::DirToUE(FVector(X,Y,Z));return FVector(FMath::Abs(V.X),FMath::Abs(V.Y),FMath::Abs(V.Z));}
 // The substation's own frame (Tools/substation): identity panel at (0,-7.28,3.25), facing the road side.
 FVector Substation(double X,double Y,double Z){return FVector(7890,-1.10,536.20)+FVector(-X,Z,Y);}
}

FVector FS47FarmNight::EntryLampRoad(){return FVector(15,-1,190);}
FVector FS47FarmNight::TrafoLampWorld(){return FVector(7990,-1,512);}
FVector FS47FarmNight::SignWorld(){return Substation(0,-7.28,3.25);}
FVector FS47FarmNight::SignLampWorld(){return Substation(0,-8.5,4.7);}

bool FS47FarmNight::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web))return false;
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Cube||!Base)return false;
 auto Part=[&](FLamp& L,const FString& Name,const FVector& Offset,const FVector& Scale,const FLinearColor& Color)
 {
  auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),Color);Mat->SetScalarParameterValue(TEXT("Roughness"),.6);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
  auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Cube);C->SetMaterial(0,Mat);
  C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->SetWorldScale3D(Scale);C->RegisterComponent();
  L.Parts.Add(C);L.Offsets.Add(Offset);return Mat;
 };
 auto Beam=[&](FLamp& L,const FString& Name,float Cone,float Reach)
 {
  auto* S=NewObject<USpotLightComponent>(M->Web,*Name);S->SetupAttachment(M->Web->GetRootComponent());S->SetMobility(EComponentMobility::Movable);
  S->SetIntensityUnits(ELightUnits::Candelas);S->SetIntensity(0);S->SetInnerConeAngle(Cone*.6f);S->SetOuterConeAngle(Cone);S->SetAttenuationRadius(Reach*100);S->SetCastShadows(false);S->SetVisibility(false);
  // STREETLAMPS-20261108: the road lamps are high-pressure sodium like every other street lamp (they were white); the sign's gooseneck stays an ordinary bulb
  if(!Name.Contains(TEXT("SignLamp")))S->SetLightColor(S47StreetLamps::Sodium());
  S->RegisterComponent();
  L.Light=S;
 };
 const FLinearColor Steel(.09,.09,.085);
 // The roadside poles are one instanced model. New lamps use the pole standing at the turn-off, and its height.
 UStaticMesh* PoleMesh=nullptr;FVector PoleScale=FVector::OneVector;double PoleTop=7.6;
 for(TObjectIterator<UInstancedStaticMeshComponent> It;It;++It)
 {
  auto* C=*It;if(C->GetWorld()!=M->Web->GetWorld()||!C->GetStaticMesh()||!C->GetStaticMesh()->GetName().Contains(TEXT("telephone_poles_Small_Pole")))continue;
  FTransform T;if(C->GetInstanceCount()>0&&C->GetInstanceTransform(0,T,true))
  {
   PoleMesh=C->GetStaticMesh();PoleScale=T.GetScale3D();
   const FBox Box=PoleMesh->GetBoundingBox();PoleTop=FMath::Clamp(Box.Max.Z*PoleScale.Z/100.,5.,12.)-.8;
   break;
  }
 }
 UE_LOG(LogTemp,Display,TEXT("S47: farm lamps use pole mesh %s, head at %.1f m"),*GetNameSafe(PoleMesh),PoleTop);
 auto LampOn=[&](FLamp& L,const FString& Tag,const FVector& Foot,const FVector& Toward,float Candela)
 {
  // An arm from the pole's top toward the road, its sodium head at the end.
  const FVector Dir=(Toward-Foot).GetSafeNormal2D();const FVector Top=Foot+FVector(0,PoleTop,0);
  const FVector ArmAt=Top+FVector(Dir.X,0,Dir.Z)*1.2,HeadAt=Top+FVector(Dir.X,-.1,Dir.Z)*2.4;
  const bool bAlongX=FMath::Abs(Dir.X)>FMath::Abs(Dir.Z);
  Part(L,TEXT("S47")+Tag+TEXT("Arm"),ArmAt,bAlongX?Size(2.5,.09,.09):Size(.09,.09,2.5),Steel);
  L.Head=Part(L,TEXT("S47")+Tag+TEXT("Head"),HeadAt,bAlongX?Size(.72,.13,.34):Size(.34,.13,.72),Steel);
  L.At=HeadAt+FVector(0,-.1,0);L.Aim=FVector(Toward.X,Foot.Y+.5,Toward.Z);L.Candela=Candela;
  Beam(L,TEXT("S47")+Tag+TEXT("Light"),66,36);
 };
 { // On the pole already standing by the farm's turn-off, reaching over the entrance.
  FLamp L;L.bRoadLocal=true;
  LampOn(L,TEXT("FarmLamp"),EntryLampRoad(),FVector(15,0,S47RoadTerrain::RanchZ(15)),1100);Lamps.Add(L);
 }
 { // Where the service track to the substation leaves the highway: no pole stands on that side, so one of the same kind does.
  FLamp L;
  // STREETLAMPS-20261108: its foot on the ground measured under it (it was set at y -1 by hand): the first thing straight down.
  FVector Foot=TrafoLampWorld();
  {
   FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47TrafoLampFoot),true);
   if(UWorld* World=M->Web->GetWorld();World&&World->LineTraceSingleByChannel(Hit,S47Three::ToUE(Foot.X,Foot.Y+12,Foot.Z),S47Three::ToUE(Foot.X,Foot.Y-12,Foot.Z),ECC_Visibility,Q))
   {
    const double Y=S47Three::ToThree(Hit.ImpactPoint).Y;
    UE_LOG(LogTemp,Display,TEXT("S47: the trafo lamp's pole stands on %s at y %.3f (was %.3f by hand)"),*GetNameSafe(Hit.GetComponent()),Y,Foot.Y);
    Foot.Y=Y-.05; // set a little into it, as a pole is
   }
  }
  LampOn(L,TEXT("TrafoLamp"),Foot,FVector(7997,0,520),1100);
  if(PoleMesh)
  {
   auto* C=NewObject<UStaticMeshComponent>(M->Web,TEXT("S47TrafoLampPole"));C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(PoleMesh);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetWorldScale3D(PoleScale);C->RegisterComponent();
   L.Parts.Add(C);L.Offsets.Add(Foot);
  }
  Lamps.Add(L);
 }
 { // A gooseneck lamp over the substation's identity panel, so the name reads from the road.
  FLamp L;L.At=SignLampWorld();L.Aim=SignWorld();L.Candela=38;
  Part(L,TEXT("S47SignLampArm"),Substation(0,-7.85,4.45),Size(.05,.05,1.3),Steel);
  L.Head=Part(L,TEXT("S47SignLampHead"),Substation(0,-8.5,4.45),Size(.42,.09,.26),Steel);
  Beam(L,TEXT("S47SignLampLight"),58,12);Lamps.Add(L);
 }
 bBuilt=true;return true;
}

void FS47FarmNight::Place(const FString& Area)
{
 const bool bRoadSide=Area==TEXT("road")||Area==TEXT("diner");
 for(FLamp& L:Lamps)
 {
  const bool bShown=L.bRoadLocal?(bRoadSide||Area==TEXT("saro")):bRoadSide;
  auto To=[&](const FVector& P){return L.bRoadLocal?S47FarmWorld::RoadToArea(Area,P):P;};
  for(int32 I=0;I<L.Parts.Num();++I)if(auto* C=L.Parts[I].Get())
  {
   if(bShown)C->SetWorldLocation(S47Three::ToUE(To(L.Offsets[I])));
   C->SetVisibility(bShown);C->SetHiddenInGame(!bShown);
  }
  if(auto* S=L.Light.Get())if(bShown)S->SetWorldLocationAndRotation(S47Three::ToUE(To(L.At)),S47Three::DirToUE(L.Aim-L.At).Rotation());
 }
}

void FS47FarmNight::SetOn(bool bNow,const FString& Area)
{
 bOn=bNow;const bool bRoadSide=Area==TEXT("road")||Area==TEXT("diner");
 for(FLamp& L:Lamps)
 {
  const bool bShown=L.bRoadLocal?(bRoadSide||Area==TEXT("saro")):bRoadSide;
  if(auto* S=L.Light.Get()){S->SetIntensity(bNow&&bShown?L.Candela:0);S->SetVisibility(bNow&&bShown);}
  if(auto* Mat=L.Head.Get()){Mat->SetVectorParameterValue(TEXT("Emissive"),bNow?FLinearColor(1,.68,.34)*40.f:FLinearColor::Black);Mat->SetScalarParameterValue(TEXT("EmissivePower"),bNow?1.f:0.f);}
 }
}

void FS47FarmNight::Stop()
{
 const bool bLive=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);
 if(bWind&&bLive&&IsValid(M->Audio))M->Audio->Stop(LoopKey,.5f);bWind=false;
 for(FLamp& L:Lamps)
 {
  for(const auto& C:L.Parts)if(C.IsValid()){C->SetVisibility(false);C->SetHiddenInGame(true);}
  if(auto* S=L.Light.Get()){S->SetIntensity(0);S->SetVisibility(false);}
 }
 LastArea.Empty();bOn=false;
 HazeNear(false);HazeGlow=0;
 // Back to the Story's air, when Freeform ends with the scene still up.
 if(!IsEngineExitRequested()&&!IsGarbageCollecting())for(const auto& H:Hazes)if(auto* F=H.Get())F->DestroyComponent();
 Hazes.Reset();
}

void FS47FarmNight::Step(double Dt)
{
 if(!M||!IsValid(M->Web)||!M->Freeform)return;
 if(Scene.Get()!=M->Web){Scene=M->Web;bBuilt=false;Lamps.Reset();LastArea.Empty();Hazes.Reset();}
 if(!Build())return;
 const FString Area=M->Web->Area();
 const double Hour=FMath::Fmod(M->Freeform->Clock(),86400.)/3600.;const bool bNight=Hour>=18.75||Hour<6.25;
 const bool bAreaChanged=Area!=LastArea;
 if(bAreaChanged){Place(Area);LastArea=Area;}
 if(bAreaChanged||bNight!=bOn)SetOn(bNight,Area);
 BuildHaze();
 StepHaze(Dt,bNight);
 // A menu or a panel holds them: nothing starts in a paused sound context, to all come at once on the way out.
 if(M->bPausedByMenu||M->ModalOpen())return;
 StepCattle(Dt,Area);
 StepSound(Dt,bNight,Area);
}

namespace
{
 // A cow, made here with no recording: a buzzing voice through a long throat, sliding up and down. Fear makes it
 // higher, rougher and shorter.
 TSharedPtr<FS47AudioBuffer> MooBuffer(bool bAfraid,int32 Seed)
 {
  static constexpr int32 Rate=22050;
  const double Dur=bAfraid?1.25:2.1;
  auto B=MakeShared<FS47AudioBuffer>(1,FMath::RoundToInt(Rate*Dur),Rate);float* Out=B->GetChannelData(0);FRandomStream R(4711+Seed);
  const double Lo=bAfraid?150:82,Up=bAfraid?248:128;
  double Ph=0,X1=0,V1=0,X2=0,V2=0,Hp=0,Lp=0;
  const auto Res=[](double F,double Bw,double& X,double& V,double In)
  {const double W=2*UE_DOUBLE_PI*F/Rate,Rr=FMath::Exp(-UE_DOUBLE_PI*Bw/Rate);const double N=2*Rr*FMath::Cos(W)*X-Rr*Rr*V+In;V=X;X=N;return N;};
  for(int32 I=0;I<B->Length();++I)
  {
   const double T=double(I)/Rate,U=T/Dur;
   const double Glide=FMath::Sin(UE_DOUBLE_PI*FMath::Pow(U,.7));
   const double F0=FMath::Lerp(Lo,Up,Glide)*(1+(bAfraid?.02:.008)*FMath::Sin(T*(bAfraid?38.:22.)));
   Ph+=F0/Rate;if(Ph>=1)Ph-=1;
   const double Src=(Ph*2-1)+(bAfraid?(R.FRand()*2-1)*.25:0);
   const double Open=FMath::Lerp(280.,640.,Glide);
   const double V=Res(Open,110,X1,V1,Src*.1)+.5*Res(Open*2.4,190,X2,V2,Src*.1);
   Lp+=.5*(V-Lp);Hp+=.012*(Lp-Hp);
   const double Env=FMath::SmoothStep(0.,.14,U)*(1-FMath::SmoothStep(.78,1.,U));
   Out[I]=float(FMath::Atan((Lp-Hp)*1.3)*.72*Env);
  }
  return B;
 }
 void PlayMoo(US47Audio* A,const FVector& At,bool bAfraid)
 {
  if(!A||!A->bReady||!A->Ctx||!A->SfxBus)return;
  // Real cows (BigSoundBank, CC0, see FREE_ASSET_CATALOG) where the files are there; the made voice otherwise.
  static TSharedPtr<FS47AudioBuffer> Real[3],Made[2][3];
  const int32 K=FMath::RandRange(0,2);
  if(!Real[K])
  {
   TArray<uint8> Bytes;
   if(FFileHelper::LoadFileToArray(Bytes,*(FPaths::ProjectContentDir()/FString::Printf(TEXT("Audio/Raw/sound_design/cow_moo_%d.wav"),K))))Real[K]=S47Wa::DecodeWav(Bytes);
  }
  auto Buf=Real[K];
  if(!Buf){auto& M2=Made[bAfraid?1:0][K];if(!M2)M2=MooBuffer(bAfraid,K);Buf=M2;}
  FS47AudioContext::FBatch Batch(*A->Ctx);
  auto Src=A->Ctx->CreateBufferSource();Src->Buffer=Buf;Src->PlaybackRate.SetValue(Real[K]&&bAfraid?FMath::FRandRange(1.25f,1.55f):FMath::FRandRange(.92f,1.08f));
  auto Gain=A->Ctx->CreateGain();Gain->Gain.SetValue(bAfraid?.95:.7);
  auto Pan=A->Ctx->CreatePanner();Pan->RefDistance=9.;Pan->RolloffFactor=.55;Pan->MaxDistance=220.;Pan->SetPosition(At.X,At.Y,At.Z);
  Src->Connect(Gain);Gain->Connect(Pan);Pan->Connect(A->SfxBus);Src->Start(A->Ctx->CurrentTime());
 }
}

void FS47FarmNight::StepCattle(double Dt,const FString& Area)
{
 if(!M->TownLayer||M->TownLayer->Herds.IsEmpty()||!M->Player||!M->Freeform)return;
 FS47Herd* H=M->TownLayer->Herds[0].Get();if(!H)return;
 // They know before the player does: a visit at the farm, a taking, or the lights over it.
 FVector Threat=FVector::ZeroVector;double Level=0;
 const auto& Site=M->Freeform->FarmSite;
 if(Site&&Site->VisitWantsGround()&&Site->PlaceId()==TEXT("farm")){Threat=Site->Centre();Level=1;}
 if(M->Freeform->Taken&&M->Freeform->Taken->Busy()&&Area==TEXT("saro")&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(FarmX,FarmZ))<170)
 {Threat=FVector(M->Player->Pos.X,0,M->Player->Pos.Z);Level=1;}
 // Lights over the farm, whichever of the two draws put them there (S47EventPlaces): not lights over somewhere else.
 if(M->Freeform->Director&&Area==TEXT("saro")&&(M->Freeform->Director->ActiveEvent()==TEXT("farm_lights")||M->Freeform->Director->ActiveEvent()==TEXT("town_lights")))
 {
  const FVector Over=M->Freeform->Director->EventAnchor();
  if(FVector2D::Distance(FVector2D(Over.X,Over.Z),FVector2D(FarmX,FarmZ))<FarmReach+120){Threat=Over;Level=FMath::Max(Level,.7);}
 }
 if(!TestAlarmAt.IsZero()){Threat=TestAlarmAt;Level=1;}
 // ANIMALS-20261108: anything else near them (a figure, a light, a craft, FS47Omen): some way off they walk off together, all one
 // way; close by they run. Only on the herd's own ground.
 if(M->Animals&&Area==TEXT("saro")){FVector Felt;const double Fear=M->Animals->HerdFear(H->Middle(),Felt);if(Fear>Level){Level=Fear;Threat=Felt;}}
 H->SetAlarm(Level,Threat);
 // A site armed for the night with no visit under way: one cow stands quite still and faces the marks. Morning clears the site.
 bool bStare=Level<=0&&Site&&Site->Armed()&&Site->PlaceId()==TEXT("farm")&&!Site->VisitWantsGround();
 if(bStare&&StareCow<0)
 {
  double Best=1e18;
  for(int32 I=0;I<H->Count();++I){FVector P;if(I!=S47Herd::MISSING&&H->CowAt(I,P)){const double D=FVector::DistSquared(P,Site->Centre());if(D<Best){Best=D;StareCow=I;}}}
  bStare=StareCow>=0;
 }
 H->SetStare(bStare?StareCow:-1,bStare?Site->Centre():FVector::ZeroVector);
 if(!bStare)StareCow=-1;
 if(Area!=TEXT("saro"))return;
 const FVector Eye(M->Player->Pos.X,M->Player->FloorY+1.6,M->Player->Pos.Z);
 FVector Near;int32 Cow=FMath::RandRange(0,H->Count()-1);
 if(!H->CowAt(Cow,Near)||FVector::Dist(Eye,Near)>260)return;
 const bool bAfraid=Level>.3;
 if(bAfraid&&MooIn>3.4)MooIn=.4; // fear does not wait its turn
 MooIn-=Dt;if(MooIn>0)return;
 MooIn=bAfraid?FMath::FRandRange(1.1f,3.4f):FMath::FRandRange(16.f,44.f);
 PlayMoo(M->Audio,Near,bAfraid);++Moos;
}

static TAutoConsoleVariable<float> CvHazeGlow(TEXT("s47.haze.Glow"),.03f,TEXT("SARO's rooms at night: the air's own glow, seen from outside. 0: none."));
static TAutoConsoleVariable<float> CvHazeExtra(TEXT("s47.haze.Extra"),.12f,TEXT("SARO's rooms at night: extinction per metre on top of the day's."));
static TAutoConsoleVariable<float> CvHazeStart(TEXT("s47.haze.Start"),150.f,TEXT("r.LocalFogVolume.GlobalStartDistance in cm for someone out in the yard at night."));
static TAutoConsoleVariable<float> CvHazeIndoors(TEXT("s47.haze.Indoors"),.2f,TEXT("SARO's rooms at night: how much of the glow is left for someone inside."));

void FS47FarmNight::BuildHaze()
{
 if(!Hazes.IsEmpty()||!M||!M->Web)return;
 // The same faint air as the garage: lamplight and sun get a shape in it. One volume per room.
 struct FRoom{const TCHAR* Name;double X0,X1,Z0,Z1;double Y;};
 const FVector O(-24,0,300);const FVector A=S47FarmHouse::Point(0,0)+O,B=S47FarmHouse::Point(12,9)+O;
 const FRoom Rooms[]={{TEXT("Control"),-5.75,5.75,-4.22,4.22,1.6},{TEXT("Corridor"),-5.8,1.8,5.,6.3,1.6},{TEXT("Records"),-5.8,-1.2,7.,10.2,1.6},
  {TEXT("Lab"),12.8,19.,-5.8,.8,1.6},{TEXT("OldHouse"),FMath::Min(A.X,B.X),FMath::Max(A.X,B.X),FMath::Min(A.Z,B.Z),FMath::Max(A.Z,B.Z),A.Y+1.4}};
 for(const FRoom& R:Rooms)
 {
  auto* F=NewObject<ULocalFogVolumeComponent>(M->Web.Get(),*FString::Printf(TEXT("S47Haze_%s"),R.Name));
  F->SetupAttachment(M->Web->GetRootComponent());F->RegisterComponent();
  F->SetWorldLocation(S47Three::ToUE(FVector((R.X0+R.X1)*.5,R.Y,(R.Z0+R.Z1)*.5)));
  F->SetWorldScale3D(FVector((R.X1-R.X0)*.12,(R.Z1-R.Z0)*.12,.42));
  F->SetRadialFogExtinction(.35f);F->SetHeightFogExtinction(.15f);F->SetHeightFogFalloff(400.f);
  F->SetFogAlbedo(FLinearColor(.85f,.8f,.7f));F->SetFogPhaseG(.55f);
  Hazes.Add(F);
 }
}

void FS47FarmNight::HazeNear(bool bNear)
{
 if(bNear==bHazeNear)return;
 bHazeNear=bNear;
 if(IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.LocalFogVolume.GlobalStartDistance")))V->Set(bNear?CvHazeStart.GetValueOnGameThread():1000.f,ECVF_SetByGameOverride);
}

void FS47FarmNight::StepHaze(double Dt,bool bNight)
{
 // Tom, T16-13: a little more haze in SARO seen from outside at night, so the light indoors stands in the air. The lamps
 // there do not scatter into the volumetric fog (S47FixtureLighting: their flicker would leave a ghost in it, and each would be
 // a shadowed pass), so the rooms' volumes carry a faint glow of their own, only while there is power, at night, and most of it
 // for someone outside: from inside it would be a veil over the room. It is inside the walls, so the night outside is no lighter.
 HazeIn-=Dt;
 if(HazeIn>0||!M->Freeform||!M->Web)return;
 HazeIn=.25;
 const FS47Taken* Tk=M->Freeform->Taken.Get();
 const bool bHere=bNight&&!(Tk&&Tk->Busy())&&M->Freeform->HasPowerForReactiveLight()&&!(M->SaroNight&&M->SaroNight->Power&&M->SaroNight->Power->Out());
 const double Want=!bHere?0.:Tk&&Tk->Indoors()?CvHazeIndoors.GetValueOnGameThread():1.;
 HazeGlow=FMath::FInterpConstantTo(HazeGlow,Want,.25,.6);
 // Each time round, for new volumes and for a switch thrown; the setters do nothing when nothing changes.
 // The engine holds local fog 10 m off the eye: from the yard the room behind the glass would have no air in it. Only for
 // someone outside, where none of it is round them; indoors a room's extinction would be a black fog over its own lamps.
 if(!FS47OutsideLight::AnyShown())HazeNear(bNight&&Tk&&!Tk->Busy()&&!Tk->Indoors());
 const FLinearColor Colour=FLinearColor(1.f,.9f,.72f)*float(HazeGlow*CvHazeGlow.GetValueOnGameThread());
 const float Thicker=float(HazeGlow*CvHazeExtra.GetValueOnGameThread());
 TInlineComponentArray<ULocalFogVolumeComponent*> Fogs(M->Web.Get());
 for(ULocalFogVolumeComponent* F:Fogs)
 {
  const FString Name=F->GetName();
  const bool bOurs=Name.StartsWith(TEXT("EastWingHaze_"))||(Name.StartsWith(TEXT("S47Haze_"))&&!Name.EndsWith(TEXT("OldHouse")));
  if(!bOurs)continue;
  // A room with its switch off has nothing to glow with.
  const TCHAR* Switch=Name.EndsWith(TEXT("_Control"))?TEXT("control"):Name.EndsWith(TEXT("_Corridor"))?TEXT("corridor"):Name.EndsWith(TEXT("_Records"))?TEXT("records"):nullptr;
  const bool bLit=!Switch||!M->RoomLights||M->RoomLights->RoomEnabled(Switch);
  F->SetFogEmissive(bLit?Colour:FLinearColor::Black);
  F->SetRadialFogExtinction(.35f+(bLit?Thicker:0.f));
 }
}

void FS47FarmNight::StepSound(double Dt,bool bNight,const FString& Area)
{
 if(!M->Audio||!M->Player)return;
 const double Dist=Area==TEXT("saro")?FVector2D(M->Player->Pos.X-FarmX,M->Player->Pos.Z-FarmZ).Size():1e9;
 // Everything out here goes quiet before it starts. That is the warning.
 const bool bHush=(M->Freeform->FarmSite&&M->Freeform->FarmSite->VisitHushed())||(M->Freeform->Taken&&M->Freeform->Taken->Busy());
 const bool bAt=Dist<FarmReach&&!bHush;
 if(bAt!=bWind)
 {
  bWind=bAt;
  if(bAt){FS47Sound S;S.Gain=.34f;S.bLoop=true;M->Audio->LoopSfx(LoopKey,TEXT("wind"),S);UE_LOG(LogTemp,Display,TEXT("S47: farm sounds on, %.0f m out"),Dist);}
  else M->Audio->Stop(LoopKey,1.5f);
 }
 if(!bAt)return;
 SoundIn-=Dt;if(SoundIn>0)return;
 SoundIn=FMath::FRandRange(bNight?7.f:11.f,bNight?19.f:26.f);
 // Each has a place, so it turns and fades as the player moves: never a sound that follows them.
 struct FNoise{const TCHAR* Name;double RoadX,RoadZ;float Gain;float RateLow,RateHigh;bool bNightOnly;};
 static const FNoise Noises[]={
  {TEXT("doorCreak0"),118,176,.8f,.55f,.8f,false}, // the loose sheet on the shed
  {TEXT("doorCreak1"),98,221,.8f,.6f,.85f,false}, // the old house, its door moving
  {TEXT("doorMetal"),150,200,.6f,.7f,.95f,false},  // something metal in the far shed
  {TEXT("gust"),60,240,.7f,.9f,1.1f,false},
  {TEXT("owl"),150,262,.85f,.92f,1.05f,true},
  {TEXT("dog0"),30,160,.6f,.85f,1.f,true},
  {TEXT("crickets"),70,225,.38f,.95f,1.05f,true}};
 TArray<const FNoise*> Pick;for(const auto& N:Noises)if(bNight||!N.bNightOnly)Pick.Add(&N);
 const FNoise& N=*Pick[FMath::RandRange(0,Pick.Num()-1)];
 const FVector At=S47FarmWorld::RoadToArea(TEXT("saro"),FVector(N.RoadX,S47FarmWorld::FloorRoad(N.RoadX,N.RoadZ)+1.5,N.RoadZ));
 const float Rate=FMath::FRandRange(N.RateLow,N.RateHigh);
 // Near noises are placed where they are; beyond the 60 m the panner carries, the far voice is what reaches the ear.
 const FVector Eye(M->Player->Pos.X,M->Player->FloorY+1.6,M->Player->Pos.Z);
 if(FVector::Dist(Eye,At)<45){FS47Sound S;S.Gain=N.Gain;S.Rate=Rate;S.At=At;M->Audio->Sfx(N.Name,S);}
 else M->Audio->Far(N.Name,At,N.Gain,Rate);
 ++Sounds;
}
