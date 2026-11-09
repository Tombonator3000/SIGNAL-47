#include "S47FarmShedDoor.h"
#include "S47FarmBuildings.h"
#include "S47FarmWorld.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Misc/PackageName.h"
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47ShedDoor,Log,All);

// WEEKLY-CASE-20261109, patch 2: the feed shed's door. See S47FarmShedDoor.h. Built as FS47FarmHouseDoors are (a leaf the walker's
// line of sight picks, a collider of its own in the SARO walk), but placed every frame by the area as S47FarmWorld places the
// buildings, so that it stands with its shed seen from the road and the diner too. Blender's axes become UE's as the buildings'
// do (townbuildings/README.md: Blender (x,y,z) is UE (x,-y,z)); the leaf opens outward, to the south, with a positive yaw.
namespace S47ShedDoorLocal
{
 constexpr int32 ShedSite=3;
 const TCHAR* const Model=TEXT("/Game/FarmBuildings/SM_FarmShedDoor.SM_FarmShedDoor");
 const TCHAR* const ModelPackage=TEXT("/Game/FarmBuildings/SM_FarmShedDoor");
 const TCHAR* const Id=TEXT("ff:sheddoor");
 const TCHAR* const Rect=TEXT("door:shed:feed");
 const TCHAR* const Walk=TEXT("saro");
 // The staple on the frame the chain hangs from (shed metres), and how high.
 const FVector2D StapleLocal(-.46,-2.05);
 constexpr double StapleUp=1.12;
 constexpr int32 LinkCount=6;
 // Where the pieces come to rest, from the hinge in the shed's metres (x along the wall, y out in front), their yaw and size.
 struct FRest{double X,Y,Yaw,Len,Wide;};
 const FRest Rests[]={{.15,-1.1,12,1.45,.16},{.62,-1.95,71,1.05,.14},{1.08,-1.38,-38,.88,.15},{.36,-2.7,104,1.25,.14},{1.32,-2.32,23,.62,.12},{.8,-.7,-64,.96,.12}};
 bool LiveEnough(US47Main* M){return !IsEngineExitRequested()&&!GExitPurge&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);}
 bool AreaShows(const FString& Area){return Area==TEXT("saro")||Area==TEXT("road")||Area==TEXT("diner");}
 UStaticMeshComponent* Part(AS47Web* W,USceneComponent* Parent,const TCHAR* Name,UStaticMesh* Mesh,UMaterialInterface* Mat,bool bPick)
 {
  auto* C=NewObject<UStaticMeshComponent>(W,Name);
  C->SetMobility(EComponentMobility::Movable);
  if(Parent)C->SetupAttachment(Parent);else C->SetupAttachment(W->GetRootComponent());
  C->SetStaticMesh(Mesh);if(Mat)C->SetMaterial(0,Mat);
  C->SetReceivesDecals(false);C->SetCastShadow(true);
  if(bPick){C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);}
  else C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->RegisterComponent();
  return C;
 }
}

const TCHAR* FS47FarmShedDoor::StateName(EState S){return S==EState::Locked?TEXT("locked"):S==EState::Broken?TEXT("broken"):TEXT("open");}
const TCHAR* FS47FarmShedDoor::MoodName(EMood S){return S==EMood::Rattle?TEXT("rattle"):S==EMood::Kicks?TEXT("kicks"):TEXT("still");}

// ---------------------------------------------------------------- the pure rules
TArray<double> FS47FarmShedDoor::Pattern(int32 InRhythm)
{
 switch(FMath::Abs(InRhythm)%4)
 {
 case 0:return {0.,.34,.68};          // three, even
 case 1:return {0.,.26,.86};          // two quick, then one
 case 2:return {0.,.52,.78,1.04};     // one, then three quick
 default:return {0.,.3,.6,1.22,1.5};  // three, a gap, two
 }
}
TArray<double> FS47FarmShedDoor::AnswerFor(const TArray<double>& InRaps)
{
 TArray<double> Out=InRaps;Out.Sort();
 if(Out.IsEmpty())return Out;
 const double First=Out[0];
 for(double& R:Out)R-=First;
 return Out;
}
double FS47FarmShedDoor::AnswerWaitFor(int32 InSeed,int32 InNight){return 6.5+3.5*double(FS47NightRhythm::Roll(InSeed,InNight,0x5348414eu));}
double FS47FarmShedDoor::KickGap(int32 InSeed,int32 InNight,int32 Index)
{
 // Uneven: mostly short, now and then a long wait that is worse than the kicks.
 const double U=FS47NightRhythm::Roll(InSeed,InNight*97+Index,0x4b49434bu);
 return U<.72?1.5+2.2*U/.72:3.9+2.2*(U-.72)/.28;
}
FBox2D FS47FarmShedDoor::ClosedBox()
{
 const FBox2D Way=S47FarmBuildings::ShedDoorway();
 const FVector A=S47FarmWorld::RoadToArea(TEXT("saro"),FVector(Way.Min.X,0,Way.Min.Y)),B=S47FarmWorld::RoadToArea(TEXT("saro"),FVector(Way.Max.X,0,Way.Max.Y));
 const double Pad=.06;
 return FBox2D(FVector2D(FMath::Min(A.X,B.X),FMath::Min(A.Z,B.Z)-Pad),FVector2D(FMath::Max(A.X,B.X),FMath::Max(A.Z,B.Z)+Pad));
}

// ---------------------------------------------------------------- where
FVector FS47FarmShedDoor::RoadOf(double LocalX,double LocalY) const
{
 const TArray<S47FarmBuildings::FSite>& Sites=S47FarmBuildings::Sites();
 if(!Sites.IsValidIndex(S47ShedDoorLocal::ShedSite))return FVector::ZeroVector;
 const S47FarmBuildings::FSite& S=Sites[S47ShedDoorLocal::ShedSite];
 const FVector2D R=S47FarmBuildings::LocalToRoad(S,LocalX,LocalY);
 return FVector(R.X,S47FarmBuildings::Anchor(S).Y,R.Y);
}
FVector FS47FarmShedDoor::AreaOf(const FVector& Road) const
{
 const FString Area=M&&M->Web?M->Web->Area():FString(S47ShedDoorLocal::Walk);
 return S47FarmWorld::RoadToArea(Area,Road);
}
FVector FS47FarmShedDoor::Middle() const
{
 const FVector2D H=HingeLocal();
 return AreaOf(RoadOf(H.X+LeafWidth*.5,H.Y))+FVector(0,1,0);
}
FVector FS47FarmShedDoor::Inside() const{return AreaOf(RoadOf(-.9,-1.1))+FVector(0,.9,0);}

// ---------------------------------------------------------------- the parts
FS47FarmShedDoor::~FS47FarmShedDoor(){Stop();}

bool FS47FarmShedDoor::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web)||!S47FarmBuildings::Available())return false;
 AS47Web* W=M->Web.Get();
 auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));if(!Cube)return false;
 auto* Wood=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TownBuildings/Materials/M_Town_DarkWood.M_Town_DarkWood"));
 auto* Metal=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/TownBuildings/Materials/M_Town_Metal.M_Town_Metal"));
 UStaticMesh* Leaf=FPackageName::DoesPackageExist(S47ShedDoorLocal::ModelPackage)?LoadObject<UStaticMesh>(nullptr,S47ShedDoorLocal::Model):nullptr;
 Life=MakeShared<uint8>(0);
 // Stopped and begun again in the same world: the parts are still there, hidden; only the interaction is offered again.
 if(!Hinge.IsValid()||LeafParts.IsEmpty())
 {
 bModel=Leaf!=nullptr;
 auto* H=NewObject<USceneComponent>(W,TEXT("S47ShedDoorHinge"));H->SetMobility(EComponentMobility::Movable);H->SetupAttachment(W->GetRootComponent());H->RegisterComponent();Hinge=H;
 LeafParts.Reset();Links.Reset();Pieces.Reset();
 // Relative to the hinge, in cm: x along the shut leaf, +y out of the shed (Blender's -y), z up from the shed's z = 0.
 const auto Box=[&](const TCHAR* Name,const FVector& At,const FVector& Size,UMaterialInterface* Mat,bool bPick)
 {
  UStaticMeshComponent* C=S47ShedDoorLocal::Part(W,H,Name,Cube,Mat,bPick);
  C->SetRelativeLocation(At);C->SetRelativeScale3D(Size/100.);LeafParts.Add(C);return C;
 };
 UStaticMeshComponent* Pick=nullptr;
 if(bModel){Pick=S47ShedDoorLocal::Part(W,H,TEXT("S47ShedDoorLeaf"),Leaf,nullptr,true);LeafParts.Add(Pick);}
 else
 {
  // The plain leaf (sheds.py door_leaf in boxes): until SM_FarmShedDoor is imported, drawn shut over the baked door.
  Pick=Box(TEXT("S47ShedDoorLeaf"),FVector(48,0,100),FVector(96,4,190),Wood,true);
  for(const double Z:{20.,100.,180.})Box(*FString::Printf(TEXT("S47ShedDoorRail%.0f"),Z),FVector(48,3,Z),FVector(96,2.2,12),Wood,false);
  for(const double Z:{40.,155.})Box(*FString::Printf(TEXT("S47ShedDoorHingeStrap%.0f"),Z),FVector(16,4.4,Z),FVector(32,.8,3.5),Metal,false);
  Box(TEXT("S47ShedDoorHasp"),FVector(89,4.6,112),FVector(9,1,5),Metal,false);
 }
 // The lock: a staple on the frame, the chain through the leaf's hasp, the padlock on it. Not on the leaf: the frame holds it.
 auto* L=NewObject<USceneComponent>(W,TEXT("S47ShedDoorLock"));L->SetMobility(EComponentMobility::Movable);L->SetupAttachment(W->GetRootComponent());L->RegisterComponent();Lock=L;
 Staple=S47ShedDoorLocal::Part(W,L,TEXT("S47ShedDoorStaple"),Cube,Metal,false);Staple->SetRelativeScale3D(FVector(.012,.03,.06));
 for(int32 I=0;I<S47ShedDoorLocal::LinkCount;++I){UStaticMeshComponent* C=S47ShedDoorLocal::Part(W,L,*FString::Printf(TEXT("S47ShedDoorLink%d"),I),Cube,Metal,false);C->SetRelativeScale3D(FVector(.032,.009,.02));Links.Add(C);}
 Padlock=S47ShedDoorLocal::Part(W,L,TEXT("S47ShedDoorPadlock"),Cube,Metal,false);Padlock->SetRelativeScale3D(FVector(.05,.022,.045));
 Shackle=S47ShedDoorLocal::Part(W,L,TEXT("S47ShedDoorShackle"),Cube,Metal,false);Shackle->SetRelativeScale3D(FVector(.034,.008,.03));
 // The pieces it leaves in the yard: boards with the leaf's paint on one face, a rail.
 for(int32 I=0;I<UE_ARRAY_COUNT(S47ShedDoorLocal::Rests);++I)
 {
  const S47ShedDoorLocal::FRest& R=S47ShedDoorLocal::Rests[I];
  FPiece P;P.C=S47ShedDoorLocal::Part(W,nullptr,*FString::Printf(TEXT("S47ShedDoorPiece%d"),I),Cube,Wood,false);
  P.C->SetVisibility(false);P.Yaw=R.Yaw;P.Size=FVector(R.Len,R.Wide,.028);
  const FVector2D Hl=HingeLocal();const FVector Road=RoadOf(Hl.X+R.X,Hl.Y+R.Y);
  P.Rest=FVector(Road.X,S47FarmWorld::FloorRoad(Road.X,Road.Z)+.014,Road.Z);
  Pieces.Add(P);
 }
 Object=W->ObjAt(W->AdoptNode(Pick,INDEX_NONE,S47ShedDoorLocal::Id));
 }
 if(!bColliderAdded){FS47Rect R;R.MinX=R.MaxX=R.MinZ=R.MaxZ=1e9;R.Id=S47ShedDoorLocal::Rect;W->CollidersMutable(S47ShedDoorLocal::Walk).Add(R);bColliderAdded=true;}
 if(IsValid(M->Inter))
 {
  const TWeakPtr<uint8> Guard=Life;
  FS47Interactable It;It.Id=S47ShedDoorLocal::Id;It.Object=Object;It.Range=2.4;
  It.Label=[this,Guard]()->FString
  {
   if(!Guard.IsValid()||Now!=EState::Locked||MoodNow==EMood::Kicks)return FString();
   return bTried?FString(TEXT("Knock")):FString(TEXT("Try the door"));
  };
  It.Use=[this,Guard]{if(Guard.IsValid())Use();};
  It.Examine=[this,Guard]()->FString
  {
   if(!Guard.IsValid())return FString();
   if(Now==EState::Locked)return TEXT("A chain through the hasp and a new padlock, locked on this side. Fresh scratches by the hinges.");
   return FString();
  };
  M->Inter->Add(It);W->MarkUsable(Object.I,S47ShedDoorLocal::Id);
  bInterAdded=true;
 }
 bBuilt=true;
 UE_LOG(LogS47ShedDoor,Display,TEXT("S47: shed door: built (%s), %s"),bModel?TEXT("SM_FarmShedDoor"):TEXT("plain leaf of boxes over the baked door"),StateName(Now));
 SetCollider();
 return true;
}

void FS47FarmShedDoor::SetCollider()
{
 S47FarmBuildings::SetShedDoorShut(bBuilt&&ShutFor(Now));
 if(!S47ShedDoorLocal::LiveEnough(M)||!Scene.IsValid())return;
 const FBox2D B=ClosedBox();
 for(FS47Rect& R:Scene->CollidersMutable(S47ShedDoorLocal::Walk))if(R.Id==S47ShedDoorLocal::Rect)
 {
  if(bBuilt&&ShutFor(Now)){R.MinX=B.Min.X;R.MaxX=B.Max.X;R.MinZ=B.Min.Y;R.MaxZ=B.Max.Y;}
  else R.MinX=R.MaxX=R.MinZ=R.MaxZ=1e9;
 }
}

bool FS47FarmShedDoor::Drawn() const{const USceneComponent* H=Hinge.Get();return bBuilt&&H&&LeafParts.Num()>0&&LeafParts[0].IsValid()&&LeafParts[0]->IsVisible();}

void FS47FarmShedDoor::Pose()
{
 USceneComponent* H=Hinge.Get();USceneComponent* L=Lock.Get();
 if(!H||!L||!M->Web)return;
 const bool bShow=S47ShedDoorLocal::AreaShows(M->Web->Area())&&S47FarmWorld::BuildingsShown(M);
 const FVector2D Hl=HingeLocal();
 const double BaseYaw=FMath::RadiansToDegrees(-S47FarmBuildings::Sites()[S47ShedDoorLocal::ShedSite].Yaw);
 // Shut, a kick pushes the latch side out against the chain and the boards flex: a little turn, a little out, shaking.
 double Yaw=0;FVector Out=FVector::ZeroVector;
 if(Now==EState::Open)Yaw=OpenDegrees*Swing;
 else if(Now==EState::Locked){Yaw=FMath::RadiansToDegrees(FMath::Atan2(Bulge,LeafWidth))+Jiggle*.35*FMath::Sin(T*47.3);Out=FVector(0,0,Bulge*.25);}
 const FVector At=AreaOf(RoadOf(Hl.X,Hl.Y))+Out;
 H->SetWorldLocationAndRotation(S47Three::ToUE(At),FRotator(0,BaseYaw+Yaw,0));
 const bool bLeaf=bShow&&Now!=EState::Broken;
 for(const auto& Weak:LeafParts)if(UStaticMeshComponent* C=Weak.Get())
 {
  C->SetVisibility(bLeaf);C->SetHiddenInGame(!bLeaf);
  if(C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||Weak==LeafParts[0])C->SetCollisionEnabled(bLeaf?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
 // The chain: a sagging loop from the staple to the hasp, shut; hanging straight down from the staple, broken.
 const bool bLock=bShow&&Now!=EState::Open;
 L->SetWorldLocationAndRotation(S47Three::ToUE(AreaOf(RoadOf(S47ShedDoorLocal::StapleLocal.X,S47ShedDoorLocal::StapleLocal.Y))+FVector(0,S47ShedDoorLocal::StapleUp,0)),FRotator(0,BaseYaw,0));
 const double Shake=Jiggle*1.4;
 for(int32 I=0;I<Links.Num();++I)if(UStaticMeshComponent* C=Links[I].Get())
 {
  const double K=double(I)/(Links.Num()-1);
  const FVector J(Shake*FMath::Sin(T*31.+I*1.7),Shake*.5*FMath::Sin(T*23.+I),Shake*FMath::Cos(T*37.+I*2.3));
  const FVector P=Now==EState::Broken?FVector(0,2,-2-4.2*I):FVector(-14*K,2+Bulge*25*(1-K),-6*FMath::Sin(PI*K));
  C->SetRelativeLocationAndRotation(P+J,FRotator(Now==EState::Broken?90:0,0,I%2?90:0));
  C->SetVisibility(bLock);C->SetHiddenInGame(!bLock);
 }
 const FVector Lp=(Now==EState::Broken?FVector(0,2.5,-29):FVector(-7,2.5,-12))+FVector(Shake*FMath::Sin(T*19.),0,Shake*FMath::Cos(T*29.));
 if(UStaticMeshComponent* C=Padlock.Get()){C->SetRelativeLocation(Lp);C->SetVisibility(bLock);C->SetHiddenInGame(!bLock);}
 if(UStaticMeshComponent* C=Shackle.Get()){C->SetRelativeLocation(Lp+FVector(0,0,3.6));C->SetVisibility(bLock);C->SetHiddenInGame(!bLock);}
 if(UStaticMeshComponent* C=Staple.Get()){C->SetRelativeLocation(FVector(0,1,0));C->SetVisibility(bLock);C->SetHiddenInGame(!bLock);}
 // The pieces fly out of the doorway and come to rest in the yard; a load finds them lying there.
 const FVector From=AreaOf(RoadOf(Hl.X+LeafWidth*.5,Hl.Y))+FVector(0,1,0);
 const double Fly=BrokeAt<0?1.:FMath::Clamp((T-BrokeAt)/.5,0.,1.);
 for(int32 I=0;I<Pieces.Num();++I)if(UStaticMeshComponent* C=Pieces[I].C.Get())
 {
  const bool bOn=bShow&&Now==EState::Broken;
  C->SetVisibility(bOn);C->SetHiddenInGame(!bOn);
  if(!bOn)continue;
  const FVector Rest=AreaOf(Pieces[I].Rest);
  const double E=1-(1-Fly)*(1-Fly);
  const FVector P=FMath::Lerp(From,Rest,E)+FVector(0,.7*FMath::Sin(PI*Fly)*(.6+.1*I),0);
  C->SetWorldLocationAndRotation(S47Three::ToUE(P),FRotator(Fly<1?(1-Fly)*(140+I*37):0,BaseYaw+Pieces[I].Yaw+(1-Fly)*90*(I%2?1:-1),Fly<1?(1-Fly)*(70+I*23):0));
  C->SetWorldScale3D(Pieces[I].Size);
 }
}

void FS47FarmShedDoor::Stop()
{
 const bool bLive=S47ShedDoorLocal::LiveEnough(M);
 Life.Reset();
 S47FarmBuildings::SetShedDoorShut(false);
 if(bLive)
 {
  if(Scene.IsValid())for(FS47Rect& R:Scene->CollidersMutable(S47ShedDoorLocal::Walk))if(R.Id==S47ShedDoorLocal::Rect)R.MinX=R.MaxX=R.MinZ=R.MaxZ=1e9;
  // Hidden, not destroyed: the leaf is a node of the scene (the walker's pick knows it by its node), and a new Begin in the
  // same world shows the same parts again.
  const auto Hide=[](UStaticMeshComponent* C){if(C){C->SetVisibility(false);C->SetHiddenInGame(true);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}};
  for(const auto& W:LeafParts)Hide(W.Get());
  for(const auto& W:Links)Hide(W.Get());
  for(const FPiece& P:Pieces)Hide(P.C.Get());
  Hide(Padlock.Get());Hide(Shackle.Get());Hide(Staple.Get());
 }
 bBuilt=false;
 Calm();
}

// ---------------------------------------------------------------- the state
void FS47FarmShedDoor::Plan(int32 InSeed,int32 InNight,int32 InRhythm)
{
 if(InSeed==Seed&&InNight==Night&&InRhythm==Rhythm)return;
 Seed=InSeed;Night=InNight;Rhythm=InRhythm;Rand.Initialize(int32(Seed^(Night*7919)^0x53484544));
}

void FS47FarmShedDoor::Set(EState State,EMood InMood,bool bAnimate)
{
 const EMood Mood=State==EState::Locked?InMood:EMood::Still;
 if(State!=Now)
 {
  const EState Was=Now;Now=State;
  if(Now==EState::Broken&&bAnimate&&Was==EState::Locked)Breach();
  else if(Now==EState::Broken)BrokeAt=-1;
  if(Now==EState::Open)Swing=bAnimate?0.:1.;
  if(Now!=EState::Locked)bTried=false;
  SetCollider();
  UE_LOG(LogS47ShedDoor,Display,TEXT("S47: shed door: %s -> %s%s"),StateName(Was),StateName(Now),bAnimate&&Now==EState::Broken?TEXT(" (the breach)"):TEXT(""));
 }
 if(Mood!=MoodNow)
 {
  MoodNow=Mood;MoodSince=T;KickIndex=0;NextKick=-1;NextRattle=T+Rand.FRandRange(1.5f,4.f);
  UE_LOG(LogS47ShedDoor,Display,TEXT("S47: shed door: mood %s"),MoodName(MoodNow));
 }
}

void FS47FarmShedDoor::Calm()
{
 Pending.Reset();CaptureFrom=-1;LastRap=-1;Raps.Reset();Answer.Reset();AnswerAt=-1;bKnocked=false;AnswerCount=0;HushUntil=0;
 Bulge=BulgeV=Jiggle=0;KickIndex=0;NextKick=-1;PushLeft=0;MoodNow=EMood::Still;
}

void FS47FarmShedDoor::Due(double After,S47ShedSound::EKind Kind,float Gain,float Rate,bool bInside,float Lp)
{
 FPend P;P.At=T+FMath::Max(0.,After);P.Kind=Kind;P.Gain=Gain;P.Rate=Rate;P.bInside=bInside;P.Lp=Lp;P.Variant=Var++;
 Pending.Add(P);
}

void FS47FarmShedDoor::Use()
{
 if(Now!=EState::Locked||MoodNow==EMood::Kicks)return;
 if(!bTried)
 {
  bTried=true;Jiggle=1;BulgeV+=.25;
  Due(0,S47ShedSound::EKind::Chain,.85f);
  // Touched, it goes still inside a moment.
  if(MoodNow==EMood::Rattle)HushUntil=FMath::Max(HushUntil,T+4);
  if(M&&M->UI)M->UI->Toast(TEXT("Chained shut through the hasp. A new padlock, locked, on this side."),4);
  return;
 }
 Knock();
}

void FS47FarmShedDoor::Knock()
{
 if(CaptureFrom<0)
 {
  // The hand knocks the way it knocks tonight (the plan's rhythm); more presses while it does add raps of the player's own.
  CaptureFrom=T;Raps=Pattern(Rhythm);
  for(const double R:Raps)Due(R,S47ShedSound::EKind::Knock,.8f,Rand.FRandRange(.97f,1.05f));
  LastRap=T+Raps.Last();
 }
 else
 {
  const double At=T-CaptureFrom;bool bNear=false;
  for(const double R:Raps)bNear|=FMath::Abs(R-At)<.12;
  if(!bNear){Raps.Add(At);Raps.Sort();Due(0,S47ShedSound::EKind::Knock,.8f,Rand.FRandRange(.97f,1.05f));LastRap=FMath::Max(LastRap,T);}
 }
 HushUntil=FMath::Max(HushUntil,LastRap+1.);
}

bool FS47FarmShedDoor::DevKnock()
{
 if(!bBuilt||Now!=EState::Locked||MoodNow==EMood::Kicks)return false;
 bTried=true;Knock();
 return true;
}

void FS47FarmShedDoor::Breach()
{
 BrokeAt=T;Jiggle=1;
 Due(0,S47ShedSound::EKind::Splinter,1.25f);
 Due(0,S47ShedSound::EKind::Kick,1.1f,.88f);
 Due(.22,S47ShedSound::EKind::Chain,.85f,.95f);
 Due(1.4,S47ShedSound::EKind::Breath,.6f,.9f,true,3200.f);
 // Standing in front of it, she is thrown back and has a moment to run (the camera's beat is patch 6).
 if(M&&M->Player&&M->World&&!M->World->bDriving&&M->Web&&M->Web->Area()==S47ShedDoorLocal::Walk)
 {
  const FVector Mid=Middle();
  const FVector2D D(M->Player->Pos.X-Mid.X,M->Player->Pos.Z-Mid.Z);
  if(D.Size()<2.8&&D.Y>-.3){PushLeft=.42;PushDir=FVector2D(D.X*.5,FMath::Max(D.Y,.8)).GetSafeNormal();}
 }
}

FString FS47FarmShedDoor::StateText() const
{
 return FString::Printf(TEXT("door %s, %s%s, knocked %d, answers %d%s"),StateName(Now),MoodName(MoodNow),bModel?TEXT(""):TEXT(" (plain leaf)"),bKnocked?1:0,AnswerCount,AnswerAt>=0?TEXT(", silence before the answer"):TEXT(""));
}

// ---------------------------------------------------------------- each frame
void FS47FarmShedDoor::StepMood(double Dt)
{
 using EK=S47ShedSound::EKind;
 // The knock closes; a long silence; then the same rhythm from inside. Twice; after that it only listens.
 if(CaptureFrom>=0&&T>LastRap+KnockClose)
 {
  bKnocked=true;
  if(MoodNow==EMood::Rattle&&AnswerCount<2)
  {
   AnswerAt=T+AnswerWaitFor(Seed,Night)*(AnswerCount==0?1.:.5);Answer=AnswerFor(Raps);
   HushUntil=FMath::Max(HushUntil,AnswerAt+Answer.Last()+5.);
  }
  else if(MoodNow==EMood::Rattle)HushUntil=FMath::Max(HushUntil,T+20.);
  UE_LOG(LogS47ShedDoor,Display,TEXT("S47: shed door: knocked %d raps%s"),Raps.Num(),AnswerAt>=0?TEXT(", an answer will come"):TEXT(", nothing answers"));
  CaptureFrom=-1;Raps.Reset();
 }
 if(AnswerAt>=0&&T>=AnswerAt)
 {
  // Heavier than knuckles, through the boards: from inside, at the leaf.
  for(const double R:Answer)Due(R,EK::Knock,1.f,.8f,true,2600.f);
  ++AnswerCount;AnswerAt=-1;
  UE_LOG(LogS47ShedDoor,Display,TEXT("S47: shed door: answered in the same rhythm (%d raps), answer %d"),Answer.Num(),AnswerCount);
 }
 if(Now!=EState::Locked)return;
 if(MoodNow==EMood::Rattle)
 {
  if(T<HushUntil||AnswerAt>=0)NextRattle=FMath::Max(NextRattle,HushUntil+Rand.FRandRange(.6f,2.2f));
  else if(T>=NextRattle)
  {
   if(Rand.FRand()<.6f){Due(0,EK::Chain,Rand.FRandRange(.3f,.55f),Rand.FRandRange(.9f,1.1f));Jiggle=FMath::Max(Jiggle,.6);BulgeV+=.12;}
   else Due(0,EK::Heavy,Rand.FRandRange(.35f,.55f),Rand.FRandRange(.9f,1.05f),true,900.f);
   NextRattle=T+Rand.FRandRange(2.4f,7.8f);
  }
 }
 else if(MoodNow==EMood::Kicks)
 {
  const double Age=T-MoodSince;
  if(NextKick<0)NextKick=FirstKick;
  if(Age>=NextKick)
  {
   const float Str=FMath::Min(1.f,.55f+.07f*KickIndex);
   Due(0,EK::Kick,.75f+.35f*Str,Rand.FRandRange(.92f,1.05f));
   Due(.03,EK::Chain,.55f+.3f*Str);
   BulgeV+=1.15*Str;Jiggle=1;
   if(KickIndex%3==2)Due(.7,EK::Heavy,.5f,1.f,true,900.f);
   const double Gap=KickGap(Seed,Night,KickIndex);
   if(Gap>3.2)Due(1.3,EK::Breath,.55f,Rand.FRandRange(.9f,1.02f),false,3000.f);
   NextKick+=Gap;++KickIndex;
  }
 }
 // The leaf against its chain: a stiff spring, the chain stopping it a few centimetres out, the frame stopping it going in.
 BulgeV+=(-70.*Bulge-6.*BulgeV)*Dt;Bulge+=BulgeV*Dt;
 if(Bulge<0){Bulge=0;BulgeV=FMath::Max(0.,-.3*BulgeV);}
 if(Bulge>.07){Bulge=.07;BulgeV=-.3*FMath::Abs(BulgeV);}
}

void FS47FarmShedDoor::StepSounds()
{
 for(int32 I=0;I<Pending.Num();)
 {
  if(Pending[I].At>T){++I;continue;}
  const FPend P=Pending[I];Pending.RemoveAtSwap(I);
  S47ShedSound::Play(M,P.Kind,P.bInside?Inside():Middle(),P.Gain,P.Rate,P.Variant,P.Lp);
 }
}

void FS47FarmShedDoor::Step(double Dt)
{
 if(!M||!IsValid(M->Web))return;
 if(Scene.Get()!=M->Web.Get()){LeafParts.Reset();Hinge.Reset();bBuilt=false;bColliderAdded=false;Scene=M->Web;}
 if(!Build())return;
 const double D=FMath::Clamp(Dt,0.,.1);
 T+=D;
 Jiggle*=FMath::Exp(-D*4.);
 if(Now==EState::Open&&Swing<1)Swing=FMath::Min(1.,Swing+D/.6);
 StepMood(D);
 StepSounds();
 if(PushLeft>0&&M->Player&&M->World&&!M->World->bDriving)
 {
  const double V=6.*PushLeft/.42;
  M->Player->MoveBy(PushDir.X*V*D,PushDir.Y*V*D);PushLeft-=D;
 }
 Pose();
}

// ---------------------------------------------------------------- self-test
bool FS47FarmShedDoor::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s shed door: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 // The answer is the knock's own rhythm, whatever the seed gave and whatever the player added.
 {
  bool bSame=true,bDiffer=true,bShort=true;
  for(int32 R=0;R<4;++R)
  {
   const TArray<double> P=Pattern(R);
   bSame&=AnswerFor(P)==P;bShort&=P.Num()>=3&&P[0]==0&&P.Last()<2.;
   for(int32 Q=R+1;Q<4;++Q)bDiffer&=Pattern(Q)!=P;
  }
  const TArray<double> Own={.5,.8,1.45,1.6};
  const TArray<double> Back=AnswerFor(Own);
  bool bOwn=Back.Num()==Own.Num();
  for(int32 I=0;bOwn&&I<Own.Num();++I)bOwn&=FMath::IsNearlyEqual(Back[I],Own[I]-Own[0],1e-9);
  bool bSeed=true;
  for(int32 Seed=1;Seed<=300;++Seed)
  {
   const double A=AnswerWaitFor(Seed,4),B=AnswerWaitFor(Seed,4);
   bSeed&=A==B&&A>=6.&&A<=10.5&&FMath::RoundToInt(A)!=47;
  }
  Check(bSame&&bDiffer&&bShort&&bOwn&&bSeed,TEXT("the answer from inside is the knock's own rhythm (the seed's four, or the player's own raps), after the same long silence for the same seed"));
 }
 // The kicks come at uneven pauses, never Story's number.
 {
  bool bRange=true,bUneven=false,bNo47=true;
  for(int32 Seed=1;Seed<=200;++Seed)
  {
   double Last=-1;
   for(int32 I=0;I<20;++I)
   {
    const double G=KickGap(Seed,4,I);
    bRange&=G>=1.5&&G<=6.1&&G==KickGap(Seed,4,I);
    bNo47&=FMath::RoundToInt(G*10)!=47;
    if(Last>=0&&FMath::Abs(G-Last)>.4)bUneven=true;
    Last=G;
   }
  }
  Check(bRange&&bUneven&&bNo47&&FirstKick!=4.7,TEXT("the kicks come at uneven pauses of a second and a half to six, the same for the same seed"));
 }
 // The states: only the locked door is shut; the doorway's collider closes the gap in the wall exactly.
 {
  const bool bShut=ShutFor(EState::Locked)&&!ShutFor(EState::Open)&&!ShutFor(EState::Broken);
  const FBox2D Way=S47FarmBuildings::ShedDoorway(),Box=ClosedBox();
  const FVector Mid=S47FarmWorld::RoadToArea(TEXT("saro"),FVector(Way.GetCenter().X,0,Way.GetCenter().Y));
  const bool bBox=Box.IsInside(FVector2D(Mid.X,Mid.Z))&&FMath::IsNearlyEqual(Box.GetSize().X,Way.GetSize().X,.001)&&FMath::IsNearlyEqual(Way.GetSize().X,LeafWidth+.02,.01);
  Check(bShut&&bBox,TEXT("locked is the only shut state, and its collider is the doorway between the wall's two runs"));
 }
 // The gap in the shed's wall follows the door: open, the walker goes through; shut, it does not.
 {
  const bool bWas=S47FarmBuildings::ShedDoorShut();
  const FVector2D C=S47FarmBuildings::ShedDoorway().GetCenter();
  S47FarmBuildings::SetShedDoorShut(false);const bool bOpen=!S47FarmBuildings::Blocked(C.X,C.Y,.27)||!S47FarmBuildings::Available();
  S47FarmBuildings::SetShedDoorShut(true);const bool bClosed=S47FarmBuildings::Blocked(C.X,C.Y,.27)||!S47FarmBuildings::Available();
  S47FarmBuildings::SetShedDoorShut(bWas);
  Check(bOpen&&bClosed,TEXT("the gap in the feed shed's wall follows the door: through it when open or broken, stopped when shut"));
 }
 return OK;
}
