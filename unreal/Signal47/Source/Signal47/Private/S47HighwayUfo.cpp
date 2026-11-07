#include "S47HighwayUfo.h"
#include "S47FarmSite.h"

#include "S47Audio.h"
#include "S47Flow.h"
#include "S47Glimpses.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Props.h"
#include "S47RoadTerrain.h"
#include "S47Story.h"
#include "S47Three.h"
#include "S47UfoPresentation.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47HighwayUfo,Log,All);

namespace
{
 constexpr double EndTime=44,QuietTime=45,CarHalfW=1.1,CarHalfL=2.65;
 const FString ColliderId=TEXT("highway-ufo-patrol");
 double Smooth(double A,double B,double X){const double T=FMath::Clamp((X-A)/(B-A),0.,1.);return T*T*(3-2*T);}
 FVector RoadOrigin(){return S47Legs::ORIGINS(TEXT("road"));}
 FVector Ground(double X,double Z){const FVector O=RoadOrigin();return FVector(X,O.Y+S47RoadTerrain::SurfaceY(X-O.X,Z-O.Z),Z);}
 TArray<double> Vector(const FVector& P){return {P.X,P.Y,P.Z};}
 FVector ReadVector(const S47J::FObj& O,const TCHAR* Key,const FVector& Fallback=FVector::ZeroVector)
 {
  const auto A=S47J::Nums(O,Key);if(A.Num()!=3)return Fallback;
  for(double V:A)if(!FMath::IsFinite(V)||FMath::Abs(V)>100000)return Fallback;
  return FVector(A[0],A[1],A[2]);
 }
 double Number(const S47J::FObj& O,const TCHAR* Key,double Lo,double Hi,double Default=0)
 {const double V=S47J::Num(O,Key,Default);return FMath::IsFinite(V)?FMath::Clamp(V,Lo,Hi):Default;}
 bool FootprintHits(const FVector& At,const FS47Obstacle& O)
 {
  if(O.bCircle)
  {const double X=FMath::Max(0.,FMath::Abs(O.X-At.X)-CarHalfW),Z=FMath::Max(0.,FMath::Abs(O.Z-At.Z)-CarHalfL);return X*X+Z*Z<O.R*O.R;}
  return At.X+CarHalfW>O.MinX&&At.X-CarHalfW<O.MaxX&&At.Z+CarHalfL>O.MinZ&&At.Z-CarHalfL<O.MaxZ;
 }
 bool Live(const UObject* O){return IsValid(O)&&!O->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);}
 UStaticMesh* Asset(const TCHAR* Path)
 {return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path)))?LoadObject<UStaticMesh>(nullptr,Path):nullptr;}
}

struct FS47HighwayUfo::FVisual
{
 TWeakObjectPtr<US47Main> Main;
 TWeakObjectPtr<US47World> CollisionWorld;
 TWeakObjectPtr<AS47Web> Web;
 TUniquePtr<FS47UfoPresentation> Ufo;
 TWeakObjectPtr<USceneComponent> Root;
 TWeakObjectPtr<UStaticMeshComponent> Body;
 TWeakObjectPtr<UBoxComponent> HitBox;
 TArray<TWeakObjectPtr<USceneComponent>> Components;
 TArray<TWeakObjectPtr<USpotLightComponent>> Headlights;
 TArray<TWeakObjectPtr<UPointLightComponent>> Beacons;
 TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> BeaconMids;
 IS47DriveArea* CollisionArea=nullptr;
 int32 CollisionSlot=INDEX_NONE;
 bool bReady=false,bTripo=false,bCarShown=false;
 TSharedPtr<FS47GainNode> UfoGain,SirenGain;
 TSharedPtr<FS47PannerNode> UfoPan,CarPan;
 TArray<TSharedPtr<FS47OscillatorNode>> Tones;
 FS47Play Engine;

 explicit FVisual(US47Main* M):Main(M){}
 bool CanTouch()const{return Main.IsValid()&&Web.IsValid()&&Live(Web.Get())&&Web->GetWorld()&&!Web->GetWorld()->bIsTearingDown;}
 ~FVisual()
 {
  SoundOff();ReleaseCollision();Ufo.Reset();
  if(CanTouch())for(auto& C:Components)if(Live(C.Get()))C->DestroyComponent();
 }
 bool Prepare(bool bCar)
 {
  if(!Main.IsValid()||!Live(Main->Web.Get())||!Main->Web->IsLoaded())return false;
  Web=Main->Web;if(!Ufo)Ufo=MakeUnique<FS47UfoPresentation>(Web.Get());
  if(!Ufo->Prepare())return false;if(!bCar)return true;if(bReady)return true;
  auto* Car=Asset(TEXT("/Game/Licensed/TripoFleet/police_car/SM_TripoFleet_PoliceCar.SM_TripoFleet_PoliceCar"));
  auto* Door=Asset(TEXT("/Game/Licensed/TripoFleet/police_car/SM_TripoFleet_PoliceCarDoor.SM_TripoFleet_PoliceCarDoor"));
  bTripo=Car&&Door;
  if(!bTripo)
  {
   Car=Asset(TEXT("/Game/Free/Vehicles/fairheaven_lt80_cop/StaticMeshes/fairheaven_lt80_cop.fairheaven_lt80_cop"));
   Door=Asset(TEXT("/Game/Free/Vehicles/fairheaven_lt80_cop/StaticMeshes/fairheaven_lt80_cop_door_driver.fairheaven_lt80_cop_door_driver"));
  }
  if(!Car||!Door)return false;
  auto* R=NewObject<USceneComponent>(Web.Get());R->SetupAttachment(Web->GetRootComponent());R->SetMobility(EComponentMobility::Movable);R->RegisterComponent();Root=R;Components.Add(R);
  auto Part=[&](UStaticMesh* Mesh,const FVector& At,double Heading)->UStaticMeshComponent*
  {
   auto* C=NewObject<UStaticMeshComponent>(Web.Get());C->SetupAttachment(R);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(true);
   double Mat[16];S47Three::Compose(At,S47Three::Euler(0,Heading,0),FVector::OneVector,Mat);C->SetRelativeTransform(S47Three::MatToUE(Mat));C->RegisterComponent();Components.Add(C);return C;
  };
  Body=Part(Car,FVector::ZeroVector,bTripo?0:UE_DOUBLE_HALF_PI);
  Part(Door,bTripo?FVector(-.99,.83,-1):FVector::ZeroVector,bTripo?0:UE_DOUBLE_HALF_PI);
  auto* Box=NewObject<UBoxComponent>(Web.Get());Box->SetupAttachment(R);Box->SetMobility(EComponentMobility::Movable);Box->SetBoxExtent(FVector(CarHalfW*100,CarHalfL*100,85));Box->SetRelativeLocation(S47Three::ToUE(0,.85,0));
  Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);Box->SetCollisionResponseToAllChannels(ECR_Ignore);Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);Box->SetHiddenInGame(true);Box->RegisterComponent();HitBox=Box;Components.Add(Box);
  auto* Sphere=Asset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
  for(int32 I=0;I<2;++I)
  {
   const FVector Lamp(I==0?-.46:.46,1.74,.21);
   if(Sphere&&Base)
   {
    auto* Lens=Part(Sphere,Lamp,0);Lens->SetRelativeScale3D(FVector(.35,.24,.12));Lens->SetCastShadow(false);
    auto* Mid=UMaterialInstanceDynamic::Create(Base,Web.Get());Mid->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(0xff3020));Mid->SetScalarParameterValue(TEXT("EmissivePower"),4);Lens->SetMaterial(0,Mid);BeaconMids.Add(Mid);
    auto* Tail=Part(Sphere,FVector(I==0?-.78:.78,.62,2.25),0);Tail->SetRelativeScale3D(FVector(.18,.04,.14));Tail->SetCastShadow(false);
    auto* TailMid=UMaterialInstanceDynamic::Create(Base,Web.Get());TailMid->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(0xc8170b));TailMid->SetScalarParameterValue(TEXT("EmissivePower"),3);Tail->SetMaterial(0,TailMid);
   }
   auto* B=NewObject<UPointLightComponent>(Web.Get());B->SetupAttachment(R);B->SetMobility(EComponentMobility::Movable);B->SetRelativeLocation(S47Three::ToUE(Lamp));B->SetLightColor(FLinearColor(1,.025,.01));B->SetAttenuationRadius(1300);B->SetIntensity(0);B->SetCastShadows(false);B->SetVolumetricScatteringIntensity(0);B->RegisterComponent();Components.Add(B);Beacons.Add(B);
   auto* H=NewObject<USpotLightComponent>(Web.Get());H->SetupAttachment(R);H->SetMobility(EComponentMobility::Movable);H->SetRelativeLocation(S47Three::ToUE(I==0?-.78:.78,.72,-2.15));H->SetRelativeRotation(FVector(0,-1,-.035).Rotation());H->SetLightColor(FLinearColor(1,.8,.52));H->SetInnerConeAngle(12);H->SetOuterConeAngle(28);H->SetAttenuationRadius(7000);H->SetIntensity(0);H->SetCastShadows(false);H->SetVolumetricScatteringIntensity(0);H->RegisterComponent();Components.Add(H);Headlights.Add(H);
  }
  bReady=true;ShowCar(false,FVector::ZeroVector,1,0,false);return true;
 }
 void ReleaseCollision()
 {
  if(CollisionWorld.IsValid()&&Live(CollisionWorld.Get())&&CollisionArea&&CollisionArea->Obstacles.IsValidIndex(CollisionSlot))
   CollisionArea->Obstacles[CollisionSlot]=FS47Obstacle::Box(1e6,1e6+1,1e6,1e6+1);
  if(Web.IsValid()&&Live(Web.Get()))for(auto& C:Web->CollidersMutable(TEXT("road")))if(C.Id==ColliderId){C.MinX=C.MinZ=1e6;C.MaxX=C.MaxZ=1e6+1;}
  if(HitBox.IsValid())HitBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }
 void BindCollision()
 {
  if(!Main.IsValid()||!Main->World)return;
  if(auto* D=Main->World->CabDrive())if(D->Area&&D->Area!=CollisionArea)
  {ReleaseCollision();CollisionWorld=Main->World;CollisionArea=D->Area;CollisionSlot=CollisionArea->Obstacles.Add(FS47Obstacle::Box(1e6,1e6+1,1e6,1e6+1));}
 }
 bool Clear(const FVector& P)const
 {
  if(CollisionWorld.IsValid()&&CollisionArea)
  {
   if(CollisionArea->Blocked(P.X,P.Z))return false;
   for(int32 I=0;I<CollisionArea->Obstacles.Num();++I)if(I!=CollisionSlot&&FootprintHits(P,CollisionArea->Obstacles[I]))return false;
  }
  if(Main.IsValid()&&Main->Player)
  {
   const FVector Player=Main->World&&Main->World->bDriving&&Main->World->CabDrive()?Main->World->CabDrive()->Pos:Main->Player->Pos;
   const double Half=Main->World&&Main->World->bDriving?3.5:.65;
   if(FMath::Abs(Player.X-P.X)<CarHalfW+1.1&&FMath::Abs(Player.Z-P.Z)<CarHalfL+Half)return false;
  }
  return true;
 }
 void ShowCar(bool On,const FVector& At,double Direction,double T,bool Calm)
 {
  if(!bReady||!CanTouch()||!Root.IsValid())return;
  bCarShown=On;Root->SetVisibility(On,true);
  if(!On){ReleaseCollision();for(auto& L:Headlights)if(L.IsValid())L->SetIntensity(0);for(auto& L:Beacons)if(L.IsValid())L->SetIntensity(0);return;}
  const double Front=Ground(At.X,At.Z+Direction*1.45).Y,Back=Ground(At.X,At.Z-Direction*1.45).Y;
  const double H=Direction>0?UE_DOUBLE_PI:0;
  double Mat[16];S47Three::Compose(At,S47Three::Euler(FMath::Atan2(Front-Back,2.9),H,0,TEXT("YXZ")),FVector::OneVector,Mat);Root->SetWorldTransform(S47Three::MatToUE(Mat));
  const double Fade=Smooth(0,3,T);float Lens[2],Turn[2];AS47Props::BeaconAt(T,Lens,Turn);
  for(int32 I=0;I<2;++I)
  {const float K=Calm?.65f:.25f+.75f*Lens[I];if(Beacons.IsValidIndex(I)&&Beacons[I].IsValid())Beacons[I]->SetIntensity(900*Fade*K);if(BeaconMids.IsValidIndex(I)&&BeaconMids[I].IsValid())BeaconMids[I]->SetScalarParameterValue(TEXT("EmissivePower"),float((3+4*K)*Fade));if(Headlights.IsValidIndex(I)&&Headlights[I].IsValid())Headlights[I]->SetIntensity(12000*Fade);}
  BindCollision();
  if(CollisionWorld.IsValid()&&CollisionArea&&CollisionArea->Obstacles.IsValidIndex(CollisionSlot))CollisionArea->Obstacles[CollisionSlot]=FS47Obstacle::Box(At.X-CarHalfW,At.X+CarHalfW,At.Z-CarHalfL,At.Z+CarHalfL);
  auto& Rows=Web->CollidersMutable(TEXT("road"));auto* C=Rows.FindByPredicate([](const FS47Rect& R){return R.Id==ColliderId;});
  if(!C){FS47Rect R;R.Id=ColliderId;Rows.Add(R);C=&Rows.Last();}C->MinX=At.X-CarHalfW;C->MaxX=At.X+CarHalfW;C->MinZ=At.Z-CarHalfL;C->MaxZ=At.Z+CarHalfL;
  if(HitBox.IsValid())HitBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 }
 void SoundOff()
 {
  for(auto& O:Tones)if(O)O->Stop();Tones.Reset();if(Engine.Src)Engine.Src->Stop();Engine={};
  if(UfoGain)UfoGain->Disconnect();if(SirenGain)SirenGain->Disconnect();if(UfoPan)UfoPan->Disconnect();if(CarPan)CarPan->Disconnect();
  UfoGain.Reset();SirenGain.Reset();UfoPan.Reset();CarPan.Reset();
 }
 void Sound(bool On,bool Follow,const FVector& Craft,const FVector& Patrol,double T,double Speed)
 {
  if(!Main.IsValid()||!Live(Main->Audio.Get())||!Main->Audio->Ctx)return;auto A=Main->Audio.Get();
  if(!On){SoundOff();return;}
  FS47AudioContext::FBatch Batch(*A->Ctx);
  if(!UfoGain)
  {
   UfoPan=A->Ctx->CreatePanner();UfoPan->RefDistance=18;UfoPan->Connect(A->SfxBus);
   UfoGain=A->Ctx->CreateGain();UfoGain->Gain.SetValue(0);UfoGain->Connect(UfoPan);
   for(double Hz:{46.,69.,138.}){auto O=A->Ctx->CreateOscillator();O->Frequency.SetValue(Hz);O->Connect(UfoGain);O->Start();Tones.Add(O);}
   if(!Follow)
   {
    CarPan=A->Ctx->CreatePanner();CarPan->RefDistance=12;CarPan->Connect(A->SfxBus);SirenGain=A->Ctx->CreateGain();SirenGain->Gain.SetValue(.025);SirenGain->Connect(CarPan);
    auto Siren=A->Ctx->CreateOscillator();Siren->Frequency.SetValue(510);Siren->Connect(SirenGain);Siren->Start();Tones.Add(Siren);
    FS47Sound S;S.Gain=.3f;S.Rate=.9f;S.bLoop=true;S.DestNode=CarPan;S.Lp=1600;Engine=A->Sfx(TEXT("engineDrive"),S);
   }
  }
  UfoPan->SetPosition(Craft.X,Craft.Y,Craft.Z);UfoGain->Gain.SetValue(.035*Smooth(0,4,T)*(1-Smooth(35,44,T)));
  if(CarPan)CarPan->SetPosition(Patrol.X,Patrol.Y+1,Patrol.Z);
  if(Tones.Num()==4)Tones[3]->Frequency.SetValue(510+160*FMath::Sin(T*1.8));
  if(SirenGain)SirenGain->Gain.SetValue(.025*(1-Smooth(26,38,T)));
  if(Engine.Gain)Engine.Gain->Gain.SetValue(.2*FMath::Clamp(Speed/14.,.15,1.));
 }
 void Hide(){if(Ufo)Ufo->Hide();ShowCar(false,FVector::ZeroVector,1,0,false);SoundOff();}
};

FS47HighwayUfo::FS47HighwayUfo(US47Main* Main):Visual(MakeUnique<FVisual>(Main)),M(Main){}
FS47HighwayUfo::~FS47HighwayUfo()=default;
void FS47HighwayUfo::Begin(const S47J::FObj& O)
{
 Stop();bEnabled=true;bActive=bFollow=bSeen=bSawCar=bSawPass=bLogged=bEscaped=bCarRetained=bResumePending=false;
 LastNight=-1000;Serial=0;Age=Rest=StartZ=CarSpeed=HiddenFor=SoundAt=0;Sign=1;CarAt=UfoAt=PassStart=PassTarget=FVector::ZeroVector;Outcome.Empty();UfoSightBlocker.Empty();CarSightBlocker.Empty();FollowSightBlocker.Empty();PhotoSightBlocker.Empty();
 if(!O||S47J::Int(O,TEXT("v"))!=1)return;
 LastNight=int32(Number(O,TEXT("night"),-1000,1000000,-1000));Serial=int32(Number(O,TEXT("serial"),0,1000000));
 Age=Number(O,TEXT("age"),0,EndTime);Rest=Number(O,TEXT("rest"),0,QuietTime);Sign=Number(O,TEXT("direction"),-1,1,1)<0?-1:1;
 StartZ=Number(O,TEXT("startZ"),0,2400);CarSpeed=Number(O,TEXT("carSpeed"),0,18);HiddenFor=Number(O,TEXT("hiddenFor"),0,3);
 bFollow=S47J::Bool(O,TEXT("follow"));bSeen=S47J::Bool(O,TEXT("seen"));bSawCar=S47J::Bool(O,TEXT("sawCar"));bSawPass=S47J::Bool(O,TEXT("sawPass"));bLogged=S47J::Bool(O,TEXT("logged"));bEscaped=S47J::Bool(O,TEXT("escaped"));
 CarAt=ReadVector(O,TEXT("car"));UfoAt=ReadVector(O,TEXT("ufo"));PassStart=ReadVector(O,TEXT("passStart"));PassTarget=ReadVector(O,TEXT("passTarget"));Outcome=S47J::Str(O,TEXT("outcome"));
 bActive=S47J::Bool(O,TEXT("active"))&&Age<EndTime&&Serial>0;
 bCarRetained=S47J::Bool(O,TEXT("carRetained"))&&!bFollow&&Serial>0;
 const auto ValidRoad=[](const FVector& P){const FVector O=RoadOrigin();return FMath::Abs(P.X-O.X)<50&&P.Z-O.Z>=-120&&P.Z-O.Z<=2700&&FMath::Abs(P.Y)<100;};
 const auto ValidSky=[](const FVector& P){const FVector O=RoadOrigin();return FMath::Abs(P.X-O.X)<1000&&P.Z-O.Z>=-1000&&P.Z-O.Z<=4000&&P.Y>-100&&P.Y<500;};
 if(((bActive||bCarRetained)&&(!ValidRoad(CarAt)||StartZ<200||StartZ>2200))||(bActive&&(!ValidSky(UfoAt)||(Age>=24&&(!ValidSky(PassStart)||!ValidSky(PassTarget))))))
 {bActive=bCarRetained=false;Outcome=TEXT("invalid-resume");Rest=FMath::Max(Rest,QuietTime);}
 bResumePending=bActive||bCarRetained;
}
void FS47HighwayUfo::Stop(){bEnabled=false;bActive=bCarRetained=false;Rest=0;if(Visual)Visual->Hide();}
bool FS47HighwayUfo::Busy()const{return bEnabled&&(bActive||Rest>0);}
bool FS47HighwayUfo::Held()const
 {return !M||M->Mode!=US47Main::EMode::Play||M->bPausedByMenu||M->ModalOpen()||(M->IsFreeform()&&M->Freeform&&M->Freeform->Held());}
bool FS47HighwayUfo::RoadVisible()const{return M&&M->Web&&M->World&&(M->Web->Area()==TEXT("road")||M->Web->Area()==TEXT("diner"))&&(M->World->Area==TEXT("road")||M->World->Area==TEXT("diner"));}
double FS47HighwayUfo::Clock()const{return M&&M->IsFreeform()&&M->Freeform?M->Freeform->Clock():M&&M->Story?M->Story->Clock():0;}
int32 FS47HighwayUfo::NightId()const{return M&&M->IsFreeform()&&M->Freeform?M->Freeform->Day():1;}
FVector FS47HighwayUfo::PlayerAt()const
 {return M&&M->World&&M->World->bDriving&&M->World->CabDrive()?M->World->CabDrive()->Pos:M&&M->Player?M->Player->Pos:FVector::ZeroVector;}
bool FS47HighwayUfo::CanStart(bool Forced)const
{
 if(!bEnabled||Busy()||Held()||!RoadVisible()||!M->World->bDriving||!M->World->CabDrive())return false;
 const auto D=M->World->CabDrive();const FVector P=D->Pos-RoadOrigin();
 if(!D->bEngine||D->Held>0||FMath::Abs(P.X)>4.2||D->Speed<1)return false;
 if(M->Story&&M->Story->IsCinematic())return false;
 if(M->IsFreeform())
 {if(!M->Freeform||!M->Freeform->Active()||(M->Freeform->Director&&M->Freeform->Director->Busy()))return false;}
 else if(M->OneAtATime())return false;
 // The finale and authored car encounter never borrow this highway event.
 if(!M->IsFreeform()&&M->Story&&M->Story->ChapterKey==TEXT("ch6"))return false;
 if(Forced)return P.Z>300&&P.Z<1900;
 double Hour=FMath::Fmod(Clock(),86400.)/3600.;if(Hour<0)Hour+=24;
 if(Hour>=6&&Hour<19)return false;
 // A full hull beside the truck is not an evening's small hint: in Freeform it waits as a body does.
 if(M->IsFreeform()&&!FS47FarmSite::TheirHours(Clock()))return false;
 if(M->IsFreeform()?NightId()-LastNight<3:LastNight>=0)return false;
 return -FMath::Cos(D->Heading)>.85&&P.Z>=650&&P.Z<=900;
}
bool FS47HighwayUfo::Start(bool Forced,bool Follow)
{
 if(!CanStart(Forced)||!Visual->Prepare(!Follow))return false;
 Visual->ReleaseCollision();Visual->BindCollision();
 const auto D=M->World->CabDrive();const FVector O=RoadOrigin();Sign=-FMath::Cos(D->Heading)>=0?1:-1;
 StartZ=FMath::Clamp(D->Pos.Z-O.Z+Sign*180.,450.,1850.);
 CarAt=Ground(O.X-Sign*1.8,O.Z+StartZ);
 if(!Follow&&!Visual->Clear(CarAt)){Visual->ReleaseCollision();return false;}
 bActive=true;bFollow=Follow;bSeen=bSawCar=bSawPass=bLogged=bEscaped=false;bCarRetained=!Follow;bResumePending=false;
 Age=Rest=HiddenFor=SoundAt=0;CarSpeed=14;LastNight=NightId();Serial=FMath::Min(1000000,Serial+1);Outcome.Empty();
 UfoAt=Follow?Ground(D->Pos.X-Sign*2,D->Pos.Z-Sign*55)+FVector(0,12,0):Ground(CarAt.X,CarAt.Z+Sign*85)+FVector(0,15,0);PassStart=PassTarget=FVector::ZeroVector;
 Draw();Changed();UE_LOG(LogS47HighwayUfo,Display,TEXT("S47: highway UFO %s started at road %.1fm, incident %d"),Follow?TEXT("follow"):TEXT("patrol"),StartZ,Serial);return true;
}
bool FS47HighwayUfo::Force(bool Follow){return Start(true,Follow);}
FString FS47HighwayUfo::PhotoIncidentId()const
 {return bActive&&RoadVisible()&&Visual&&Visual->Ufo&&Visual->Ufo->Presented()?FString::Printf(TEXT("highway-ufo.%s.%d.%d"),bFollow?TEXT("follow"):TEXT("patrol"),LastNight,Serial):FString();}
TArray<UStaticMeshComponent*> FS47HighwayUfo::PhotoParts()const
 {return !PhotoIncidentId().IsEmpty()?Visual->Ufo->PhotoParts():TArray<UStaticMeshComponent*>();}
bool FS47HighwayUfo::CancelForTest(){if(!bActive)return false;Finish(TEXT("cancelled"));bCarRetained=false;Visual->Hide();return true;}
void FS47HighwayUfo::Changed()const{if(M)M->RequestAutosave();}
bool FS47HighwayUfo::ClearSight(const FVector& Eye,const FVector& At,const UPrimitiveComponent* Target,FString* BlockedBy)const
{
 if(BlockedBy)BlockedBy->Empty();
 if(!M||!M->Web||!M->Web->GetWorld()){if(BlockedBy)*BlockedBy=TEXT("world-unavailable");return false;}
 auto* Web=M->Web.Get();FCollisionQueryParams Q(SCENE_QUERY_STAT(HighwayUfoSight),true);if(M->Player)Q.AddIgnoredActor(M->Player);
 // Match the established Glimpses/SC03 sight policy. Keep tracing after an
 // interaction proxy or clear pane; the wall, sill or cab pillar behind it still blocks.
 for(int32 I=0;I<24;++I)
 {
  FHitResult Hit;
  if(!Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(Eye),S47Three::ToUE(At),ECC_Visibility,Q))return true;
  const auto* C=Hit.GetComponent();if(!C){if(BlockedBy)*BlockedBy=TEXT("unknown-component");return false;}
  if(Target&&C==Target)return true;
  bool Solid=C->IsVisible()&&!C->bHiddenInGame;
  const auto* Node=Web->Node(Web->NodeOf(C));
  if(Node&&Node->Name.StartsWith(TEXT("proxy")))Solid=false;
  if(const auto* Mat=C->GetMaterial(FMath::Max<int32>(0,Hit.ElementIndex)))
  {
   float Opacity=1;Mat->GetScalarParameterValue(TEXT("Opacity"),Opacity);
   if(Mat->GetBlendMode()==BLEND_Additive||(Mat->GetBlendMode()==BLEND_Translucent&&Opacity<.5f))Solid=false;
  }
  if(Solid){if(BlockedBy)*BlockedBy=Node&&!Node->Name.IsEmpty()?Node->Name:C->GetName();return false;}
  Q.AddIgnoredComponent(C);
 }
 if(BlockedBy)*BlockedBy=TEXT("trace-limit");return false;
}
bool FS47HighwayUfo::PhotoLineClear(const FVector& FromUE,const FVector& AtUE,const UPrimitiveComponent* Subject)const
 {return ClearSight(S47Three::ToThree(FromUE),S47Three::ToThree(AtUE),Subject,&PhotoSightBlocker);}
bool FS47HighwayUfo::Visible(const FVector& At,const UPrimitiveComponent* Target)const
{
 FString& Blocker=Visual&&Target==Visual->HitBox.Get()?CarSightBlocker:UfoSightBlocker;Blocker.Empty();
 if(!M||!M->Night||!M->Web||!RoadVisible()){Blocker=TEXT("outside-road");return false;}
 const auto Cam=M->Night->GlimpseIn(true).Cam;const FVector To=At-Cam.Pos;
 if(To.Size()>600){Blocker=TEXT("outside-range");return false;}
 if(!To.IsNearlyZero()&&FVector::DotProduct(To.GetSafeNormal(),Cam.Forward)<.70){Blocker=TEXT("outside-view");return false;}
 return ClearSight(Cam.Pos,At,Target,&Blocker);
}
bool FS47HighwayUfo::ClearToPlayer()const
 {return ClearSight(UfoAt,PlayerAt()+FVector(0,1.2,0),nullptr,&FollowSightBlocker);}
FString FS47HighwayUfo::StageName()const
 {if(!bActive)return Outcome.IsEmpty()?TEXT("idle"):TEXT("done");if(Age<8)return TEXT("approach");if(Age<24)return TEXT("pursuit");if(Age<32)return TEXT("pass");return TEXT("depart");}
void FS47HighwayUfo::Advance(double Dt)
{
 const double Before=Age;Age=FMath::Min(EndTime,Age+Dt);const FVector O=RoadOrigin();
 if(!bFollow)
 {
  const double Desired=14*(1-Smooth(28,40,Age));double Speed=FMath::Min(Desired,CarSpeed+2.5*Dt);
  // Braking checks the swept stopping corridor, not just the destination.
  const double Brake=Speed*Speed/8+5;bool Clear=true;
  for(double S=1;S<=Brake;S+=1)if(!Visual->Clear(CarAt+FVector(0,0,Sign*S))){Clear=false;break;}
  if(!Clear)Speed=FMath::Max(0.,CarSpeed-5*Dt);
  FVector Next=Ground(FMath::Lerp(O.X-Sign*1.8,O.X-Sign*7.7,Smooth(31,40,Age)),CarAt.Z+Sign*Speed*Dt);
  if(Next.Z-O.Z<320||Next.Z-O.Z>1650){Speed=0;Next=CarAt;}
  if(!Visual->Clear(Next)){Speed=0;Next=CarAt;}
  CarAt=Next;CarSpeed=Speed;
  if(Age<24)UfoAt=Ground(O.X-Sign*1.3,CarAt.Z+Sign*FMath::Lerp(85.,48.,Smooth(8,24,Age)))+FVector(0,FMath::Lerp(15.,9.,Smooth(0,24,Age)),0);
 }
 else if(Age<24)
 {
  const FVector Target=PlayerAt()+FVector(-Sign*2,12,-Sign*35);const FVector To=Target-UfoAt;
  UfoAt+=To.GetClampedToMaxSize(22*Dt);
  const bool Outrun=FVector::Dist(UfoAt,Target)>45;
  const bool Covered=FMath::Abs(PlayerAt().X-O.X)>8&&!ClearToPlayer();
  HiddenFor=(Outrun||Covered)?HiddenFor+Dt:0;
  if(Age>8&&HiddenFor>=2){bEscaped=true;Age=24;}
 }
 if(Before<24&&Age>=24)
 {
  PassStart=UfoAt;FVector Target=PlayerAt();if(FVector::Dist2D(S47Three::ToUE(Target),S47Three::ToUE(UfoAt))>23000)Target=CarAt;
  PassTarget=Ground(Target.X+Sign*5,Target.Z-Sign*65)+FVector(0,bEscaped?45:10,0);Changed();
 }
 if(Age>=24&&Age<32)UfoAt=FMath::Lerp(PassStart,PassTarget,Smooth(24,32,Age));
 if(Age>=32)UfoAt=PassTarget+FVector(Sign*270,190,-Sign*520)*Smooth(32,44,Age);
 if(Age>=EndTime)Finish(bEscaped?TEXT("escaped"):TEXT("departed"));
}
void FS47HighwayUfo::Draw()
{
 if(!Visual||!RoadVisible()){if(Visual)Visual->Hide();return;}
 const bool Calm=M&&M->Flow&&M->Flow->bCalmFlash;
 Visual->ShowCar(bCarRetained&&!bFollow,CarAt,Sign,Age,Calm);
 if(!bActive){if(Visual->Ufo)Visual->Ufo->Hide();Visual->SoundOff();return;}
 if(!Visual->Prepare(!bFollow))return;
 FS47UfoPresentation::FFrame F;F.Position=UfoAt;F.GroundTarget=Ground(UfoAt.X,UfoAt.Z);F.Forward=Age<24?FVector(0,0,Sign):FVector(0,0,-Sign);F.Seconds=Age;
 F.Width=8;F.Opacity=Smooth(0,3,Age)*(1-Smooth(40,44,Age));F.Beam=Age<24?.40:.65*(1-Smooth(32,40,Age));F.NearWhite=Smooth(22,27,Age)*(1-Smooth(31,36,Age));F.bCalm=Calm;
 Visual->Ufo->Show(F);Visual->Sound(true,bFollow,UfoAt,CarAt,Age,CarSpeed);
}
void FS47HighwayUfo::Note()
{
 if(bLogged||!bSeen||!M)return;bLogged=true;
 FString Text=bFollow?TEXT("A low light followed the road near my vehicle."):TEXT("A low red light moved along the highway.");
 if(bSawCar&&!bFollow)Text+=TEXT(" A patrol car followed it.");
 if(bSawPass)Text+=TEXT(" The light reversed and passed close to my vehicle, lighting the road.");
 if(bEscaped)Text+=TEXT(" I broke contact. The vehicle remained usable.");
 Text+=TEXT(" Its origin is unconfirmed.");
 if(M->IsFreeform()&&M->Freeform)M->Freeform->RecordEvidence(FString::Printf(TEXT("highway-light-%d"),Serial),TEXT("FIELD OBSERVATION / HIGHWAY\n")+Text);
 else if(M->Story)M->Story->Note(Text);
}
void FS47HighwayUfo::Finish(const FString& Why)
{
 if(!bActive)return;bActive=false;Outcome=Why;Rest=QuietTime;Note();if(Visual->Ufo)Visual->Ufo->Hide();Visual->SoundOff();Changed();
 UE_LOG(LogS47HighwayUfo,Display,TEXT("S47: highway UFO %s; seen=%d car=%d pass=%d incident=%d"),*Why,bSeen,bSawCar,bSawPass,Serial);
}
void FS47HighwayUfo::Step(double Dt)
{
 if(!bEnabled||!M||!FMath::IsFinite(Dt)||Dt<=0)return;
 if(Held()){if(Visual)Visual->SoundOff();return;}
 Dt=FMath::Min(Dt,.25);
 if(bResumePending)
 {
  bResumePending=false;
  if(!RoadVisible()){if(bActive)Finish(TEXT("resume-away"));bCarRetained=false;Visual->Hide();Changed();}
  else if(!Visual->Prepare(!bFollow)){if(bActive)Finish(TEXT("art-unavailable"));bCarRetained=false;Visual->Hide();Changed();}
 }
 if(Rest>0)Rest=FMath::Max(0.,Rest-Dt);
 if(!bActive)
 {
  if(bCarRetained&&(!RoadVisible()||FVector::Dist(PlayerAt(),CarAt)>700)){bCarRetained=false;Visual->Hide();Changed();}
  else if(bCarRetained)Draw();
  if(CanStart(false))Start(false,M->IsFreeform()&&(Serial%2)==1);return;
 }
 if(!RoadVisible()){Finish(TEXT("left-road"));bCarRetained=false;Visual->Hide();return;}
 Advance(Dt);Draw();
 if(bActive&&Visual->Ufo&&Visual->Ufo->Presented())
 {
  const bool SeenNow=Visible(UfoAt,Visual->Ufo->SightTarget());bSeen|=SeenNow;bSawPass|=SeenNow&&Age>=24&&Age<32&&FVector::Dist(PlayerAt(),UfoAt)<=45;
  bSawCar|=!bFollow&&Visual->bCarShown&&Visible(CarAt+FVector(0,1,0),Visual->HitBox.Get());
 }
}
S47J::FObj FS47HighwayUfo::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("active"),bActive);S47J::Set(O,TEXT("follow"),bFollow);S47J::Set(O,TEXT("seen"),bSeen);S47J::Set(O,TEXT("sawCar"),bSawCar);S47J::Set(O,TEXT("sawPass"),bSawPass);S47J::Set(O,TEXT("logged"),bLogged);S47J::Set(O,TEXT("escaped"),bEscaped);S47J::Set(O,TEXT("carRetained"),bCarRetained);
 S47J::Set(O,TEXT("night"),LastNight);S47J::Set(O,TEXT("serial"),Serial);S47J::Set(O,TEXT("age"),Age);S47J::Set(O,TEXT("rest"),Rest);S47J::Set(O,TEXT("startZ"),StartZ);S47J::Set(O,TEXT("direction"),Sign);S47J::Set(O,TEXT("carSpeed"),CarSpeed);S47J::Set(O,TEXT("hiddenFor"),HiddenFor);S47J::Set(O,TEXT("outcome"),Outcome);
 S47J::Set(O,TEXT("car"),Vector(CarAt));S47J::Set(O,TEXT("ufo"),Vector(UfoAt));S47J::Set(O,TEXT("passStart"),Vector(PassStart));S47J::Set(O,TEXT("passTarget"),Vector(PassTarget));return O;
}
S47J::FObj FS47HighwayUfo::Readback()const
{
 auto O=Save();S47J::Set(O,TEXT("phase"),StageName());S47J::Set(O,TEXT("busy"),Busy());S47J::Set(O,TEXT("roadVisible"),RoadVisible());S47J::Set(O,TEXT("carShown"),Visual&&Visual->bCarShown);S47J::Set(O,TEXT("tripoCar"),Visual&&Visual->bTripo);S47J::Set(O,TEXT("playerEngine"),M&&M->World&&M->World->CabDrive()?M->World->CabDrive()->bEngine:false);
 S47J::Set(O,TEXT("ufoSightBlocker"),UfoSightBlocker);S47J::Set(O,TEXT("patrolSightBlocker"),CarSightBlocker);S47J::Set(O,TEXT("followSightBlocker"),FollowSightBlocker);S47J::Set(O,TEXT("photoSightBlocker"),PhotoSightBlocker);
 if(Visual&&Visual->Ufo)S47J::Set(O,TEXT("presentation"),Visual->Ufo->Readback());return O;
}
bool FS47HighwayUfo::SelfTest(TArray<FString>& Lines)
{
 bool Ok=true;auto Check=[&](bool V,const TCHAR* Text){Lines.Add(FString::Printf(TEXT("%s highway UFO: %s"),V?TEXT("PASS"):TEXT("FAIL"),Text));Ok&=V;};
 FS47HighwayUfo H(nullptr);H.Begin(nullptr);Check(!H.Busy()&&H.StageName()==TEXT("idle"),TEXT("old cases begin without an invented encounter"));
 auto S=S47J::New();S47J::Set(S,TEXT("v"),1);S47J::Set(S,TEXT("active"),true);S47J::Set(S,TEXT("serial"),3);S47J::Set(S,TEXT("night"),7);S47J::Set(S,TEXT("age"),25.5);S47J::Set(S,TEXT("startZ"),850);S47J::Set(S,TEXT("car"),Vector(Ground(7998.2,1100)));S47J::Set(S,TEXT("ufo"),TArray<double>{8005,14,1000});S47J::Set(S,TEXT("passStart"),TArray<double>{7998,14,1150});S47J::Set(S,TEXT("passTarget"),TArray<double>{8005,11,950});S47J::Set(S,TEXT("seen"),true);S47J::Set(S,TEXT("logged"),true);S47J::Set(S,TEXT("carRetained"),true);
 H.Begin(S);const auto Round=H.Save();Check(H.Busy()&&H.StageName()==TEXT("pass")&&H.EventAge()==25.5&&S47J::Bool(Round,TEXT("logged"))&&S47J::Int(Round,TEXT("serial"))==3,TEXT("pass reload preserves time, identity and written observation"));
 S47J::Set(S,TEXT("car"),TArray<double>{0,0,0});H.Begin(S);Check(!S47J::Bool(H.Save(),TEXT("active"))&&!S47J::Bool(H.Save(),TEXT("carRetained")),TEXT("malformed positions cannot materialize a car in another area"));
 H.Begin(nullptr);H.bActive=true;H.Age=24;H.PassStart=FVector(8000,12,1200);H.PassTarget=FVector(8005,10,1000);H.CarAt=Ground(7998.2,1100);H.StartZ=850;H.bFollow=true;H.Advance(4);Check(H.UfoAt.Z<1200&&H.UfoAt.Z>1000&&H.Age==28,TEXT("near pass travels a fixed world route without moving the player"));
 Check(FootprintHits(FVector(0,0,0),FS47Obstacle::Box(1,2,-1,1))&&!FootprintHits(FVector(0,0,0),FS47Obstacle::Box(2,3,-1,1))&&FootprintHits(FVector(0,0,0),FS47Obstacle::Circle(0,3,.5)),TEXT("vehicle clearance handles wall and circular road obstacles"));
 H.Stop();Check(!H.Busy(),TEXT("leave-play releases both active encounter and quiet reservation"));return Ok;
}
