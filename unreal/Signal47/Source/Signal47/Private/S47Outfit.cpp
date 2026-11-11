#include "S47Outfit.h"
#include "S47Audio.h"
#include "S47Drive.h"
#include "S47Flashlight.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Happenings.h"
#include "S47Keys.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47Openings.h"
#include "S47Places.h"
#include "S47ScriptPlayer.h"
#include "S47Tape.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Outfit, Log, All);

namespace S47OutfitLocal
{
 constexpr double Mph=2.23694;
 // The two surfaces the F10 line and the self-test compare on: the county blacktop and a dirt track (S47RoadArea's own values).
 FS47Surface Blacktop(){FS47Surface S;S.Kind=ES47SurfaceKind::Asphalt;S.Grip=1;S.Top=30;S.Rough=.05;return S;}
 FS47Surface Track(){FS47Surface S;S.Kind=ES47SurfaceKind::Dirt;S.Grip=.55;S.Top=10;S.Rough=.8;return S;}
 bool NightHour(double Clock){const double H=FMath::Fmod(Clock,86400.)/3600.;return H>=19||H<6;}
}

FS47TruckTune FS47Outfit::TuneOf(bool bEngine,bool bTires,bool bTank,bool bRack)
{
 FS47TruckTune T;
 if(bEngine){T.Pull=1.3;T.Top=1.15;T.Burn=1.1;}
 // tires: the dirt's grip .55 becomes about .69 and its top 10 m/s 13; the sand digs in half as fast
 if(bTires){T.LooseGrip=1.25;T.LooseTop=1.3;T.Bog=.5;}
 T.Tank=bTank?BigTank:StockTank;
 T.Load=bRack?RackLoad:StockLoad;
 return T;
}

FS47TruckTune FS47Outfit::TuneFor(const FS47FreeformSession* F)
{
 if(!F)return FS47TruckTune();
 return TuneOf(F->HasKit(TEXT("truck_engine")),F->HasKit(TEXT("truck_tires")),F->HasKit(TEXT("truck_tank")),F->HasKit(TEXT("truck_rack")));
}

FString FS47Outfit::RdfLine(double Metres,const TCHAR* Way)
{
 const TCHAR* How=Metres<800?TEXT("loud, close"):Metres<1800?TEXT("clear"):TEXT("faint, a long way off");
 return FString::Printf(TEXT("The direction finder points %s: something is transmitting out there, %s. No way to tell what."),Way?Way:TEXT("?"),How); // D45-20261110: plain words
}

FString FS47Outfit::Compare(const FS47TruckTune& Now)
{
 using namespace S47OutfitLocal;
 const FS47TruckTune Stock;
 double TopA0=0,SecA0=0,TopA1=0,SecA1=0,TopD0=0,SecD0=0,TopD1=0,SecD1=0;
 FS47DriveController::StraightRun(Blacktop(),Stock.Pull,Stock.Top,Stock.LooseGrip,Stock.LooseTop,40/Mph,TopA0,SecA0);
 FS47DriveController::StraightRun(Blacktop(),Now.Pull,Now.Top,Now.LooseGrip,Now.LooseTop,40/Mph,TopA1,SecA1);
 FS47DriveController::StraightRun(Track(),Stock.Pull,Stock.Top,Stock.LooseGrip,Stock.LooseTop,20/Mph,TopD0,SecD0);
 FS47DriveController::StraightRun(Track(),Now.Pull,Now.Top,Now.LooseGrip,Now.LooseTop,20/Mph,TopD1,SecD1);
 const auto Sec=[](double S){return S>1e8?FString(TEXT("never")):FString::Printf(TEXT("%.1f s"),S);};
 return FString::Printf(TEXT("BLACKTOP top %.0f mph (stock %.0f), 0-40 mph %s (stock %s). DIRT top %.0f mph (stock %.0f), 0-20 mph %s (stock %s). TANK %.0f units, %.0f km (stock 4, 48 km). PARCEL %d items (stock %d). SAND digs in at x%.1f."),
  TopA1*Mph,TopA0*Mph,*Sec(SecA1),*Sec(SecA0),TopD1*Mph,TopD0*Mph,*Sec(SecD1),*Sec(SecD0),Now.Tank,Now.Tank*12/Now.Burn,Now.Load,int32(StockLoad),Now.Bog);
}

void FS47Outfit::Begin(const S47J::FObj& Saved)
{
 // CRIT6-20261109: the night last rolled survives a Continue; a new game (nothing saved) starts with none.
 RolledNight=FMath::Max(0,S47J::Int(Saved,TEXT("rolled"),0));RdfIn=5;
}

S47J::FObj FS47Outfit::Save() const
{
 const S47J::FObj O=S47J::New();
 if(RolledNight>0)S47J::Set(O,TEXT("rolled"),RolledNight);
 return O;
}

void FS47Outfit::Stop()
{
 // CRIT6-20261109: as the other Stops: at exit or in the UObject purge the world (and the cab in it) may already be gone, so it is
 // asked before M->World is read at all.
 if(!M||IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)||!IsValid(M->World))return;
 if(FS47DriveController* D=M->World->CabDrive()){D->PullScale=D->TopScale=D->LooseGripScale=D->LooseTopScale=1;}
}

void FS47Outfit::Step(double Dt)
{
 using namespace S47OutfitLocal;
 if(!M||!M->Freeform||!M->World||!FMath::IsFinite(Dt)||Dt<=0)return;
 FS47FreeformSession& F=*M->Freeform;
 const FS47TruckTune T=TuneFor(&F);
 FS47DriveController* D=M->World->CabDrive();
 if(D){D->PullScale=T.Pull;D->TopScale=T.Top;D->LooseGripScale=T.LooseGrip;D->LooseTopScale=T.LooseTop;}
 if(!M->Omen||!M->Player||!M->Web)return;
 // ---- the direction finder: in the cab at night, every so often, toward the strongest thing keying the band within its reach ----
 if(F.HasKit(TEXT("rdf"))&&M->World->bDriving&&NightHour(F.Clock()))
 {
  RdfIn-=Dt;
  if(RdfIn<=0)
  {
   RdfIn=RdfEvery;
   const FVector P=M->Player->Pos;const FS47OmenSource* Best=nullptr;double BestM=RdfReach;
   for(const FS47OmenSource& S:M->Omen->Sources())
   {
    if(S.bResidual||S.Severity<=0)continue;
    const double Metres=FVector2D(S.At.X-P.X,S.At.Z-P.Z).Size();
    if(Metres<BestM){BestM=Metres;Best=&S;}
   }
   double Hx=0,Hz=0,Tx=0,Tz=0;const FString Area=M->Web->Area();
   if(Best&&S47OpenLand::RoadOf(Area,P.X,P.Z,Hx,Hz)&&S47OpenLand::RoadOf(Area,Best->At.X,Best->At.Z,Tx,Tz))
   {
    const TCHAR* Way=FS47Omen::Way(FS47Omen::BearingOf(FVector2D(Hx,Hz),FVector2D(Tx,Tz)));
    // LOOP-1-20261109 (HELHETLIG_LOKKE step 1): the null is a line on the map from where the truck stands, not only a word, so that
    // two taken from different places cross (FS47Places::AddCarFix, Cross). A loop's null is a few degrees wide either way.
    const double Az=FS47Places::AzimuthOf(Hx,Hz,Tx,Tz)+FMath::FRandRange(-3.,3.);
    const bool bLine=F.Places&&F.Places->AddCarFix(Hx,Hz,Az,Best->Id,F.Clock(),F.Day());
    if(M->Audio){M->Audio->Beep(700.f,.18f,.05f);M->Audio->Beep(700.f,.18f,.03f,.3f);}
    if(M->UI)M->UI->Toast(RdfLine(BestM,Way)+(bLine?FString::Printf(TEXT(" Penciled in from here on your map (%s)."),*S47Keys::MapSaid()):FString()),6);
    UE_LOG(LogS47Outfit,Display,TEXT("S47: rdf: %s at %.0f m, toward the %s%s"),*Best->Id,BestM,Way,bLine?TEXT(", a new line on the map"):TEXT(""));
   }
  }
 }
 else RdfIn=FMath::Min(RdfIn,5.);
 // ---- gear gives out close to something: the flashlight's cell cooks, or the recorder's heads go ----
 if(!F.RulesLive())return;
 const double Level=M->Omen->Level();
 const int32 Night=FS47FreeformSession::NightOf(F.Clock());
 const bool bLamp=M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn&&M->Night->Flashlight->bDrains;
 const bool bTape=F.Tape&&F.Tape->Recording();
 // CRIT6-20261109: the big night is a presence, never a danger, and a director script is authored: neither breaks gear, and the
 // night's one roll is left for later.
 const bool bAuthored=(F.Happenings&&F.Happenings->BigLive())||FS47ScriptPlayer::Busy(M);
 if(RollsNow(Level,Night,RolledNight,bLamp||bTape,bAuthored))
 {
  const float Roll=FMath::FRand();
  const bool bBreak=BreaksNow(Level,Night,RolledNight,Roll);
  RolledNight=Night;
  UE_LOG(LogS47Outfit,Display,TEXT("S47: gear near something (level %.2f): roll %.2f, %s"),Level,Roll,bBreak?TEXT("gives out"):TEXT("holds"));
  if(!bBreak)return;
  if(bLamp)
  {
   M->Night->Flashlight->SetBattery(0);M->Night->Flashlight->bOn=false;
   F.BreakGear(TEXT("flashlight"),4);
   // D45-20261110: plain words for where it charges and where a fresh cell is sold (the store was nowhere in particular)
   if(M->UI)M->UI->Toast(TEXT("The flashlight goes hot in your hand and dies. The cell is burnt out and the contacts with it. The repair, $4, goes on the next sheet. ")+(F.Field?F.Field->FlashlightHelp():FS47FreeformWorld::FlashlightHelpFor(false)),7);
  }
  else
  {
   F.BreakGear(TEXT("field recorder"),6);
   if(M->UI)M->UI->Toast(TEXT("The recorder's motor drags and whines. The heads will want demagnetizing: $6 on the next sheet."),6);
  }
 }
}

bool FS47Outfit::SelfTest(TArray<FString>& Lines)
{
 using namespace S47OutfitLocal;
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s outfit: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 const FS47TruckTune Stock=TuneOf(false,false,false,false),Engine=TuneOf(true,false,false,false),Tires=TuneOf(false,true,false,false),All=TuneOf(true,true,true,true);
 double TopA0,SecA0,TopA1,SecA1,TopD0,SecD0,TopD1,SecD1,TopDT,SecDT;
 FS47DriveController::StraightRun(Blacktop(),1,1,1,1,40/Mph,TopA0,SecA0);
 FS47DriveController::StraightRun(Blacktop(),Engine.Pull,Engine.Top,Engine.LooseGrip,Engine.LooseTop,40/Mph,TopA1,SecA1);
 FS47DriveController::StraightRun(Track(),1,1,1,1,20/Mph,TopD0,SecD0);
 FS47DriveController::StraightRun(Track(),Engine.Pull,Engine.Top,Engine.LooseGrip,Engine.LooseTop,20/Mph,TopD1,SecD1);
 FS47DriveController::StraightRun(Track(),Tires.Pull,Tires.Top,Tires.LooseGrip,Tires.LooseTop,20/Mph,TopDT,SecDT);
 Check(TopA0*Mph>55&&TopA0*Mph<75&&SecA0<20,FString::Printf(TEXT("stock truck: %.0f mph on the blacktop, 0-40 in %.1f s"),TopA0*Mph,SecA0));
 Check(TopA1>TopA0*1.08&&SecA1<SecA0*.85,FString::Printf(TEXT("the engine rebuild: %.0f mph (was %.0f), 0-40 in %.1f s (was %.1f)"),TopA1*Mph,TopA0*Mph,SecA1,SecA0));
 Check(TopD1>TopD0&&SecD1<SecD0,FString::Printf(TEXT("the engine rebuild on dirt: %.0f mph (was %.0f)"),TopD1*Mph,TopD0*Mph));
 Check(TopDT>TopD0*1.15&&SecDT<SecD0,FString::Printf(TEXT("the tires on dirt: %.0f mph (was %.0f), 0-20 in %.1f s (was %.1f)"),TopDT*Mph,TopD0*Mph,SecDT,SecD0));
 double TopAT,SecAT;FS47DriveController::StraightRun(Blacktop(),Tires.Pull,Tires.Top,Tires.LooseGrip,Tires.LooseTop,40/Mph,TopAT,SecAT);
 Check(FMath::IsNearlyEqual(TopAT,TopA0,1e-6)&&FMath::IsNearlyEqual(SecAT,SecA0,1e-6),TEXT("the tires change nothing on the blacktop"));
 Check(Stock.Tank==4&&All.Tank==6&&All.Tank*12/All.Burn>60&&Stock.Load==8&&All.Load==20&&All.Bog<1&&Stock.Bog==1,TEXT("the tank (four units to six), the rack (eight items to twenty) and the sand"));
 Check(TuneFor(nullptr).Pull==1&&TuneFor(nullptr).Tank==4,TEXT("no session, a stock truck"));
 const FS47Surface Dirt=FS47DriveController::Tuned(Track(),1,1.25,1.3),Black=FS47DriveController::Tuned(Blacktop(),1.15,1.25,1.3);
 Check(Dirt.Grip<=1&&Dirt.Grip>.55&&FMath::IsNearlyEqual(Black.Grip,1.)&&FMath::IsNearlyEqual(Black.Top,34.5,1e-6),TEXT("tuned surfaces: grip never over 1, the blacktop's top by the engine only"));
 Check(BreaksNow(.6,3,2,.1f)&&!BreaksNow(.6,3,3,.1f)&&!BreaksNow(.3,3,2,.1f)&&!BreaksNow(.6,3,2,.9f),TEXT("gear gives out only near something dangerous, at most once a night, not always"));
 Check(RollsNow(.64,2,0,true,false)&&!RollsNow(.64,2,0,true,true)&&!RollsNow(.64,2,0,false,false)&&!RollsNow(.64,2,2,true,false),TEXT("CRIT6: no roll in the big night or under a script (the night is kept for later), none with the gear off, one a night"));
 {
  FS47Outfit A(nullptr);const S47J::FObj None=S47J::New();A.Begin(None);const bool bFresh=A.LastRolledNight()==0&&A.Save()->Values.Num()==0;
  const S47J::FObj Was=S47J::New();S47J::Set(Was,TEXT("rolled"),3);A.Begin(S47J::Read(S47J::Write(Was)));
  FS47Outfit B(nullptr);B.Begin(S47J::Read(S47J::Write(A.Save())));
  Check(bFresh&&A.LastRolledNight()==3&&B.LastRolledNight()==3&&!RollsNow(.64,3,B.LastRolledNight(),true,false),TEXT("CRIT6: the night last rolled is saved and read back, and a new game starts with none"));
 }
 Check(RdfLine(500,TEXT("north")).Contains(TEXT("north"))&&RdfLine(2500,TEXT("east")).Contains(TEXT("faint")),TEXT("the direction finder says which way and how strong, never what"));
 for(const TCHAR* Id:{TEXT("truck_tires"),TEXT("truck_rack"),TEXT("truck_tank"),TEXT("truck_engine"),TEXT("field_recorder"),TEXT("night_camera"),TEXT("rdf")})
  Check(FS47Openings::Def(Id)!=nullptr,FString::Printf(TEXT("%s is on the stores list"),Id));
 return OK;
}
