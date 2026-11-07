#include "S47IndoorDread.h"
#include "S47Audio.h"
#include "S47FarmHouse.h"
#include "S47FarmSite.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Glimpses.h"
#include "S47HighwayUfo.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Seen.h"
#include "S47Sky.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47WorldLife.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SceneComponent.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "UObject/GarbageCollection.h"

namespace
{
 const TCHAR* const PeekAsset=TEXT("/Game/Licensed/TripoNPC/Roswell/SK_Tripo_Roswell.SK_Tripo_Roswell");
 constexpr double RiseSeconds=1.25,DuckSeconds=1.1,PeekMax=10;
 constexpr double PeekChance=.5,FloodChance=.4;
 constexpr int32 Bearings=36,Rings=7;
 // Crouched, the top of its head stands this high over its feet; raised, what looks over the sill.
 const double LowHead=S47Roswell::HeightM-S47Roswell::DuckM+.04,HighHead=S47Roswell::ProbeM;
 double PeekEase(double X){X=FMath::Clamp(X,0.,1.);return X*X*(3.-2.*X);}
 const FVector HouseFrame(-24,0,300); // The old house stands in the farm's part of the SARO area.
}

void FS47IndoorDread::Begin(const S47J::FObj& Saved)
{
 Stop();
 PeekRollDay=S47J::Int(Saved,TEXT("peekRollDay"));PeekCount=S47J::Int(Saved,TEXT("peeks"));
 InsideFor=FloodFor=0;bPeekTonight=bDevWanted=false;ProbeAt=SearchRings=EmptyRings=0;Best=FCandidate();
 FloodArea.Empty();FloodTitle.Empty();FloodAt=FVector::ZeroVector;FloodClock=0;FloodWho=FloodRollDay=FloodCount=0;
 const S47J::FObj Room=S47J::Obj(Saved,TEXT("floodRoom"));
 const TArray<double> At=S47J::Nums(Room,TEXT("at"));
 if(Room.IsValid()&&At.Num()==3&&!S47J::Str(Room,TEXT("area")).IsEmpty())
 {
  FloodArea=S47J::Str(Room,TEXT("area"));FloodTitle=S47J::Str(Room,TEXT("title"));FloodAt=FVector(At[0],At[1],At[2]);
  FloodClock=S47J::Num(Room,TEXT("clock"));FloodWho=FMath::Clamp(S47J::Int(Room,TEXT("who")),0,2);
  FloodRollDay=S47J::Int(Room,TEXT("rollDay"));FloodCount=S47J::Int(Room,TEXT("count"));
 }
 if(M)Sight.Init(M);
}

void FS47IndoorDread::Stop()
{
 const bool bLive=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);
 if(bLive&&Scene.Get()==M->Web.Get()&&Body.Ready())Body.Peek(false,1,0,0);
 Stage=EStage::None;T=Noticed=0;Withdraw=-1;ExposureNow=0;
}

S47J::FObj FS47IndoorDread::Save() const
{
 const auto O=S47J::New();
 if(PeekRollDay>0)S47J::Set(O,TEXT("peekRollDay"),PeekRollDay);
 if(PeekCount>0)S47J::Set(O,TEXT("peeks"),PeekCount);
 if(!FloodArea.IsEmpty())
 {
  const auto R=S47J::New();
  S47J::Set(R,TEXT("area"),FloodArea);S47J::Set(R,TEXT("title"),FloodTitle);S47J::Set(R,TEXT("at"),TArray<double>{FloodAt.X,FloodAt.Y,FloodAt.Z});
  S47J::Set(R,TEXT("clock"),FloodClock);S47J::Set(R,TEXT("who"),FloodWho);S47J::Set(R,TEXT("rollDay"),FloodRollDay);S47J::Set(R,TEXT("count"),FloodCount);
  S47J::Set(O,TEXT("floodRoom"),R);
 }
 return O;
}

void FS47IndoorDread::AfterTaken(){if(M&&M->Freeform&&!FloodArea.IsEmpty())FloodClock=M->Freeform->Clock();}

void FS47IndoorDread::SurvivedFlood(const FVector& At,const FString& Area,int32 Who,const FString& Title)
{
 if(!M||!M->Freeform)return;
 const bool bSameRoom=Area==FloodArea&&FVector::Dist(At,FloodAt)<6;
 FloodArea=Area;FloodTitle=Title;FloodAt=At;FloodWho=FMath::Clamp(Who,0,2);FloodClock=M->Freeform->Clock();FloodFor=0;
 FloodCount=bSameRoom?FloodCount+1:1;
 UE_LOG(LogTemp,Display,TEXT("S47: flood room remembered in %s at %s, %d times"),*Area,*At.ToCompactString(),FloodCount);
 if(bSameRoom&&FloodCount>=2)
 {
  FString Note=FString::Printf(TEXT("%s, the same room as before. The light came back to it and I got out of it again. It says where and when, not why here."),*Title);
  Note[0]=FChar::ToUpper(Note[0]);
  M->Freeform->RecordEvidence(FString::Printf(TEXT("flood-room-%d-%d"),M->Freeform->Day(),FloodCount),Note);
 }
 M->RequestAutosave();
}

TOptional<FVector> FS47IndoorDread::PeekEye() const
{
 if(Stage==EStage::None)return {};
 return BasePos+Cover*(1.-ExposureNow)+FVector(0,bWindow?LowHead+(HighHead-LowHead)*ExposureNow:HighHead,0);
}

FVector FS47IndoorDread::PeekHiddenHead() const{return BasePos+Cover+FVector(0,bWindow?LowHead:HighHead,0);}
FVector FS47IndoorDread::PeekRaisedHead() const{return BasePos+FVector(0,HighHead,0);}

bool FS47IndoorDread::PlayerInPeekPlace() const
{
 if(!M||!M->Web||!M->Player||!M->Freeform||!M->Freeform->Taken)return false;
 const FString Area=M->Web->Area();
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 // The motel room has its own area, with the walk outside its door in it.
 if(Area==TEXT("room6"))return M->World&&M->World->Indoors(Feet);
 if(!M->Freeform->Taken->Indoors())return false;
 if(Area==TEXT("road")||Area==TEXT("diner"))return true;
 return Area==TEXT("saro")&&S47FarmHouse::OnPad(Feet.X-HouseFrame.X,Feet.Z-HouseFrame.Z);
}

bool FS47IndoorDread::Quiet() const
{
 if(!M||!M->Freeform||!M->Freeform->Active()||M->Freeform->Dead()||M->Freeform->InTutorial()||M->Freeform->Held())return false;
 if(M->Mode!=US47Main::EMode::Play||!M->World||M->World->bDriving||!M->Player||!M->Night||!M->Night->Glimpses)return false;
 if(M->HighwayUfo&&M->HighwayUfo->Busy())return false;
 if(M->Freeform->Director&&!M->Freeform->Director->ActiveEvent().IsEmpty())return false;
 if(M->Freeform->Taken&&M->Freeform->Taken->Busy())return false;
 if(M->Freeform->FarmSite&&M->Freeform->FarmSite->VisitHushed())return false;
 if(!M->Night->Glimpses->Individuals().IsEmpty())return false;
 return M->Life&&M->Life->Sky.IsValid()&&M->Life->Sky->Daylight()<.1;
}

bool FS47IndoorDread::BuildBody()
{
 if(!M||!IsValid(M->Web.Get())||(bBodyFailed&&Scene.Get()==M->Web.Get()))return false;
 if(Scene.Get()!=M->Web.Get()){Scene=M->Web.Get();Root.Reset();Body=FS47TripoCreature();bBodyFailed=false;Sight.Init(M);}
 if(Body.Ready())return true;
 AS47Web* W=M->Web.Get();
 auto* R=NewObject<USceneComponent>(W,TEXT("S47PeekRoot"));
 W->AddInstanceComponent(R);R->SetMobility(EComponentMobility::Movable);R->SetupAttachment(W->GetRootComponent());R->RegisterComponent();
 Root=R;
 // No model in this build: nothing looks in. The rest of the night goes on.
 if(!Body.Build(W,R,TEXT("S47PeekBody"),PeekAsset)){bBodyFailed=true;UE_LOG(LogTemp,Display,TEXT("S47: peek: no Roswell model in this build"));return false;}
 return true;
}

TOptional<double> FS47IndoorDread::GroundUnder(double X,double Z,double FromY,double ToY) const
{
 if(!M||!M->Web||!M->Web->GetWorld())return {};
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47PeekGround),true);
 if(M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(X,FromY,Z),S47Three::ToUE(X,ToY,Z),ECC_Visibility,Q)&&Hit.ImpactNormal.Z>=.68)
  return S47Three::ToThree(Hit.ImpactPoint).Y;
 return {};
}

// Outside a window: open sky over it, its crouched head behind the sill from here, its raised head over it from here,
// close behind whatever hides it. The feet may stand a little above the ground, on something: they can only be seen
// from outside, by which time it has gone.
bool FS47IndoorDread::TryWindow(const FS47Cam& Cam,const FVector& At,FCandidate& Out) const
{
 auto* World=M->Web->GetWorld();
 const double Floor=M->Player->FloorY;
 const auto Ground=GroundUnder(At.X,At.Z,Cam.Pos.Y+.6,Floor-3);
 if(!Ground.IsSet()||Ground.GetValue()>Floor+.35||Ground.GetValue()<Floor-1.6)return false;
 {
  FHitResult Roof;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47PeekRoof),true);
  const FVector Over(At.X,Ground.GetValue()+1.4,At.Z);
  if(World->LineTraceSingleByChannel(Roof,S47Three::ToUE(Over),S47Three::ToUE(Over+FVector(0,9,0)),ECC_Visibility,Q))return false;
 }
 for(const double Lift:{0.,.2,.4})
 {
  const FVector Feet(At.X,Ground.GetValue()+Lift,At.Z);
  const FVector Low=Feet+FVector(0,LowHead,0),High=Feet+FVector(0,HighHead,0);
  if(Sight.ClearSight(Cam.Pos,Low)||Sight.ClearSight(Cam.Pos,Feet+FVector(0,.1,0))||!Sight.ClearSight(Cam.Pos,High))continue;
  // Whatever hides it is close in front of it, and it is not standing in that wall.
  FHitResult Sill;FCollisionQueryParams Q(SCENE_QUERY_STAT(S47PeekSill),true);
  if(!World->LineTraceSingleByChannel(Sill,S47Three::ToUE(Cam.Pos),S47Three::ToUE(Low),ECC_Visibility,Q))continue;
  const double Behind=FVector::Dist(S47Three::ToThree(Sill.ImpactPoint),Low);
  if(Behind<.45||Behind>2.2)continue;
  Out.bFound=true;Out.bIsWindow=true;Out.Feet=Feet;Out.Cover=FVector::ZeroVector;
  return true;
 }
 return false;
}

// Round the edge of a door or a wall: it stands here in plain sight of the player, a step to the side it would not be,
// and the step between is clear. It leans out from there.
bool FS47IndoorDread::TryCorner(const FS47Cam& Cam,const FVector& At,FCandidate& Out) const
{
 const double Floor=M->Player->FloorY;
 const auto Ground=GroundUnder(At.X,At.Z,Floor+1.2,Floor-1);
 if(!Ground.IsSet()||FMath::Abs(Ground.GetValue()-Floor)>.3)return false;
 const FVector Feet(At.X,Ground.GetValue(),At.Z);
 if(!Sight.ClearSight(Cam.Pos,Feet+FVector(0,HighHead,0))||!Sight.ClearSight(Cam.Pos,Feet+FVector(0,.5,0)))return false;
 const FVector Along=FVector(Feet.X-Cam.Pos.X,0,Feet.Z-Cam.Pos.Z).GetSafeNormal();
 const FVector Side(-Along.Z,0,Along.X);
 for(const double Sign:{1.,-1.})
 {
  const FVector Aside=Feet+Side*(Sign*1.1);
  const auto AsideGround=GroundUnder(Aside.X,Aside.Z,Floor+1.2,Floor-1);
  if(!AsideGround.IsSet()||FMath::Abs(AsideGround.GetValue()-Floor)>.3)continue;
  const FVector Hid(Aside.X,AsideGround.GetValue(),Aside.Z);
  if(Sight.ClearSight(Cam.Pos,Hid+FVector(0,HighHead,0))||Sight.ClearSight(Cam.Pos,Hid+FVector(0,.5,0)))continue;
  if(!Sight.ClearSight(Feet+FVector(0,.7,0),Hid+FVector(0,.7,0)))continue;
  Out.bFound=true;Out.bIsWindow=false;Out.Feet=Feet;Out.Cover=Hid-Feet;
  return true;
 }
 return false;
}

void FS47IndoorDread::ScoreOf(const FS47Cam& Cam,FCandidate& C) const
{
 const FVector Head=C.Feet+FVector(0,HighHead,0);
 const FVector N=Cam.Project(Head);
 C.bInView=N.Z<1&&FMath::Abs(N.X)<.9&&FMath::Abs(N.Y)<.9;
 // At the edge of what they are looking at, best. Dead centre is a little too much of a gift.
 const double Dot=FVector::DotProduct(Cam.Forward,(Head-Cam.Pos).GetSafeNormal());
 C.Score=(C.bInView?1.:0.)+(C.bIsWindow?.3:0.)+(Dot>.985?-.5:0.)+FMath::FRand()*.2;
}

void FS47IndoorDread::SearchStep(const FS47Cam& Cam)
{
 // Close together: a sill hides a crouched head only from just behind it.
 static const double Steps[Rings]={2.2,2.8,3.4,4.2,5.2,6.5,8.};
 for(int32 N=0;N<10&&ProbeAt<Bearings*Rings;++N)
 {
  const int32 Index=ProbeAt++;
  const double A=2*UE_DOUBLE_PI*(Index%Bearings)/Bearings+(Index/Bearings)*.11;
  const double Reach=Steps[Index/Bearings];
  const FVector At(Cam.Pos.X+FMath::Cos(A)*Reach,Cam.Pos.Y,Cam.Pos.Z+FMath::Sin(A)*Reach);
  FCandidate C;
  if(TryWindow(Cam,At,C)||(Reach>=3&&TryCorner(Cam,At,C)))
  {
   ScoreOf(Cam,C);
   if(C.Score>Best.Score)Best=C;
  }
 }
}

bool FS47IndoorDread::StillHidden(const FS47Cam& Cam,const FCandidate& C) const
{
 const FVector Start=C.Feet+C.Cover;
 return !Sight.ClearSight(Cam.Pos,Start+FVector(0,C.bIsWindow?LowHead:HighHead,0));
}

void FS47IndoorDread::StartPeek(const FS47Cam& Cam,const FCandidate& C)
{
 if(!BuildBody())return;
 Stage=EStage::Up;PeekArea=M->Web->Area();BasePos=C.Feet;Cover=C.Cover;bWindow=C.bIsWindow;bSeen=bDuckSound=false;
 Facing=FMath::Atan2(Cam.Pos.X-BasePos.X,Cam.Pos.Z-BasePos.Z);T=Noticed=LeftAt=0;Withdraw=-1;ExposureNow=0;++PeekCount;
 Draw();
 UE_LOG(LogTemp,Display,TEXT("S47: peek starts in %s, %s, %.1f m from the eyes, in view %d, at clock %.0f"),*PeekArea,bWindow?TEXT("window"):TEXT("corner"),FVector::Dist(Cam.Pos,BasePos),C.bInView,M->Freeform->Clock());
 M->RequestAutosave();
}

void FS47IndoorDread::Draw()
{
 auto* R=Root.Get();if(!R||!Body.Ready())return;
 const double E=ExposureNow;
 R->SetWorldLocation(S47Three::ToUE(BasePos+Cover*(1.-E)));
 R->SetWorldRotation(FRotationMatrix::MakeFromX(S47Three::DirToUE(FVector(FMath::Sin(Facing),0,FMath::Cos(Facing)))).Rotator());
 const double HeadTurn=.2*FMath::Sin(T*.85);
 if(Stage==EStage::Leaving)Body.Apply(true,0,.9,T*6.,T,false,false);
 else if(bWindow||Cover.IsNearlyZero()||E>.995||E<.005)Body.Peek(true,bWindow?E:1.,HeadTurn,T);
 else Body.Apply(true,HeadTurn,.65,T*5.,T,false,true);
}

void FS47IndoorDread::EndPeek()
{
 if(Body.Ready())Body.Peek(false,1,0,0);
 Stage=EStage::None;ExposureNow=0;Withdraw=-1;
 UE_LOG(LogTemp,Display,TEXT("S47: peek over, seen %d, after %.1f s"),bSeen,T);
 if(bSeen&&M&&M->Freeform)
 {
  const FString Key=FString::Printf(TEXT("peek-%d"),M->Freeform->Day());
  if(!M->Freeform->HasFinding(Key))M->Freeform->RecordEvidence(Key,bWindow
   ?FString(TEXT("Something small looked in at the window and was gone when I looked properly. No marks on the sill. It says what I saw, not what it was."))
   :FString(TEXT("Something small leaned round the edge of the doorway and went back. It says what I saw, not what it was.")));
 }
 if(M)M->RequestAutosave();
}

void FS47IndoorDread::StepPeek(double Dt)
{
 // Gone from here, in the truck or something bigger going on: it is simply not looking any more.
 if(!M||!M->Web||M->Web->Area()!=PeekArea||!M->Player||!M->Night||(M->World&&M->World->bDriving)){EndPeek();return;}
 // A menu holds it where it is, for no time at all.
 if(M->Freeform->Held())return;
 T+=Dt;
 const auto In=M->Night->GlimpseIn(true);
 const FVector Eye=BasePos+FVector(0,HighHead,0);
 const FVector N=In.Cam.Project(Eye);
 const bool bInView=N.Z<1&&FMath::Abs(N.X)<.96&&FMath::Abs(N.Y)<.96;
 const double Off=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(In.Cam.Forward,(Eye-In.Cam.Pos).GetSafeNormal()),-1.,1.)));
 const double Dist=FVector::Dist(In.Cam.Pos,Eye);
 const bool bClear=Sight.ClearSight(In.Cam.Pos,Eye);
 if(Stage==EStage::Leaving)
 {
  // Caught still in plain sight after it ducked: off it goes, along the ground, until something is in the way.
  const FVector Away=FVector(BasePos.X-In.Cam.Pos.X,0,BasePos.Z-In.Cam.Pos.Z).GetSafeNormal();
  BasePos+=Away*(1.7*Dt);
  if(const auto Ground=GroundUnder(BasePos.X,BasePos.Z,BasePos.Y+1.2,BasePos.Y-2))BasePos.Y=FMath::FInterpTo(BasePos.Y,Ground.GetValue(),Dt,10.);
  ExposureNow=1;Draw();
  if(!bInView||!bClear||Dist>16||T-LeftAt>9)EndPeek();
  return;
 }
 const bool bLooked=bInView&&Off<18&&bClear;
 if(T>RiseSeconds&&bLooked){Noticed+=Dt;bSeen|=Body.Rendered();}
 else Noticed=FMath::Max(0.,Noticed-Dt*.4);
 if(Withdraw<0&&(Noticed>1.15||Dist<2||T>PeekMax||(In.bFlashlight&&bLooked&&T>2.5)||!Quiet()))Withdraw=T;
 ExposureNow=Withdraw>=0?1.-PeekEase((T-Withdraw)/DuckSeconds):PeekEase(T/RiseSeconds);
 Draw();
 if(Withdraw>=0&&!bDuckSound&&M->Audio)
 {
  bDuckSound=true;
  M->Audio->Sfx(TEXT("stepConcrete0"),FS47Sound{.Gain=.16f,.Rate=.8f,.At=BasePos});
 }
 if(Withdraw>=0&&T-Withdraw>=DuckSeconds)
 {
  // Down, or back round: gone if that hides it from where the player is now. Otherwise it has been watched doing it.
  const FVector Start=BasePos+Cover+FVector(0,bWindow?LowHead:HighHead,0);
  if(!bInView||!Sight.ClearSight(In.Cam.Pos,Start)){EndPeek();return;}
  if(T-Withdraw>=DuckSeconds+1.5){Stage=EStage::Leaving;LeftAt=T;BasePos+=Cover;Cover=FVector::ZeroVector;}
 }
}

void FS47IndoorDread::StepPeekDraw(double Dt)
{
 const bool bQuietNow=Quiet();
 const bool bHour=M&&M->Freeform&&FS47FarmSite::TheirHours(M->Freeform->Clock());
 const bool bInside=bQuietNow&&PlayerInPeekPlace();
 InsideFor=bInside?InsideFor+Dt:0;
 if(!bQuietNow||!bInside||(!bHour&&!bDevWanted))
 {
  ProbeAt=0;Best=FCandidate();SearchRings=0;
  return;
 }
 const int32 Day=M->Freeform->Day();
 if(!bDevWanted)
 {
  if(PeekRollDay==Day&&!bPeekTonight)return;
  // A night's own draw, once they have been indoors a little while.
  if(PeekRollDay!=Day)
  {
   if(InsideFor<20)return;
   PeekRollDay=Day;bPeekTonight=FMath::FRand()<PeekChance;PeekWait=InsideFor+FMath::FRandRange(5.f,50.f);
   UE_LOG(LogTemp,Display,TEXT("S47: peek draw for day %d: %d, in %.0f s"),Day,bPeekTonight,PeekWait-InsideFor);
   M->RequestAutosave();
   if(!bPeekTonight)return;
  }
  if(InsideFor<PeekWait)return;
 }
 if(RingPause>0){RingPause-=Dt;return;}
 const auto In=M->Night->GlimpseIn(true);
 SearchStep(In.Cam);
 if(ProbeAt<Bearings*Rings)return;
 ProbeAt=0;++SearchRings;
 // One in their sight if the room gives it soon. Otherwise any: turning round to it is its own thing.
 if(Best.bFound&&(Best.bInView||SearchRings>=8)&&StillHidden(In.Cam,Best))
 {
  const FCandidate Chosen=Best;Best=FCandidate();SearchRings=0;
  StartPeek(In.Cam,Chosen);
  // The night's one, whether a body could be had for it or not.
  bPeekTonight=false;bDevWanted=false;
  return;
 }
 if(!Best.bFound)
 {
  ++EmptyRings;
  if(EmptyRings==1||EmptyRings%20==0)UE_LOG(LogTemp,Display,TEXT("S47: peek: no window or edge in sight from %s, %d rounds"),*In.Cam.Pos.ToCompactString(),EmptyRings);
  if(bDevWanted){bDevWanted=false;if(M->UI)M->UI->Toast(TEXT("Developer event: no window or door edge in sight from here."),3);}
 }
 Best=FCandidate();RingPause=1.5;
}

void FS47IndoorDread::StepFloodRoom(double Dt)
{
 if(FloodArea.IsEmpty()||!M||!M->Web||!M->Player||!M->Freeform->Taken)return;
 const double Clock=M->Freeform->Clock();
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const bool bHere=Quiet()&&M->Web->Area()==FloodArea&&FVector2D::Distance(FVector2D(Feet.X,Feet.Z),FVector2D(FloodAt.X,FloodAt.Z))<6&&M->Freeform->Taken->Indoors();
 FloodFor=bHere?FloodFor+Dt:0;
 if(!bHere||!FS47FarmSite::TheirHours(Clock)||Clock<FloodClock+FloodRestSeconds)return;
 const int32 Day=M->Freeform->Day();
 if(FloodRollDay==Day||FloodFor<25)return;
 // A night's own draw, and at most one. The rest is long whatever it says.
 FloodRollDay=Day;
 const bool bAgain=FMath::FRand()<FloodChance;
 UE_LOG(LogTemp,Display,TEXT("S47: flood room draw for day %d: %d"),Day,bAgain);
 M->RequestAutosave();
 if(bAgain)M->Freeform->Taken->Begin(ES47Taker(FloodWho),FloodTitle);
}

void FS47IndoorDread::Step(double Dt)
{
 if(!M||!M->Freeform||!M->Freeform->Active()||Dt<=0||!M->Web)return;
 if(M->bPausedByMenu||M->ModalOpen())
 {
  // Nothing moves and nothing is counted. The body stays as it was.
  return;
 }
 if(Stage!=EStage::None){StepPeek(Dt);return;}
 StepPeekDraw(Dt);
 if(Stage==EStage::None)StepFloodRoom(Dt);
}

bool FS47IndoorDread::DevPeek()
{
 if(!M||!M->Freeform||!M->Freeform->Active()||Stage!=EStage::None||!PlayerInPeekPlace())return false;
 bDevWanted=true;ProbeAt=0;SearchRings=7;Best=FCandidate();RingPause=0;
 return true;
}

bool FS47IndoorDread::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s indoor dread: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 FS47IndoorDread Old(nullptr);Old.Begin(nullptr);
 Check(!Old.HasFloodRoom()&&!Old.PeekActive()&&Old.Save()->Values.Num()==0,TEXT("an older case has no lit room and nothing to save"));
 const auto Saved=S47J::New();const auto Room=S47J::New();
 S47J::Set(Room,TEXT("area"),TEXT("diner"));S47J::Set(Room,TEXT("title"),TEXT("the church"));S47J::Set(Room,TEXT("at"),TArray<double>{1.,2.,3.});
 S47J::Set(Room,TEXT("clock"),90000.);S47J::Set(Room,TEXT("who"),2);S47J::Set(Room,TEXT("rollDay"),3);S47J::Set(Room,TEXT("count"),1);
 S47J::Set(Saved,TEXT("floodRoom"),Room);S47J::Set(Saved,TEXT("peekRollDay"),4);
 FS47IndoorDread Copy(nullptr);Copy.Begin(S47J::Read(S47J::Write(Saved)));
 Check(Copy.HasFloodRoom()&&Copy.FloodRoomClock()==90000&&Copy.FloodRoomAt().Equals(FVector(1,2,3))&&Copy.FloodRoomRollDay()==3&&Copy.PeekRollDayOf()==4,TEXT("the lit room and the night's draws survive Continue"));
 FS47IndoorDread Again(nullptr);Again.Begin(Copy.Save());
 Check(S47J::Write(Copy.Save())==S47J::Write(Again.Save()),TEXT("saving again changes nothing"));
 return OK;
}
