#include "S47WeekCase.h"
#include "S47Drive.h"
#include "S47FarmShedDoor.h"
#include "S47FarmWorld.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Noise.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47Trouble.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/SpotLightComponent.h"
#include "CoreGlobals.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47WeekCab,Log,All);

namespace S47Week{double Pace(const US47Main* M);}

// WEEKLY-CASE-20261109, patch 3: the truck on the yard and the cab, the player's thoughts at the door and in the dead cab. See
// S47WeekCase.h (the cab's part). The searcher is patch 4's; until it is in, ReserveSearcher stands in for it, so that every rule here
// can be met and tried now. Nothing here is saved: on the ground nothing is, and Continue redoes the night.
namespace S47WeekCabLocal
{
 TAutoConsoleVariable<float> CvSweepLumens(TEXT("s47.weekcab.SweepLumens"),26000.f,TEXT("Week case: the light that sweeps through the truck's windshield, lumens."));
 TAutoConsoleVariable<float> CvSweepCone(TEXT("s47.weekcab.SweepCone"),11.f,TEXT("Week case: that light's outer cone, degrees."));
 TAutoConsoleVariable<float> CvShake(TEXT("s47.weekcab.Shake"),.8f,TEXT("Week case: how hard the truck is rocked while it is at the cab, 0 to 1."));
 const FLinearColor Cold(.8f,.86f,1.f);
 constexpr double Never=1e9;
 bool LiveEnough(US47Main* M){return !IsEngineExitRequested()&&!GExitPurge&&!IsGarbageCollecting()&&IsValid(M);}
}

// ---------------------------------------------------------------- the words
const TCHAR* FS47WeekCase::ThoughtLocked(){return TEXT("Somebody's locked in there.");}
const TCHAR* FS47WeekCase::ThoughtLockedIn(){return TEXT("…or somebody locked it in.");}
const TCHAR* FS47WeekCase::ThoughtStuck(){return TEXT("Don't flood it. Get out of sight.");}
const TCHAR* FS47WeekCase::ThoughtHeld(){return TEXT("The key won't even turn. Not with it this close.");}

// ---------------------------------------------------------------- the pure rules
FS47WeekCase::FCabOut FS47WeekCase::StepCab(FCab& C,const FCabIn& In)
{
 FCabOut O;
 const double Dt=FMath::Max(0.,In.Dt);
 const bool bGround=In.Phase==EPhase::Phase1||In.Phase==EPhase::Phase2||In.Phase==EPhase::Phase3;
 if(!bGround)return O;
 // Rolling in onto the yard on the case's night: dead, once.
 if(!C.bStalled&&In.bInCab&&In.bAtYard){C.bStalled=true;C.StallAge=0;C.Quiet=0;C.Turns=0;O.bStallNow=true;O.Why=TEXT("died on the yard");}
 if(!C.bStalled||C.bCaught)return O;
 C.StallAge+=Dt;C.Quiet+=Dt;
 if(In.bNoise)C.Quiet=0;
 C.Listen=FMath::Max(0.,C.Listen-Dt);
 const bool bNear=In.bSearcherNear||In.SearcherDist<NearCab;
 if(bNear&&In.bInCab){O.bHoldKey=true;O.bShake=true;O.bSweep=true;C.NearFor+=Dt;}
 else C.NearFor=0;
 // Week one: whatever it does at the cab, nobody is taken in it.
 O.bTake=bNear&&In.bInCab&&MayTakeInCab(In.Week);
 if(In.bTurned&&In.bInCab&&!O.bHoldKey)
 {
  O.bHeard=true;++C.Turns;
  const bool bFuse=C.StallAge>=CatchFuse;
  const bool bRule=In.Phase!=EPhase::Phase2&&In.SearcherDist>CatchFar&&C.Quiet>=CatchQuiet;
  if(bFuse||bRule){O.bCatch=true;C.bCaught=true;O.Why=bFuse?TEXT("caught: five minutes dead"):TEXT("caught: it far off and quiet long enough");}
  else O.Why=In.Phase==EPhase::Phase2?TEXT("cranks: it is awake in there"):TEXT("cranks: too soon or too close");
  if(In.Phase==EPhase::Phase2&&!O.bCatch&&++C.Phase2Turns==2){C.Listen=ListenSecs;O.bListen=true;O.Why=TEXT("the second turn: the kicks stop, it listens");}
  C.Quiet=0;
 }
 // Stuck in the dead cab: turning and turning, or a long while there after it is loose.
 if(!C.bStuckSaid&&!C.bCaught&&In.bInCab&&(C.Turns>=StuckTurns||(In.Phase!=EPhase::Phase1&&C.StallAge>=StuckSecs&&C.Turns>=1))){C.bStuckSaid=true;O.bStuck=true;}
 return O;
}

void FS47WeekCase::ReserveSearcher(EPhase Phase,double ToShed,double Since,bool& bNear,double& Dist)
{
 bNear=false;Dist=1e4;
 switch(Phase)
 {
 case EPhase::Phase1:case EPhase::Phase2:Dist=ToShed;break; // shut in
 case EPhase::Phase3:
  // To the last sound: a turn of the key brings it to the truck, a while there, then off among the buildings.
  if(Since<ReserveCome){Dist=FMath::Lerp(ToShed,4.,FMath::Clamp(Since/ReserveCome,0.,1.));}
  else if(Since<ReserveCome+ReserveStay){Dist=4.;bNear=true;}
  else Dist=60.;
  break;
 default:break;
 }
}

FVector FS47WeekCase::CabYard()
{
 // Where the farm's road goes over to ruts, at the edge of the yard (S47FarmWorld's decals: road to x 110, ruts by 118).
 constexpr double X=114.;const double Z=S47RoadTerrain::RanchZ(X);
 return S47FarmWorld::RoadToArea(TEXT("saro"),FVector(X,S47FarmWorld::FloorRoad(X,Z),Z));
}

FString FS47WeekCase::CabText() const
{
 if(!Cab.bStalled)return TEXT("truck running");
 return FString::Printf(TEXT("truck %s, %d turns, dead %.0f s, quiet %.0f s%s%s"),Cab.bCaught?TEXT("caught"):TEXT("dead"),Cab.Turns,Cab.StallAge,Cab.Quiet,
  Cab.Listen>0?TEXT(", it listens"):TEXT(""),bHeldKey?TEXT(", the key held"):TEXT(""));
}

// ---------------------------------------------------------------- in the game
void FS47WeekCase::Think(const TCHAR* Text,float Secs,double After)
{
 FDueThought D;D.At=Paced+FMath::Max(0.,After);D.Text=Text;D.Secs=Secs;
 ThoughtsDue.Add(D);
}

void FS47WeekCase::StepThoughts(double Dt)
{
 if(!M->UI)return;
 // The two at the door: once something has answered a knock, or after a while close to it without one.
 if(S.Phase==EPhase::Phase1&&!bThoughtLocked)
 {
  double ToDoor=1e4;
  if(M->Player&&!(M->World&&M->World->bDriving)){const FVector D=ShedDoor();ToDoor=FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(D.X,D.Z));}
  NearDoorFor=ToDoor<7.?NearDoorFor+Dt:0.;
  if((DoorObj&&DoorObj->Answers()>0)||NearDoorFor>=35.)
  {
   bThoughtLocked=true;
   Think(ThoughtLocked(),4.f,DoorObj&&DoorObj->Answers()>0?2.5:0.);
   Think(ThoughtLockedIn(),4.5f,DoorObj&&DoorObj->Answers()>0?9.:6.5);
  }
 }
 for(int32 I=0;I<ThoughtsDue.Num();)
 {
  if(Paced<ThoughtsDue[I].At){++I;continue;}
  M->UI->Thought(ThoughtsDue[I].Text,ThoughtsDue[I].Secs);
  UE_LOG(LogS47WeekCab,Display,TEXT("S47: week case: thought: %s"),*ThoughtsDue[I].Text);
  ThoughtsDue.RemoveAt(I);
 }
}

void FS47WeekCase::StepCab(double Dt)
{
 using namespace S47WeekCabLocal;
 auto* F=M->Freeform.Get();US47World* W=M->World;
 if(!W||!HoldsGround()){StepSweep(Dt,false);return;}
 const bool bHeld=F->Held()||M->ModalOpen();
 const double D=bHeld?0.:Dt*S47Week::Pace(M);
 const bool bInCab=W->bDriving;
 const FS47DriveController* Dr=bInCab?W->CabDrive():nullptr;
 const FVector Door=ShedDoor(),Yard=CabYard();
 const FVector Truck=Dr?Dr->Pos:(M->Player?M->Player->Pos:Door);
 // The key's turns, read off the stall (a fresh stall counts from nought again).
 const int32 Turns=W->Trouble.Kind()==ES47Trouble::Stall?W->Trouble.Turns:0;
 const bool bTurned=bInCab&&Turns>TurnsSeen;
 TurnsSeen=Turns;
 SinceTurn+=D;
 FCabIn In;
 In.Phase=S.Phase;In.Dt=D;In.bInCab=bInCab;In.bTurned=bTurned;
 In.bAtYard=Dr&&FVector2D::Distance(FVector2D(Dr->Pos.X,Dr->Pos.Z),FVector2D(Yard.X,Yard.Z))<CabYardReach;
 In.bNoise=M->FieldRadio.IsValid()&&M->FieldRadio->LoudNow();
 ReadSearcher(Truck,In.bSearcherNear,In.SearcherDist);
 const FCabOut O=StepCab(Cab,In);
 if(!O.Why.IsEmpty())UE_LOG(LogS47WeekCab,Display,TEXT("S47: week case: cab: %s (%s)"),*O.Why,*CabText());
 if(O.bStallNow)
 {
  FS47TroubleOpts K;K.bKey=true;K.Tries=Never;
  W->Trouble.Start(ES47Trouble::Stall,K);HoldStall();bCatchPending=false;TurnsSeen=0;
  W->bKeyStall=false;
 }
 // Out of the truck before it caught: dead again when she gets back in (and the rules hold it).
 if(Cab.bStalled&&!Cab.bCaught&&!bInCab&&!W->bKeyStall){W->bKeyStall=true;bArmedKeyStall=true;}
 if(bInCab&&W->Trouble.Kind()==ES47Trouble::Stall&&Cab.bStalled)
 {
  if(O.bCatch||bDevCatch){W->Trouble.CatchOn(W->Trouble.Cranking()?W->Trouble.Turns:W->Trouble.Turns+1);bCatchPending=true;Cab.bCaught=true;bDevCatch=false;}
  else if(!bCatchPending)W->Trouble.CatchOn(Never);
  if(O.bHoldKey!=bHeldKey){W->Trouble.HoldKey(O.bHoldKey);bHeldKey=O.bHoldKey;if(bHeldKey&&!bThoughtHeld){bThoughtHeld=true;Think(ThoughtHeld(),4.f,1.5);}}
 }
 if(bCatchPending&&bInCab&&W->Trouble.Kind()==ES47Trouble::None){bCatchPending=false;UE_LOG(LogS47WeekCab,Display,TEXT("S47: week case: the truck caught (%s)"),*CabText());}
 if(O.bHeard)
 {
  // The starter is heard: a noise at the truck for whatever listens (S47Noise: the small ones, a visit's attention), and the place
  // the searcher goes to first.
  S47Noise::Make(Truck,1.6,TEXT("a starter cranking"));
  SoundAt=Truck;SoundTime=FPlatformTime::Seconds();SinceTurn=0;
 }
 if(O.bShake)W->CabShake=FMath::Max(W->CabShake,double(CvShake.GetValueOnGameThread()));
 StepSweep(D,O.bSweep);
 if(O.bStuck)Think(ThoughtStuck(),5.f,.5);
}

// Where the searcher is from the truck: patch 4's, or the reserve.
void FS47WeekCase::ReadSearcher(const FVector& Truck,bool& bNear,double& Dist) const
{
 bNear=false;Dist=1e4;
 if(SearcherFeed&&SearcherFeed(Dist,bNear))return;
 const FVector Door=ShedDoor();
 ReserveSearcher(S.Phase,FVector2D::Distance(FVector2D(Truck.X,Truck.Z),FVector2D(Door.X,Door.Z)),SinceTurn,bNear,Dist);
}

// The light through the windshield: something standing a few metres off the bonnet, its light going over the cab and back.
void FS47WeekCase::StepSweep(double Dt,bool bOn)
{
 using namespace S47WeekCabLocal;
 USpotLightComponent* L=Sweep.Get();
 if(!bOn)
 {
  if(L&&LiveEnough(M)&&L->IsVisible()){L->SetIntensity(0.f);L->SetVisibility(false);}
  SweepT=0;return;
 }
 const FS47DriveController* Dr=M->World?M->World->CabDrive():nullptr;
 if(!Dr||!LiveEnough(M)||!IsValid(M->Web))return;
 if(!L||L->GetOwner()!=M->Web.Get())
 {
  AS47Web* Web=M->Web.Get();
  L=NewObject<USpotLightComponent>(Web,TEXT("S47WeekCabSweep"));
  L->SetupAttachment(Web->GetRootComponent());L->SetMobility(EComponentMobility::Movable);
  L->SetIntensityUnits(ELightUnits::Lumens);L->SetIntensity(0.f);L->SetLightColor(Cold);
  L->SetAttenuationRadius(2600.f);L->SetCastShadows(true);L->SetCastVolumetricShadow(true);L->SetVolumetricScatteringIntensity(2.f);
  L->RegisterComponent();Sweep=L;
 }
 SweepT+=Dt;
 const double H=Dr->Heading;
 const FVector Fwd(-FMath::Sin(H),0,-FMath::Cos(H)),Right(FMath::Cos(H),0,-FMath::Sin(H));
 // It stands off the front and to one side, and the beam goes across the glass and back, slowly, never quite still.
 const double Side=FMath::Sin(SweepT*.55)*2.6+FMath::Sin(SweepT*1.7)*.4;
 const FVector From=Dr->Pos+Fwd*6.5+Right*(1.2+.6*FMath::Sin(SweepT*.31))+FVector(0,2.3,0);
 const FVector Aim=Dr->Pos+Fwd*.4+Right*Side+FVector(0,1.25,0);
 L->SetWorldLocation(S47Three::ToUE(From));
 L->SetWorldRotation((S47Three::ToUE(Aim)-S47Three::ToUE(From)).Rotation());
 L->SetOuterConeAngle(CvSweepCone.GetValueOnGameThread());L->SetInnerConeAngle(CvSweepCone.GetValueOnGameThread()*.45f);
 L->SetIntensity(CvSweepLumens.GetValueOnGameThread()*float(FMath::Clamp(SweepT/.8,0.,1.)));
 L->SetVisibility(true);
}

// Every way out (Clean): the next turn catches, the stall is let go, the shake and the light end; the armed stall on getting in is
// taken back.
void FS47WeekCase::StopCab()
{
 const bool bWorld=S47WeekCabLocal::LiveEnough(M)&&IsValid(M->World);
 if(bWorld)
 {
  US47World* W=M->World;
  if(W->Trouble.Kind()==ES47Trouble::Stall&&(Cab.bStalled||bStall)){W->Trouble.HoldKey(false);W->Trouble.CatchOn(W->Trouble.Cranking()?W->Trouble.Turns:W->Trouble.Turns+1);}
  if(bArmedKeyStall)W->bKeyStall=false;
  W->CabShake=0;
 }
 bArmedKeyStall=false;bHeldKey=false;bCatchPending=false;bDevCatch=false;
 if(USpotLightComponent* L=Sweep.Get())
 {
  if(S47WeekCabLocal::LiveEnough(M)){L->SetIntensity(0.f);L->SetVisibility(false);}
 }
 SweepT=0;
}

// ---------------------------------------------------------------- F10
bool FS47WeekCase::DevStall(FString& Why)
{
 if(!HoldsGround()){Why=TEXT("the case is not on the ground at the shed");return false;}
 US47World* W=M?M->World:nullptr;
 if(!W||!W->bDriving){Why=TEXT("not in the truck");return false;}
 if(Cab.bStalled&&!Cab.bCaught&&W->Trouble.Kind()==ES47Trouble::Stall){Why=TEXT("it is dead already");return false;}
 Cab=FCab();Cab.bStalled=true;
 FS47TroubleOpts K;K.bKey=true;K.Tries=S47WeekCabLocal::Never;
 W->Trouble.Start(ES47Trouble::Stall,K);HoldStall();bCatchPending=false;TurnsSeen=0;
 UE_LOG(LogS47WeekCab,Display,TEXT("S47: week case: F10 the truck dies on the yard"));
 return true;
}
bool FS47WeekCase::DevTurnKey(FString& Why)
{
 US47World* W=M?M->World:nullptr;
 if(!W||!W->bDriving){Why=TEXT("not in the truck");return false;}
 if(!W->Trouble.TurnKey()){Why=bHeldKey?TEXT("the key does nothing: it is at the cab"):TEXT("the engine does not want the key now");return false;}
 return true;
}
bool FS47WeekCase::DevCatch(FString& Why)
{
 US47World* W=M?M->World:nullptr;
 if(!W||!W->bDriving||W->Trouble.Kind()!=ES47Trouble::Stall){Why=TEXT("no dead truck to start");return false;}
 if(bHeldKey){Why=TEXT("not while it is at the cab");return false;}
 bDevCatch=true;if(!Cab.bStalled){Cab=FCab();Cab.bStalled=true;HoldStall();}
 if(!W->Trouble.WantsKey()&&!W->Trouble.Cranking()){Why=TEXT("the engine does not want the key now");return false;}
 if(W->Trouble.WantsKey())W->Trouble.TurnKey();
 return true;
}
