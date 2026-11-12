#include "S47DeputyRound.h"
#include "S47NightRhythm.h"
#include "S47Audio.h"
#include "S47CarPaint.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47HighwayUfo.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "UObject/GarbageCollection.h"

namespace
{
 constexpr double Speed=15; // A little under the limit, looking.
 UStaticMesh* Mesh(const TCHAR* Path){return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path)))?LoadObject<UStaticMesh>(nullptr,Path):nullptr;}
}

struct FS47DeputyRound::FCar
{
 TWeakObjectPtr<AS47Web> Web;
 TWeakObjectPtr<USceneComponent> Root;
 TArray<TWeakObjectPtr<USceneComponent>> Parts;
 TArray<TWeakObjectPtr<USpotLightComponent>> Lamps;
 TSharedPtr<FS47PannerNode> Pan;
 FS47Play Engine;
 bool bReady=false;
 bool Prepare(US47Main* Main)
 {
  if(bReady&&Web.Get()==Main->Web.Get()&&Root.IsValid())return true;
  bReady=false;Web=Main->Web;Parts.Reset();Lamps.Reset();
  auto* Body=Mesh(TEXT("/Game/Licensed/TripoFleet/police_car/SM_TripoFleet_PoliceCar.SM_TripoFleet_PoliceCar"));
  auto* Door=Mesh(TEXT("/Game/Licensed/TripoFleet/police_car/SM_TripoFleet_PoliceCarDoor.SM_TripoFleet_PoliceCarDoor"));
  // SHERIFF-CAR: the county's tan and white Fairheaven; the Tripo STATE POLICE car only with -S47TripoPatrol.
  const bool bTripo=Body&&Door&&S47CarPaint::TripoPatrol();
  if(!bTripo)
  {
   Body=Mesh(TEXT("/Game/Free/Vehicles/fairheaven_lt80_cop/StaticMeshes/fairheaven_lt80_cop.fairheaven_lt80_cop"));
   Door=Mesh(TEXT("/Game/Free/Vehicles/fairheaven_lt80_cop/StaticMeshes/fairheaven_lt80_cop_door_driver.fairheaven_lt80_cop_door_driver"));
  }
  if(!Body||!Door||!IsValid(Main->Web.Get()))return false;
  auto* R=NewObject<USceneComponent>(Main->Web.Get());R->SetupAttachment(Main->Web->GetRootComponent());R->SetMobility(EComponentMobility::Movable);R->RegisterComponent();Root=R;Parts.Add(R);
  const auto Part=[&](UStaticMesh* Asset,const FVector& Where,double Heading)
  {
   auto* C=NewObject<UStaticMeshComponent>(Main->Web.Get());C->SetupAttachment(R);C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(Asset);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(true);
   double Mat[16];S47Three::Compose(Where,S47Three::Euler(0,Heading,0),FVector::OneVector,Mat);C->SetRelativeTransform(S47Three::MatToUE(Mat));C->RegisterComponent();Parts.Add(C);
  };
  Part(Body,FVector::ZeroVector,bTripo?0:UE_DOUBLE_HALF_PI);
  Part(Door,bTripo?FVector(-.99,.83,-1):FVector::ZeroVector,bTripo?0:UE_DOUBLE_HALF_PI);
  for(int32 I=0;I<2;++I)
  {
   auto* L=NewObject<USpotLightComponent>(Main->Web.Get());L->SetupAttachment(R);L->SetMobility(EComponentMobility::Movable);L->SetRelativeLocation(S47Three::ToUE(I==0?-.78:.78,.72,-2.15));L->SetRelativeRotation(FVector(0,-1,-.035).Rotation());
   L->SetLightColor(FLinearColor(1,.8,.52));L->SetInnerConeAngle(12);L->SetOuterConeAngle(28);L->SetAttenuationRadius(7000);L->SetIntensity(0);L->SetCastShadows(false);L->SetVolumetricScatteringIntensity(0);L->RegisterComponent();Parts.Add(L);Lamps.Add(L);
  }
  bReady=true;return true;
 }
 void Sound(US47Main* Main,const FVector& Where,bool bOn)
 {
  if(!bOn||!IsValid(Main->Audio.Get())||!Main->Audio->Ctx)
  {
   if(Engine.Src)Engine.Src->Stop();
   Engine={};if(Pan)Pan->Disconnect();
   Pan.Reset();return;
  }
  if(!Pan)
  {
   Pan=Main->Audio->Ctx->CreatePanner();Pan->RefDistance=14;Pan->Connect(Main->Audio->SfxBus);
   FS47Sound S;S.Gain=.3f;S.Rate=.9f;S.bLoop=true;S.DestNode=Pan;S.Lp=1600;Engine=Main->Audio->Sfx(TEXT("engineDrive"),S);
  }
  Pan->SetPosition(Where.X,Where.Y+1,Where.Z);
 }
 void Show(bool bOn)
 {
  if(auto* R=Root.Get())R->SetVisibility(bOn,true);
  for(auto& L:Lamps)if(auto* C=L.Get())C->SetIntensity(bOn?12000.f:0.f);
 }
};

FS47DeputyRound::FS47DeputyRound(US47Main* Main):Car(MakeUnique<FCar>()),M(Main){}
FS47DeputyRound::~FS47DeputyRound(){Car.Reset();}
bool FS47DeputyRound::Comes(int32 Seed,int32 Night,int32 Trust){return FS47NightRhythm::Roll(Seed,Night,0x44455055u)<(Trust>=2?.8f:.55f);}
double FS47DeputyRound::DueHour(int32 Seed,int32 Night){return 22.5+5.*FS47NightRhythm::Roll(Seed,Night,0x444f4e45u);}
void FS47DeputyRound::Begin(const S47J::FObj& Saved)
{
 Stop();bEnabled=true;LastNight=Saved.IsValid()?FMath::Max(0,S47J::Int(Saved,TEXT("night"))):0;
}
S47J::FObj FS47DeputyRound::Save() const
{
 auto O=S47J::New();if(LastNight>0)S47J::Set(O,TEXT("night"),LastNight);return O;
}
void FS47DeputyRound::End()
{
 bRun=false;
 if(!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&Car){Car->Show(false);Car->Sound(M,At,false);}
}
void FS47DeputyRound::Stop(){bEnabled=false;End();}
bool FS47DeputyRound::ForStory(FString& Why)
{
 if(!bEnabled||!M||!M->Freeform){Why=TEXT("the county patrol is not running");return false;}
 if(bRun){Why=TEXT("the county car is on the road already");return false;}
 bTestNow=true;LastNight=0;
 return true;
}
bool FS47DeputyRound::Start()
{
 if(!M||!M->Web||!M->World||!Car->Prepare(M))return false;
 const FString Area=M->Web->Area();const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 RunArea=Area;Sign=FMath::RandBool()?1:-1;Along=0;bSeen=false;
 if(Area==TEXT("saro")){FromZ=Sign>0?S47Legs::SARO_HWY.Z0:S47Legs::SARO_HWY.Z1;Length=S47Legs::SARO_HWY.Z1-S47Legs::SARO_HWY.Z0;LaneX=S47Legs::SARO_HWY.X-Sign*1.8;}
 else{FromZ=Road.Z+(Sign>0?320.:1860.);Length=1540;LaneX=Road.X-Sign*1.8;}
 bRun=true;Draw();Car->Show(true);
 UE_LOG(LogTemp,Display,TEXT("S47: deputy round: on the highway in %s, heading %s"),*Area,Sign>0?TEXT("south"):TEXT("north"));
 return true;
}
void FS47DeputyRound::Draw()
{
 const double Z=FromZ+Sign*Along;
 const FVector O=S47Legs::ORIGINS(TEXT("road"));
 const auto Floor=[&](double X,double AtZ){return RunArea==TEXT("saro")?US47Night::SaroFloorAt(M->Web.Get(),X,AtZ):O.Y+S47RoadTerrain::SurfaceY(X-O.X,AtZ-O.Z);};
 At=FVector(LaneX,Floor(LaneX,Z),Z);
 const double Front=Floor(LaneX,Z+Sign*1.45),Back=Floor(LaneX,Z-Sign*1.45);
 double Mat[16];S47Three::Compose(At,S47Three::Euler(FMath::Atan2(Front-Back,2.9),Sign>0?UE_DOUBLE_PI:0,0,TEXT("YXZ")),FVector::OneVector,Mat);
 if(auto* R=Car->Root.Get())R->SetWorldTransform(S47Three::MatToUE(Mat));
 Car->Sound(M,At,true);
}
void FS47DeputyRound::Step(double Dt)
{
 if(!bEnabled||!M||!M->Freeform||!M->Web||!M->Player||Dt<=0)return;
 auto& F=*M->Freeform;
 if(F.Held())return;
 const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.,Late=Hour<12?Hour+24:Hour;
 if(bRun)
 {
  // The player left that stretch: the car simply is not on this one. It does not follow.
  const FString Here=M->Web->Area();
  const bool bTown=(RunArea==TEXT("road")||RunArea==TEXT("diner"))&&(Here==TEXT("road")||Here==TEXT("diner"));
  if(Here!=RunArea&&!bTown){End();return;}
  Along+=Speed*Dt;
  if(Along>=Length){End();return;}
  Draw();
  const FVector Foot=M->Player->Pos;
  if(!bSeen&&FVector2D(Foot.X-At.X,Foot.Z-At.Z).Size()<150)
  {
   bSeen=true;
   const int32 S=int32(FMath::Fmod(F.Clock(),86400.));
   F.RecordEvidence(FString::Printf(TEXT("deputy-round-day%d"),F.Day()),FString::Printf(TEXT("The county car went by on the highway at %02d:%02d, slow, roof lights off. It did not stop."),S/3600,(S/60)%60));
  }
  return;
 }
 const int32 Night=F.Day();
 if(LastNight==Night)return;
 if(!bTestNow&&(!F.DeputyOnRoute()||!Comes(F.Seed(),Night,F.SheriffTrust())||Late<22||Late<DueHour(F.Seed(),Night)||Late>=30))return;
 const FString Here=M->Web->Area();
 if(Here!=TEXT("saro")&&Here!=TEXT("road")&&Here!=TEXT("diner"))return;
 if(M->HighwayUfo&&M->HighwayUfo->Busy())return;
 // Spent whether or not anybody sees it: a car that comes when the player looks would be for them.
 LastNight=Night;bTestNow=false;
 if(!Start())UE_LOG(LogTemp,Warning,TEXT("S47: deputy round: no car to send"));
 M->RequestAutosave();
}
bool FS47DeputyRound::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s deputy round: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 int32 On=0,More=0,Total=0;bool bHours=true;
 for(int32 Seed:{1,47,-9})for(int32 N=1;N<=200;++N){++Total;On+=Comes(Seed,N,1)?1:0;More+=Comes(Seed,N,2)?1:0;const double H=DueHour(Seed,N);bHours&=H>=22.5&&H<=27.5;}
 Check(On>Total/3&&On<Total*3/4&&More>On,TEXT("about every second night, more once he trusts the station"));
 Check(bHours,TEXT("between half past ten and half past three"));
 FS47DeputyRound R(nullptr);R.Begin(nullptr);Check(R.Save()->Values.Num()==0,TEXT("an old save has no such field, and nothing is written until it has come"));
 auto O=S47J::New();S47J::Set(O,TEXT("night"),7);FS47DeputyRound B(nullptr);B.Begin(S47J::Read(S47J::Write(O)));Check(S47J::Int(B.Save(),TEXT("night"))==7,TEXT("the night it came survives a Continue"));
 return OK;
}
