#include "S47FreeformCarry.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47World.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "S47Three.h"
#include "S47Audio.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "CollisionShape.h"
#include "Materials/MaterialInterface.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

void US47FreeformCarryContact::Hit(UPrimitiveComponent* Body,AActor* Other,UPrimitiveComponent* Part,FVector Impulse,const FHitResult& Contact)
{ if(Notify)Notify(Impulse,Contact); }
namespace
{
 bool NativeLive(const UObject* O){return IsValid(O)&&!O->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);}
 FVector Vec(const S47J::FObj& O,const TCHAR* K,const FVector& D=FVector::ZeroVector)
 {
  const auto A=S47J::Nums(O,K);return A.Num()==3&&FMath::IsFinite(A[0])&&FMath::IsFinite(A[1])&&FMath::IsFinite(A[2])?FVector(A[0],A[1],A[2]):D;
 }
 void SetVec(const S47J::FObj& O,const TCHAR* K,const FVector& V){S47J::Set(O,K,TArray<double>{V.X,V.Y,V.Z});}
}
FS47FreeformCarry::FS47FreeformCarry(US47Main* Main):M(Main){}
FS47FreeformCarry::~FS47FreeformCarry(){Life.Reset();Stop();}
bool FS47FreeformCarry::Live()const
{return !IsEngineExitRequested()&&!IsGarbageCollecting()&&NativeLive(M)&&NativeLive(M->Web.Get())&&!M->Web->IsActorBeingDestroyed();}
FString FS47FreeformCarry::CurrentScene()const
{return Live()&&M->World&&(M->World->Area==TEXT("road")||M->World->Area==TEXT("diner"))?TEXT("road"):TEXT("saro");}
UStaticMeshComponent* FS47FreeformCarry::Body()const
{
 if(!Live())return nullptr;const FS47Obj O=CurrentScene()==TEXT("road")?RoadBody:SaroBody;
 const auto* N=O.W==M->Web.Get()?M->Web->Node(O.I):nullptr;return N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
}
void FS47FreeformCarry::Configure(FS47Obj Saro,FS47Obj Road)
{
 SaroBody=Saro;RoadBody=Road;if(!Live()||bBodyBound)return;
 auto* R=NewObject<US47FreeformCarryContact>(M->Web);M->Web->AddInstanceComponent(R);R->RegisterComponent();Receiver=R;
 const TWeakPtr<uint8> Guard=Life;R->Notify=[this,Guard](const FVector& I,const FHitResult& H){if(Guard.IsValid())Contact(I,H);};
 for(const FS47Obj O:{SaroBody,RoadBody})if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))
 {C->OnComponentHit.AddDynamic(R,&US47FreeformCarryContact::Hit);C->SetNotifyRigidBodyCollision(true);C->SetLinearDamping(.65f);C->SetAngularDamping(1.5f);C->BodyInstance.bUseCCD=true;}
 auto* Floor=NewObject<UBoxComponent>(M->Web,TEXT("FreeformPackageContactFloor"));Floor->SetupAttachment(M->Web->GetRootComponent());Floor->SetMobility(EComponentMobility::Movable);
 Floor->SetBoxExtent(FVector(110,110,2));Floor->SetCollisionObjectType(ECC_WorldStatic);Floor->SetCollisionResponseToAllChannels(ECR_Ignore);Floor->SetCollisionResponseToChannel(ECC_PhysicsBody,ECR_Block);
 Floor->SetCollisionEnabled(ECollisionEnabled::NoCollision);Floor->SetHiddenInGame(true);Floor->RegisterComponent();Support=Floor;bBodyBound=true;
}
void FS47FreeformCarry::Restore(const S47J::FObj& O,const FVector& Legacy)
{
 Stop();LastState=-1;ItemId.Empty();bHavePose=false;bDynamic=bPaused=bBlocked=bSleep=false;Velocity=Angular=FVector::ZeroVector;ImpactCount=0;ActiveAge=0;LastImpact=LastSave=-10;
 if(!O||S47J::Int(O,TEXT("v"))!=1)
 {
  if(!Legacy.IsNearlyZero()){Pose=FTransform(FQuat::Identity,S47Three::ToUE(Legacy));Scene=FMath::Abs(Legacy.X)>1000?TEXT("road"):TEXT("saro");bHavePose=true;}
  return;
 }
 const FVector P=Vec(O,TEXT("position"));const auto Q=S47J::Nums(O,TEXT("rotation"));
 if(Q.Num()!=4||!FMath::IsFinite(Q[0])||!FMath::IsFinite(Q[1])||!FMath::IsFinite(Q[2])||!FMath::IsFinite(Q[3]))return;
 FQuat Rotation(Q[0],Q[1],Q[2],Q[3]);if(Rotation.SizeSquared()<.5||Rotation.SizeSquared()>1.5)return;Rotation.Normalize();
 Scene=S47J::Str(O,TEXT("scene"));if(Scene!=TEXT("saro")&&Scene!=TEXT("road"))return;
 Pose=FTransform(Rotation,S47Three::ToUE(P));bHavePose=true;bDynamic=S47J::Bool(O,TEXT("dynamic"));bSleep=S47J::Bool(O,TEXT("sleep"));
 Velocity=Vec(O,TEXT("velocity")).GetClampedToMaxSize(700);Angular=Vec(O,TEXT("angular")).GetClampedToMaxSize(400);
 SupportPoint=S47Three::ToUE(Vec(O,TEXT("support")));SupportNormal=Vec(O,TEXT("normal"),FVector::UpVector).GetSafeNormal();
 if(SupportNormal.Z<.65)bDynamic=false;LastSafe=Pose.GetLocation();ImpactCount=FMath::Clamp(S47J::Int(O,TEXT("impacts")),0,1000000);
}
S47J::FObj FS47FreeformCarry::Save()const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);FTransform T=Pose;FVector V=Velocity,A=Angular;bool Sleep=bSleep;
 if(LastState==4&&Scene==CurrentScene())if(auto* C=Body())if(C->IsSimulatingPhysics()){T=C->GetComponentTransform();V=C->GetPhysicsLinearVelocity();A=C->GetPhysicsAngularVelocityInDegrees();Sleep=!C->RigidBodyIsAwake();}
 SetVec(O,TEXT("position"),S47Three::ToThree(T.GetLocation()));const FQuat Q=T.GetRotation();S47J::Set(O,TEXT("rotation"),TArray<double>{Q.X,Q.Y,Q.Z,Q.W});
 S47J::Set(O,TEXT("scene"),Scene);S47J::Set(O,TEXT("dynamic"),bDynamic);S47J::Set(O,TEXT("sleep"),Sleep);
 SetVec(O,TEXT("velocity"),V);SetVec(O,TEXT("angular"),A);SetVec(O,TEXT("support"),S47Three::ToThree(SupportPoint));SetVec(O,TEXT("normal"),SupportNormal);S47J::Set(O,TEXT("impacts"),ImpactCount);return O;
}
FCollisionQueryParams FS47FreeformCarry::Query()const
{
 FCollisionQueryParams Q(SCENE_QUERY_STAT(FreeformPackage),true);
 if(Live())for(const FS47Obj O:{SaroBody,RoadBody})if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))Q.AddIgnoredComponent(C);
 if(Support.IsValid())Q.AddIgnoredComponent(Support.Get());return Q;
}
FVector FS47FreeformCarry::Extent()const
{return FVector(23,19,16);}
bool FS47FreeformCarry::Ground(const FVector& P,FHitResult& Hit)const
{
 return Live()&&M->Web->GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,12),P-FVector(0,0,500),ECC_Visibility,Query())&&Hit.ImpactNormal.Z>=.65;
}
FVector FS47FreeformCarry::Clearance(const FVector& From,const FVector& To,const FQuat& Q,bool& Hit)const
{
 Hit=false;if(!Live())return From;FHitResult H;
 if(M->Web->GetWorld()->SweepSingleByChannel(H,From,To,Q,ECC_Visibility,FCollisionShape::MakeBox(Extent()*.95),Query()))
 {Hit=true;return H.bStartPenetrating?From:H.Location+H.ImpactNormal*1.2;}
 return To;
}
void FS47FreeformCarry::Place(const FVector& P,const FQuat& Q)
{
 if(auto* C=Body()){C->SetWorldLocationAndRotation(P,Q,false,nullptr,ETeleportType::TeleportPhysics);Pose=FTransform(Q,P);bHavePose=true;LastSafe=P;}
}
void FS47FreeformCarry::Freeze(bool Keep)
{
 if(!Live())return;
 for(const FS47Obj O:{SaroBody,RoadBody})if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))if(C->IsSimulatingPhysics())
 {
  if(Keep){Pose=C->GetComponentTransform();Velocity=C->GetPhysicsLinearVelocity();Angular=C->GetPhysicsAngularVelocityInDegrees();bSleep=!C->RigidBodyIsAwake();}
  C->SetSimulatePhysics(false);
 }
 if(Support.IsValid())Support->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void FS47FreeformCarry::Resume()
{
 if(!Live()||!bDynamic||!Support.IsValid())return;auto* C=Body();if(!C)return;
 FHitResult NativeGround;
 // Revalidate saved contact geometry against the currently loaded native scene.
 // An unloaded or changed floor never creates a floating physics support.
 if(!Ground(Pose.GetLocation(),NativeGround)){bDynamic=false;return;}
 SupportPoint=NativeGround.ImpactPoint;SupportNormal=NativeGround.ImpactNormal;
 Support->SetWorldLocationAndRotation(SupportPoint-SupportNormal*2,FQuat::FindBetweenNormals(FVector::UpVector,SupportNormal));Support->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
 C->SetCollisionObjectType(ECC_PhysicsBody);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);C->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);C->SetCollisionResponseToChannel(ECC_PhysicsBody,ECR_Block);
 C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 C->SetMassOverrideInKg(NAME_None,ItemId==TEXT("donor")?8:ItemId==TEXT("fuel")?6:ItemId==TEXT("service_case")?5:3.5,true);C->SetSimulatePhysics(true);C->SetPhysicsLinearVelocity(Velocity);C->SetPhysicsAngularVelocityInDegrees(Angular);if(bSleep)C->PutRigidBodyToSleep();
}
void FS47FreeformCarry::SetShown(bool Show)
{
 if(!Live())return;for(FS47Obj O:{SaroBody,RoadBody})if(O)
 {
  const bool Here=(O==RoadBody)==(CurrentScene()==TEXT("road"));O.SetVisible(Show&&Here);
  if(const auto* N=M->Web->Node(O.I))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get()))if(!C->IsSimulatingPhysics())C->SetCollisionEnabled(Show&&Here&&LastState!=2?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
}
void FS47FreeformCarry::ReleaseSpeed()
{
 if(bOwnSpeed&&Live()&&NativeLive(M->Player.Get())&&FMath::IsNearlyEqual(M->Player->Speed,double(AppliedSpeed),.001))M->Player->Speed=BaseSpeed;bOwnSpeed=false;
}
void FS47FreeformCarry::Stop()
{
 ReleaseSpeed();if(Live()){Freeze(false);SetShown(false);}LastState=-1;
}
bool FS47FreeformCarry::Release(bool Drop)
{
 if(!Live()||LastState!=2||!M->Freeform||M->Freeform->Held())return false;auto* C=Body();if(!C)return false;FHitResult H;
 const FVector Start=C->GetComponentLocation();if(!Ground(Start,H))return false;
 SupportPoint=H.ImpactPoint;SupportNormal=H.ImpactNormal;
 // The support is a measured local contact patch, never an invented floor.
 // Query-only native scenery is also enforced by the sweep guard each update.
 Scene=CurrentScene();bDynamic=true;bPaused=false;bSleep=false;Velocity=Angular=FVector::ZeroVector;
 if(!Drop)
 {
  const FVector Target=SupportPoint+SupportNormal*(Extent().Z+2.5);bool Block=false;
  const FVector At=Clearance(Start,Target,C->GetComponentQuat(),Block);if(Block&&FVector::Dist(At,Target)>6)return false;
  Place(At,C->GetComponentQuat());
 }
 else
 {
  Velocity=M->Player->ForwardUE()*35.;Velocity.Z=0;Angular=FVector(8,17,5);
 }
 Pose=C->GetComponentTransform();LastSafe=Pose.GetLocation();bHavePose=true;ReleaseSpeed();Resume();return true;
}
void FS47FreeformCarry::Contact(const FVector& Impulse,const FHitResult& Hit)
{
 if(!Live()||LastState!=4||bPaused||!M->Audio||!M->Freeform||M->Freeform->Held()||ActiveAge-LastImpact<.28)return;
 const double Speed=Impulse.Size()/FMath::Max(1.f,Body()?Body()->GetMass():3.5f);if(Speed<35)return;LastImpact=ActiveAge;++ImpactCount;
 FString Surface;const auto* Part=Hit.GetComponent();if(Part)if(auto* Mat=Part->GetMaterial(0))Surface=Mat->GetName().ToLower();
 FS47Sound S;S.At=S47Three::ToThree(Hit.ImpactPoint);S.Gain=float(FMath::Clamp(Speed/750.,.10,.50));S.Rate=ItemId==TEXT("donor")?.75f:ItemId==TEXT("fuel")?.85f:1.08f;S.Lp=ItemId==TEXT("donor")?4200.f:ItemId==TEXT("fuel")?3000.f:1800.f;
 // Existing dry recordings; no new sound or licensed data is generated here.
 M->Audio->Sfx((ItemId==TEXT("fuel")||ItemId==TEXT("donor")||ItemId==TEXT("service_case")||Surface.Contains(TEXT("metal")))?M->Audio->Or(TEXT("metalTap"),TEXT("thudSoft")):TEXT("thudSoft"),S);
}
void FS47FreeformCarry::Step(double Dt,int32 State,const FString& Id,const FVector& Anchor)
{
 if(!Live()||!bBodyBound)return;
 if(ItemId!=Id)
 {
  const uint32 Colour=Id==TEXT("service_case")?0x59635d:0x9f8250;
  for(FS47Obj O:{SaroBody,RoadBody})if(O)O.SetColor(S47Three::Hex(Colour));
 }
 ItemId=Id;
 if(State!=LastState)
 {
  if(State!=4)Freeze(LastState==4&&State==2);if(State!=2)ReleaseSpeed();
  if(State==1||State==3){Scene=CurrentScene();bHavePose=false;bDynamic=false;}
  if(State==2)
  {
   Scene=CurrentScene();if(!bHavePose){if(auto* C=Body()){Pose=C->GetComponentTransform();bHavePose=true;}}
   if(auto* C=Body())C->SetSimulatePhysics(false);
   BaseSpeed=M->Player?float(M->Player->Speed):1.9f;AppliedSpeed=FMath::Min(BaseSpeed,Id==TEXT("donor")?1.35f:1.5f);bOwnSpeed=true;
  }
  LastState=State;
 }
 if(State<=0){bHavePose=false;bDynamic=false;SetShown(false);return;}
 if(State==1||State==3)
 {
  Scene=State==1?TEXT("road"):(FMath::Abs(Anchor.X)>1000?TEXT("road"):TEXT("saro"));
  if(CurrentScene()!=Scene){SetShown(false);return;}Place(S47Three::ToUE(Anchor),FQuat::Identity);SetShown(true);return;
 }
 if(State==2)
 {
  if(Scene!=CurrentScene()&&M->Player)
  {
   Scene=CurrentScene();Place(M->Player->EyeUE()-FVector(0,0,60),FQuat::Identity);
  }
  Scene=CurrentScene();if(M->Player&&bOwnSpeed)M->Player->Speed=AppliedSpeed;
  if(M->Freeform&&M->Freeform->Held()){SetShown(true);return;}
  if(!M->Player)return;ActiveAge+=FMath::Clamp(Dt,0.,.1);
  const FVector Chest=M->Player->EyeUE()-FVector(0,0,60),Forward=M->Player->ForwardUE();
  FVector Desired=Chest+FVector(Forward.X,Forward.Y,0).GetSafeNormal()*72.;
  Desired.Z+=FMath::Sin(ActiveAge*5.)*1.2;
  const FQuat Q=FMath::RInterpTo(Pose.GetRotation().Rotator(),FRotator(0,M->Player->ForwardUE().Rotation().Yaw,0),float(Dt),5.).Quaternion();
  bool FromChest=false;Desired=Clearance(Chest,Desired,Q,FromChest);
  bool FromBody=false;const FVector Smooth=FMath::VInterpTo(Pose.GetLocation(),Desired,float(Dt),Id==TEXT("donor")?6.f:8.f);
  const FVector P=Clearance(Pose.GetLocation(),Smooth,Q,FromBody);bBlocked=FromChest||FromBody;Place(P,Q);SetShown(true);return;
 }
 if(State==4)
 {
  const bool Here=CurrentScene()==Scene;
  if(!Here){if(!bPaused){Freeze(true);bPaused=true;}SetShown(false);return;}
  if(!bHavePose){Pose=FTransform(FQuat::Identity,S47Three::ToUE(Anchor));bHavePose=true;}
  const bool Pause=M->Freeform&&M->Freeform->Held();
  if(Pause){if(!bPaused){Freeze(true);bPaused=true;}SetShown(true);return;}
  if(bPaused||LastState==4){if(auto* C=Body())if(!C->IsSimulatingPhysics()){Place(Pose.GetLocation(),Pose.GetRotation());Resume();}bPaused=false;}
  ActiveAge+=FMath::Clamp(Dt,0.,.1);auto* C=Body();if(!C)return;
  if(C->IsSimulatingPhysics())
  {
   bool Hit=false;const FVector Current=C->GetComponentLocation();const FVector Safe=Clearance(LastSafe,Current,C->GetComponentQuat(),Hit);
   if(Hit&&FVector::DistSquared(Safe,Current)>1)
   {const FVector V=C->GetPhysicsLinearVelocity();C->SetWorldLocation(Safe,false,nullptr,ETeleportType::TeleportPhysics);C->SetPhysicsLinearVelocity(-V*.12);}
   LastSafe=C->GetComponentLocation();Pose=C->GetComponentTransform();Velocity=C->GetPhysicsLinearVelocity();Angular=C->GetPhysicsAngularVelocityInDegrees();bSleep=!C->RigidBodyIsAwake();
   // Keep the one physical contact patch bounded; stop on actual native ground
   // rather than simulating a lost transaction far outside its support.
   if(FVector::Dist2D(LastSafe,SupportPoint)>85 || Pose.GetLocation().Z<SupportPoint.Z-5)
   {FHitResult G;if(Ground(LastSafe,G))
    {
     Freeze(true);bDynamic=false;Velocity=Angular=FVector::ZeroVector;
     const FQuat Q=Pose.GetRotation();const FVector E=Extent();
     const double Height=FMath::Abs(FVector::DotProduct(Q.GetAxisX(),G.ImpactNormal))*E.X+FMath::Abs(FVector::DotProduct(Q.GetAxisY(),G.ImpactNormal))*E.Y+FMath::Abs(FVector::DotProduct(Q.GetAxisZ(),G.ImpactNormal))*E.Z;
     Place(G.ImpactPoint+G.ImpactNormal*(Height+1),Q);
    }}
   if(ActiveAge-LastSave>2){LastSave=ActiveAge;M->RequestAutosave();}
  }
  SetShown(true);
 }
}
FVector FS47FreeformCarry::Position()const{return S47Three::ToThree(Pose.GetLocation());}
bool FS47FreeformCarry::Simulating()const {auto* C=Body();return C&&C->IsSimulatingPhysics();}
bool FS47FreeformCarry::SelfTest(TArray<FString>& Lines)
{
 FS47FreeformCarry C(nullptr);auto S=S47J::New();S47J::Set(S,TEXT("v"),1);SetVec(S,TEXT("position"),FVector(12,.6,-3));S47J::Set(S,TEXT("rotation"),TArray<double>{0,0,0,1});S47J::Set(S,TEXT("scene"),TEXT("saro"));S47J::Set(S,TEXT("dynamic"),true);SetVec(S,TEXT("velocity"),FVector(20,0,-40));
 C.Restore(S,FVector::ZeroVector);auto Saved=S47J::Read(S47J::Write(C.Save()));bool OK=C.Position().Equals(FVector(12,.6,-3),.0001)&&S47J::Bool(Saved,TEXT("dynamic"))&&Vec(Saved,TEXT("velocity")).Equals(FVector(20,0,-40));Lines.Add(FString(OK?TEXT("PASS "):TEXT("FAIL "))+TEXT("Freeform dropped package pose, scene and velocity survive JSON"));return OK;
}
