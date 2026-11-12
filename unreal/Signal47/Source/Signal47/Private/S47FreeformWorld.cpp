#include "S47FreeformWorld.h"
#include "S47Townsfolk.h"
#include "S47Mail.h"
#include "S47FieldLife.h"
#include "S47Legs.h"
#include "S47OpenLand.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47Observing.h"
#include "S47Happenings.h"
#include "S47FreeformCarry.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47UI.h"
#include "S47Kit.h"
#include "S47Three.h"
#include "S47GroundCover.h"
#include "S47Substation.h"
#include "S47Audio.h"
#include "S47Night.h"
#include "S47Flashlight.h"
#include "S47Props.h"
#include "S47Glow.h"
#include "S47Album.h"
#include "S47SaroNight.h"
#include "S47SaroPower.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CoreGlobals.h"
#include "S47Outfit.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/GarbageCollection.h"

namespace
{
 bool LoggerOriginalComplete(const FS47FreeformSession::FJob& J)
 {
  if(J.Source<0||J.Source>=FS47Observing::Runs().Num())return false;
  const double Required=J.bGenerated?J.Card.Seconds:FS47Observing::Runs()[J.Source].Seconds;
  return !J.bLegacyTrace&&J.Stage>=FS47FreeformSession::EJobStage::Recorded&&!J.OriginalPage.IsEmpty()&&!J.OriginalStamp.IsEmpty()&&FMath::IsFinite(J.Elapsed)&&FMath::IsFinite(Required)&&Required>0&&J.Elapsed>=Required-.001;
 }

 // EASTWING-01: the service bench stands inside the east wing's calibration / repair room,
 // against the partition to parts storage (S47EastWing; the outdoor bench was retired).
 // EASTWING reference pass: the stations are part of the casework runs (Tom 4 Nov).
 // Calibration: west end of the 2.4 m south-wall run, front faces north. Sample: lab run under the north windows.
 const FVector BenchAt(28.95,0.0,4.925);constexpr double CaseTop=.90;
 const FVector SampleAt(23.0,0,-4.925),DepotAt(28.8,0,-3.8),DeliveryAt(32.6,0,-4.15);
 const FVector FilmAt(18.55,.98,-2.35); // dry photo-lab archive bench; clear of camera and reference sheet
 const FVector OldBenchAt(24.0,-.57,6.2);
 // Only packages supported by the removed outdoor table move. Ground/custom drops stay put.
 bool OnRetiredBench(const FVector& P)
 {return FMath::Abs(P.X-OldBenchAt.X)<=.93&&FMath::Abs(P.Z-OldBenchAt.Z)<=.41&&P.Y>.12&&P.Y<.95;}
 bool ReadPoint(const S47J::FObj& O,const TCHAR* Key,FVector& P)
 {const auto A=S47J::Nums(O,Key);if(A.Num()!=3||!FMath::IsFinite(A[0])||!FMath::IsFinite(A[1])||!FMath::IsFinite(A[2]))return false;P=FVector(A[0],A[1],A[2]);return true;}
 S47J::FObj MigrateBenchParcel(const S47J::FObj& Saved,FVector& Legacy,int32 State)
 {
  const auto Original=S47J::Obj(Saved,TEXT("carry"));
  if(State!=4||S47J::Int(Saved,TEXT("eastWingAnchors"))>=1)return Original;
  const FVector Delta=DeliveryAt-OldBenchAt;
  if(Original)
  {
   FVector Position,Support;
   if(S47J::Str(Original,TEXT("scene"))!=TEXT("saro")||!ReadPoint(Original,TEXT("position"),Position)||!OnRetiredBench(Position)
    ||!ReadPoint(Original,TEXT("support"),Support)||FMath::Abs(Support.X-OldBenchAt.X)>.93||FMath::Abs(Support.Z-OldBenchAt.Z)>.41||Support.Y<.1||Support.Y>.6)return Original;
   auto Migrated=S47J::Read(S47J::Write(Original));Position+=Delta;Support+=Delta;Legacy=Position;
   S47J::Set(Migrated,TEXT("position"),TArray<double>{Position.X,Position.Y,Position.Z});
   S47J::Set(Migrated,TEXT("support"),TArray<double>{Support.X,Support.Y,Support.Z});return Migrated;
  }
  if(OnRetiredBench(Legacy))Legacy+=Delta;
  return Original;
 }
 // SARO-GEN-RAMP-20261109 (Tom's test of d37): started and fuelled at the standby generator itself, outside in the pocket behind the
 // control room (Grounds.ts things(): the housing at x 4.3, z 8.4 on the ground at -0.6), at the black control box on its east face
 // (0.25 x 0.35 x 0.18 at x 4.9, y 0.45, z 7.6). The brown box on the control room's east wall (5.89, 1.35, .65) is gone.
 const FVector GeneratorAt(4.9,.45,7.6);
 const FVector GeneratorBox(.27,.37,.20); // the control box's own, a centimetre proud of it all round: one dark box, a bigger mark to aim at
 // Original S-03 CABLE JUNCTION front, unchanged ground placement and atlas.
 FVector ServiceAt() { return FVector(4.647,-.01,-39.4); }
 const TCHAR* Items[] = { TEXT("fuse"), TEXT("relay"), TEXT("donor"), TEXT("fuel"), TEXT("film"), TEXT("recorder_donor"), TEXT("cell"), TEXT("foil"), TEXT("drive_motor"), TEXT("water"), TEXT("spare"), TEXT("tubes") }; // N1110: tubes, a pair of ceiling fluorescents for the control room (FS47FreeformSession::MendTubes)
 // TOWN-USE-20261108: what the Feed & Seed sells besides the counter's own supplies (S47TownUse.cpp); carried and unpacked the same way.
 // rope: the come-along digs the truck out quicker (FS47TruckCare); wire: a longer trip line (FS47TripCamera); salt and feed: set out
 // at a place on the land, they say in the morning what came to them (FS47TownUse).
 const TCHAR* TownItems[] = { TEXT("rope"), TEXT("wire"), TEXT("salt"), TEXT("feed") };
 TArray<const TCHAR*> AllItems() { TArray<const TCHAR*> A; for(const TCHAR* I:Items)A.Add(I); for(const TCHAR* I:TownItems)A.Add(I); return A; }
 int32 Price(const FString& Id) { return Id==TEXT("water")?1:Id==TEXT("spare")?15:Id==TEXT("fuse")?5:Id==TEXT("relay")?25:Id==TEXT("donor")?15:(Id==TEXT("fuel")||Id==TEXT("film"))?10:Id==TEXT("recorder_donor")?20:Id==TEXT("cell")?4:Id==TEXT("foil")?3:Id==TEXT("drive_motor")?35:Id==TEXT("rope")?12:Id==TEXT("wire")?4:Id==TEXT("salt")?3:Id==TEXT("feed")?6:Id==TEXT("tubes")?4:0; }
 // D46-20261110 (D46-10): the stores list is where a new operator first reads that a trip camera can be built from a donor at the bench.
 FString ItemTitle(const FString& Id) { return Id==TEXT("water")?TEXT("can of radiator water"):Id==TEXT("spare")?TEXT("spare wheel"):Id==TEXT("drive_motor")?TEXT("reconditioned antenna drive motor"):Id==TEXT("splice_kit")?TEXT("feed-cable splice"):Id==TEXT("recorder_donor")?TEXT("used field recorder, untested / strip at bench for a logger or a trip camera"):Id==TEXT("cell")?TEXT("dry cell"):Id==TEXT("foil")?TEXT("shielding foil"):Id==TEXT("fuse")?TEXT("control fuse"):Id==TEXT("relay")?TEXT("drive relay"):Id==TEXT("donor")?TEXT("used controller / relay donor"):Id==TEXT("film")?TEXT("photographic film / 36 exposures"):Id==TEXT("fieldlogger")?TEXT("SARO loan field logger / fieldlogger01"):Id==TEXT("service_case")?TEXT("reusable SARO service case"):Id==TEXT("rope")?TEXT("tow rope and come-along"):Id==TEXT("wire")?TEXT("roll of smooth fence wire"):Id==TEXT("salt")?TEXT("salt lick block"):Id==TEXT("feed")?TEXT("sack of range cubes"):Id==TEXT("tubes")?TEXT("fluorescent tubes, pair / for the control room ceiling"):TEXT("six-hour fuel can"); }
 bool ValidItem(const FString& Id) { return Price(Id)>0 || Id==TEXT("splice_kit") || Id==TEXT("battery") || Id==TEXT("fragment") || Id==TEXT("archived_fragment"); }
 // Cash is integer-valued: each completed metering interval delivers up to
 // one quarter of the shared tank unit and costs exactly one operating dollar.
 // M7 (Tom, 9 November): Cap is the truck's tank (four units, six with the auxiliary tank, FS47Outfit) and Per what a dollar buys this
 // week (FuelPerDollar: the price goes up a little with the weeks).
 double PumpIncrement(double Tank,double Remaining,double Cap=4.,double Per=.25) { return FMath::Max(0.,FMath::Min(Per,FMath::Min(Cap-Tank,Remaining))); }
 double DayClock(double T) { return FMath::Fmod(FMath::Fmod(T,86400.)+86400.,86400.); }
 // M7 (C): the pump runs on to a full tank, unless the nozzle is put back or the budget runs out first (StepPump).
 double PumpVolumeFor(double Tank,double Cap=4.) { return FMath::Max(0.,Cap-Tank); }
 // M7 (C, TODO "Henting av bilen skal koste"): the county's roadside return. A dry tank is the cheap call-out (no hook-up, a can and
 // a ride), a truck in trouble the full tow; both take the tow's 45 to 90 minutes by how far out it is, and after dark the one wrecker
 // on duty is three quarters of an hour longer coming. Nobody is sent while something is close (the Director's RestBlock, as a wait).
 constexpr int32 EmptyTankFee=20;
 constexpr double NightTowWait=45;
 struct FRoadsideTerms { int32 Fee=0; double Minutes=0; bool bRefused=false; };
 FRoadsideTerms RoadsideTermsFor(bool bTrouble,int32 TowFee,double TowMinutes,double Hour,const FString& Block)
 {
  FRoadsideTerms T;T.bRefused=!Block.IsEmpty();
  T.Fee=bTrouble?FMath::Max(0,TowFee):FMath::Max(EmptyTankFee,TowFee-15);
  T.Minutes=FMath::Clamp(TowMinutes,45.,90.)+(Hour>=21||Hour<6?NightTowWait:0.);
  return T;
 }
}

FS47FreeformWorld::FS47FreeformWorld(US47Main* InM) : M(InM), Carry(MakeShared<FS47FreeformCarry>(InM)) { for(int32 I=0;I<27;++I){PilotOf[I]=-1;PilotShown[I]=-1;PilotSize[I]=.14f;PilotBlink[I]=FVector2D::ZeroVector;} }
FS47FreeformWorld::~FS47FreeformWorld() { Stop(); Life.Reset(); }
bool FS47FreeformWorld::Ready() const { return bActive && M && M->Freeform && M->Freeform->Active() && M->Web && M->Player; }
int32 FS47FreeformWorld::InventoryCount(const FString& Id) const { return Stock.FindRef(Id); }
void FS47FreeformWorld::AddItem(const FString& Id,int32 Count) { if(ValidItem(Id)&&Count>0) { Stock.FindOrAdd(Id)=FMath::Clamp(Stock.FindRef(Id)+Count,0,99); Change(); } }
bool FS47FreeformWorld::Consume(const FString& Id) { int32* N=Stock.Find(Id); if(!N||*N<=0)return false;--*N;Change();return true; }
namespace
{
 bool ValidServiceDish(const FString& Id)
 {
  for(int32 N=1;N<=27;++N)if(Id==FString::Printf(TEXT("S-%02d"),N))return true;
  return false;
 }
 FString OrdinaryServiceDish(int32 Serial)
 {
  // Step ten is coprime to 27: every installed antenna is visited before repeating.
  return FString::Printf(TEXT("S-%02d"),(Serial%27*10+4)%27+1);
 }
}
int32 FS47FreeformWorld::ServiceDishIndex() const { if(M&&M->Web)for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==ServiceTarget)return I;return INDEX_NONE; }
FVector FS47FreeformWorld::ServicePosition() const
{
 if(ServiceTarget==TEXT("S-03"))return ServiceAt();
 if(M&&M->Web)if(const auto* I=M->Web->Item(TEXT("field:")+ServiceTarget))return M->Web->ObjAt(I->Node).WorldPosition();
 return FVector::ZeroVector;
}
FString FS47FreeformWorld::ServiceWords(const FString& Text) const
{
 FString Result=Text.Replace(TEXT("S-03"),*ServiceTarget);
 if(ServiceTarget!=TEXT("S-03"))Result=Result.Replace(TEXT("CABLE JUNCTION"),TEXT("AZ / EL DRIVE"));
 return Result;
}

void FS47FreeformWorld::Begin(const S47J::FObj& O)
{
 Stop(); Logger.Load(S47J::Obj(O,TEXT("fieldlogger")));Crafting.Load(S47J::Obj(O,TEXT("workshop")));bWorkshopResume=false;Wear.Load(S47J::Obj(O,TEXT("arrayWear")));
 // Tests written for the full array keep it; only their own explicit fixture wears it down.
 if(!O&&(!(FParse::Param(FCommandLine::Get(),TEXT("S47FreeformTest"))||FParse::Param(FCommandLine::Get(),TEXT("S47FreeformEvidenceTest")))||FParse::Param(FCommandLine::Get(),TEXT("S47ArrayWearTest"))||FParse::Param(FCommandLine::Get(),TEXT("S47RescueJourneyTest"))))Wear.Start();
bLoggerCalibrating=false;LoggerJob.Empty();LoggerJobElapsed=0; bActive=true;Service=Feeder=LastFaultDay=EmergencyDay=Payload=0;Item.Empty();Contents.Reset();BenchJob.Empty();
 ServiceTarget=TEXT("S-03");OrdinaryServiceSerial=0;
 PowerFaultSerial=EmergencyFuseFault=0;bTrackedPowerFault=false;
 bGenerator=false;GeneratorSeconds=BenchSeconds=FeederSeconds=Settle=ServiceAz=ServiceEl=0;Fuel=3;LastClock=-1;Dropped=FVector::ZeroVector;
 Delivered=DeliveredByTruck=ServiceDeliveries=DonorTests=ServiceTests=0;bParcelWasLoaded=false;
 bPumping=false;PumpRemaining=PumpElapsed=PumpVolume=0;PumpPaid=0;LastPumpDisplay.Empty();
 Stock.Reset();Stock.Add(TEXT("water"),2);Stock.Add(TEXT("spare"),1);Stock.Add(TEXT("fuse"),3);Stock.Add(TEXT("relay"),1);Stock.Add(TEXT("fuel"),2);Stock.Add(TEXT("donor"),0);Stock.Add(TEXT("battery"),0);Stock.Add(TEXT("film"),O?0:1);Stock.Add(TEXT("fragment"),0);Stock.Add(TEXT("archived_fragment"),0);
 if(O && S47J::Int(O,TEXT("v"))==1)
 {
  const FString SavedTarget=S47J::Str(O,TEXT("serviceDish"));if(ValidServiceDish(SavedTarget))ServiceTarget=SavedTarget;
  OrdinaryServiceSerial=FMath::Clamp(S47J::Int(O,TEXT("ordinaryServiceSerial")),0,1000000);
  Service=FMath::Clamp(S47J::Int(O,TEXT("service")),0,9);Feeder=FMath::Clamp(S47J::Int(O,TEXT("feeder")),0,5);
  LastFaultDay=FMath::Max(0,S47J::Int(O,TEXT("faultDay")));EmergencyDay=FMath::Max(0,S47J::Int(O,TEXT("emergencyDay")));
  PowerFaultSerial=FMath::Clamp(S47J::Int(O,TEXT("powerFaultSerial")),0,1000000000);
  EmergencyFuseFault=FMath::Clamp(S47J::Int(O,TEXT("emergencyFuseFault")),0,PowerFaultSerial);
  bTrackedPowerFault=S47J::Bool(O,TEXT("trackedPowerFault"));
  Payload=FMath::Clamp(S47J::Int(O,TEXT("payload")),0,4);Item=S47J::Str(O,TEXT("item"));
  if(Payload>0&&Item!=TEXT("service_case")&&Item!=TEXT("fieldlogger"))
  {
   if(!O->HasField(TEXT("contents")))
   {if(Price(Item)>0)Contents.Add(Item,1);else{Payload=0;Item.Empty();}} // Legacy paid singleton.
   else
   {
    const auto SavedContents=S47J::Obj(O,TEXT("contents"));bool Valid=SavedContents&&SavedContents->Values.Num()>0;
    if(SavedContents)for(const auto& Pair:SavedContents->Values)
    {
     double Count=0;const FString Key(Pair.Key);
     if(Price(Key)<=0||!Pair.Value||!Pair.Value->TryGetNumber(Count)||!FMath::IsFinite(Count)||Count<1||Count>99||FMath::FloorToDouble(Count)!=Count){Valid=false;break;}
     Contents.Add(Key,int32(Count));
    }
    if(!Valid){Contents.Reset();Payload=0;Item.Empty();}
    else if(!Contents.Contains(Item))for(const TCHAR* Key:AllItems())if(Contents.Contains(Key)){Item=Key;break;}
   }
  }
  bParcelWasLoaded=S47J::Bool(O,TEXT("parcelLoaded"),Payload==3);
  Delivered=FMath::Clamp(S47J::Int(O,TEXT("delivered")),0,1000000);DeliveredByTruck=FMath::Clamp(S47J::Int(O,TEXT("cargoDelivered")),0,1000000);
  ServiceDeliveries=FMath::Clamp(S47J::Int(O,TEXT("serviceDelivered")),0,1000000);DonorTests=FMath::Clamp(S47J::Int(O,TEXT("donorTests")),0,1000000);ServiceTests=FMath::Clamp(S47J::Int(O,TEXT("serviceTests")),0,1000000);
  bGenerator=S47J::Bool(O,TEXT("generator"));GeneratorSeconds=FMath::Clamp(S47J::Num(O,TEXT("generatorSeconds")),0.,43200.);
  Fuel=FMath::Clamp(S47J::Num(O,TEXT("fuel"),3),0.,FS47Outfit::BigTank);LastClock=S47J::Num(O,TEXT("lastClock"),-1);
  bPumping=S47J::Bool(O,TEXT("pumping"));PumpRemaining=FMath::Clamp(S47J::Num(O,TEXT("pumpRemaining")),0.,FS47Outfit::BigTank);
  PumpElapsed=FMath::Clamp(S47J::Num(O,TEXT("pumpElapsed")),0.,2.);PumpVolume=FMath::Clamp(S47J::Num(O,TEXT("pumpVolume")),0.,FS47Outfit::BigTank);PumpPaid=FMath::Clamp(S47J::Int(O,TEXT("pumpPaid")),0,40);
  if(PumpRemaining<.00001||Fuel>=FS47Outfit::BigTank-.00001)bPumping=false;
  ServiceAz=S47J::Num(O,TEXT("serviceAz"));ServiceEl=S47J::Num(O,TEXT("serviceEl"));Settle=FMath::Clamp(S47J::Num(O,TEXT("settle")),0.,1.);
  BenchJob=S47J::Str(O,TEXT("benchJob"));if(BenchJob!=TEXT("donor")&&BenchJob!=TEXT("fragment")&&!BenchJob.StartsWith(TEXT("ws:")))BenchJob.Empty();BenchSeconds=FMath::Clamp(S47J::Num(O,TEXT("benchSeconds")),0.,3.);
  FeederSeconds=FMath::Clamp(S47J::Num(O,TEXT("feederSeconds")),0.,3.);
  const auto P=S47J::Nums(O,TEXT("dropped"));if(P.Num()==3 && FMath::IsFinite(P[0])&&FMath::IsFinite(P[1])&&FMath::IsFinite(P[2]))Dropped=FVector(P[0],P[1],P[2]);else if(Payload==4){Payload=1;}
  const auto S=S47J::Obj(O,TEXT("stock"));if(S)for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("donor"),TEXT("fuel"),TEXT("battery"),TEXT("film"),TEXT("fragment"),TEXT("archived_fragment"),TEXT("recorder_donor"),TEXT("cell"),TEXT("foil"),TEXT("drive_motor"),TEXT("water"),TEXT("spare"),TEXT("splice_kit"),TEXT("rope"),TEXT("wire"),TEXT("salt"),TEXT("feed"),TEXT("tubes")})Stock.FindOrAdd(Id)=FMath::Clamp(S47J::Int(S,Id),0,99);
 }
 if(Logger.Site>=0&&Item==TEXT("fieldlogger")){Item.Empty();Payload=0;}
 // Old saves already carrying the device retain their physical loan. New loans
 // may carry an unassembled kit from parts storage to the workbench.
 if(Item==TEXT("fieldlogger")&&Payload>0){Logger.bLoanIssued=true;Logger.bCheckedOut=true;Logger.Checkouts=FMath::Max(1,Logger.Checkouts);}
 if(Logger.Site>=0)Logger.bLoanIssued=true;
 Logger.bCheckedOut=Logger.Site>=0||(Item==TEXT("fieldlogger")&&Payload>0); // actual custody is authoritative on restore
 if(M&&M->Freeform)TrackPowerFault(M->Freeform->PowerFault());
 const auto RestoredCarry=MigrateBenchParcel(O,Dropped,Payload);
 Build();if(Carry)Carry->Restore(RestoredCarry,Payload==4?Dropped:FVector::ZeroVector);OverrideExisting();
 if(M&&M->Web)for(auto& C:M->Web->CollidersMutable(TEXT("saro")))if(C.Id==TEXT("ff:bench")||C.Id==TEXT("ff:sample")||C.Id==TEXT("ff:depot")||C.Id==TEXT("ff:delivery")||C.Id==TEXT("ff:service"))C.bEnabled=true;
 ApplyWear();
 if(M&&M->Web&&ServiceDishIndex()>=0)
 {
  M->Web->SetDishRemoteAvailable(ServiceDishIndex(),!HasArrayFault());
  if(Service==7)M->Web->PointDish(ServiceDishIndex(),ServiceAz+5,ServiceEl,2.2,true);
  if(Service==8)M->Web->PointDish(ServiceDishIndex(),ServiceAz,ServiceEl,2.2,true);
 }
 Apply();
}

void FS47FreeformWorld::Stop()
{
 bActive=false;bPumping=false;if(Carry)Carry->Stop();
 ChargerShown=-1; // D45-20261110: the charger is hidden with the other parts below; its next lit frame shows it again
 StopGeneratorSound(); // D45-20261110: the generator's loop goes with the session (guarded for engine exit); a load brings it back
 // The shared session is also released from US47Main's UObject destructor.
 // At purge/engine exit its actor-owned maps and callbacks may already be gone.
 if(IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed))
 {
  Overrides.Reset();bFuelHeld=false;return;
 }
 auto Live=[](const UObject* O){return IsValid(O)&&!O->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed);};
 if(Live(M->Inter.Get()))for(auto& Entry:Overrides)if(auto* I=M->Inter->Get(Entry.Key))
 {I->bLook=Entry.Value.bLook;I->Label=MoveTemp(Entry.Value.Label);I->Use=MoveTemp(Entry.Value.Use);I->Examine=MoveTemp(Entry.Value.Examine);}
 Overrides.Reset();
 if(bFuelHeld&&Live(M->World.Get()))M->World->HoldTruck(0);bFuelHeld=false;
 AS47Web* Web=M->Web.Get();
 if(!Live(Web)||Web->IsActorBeingDestroyed())return;
 RestorePumpModel();
 RestoreServiceCabinet();
 RestorePilots();
 SetFieldZone(false);PlacedTarget.Empty();bFirstDoorStep=true;DoorK=0;
 for(auto& C:Web->CollidersMutable(TEXT("saro")))if(C.Id==TEXT("ff:bench")||C.Id==TEXT("ff:sample")||C.Id==TEXT("ff:depot")||C.Id==TEXT("ff:delivery")||C.Id==TEXT("ff:service"))C.bEnabled=false;
 if(ServiceDishIndex()>=0)Web->SetDishRemoteAvailable(ServiceDishIndex(),true);
 for(const auto& Pose:WearPose){Web->SetDishRemoteAvailable(Pose.Key,true);Web->SnapDish(Pose.Key,Pose.Value.X,Pose.Value.Y);}
 WearPose.Reset();
 for(FS47Obj O:Parts)if(O.W==Web&&O)
 {
  O.SetVisible(false);
  if(const auto* N=Web->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))
   if(Live(C))C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }
}

S47J::FObj FS47FreeformWorld::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("eastWingAnchors"),1);S47J::Set(O,TEXT("fieldlogger"),Logger.Save());S47J::Set(O,TEXT("workshop"),Crafting.Save());S47J::Set(O,TEXT("arrayWear"),Wear.Save());S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("service"),Service);S47J::Set(O,TEXT("feeder"),Feeder);
 S47J::Set(O,TEXT("faultDay"),LastFaultDay);S47J::Set(O,TEXT("emergencyDay"),EmergencyDay);S47J::Set(O,TEXT("payload"),Payload);S47J::Set(O,TEXT("item"),Item);
 auto C=S47J::New();if(Payload>0&&Price(Item)>0)for(const auto& Pair:Contents)S47J::Set(C,*Pair.Key,Pair.Value);S47J::Set(O,TEXT("contents"),C);
 S47J::Set(O,TEXT("powerFaultSerial"),PowerFaultSerial);S47J::Set(O,TEXT("emergencyFuseFault"),EmergencyFuseFault);S47J::Set(O,TEXT("trackedPowerFault"),bTrackedPowerFault);
 S47J::Set(O,TEXT("parcelLoaded"),bParcelWasLoaded);S47J::Set(O,TEXT("delivered"),Delivered);S47J::Set(O,TEXT("cargoDelivered"),DeliveredByTruck);S47J::Set(O,TEXT("serviceDelivered"),ServiceDeliveries);S47J::Set(O,TEXT("donorTests"),DonorTests);S47J::Set(O,TEXT("serviceTests"),ServiceTests);
 if(Carry&&Payload>0)S47J::Set(O,TEXT("carry"),Carry->Save());
 S47J::Set(O,TEXT("generator"),bGenerator);S47J::Set(O,TEXT("generatorSeconds"),GeneratorSeconds);S47J::Set(O,TEXT("fuel"),Fuel);S47J::Set(O,TEXT("lastClock"),LastClock);
 S47J::Set(O,TEXT("pumping"),bPumping);S47J::Set(O,TEXT("pumpRemaining"),PumpRemaining);S47J::Set(O,TEXT("pumpElapsed"),PumpElapsed);S47J::Set(O,TEXT("pumpVolume"),PumpVolume);S47J::Set(O,TEXT("pumpPaid"),PumpPaid);
 S47J::Set(O,TEXT("serviceDish"),ServiceTarget);S47J::Set(O,TEXT("ordinaryServiceSerial"),OrdinaryServiceSerial);
 S47J::Set(O,TEXT("serviceAz"),ServiceAz);S47J::Set(O,TEXT("serviceEl"),ServiceEl);S47J::Set(O,TEXT("settle"),Settle);
 S47J::Set(O,TEXT("benchJob"),BenchJob);S47J::Set(O,TEXT("benchSeconds"),BenchSeconds);S47J::Set(O,TEXT("feederSeconds"),FeederSeconds);
 S47J::Set(O,TEXT("dropped"),TArray<double>{Dropped.X,Dropped.Y,Dropped.Z});auto S=S47J::New();for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("donor"),TEXT("fuel"),TEXT("battery"),TEXT("film"),TEXT("fragment"),TEXT("archived_fragment"),TEXT("recorder_donor"),TEXT("cell"),TEXT("foil"),TEXT("drive_motor"),TEXT("water"),TEXT("spare"),TEXT("splice_kit"),TEXT("rope"),TEXT("wire"),TEXT("salt"),TEXT("feed"),TEXT("tubes")})S47J::Set(S,Id,Stock.FindRef(Id));S47J::Set(O,TEXT("stock"),S);return O;
}

bool FS47FreeformWorld::Near(const FVector& At,double Range) const
{
 return Ready() && (!M->World||!M->World->bDriving) && FVector2D(M->Player->Pos.X-At.X,M->Player->Pos.Z-At.Z).Size()<=Range && FMath::Abs(M->Player->FloorY-At.Y)<3.;
}
FVector FS47FreeformWorld::ShopPoint() const
{
 // Native gas_station/src/gs_interior.py: existing L-counter, floor 5.446 m.
 return FVector(7981.8,6.506,2070.1);
}
FVector FS47FreeformWorld::ParcelShopPoint() const{return FVector(7982.4,6.556,2070.1);}
bool FS47FreeformWorld::AtShopCounter() const
{
 if(!Ready()||!M->World||M->World->bDriving||!Near(ShopPoint()))return false;
 if(M->Web->Area()!=TEXT("road")&&M->Web->Area()!=TEXT("diner"))return false;
 const FVector P=M->Player->Pos;
 return P.X>=7973.3&&P.X<=7982.7&&P.Z>=2066.3&&P.Z<=2077.5&&FMath::Abs(M->Player->FloorY-5.446)<.3;
}
FVector FS47FreeformWorld::TruckPoint() const
{
 if(M&&M->World)
 {
  const FVector P=M->World->ServicePosition();const double H=M->World->ServiceHeading();
  return P+FVector(FMath::Sin(H)*1.8,.95,FMath::Cos(H)*1.8);
 }
 return FVector(16,.4,11);
}
FVector FS47FreeformWorld::StandPoint(const FString& Id) const
{
 const FVector P=InteractionPoint(Id);
 if(Id==TEXT("ff:bench"))return P-FVector(0,0,1.1);   // calibration run faces north
 if(Id==TEXT("ff:sample"))return P+FVector(0,0,1.1);  // lab run faces south
 return P+FVector(0,0,1.5);
}

FVector FS47FreeformWorld::InteractionPoint(const FString& Id) const
{
 if(Id==TEXT("ff:film"))return FilmAt;
 if(Id==TEXT("ff:logger-device"))return LoggerPoint(Logger.Site);
 if(Id==TEXT("ff:logger-a"))return LoggerPoint(0);
 if(Id==TEXT("ff:logger-b"))return LoggerPoint(1);
 if(Id==TEXT("ff:pump")||Id==TEXT("town:pumps"))return PumpPoint();
 if(Id==TEXT("ff:shop"))return ShopPoint();
 if(Id==TEXT("ff:bench"))return BenchAt+FVector(0,.90,0);
 if(Id==TEXT("ff:depot"))return DepotAt+FVector(0,.90,0);
 if(Id==TEXT("ff:sample"))return SampleAt+FVector(0,.90,0);
 if(Id==TEXT("ff:delivery"))return DeliveryAt+FVector(0,.90,0);
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return GeneratorAt;
 if(Id==TEXT("ff:feeder"))return FS47Substation::Site(FVector(5.52,-9.04,1.30));
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service"))return ServicePosition();
 if(Id.StartsWith(TEXT("field:"))&&M&&M->Web)if(const auto* I=M->Web->Item(Id))return M->Web->ObjAt(I->Node).WorldPosition();
 if(Id==TEXT("ff:cargo"))return TruckPoint();
 if(Id==TEXT("ff:parcel"))return Payload==1?ParcelShopPoint():Payload==3?TruckPoint():Payload==4?ParcelPosition():M&&M->Player?FVector(M->Player->Pos.X,M->Player->FloorY+1.,M->Player->Pos.Z):FVector::ZeroVector;
 return FVector::ZeroVector;
}
FVector FS47FreeformWorld::PumpPoint() const
{
 // Original proxy anchors the same two pumps retained by Tom's own station.
 // The replacement model raises the island top and keeps these x/z centres.
 const FS47Obj P=M&&M->Web?M->Web->Obj(TEXT("world.town.proxies.pumps")):FS47Obj();
 return P?P.WorldPosition()+FVector(0,.75,-2.6):FVector(7989.75,6.136,2052.4);
}
bool FS47FreeformWorld::AtDiesel() const
{
 if(!M||!M->Player)return false;const FVector P=PumpPoint();return M->Player->Pos.Z>P.Z+2.6;
}
bool FS47FreeformWorld::PumpConditions() const
{
 if(!Ready()||!M->World||M->World->bDriving||M->World->bBusy||Payload==2)return false;
 if(M->World->Area!=TEXT("diner")&&M->World->Area!=TEXT("road"))return false;
 if(AtDiesel()||!Near(PumpPoint(),2.8))return false;
 const FVector Truck=M->World->ServicePosition(),At=PumpPoint();
 return FVector2D(Truck.X-At.X,Truck.Z-At.Z).Size()<=4.5&&FMath::Abs(Truck.Y-At.Y)<3.;
}
bool FS47FreeformWorld::StartPump()
{
 if(!Ready()||M->Freeform->Held()||bPumping)return false;
 if(!M->Web->Obj(TEXT("world.town.proxies.pumps"))||!PumpConditions())
 {Change(TEXT("Park the SARO service truck beside the red REGULAR pump, get out, and approach with empty hands."));return false;}
 const double Volume=PumpVolumeFor(Fuel,TankCap());
 if(Volume<.00001){Change(TEXT("Service truck tank is full."));return false;}
 // Metered: the last dollars still buy what they cover. StepPump stops when the budget runs out.
 if(M->Freeform->Budget()<1){Change(TEXT("REGULAR: the budget is empty, so the pump stays off. Work registered at OPS pays into it. On a dry tank, you can still call roadside return from the truck."));return false;}
 bPumping=true;PumpRemaining=Volume;PumpElapsed=PumpVolume=0;PumpPaid=0;
 Change(FString::Printf(TEXT("REGULAR into the SARO truck at $%.2f a unit this week: $1 every two seconds while it runs. Use the pump again to stop; you pay only for what went in."),1./FuelPerDollar()));return true;
}
void FS47FreeformWorld::StopPump()
{
 if(!bPumping)return;bPumping=false;PumpRemaining=PumpElapsed=0;
 ApplyPump();Change(FString::Printf(TEXT("Nozzle back on the pump. Pumped %.2f for $%d; the gauge reads %.2f / %.0f."),PumpVolume,PumpPaid,Fuel,TankCap()));
}
void FS47FreeformWorld::StepPump(double Dt)
{
 if(!bPumping)return;
 if(!PumpConditions()){StopPump();return;}
 PumpElapsed+=FMath::Clamp(Dt,0.,.1);if(PumpElapsed<2.)return;
 const double Amount=PumpIncrement(Fuel,PumpRemaining,TankCap(),FuelPerDollar());
 if(Amount<.00001||!M->Freeform->Spend(1)){StopPump();return;}
 // Spend's autosave request is deferred; fuel and receipt are updated in this
 // same game-thread transaction before any snapshot can observe the new cash.
 Fuel=FMath::Min(TankCap(),Fuel+Amount);PumpRemaining=FMath::Max(0.,PumpRemaining-Amount);PumpVolume+=Amount;++PumpPaid;PumpElapsed=0;
 if(bFuelHeld&&M->World){M->World->HoldTruck(0);bFuelHeld=false;}
 if(PumpRemaining<.00001||Fuel>=TankCap()-.00001)StopPump();else{M->RequestAutosave();ApplyPump();}
}
FString FS47FreeformWorld::PumpDisplay() const
{
 if(bPumping)return FString::Printf(TEXT("SARO / REGULAR\nTANK %.2f / %.0f\nSALE $%d / %.2f\nNEXT %.2f / $1\n%.0f%% / E STOP"),Fuel,TankCap(),PumpPaid,PumpVolume,PumpIncrement(Fuel,PumpRemaining,TankCap(),FuelPerDollar()),FMath::Clamp(PumpElapsed/2.,0.,1.)*100.);
 const double Volume=PumpVolumeFor(Fuel,TankCap());return FString::Printf(TEXT("SARO / REGULAR\nTANK %.2f / %.0f\nUP TO %.2f / $%d\nE START / E STOP\nLAST $%d / %.2f"),Fuel,TankCap(),Volume,FMath::CeilToInt(Volume/FuelPerDollar()-.00001),PumpPaid,PumpVolume);
}
void FS47FreeformWorld::ApplyPump()
{
 const FString Display=PumpDisplay();if(Display==LastPumpDisplay)return;LastPumpDisplay=Display;
 if(bTripoPump)
 {
  const FString Left=FString::Printf(TEXT("TANK\n%.2f OF %.0f\nSALE $%d"),Fuel,TankCap(),PumpPaid);
  const FString Right=FString::Printf(TEXT("FUEL $%.2f/U\n%.2f SOLD\nE %s"),1./FuelPerDollar(),PumpVolume,bPumping?TEXT("STOP"):TEXT("START"));
  if(auto* C=PumpTextEast.Get())C->SetText(FText::FromString(Left));if(auto* C=PumpTextWest.Get())C->SetText(FText::FromString(Right));
 }
 else for(const auto& Weak:{PumpTextEast,PumpTextWest})if(auto* C=Weak.Get())C->SetText(FText::FromString(Display));
}
bool FS47FreeformWorld::TripoPumpLoaded() const
{
 if(!bTripoPump||!M||!M->Web||!PumpModel||!PumpStation.IsValid())return false;
 const auto* N=M->Web->Node(PumpModel.I);const auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
 const auto* Station=PumpStation.Get();return C&&C->GetStaticMesh()&&Station->GetStaticMesh()&&
 C->GetStaticMesh()->GetPathName()==TEXT("/Game/Licensed/TripoPump/caliche_regular_pump/SM_Tripo_CalicheRegularPump.SM_Tripo_CalicheRegularPump")&&
 Station->GetStaticMesh()->GetPathName()==TEXT("/Game/Licensed/TripoPump/Station/SM_CalicheStation_NoRegularPump.SM_CalicheStation_NoRegularPump");
}
bool FS47FreeformWorld::InstallPumpModel(int32 RoadParent)
{
 if(!M||!M->Web||RoadParent<0)return false;
 auto* Trim=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Licensed/TripoPump/Station/SM_CalicheStation_NoRegularPump.SM_CalicheStation_NoRegularPump"));
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Licensed/TripoPump/caliche_regular_pump/SM_Tripo_CalicheRegularPump.SM_Tripo_CalicheRegularPump"));
 if(!Trim||!Mesh)return false;
 if(!PumpStation.IsValid())for(int32 I=0;;++I)
 {
  const auto* N=M->Web->Node(I);if(!N)break;
  if(N->Name==TEXT("prop:gasStation"))if(auto* C=Cast<UStaticMeshComponent>(N->Comp.Get())){PumpStation=C;PreviousStation.Reset(C->GetStaticMesh());PumpOriginalMaterials.Reset();for(int32 Slot=0;Slot<C->GetNumMaterials();++Slot)PumpOriginalMaterials.Emplace(C->GetMaterial(Slot));break;}
 }
 auto* Station=PumpStation.Get();if(!Station||!PreviousStation.IsValid())return false;
 if(!PumpModel)
 {
  // Ground pivot, preserved island anchor; normalized front +Y becomes +X
  // after the measured FBX conversion and a native 90-degree placement yaw.
  const FVector Base=PumpPoint()-FVector(0,1.59,0);
  // Critic 8 November: fitted to 2.10 m it stood over the island like a cabinet. A 1980s pump is about 1.6-1.7 m; 0.8 of the fit, on the same foot.
  const double Width=Mesh->GetBounds().BoxExtent.X*.02*.8;
  PumpModel=Model(TEXT("FreeformTripoRegularPump"),TEXT("/Game/Licensed/TripoPump/caliche_regular_pump/SM_Tripo_CalicheRegularPump.SM_Tripo_CalicheRegularPump"),Base,Width,RoadParent);
  if(!PumpModel)return false;
  if(const auto* N=M->Web->Node(PumpModel.I))if(auto* C=N->Comp.Get())C->SetWorldRotation(FRotator(0,90,0));
 }
 // Removed pump-only slots may be compacted by FBX. Rebind by original
 // slot name, preserving this component's existing lighting material instances.
 TArray<int32> Mapping;for(const auto& Slot:Trim->GetStaticMaterials())
 {
  const int32 Source=PreviousStation->GetMaterialIndex(Slot.MaterialSlotName);if(!PumpOriginalMaterials.IsValidIndex(Source))return false;Mapping.Add(Source);
 }
 Station->SetStaticMesh(Trim);for(int32 I=0;I<Mapping.Num();++I)Station->SetMaterial(I,PumpOriginalMaterials[Mapping[I]].Get());
 bTripoPump=true;return true;
}
void FS47FreeformWorld::RestorePumpModel()
{
 if(auto* C=PumpStation.Get())if(auto* Original=PreviousStation.Get()){C->SetStaticMesh(Original);for(int32 I=0;I<PumpOriginalMaterials.Num();++I)C->SetMaterial(I,PumpOriginalMaterials[I].Get());}
 bTripoPump=false;
}
bool FS47FreeformWorld::InstallServiceCabinet()
{
 if(!M||!M->Web)return false;
 constexpr int32 Nodes[]={802,803,804,805};
 constexpr int32 Geos[]={533,534,535,536};
 TArray<UStaticMesh*> Trim;
 for(int32 K=0;K<4;++K)
 {
  const auto* N=M->Web->Node(Nodes[K]);
  auto* C=N?Cast<UStaticMeshComponent>(N->Comp.Get()):nullptr;
  auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/ArrayCabinet/StaticMeshes/g%d.g%d"),Geos[K],Geos[K]));
  if(!C||N->Geo!=Geos[K]||!C->GetStaticMesh()||!Mesh||Mesh->GetStaticMaterials().Num()!=1)return false;
  Trim.Add(Mesh);
 }
 auto* BodyMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ArrayCabinet/StaticMeshes/SM_S03CabinetBody.SM_S03CabinetBody"));
 auto* DoorMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ArrayCabinet/StaticMeshes/SM_S03CabinetDoor.SM_S03CabinetDoor"));
 const auto* Parent=M->Web->Node(773);
 if(!BodyMesh||!DoorMesh||!Parent||!Parent->Comp.IsValid()||BodyMesh->GetStaticMaterials().Num()!=3||DoorMesh->GetStaticMaterials().Num()!=2)return false;
 // Validate all six replacements before touching the merged originals.
 if(CabinetOriginals.IsEmpty())for(int32 K=0;K<4;++K)
 {
  auto* C=Cast<UStaticMeshComponent>(M->Web->Node(Nodes[K])->Comp.Get());
  FCabinetOriginal O;O.Component=C;O.Mesh.Reset(C->GetStaticMesh());
  for(int32 J=0;J<C->GetNumMaterials();++J)O.Materials.Emplace(C->GetMaterial(J));
  CabinetOriginals.Add(MoveTemp(O));
 }
 auto Part=[&](const TCHAR* Name,UStaticMesh* Mesh,const FVector& At,const TArray<int32>& Sources)
 {
  auto* C=NewObject<UStaticMeshComponent>(M->Web,Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(Parent->Comp.Get());C->SetStaticMesh(Mesh);
  for(int32 K=0;K<Sources.Num();++K)C->SetMaterial(K,CabinetOriginals[Sources[K]].Materials[0].Get());
  C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldRotation(FRotator::ZeroRotator);C->SetWorldScale3D(FVector(1));
  C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();
  auto O=M->Web->ObjAt(M->Web->AdoptNode(C,773,Name));Parts.Add(O);M->Web->MarkUsable(O.I,TEXT("field:S-03"));return O;
 };
 if(!CabinetBody)CabinetBody=Part(TEXT("FreeformOriginalS03Body"),BodyMesh,FVector::ZeroVector,{0,1,3});
 if(!Lid)Lid=Part(TEXT("FreeformOriginalS03Door"),DoorMesh,FVector(4.647,-.01,-39.72),{1,2});
 if(!FieldLid)
 {
  FieldLid=Part(TEXT("FreeformFieldServiceDoor"),DoorMesh,FVector(4.647,-.01,-39.72),{1,2});
  M->Web->MarkUsable(FieldLid.I,TEXT("ff:service"));FieldLid.SetVisible(false);
  // The door's texture is painted for S-03. On the other antennas a plate covers that name; their box has its own.
  if(const auto* Node=M->Web->Node(FieldLid.I))if(auto* DoorComp=Node->Comp.Get())
  {
   auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
   auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
   if(Cube&&Base)
   {
    auto* Plate=NewObject<UStaticMeshComponent>(M->Web,TEXT("FreeformFieldServicePlate"));
    Plate->SetMobility(EComponentMobility::Movable);Plate->SetupAttachment(DoorComp);Plate->SetStaticMesh(Cube);
    auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(0x8b8d8a));Plate->SetMaterial(0,Mat);
    Plate->SetCollisionEnabled(ECollisionEnabled::NoCollision);Plate->SetCastShadow(false);
    Plate->RegisterComponent();
    Plate->SetRelativeLocation(FVector(-2.2,32,24));Plate->SetRelativeScale3D(FVector(.012,.56,.25));
    auto* Words=NewObject<UTextRenderComponent>(M->Web,TEXT("FreeformFieldServiceWords"));
    Words->SetMobility(EComponentMobility::Movable);Words->SetupAttachment(DoorComp);
    Words->SetText(FText::FromString(TEXT("RELAY HATCH")));Words->SetWorldSize(4.2f);Words->SetHorizontalAlignment(EHTA_Center);Words->SetVerticalAlignment(EVRTA_TextCenter);
    Words->SetTextRenderColor(FColor(28,28,26));Words->SetCollisionEnabled(ECollisionEnabled::NoCollision);Words->RegisterComponent();
    Words->SetRelativeLocation(FVector(-3.0,32,24));Words->SetRelativeRotation(FRotator(0,180,0));
   }
  }
 }
 for(int32 K=0;K<4;++K)if(auto* C=CabinetOriginals[K].Component.Get())
 {C->SetStaticMesh(Trim[K]);C->SetMaterial(0,CabinetOriginals[K].Materials[0].Get());}
 bServiceCabinet=true;return true;
}
FVector FS47FreeformWorld::TruckPointForTest(){return TruckPoint();}
void FS47FreeformWorld::PlaceServiceDoor()
{
 if(!M||!M->Web||PlacedTarget==ServiceTarget)return;
 FVector At=ServiceAt(),Door(4.647,-.01,-39.72);DoorYaw=0;
 if(ServiceTarget!=TEXT("S-03"))
 {
  const auto* I=M->Web->Item(TEXT("field:")+ServiceTarget);const int32 D=ServiceDishIndex();
  if(!I||D==INDEX_NONE)return;
  // The face of the drive box turned away from its antenna. Rotated from the S-03 cabinet, whose door is on its -X side.
  const FVector Mid=(I->Min+I->Max)*.5,Size=I->Max-I->Min;
  const bool bThinX=Size.X<Size.Z;
  const FVector2D Out=bThinX?FVector2D(FMath::Sign(Mid.X-M->Web->DishX(D)),0):FVector2D(0,FMath::Sign(Mid.Z-M->Web->DishZ(D)));
  DoorYaw=FMath::Atan2(Out.Y,-Out.X);
  const double Half=(bThinX?Size.X:Size.Z)*.5;
  const auto Local=[&](double X,double Y,double Z){return FVector(Mid.X+X*FMath::Cos(DoorYaw)+Z*FMath::Sin(DoorYaw),Y,Mid.Z-X*FMath::Sin(DoorYaw)+Z*FMath::Cos(DoorYaw));};
  At=Local(-Half-.335,-.01,0);Door=Local(-Half-.01,-.01,-.32);
 }
 PlacedTarget=ServiceTarget;DoorFace=At;
 const double Deg=-FMath::RadiansToDegrees(DoorYaw);
 const auto Put=[&](FS47Obj O,const FVector& Local)
 {
  const FVector P=At+FVector(Local.X*FMath::Cos(DoorYaw)+Local.Z*FMath::Sin(DoorYaw),Local.Y,-Local.X*FMath::Sin(DoorYaw)+Local.Z*FMath::Cos(DoorYaw));
  if(O)if(const auto* N=O.W->Node(O.I))if(auto* C=N->Comp.Get())C->SetWorldLocationAndRotation(S47Three::ToUE(P),FRotator(0,Deg,0));
 };
 Put(Panel,FVector(.32,0,0));Put(Relay,FVector(.24,.04,0));Put(Selector,FVector(.23,-.20,-.16));
 if(ServiceText.IsValid())
 {
  ServiceText->SetWorldLocationAndRotation(S47Three::ToUE(At+FVector(.30*FMath::Cos(DoorYaw),.30,-.30*FMath::Sin(DoorYaw))),FRotator(0,180+Deg,0));
 }
 if(FieldLid)if(const auto* N=FieldLid.W->Node(FieldLid.I))if(auto* C=N->Comp.Get())C->SetWorldLocation(S47Three::ToUE(Door));
}
void FS47FreeformWorld::ApplyServiceDoor()
{
 if(!M||!M->Web)return;
 PlaceServiceDoor();
 const bool bS03=ServiceTarget==TEXT("S-03");
 const bool Show=bActive&&bServiceCabinet&&DoorK>.02;
 const double A=-1.4*DoorK;
 if(Lid){Lid.SetRotY(bS03?A:0);Lid.SetVisible(bActive&&bServiceCabinet);}
 if(FieldLid)
 {
  const bool bHere=bActive&&bServiceCabinet&&!bS03;
  FieldLid.SetVisible(bHere);
  if(const auto* N=FieldLid.W->Node(FieldLid.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))
  {
   C->SetCollisionEnabled(bHere?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
   C->SetWorldRotation(FRotator(0,-FMath::RadiansToDegrees(DoorYaw+A),0));
  }
 }
 if(bServiceCabinet)
 {
  const double G=bS03?A:0;const FVector D(-.017,.41,.54);
  FS47GlowView(M->Web,TEXT("world.field.glow.points")).SetPos(2,FVector(4.647,-.01,-39.72)+FVector(FMath::Cos(G)*D.X+FMath::Sin(G)*D.Z,D.Y,-FMath::Sin(G)*D.X+FMath::Cos(G)*D.Z));
 }
 if(ServiceText.IsValid())ServiceText->SetVisibility(Show);
 for(FS47Obj O:{Panel,Relay,Selector})if(O){O.SetVisible(Show);if(const auto* N=O.W->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(Show?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
}
void FS47FreeformWorld::StepServiceDoor(double Dt)
{
 const bool bOpen=bActive&&Service>=4&&Service<=5;
 if(bOpen!=bDoorOpen)
 {
  bDoorOpen=bOpen;
  if(M&&M->Audio&&!bFirstDoorStep)M->Audio->Sfx(bOpen?TEXT("doorCreak0"):TEXT("doorClose"));
 }
 bFirstDoorStep=false;
 const double Was=DoorK;
 DoorK=FMath::FInterpConstantTo(DoorK,bOpen?1.:0.,Dt,1.6);
 if(DoorK!=Was||PlacedTarget!=ServiceTarget)ApplyServiceDoor();
}
void FS47FreeformWorld::SetFieldZone(bool bOn)
{
 if(!M||!IsValid(M->Web)||bOn==bFieldZone)return;
 bFieldZone=bOn;
 auto& Zones=M->Web->ZonesMutable(TEXT("saro"));
 Zones.RemoveAll([](const FS47Rect& R){return R.Id==TEXT("ff:arrayfield");});
 // Out of the truck anywhere it can be driven, one can walk.
 if(bOn)Zones.Add({S47Legs::ARRAY_FIELD.MinX,S47Legs::ARRAY_FIELD.MaxX,S47Legs::ARRAY_FIELD.MinZ,S47Legs::ARRAY_FIELD.MaxZ,TEXT("ff:arrayfield"),true});
}
void FS47FreeformWorld::RestoreServiceCabinet()
{
 for(auto& O:CabinetOriginals)if(auto* C=O.Component.Get())if(O.Mesh.IsValid())
 {C->SetStaticMesh(O.Mesh.Get());for(int32 K=0;K<O.Materials.Num();++K)C->SetMaterial(K,O.Materials[K].Get());}
 FS47GlowView(M->Web,TEXT("world.field.glow.points")).SetPos(2,FVector(4.63,.40,-39.18));
 bServiceCabinet=false;
}
bool FS47FreeformWorld::ShopOpen() const { return Ready() && DayClock(M->Freeform->Clock())>=8*3600 && DayClock(M->Freeform->Clock())<20*3600; }
void FS47FreeformWorld::Change(const FString& Message)
{
 if(M){if(!Message.IsEmpty()&&M->UI)M->UI->Toast(ServiceWords(Message),5);M->RequestAutosave();}Apply();
}

FS47Obj FS47FreeformWorld::Box(const FString& Name,const FVector& At,const FVector& Size,uint32 Colour,int32 Parent,const FString& Id)
{
 if(!M||!M->Web)return {};const auto* P=M->Web->Node(Parent);if(!P||!P->Comp.IsValid())return {};
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Mesh||!Base)return {};
 auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Colour));Mat->SetScalarParameterValue(TEXT("Roughness"),.75);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
 auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(P->Comp.Get());C->SetStaticMesh(Mesh);C->SetMaterial(0,Mat);C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(S47Three::DirToUE(Size));
 C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();
 const FS47Obj O=M->Web->ObjAt(M->Web->AdoptNode(C,Parent,Name));Parts.Add(O);if(!Id.IsEmpty())Bind(O,Id);return O;
}
FS47Obj FS47FreeformWorld::Model(const FString& Name,const TCHAR* Asset,const FVector& At,double Width,int32 Parent,double Yaw)
{
 const auto* P=M->Web->Node(Parent);auto* Mesh=LoadObject<UStaticMesh>(nullptr,Asset);if(!P||!P->Comp.IsValid()||!Mesh)return {};
 const FBoxSphereBounds B=Mesh->GetBounds();const double K=Width*100/FMath::Max(1.,B.BoxExtent.X*2.);
 auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(P->Comp.Get());C->SetStaticMesh(Mesh);
 C->SetWorldScale3D(FVector(K));const FVector Offset(B.Origin.X,B.Origin.Y,B.Origin.Z-B.BoxExtent.Z);const FRotator Rot(0,Yaw,0);
 C->SetWorldRotation(Rot);C->SetWorldLocation(S47Three::ToUE(At)-Rot.RotateVector(Offset*K));
 C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();
 const FS47Obj O=M->Web->ObjAt(M->Web->AdoptNode(C,Parent,Name));Parts.Add(O);return O;
}
void FS47FreeformWorld::Bind(FS47Obj Object,const FString& Id,double Range)
{
 if(!Object||!M||!M->Inter)return;const TWeakPtr<uint8> Guard=Life;FS47Interactable I;I.Id=Id;I.Object=Object;I.Range=Range;
 I.Label=[this,Guard,Id]{return Guard.IsValid()?Label(Id):FString();};I.Use=[this,Guard,Id]{if(Guard.IsValid())Use(Id);};
 I.Examine=[this,Guard,Id]{return Guard.IsValid()?(Id==TEXT("ff:service")?ServiceStatus():Label(Id)):FString();};M->Inter->Add(I);M->Web->MarkUsable(Object.I,Id);
}

void FS47FreeformWorld::OverrideExisting()
{
 if(!M||!M->Inter||!bActive)return;
 const TWeakPtr<uint8> Guard=Life;
 TArray<FString> Ids={TEXT("substation:journal"),TEXT("substation:service"),TEXT("substation:lamp"),TEXT("obs:generator"),TEXT("town:pumps")};
 for(int32 N=1;N<=27;++N)Ids.Add(FString::Printf(TEXT("field:S-%02d"),N));
 for(const FString& Id:Ids)
 {
  if(Overrides.Contains(Id))continue;auto* I=M->Inter->Get(Id);if(!I)continue;
  FOverride O;O.bLook=I->bLook;O.Label=I->Label;O.Examine=I->Examine;O.Use=I->Use;Overrides.Add(Id,MoveTemp(O));
  if(Id.StartsWith(TEXT("field:")))I->bLook=false;
  I->Label=[this,Guard,Id]{return Guard.IsValid()?Label(Id):FString();};
  I->Examine=[this,Guard,Id]{return Guard.IsValid()?(Id==TEXT("field:")+ServiceTarget?ServiceStatus():Label(Id)):FString();};
  I->Use=[this,Guard,Id]{if(Guard.IsValid())Use(Id);};
 }
}

bool FS47FreeformWorld::Build()
{
 if(bBuilt){if(M&&M->Web){InstallPumpModel(M->Web->Handle(TEXT("world.legs.road.area.group")));InstallServiceCabinet();}return true;}if(!M||!M->Web||!M->Web->IsLoaded()||!M->Inter)return false;
 const int32 Saro=M->Web->Handle(TEXT("ext.group")),Road=M->Web->Handle(TEXT("world.legs.road.area.group"));if(Saro<0||Road<0)return false;
 // The existing photo-lab storage has film cartons. This small labelled stock
 // carton brings that supply to the dry camera bench without changing its model.
 Box(TEXT("FreeformPhotoFilmCarton"),FilmAt,FVector(.18,.16,.13),0xcba65d,Saro,TEXT("ff:film"));
 Box(TEXT("FreeformPhotoFilmLabel"),FilmAt+FVector(-.091,0,0),FVector(.004,.095,.10),0xeee5cc,Saro,TEXT("ff:film"));
 // Reuse project tables at distinct physical workstations inside the east wing.
 // EASTWING-01: the relay/donor calibration bench matches the wing's green steel benches.
 // The calibration casework run IS the station (instruments on its east half come from S47EastWingProps).
 Bench=Model(TEXT("FreeformServiceBench"),TEXT("/Game/EastWing/Props/Extras/SM_EW_CaseworkRun24.SM_EW_CaseworkRun24"),BenchAt+FVector(.65,0,0),2.4,Saro,180);
 const bool bCase=bool(Bench);
 const double BenchRaise=0.;
 if(!Bench)Bench=Model(TEXT("FreeformServiceBench"),TEXT("/Game/ArchiveProps/SM_ArchiveProps_WorkTable.SM_ArchiveProps_WorkTable"),BenchAt,1.8,Saro);
 if(!Bench)Bench=Box(TEXT("FreeformServiceBenchTop"),BenchAt+FVector(0,.78,0),FVector(1.8,.12,.75),0x514638,Saro);
 Bind(Bench,TEXT("ff:bench"));
 for(int32 I=0;I<2;++I)LoggerPosts[I]=Box(FString::Printf(TEXT("FreeformLoggerPost%d"),I),LoggerPoint(I)-FVector(0,.4,0),FVector(.14,.8,.14),0x777e68,Saro,I==0?TEXT("ff:logger-a"):TEXT("ff:logger-b"));
 LoggerBody=Box(TEXT("FreeformDeployedLogger"),LoggerPoint(0),FVector(.46,.32,.38),0x59635d,Saro,TEXT("ff:logger-device"));
 if(bCase)M->Web->CollidersMutable(TEXT("saro")).Add({BenchAt.X+.65-1.22,BenchAt.X+.65+1.22,BenchAt.Z-.34,BenchAt.Z+.33,TEXT("ff:bench"),true});
 else M->Web->CollidersMutable(TEXT("saro")).Add({BenchAt.X-.92,BenchAt.X+.92,BenchAt.Z-.4,BenchAt.Z+.4,TEXT("ff:bench"),true});
 // EASTWING-01: field samples are analysed at a green steel lab bench with its own scope and jars.
 // The lab casework run under the north windows IS the sample station.
 SampleBench=Model(TEXT("FreeformSampleBench"),TEXT("/Game/EastWing/Props/Extras/SM_EW_CaseworkRun24.SM_EW_CaseworkRun24"),SampleAt,2.4,Saro);
 const bool bSampleCase=bool(SampleBench);
 if(SampleBench)
 {
  const double Top=CaseTop;
  Model(TEXT("FreeformSampleScope"),TEXT("/Game/EastWing/Props/Lab/SM_EW_Microscope.SM_EW_Microscope"),SampleAt+FVector(-.55,Top,.05),.21,Saro);
  Model(TEXT("FreeformSampleJarA"),TEXT("/Game/EastWing/Props/Lab/SM_EW_SampleJar.SM_EW_SampleJar"),SampleAt+FVector(.42,Top,-.12),.082,Saro);
  Model(TEXT("FreeformSampleJarB"),TEXT("/Game/EastWing/Props/Lab/SM_EW_SampleJar.SM_EW_SampleJar"),SampleAt+FVector(.56,Top,-.05),.082,Saro);
  Model(TEXT("FreeformSampleScale"),TEXT("/Game/EastWing/Props/Lab/SM_EW_LabScale.SM_EW_LabScale"),SampleAt+FVector(.62,Top,.12),.23,Saro);
 }
 else SampleBench=Model(TEXT("FreeformSampleBench"),TEXT("/Game/ArchiveProps/SM_ArchiveProps_WorkTable.SM_ArchiveProps_WorkTable"),SampleAt,1.8,Saro);
 if(!SampleBench)SampleBench=Box(TEXT("FreeformSampleBenchTop"),SampleAt+FVector(0,.78,0),FVector(1.8,.12,.75),0x514638,Saro);
 Bind(SampleBench,TEXT("ff:sample"));
 // EASTWING-01: the stock counter is two real drawer cabinets; the receiving table is a green steel bench.
 Depot=Model(TEXT("FreeformDepotCabinetA"),TEXT("/Game/EastWing/Props/Workshop/SM_EW_DrawerCabinet.SM_EW_DrawerCabinet"),DepotAt+FVector(-.36,0,0),.72,Saro);
 if(Depot){Bind(Depot,TEXT("ff:depot"));if(FS47Obj B=Model(TEXT("FreeformDepotCabinetB"),TEXT("/Game/EastWing/Props/Workshop/SM_EW_DrawerCabinet.SM_EW_DrawerCabinet"),DepotAt+FVector(.36,0,0),.72,Saro))Bind(B,TEXT("ff:depot"));}
 else Depot=Box(TEXT("FreeformDepotBin"),DepotAt+FVector(0,.48,0),FVector(1.2,.96,.65),0x687260,Saro,TEXT("ff:depot"));
 Delivery=Model(TEXT("FreeformReceivingBench"),TEXT("/Game/EastWing/Props/Workshop/SM_EW_GreenBench.SM_EW_GreenBench"),DeliveryAt,1.2,Saro);
 if(Delivery)Bind(Delivery,TEXT("ff:delivery"));else Delivery=Box(TEXT("FreeformReceivingTable"),DeliveryAt+FVector(0,.78,0),FVector(1.2,.12,.75),0x514638,Saro,TEXT("ff:delivery"));
 if(bSampleCase)M->Web->CollidersMutable(TEXT("saro")).Add({SampleAt.X-1.22,SampleAt.X+1.22,SampleAt.Z-.33,SampleAt.Z+.34,TEXT("ff:sample"),true});
 else M->Web->CollidersMutable(TEXT("saro")).Add({SampleAt.X-.92,SampleAt.X+.92,SampleAt.Z-.4,SampleAt.Z+.4,TEXT("ff:sample"),true});
 M->Web->CollidersMutable(TEXT("saro")).Add({DepotAt.X-.74,DepotAt.X+.74,DepotAt.Z-.34,DepotAt.Z+.31,TEXT("ff:depot"),true});
 M->Web->CollidersMutable(TEXT("saro")).Add({DeliveryAt.X-.6,DeliveryAt.X+.6,DeliveryAt.Z-.4,DeliveryAt.Z+.4,TEXT("ff:delivery"),true});
 // EASTWING-01: SARO RT-12 relay test set; its bezel window frames the live status lamp below.
 // Bounds centre sits .061 m ahead of the case (test leads), hence the z offset.
 BenchMeter=bCase?Model(TEXT("FreeformBenchMeter"),TEXT("/Game/EastWing/Props/Extras/SM_EW_RelayTester.SM_EW_RelayTester"),BenchAt+FVector(-.10,CaseTop,.10-.061),.42,Saro,180):FS47Obj();
 if(BenchMeter)Bind(BenchMeter,TEXT("ff:bench"));
 else BenchMeter=Box(TEXT("FreeformBenchMeter"),BenchAt+FVector(-.44,1.02+BenchRaise,-.10),FVector(.42,.28,.27),0x555b4c,Saro,TEXT("ff:bench"));
 BenchLamp=Box(TEXT("FreeformBenchTestIndicator"),bCase?BenchAt+FVector(-.10,CaseTop+.20,-.05):BenchAt+FVector(-.44,1.04+BenchRaise,.05),FVector(.28,.11,.015),0x5e8b61,Saro);
 // A dead branch shows it from the jeep: an amber fault lamp on top of its junction box.
 for(int32 I=0;I<FS47ArrayWear::Branches().Num()&&I<4;++I)
 {
  const FVector At=InteractionPoint(FString(TEXT("field:"))+FS47ArrayWear::Branches()[I].Junction);
  // D45-20261110: red, as the cabinet pilots of the dead antennas are; this one marks where the branch is mended
  if(!At.IsNearlyZero())WearLamps[I]=Box(FString::Printf(TEXT("FreeformBranchFaultLamp%d"),I),At+FVector(0,.80,0),FVector(.11,.09,.11),0xd8321e,Saro);
 }
 // EASTWING workshop tray: one small part per slot on the free right of the worktop.
 for(int32 I=0;I<FS47Workshop::TraySize;++I)
 {
  // Measured free worktop: +x past the oscilloscope, either side of the bench's hand tester.
  static const FVector2D Slots[FS47Workshop::TraySize]={{1.04,.14},{1.22,.14},{1.40,.14},{1.58,.14},{1.76,.14},{.96,-.20},{1.56,-.20},{1.74,-.20}};
  const FVector Slot=BenchAt+FVector(Slots[I].X,(bCase?CaseTop:1.02+BenchRaise)+.035,Slots[I].Y);
  TrayParts[I]=Box(FString::Printf(TEXT("FreeformWorkshopPart%d"),I),Slot,FVector(.13,.07,.10),0x5a5f58,Saro,TEXT("ff:bench"));
 }
 // Base-normalised authored tray rests on the actual green worktop; visibility still follows the retained sample.
 BenchSample=Model(TEXT("FreeformBenchSampleHolder"),TEXT("/Game/EastWing/Props/Extras/SM_EW_SampleHolder.SM_EW_SampleHolder"),SampleAt+FVector(.03,CaseTop,.05),.15,Saro);
 if(!BenchSample)BenchSample=Box(TEXT("FreeformBenchSampleHolder"),SampleAt+FVector(.03,CaseTop+.06,.05),FVector(.15,.045,.12),0x9f9b89,Saro);
 M->Web->MarkUsable(BenchLamp.I,TEXT("ff:bench"));M->Web->MarkUsable(BenchSample.I,TEXT("ff:sample"));
 Shop=Box(TEXT("FreeformGasSupplyCard"),ShopPoint(),FVector(.44,.20,.025),0x243d35,Road,TEXT("ff:shop"));
 Box(TEXT("FreeformGasSupplyFoot"),ShopPoint()+FVector(0,-.098,0),FVector(.46,.025,.16),0x243d35,Road);
 ParcelSaro=Box(TEXT("FreeformParcelSaro"),DeliveryAt+FVector(0,1.0,0),FVector(.46,.32,.38),0x9f8250,Saro);
 ParcelRoad=Box(TEXT("FreeformParcelRoad"),ParcelShopPoint(),FVector(.46,.32,.38),0x9f8250,Road);
 // Both visible parcel representations share one authoritative transaction/state.
 Bind(ParcelSaro,TEXT("ff:parcel"));M->Web->MarkUsable(ParcelRoad.I,TEXT("ff:parcel"));
 CargoSaro=Box(TEXT("FreeformCargoSaro"),TruckPoint(),FVector(.62,.12,.52),0x383e37,Saro);
 CargoRoad=Box(TEXT("FreeformCargoRoad"),TruckPoint(),FVector(.62,.12,.52),0x383e37,Road);
 Bind(CargoSaro,TEXT("ff:cargo"),3.0);M->Web->MarkUsable(CargoRoad.I,TEXT("ff:cargo"));
 Generator=Box(TEXT("FreeformGeneratorControl"),GeneratorAt,GeneratorBox,0x2a2c2e,Saro,TEXT("ff:generator"));
 {
  const FVector At=ServiceAt();
  // Adapt the authored cabinet. Its existing ground collider remains authoritative.
  InstallServiceCabinet();
  Panel=Box(TEXT("FreeformS03Panel"),At+FVector(.32,0,0),FVector(.025,.80,.54),0x333e36,Saro,TEXT("ff:service"));
  Relay=Box(TEXT("FreeformS03Relay"),At+FVector(.24,.04,0),FVector(.12,.16,.16),0xb19651,Saro);
  Selector=Box(TEXT("FreeformS03Selector"),At+FVector(.23,-.20,-.16),FVector(.08,.09,.05),0xab4931,Saro);
  for(const FS47Obj& O:{Relay,Selector})M->Web->MarkUsable(O.I,TEXT("ff:service"));
 }
 FeederPanel=Box(TEXT("FreeformFeederPanel"),InteractionPoint(TEXT("ff:feeder")),FVector(.40,.32,.08),0x56645d,Road,TEXT("ff:feeder"));
 FeederLever=Box(TEXT("FreeformFeederLever"),InteractionPoint(TEXT("ff:feeder"))+FVector(.11,0,-.06),FVector(.035,.18,.04),0x92663e,Road);
 FeederLamp=Box(TEXT("FreeformFeederIndicator"),InteractionPoint(TEXT("ff:feeder"))+FVector(-.1,.08,-.07),FVector(.07,.07,.04),0x58a769,Road);
 M->Web->MarkUsable(FeederLever.I,TEXT("ff:feeder"));M->Web->MarkUsable(FeederLamp.I,TEXT("ff:feeder"));
 auto LabelPart=[&](const TCHAR* Name,const FString& Text,const FVector& At,float Yaw,float Size,int32 Parent)
 {
  const auto* P=M->Web->Node(Parent);if(!P||!P->Comp.IsValid())return;
  auto* C=NewObject<UTextRenderComponent>(M->Web,Name);C->SetupAttachment(P->Comp.Get());C->SetMobility(EComponentMobility::Movable);
  C->SetText(FText::FromString(Text));C->SetWorldSize(Size);C->SetHorizontalAlignment(EHTA_Center);C->SetVerticalAlignment(EVRTA_TextCenter);
  C->SetTextRenderColor(FColor(221,210,171));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldRotation(FRotator(0,Yaw,0));
  Parts.Add(M->Web->ObjAt(M->Web->AdoptNode(C,Parent,Name)));
  if(FString(Name)==TEXT("FreeformS03Text"))ServiceText=C;
 };
 // The existing cabinet door is the closed interaction surface; controls sit inside it.
 Bind(Panel,TEXT("ff:service"));
 LabelPart(TEXT("FreeformS03Text"),TEXT("LOCAL / RELAY / TEST"),ServiceAt()+FVector(.30,.30,0),180,1.5,Saro);
 LabelPart(TEXT("FreeformGasLabel"),TEXT("SUPPLIES\n08:00 - 20:00"),ShopPoint()+FVector(0,0,-.017),-90,2.3,Road);
 if(FS47Obj Sign=Model(TEXT("FreeformBenchLabel"),TEXT("/Game/EastWing/Props/Extras/SM_EW_CalibrationSign.SM_EW_CalibrationSign"),BenchAt+FVector(-.55,CaseTop,.27),.42,Saro,180))M->Web->MarkUsable(Sign.I,TEXT("ff:bench"));
 else LabelPart(TEXT("FreeformBenchLabel"),TEXT("CALIBRATION / REPAIR\nDONOR / LOGGER TEST"),BenchAt+FVector(0,1.45,-.35),90,3.5,Saro);
 if(FS47Obj Sign=Model(TEXT("FreeformSampleLabel"),TEXT("/Game/EastWing/Props/Extras/SM_EW_SampleAnalysisSign.SM_EW_SampleAnalysisSign"),SampleAt+FVector(-.05,CaseTop,-.27),.42,Saro))M->Web->MarkUsable(Sign.I,TEXT("ff:sample"));
 else LabelPart(TEXT("FreeformSampleLabel"),TEXT("GENERAL TEST LAB\nFIELD SAMPLE ANALYSIS"),SampleAt+FVector(0,1.45,-.35),90,3.0,Saro);
 // Physical checkout placard replaces the floating text in front of the storage rack.
 if(FS47Obj Sign=Model(TEXT("FreeformDepotLabel"),TEXT("/Game/EastWing/Props/Extras/SM_EW_DepotCheckoutSign.SM_EW_DepotCheckoutSign"),DepotAt+FVector(0,.9245,.10),.42,Saro))M->Web->MarkUsable(Sign.I,TEXT("ff:depot"));
 else LabelPart(TEXT("FreeformDepotLabel"),TEXT("PARTS STORAGE\nEQUIPMENT CHECKOUT"),DepotAt+FVector(0,1.45,-.3),90,3.0,Saro);
 if(FS47Obj Sign=Model(TEXT("FreeformReceivingLabel"),TEXT("/Game/EastWing/Props/Extras/SM_EW_ReceivingSign.SM_EW_ReceivingSign"),DeliveryAt+FVector(0,.9325*1.2/1.8,-.15),.42,Saro))M->Web->MarkUsable(Sign.I,TEXT("ff:delivery"));
 else LabelPart(TEXT("FreeformReceivingLabel"),TEXT("GARAGE RECEIVING\nUNPACK CARRIED SUPPLIES"),DeliveryAt+FVector(0,1.45,-.35),90,2.7,Saro);
 // Existing own-model REGULAR pump at z=2052.4; only its mechanical
 // counter windows receive small live account-meter plates. The adjacent
 // DIESEL body stays authored and is deliberately incompatible with the Jeep.
 if(M->Web->Obj(TEXT("world.town.proxies.pumps")))
 {
  const bool NewPump=InstallPumpModel(Road);
  const FVector At=PumpPoint()+FVector(0,NewPump?-.10:0,0);const double Face=NewPump?.27:.348,Height=NewPump?.18:.50,Width=NewPump?.26:.55;
  const FVector Left=At+FVector(Face,0,NewPump?-.19:0),Right=At+FVector(NewPump?Face:-Face,0,NewPump?.19:0);
  Box(TEXT("FreeformRegularMeterEast"),Left,FVector(.012,Height,Width),0x202821,Road,TEXT("ff:pump"));
  FS47Obj West=Box(TEXT("FreeformRegularMeterWest"),Right,FVector(.012,Height,Width),0x202821,Road);
  M->Web->MarkUsable(West.I,TEXT("ff:pump"));
  auto Meter=[&](const TCHAR* Name,double Side)
  {
   const auto* P=M->Web->Node(Road);auto* C=NewObject<UTextRenderComponent>(M->Web,Name);C->SetupAttachment(P->Comp.Get());C->SetMobility(EComponentMobility::Movable);
   C->SetWorldSize(NewPump?3.0:2.6);C->SetHorizontalAlignment(EHTA_Center);C->SetVerticalAlignment(EVRTA_TextCenter);C->SetTextRenderColor(FColor(229,226,186));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();
   C->SetWorldLocation(S47Three::ToUE((Side>0?Left:Right)+FVector(NewPump?.012:Side*.012,0,0)));C->SetWorldRotation(FRotator(0,NewPump||Side>0?0:180,0));Parts.Add(M->Web->ObjAt(M->Web->AdoptNode(C,Road,Name)));return C;
  };
  PumpTextEast=Meter(TEXT("FreeformRegularTextEast"),1);PumpTextWest=Meter(TEXT("FreeformRegularTextWest"),-1);
 }
 bBuilt=Bench&&Shop&&Panel&&FeederPanel;if(Carry)Carry->Configure(ParcelSaro,ParcelRoad);Apply();return bBuilt;
}

void FS47FreeformWorld::Apply()
{
 if(!bBuilt)return;ApplyPump();for(FS47Obj O:Parts)if(O&&!(O==ParcelSaro)&&!(O==ParcelRoad)){O.SetVisible(bActive);if(const auto* N=O.W->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(bActive&&C->IsA<UStaticMeshComponent>()?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
 if(PumpModel){PumpModel.SetVisible(bActive&&bTripoPump);if(!bTripoPump)if(const auto* N=PumpModel.W->Node(PumpModel.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
 for(int32 I=0;I<2;++I)if(LoggerPosts[I]){const bool Show=bActive&&LoggerAccess();LoggerPosts[I].SetVisible(Show);if(const auto* N=M->Web->Node(LoggerPosts[I].I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(Show?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
 if(LoggerBody){const bool Show=bActive&&Logger.Site>=0;LoggerBody.SetVisible(Show);if(const auto* N=M->Web->Node(LoggerBody.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())){C->SetCollisionEnabled(Show?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);if(Show)C->SetWorldLocation(S47Three::ToUE(LoggerPoint(Logger.Site)));}}
 for(int32 I=0;I<FS47ArrayWear::Branches().Num()&&I<4;++I)if(WearLamps[I])
 {
  const FS47ArrayWear::FBranch& B=FS47ArrayWear::Branches()[I];const bool Down=bActive&&Wear.bActive&&!Wear.IsRestored(B.Id);
  WearLamps[I].SetVisible(Down);
  if(const auto* N=M->Web->Node(WearLamps[I].I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  if(Down){WearLamps[I].SetColor(S47Three::Hex(0xd8321e));WearLamps[I].SetEmissive(S47Three::Hex(0xd8321e),Wear.Job==B.Id||Wear.NextBranch()==&B?1.f:.6f);} // D45-20261110: brighter on the next one
 }
 for(int32 I=0;I<FS47Workshop::TraySize;++I)if(TrayParts[I])
 {
  const auto* P=Crafting.Tray.IsValidIndex(I)?&Crafting.Tray[I]:nullptr;const bool Show=bActive&&P;TrayParts[I].SetVisible(Show);
  if(const auto* N=M->Web->Node(TrayParts[I].I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(Show?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
  if(P)TrayParts[I].SetColor(S47Three::Hex(P->bTested&&P->bFaulty?0x8a3b2e:P->Usable()?0x5e8b61:P->Kind==TEXT("housing")?0x6d6f68:P->Kind==TEXT("recorder")?0x3f4a52:P->Kind==TEXT("timer")?0x6b6240:0x4c4238));
 }
 if(BenchSample)BenchSample.SetVisible(bActive&&(InventoryCount(TEXT("fragment"))>0||InventoryCount(TEXT("archived_fragment"))>0||BenchJob==TEXT("fragment")));
 if(BenchLamp){const bool Testing=!BenchJob.IsEmpty()||bLoggerCalibrating;BenchLamp.SetColor(S47Three::Hex(Testing?0xc8a046:0x5e8b61));BenchLamp.SetEmissive(S47Three::Hex(Testing?0xc8a046:0x5e8b61),Testing?.2f+float((bLoggerCalibrating?Logger.Calibration:BenchSeconds)/3.)*.8f:.15f);}
 ApplyServiceDoor();
 if(Relay)Relay.SetColor(S47Three::Hex(Service>=5?0x739470:0xb19651));
 if(Selector)
 {
  Selector.SetColor(S47Three::Hex(HasArrayFault()?0xb54e31:0x4e8d5c));
  // AdoptNode retains position, not native scale. Rotate the actual component
  // so the fitted 4.5 cm selector cannot become a one-metre interaction blocker.
  if(const auto* N=Selector.W->Node(Selector.I))if(auto* C=N->Comp.Get())
   C->SetWorldRotation(FRotator(0,-FMath::RadiansToDegrees(DoorYaw),Service>=3&&Service<=8?FMath::RadiansToDegrees(.65):0));
 }
 if(FeederLamp)FeederLamp.SetColor(S47Three::Hex(M&&M->Freeform&&M->Freeform->PowerFault()?0xc57d32:0x58a769));
 if(FeederLever)FeederLever.SetRotX(Feeder>=2&&Feeder<5?.55:0);
 const bool Road=M&&M->World&&(M->World->Area==TEXT("road")||M->World->Area==TEXT("diner"));
 if(Carry)Carry->Step(0,bActive?Payload:0,Item,Payload==1?ParcelShopPoint():Payload==3?TruckPoint():Dropped);
 for(auto Pair:TArray<TPair<FS47Obj,bool>>{{CargoSaro,!Road},{CargoRoad,Road}})if(Pair.Key)
 {
  const bool Show=bActive&&Pair.Value&&M&&M->World&&!M->World->bDriving;if(const auto* N=Pair.Key.W->Node(Pair.Key.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())){C->SetWorldLocation(S47Three::ToUE(TruckPoint()));C->SetCollisionEnabled(Show?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);C->SetRenderInMainPass(false);C->SetRenderInDepthPass(false);C->SetCastShadow(false);C->SetVisibleInRayTracing(false);}Pair.Key.SetVisible(Show);
 }
 ApplyPilots();
}
void FS47FreeformWorld::ApplyPilots()
{
 // D45-20261110 (Tom: every box out there shows green): each drive cabinet's pilot says what its antenna is doing. Green in service,
 // red out of service, amber under local work or test, red blinking on the one the operator has made next. Set only on a change.
 // While SARO is dark FS47SaroPower owns these lamps (a colour set then would become the lamp's own); the first lit frame catches up.
 if(!bActive||!M||!M->Web||!M->Freeform)return;
 if(!M->Freeform->HasPowerForReactiveLight()||(IsValid(M->SaroNight)&&M->SaroNight->Power&&M->SaroNight->Power->Out()))return;
 FS47GlowPoints* G=M->Web->Glow(TEXT("world.field.glow.points"));if(!G||G->Num()==0)return;
 const int32 N=FMath::Min(M->Web->DishCount(),27);
 if(PilotGlow!=G)
 {
  // The pilot nearest each cabinet (the export adds them in antenna order; S-03's sits on its service box)
  const FS47GlowView View(M->Web,TEXT("world.field.glow.points"));
  for(int32 I=0;I<27;++I)
  {
   PilotOf[I]=-1;PilotShown[I]=-1;if(I>=N)continue;
   const FVector Cab=InteractionPoint(TEXT("field:")+M->Web->DishId(I));double Best=3.;
   if(!Cab.IsNearlyZero())for(int32 K=0;K<G->Num();++K){const FVector At=View.Pos(K);const double D=FVector2D(At.X-Cab.X,At.Z-Cab.Z).Size();if(D<Best){Best=D;PilotOf[I]=K;}}
   if(PilotOf[I]<0&&G->Num()==N)PilotOf[I]=I;
   const int32 Pk=PilotOf[I];if(Pk<0)continue;
   PilotBase[I]=View.Col(Pk);PilotSize[I]=G->Size.IsValidIndex(Pk)?G->Size[Pk]:.14f;PilotBlink[I]=G->Blink.IsValidIndex(Pk)?G->Blink[Pk]:FVector2D::ZeroVector;
  }
  PilotGlow=G;
 }
 static const FLinearColor PilotRed=S47Three::Hex(0xff3424),PilotAmber=S47Three::Hex(0xffa040),PilotGreen=S47Three::Hex(0x7dff9a);
 const FS47ArrayWear::FBranch* Next=Wear.NextBranch();
 const FS47ArrayWear::FBranch* Working=Wear.Job.IsEmpty()?nullptr:FS47ArrayWear::Branch(Wear.Job);
 const bool bRelayNext=HasArrayFault()&&Service<=2&&(M->Freeform->ServiceGuideFocused()||!Next); // as MAINTENANCE lists it first
 for(int32 I=0;I<N;++I)
 {
  const int32 Pk=PilotOf[I];if(Pk<0||Pk>=G->Num())continue;
  const FString Id=M->Web->DishId(I);
  int32 Want=Wear.BranchOfDish(Id)||!M->Web->DishRemoteAvailable(I)?1:0;
  if(bRelayNext?Id==ServiceTarget:Next&&Id==Next->Junction)Want=3;
  if((Id==ServiceTarget&&Service>=3&&Service<=8)||(Working&&Id==Working->Junction))Want=2;
  if(Want==PilotShown[I])continue;
  PilotShown[I]=int8(Want);
  G->SetColor(Pk,Want==0?(Id==TEXT("S-03")?PilotGreen:PilotBase[I]):Want==2?PilotAmber:PilotRed);
  G->SetBlink(Pk,Want==3?FVector2D(4.2,0):PilotBlink[I]);
  G->SetSize(Pk,Want==0?PilotSize[I]:FMath::Max(PilotSize[I],.20f)); // a lit fault reads from the apron, not only up close
 }
}
void FS47FreeformWorld::RestorePilots()
{
 // D45-20261110: outside Freeform (Story, the title) the pilots are as exported again
 FS47GlowPoints* G=M&&IsValid(M->Web)?M->Web->Glow(TEXT("world.field.glow.points")):nullptr;
 for(int32 I=0;I<27;++I)
 {
  const int32 Pk=PilotOf[I];
  if(G&&G==PilotGlow&&PilotShown[I]>=0&&Pk>=0&&Pk<G->Num()){G->SetColor(Pk,PilotBase[I]);G->SetBlink(Pk,PilotBlink[I]);G->SetSize(Pk,PilotSize[I]);}
  PilotShown[I]=-1;
 }
 PilotGlow=nullptr;
}

void FS47FreeformWorld::TrackPowerFault(bool bFault)
{
 // A repair loan belongs to one actual outage, not to a calendar day. Older
 // saves acquire an identity for their current outage without granting stock.
 if(bFault&&(!bTrackedPowerFault||PowerFaultSerial==0))PowerFaultSerial=FMath::Min(1000000000,PowerFaultSerial+1);
 bTrackedPowerFault=bFault;
}
bool FS47FreeformWorld::CanIssueEmergencyFuse(bool bFault) const
{
 return bFault&&PowerFaultSerial>0&&EmergencyFuseFault!=PowerFaultSerial&&
  Feeder!=4&&InventoryCount(TEXT("fuse"))==0&&Payload==0;
}
bool FS47FreeformWorld::EmergencyFuseAvailable() const
{
 return M&&M->Freeform&&CanIssueEmergencyFuse(M->Freeform->PowerFault());
}
bool FS47FreeformWorld::RequestEmergencyFuse()
{
 if(!AtShopCounter())return false;
 TrackPowerFault(M->Freeform->PowerFault());
 if(!CanIssueEmergencyFuse(M->Freeform->PowerFault()))
 {
  Change(Payload!=0?TEXT("Finish or recover your existing parcel first. The hatch handles one physical parcel at a time."):
   InventoryCount(TEXT("fuse"))>0||Feeder==4?TEXT("A control fuse is already retained or fitted. Use it to finish the feeder repair."):
   !M->Freeform->PowerFault()?TEXT("The feeder has no active supply fault."):
   TEXT("The emergency fuse for this outage has already been issued. Recover its parcel or use the retained fuse."));
  return false;
 }
 if(!M->Freeform->ChargeEmergencyFeederFuse()){Change(TEXT("The requisition balance cannot cover another $5. No parcel was issued or budget charged."));return false;}
 EmergencyFuseFault=PowerFaultSerial;Item=TEXT("fuse");Contents.Reset();Contents.Add(Item,1);Payload=1;bParcelWasLoaded=false;
 Change(TEXT("One feeder-repair fuse, labeled for SARO: $5 on the budget or the requisition. Collect the parcel, unpack it at SARO and repair the feeder."));return true;
}
int32 FS47FreeformWorld::ShopPrice(const FString& Supply){return Price(Supply);}
FString FS47FreeformWorld::SupplyTitle(const FString& Supply){return ItemTitle(Supply);}
bool FS47FreeformWorld::TakePostedParcel(const TMap<FString,int32>& Posted,const FVector& At,const FString& Said)
{
 if(!Ready()||Payload!=0||Posted.IsEmpty()||!M->World||M->World->bDriving)return false;
 for(const auto& Pair:Posted)if(Price(Pair.Key)<=0||Pair.Value<1||Pair.Value>99)return false;
 Contents=Posted;Item.Empty();
 for(const TCHAR* Key:AllItems())if(Contents.Contains(Key)){Item=Key;break;}
 Payload=2;bParcelWasLoaded=false;if(Carry)Carry->Restore(nullptr,At);
 Change(Said.IsEmpty()?FString(TEXT("Parcel from the mailbox carried. Load it at the truck or unpack it at GARAGE RECEIVING. Use parcel handling to set it down or drop it safely.")):Said);
 return true;
}
bool FS47FreeformWorld::Purchase(const FString& Id,bool Emergency)
{
 if(!AtShopCounter()||Price(Id)<=0||(Emergency?Payload!=0:!CanAddShopItem()))return false;
 if(InventoryCount(Id)+Contents.FindRef(Id)>=99){Change(TEXT("SARO stock plus this paid parcel has reached the 99-item storage limit for that supply."));return false;}
 // M7 (Tom, 9 November): one parcel is what the truck's rack takes (FS47Outfit: eight items, twenty with the bed rack)
 if(!Emergency&&Payload==1&&ParcelCount()>=ParcelRoom()){Change(FString::Printf(TEXT("The parcel is as big as the truck's rack takes: %d items.%s"),ParcelRoom(),ParcelRoom()<FS47Outfit::RackLoad?TEXT(" A bed rack (OPS stores list) takes more."):TEXT("")));return false;}
 if(!Emergency&&!ShopOpen()){Change(TEXT("The shop counter sells supplies 08:00–20:00. Essential credit requisitions remain available here after hours."));return false;}
 if(Emergency&&((Id==TEXT("donor")||Id==TEXT("film")||Id==TEXT("recorder_donor")||Id==TEXT("cell")||Id==TEXT("foil")||Id==TEXT("drive_motor"))||EmergencyDay==M->Freeform->Day())){Change(TEXT("One essential emergency parcel per day; donor equipment is sold during opening hours."));return false;}
 // A coupon clipped from this week's Ledger takes something off (FS47Mail); never off an emergency requisition.
 const int32 Cost=!Emergency&&M->Freeform->Mail?M->Freeform->Mail->CounterPrice(Id,Price(Id)):Price(Id);
 if(!M->Freeform->Spend(Cost,Emergency)){Change(M->Freeform->FeedbackText());return false;}
 if(Emergency)EmergencyDay=M->Freeform->Day();if(Payload==0){Item=Id;Contents.Reset();}++Contents.FindOrAdd(Id);Payload=1;bParcelWasLoaded=false;Change(TEXT("Paid and added to your counter parcel. Add more supplies or close the list and collect the sealed parcel."));return true;
}
int32 FS47FreeformWorld::ParcelCount() const{int32 N=0;for(const auto& Pair:Contents)N+=FMath::Max(0,Pair.Value);return N;}
int32 FS47FreeformWorld::ParcelRoom() const{return FS47Outfit::TuneFor(M?M->Freeform.Get():nullptr).Load;}
bool FS47FreeformWorld::CanAddShopItem() const
{
 return Payload==0||(Payload==1&&Price(Item)>0&&!Contents.IsEmpty());
}
FString FS47FreeformWorld::ParcelDescription() const
{
 if(Contents.IsEmpty())return Payload>0?ItemTitle(Item):FString(TEXT("empty"));
 TArray<FString> Lines;
 for(const TCHAR* Key:AllItems())if(const int32 Count=Contents.FindRef(Key);Count>0)Lines.Add(FString::Printf(TEXT("%d x %s"),Count,*ItemTitle(Key)));
 return FString::Join(Lines,TEXT(", "));
}
void FS47FreeformWorld::OpenShop()
{
 if(!AtShopCounter()||!M->UI)return;TArray<S47Kit::FW> Rows;
 TrackPowerFault(M->Freeform->PowerFault());
 Rows.Add(S47Kit::Paper(FString::Printf(TEXT("SUPPLIES / 08:00–20:00 / BUDGET $%d\nBuy items here, then close this list and collect your parcel.\nLoad the Truck; unpack at GARAGE RECEIVING to add stock.\nTHE TRUCK'S RACK TAKES %d ITEMS IN ONE PARCEL"),M->Freeform->Budget(),ParcelRoom())));
 if(Payload>0)Rows.Add(S47Kit::Paper(TEXT("PAID PARCEL / ")+ParcelDescription()));
 if(!CanAddShopItem())Rows.Add(S47Kit::Small(TEXT("The counter handles one parcel at a time, and you already have one out. GARAGE RECEIVING: Unpack supplies. CALIBRATION / REPAIR: Return a service case or logger. Then buy here again.")));
 if(Fuel<=.001)Rows.Add(S47Kit::Small(TEXT("TANK EMPTY: A fuel can from this counter must be unpacked at SARO before it can be poured. Roadside return from the truck costs money and a long wait.")));
 for(const TCHAR* Id:Items)
 {
  const FString Key(Id);
  const int32 Cost=M->Freeform->Mail?M->Freeform->Mail->CounterPrice(Key,Price(Key)):Price(Key);
  const FString Tag=Cost<Price(Key)?FString::Printf(TEXT("%s / $%d WITH THE LEDGER'S COUPON (was $%d)"),*ItemTitle(Key),Cost,Price(Key)):FString::Printf(TEXT("%s / $%d"),*ItemTitle(Key),Price(Key));
  Rows.Add(S47Kit::Button(Tag,[this,Key]{Purchase(Key);OpenShop();},false,CanAddShopItem()&&ShopOpen()&&InventoryCount(Key)+Contents.FindRef(Key)<99&&(Payload!=1||ParcelCount()<ParcelRoom())));
 }
 Rows.Add(S47Kit::Small(TEXT("Essential after-hours/credit requisition: one per day, charged against future work, and only with no requisition balance already standing. Donor controllers require bench testing before harvesting their relay.")));
 for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("fuel")}){const FString Key(Id);Rows.Add(S47Kit::Button(FString(TEXT("Emergency "))+ItemTitle(Key),[this,Key]{if(Purchase(Key,true))M->UI->Close();},false,Payload==0&&EmergencyDay!=M->Freeform->Day()));}
 if(M->Freeform->PowerFault())
 {
  Rows.Add(S47Kit::Small(TEXT("ACTIVE FEEDER FAULT: one repair fuse may be charged to the requisition even with an older balance or today's essential parcel already used. Use existing stock or a paid parcel first.")));
  Rows.Add(S47Kit::Button(TEXT("Request feeder-repair fuse / $5 / credit available"),[this]{if(RequestEmergencyFuse())M->UI->Close();},true,EmergencyFuseAvailable()));
 }
 // The attendant's one thing today, or who is minding the counter while he is gone (TOWNSFOLK-20261108, S47Townsfolk).
 if(M->Freeform->Townsfolk){const FString Said=M->Freeform->Townsfolk->CounterLine(TEXT("attendant"));if(!Said.IsEmpty())Rows.Add(S47Kit::Small(Said));}
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("CALICHE / SHOP COUNTER"),S47Kit::Col(Rows),880));
}

FVector FS47FreeformWorld::ParcelPosition() const { return Carry?Carry->Position():Dropped; }
bool FS47FreeformWorld::ParcelSimulating() const { return Carry&&Carry->Simulating(); }
bool FS47FreeformWorld::CarryBlocked() const { return Carry&&Carry->Blocked(); }
int32 FS47FreeformWorld::ParcelImpactCount() const { return Carry?Carry->Impacts():0; }
bool FS47FreeformWorld::DropParcel(bool bDrop)
{
 if(!Ready()||Payload!=2||!Carry||!Carry->Release(bDrop))
 {Change(TEXT("No clear firm surface within reach. Keep hold of the parcel and move away from the obstruction."));return false;}
 Payload=4;Dropped=Carry->Position();Change(bDrop?TEXT("Parcel released. It remains yours; pick it up again where it lands."):TEXT("Parcel set down on the measured surface. Pick it up to continue the delivery."));return true;
}
void FS47FreeformWorld::OpenCarryMenu()
{
 if(!Ready()||Payload!=2||!M->UI)return;
 TArray<S47Kit::FW> R;R.Add(S47Kit::Paper(ParcelDescription()+TEXT("\nHeld in both hands. The package remains part of the same paid delivery.")));
 R.Add(S47Kit::Button(TEXT("Set down on a clear surface"),[this]{M->UI->Close();DropParcel(false);},true));
 R.Add(S47Kit::Button(TEXT("Drop from your hands"),[this]{M->UI->Close();DropParcel(true);}));
 R.Add(S47Kit::Small(TEXT("A heavier package slows walking and trails a turn. Walls stop it. Loading the truck secures it for travel.")));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("CARRIED PARCEL"),S47Kit::Col(R),800));
}

bool FS47FreeformWorld::ParcelUse()
{
 if(!Payload||Payload==2)return true;if(Payload==1&&!AtShopCounter())return true;if(!Near(InteractionPoint(TEXT("ff:parcel")),2.8))return true;
 Payload=2;Change(TEXT("Parcel carried. Load it at the truck or unpack at SARO. Use parcel handling to set it down or drop it safely."));return true;
}
bool FS47FreeformWorld::CargoUse()
{
 if(M&&M->World&&M->World->bDriving&&M->Freeform&&M->Freeform->Care&&M->Freeform->Care->NeedsRescue()){Change(TEXT("The truck is in trouble. Get out first (the exit key), then use the cargo rack on the back: the spade, the jack and the county tow are all there."));return true;}
 if(!M||!M->World||M->World->bDriving||!Near(M->World->ServicePosition(),4.8)){Change(TEXT("Move closer to your parked service truck to load or unload its cargo."));return true;}
 const auto Unload=[this]{Payload=2;Change(TEXT("GARAGE RECEIVING: Parcel unloaded. Carry supplies to the receiving table to add them to stock. Return loan equipment at CALIBRATION / REPAIR."));};
 if(Payload==2){Payload=3;bParcelWasLoaded=true;Change(TEXT("Parcel secured in the service-truck cargo rack. It travels with the truck."));}
 else if(Payload==3&&Fuel>.001&&!(M->Freeform->Care&&M->Freeform->Care->NeedsRescue()))Unload();
 else
 {
  // A loaded rack on an empty tank still opens this menu: roadside return carries the truck and its cargo.
  TArray<S47Kit::FW> R;R.Add(S47Kit::Section(TEXT("FUEL"),FString::Printf(TEXT("%.1f / %.0f"),Fuel,TankCap())));
  R.Add(S47Kit::Section(TEXT("CARGO")));
  R.Add(S47Kit::Status(TEXT("Parcel"),Payload==3?TEXT("LOADED"):TEXT("EMPTY"),Payload==3?1:0,FString(),false,Payload==3));
  if(Payload==3)
  {
   TArray<TPair<FString,FString>> Lines;
   for(const TCHAR* Key:AllItems())if(const int32 Count=Contents.FindRef(Key);Count>0)Lines.Add({ItemTitle(Key),FString::FromInt(Count)});
   if(!Lines.IsEmpty())R.Add(S47Kit::Table(TEXT("CONTENTS"),TEXT("QTY"),Lines));
  }
  R.Add(S47Kit::Button(TEXT("Refuel from a stock can"),[this]{RefuelTruck();M->UI->Close();},true,InventoryCount(TEXT("fuel"))>0&&Fuel<TankCap()-.01));
  // The truck's own trouble: what the player can do about it, and the county if not.
  const auto* Care=M->Freeform->Care.Get();const bool bCare=Care&&Care->NeedsRescue();
  if(bCare)R.Add(S47Kit::Paper(Care->Steps(InventoryCount(TEXT("spare")),InventoryCount(TEXT("water")))));
  if(Care)
  {
   R.Add(S47Kit::Section(TEXT("RADIATOR"),FString::Printf(TEXT("%.0f%%"),Care->Water()*100)));
   R.Add(S47Kit::Section(TEXT("SPARE WHEEL"),FString::Printf(TEXT("%d"),InventoryCount(TEXT("spare")))));
   if(!Care->StatusLine().IsEmpty())R.Add(S47Kit::Small(Care->StatusLine()));
   if(Care->Stuck())R.Add(S47Kit::Button(FString::Printf(TEXT("Dig it out with the spade / about %.0f minutes"),Care->DigTime()),[this]{M->Freeform->Care->DigOut();M->UI->Close();},true));
   if(Care->Flat())R.Add(S47Kit::Button(FString::Printf(TEXT("Jack it up and change the wheel / about %.0f minutes"),FS47TruckCare::WheelMinutes),[this]{M->Freeform->Care->ChangeWheel();M->UI->Close();},true,InventoryCount(TEXT("spare"))>0));
   if(Care->Water()<1)R.Add(S47Kit::Button(TEXT("Fill the radiator from a can"),[this]{M->Freeform->Care->TopUp();M->UI->Close();},Care->Hot(),InventoryCount(TEXT("water"))>0));
  }
  // What the county will charge and how long it will be, by how far out and the hour (RoadsideAssist charges the same).
  const auto Terms=RoadsideTermsFor(bCare,Care?Care->TowFee():FS47TruckCare::TowFeeFor(0),Care?Care->TowMinutes():FS47TruckCare::TowMinutesFor(0),DayClock(M->Freeform->Clock())/3600.,FString());
  R.Add(S47Kit::Button(bCare?FString::Printf(TEXT("Call the county tow to SARO / $%d / about %.0f minutes"),Terms.Fee,Terms.Minutes):FString::Printf(TEXT("Request roadside return to SARO / $%d / about %.0f minutes"),Terms.Fee,Terms.Minutes),[this]{RoadsideAssist();M->UI->Close();},false,Fuel<=.001||bCare));
  if(Payload==3)R.Add(S47Kit::Button(TEXT("Unload parcel from the rack"),[this,Unload]{Unload();M->UI->Close();}));
  R.Add(S47Kit::Small(bCare?FString::Printf(TEXT("The county tow brings you, the truck and its cargo back to SARO and leaves it sound. $%d, paid from the budget if it covers it, otherwise on the weekly sheet; it takes about %.0f minutes. Mending it yourself costs only time."),Terms.Fee,Terms.Minutes):Fuel>.001?FString(TEXT("Roadside return is available when the truck is out of fuel, out of water or stuck.")):FString::Printf(TEXT("TANK EMPTY: County roadside service brings you, the truck and its cargo back to SARO with a reserve in the tank. $%d for the call-out, paid from the budget if it covers it, otherwise on the weekly sheet; about %.0f minutes. After dark the one wrecker on duty is slow to come. A fuel can from stock costs nothing."),Terms.Fee,Terms.Minutes)));
  M->UI->Open(S47Kit::Shell(M->UI,TEXT("SERVICE TRUCK / CARGO"),S47Kit::Col(R),800));
 }
 return true;
}
bool FS47FreeformWorld::Deposit(const FString& Station)
{
 const FVector At=Station==TEXT("ff:delivery")?DeliveryAt:Station==TEXT("ff:depot")?DepotAt:BenchAt;
 if(!Near(At,2.6))return true;
 if(Payload==2&&Item==TEXT("fieldlogger")&&Station!=TEXT("ff:bench"))
 {Change(TEXT("CALIBRATION / REPAIR: Return the loan logger and its original tape at the workbench, then compare there."));return true;}
 if(Payload==2&&Item==TEXT("service_case")&&Station!=TEXT("ff:bench"))
 {Change(TEXT("CALIBRATION / REPAIR: Return the service case at the bench."));return true;}
 if(Payload==2&&Station!=TEXT("ff:delivery")&&Item!=TEXT("fieldlogger")&&Item!=TEXT("service_case"))
 {Change(TEXT("GARAGE RECEIVING: Carry purchased supplies to the labeled receiving table to unpack them."));return true;}
 if(Payload==2&&Item==TEXT("fieldlogger")){Payload=0;Item.Empty();Contents.Reset();bParcelWasLoaded=false;Logger.bCheckedOut=false;Logger.Returns=FMath::Min(1000000,Logger.Returns+1);Change(Logger.bOwned?FString(TEXT("CALIBRATION / REPAIR: Logger and any retained original returned to the bench; bench record updated. Prepare, collect or compare here.")):FString(TEXT("CALIBRATION / REPAIR: Logger and any retained original returned to the bench; loan record updated. Prepare, collect or compare here.")));OpenWorkbench();return true;}
 if(Payload==2&&Item==TEXT("service_case")){Payload=0;Item.Empty();bParcelWasLoaded=false;Change(TEXT("Reusable service case returned to the bench. Stock and budget are unchanged."));return true;}
 if(Payload==2)
 {
  if(Contents.IsEmpty()){Change(TEXT("This parcel has no verified paid contents; no stock was added."));return true;}
  for(const auto& Pair:Contents)if(InventoryCount(Pair.Key)+Pair.Value>99){Change(TEXT("Not enough stock space for the whole parcel. Use existing supplies first; your paid parcel remains intact."));return true;}
  const FString Description=ParcelDescription();
  Delivered=FMath::Min(1000000,Delivered+1);
  if(bParcelWasLoaded){DeliveredByTruck=FMath::Min(1000000,DeliveredByTruck+1);if(Contents.FindRef(TEXT("relay"))>0||Contents.FindRef(TEXT("donor"))>0)ServiceDeliveries=FMath::Min(1000000,ServiceDeliveries+1);}
  // Commit the whole paid contents before one autosave; no partial parcel credit.
  for(const auto& Pair:Contents)Stock.FindOrAdd(Pair.Key)+=Pair.Value;
  Contents.Reset();bParcelWasLoaded=false;Payload=0;Item.Empty();
  Change(TEXT("Delivered to SARO stock: ")+Description+TEXT(". Parcel unpacked."));return true;
 }
 if(Station==TEXT("ff:depot"))OpenDepot();
 else if(Station==TEXT("ff:bench"))OpenWorkbench();
 else Change(TEXT("GARAGE RECEIVING: Unload the parked Truck, then carry the parcel to this table. Stored parts are in PARTS STORAGE."));
 return true;
}
bool FS47FreeformWorld::StartDonorTest()
{
 if(!Ready()||!Near(BenchAt)||!BenchJob.IsEmpty()||InventoryCount(TEXT("donor"))<=0)return false;
 DonorTest();return BenchJob==TEXT("donor");
}
void FS47FreeformWorld::DonorTest()
{
 if(!Ready()||!Near(BenchAt)||!BenchJob.IsEmpty()||!Consume(TEXT("donor")))return;
 BenchJob=TEXT("donor");BenchSeconds=0;M->UI->Close();Change(TEXT("CALIBRATION / REPAIR: Wait for the automatic three-second donor relay test; its result will be added to stock."));
}
bool FS47FreeformWorld::AnalyzeSample()
{
 if(!Ready()||!Near(SampleAt,2.6)||!BenchJob.IsEmpty()||M->Freeform->HasFinding(TEXT("sample-analysis"))||InventoryCount(TEXT("fragment"))<=0)return false;
 if(!Consume(TEXT("fragment")))return false;
 BenchJob=TEXT("fragment");BenchSeconds=0;M->UI->Close();Change(TEXT("TEST LAB: Field fragment placed in the sample holder. Reference and conductivity comparison runs automatically."));return true;
}
void FS47FreeformWorld::OpenWorkbench()
{
 if(!Ready()||!Near(BenchAt)||!M->UI)return;
 TArray<S47Kit::FW> R;R.Add(S47Kit::Paper(FString::Printf(TEXT("SARO / EAST WING / CALIBRATION BENCH\nFUSES %d / RELAYS %d / DONORS %d / FUEL CANS %d / FILM %d\nGENERATOR %.1f h / TRUCK %.1f / %.0f\n%s"),InventoryCount(TEXT("fuse")),InventoryCount(TEXT("relay")),InventoryCount(TEXT("donor")),InventoryCount(TEXT("fuel")),InventoryCount(TEXT("film")),GeneratorHours(),Fuel,TankCap(),BenchJob.IsEmpty()?TEXT("Bench ready"):BenchJob==TEXT("fragment")?TEXT("Field sample comparison running"):TEXT("Controller relay test running"))));
 R.Add(S47Kit::Small(Logger.bOwned?FString::Printf(TEXT("FIELD LOGGER / operator-built, two field sites, %.0f-hour battery. Measures local electrical noise only."),Logger.MaxBattery()/3600.):LoggerAccess()?TEXT("FIELD LOGGER AUTHORIZED / loan kit, two field sites, six-hour battery. Measures local electrical noise only."):TEXT("FIELD LOGGER LOCKED / complete a supported conclusion in The repeated interval.")));
 if(LoggerAccess()&&!Logger.bLoanIssued)R.Add(S47Kit::Small(TEXT("PARTS STORAGE: Collect the loan kit once at EQUIPMENT CHECKOUT, then carry it to this bench. All later preparation, collection, return and comparison happen here.")));
 if(LoggerAccess()&&Logger.bLoanIssued)
 {
  R.Add(S47Kit::Paper(FString::Printf(TEXT("fieldlogger01 / connections %d/3 / battery %.1f h / %.3f MHz / calibration %s\nSITE A: generator service path (8, 5). SITE B: east array edge (34, -28).\nTwo-hour recording starts on placement. Return to SARO and acquire a matching-frequency original while it runs; return the logger to this bench and compare its retained original here."),Logger.Assembly,Logger.Battery/3600.,Logger.Frequency,Logger.bCalibrated?TEXT("+2.4 s offset retained"):TEXT("required"))));
  const TCHAR* Connections[]={TEXT("Seat loan receiver in case"),TEXT("Connect shielded antenna lead"),TEXT("Connect charged loan battery")};
  if(Logger.Assembly<3&&!Logger.bCheckedOut)R.Add(S47Kit::Button(Connections[Logger.Assembly],[this]{LoggerAction(TEXT("connect"));},true));
  R.Add(S47Kit::Button(TEXT("Calibrate against bench clock / recharge battery"),[this]{LoggerAction(TEXT("calibrate"));},false,Logger.Assembly==3&&!Logger.bCheckedOut&&Logger.Site<0&&Payload==0&&!bLoggerCalibrating));
  const FString TuneBlock=LoggerPreparationBlock(TEXT("tune"));
  R.Add(S47Kit::Button(TEXT("Tune logger to selected SARO work card"),[this]{LoggerAction(TEXT("tune"));},false,TuneBlock.IsEmpty()));
  if(!TuneBlock.IsEmpty())R.Add(S47Kit::Small(TuneBlock));
  R.Add(S47Kit::Small(FString::Printf(TEXT("%s / %s / collections %d / bench returns %d"),Logger.bOwned?TEXT("BENCH RECORD / fieldlogger02"):TEXT("LOAN RECORD / fieldlogger01"),Logger.bCheckedOut?TEXT("OUT WITH OPERATOR"):TEXT("AT THIS BENCH"),Logger.Checkouts,Logger.Returns)));
  const FString TakeBlock=LoggerPreparationBlock(TEXT("take"));
  R.Add(S47Kit::Button(TEXT("Collect prepared field logger from bench"),[this]{LoggerAction(TEXT("take"));},false,TakeBlock.IsEmpty()));
  if(!TakeBlock.IsEmpty()&&TakeBlock!=TuneBlock)R.Add(S47Kit::Small(TakeBlock));
  R.Add(S47Kit::Button(TEXT("Read and compare retained field originals"),[this]{LoggerAction(TEXT("compare"));},false,!Logger.bCheckedOut&&Payload==0));
 }
 R.Add(S47Kit::Button(TEXT("Take reusable SARO service case"),[this]
 {
  if(!Ready()||!Near(BenchAt)||Payload!=0)return;Item=TEXT("service_case");Contents.Reset();Payload=2;bParcelWasLoaded=false;if(Carry)Carry->Restore(nullptr,BenchAt+FVector(0,1.05,0));M->UI->Close();Change(TEXT("SARO service case in hand. Set it down, carry it in the truck, or return it to this bench. It imposes no repair requirement."));
 },false,Payload==0));
 R.Add(S47Kit::Button(TEXT("Test donor controller and harvest compatible relay"),[this]{DonorTest();},true,InventoryCount(TEXT("donor"))>0&&BenchJob.IsEmpty()));
 R.Add(S47Kit::Button(TEXT("Open workshop / donors, parts and recipes"),[this]{OpenWorkshop();},false));
 R.Add(S47Kit::Small(TEXT("TEST LAB: Analyze field fragments. PARTS STORAGE: Inspect spare stock. PHOTO LAB: Load photographic film. GARAGE RECEIVING: Unpack carried supplies.")));
 R.Add(S47Kit::Paper(M->Freeform->MeasurementRecord()));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / CALIBRATION AND REPAIR"),S47Kit::Col(R),880));
}
FString FS47FreeformWorld::WorkshopBlock(const FString& Action,const FString& Arg) const
{
 if(!Ready()||M->Web->Area()!=TEXT("saro")||!Near(BenchAt))return TEXT("Use the CALIBRATION / REPAIR bench.");
 if(Payload!=0)return TEXT("Free both hands to work at the bench.");
 if(!BenchJob.IsEmpty()||bLoggerCalibrating)return TEXT("Wait for the bench to finish its current job.");
 if(Action==TEXT("unpack"))return InventoryCount(TEXT("recorder_donor"))<=0?TEXT("No used field recorder in stock. Caliche sells them; unpack the parcel at GARAGE RECEIVING."):!Crafting.CanUnpack()?TEXT("The tray needs room for four parts. Scrap what you no longer need."):TEXT("");
 if(Action==TEXT("testall"))return Crafting.Tray.ContainsByPredicate([](const FS47Workshop::FPart& P){return !P.bTested;})?TEXT(""):TEXT("Nothing left to test.");
 if(Action==TEXT("scrapdead"))return Crafting.Tray.ContainsByPredicate([](const FS47Workshop::FPart& P){return P.bTested&&P.bFaulty;})?TEXT(""):TEXT("No dead parts on the tray.");
 if(Action==TEXT("test")||Action==TEXT("scrap"))
 {
  const auto* P=Crafting.Part(FCString::Atoi(*Arg));if(!P)return TEXT("That part is no longer on the tray.");
  return Action==TEXT("test")&&P->bTested?TEXT("That part has already been tested."):TEXT("");
 }
 const auto* R=FS47Workshop::Recipe(Arg);if(!R)return TEXT("Unknown workshop job.");
 const bool LoggerOut=Logger.Site>=0||Logger.bCheckedOut;
 if(Action==TEXT("build"))
 {
  FString Missing=Crafting.Missing(*R,InventoryCount(TEXT("cell")),InventoryCount(TEXT("foil")));
  if(Arg==TEXT("geiger")&&!Crafting.IsBuilt(Arg)){const FString Tube=FS47Happenings::GeigerBench(M);if(!Tube.IsEmpty())Missing+=(Missing.IsEmpty()?FString():TEXT(", "))+Tube;}
  if(!Missing.IsEmpty())return TEXT("Needs: ")+Missing+TEXT(".");
  return LoggerOut?TEXT("Return the logger to this bench first."):TEXT("");
 }
 if(Action==TEXT("strip"))
 {
  if(!Crafting.IsBuilt(Arg))return TEXT("Not built.");
  if(LoggerOut)return TEXT("Return the logger to this bench first.");
  return Crafting.CanStrip(Arg)?TEXT(""):TEXT("The tray has no room for the returned parts.");
 }
 return TEXT("Unknown workshop job.");
}
bool FS47FreeformWorld::WorkshopAction(const FString& Action,const FString& Arg)
{
 const FString Why=WorkshopBlock(Action,Arg);
 if(!Why.IsEmpty()){if(Ready())Change(Why);return false;}
 if(Action==TEXT("scrapdead"))
 {
  const int32 N=Crafting.Tray.RemoveAll([](const FS47Workshop::FPart& P){return P.bTested&&P.bFaulty;});
  Change(FString::Printf(TEXT("CALIBRATION / REPAIR: %d dead part%s scrapped."),N,N==1?TEXT(""):TEXT("s")));return true;
 }
 if(Action==TEXT("scrap"))
 {
  const auto* P=Crafting.Part(FCString::Atoi(*Arg));const FString Title=P?FS47Workshop::PartTitle(P->Kind):FString();
  if(!Crafting.Scrap(FCString::Atoi(*Arg)))return false;
  Change(TEXT("CALIBRATION / REPAIR: ")+Title+TEXT(" scrapped; its tray slot is free."));return true;
 }
 if(Action==TEXT("unpack")&&!Consume(TEXT("recorder_donor")))return false;
 BenchJob=TEXT("ws:")+Action+(Arg.IsEmpty()?FString():TEXT(":")+Arg);BenchSeconds=0;
 Change(Action==TEXT("unpack")?TEXT("CALIBRATION / REPAIR: Stripping the used recorder down to its parts."):Action==TEXT("test")||Action==TEXT("testall")?TEXT("CALIBRATION / REPAIR: Parts on the test set; the readings settle in three seconds."):Action==TEXT("build")?TEXT("CALIBRATION / REPAIR: Assembling and bench-checking; three seconds."):TEXT("CALIBRATION / REPAIR: Taking it apart; three seconds."));
 return true;
}
void FS47FreeformWorld::FinishWorkshopJob(const FString& Job)
{
 TArray<FString> Bits;Job.ParseIntoArray(Bits,TEXT(":"));const FString Action=Bits.IsValidIndex(1)?Bits[1]:FString(),Arg=Bits.IsValidIndex(2)?Bits[2]:FString();
 if(Action==TEXT("unpack"))
 {
  if(!Crafting.Unpack()){AddItem(TEXT("recorder_donor"));Change(TEXT("CALIBRATION / REPAIR: No room on the tray; the recorder went back to stock."));return;}
  Change(TEXT("CALIBRATION / REPAIR: Four parts on the tray. Used parts can be dead; test each before relying on it."));
 }
 else if(Action==TEXT("testall"))
 {
  TArray<FString> Dead;int32 Good=0;
  for(const auto& P:TArray<FS47Workshop::FPart>(Crafting.Tray))if(!P.bTested&&Crafting.Test(P.Id)){if(P.bFaulty)Dead.Add(FS47Workshop::PartTitle(P.Kind));else ++Good;}
  Change(FString::Printf(TEXT("CALIBRATION / REPAIR: %d good on the test set."),Good)+(Dead.IsEmpty()?FString():TEXT(" DEAD: ")+FString::Join(Dead,TEXT(", "))+TEXT(".")));
 }
 else if(Action==TEXT("test"))
 {
  const int32 Id=FCString::Atoi(*Arg);if(!Crafting.Test(Id))return;const auto* P=Crafting.Part(Id);
  Change(TEXT("CALIBRATION / REPAIR: ")+FS47Workshop::PartTitle(P->Kind)+(P->bFaulty?TEXT(" reads DEAD on the test set. Scrap it to free the slot."):TEXT(" reads GOOD on the test set.")));
 }
 else if(Action==TEXT("build"))
 {
  const auto* R=FS47Workshop::Recipe(Arg);if(!R)return;const int32 Cells=InventoryCount(TEXT("cell")),Foil=InventoryCount(TEXT("foil"));
  if(!Crafting.Build(Arg,Cells,Foil)){Change(TEXT("CALIBRATION / REPAIR: Assembly abandoned; the parts are unchanged."));return;}
  for(int32 I=0;I<R->Cells;++I)Consume(TEXT("cell"));for(int32 I=0;I<R->Foil;++I)Consume(TEXT("foil"));
  if(R->Stock)AddItem(R->Stock);
  if(Arg==TEXT("own_logger")){Logger.bOwned=true;Logger.bLoanIssued=true;Logger.Assembly=3;Logger.bCalibrated=false;Logger.Calibration=0;Logger.Battery=0;}
  if(Arg==TEXT("battery_pack"))Logger.bBatteryPack=true;
  if(Arg==TEXT("shielded_lead"))Logger.bShieldedLead=true;
  M->Freeform->RecordEvidence(TEXT("workshop-")+Arg,FString(R->Title)+TEXT(" built at the SARO bench from tested donor parts."));
  Change(TEXT("CALIBRATION / REPAIR: ")+FString(R->Title)+TEXT(" built. ")+R->Effect);
 }
 else if(Action==TEXT("strip"))
 {
  const auto* R=FS47Workshop::Recipe(Arg);if(!R||!Crafting.Strip(Arg))return;
  if(Arg==TEXT("own_logger")){Logger.bOwned=false;Logger.bLoanIssued=false;Logger.Assembly=0;Logger.bCalibrated=false;Logger.Calibration=0;Logger.Battery=0;}
  if(Arg==TEXT("battery_pack")){Logger.bBatteryPack=false;Logger.Battery=FMath::Min(Logger.Battery,Logger.MaxBattery());}
  if(Arg==TEXT("shielded_lead"))Logger.bShieldedLead=false;
  Change(TEXT("CALIBRATION / REPAIR: ")+FString(R->Title)+TEXT(" taken apart. Its parts are back on the tray; spent cells and foil do not come back."));
 }
 if(bWorkshopResume&&M&&M->UI&&!M->UI->Modal()&&Near(BenchAt))OpenWorkshop();
 bWorkshopResume=false;
}
void FS47FreeformWorld::OpenWorkshop()
{
 if(!Ready()||!Near(BenchAt)||!M->UI)return;
 const int32 Cells=InventoryCount(TEXT("cell")),Foil=InventoryCount(TEXT("foil"));
 // The world is paused under a panel: step back for the bench job, then return here when it is done.
 auto Act=[this](const FString& Action,const FString& Arg){WorkshopAction(Action,Arg);if(BenchJob.IsEmpty())OpenWorkshop();else{M->UI->Close();bWorkshopResume=true;}};
 // Left: what is on the bench and the one thing to do with it.
 TArray<S47Kit::FW> L;L.Add(S47Kit::Section(TEXT("PARTS"),FString::Printf(TEXT("%d OF %d"),Crafting.Tray.Num(),FS47Workshop::TraySize)));
 int32 Untested=0,Dead=0;
 for(const auto& P:Crafting.Tray)
 {
  if(!P.bTested)++Untested;else if(P.bFaulty)++Dead;
  L.Add(S47Kit::Status(FS47Workshop::PartTitle(P.Kind),!P.bTested?TEXT("UNTESTED"):P.bFaulty?TEXT("DEAD"):TEXT("GOOD"),!P.bTested?0:P.bFaulty?-1:1));
 }
 if(Crafting.Tray.IsEmpty())L.Add(S47Kit::Small(TEXT("Nothing on the bench.")));
 if(Untested>0)L.Add(S47Kit::Button(FString::Printf(TEXT("Test all %d untested"),Untested),[Act]{Act(TEXT("testall"),FString());},true,WorkshopBlock(TEXT("testall")).IsEmpty()));
 if(Dead>0)L.Add(S47Kit::Button(FString::Printf(TEXT("Scrap %d dead"),Dead),[Act]{Act(TEXT("scrapdead"),FString());},false,WorkshopBlock(TEXT("scrapdead")).IsEmpty()));
 const FString StripBlock=WorkshopBlock(TEXT("unpack"));
 L.Add(S47Kit::Button(FString::Printf(TEXT("Strip a used recorder (%d)"),InventoryCount(TEXT("recorder_donor"))),[Act]{Act(TEXT("unpack"),FString());},Untested==0&&Dead==0&&Crafting.Tray.IsEmpty(),StripBlock.IsEmpty()));
 L.Add(S47Kit::Small(FString::Printf(TEXT("Stock: %d dry cells / %d foil."),Cells,Foil)));
 // Right: the known recipes, one in detail.
 TArray<S47Kit::FW> R;R.Add(S47Kit::Section(TEXT("BUILD")));
 const FS47Workshop::FRecipe* Pick=FS47Workshop::Recipe(WorkshopPick);if(Pick&&!Crafting.Known(*Pick))Pick=nullptr;
 for(const auto& Recipe:FS47Workshop::Recipes())if(Crafting.Known(Recipe))
 {
  if(!Pick)Pick=&Recipe;const FString Id(Recipe.Id);
  const FString State=Crafting.IsBuilt(Id)?TEXT("BUILT"):Crafting.Missing(Recipe,Cells,Foil).IsEmpty()&&(Id!=TEXT("geiger")||FS47Happenings::GeigerBench(M).IsEmpty())?TEXT("READY"):TEXT("NEEDS PARTS");
  // the recipe name left, its state in the right-hand column, as the parts list
  R.Add(S47Kit::Choice(FString(Recipe.Title),Pick==&Recipe,[this,Id]{WorkshopPick=Id;OpenWorkshop();},false,State));
 }
 if(!Pick)R.Add(S47Kit::Small(TEXT("Strip a recorder to see what its parts can become. The loan logger needs none of this.")));
 else
 {
  const FString Id(Pick->Id);R.Add(S47Kit::Small(Pick->Effect));
  if(Crafting.IsBuilt(Id))
  {
   TArray<FString> Back;for(const TCHAR* K:Pick->Parts)Back.Add(FS47Workshop::PartTitle(K));
   R.Add(S47Kit::Small(TEXT("Taking it apart returns ")+FString::Join(Back,TEXT(", "))+TEXT(". Cells and foil do not come back.")));
   R.Add(S47Kit::Button(TEXT("Take apart"),[Act,Id]{Act(TEXT("strip"),Id);},false,WorkshopBlock(TEXT("strip"),Id).IsEmpty()));
  }
  else
  {
   TArray<int32> Used;
   for(const TCHAR* K:Pick->Parts)
   {
    const auto* Good=Crafting.Tray.FindByPredicate([&](const FS47Workshop::FPart& P){return P.Kind==K&&P.Usable()&&!Used.Contains(P.Id);});
    if(Good)Used.Add(Good->Id);
    const bool Waiting=!Good&&Crafting.Tray.ContainsByPredicate([&](const FS47Workshop::FPart& P){return P.Kind==K&&!P.bTested;});
    R.Add(S47Kit::Status(FS47Workshop::PartTitle(K),Good?TEXT("HAVE"):Waiting?TEXT("TEST FIRST"):TEXT("MISSING"),Good?1:Waiting?0:-1));
   }
   if(Pick->Cells>0)R.Add(S47Kit::Status(TEXT("Dry cell"),FString::Printf(TEXT("NEED %d / HAVE %d"),Pick->Cells,Cells),Cells>=Pick->Cells?1:-1));
   if(Pick->Foil>0)R.Add(S47Kit::Status(TEXT("Shielding foil"),FString::Printf(TEXT("NEED %d / HAVE %d"),Pick->Foil,Foil),Foil>=Pick->Foil?1:-1));
   if(Id==TEXT("geiger")){const bool bTube=FS47Happenings::GeigerBench(M).IsEmpty();R.Add(S47Kit::Status(TEXT("Geiger-Muller tube"),bTube?TEXT("HAVE"):TEXT("FROM THE STORES (OPS)"),bTube?1:-1));}
   const FString Block=WorkshopBlock(TEXT("build"),Id);
   if(Block.IsEmpty())R.Add(S47Kit::Action(TEXT("Build"),[Act,Id]{Act(TEXT("build"),Id);}));else R.Add(S47Kit::Button(TEXT("Build"),[]{},false,false));
   if(Crafting.Missing(*Pick,Cells,Foil).IsEmpty()&&!Block.IsEmpty())R.Add(S47Kit::Small(Block));
  }
 }
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / WORKSHOP"),S47Kit::Two(S47Kit::Col(L),S47Kit::Col(R),.42f,.58f),980));
}
void FS47FreeformWorld::ApplyWear()
{
 if(!M||!M->Web||!Wear.bActive)return;
 for(int32 I=0;I<M->Web->DishCount();++I)if(Wear.BranchOfDish(M->Web->DishId(I)))
 {
  // Out of service, parked low and left: the state of the array reads from the ground.
  if(!WearPose.Contains(I))WearPose.Add(I,FVector2D(M->Web->DishAz(I),M->Web->DishEl(I)));
  M->Web->SetDishRemoteAvailable(I,false);M->Web->SnapDish(I,WearPose[I].X,12);
 }
}
FString FS47FreeformWorld::BranchNeeds(const FS47ArrayWear::FBranch& B) const
{
 TArray<FString> Out;
 for(const auto& N:B.Needs){const int32 Have=InventoryCount(N.Item);if(Have<N.Count)Out.Add(FString::Printf(TEXT("%d x %s (have %d)"),N.Count,*ItemTitle(N.Item),Have));}
 return FString::Join(Out,TEXT(", "));
}
void FS47FreeformWorld::OpenBranch(const FString& BranchId)
{
 const auto* B=FS47ArrayWear::Branch(BranchId);if(!Ready()||!B||!M->UI||Wear.IsRestored(BranchId))return;
 TArray<S47Kit::FW> R;R.Add(S47Kit::Banner(TEXT("BRANCH STATUS"),FString::Printf(TEXT("%d antenna%s out of service"),B->Dishes.Num(),B->Dishes.Num()==1?TEXT(""):TEXT("s")),B->Fault,TEXT("FAULT"),-1));
 for(const auto& N:B->Needs)
 {
  const int32 Have=InventoryCount(N.Item);const bool Enough=Have>=N.Count;
  const FString Needed=ItemTitle(N.Item);
  R.Add(S47Kit::Status(Needed.Left(1).ToUpper()+Needed.Mid(1),Enough?TEXT("READY"):TEXT("MISSING"),Enough?1:-1,FString::Printf(TEXT("Required: %d / in stock: %d"),N.Count,Have),true));
 }
 const FString Id(B->Id),Missing=BranchNeeds(*B);
 if(Missing.IsEmpty()&&Wear.Job.IsEmpty())R.Add(S47Kit::Action(TEXT("Repair and test this branch"),[this,Id]{RepairBranch(Id);}));else R.Add(S47Kit::Button(TEXT("Repair and test this branch"),[]{},false,false));
 if(FString(B->Id)==TEXT("centre"))R.Add(S47Kit::Small(TEXT("CALIBRATION / REPAIR: make up the splice in the workshop from two good antenna leads and foil.")));
 else if(!Missing.IsEmpty())R.Add(S47Kit::Small(TEXT("Caliche sells these. Unpack the parcel at GARAGE RECEIVING and bring them back in your pouch.")));
 M->UI->Open(S47Kit::Shell(M->UI,FString(B->Title)+TEXT(" JUNCTION"),S47Kit::Col(R),760));
}
bool FS47FreeformWorld::BranchStrandsFeeder(const FS47ArrayWear::FBranch& B,bool bFault) const
{
 // Each outage issues one credit fuse. Spent on a branch, it would leave no fuse, no credit and no paid work.
 if(!bFault||PowerFaultSerial==0||EmergencyFuseFault!=PowerFaultSerial||Feeder>=4)return false;
 for(const auto& N:B.Needs)if(FString(N.Item)==TEXT("fuse"))return InventoryCount(TEXT("fuse"))-N.Count<1;
 return false;
}
bool FS47FreeformWorld::RepairBranch(const FString& BranchId)
{
 const auto* B=FS47ArrayWear::Branch(BranchId);
 if(!Ready()||!B||Wear.IsRestored(BranchId)||!Wear.bActive||!Wear.Job.IsEmpty())return false;
 if(!Near(InteractionPoint(FString(TEXT("field:"))+B->Junction),4.0)){Change(FString::Printf(TEXT("%s: work at its junction beside %s."),B->Title,B->Junction));return false;}
 if(!BranchNeeds(*B).IsEmpty()){Change(FString(B->Title)+TEXT(" needs ")+BranchNeeds(*B)+TEXT("."));return false;}
 if(BranchStrandsFeeder(*B,M->Freeform->PowerFault())){Change(FString(B->Title)+TEXT(": SERVICE BAY FEEDER needs your control fuse first. Restore mains, then repair this branch; the parts stay in your pouch."));return false;}
 Wear.Job=BranchId;Wear.JobSeconds=0;if(M->UI)M->UI->Close();
 Change(FString(B->Title)+TEXT(": Parts fitted, running the three-second branch test."));return true;
}
void FS47FreeformWorld::FinishBranch(const FString& BranchId)
{
 const auto* B=FS47ArrayWear::Branch(BranchId);if(!Ready()||!B)return;
 if(!BranchNeeds(*B).IsEmpty()){Change(FString(B->Title)+TEXT(": Test abandoned; the parts are no longer in your pouch."));return;}
 for(const auto& N:B->Needs)for(int32 I=0;I<N.Count;++I)Consume(N.Item);
 Wear.Restore(BranchId);
 FVector2D Aim(180,45);for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I)){Aim=FVector2D(M->Web->DishAz(I),M->Web->DishEl(I));break;}
 for(const TCHAR* D:B->Dishes)for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==D)
 {
  M->Web->SetDishRemoteAvailable(I,true);M->Web->PointDish(I,Aim.X,Aim.Y,2.2,true); // visibly comes back up
  WearPose.Remove(I);
 }
 const int32 Up=M->Freeform->ArrayUpCount(); // D45-20261110: the one live count, as the station goal shows it
 M->Freeform->RecordEvidence(TEXT("service-branch-")+BranchId,FString(B->Title)+FString::Printf(TEXT(" repaired and tested at its junction. %d of 27 antennas in service."),Up));
 Change(FString::Printf(TEXT("%s back in service. ARRAY %d/27 / MEASUREMENT %.0f%%."),B->Title,Up,100*FS47ArrayWear::RateForCount(Up))+(Wear.DownCount()==0?TEXT(" SARO is whole again."):TEXT("")));
}
bool FS47FreeformWorld::PlanBranch(const FString& BranchId,int32 To)
{
 // D45-20261110: the order is the operator's; the repair itself stays at the junction cabinet
 if(!bActive||!Wear.Move(BranchId,To))return false;
 Change();return true;
}
void FS47FreeformWorld::RestoreWholeArrayFixture()
{
 if(!M||!M->Web)return;
 FVector2D Aim(180,45);for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishRemoteAvailable(I)){Aim=FVector2D(M->Web->DishAz(I),M->Web->DishEl(I));break;}
 for(const auto& B:FS47ArrayWear::Branches())
 {
  if(!Wear.Restore(B.Id))continue;
  for(const TCHAR* D:B.Dishes)for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==D){M->Web->SetDishRemoteAvailable(I,true);M->Web->SnapDish(I,Aim.X,Aim.Y);WearPose.Remove(I);}
 }
}
void FS47FreeformWorld::OpenSampleLab()
{
 if(!Ready()||!Near(SampleAt,2.6)||!M->UI)return;
 TArray<S47Kit::FW> R;
 R.Add(S47Kit::Paper(FString::Printf(TEXT("GENERAL TEST LAB\nFIELD FRAGMENTS %d / SEALED ARCHIVED ORIGINALS %d\n%s"),InventoryCount(TEXT("fragment")),InventoryCount(TEXT("archived_fragment")),BenchJob==TEXT("fragment")?TEXT("REFERENCE COMPARISON RUNNING"):TEXT("Each original keeps the note of where it was found."))));
 R.Add(S47Kit::Button(TEXT("Analyze field fragment and retain sealed original"),[this]{AnalyzeSample();},true,InventoryCount(TEXT("fragment"))>0&&BenchJob.IsEmpty()&&!M->Freeform->HasFinding(TEXT("sample-analysis"))));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / TEST LAB"),S47Kit::Col(R),880));
}
bool FS47FreeformWorld::LoadFilmAtPhotoLab()
{
 if(!Ready()||M->Web->Area()!=TEXT("saro")||!Near(FilmAt,2.6)||!M->Night||!M->Night->Album)return false;
 if(InventoryCount(TEXT("film"))<=0){Change(TEXT("PHOTO LAB: No photographic film in stock. Buy a 36-exposure pack at the Caliche shop, deliver it at GARAGE RECEIVING, then load it here."));return false;}
 if(!M->Night->Album->AddFilm(FS47Album::ROLL)){Change(TEXT("PHOTO LAB: Film capacity limit reached. Your stock and existing photographs are unchanged."));return false;}
 --Stock.FindOrAdd(TEXT("film")); // Film and stock change together before the single world autosave.
 Change(TEXT("PHOTO LAB: 36-exposure photographic film loaded. Expose, develop at the wet bench, then register the developed original at OPS. Earlier photographs are retained."));return true;
}
void FS47FreeformWorld::OpenDepot()
{
 if(!Ready()||!Near(DepotAt,2.6)||!M->UI)return;
 TArray<S47Kit::FW> R;
 R.Add(S47Kit::Paper(FString::Printf(TEXT("PARTS STORAGE\nFUSES %d / RELAYS %d / DONORS %d / FUEL CANS %d / FILM %d"),InventoryCount(TEXT("fuse")),InventoryCount(TEXT("relay")),InventoryCount(TEXT("donor")),InventoryCount(TEXT("fuel")),InventoryCount(TEXT("film")))));
 R.Add(S47Kit::Small(!LoggerAccess()?TEXT("EQUIPMENT CHECKOUT LOCKED: Complete a supported conclusion in The repeated interval."):!Logger.bLoanIssued?TEXT("EQUIPMENT CHECKOUT / fieldlogger01: First issue only. Collect the loan kit here, then carry it to CALIBRATION / REPAIR for assembly and clock calibration."):TEXT("EQUIPMENT CHECKOUT / fieldlogger01: Already issued. All preparation, collection, return and original comparison now happen at CALIBRATION / REPAIR.")));
 if(LoggerAccess()&&!Logger.bLoanIssued)R.Add(S47Kit::Button(TEXT("Issue loan field logger / carry kit to workbench"),[this]{LoggerAction(TEXT("issue"));},true,Payload==0));
 // N1110 (Tom): the sabotage takes two ceiling tubes each time. A pair from the store (CALICHE / SHOP COUNTER, $4) puts the last two back.
 if(M->Freeform->BrokenTubeCount()>0||InventoryCount(TEXT("tubes"))>0)
 {
  const int32 Out=M->Freeform->BrokenTubeCount(),Pairs=InventoryCount(TEXT("tubes"));
  R.Add(S47Kit::Small(FString::Printf(TEXT("CONTROL ROOM CEILING / %d tubes dark, %d pairs of tubes in stock. A pair is $4 at the Caliche shop counter."),Out,Pairs)));
  R.Add(S47Kit::Button(TEXT("Fit a pair of fluorescent tubes in the control room ceiling"),[this]
  {
   if(!Ready()||M->Freeform->BrokenTubeCount()<=0||!Consume(TEXT("tubes")))return;
   const int32 Back=M->Freeform->MendTubes(2);
   Change(Back>1?FString(TEXT("A pair of tubes goes up in the control room ceiling. The room is brighter.")):FString(TEXT("The last dark tube is replaced. The control room ceiling is whole again.")));
   OpenDepot();
  },false,Out>0&&Pairs>0));
 }
 R.Add(S47Kit::Small(TEXT("Small spares travel in your service pouch. GARAGE RECEIVING: Unpack purchased parcels. PHOTO LAB: Load the stocked 36-exposure photographic film at the dry bench.")));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / PARTS STORAGE"),S47Kit::Col(R),880));
}
double FS47FreeformWorld::TankCap()const{return FS47Outfit::TuneFor(M?M->Freeform.Get():nullptr).Tank;}
double FS47FreeformWorld::FuelPerDollar()const
{
 // M7 (Tom, 9 November): the pump's price goes up a little with the weeks, $4.00 a unit in week 1 to $4.55 from week 4
 static const double Per[]={.25,.24,.23,.22};
 const int32 Week=M&&M->Freeform&&M->Freeform->RulesLive()?(M->Freeform->Day()-1)/7:0;
 return Per[FMath::Clamp(Week,0,3)];
}
void FS47FreeformWorld::RefuelTruck(){if(Ready()&&Near(TruckPoint(),3.3)&&Fuel<TankCap()-.01&&Consume(TEXT("fuel"))){Fuel=FMath::Min(TankCap(),Fuel+2.);Change(TEXT("Fuel can transferred to the service truck."));}}
void FS47FreeformWorld::RoadsideAssist()
{
 const bool bTrouble=M&&M->Freeform&&M->Freeform->Care&&M->Freeform->Care->NeedsRescue();
 if(!Ready()||!M->World||(Fuel>.001&&!bTrouble))return;
 // M7 (C): a dry tank is no longer free, and nobody comes while something is close. A truck that cannot be mended is the full tow.
 const FS47TruckCare* Care=M->Freeform->Care.Get();
 const double Hour=DayClock(M->Freeform->Clock())/3600.;
 const FString Block=M->Freeform->Director?M->Freeform->Director->RestBlock():FString();
 const FRoadsideTerms Terms=RoadsideTermsFor(bTrouble,Care?Care->TowFee():FS47TruckCare::TowFeeFor(0),Care?Care->TowMinutes():FS47TruckCare::TowMinutesFor(0),Hour,Block);
 // (not the rest block's words: they are about lying down, and would say what is out there)
 if(Terms.bRefused){Change(TEXT("The county radio answers, then only static. Nobody is coming out yet."));return;}
 const bool bCash=M->Freeform->BillTow(Terms.Fee);
 if(M->World->bDriving)M->World->GetOutHere(true);
 if(M->Freeform->Care)M->Freeform->Care->Clear();
 M->World->Enter(TEXT("saro"));M->World->PlaceTruck(TEXT("saro"));M->Player->Place(16,9,0);
 // Two reserve units cover 24 km at the normal 1/12000-m consumption. SARO
 // and Caliche use separate scene origins; their 8-km rendering offset is
 // not a driven segment. This still leaves ample margin for the full return
 // supply journey and a feeder detour before stock fuel can be unpacked.
 Fuel=FMath::Max(Fuel,2.);M->Freeform->SpendTime(Terms.Minutes*60);
 const FString Billed=bCash?FString::Printf(TEXT(" $%d paid from the budget."),Terms.Fee):FString::Printf(TEXT(" $%d is on the weekly sheet."),Terms.Fee);
 // No minutes in the telling: they are on the button already.
 const FString Wait=Hour>=21||Hour<6?FString(TEXT("You sat in the dark cab a long while, the engine ticking as it cooled, before the county's amber light came up the road. "))
  :FString(TEXT("The county truck was a long time coming out. "));
 Change(Wait+(bTrouble?TEXT("It towed the truck and cargo back to SARO and left it sound, with at least fifteen miles of fuel in the tank."):TEXT("It brought the truck and cargo back to SARO, with at least fifteen miles of fuel in the tank."))+Billed+TEXT(" Collect supplies in Caliche and refuel before the next long trip."));
 M->Freeform->RecordEvidence(TEXT("roadside"),FString(TEXT("Roadside return used; the interrupted trip and cargo were retained."))+Billed);
 UE_LOG(LogTemp,Display,TEXT("S47: roadside return: %s, $%d, %.0f minutes (%s)"),bTrouble?TEXT("tow"):TEXT("dry tank"),Terms.Fee,Terms.Minutes,bCash?TEXT("cash"):TEXT("on the sheet"));
}

void FS47FreeformWorld::SkipFaultToday(){if(M&&M->Freeform)LastFaultDay=M->Freeform->Day();}
FString FS47FreeformWorld::NextServiceDish() const
{
 int32 Serial=OrdinaryServiceSerial;
 for(int32 Skip=0;Skip<27&&Wear.BranchOfDish(OrdinaryServiceDish(Serial));++Skip)Serial=(Serial+1)%27;
 return OrdinaryServiceDish(Serial);
}
bool FS47FreeformWorld::TriggerServiceFault(bool bTraining,const FString& RequestedDish)
{
 if(!Ready()||HasArrayFault()||M->Freeform->PowerFault()||(Service==9&&M->Freeform->ActiveServiceGuide()==1))return false;
 if(!RequestedDish.IsEmpty()&&!ValidServiceDish(RequestedDish))return false;
 if(!RequestedDish.IsEmpty()&&Wear.BranchOfDish(RequestedDish))return false;
 if(!bTraining&&RequestedDish.IsEmpty())for(int32 Skip=0;Skip<27&&Wear.BranchOfDish(OrdinaryServiceDish(OrdinaryServiceSerial));++Skip)OrdinaryServiceSerial=(OrdinaryServiceSerial+1)%27;
 const FString Target=bTraining?TEXT("S-03"):RequestedDish.IsEmpty()?OrdinaryServiceDish(OrdinaryServiceSerial):RequestedDish;
 int32 Index=INDEX_NONE;for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==Target)Index=I;
 // Authored cabinet items use the existing HUD fallback; not all have native Inter overrides.
 if(Index<0||!M->Web->Item(TEXT("field:")+Target))return false;
 ServiceTarget=Target;if(!bTraining&&RequestedDish.IsEmpty())OrdinaryServiceSerial=(OrdinaryServiceSerial+1)%27;
 Service=1;Settle=0;LastFaultDay=M->Freeform->Day();M->Web->SetDishRemoteAvailable(Index,false);
 Change(TEXT("ARRAY STATUS (left-hand PC): Select S-03 to diagnose its failed remote link, or inspect the S-03 CABLE JUNCTION cabinet beside the antenna. Other antennas remain available."));return true;
}
void FS47FreeformWorld::DiagnoseService(){if(Ready()&&Service==1){Service=2;Change(TEXT("S-03 command relay failed. At the S-03 CABLE JUNCTION cabinet: open the hatch, fit one drive relay, close it and run the movement test. Any relays from stock are in your service pouch."));}}
FString FS47FreeformWorld::ServiceStatus() const
{
 const int32 I=ServiceDishIndex();
 const bool Powered=M&&M->Freeform&&(!M->Freeform->PowerFault()||HasBackupPower());
 const bool Moving=I>=0&&(FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),M->Web->DishTargetAz(I)))>.1||FMath::Abs(M->Web->DishEl(I)-M->Web->DishTargetEl(I))>.1);
 const FString State=!Powered?TEXT("NO POWER / movement held"):I<0?TEXT("ENCODER UNAVAILABLE"):Service==1||Service==2?TEXT("FAULT / command relay failed"):Service>=3&&Service<=6?TEXT("LOCAL / remote isolated"):Service==7||Service==8?TEXT("LOCAL TEST / moving and settling"):Moving?TEXT("REMOTE / moving"):TEXT("REMOTE / ready");
 const FString Next=Service==0||Service==9?TEXT("Select a source and command tracking on the tracking PC."):Service==7||Service==8?Powered?TEXT("Watch the antenna turn out and return; REMOTE follows a passed test."):TEXT("Restore feeder power or start the fueled backup generator; the test resumes."):Label(TEXT("ff:service"));
 const FString Encoder=I>=0?FString::Printf(TEXT("ENCODER %.1f / %.1f | TARGET %.1f / %.1f | %s"),M->Web->DishAz(I),M->Web->DishEl(I),M->Web->DishTargetAz(I),M->Web->DishTargetEl(I),M->Web->DishRemoteAvailable(I)?TEXT("REMOTE ENABLED"):TEXT("REMOTE DISABLED")):TEXT("ENCODER UNAVAILABLE");
 return ServiceWords(FString::Printf(TEXT("S-03: %s\n%s\nLOCAL SERVICE: existing S-03 CABLE JUNCTION cabinet at ground level.\nDRIVE RELAYS IN POUCH: %d / required: %d\nNEXT: %s\n%s"),*State,*Encoder,InventoryCount(TEXT("relay")),HasArrayFault()&&Service<5?1:0,*Next,Service>=5?TEXT("Replacement already fitted. No further relay is consumed for closing, testing or Continue."):InventoryCount(TEXT("relay"))>0?TEXT("Stock relays can be fitted directly; no upgrade or service case is required."):TEXT("CALICHE GAS STATION: Buy a drive relay. GARAGE RECEIVING: Unload and unpack its parcel. CALIBRATION / REPAIR: Test donor controllers before use.")));
}
bool FS47FreeformWorld::ServiceUse()
{
 if(!Near(InteractionPoint(TEXT("ff:service")),2.6)){Change(TEXT("Approach the S-03 CABLE JUNCTION cabinet beside the antenna. Use the hatch from ground level."));return true;}
 switch(Service)
 {
  case 0:case 9:Change(ServiceStatus());return true;
  case 1:case 2:Service=4;Change(TEXT("S-03 CABLE JUNCTION: Failed command relay identified. LOCAL selected, remote movement isolated and hatch opened. Seat one replacement relay."));break;
  case 3:Service=4;Change(TEXT("S-03 CABLE JUNCTION: Hatch open. Seat a compatible replacement relay from your stock."));break;
  case 4:if(!Consume(TEXT("relay"))){Change(TEXT("CALICHE GAS STATION: Buy a relay; your pouch is empty. GARAGE RECEIVING: Unpack its parcel. CALIBRATION / REPAIR: Test a donor before using its relay."));return true;}Service=5;Change(TEXT("S-03 CABLE JUNCTION: Replacement relay seated. Close the cover and run the movement test."));break;
  case 5:Service=6; // Closing the hatch and requesting the test are one physical action.
   // Retain legacy stage 6 if power or the encoder prevents movement.
   [[fallthrough]];
  case 6:if(M->Freeform->PowerFault()&&!HasBackupPower()){Change(TEXT("NO POWER: restore feeder supply or start the fueled backup generator before the local test. The new relay stays fitted."));return true;}if(ServiceDishIndex()<0){Change(TEXT("S-03 encoder unavailable. Test cannot start; the fitted relay is retained."));return true;}ServiceAz=M->Web->DishAz(ServiceDishIndex());ServiceEl=M->Web->DishEl(ServiceDishIndex());Service=7;Settle=0;M->Web->PointDish(ServiceDishIndex(),ServiceAz+5,ServiceEl,2.2,true);Change(TEXT("S-03 CABLE JUNCTION: Wait for the automatic five-degree out-and-back test. REMOTE returns after the test passes."));break;
  case 7:case 8:Change(ServiceStatus());break;
 }
 return true;
}
int32 FS47FreeformWorld::SkyTransfer()
{
 if(!Ready()||bGenerator||!M->Freeform->HasKit(TEXT("transfer")))return -1;
 if(GeneratorSeconds<=0&&Consume(TEXT("fuel")))GeneratorSeconds=6*3600.;
 if(M->Audio)M->Audio->Sfx(TEXT("switch"),FS47Sound{.Gain=.6f});
 if(GeneratorSeconds<=0){GeneratorSound(EGenSound::Dry);return 0;} // D45-20261110: it turns over and does not catch
 bGenerator=true;GeneratorSound(EGenSound::Start);Change();
 return 1;
}
bool FS47FreeformWorld::GeneratorUse()
{
 if(!Near(GeneratorAt,2.9))return true;
 // D45-20261110: each use is heard at the generator: the stop, a dry crank, the start (the loop follows in StepGeneratorSound)
 if(bGenerator){bGenerator=false;GeneratorSound(EGenSound::Off);Change(TEXT("Backup generator stopped. Remaining tank fuel is retained."));return true;}
 if(GeneratorSeconds<=0){if(!Consume(TEXT("fuel"))){GeneratorSound(EGenSound::Dry);Change(TEXT("Generator tank empty. Bring a fuel can from the depot or Caliche."));return true;}GeneratorSeconds=6*3600.;}
 bGenerator=true;GeneratorSound(EGenSound::Start);Change(TEXT("Backup generator running. A full can keeps it going for six hours."));return true;
}
bool FS47FreeformWorld::FeederUse()
{
 if(!Near(InteractionPoint(TEXT("ff:feeder")),3.0))return true;
 if(!M->Freeform->PowerFault()){Change(TEXT("SARO feeder voltage and local fuse are normal."));return true;}
 if(Feeder<=1){Feeder=2;Change(TEXT("SERVICE BAY FEEDER: Low supply confirmed. Open the enclosed isolation switch."));}
 else if(Feeder==2){Feeder=3;Change(TEXT("SERVICE BAY FEEDER: Supply isolated. Replace the blown control fuse from your service stock."));}
 else if(Feeder==3){if(!Consume(TEXT("fuse"))){Change(TEXT("No spare fuse in stock. The CALICHE STORE COUNTER sells a $5 feeder fuse on requisition, any hour, even with a balance standing. Unpack it at GARAGE RECEIVING, then come back here."));return true;}Feeder=4;Change(TEXT("SERVICE BAY FEEDER: Replacement fuse seated. Start the enclosed three-second test."));}
 else if(Feeder==4){if(FeederSeconds<=0){FeederSeconds=.001;Change(TEXT("SERVICE BAY FEEDER: Wait for the automatic three-second test. The indicator settles and mains supply returns when it passes."));}}
 return true;
}

bool FS47FreeformWorld::Use(const FString& Id)
{
 if(!Ready())return false;
 if(Id==TEXT("ff:charger"))
 {
  // D45-20261110: the lamp goes off on the charger; the old rule does the charging (off, indoors at SARO, on station power)
  FS47Flashlight* Lamp=M->Night&&M->Night->Flashlight?M->Night->Flashlight.Get():nullptr;
  if(Lamp&&Lamp->bHave&&Lamp->bOn&&Lamp->Toggle())
  {
   if(M->UI)M->UI->Toast(TEXT("You switch the flashlight off and stand it on the charger. It fills while you're in the building and the station has power."),5);
   return true;
  }
  if(M->UI)M->UI->Toast(Label(Id),4);
  return true;
 }
 if(Id==TEXT("ff:film"))return LoadFilmAtPhotoLab();
 if(Id==TEXT("ff:logger-device"))return Logger.Site>=0&&LoggerAction(Logger.Site==0?TEXT("site0"):TEXT("site1"));
 if(Id==TEXT("ff:logger-a")||Id==TEXT("ff:logger-b"))return LoggerAction(Id==TEXT("ff:logger-a")?TEXT("site0"):TEXT("site1"));
 if(Id==TEXT("ff:pump")||Id==TEXT("town:pumps"))
 {
  if(AtDiesel()){Change(TEXT("DIESEL is incompatible with the SARO service truck. Use the red REGULAR pump on the same island."));return true;}
  if(bPumping)StopPump();else StartPump();return true;
 }
 if(Id==TEXT("ff:carry")){if(Payload!=2)return false;OpenCarryMenu();return true;}
 if(Id==TEXT("ff:shop")){OpenShop();return true;}
 if(Id==TEXT("ff:bench")||Id==TEXT("ff:depot")||Id==TEXT("ff:delivery"))return Deposit(Id);
 if(Id==TEXT("ff:sample")){OpenSampleLab();return true;}
 if(Id==TEXT("ff:parcel"))return ParcelUse();
 if(Id==TEXT("ff:cargo"))return CargoUse();
 if(Id.StartsWith(TEXT("field:")))
 {
  if(const auto* Down=Wear.DownJunction(Id.Mid(6))){OpenBranch(Down->Id);return true;}
  if(Id!=TEXT("field:")+ServiceTarget||Wear.BranchOfDish(Id.Mid(6))){Change(Label(Id));return true;}
  return ServiceUse();
 }
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service"))return ServiceUse();
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return GeneratorUse();
 if(Id==TEXT("ff:feeder")||Id==TEXT("substation:service")||Id==TEXT("substation:lamp")||Id==TEXT("substation:journal"))return FeederUse();
 return false;
}
FString FS47FreeformWorld::Label(const FString& Id) const
{
 if(!Ready())return FString();
 if(Id==TEXT("ff:charger"))
 {
  // D45-20261110: the charger says what it is doing
  const FS47Flashlight* Lamp=M->Night&&M->Night->Flashlight?M->Night->Flashlight.Get():nullptr;
  if(!Lamp||!Lamp->bHave)return TEXT("Flashlight charger / the flashlight is still on the truck's passenger seat");
  const int32 Pct=FMath::RoundToInt32(100*Lamp->BatteryLevel());
  if(ChargerState()==0)return FString::Printf(TEXT("Flashlight charger / no station power / the lamp at %d%%"),Pct);
  if(Lamp->bOn)return FString::Printf(TEXT("Flashlight charger / switch the lamp off and leave it to charge (%d%%)"),Pct);
  return Pct>=100?FString(TEXT("Flashlight charger / the lamp is full")):FString::Printf(TEXT("Flashlight charger / charging, %d%%"),Pct);
 }
 if(Id==TEXT("ff:film"))return FString::Printf(TEXT("PHOTO LAB: Load photographic film / 36 exposures / %d packs in stock"),InventoryCount(TEXT("film")));
 if(Id==TEXT("ff:logger-device"))return Logger.Site>=0?TEXT("Retrieve field logger and original tape"):FString();
 if(Id==TEXT("ff:logger-a")||Id==TEXT("ff:logger-b"))return !LoggerAccess()?FString():FString(Id==TEXT("ff:logger-a")?TEXT("FIELD SITE A / generator path / "):TEXT("FIELD SITE B / array east / "))+(Logger.Site==(Id==TEXT("ff:logger-a")?0:1)?TEXT("retrieve logger and original tape"):TEXT("place carried logger and start two-hour record"));
 if(Id==TEXT("ff:pump")||Id==TEXT("town:pumps"))
 {
  if(AtDiesel())return TEXT("DIESEL / service truck needs the red REGULAR pump");
  if(bTripoPump)return bPumping?TEXT("Stop pump"):TEXT("Use pump");
  if(bPumping)return FString::Printf(TEXT("Stop fuel / tank %.2f / %.0f / delivered %.2f / $%d"),Fuel,TankCap(),PumpVolume,PumpPaid);
  const double Volume=PumpVolumeFor(Fuel,TankCap());return Volume<.00001?TEXT("REGULAR / service truck tank full"):FString::Printf(TEXT("REGULAR / fill up to %.2f / $%d / tank %.2f of %.0f"),Volume,FMath::CeilToInt(Volume/FuelPerDollar()-.00001),Fuel,TankCap());
 }
 if(Id==TEXT("ff:carry"))return Payload==2?TEXT("Handle carried parcel / set down or drop"):FString();
 if(Id==TEXT("ff:shop"))return ShopOpen()?TEXT("Buy service supplies / collect a physical parcel"):TEXT("Store counter closed / essential requisition");
 if(Id==TEXT("ff:bench")&&Payload==2&&Item==TEXT("fieldlogger"))return TEXT("CALIBRATION / REPAIR: Return logger and original tape");
 if(Id==TEXT("ff:bench"))return Payload==2&&Item==TEXT("service_case")?TEXT("Return service case to calibration bench"):TEXT("CALIBRATION / REPAIR: Test donors and prepare field logger");
 if(Id==TEXT("ff:sample"))return TEXT("TEST LAB: Analyze field sample and inspect sealed originals");
 if(Id==TEXT("ff:depot"))return TEXT("PARTS STORAGE: First logger issue and spare stock");
 if(Id==TEXT("ff:delivery"))return Payload==2?TEXT("GARAGE RECEIVING: Unpack carried supplies into SARO stock"):TEXT("GARAGE RECEIVING: Delivery instructions");
 if(Id==TEXT("ff:parcel"))return Payload>0&&Payload!=2?FString(TEXT("Carry "))+ParcelDescription():FString();
 if(Id==TEXT("ff:cargo"))return Payload==2?TEXT("Secure carried parcel in truck"):(M&&M->Freeform&&M->Freeform->Care&&M->Freeform->Care->NeedsRescue())?TEXT("Truck in trouble / dig out, change the wheel or call the tow"):Fuel<=.001?TEXT("Tank empty / roadside return and cargo rack"):Payload==3?TEXT("Unload parcel from truck"):TEXT("Truck fuel / cargo rack");
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return bGenerator?TEXT("Stop backup generator"):TEXT("Start / fuel backup generator");
 if(Id==TEXT("ff:feeder")||Id==TEXT("substation:service")||Id==TEXT("substation:lamp")||Id==TEXT("substation:journal"))return M->Freeform->PowerFault()?Feeder==4&&FeederSeconds>0?TEXT("Feeder test running"):Feeder==4?TEXT("Run feeder replacement-fuse test"):Feeder==3?TEXT("Replace isolated feeder control fuse"):Feeder==2?TEXT("Open enclosed feeder isolation switch"):TEXT("Measure SARO feeder supply"):TEXT("Check normal feeder supply");
 if(Id.StartsWith(TEXT("field:")))
  if(const auto* Down=Wear.BranchOfDish(Id.Mid(6)))
   return Wear.DownJunction(Id.Mid(6))?FString::Printf(TEXT("%s JUNCTION / %s / OUT OF SERVICE"),Down->Title,*Id.Mid(6)):FString::Printf(TEXT("%s / OUT OF SERVICE / %s junction at %s"),*Id.Mid(6),Down->Title,Down->Junction);
 if(Id.StartsWith(TEXT("field:"))&&Id!=TEXT("field:")+ServiceTarget)
  return FString::Printf(TEXT("%s / REMOTE ready / no local fault; active work order: %s"),*Id.Mid(6),*ServiceTarget);
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service")||Id==TEXT("field:")+ServiceTarget)
 {
  if(Service>=6&&Service<=8&&M->Freeform->PowerFault()&&!HasBackupPower())return ServiceWords(TEXT("S-03 NO POWER / restore feeder or generator"));
  if(Service==4&&InventoryCount(TEXT("relay"))<=0)return ServiceWords(TEXT("GARAGE RECEIVING: Unpack a drive relay for S-03"));
  static const TCHAR* Labels[]={TEXT("Inspect healthy S-03 control"),TEXT("Inspect, isolate and open S-03 relay hatch"),TEXT("Isolate and open S-03 relay hatch"),TEXT("Open S-03 relay hatch"),TEXT("Seat compatible replacement relay"),TEXT("Close S-03 hatch and run movement test"),TEXT("Run S-03 local movement test"),TEXT("Local test: outward movement"),TEXT("Local test: return movement"),TEXT("S-03 verified / REMOTE restored")};return ServiceWords(Labels[FMath::Clamp(Service,0,9)]);
 }
 return FString();
}

void FS47FreeformWorld::AdvanceUnobservedTo(double Clock)
{
 if(!bActive||!FMath::IsFinite(Clock)||Clock<0)return;
 if(LastClock<0){LastClock=Clock;return;}
 const double Delta=FMath::Max(0.,Clock-LastClock);if(Delta<=0)return;LastClock=Clock;
 Logger.Skip(Clock,Delta);LoggerJob.Empty();LoggerJobElapsed=0;
 if(bGenerator){GeneratorSeconds=FMath::Max(0.,GeneratorSeconds-Delta);if(GeneratorSeconds<=0)bGenerator=false;}
 if(bLoggerCalibrating){bLoggerCalibrating=false;Logger.Calibration=0;Logger.bCalibrated=false;}
 bPumping=false; // no held dispenser input or driven distance occurs during a time jump
 Apply(); // caller requests one save after every system has accounted for the same jump
}
void FS47FreeformWorld::Step(double Dt)
{
 if(!Ready())return;if(!bBuilt)Build();
 SetFieldZone(true);StepServiceDoor(Dt);
 StepCharger(Dt); // D45-20261110
 const bool bFaultBefore=bTrackedPowerFault;
 TrackPowerFault(M->Freeform->PowerFault());
 // M7 (C): the transfer switch from OPS (week 3 on, FS47Openings "transfer"): a feeder fault as it starts throws the backup generator
 // in by itself, from its own tank or a stock can. Heard in the building; what runs on the generator (HasPower) runs on.
 if(!bFaultBefore&&bTrackedPowerFault&&!bGenerator&&M->Freeform->HasKit(TEXT("transfer")))
 {
  if(GeneratorSeconds<=0&&Consume(TEXT("fuel")))GeneratorSeconds=6*3600.;
  if(M->Audio)M->Audio->Sfx(TEXT("switch"),FS47Sound{.Gain=.6f});
  if(GeneratorSeconds>0)
  {
   bGenerator=true;GeneratorSound(EGenSound::Start); // D45-20261110: placed at the generator, and its running loop follows
   Change(TEXT("The lights dip. Down in the service bay the transfer switch clunks over, and out behind the control room the standby generator coughs and takes the load."));
  }
  else{GeneratorSound(EGenSound::Dry);Change(TEXT("The lights go. Down in the service bay the transfer switch clunks over, and out behind the control room the standby generator turns and does not catch: no fuel for it."));}
  UE_LOG(LogTemp,Display,TEXT("S47: transfer switch: generator %s"),bGenerator?TEXT("started"):TEXT("dry"));
 }
 if(Carry)Carry->Step(Dt,Payload,Item,Payload==1?ParcelShopPoint():Payload==3?TruckPoint():Dropped);
 const double Now=M->Freeform->Clock();if(LastClock<0)LastClock=Now;
 const double WorldDelta=FMath::Max(0.,Now-LastClock);
 if(M->Freeform->Held()||WorldDelta>3.){AdvanceUnobservedTo(Now);Apply();StepGeneratorSound(Dt);return;} // D45-20261110: a jump that drained the tank fades the loop
 LastClock=Now;
 StepLogger(Dt,WorldDelta);
 StepPump(Dt);
 if(bGenerator){GeneratorSeconds=FMath::Max(0.,GeneratorSeconds-WorldDelta);if(GeneratorSeconds<=0){GeneratorSound(EGenSound::Stall);bGenerator=false;Change(TEXT("Backup generator tank empty. Array work needs restored mains or another fuel can."));}}
 StepGeneratorSound(Dt); // D45-20261110: the running loop follows the generator's state every frame
 if(M->World&&M->World->bDriving)
 {
  // (M7: the engine rebuild drinks a tenth more, FS47Outfit)
  Fuel=FMath::Max(0.,Fuel-FMath::Abs(M->World->DrivingSpeed())*FMath::Min(Dt,.2)/12000.*FS47Outfit::TuneFor(M->Freeform.Get()).Burn);
  Fuel=FMath::Min(Fuel,TankCap());
  if(Fuel<=.001)
  {
   if(!bFuelHeld)Change(TEXT("SERVICE TRUCK: Tank empty; the engine dies. Stop, get out and use the cargo rack: a fuel can from stock, or the county's roadside return, which costs money and a long wait."));
   M->World->HoldTruck(25.);bFuelHeld=true;
  }
  else if(bFuelHeld){M->World->HoldTruck(0);bFuelHeld=false;}
 }
 // IGNITION-20261111: left idling it drinks a little (about a gallon in three hours), and dies when the tank is dry
 else if(M->World&&M->World->EngineIdling())
 {
  Fuel=FMath::Max(0.,Fuel-FMath::Min(Dt,.2)*.0001*FS47Outfit::TuneFor(M->Freeform.Get()).Burn);
  if(Fuel<=.001){M->World->EndIdle(true);Change(TEXT("SERVICE TRUCK: Left idling, it ran the tank dry and died. Its lights are out."));}
 }
 if((Service==7||Service==8)&&(!M->Freeform->PowerFault()||HasBackupPower()))
 {
  const int32 I=ServiceDishIndex();if(I>=0){const double Target=ServiceAz+(Service==7?5.:0.);
   const bool On=FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),Target))<=.1&&FMath::Abs(M->Web->DishEl(I)-ServiceEl)<=.1;
   Settle=On?Settle+FMath::Min(Dt,.1):0;
   if(Settle>=1){Settle=0;if(Service==7){Service=8;M->Web->PointDish(I,ServiceAz,ServiceEl,2.2,true);}else{Service=9;ServiceTests=FMath::Min(1000000,ServiceTests+1);M->Web->SetDishRemoteAvailable(I,true);M->Freeform->RecordEvidence(TEXT("service-")+ServiceTarget.ToLower().Replace(TEXT("-"),TEXT("")),ServiceWords(TEXT("S-03 replacement relay passed local five-degree out-and-back test. Remote pointing restored.")));Change(FString(M->Freeform->ActiveServiceGuide()==1?TEXT("ARRAY STATUS (left-hand PC) / MAINTENANCE: Local test passed and REMOTE is back. Sign off the work order there."):TEXT("TRACKING (right-hand PC): S-03 local test passed and REMOTE is restored. Resume the selected observation."))+FString::Printf(TEXT(" ARRAY %d/27."),M->Freeform->ArrayUpCount()));}} // D45-20261110: a relay fix moves the count too
  }
 }
 if(!Wear.Job.IsEmpty())
 {
  Wear.JobSeconds+=FMath::Min(Dt,.1);
  if(Wear.JobSeconds>=3){const FString Finished=Wear.Job;Wear.Job.Empty();Wear.JobSeconds=0;FinishBranch(Finished);}
 }
 if(!BenchJob.IsEmpty())
 {
  BenchSeconds+=FMath::Min(Dt,.1);
  if(BenchSeconds>=3)
  {
   const FString Finished=BenchJob;BenchJob.Empty();BenchSeconds=0;
   if(Finished.StartsWith(TEXT("ws:")))FinishWorkshopJob(Finished);
   else if(Finished==TEXT("fragment"))
   {
    AddItem(TEXT("archived_fragment"));
    M->Freeform->RecordEvidence(TEXT("sample-analysis"),TEXT("SARO test-lab comparison of the retained field fragment. Provenance: original landing_trace field record and its photograph, where available, remain unchanged in the case archive. The fragment conducts differently from the stock relay and contact reference. Original sample sealed and retained in the test-lab archive."));
    Change(TEXT("TEST LAB: Comparison complete. Measurement retained; original fragment sealed in the archive. Origin remains unassigned."));
   }
   else
   {
    DonorTests=FMath::Min(1000000,DonorTests+1);AddItem(TEXT("relay"));M->Freeform->RecordEvidence(TEXT("donor-relay"),TEXT("Donor controller relay tested at SARO bench and harvested as a compatible spare."));
    Change(TEXT("CALIBRATION / REPAIR: Relay test passed; one compatible relay added to stock. S-03 CABLE JUNCTION: Fit it if the command relay needs replacement. Donor casing retained for scrap."));
   }
  }
 }
 if(M->Freeform->PowerFault()&&(Feeder==0||Feeder==5))Feeder=1;
 if(!M->Freeform->PowerFault()&&Feeder>0&&Feeder<5){Feeder=0;FeederSeconds=0;}
 if(Feeder==4&&FeederSeconds>0){FeederSeconds+=FMath::Min(Dt,.1);if(FeederSeconds>=3){Feeder=5;FeederSeconds=0;M->Freeform->SetPowerFault(false);TrackPowerFault(false);M->Freeform->RecordEvidence(TEXT("feeder-repair"),TEXT("Feeder supply measured, isolated, control fuse replaced and enclosed test passed."));Change(TEXT("SERVICE BAY FEEDER: Test passed; mains restored. ")+M->Freeform->Objective());}}
 // A bounded ordinary service need, not a constant global wear drain.
 if(LastFaultDay!=M->Freeform->Day()&&M->Freeform->FaultDue()&&!HasArrayFault()&&!M->Freeform->PowerFault())TriggerServiceFault();
 Apply();
}

bool FS47FreeformWorld::LampCharging() const
{
 // D45-20261110: S47Night::FlashlightStep's rule, read here for the pilot and the notebook (it is not changed)
 const FS47Flashlight* Lamp=M&&M->Night&&M->Night->Flashlight?M->Night->Flashlight.Get():nullptr;
 if(!Lamp||!Lamp->bHave||Lamp->bOn||Lamp->BatteryLevel()>=1||M->Space!=TEXT("room"))return false;
 const FString Where=M->World?M->World->Area:M->Web?M->Web->Area():FString(TEXT("saro"));
 return Where==TEXT("saro")&&M->Freeform&&M->Freeform->HasPowerForReactiveLight();
}
int32 FS47FreeformWorld::ChargerState() const
{
 if(!M||!M->Freeform||!M->Freeform->HasPowerForReactiveLight())return 0;
 return LampCharging()?1:2;
}
FString FS47FreeformWorld::FlashlightHelpFor(bool bCharger)
{
 return bCharger?FString(TEXT("Put the lamp on the charger on the hall dresser, or buy dry cells at Earl's (1420 AM) or the Feed & Seed."))
  :FString(TEXT("Switch it off indoors at SARO and it charges on station power, or buy dry cells at Earl's (1420 AM) or the Feed & Seed."));
}
void FS47FreeformWorld::StepCharger(double Dt)
{
 // D45-20261110: looked for once a second until the dresser is placed (the props come after the web; FS47FieldRadio::Rest does the
 // same), then its pilot follows the lamp, set only on a change. On the dresser's top from its bounds: between the recorder at its
 // left end and the walkie at its right (C.X+.34), a squat dark box with the pilot on its face to the corridor (+Z).
 if(!ChargerBody||ChargerBody.W!=M->Web.Get())
 {
  ChargerRetry-=Dt;if(ChargerRetry>0||!bActive)return;
  ChargerRetry=1;
  const UStaticMeshComponent* Dresser=AS47Props::PlacedMesh(TEXT("hallDresser"));
  const int32 Saro=M->Web->Handle(TEXT("ext.group"));
  if(!Dresser||Saro<0)return;
  const FBox Top=Dresser->Bounds.GetBox();const FVector C=S47Three::ToThree(Top.GetCenter());const double Y=Top.Max.Z/100.;
  ChargerBody=Box(TEXT("FreeformFlashlightCharger"),FVector(C.X+.04,Y+.045,C.Z+.02),FVector(.17,.09,.12),0x2f322d,Saro,TEXT("ff:charger"));
  if(!ChargerBody)return;
  ChargerLamp=Box(TEXT("FreeformFlashlightChargerPilot"),FVector(C.X+.10,Y+.06,C.Z+.083),FVector(.018,.018,.008),0x1a1a16,Saro);
  if(ChargerLamp)M->Web->MarkUsable(ChargerLamp.I,TEXT("ff:charger"));
  ChargerShown=-1;
  UE_LOG(LogTemp,Display,TEXT("S47: flashlight charger: on the hall dresser (%.2f %.2f %.2f)"),C.X+.04,Y,C.Z+.02);
 }
 const int32 Want=bActive?ChargerState():-1;
 if(Want==ChargerShown)return;
 ChargerShown=Want;
 ChargerBody.SetVisible(bActive);ChargerLamp.SetVisible(bActive);
 const FLinearColor Pilot=S47Three::Hex(Want==1?0xffa040:Want==2?0x7dff9a:0x1a1a16);
 ChargerLamp.SetColor(Pilot);ChargerLamp.SetEmissive(Pilot,Want==1?1.f:Want==2?.6f:0.f);
}
bool FS47FreeformWorld::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto C=[&](bool B,const TCHAR* T){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+T);};
 // D45-20261110 (Tom: where is the flashlight's charger?): the help says where it charges and where cells are sold, plainly
 C(FlashlightHelpFor(true).Contains(TEXT("charger on the hall dresser"))&&FlashlightHelpFor(true).Contains(TEXT("Feed & Seed"))&&FlashlightHelpFor(true).Contains(TEXT("Earl's"))
  &&!FlashlightHelpFor(false).Contains(TEXT("dresser"))&&FlashlightHelpFor(false).Contains(TEXT("indoors at SARO")),TEXT("D45: the flashlight help names the charger on the hall dresser and the two counters that sell dry cells"));
 FS47FreeformWorld W(nullptr);W.Begin();C(W.InventoryCount(TEXT("fuse"))==3&&W.InventoryCount(TEXT("relay"))==1&&W.InventoryCount(TEXT("fuel"))==2,TEXT("Freeform depot starts with bounded essential spares"));
 W.AddItem(TEXT("relay"),200);C(W.InventoryCount(TEXT("relay"))==99,TEXT("Freeform stock is bounded"));for(int32 I=0;I<99;++I)W.Consume(TEXT("relay"));C(!W.Consume(TEXT("relay"))&&W.InventoryCount(TEXT("relay"))==0,TEXT("Freeform cannot consume nonexistent stock"));
 W.Item=TEXT("fuel");W.Contents.Add(TEXT("fuel"),1);W.Payload=3;W.bParcelWasLoaded=true;W.Delivered=4;W.DeliveredByTruck=3;W.ServiceDeliveries=2;W.DonorTests=1;W.ServiceTests=2;W.Service=4;W.Fuel=.7;W.BenchJob=TEXT("donor");W.BenchSeconds=1.2;W.GeneratorSeconds=7200;W.bGenerator=true;
 const auto Save=S47J::Read(S47J::Write(W.Save()));FS47FreeformWorld R(nullptr);R.Begin(Save);
 C(R.DeliveredParcels()==4&&R.CargoDeliveries()==3&&R.ServiceSupplyDeliveries()==2&&R.DonorTestsCompleted()==1&&R.ServiceCompletions()==2&&R.bParcelWasLoaded,TEXT("Freeform delivery and service counters survive JSON without repeating actions"));
 C(R.Payload==3&&R.Item==TEXT("fuel")&&R.Service==4&&R.Fuel==.7,TEXT("Freeform cargo and opened service survive a serialized restore"));
 C(R.BenchJob==TEXT("donor")&&R.BenchSeconds==1.2&&R.GeneratorSeconds==7200&&R.bGenerator,TEXT("Freeform in-progress donor and generator preserve consumed inputs"));
 C(!R.Purchase(TEXT("relay"))&&R.Payload==3,TEXT("Freeform purchase requires actual world and cannot duplicate loaded parcel"));
 W.Contents.Reset();W.Contents.Add(TEXT("fuse"),2);W.Contents.Add(TEXT("relay"),1);W.Item=TEXT("fuse");
 FS47FreeformWorld Basket(nullptr);Basket.Begin(S47J::Read(S47J::Write(W.Save())));
 C(Basket.ParcelStage()==3&&Basket.ParcelContents().FindRef(TEXT("fuse"))==2&&Basket.ParcelContents().FindRef(TEXT("relay"))==1&&Basket.ParcelDescription().Contains(TEXT("2 x control fuse")),TEXT("Paid multi-item contents survive JSON cargo restore without becoming stock"));
 auto LegacyContents=W.Save();LegacyContents->RemoveField(TEXT("contents"));FS47FreeformWorld Single(nullptr);Single.Begin(LegacyContents);
 C(Single.ParcelContents().Num()==1&&Single.ParcelContents().FindRef(TEXT("fuse"))==1,TEXT("Legacy paid singleton migrates once when contents is absent"));
 auto Invalid=W.Save();auto Bad=S47J::New();S47J::Set(Bad,TEXT("unknown"),2);S47J::Set(Invalid,TEXT("contents"),Bad);FS47FreeformWorld BadBasket(nullptr);BadBasket.Begin(Invalid);
 C(BadBasket.ParcelStage()==0&&BadBasket.ParcelContents().IsEmpty(),TEXT("Explicit corrupt contents cannot fall back to a free singleton"));
 W.Payload=1;C(W.CanAddShopItem(),TEXT("Paid counter parcel accepts additional ordinary supplies"));
 bool Locked=true;for(int32 Stage=2;Stage<=4;++Stage){W.Payload=Stage;Locked&=!W.CanAddShopItem();}C(Locked,TEXT("Carried, loaded and dropped parcels cannot receive remote shop additions"));
 W.Payload=3;W.Item=TEXT("fuel");W.Contents.Reset();W.Contents.Add(TEXT("fuel"),1);
 Basket.Stop();Single.Stop();BadBasket.Stop();
 W.AddItem(TEXT("fragment"),1);W.AddItem(TEXT("archived_fragment"),1);W.BenchJob=TEXT("fragment");W.BenchSeconds=1.7;
 FS47FreeformWorld Sample(nullptr);Sample.Begin(S47J::Read(S47J::Write(W.Save())));
 C(Sample.InventoryCount(TEXT("fragment"))==1&&Sample.InventoryCount(TEXT("archived_fragment"))==1&&Sample.BenchJob==TEXT("fragment")&&Sample.BenchSeconds==1.7,TEXT("Freeform field fragments, sealed originals and unfinished analysis persist"));
 C(Price(TEXT("fragment"))==0&&Price(TEXT("archived_fragment"))==0,TEXT("Freeform field samples cannot be purchased or loaned as shop stock"));
 W.Item=TEXT("service_case");W.Payload=4;W.Dropped=FVector(24,.45,8);
 auto Legacy=W.Save();Legacy->RemoveField(TEXT("carry"));FS47FreeformWorld Case(nullptr);Case.Begin(S47J::Read(S47J::Write(Legacy)));
 C(Case.ParcelItem()==TEXT("service_case")&&Case.ParcelStage()==4&&Case.ParcelPosition().Equals(W.Dropped,.0001)&&Case.InventoryCount(TEXT("service_case"))==0,TEXT("Reusable case keeps one saved identity and old dropped saves retain their position without becoming stock"));
 // Only the removed table's support migrates; carry pose is authoritative over old dropped data.
 auto OldTable=W.Save();OldTable->RemoveField(TEXT("eastWingAnchors"));OldTable->RemoveField(TEXT("carry"));
 S47J::Set(OldTable,TEXT("dropped"),TArray<double>{24,.43,6.2});
 FS47FreeformWorld MovedLegacy(nullptr);MovedLegacy.Begin(OldTable);
 const FVector ReceivingPose=FVector(24,.43,6.2)+DeliveryAt-OldBenchAt;
 C(MovedLegacy.ParcelPosition().Equals(ReceivingPose,.0001)&&MovedLegacy.Payload==4&&MovedLegacy.Item==TEXT("service_case"),TEXT("Legacy package on removed bench moves once to garage receiving with its identity intact"));
 auto CarryPose=S47J::New();S47J::Set(CarryPose,TEXT("v"),1);S47J::Set(CarryPose,TEXT("scene"),TEXT("saro"));
 S47J::Set(CarryPose,TEXT("position"),TArray<double>{24,.43,6.2});S47J::Set(CarryPose,TEXT("support"),TArray<double>{24,.27,6.2});
 S47J::Set(CarryPose,TEXT("rotation"),TArray<double>{0,0,0,1});S47J::Set(CarryPose,TEXT("normal"),TArray<double>{0,0,1});
 S47J::Set(CarryPose,TEXT("velocity"),TArray<double>{1,2,3});S47J::Set(OldTable,TEXT("carry"),CarryPose);
 FS47FreeformWorld MovedModern(nullptr);MovedModern.Begin(OldTable);const auto Migrated=MovedModern.Save();
 FVector Support;ReadPoint(S47J::Obj(Migrated,TEXT("carry")),TEXT("support"),Support);
 C(MovedModern.ParcelPosition().Equals(ReceivingPose,.0001)&&Support.Equals(FVector(24,.27,6.2)+DeliveryAt-OldBenchAt,.0001)
  &&MovedModern.Dropped.Equals(ReceivingPose,.0001)&&S47J::Nums(CarryPose,TEXT("position"))[0]==24,TEXT("Modern package migration translates pose, support and legacy anchor without mutating the input save"));
 FS47FreeformWorld MovedAgain(nullptr);MovedAgain.Begin(S47J::Read(S47J::Write(Migrated)));
 C(MovedAgain.ParcelPosition().Equals(ReceivingPose,.0001)&&MovedAgain.DeliveredParcels()==MovedModern.DeliveredParcels(),TEXT("East-wing migration survives JSON and Continue without another move or delivery credit"));
 S47J::Set(CarryPose,TEXT("position"),TArray<double>{12,.43,8});S47J::Set(CarryPose,TEXT("support"),TArray<double>{12,.27,8});
 FS47FreeformWorld CustomDrop(nullptr);CustomDrop.Begin(OldTable);
 C(CustomDrop.ParcelPosition().Equals(FVector(12,.43,8),.0001),TEXT("Player-chosen modern placement wins over stale legacy dropped coordinates"));
 OldTable->RemoveField(TEXT("carry"));S47J::Set(OldTable,TEXT("dropped"),TArray<double>{24,-.41,6.2});
 FS47FreeformWorld GroundDrop(nullptr);GroundDrop.Begin(OldTable);
 C(GroundDrop.ParcelPosition().Equals(FVector(24,-.41,6.2),.0001),TEXT("Ground-level parcel near the old bench stays at the player's chosen outdoor location"));
 C(W.InteractionPoint(TEXT("ff:sample")).X<26&&W.InteractionPoint(TEXT("ff:depot")).Z<.5
  &&W.InteractionPoint(TEXT("ff:delivery")).X>31.6&&!W.InteractionPoint(TEXT("ff:bench")).Equals(W.InteractionPoint(TEXT("ff:depot"))),TEXT("Analysis, parts stock and delivery have separate physical anchors in their east-wing rooms"));
 C(FMath::IsNearlyEqual(PumpIncrement(3.9,1.),.1)&&PumpIncrement(4.,1.)==0&&FMath::IsNearlyEqual(PumpIncrement(2.,.25),.25),TEXT("Pump metering respects remaining delivery and the actual four-unit tank capacity"));
 W.Fuel=2.25;W.bPumping=true;W.PumpRemaining=.75;W.PumpElapsed=1.3;W.PumpVolume=.25;W.PumpPaid=1;
 FS47FreeformWorld Pump(nullptr);Pump.Begin(S47J::Read(S47J::Write(W.Save())));
 C(Pump.PumpActive()&&Pump.VehicleFuel()==2.25&&Pump.PumpDelivered()==.25&&Pump.PumpCharges()==1&&FMath::IsNearlyEqual(Pump.PumpElapsed,1.3),TEXT("Pump restore preserves delivered/paid fuel and only unfinished metering time; it does not replay a charge"));
 Pump.StopPump();Pump.StopPump();C(Pump.VehicleFuel()==2.25&&Pump.PumpDelivered()==.25&&Pump.PumpCharges()==1&&!Pump.PumpActive()&&Pump.PumpElapsed==0,TEXT("Stopping pump repeatedly never charges or grants the incomplete delivery interval"));
 auto OldPump=S47J::New();S47J::Set(OldPump,TEXT("v"),1);S47J::Set(OldPump,TEXT("fuel"),.7);Pump.Begin(OldPump);
 C(!Pump.PumpActive()&&Pump.VehicleFuel()==.7&&Pump.PumpCharges()==0,TEXT("Old Freeform saves retain their tank and start with no pending pump transaction"));
 // Continue must preserve the already consumed replacement and both local-test legs.
 for(int32 Stage=1;Stage<=9;++Stage)
 {
  W.Service=Stage;W.ServiceAz=358.5;W.ServiceEl=31.25;W.Settle=.7;W.Stock.FindOrAdd(TEXT("relay"))=Stage<5?1:0;
  FS47FreeformWorld ServiceRestore(nullptr);ServiceRestore.Begin(S47J::Read(S47J::Write(W.Save())));
  C(ServiceRestore.ServiceStage()==Stage&&ServiceRestore.InventoryCount(TEXT("relay"))==(Stage<5?1:0)&&ServiceRestore.ServiceAz==358.5&&ServiceRestore.ServiceEl==31.25&&ServiceRestore.Settle==.7,TEXT("S-03 Continue retains repair stage, one-time part consumption and local-test progress"));
 }
 C(W.InteractionPoint(TEXT("ff:service")).Equals(FVector(4.647,-.01,-39.4),.001),TEXT("S-03 service uses the original cable-junction front, not a second stand"));
 TSet<FString> Targets;bool Valid=true;
 for(int32 N=0;N<27;++N){const FString Id=OrdinaryServiceDish(N);Targets.Add(Id);Valid&=ValidServiceDish(Id)&&Id==OrdinaryServiceDish(N+27);}
 C(Valid&&Targets.Num()==27&&!ValidServiceDish(TEXT("S-00"))&&!ValidServiceDish(TEXT("S-28"))&&!ValidServiceDish(TEXT("S-3")),TEXT("Ordinary service visits each of 27 actual antenna IDs before repeating and rejects malformed targets"));
 bool Preserved=true;
 for(int32 N=1;N<=27;++N)for(int32 Stage:{1,4,5,7,8,9})
 {
  W.ServiceTarget=FString::Printf(TEXT("S-%02d"),N);W.Service=Stage;W.OrdinaryServiceSerial=19;
  W.Stock.FindOrAdd(TEXT("relay"))=Stage<5?1:0;
  FS47FreeformWorld Copy(nullptr);Copy.Begin(S47J::Read(S47J::Write(W.Save())));
  Preserved&=Copy.ServiceDishId()==W.ServiceDishId()&&Copy.ServiceStage()==Stage&&Copy.OrdinaryServiceSerial==19
   &&Copy.InventoryCount(TEXT("relay"))==(Stage<5?1:0)&&Copy.ServiceAz==W.ServiceAz&&Copy.ServiceEl==W.ServiceEl&&Copy.Settle==W.Settle;
 }
 C(Preserved,TEXT("All 27 cabinet targets retain repair/test stage, relay consumption, encoder origin and selection sequence through Continue"));
 auto OldService=W.Save();OldService->RemoveField(TEXT("serviceDish"));OldService->RemoveField(TEXT("ordinaryServiceSerial"));
 FS47FreeformWorld OldServiceCopy(nullptr);OldServiceCopy.Begin(S47J::Read(S47J::Write(OldService)));
 C(OldServiceCopy.ServiceDishId()==TEXT("S-03")&&OldServiceCopy.OrdinaryServiceSerial==0&&OldServiceCopy.ServiceStage()==W.ServiceStage(),TEXT("Legacy service saves keep original S-03 identity and stage without choosing another fault"));
 S47J::Set(OldService,TEXT("serviceDish"),TEXT("S-99"));OldServiceCopy.Begin(OldService);
 C(OldServiceCopy.ServiceDishId()==TEXT("S-03"),TEXT("Invalid saved service target falls back to original S-03 instead of indexing unrelated geometry"));
 W.ServiceTarget=TEXT("S-03");

 FS47FreeformWorld Rescue(nullptr);Rescue.Begin();Rescue.Stock.FindOrAdd(TEXT("fuse"))=0;Rescue.EmergencyDay=7;Rescue.Feeder=3;
 Rescue.TrackPowerFault(true);const int32 Fault=Rescue.PowerFaultSerial;
 C(Fault==1&&Rescue.CanIssueEmergencyFuse(true)&&!Rescue.CanIssueEmergencyFuse(false),TEXT("Feeder rescue is tied to an actual outage and is not consumed by the ordinary daily requisition"));
 bool ParcelGuard=true;for(int32 Stage=1;Stage<=4;++Stage){Rescue.Payload=Stage;Rescue.Item=TEXT("fuse");ParcelGuard&=!Rescue.CanIssueEmergencyFuse(true);}
 Rescue.Payload=0;Rescue.Item.Empty();Rescue.Stock.FindOrAdd(TEXT("fuse"))=1;const bool StockGuard=!Rescue.CanIssueEmergencyFuse(true);
 Rescue.Stock.FindOrAdd(TEXT("fuse"))=0;Rescue.Feeder=4;
 C(ParcelGuard&&StockGuard&&!Rescue.CanIssueEmergencyFuse(true),TEXT("A shop, carried, loaded or dropped parcel, retained fuse or fitted replacement prevents an extra rescue fuse"));
 Rescue.Feeder=3;Rescue.EmergencyFuseFault=Fault;Rescue.TrackPowerFault(true);
 FS47FreeformWorld RescueCopy(nullptr);RescueCopy.Begin(S47J::Read(S47J::Write(Rescue.Save())));RescueCopy.TrackPowerFault(true);
 C(RescueCopy.PowerFaultSerial==Fault&&RescueCopy.EmergencyFuseFault==Fault&&!RescueCopy.CanIssueEmergencyFuse(true),TEXT("Issued feeder rescue survives Continue and cannot be renewed during the same outage"));
 RescueCopy.TrackPowerFault(false);RescueCopy.TrackPowerFault(true);RescueCopy.TrackPowerFault(true);
 C(RescueCopy.PowerFaultSerial==Fault+1&&RescueCopy.CanIssueEmergencyFuse(true),TEXT("Only a restored feeder followed by a new failure creates another bounded repair entitlement"));
 auto OldRescue=Rescue.Save();OldRescue->RemoveField(TEXT("powerFaultSerial"));OldRescue->RemoveField(TEXT("emergencyFuseFault"));OldRescue->RemoveField(TEXT("trackedPowerFault"));
 RescueCopy.Begin(S47J::Read(S47J::Write(OldRescue)));RescueCopy.TrackPowerFault(true);
 C(RescueCopy.PowerFaultSerial==1&&RescueCopy.CanIssueEmergencyFuse(true)&&RescueCopy.InventoryCount(TEXT("fuse"))==0&&RescueCopy.ParcelStage()==0,TEXT("Legacy outage gains a repair identity without inventing stock, a paid parcel or a transaction"));
 C(!RescueCopy.RequestEmergencyFuse()&&RescueCopy.EmergencyFuseFault==0&&RescueCopy.ParcelStage()==0,TEXT("A rescue request still requires the actual player at the indoor store counter"));
 {
  const auto& WestBranch=FS47ArrayWear::Branches()[0];FS47FreeformWorld Reserve(nullptr);Reserve.Begin();Reserve.Stock.FindOrAdd(TEXT("fuse"))=2;Reserve.Feeder=3;Reserve.TrackPowerFault(true);
  const bool BeforeIssue=!Reserve.BranchStrandsFeeder(WestBranch,true);Reserve.EmergencyFuseFault=Reserve.PowerFaultSerial;const bool AfterIssue=Reserve.BranchStrandsFeeder(WestBranch,true);
  Reserve.Stock.FindOrAdd(TEXT("fuse"))=3;const bool SpareLeft=!Reserve.BranchStrandsFeeder(WestBranch,true);Reserve.Stock.FindOrAdd(TEXT("fuse"))=2;
  C(BeforeIssue&&AfterIssue&&SpareLeft&&!Reserve.BranchStrandsFeeder(WestBranch,false)&&!Reserve.BranchStrandsFeeder(FS47ArrayWear::Branches()[1],true),TEXT("A fuse branch cannot spend the one credit fuse of an active outage; other branches, spare fuses and normal mains are unaffected"));
  Reserve.Stop();
 }
 FS47FreeformSession Credit(nullptr);Credit.Begin(nullptr);auto CreditState=Credit.Save();
 S47J::Set(CreditState,TEXT("cash"),0);S47J::Set(CreditState,TEXT("loan"),25);Credit.Begin(CreditState);
 C(!Credit.Spend(5,true)&&Credit.Budget()==0&&Credit.Debt()==25&&Credit.ChargeEmergencyFeederFuse()&&Credit.Budget()==0&&Credit.Debt()==30,TEXT("Existing debt still blocks ordinary credit but the bounded feeder transaction adds exactly five dollars without requiring working instruments"));
 FS47FreeformSession CreditCopy(nullptr);CreditCopy.Begin(S47J::Read(S47J::Write(Credit.Save())));
 C(CreditCopy.Budget()==0&&CreditCopy.Debt()==30,TEXT("Continue preserves the repair charge and never repeats it"));
 CreditCopy.Earn(20);const bool FirstPayment=CreditCopy.Debt()==10&&CreditCopy.Budget()==0;CreditCopy.Earn(20);
 C(FirstPayment&&CreditCopy.Debt()==0&&CreditCopy.Budget()==10,TEXT("Later completed work repays repair and previous debt before paying cash"));
 S47J::Set(CreditState,TEXT("cash"),2);Credit.Begin(CreditState);
 C(Credit.ChargeEmergencyFeederFuse()&&Credit.Budget()==0&&Credit.Debt()==28,TEXT("Partial cash funds the fuse first; only the missing amount becomes repair debt"));
 S47J::Set(CreditState,TEXT("cash"),3);S47J::Set(CreditState,TEXT("loan"),999999);Credit.Begin(CreditState);
 C(!Credit.ChargeEmergencyFeederFuse()&&Credit.Budget()==3&&Credit.Debt()==999999,TEXT("Debt-limit rejection is atomic: neither budget nor debt changes"));
 Credit.Stop();CreditCopy.Stop();
 FS47FreeformWorld Jump(nullptr);Jump.Begin();Jump.LastClock=100;Jump.bGenerator=true;Jump.GeneratorSeconds=3600;
 Jump.Logger.Connect();Jump.Logger.Connect();Jump.Logger.Connect();Jump.Logger.Calibrate(3);Jump.Logger.Start(0,1,100);
 Jump.AdvanceUnobservedTo(3700);
 C(!Jump.GeneratorRunning()&&Jump.GeneratorSeconds==0&&Jump.Logger.Tapes.Last().GapSeconds==3600&&Jump.Logger.Tapes.Last().Seconds==0&&Jump.LastClock==3700,TEXT("Rest settles generator fuel and logger gaps before save without generating measurements"));
 FS47FreeformWorld JumpCopy(nullptr);JumpCopy.Begin(S47J::Read(S47J::Write(Jump.Save())));JumpCopy.AdvanceUnobservedTo(3700);
 C(JumpCopy.Logger.Battery==FS47FieldLogger::Capacity-3600&&JumpCopy.Logger.Tapes.Last().GapSeconds==3600&&!JumpCopy.GeneratorRunning(),TEXT("Continue after wait preserves consumed fuel and battery without a second charge"));Jump.Stop();JumpCopy.Stop();
 FS47FreeformSession::FJob Original;Original.Source=0;Original.bGenerated=true;Original.Card.Seconds=12;Original.Stage=FS47FreeformSession::EJobStage::Recorded;Original.Elapsed=12;
 C(!LoggerOriginalComplete(Original),TEXT("Logger comparison rejects stage-only job without retained physical original"));
 Original.OriginalPage=TEXT("unchanged actual original test document");Original.OriginalStamp=TEXT("original counter 123");
 C(LoggerOriginalComplete(Original),TEXT("Logger original gate accepts actual complete retained acquisition"));
 Original.Elapsed=4;C(!LoggerOriginalComplete(Original),TEXT("Logger comparison rejects partial acquisition even with recorded stage and document"));Original.Elapsed=12;Original.bLegacyTrace=true;
 C(!LoggerOriginalComplete(Original),TEXT("Logger comparison cannot promote reconstructed legacy trace into measured original"));
 {
  // M7 (C): the pump fills the tank in one go; the county's roadside return costs money and a wait, and is refused while something is close.
  double PTank=.1,PLeft=PumpVolumeFor(PTank);int32 PDollars=0;
  for(int32 Guard=0;Guard<100;++Guard){const double A=PumpIncrement(PTank,PLeft);if(A<.00001)break;PTank=FMath::Min(4.,PTank+A);PLeft-=A;++PDollars;}
  C(FMath::IsNearlyEqual(PTank,4.)&&PDollars==16,TEXT("M7: one use of the pump fills a dry tank from 0.1 to 4 for $16"));
  const FRoadsideTerms RDry=RoadsideTermsFor(false,FS47TruckCare::TowFeeFor(0),FS47TruckCare::TowMinutesFor(0),14,FString());
  const FRoadsideTerms RFar=RoadsideTermsFor(false,FS47TruckCare::TowFeeFor(20000),FS47TruckCare::TowMinutesFor(20000),14,FString());
  const FRoadsideTerms RNight=RoadsideTermsFor(false,FS47TruckCare::TowFeeFor(0),FS47TruckCare::TowMinutesFor(0),2.5,FString());
  const FRoadsideTerms RTow=RoadsideTermsFor(true,FS47TruckCare::TowFeeFor(4000),FS47TruckCare::TowMinutesFor(4000),14,FString());
  const FRoadsideTerms RNear=RoadsideTermsFor(false,35,45,2.5,TEXT("Not now."));
  C(RDry.Fee>0&&RDry.Fee==EmptyTankFee&&RDry.Minutes>=45&&RFar.Fee>RDry.Fee&&RFar.Fee<RTow.Fee+20&&RFar.Minutes<=90&&!RDry.bRefused,TEXT("M7: a dry tank's roadside return costs $20 to $45 and 45 to 90 minutes by how far out"));
  C(RNight.Minutes>=RDry.Minutes+NightTowWait-.001&&RTow.Fee==FS47TruckCare::TowFeeFor(4000)&&RNear.bRefused,TEXT("M7: after dark the wrecker is longer coming; a truck in trouble pays the full tow; nobody comes while something is close (RestBlock)"));
 }
 // SARO-GEN-RAMP-20261109: no generator button inside; one starts and fuels it at the standby generator outside, from the open
 // ground east of its housing (its walk box x 3.7..4.9, z 7.05..9.75; the pocket's zone x 2.65..6.55, z 5.95..20).
 {
  FS47FreeformWorld G(nullptr);G.Begin();
  const FVector At=G.InteractionPoint(TEXT("ff:generator")),Old(5.89,1.35,.65);
  const bool Outside=!(At.X>-6.7&&At.X<6.7&&At.Z>-5.9&&At.Z<5.9)&&!(At.X>-6.6&&At.X<2.6&&At.Z>5.8&&At.Z<11.1);
  C(Outside&&FVector2D(At.X-Old.X,At.Z-Old.Z).Size()>5&&G.InteractionPoint(TEXT("obs:generator"))==At,TEXT("SARO-GEN: no generator button in the control room; the start point is outside, not on its east wall"));
  C(At.X>4.85&&At.X<5.05&&At.Z>7.05&&At.Z<9.75&&At.Y>0&&At.Y<1&&At.X+.3<6.55-.27,TEXT("SARO-GEN: the start point is the standby generator's control box, on the housing's east face, with room to stand by it in the pocket"));
  // One standing on the ground just east of the box: in the pocket's zone, clear of the housing's walk box, near enough (Near: 2.9 m).
  const double Body=.27,Stand=At.X+GeneratorBox.X*.5+Body+.05;
  C(Stand>=2.65+Body&&Stand<=6.55-Body&&Stand>=4.9+Body&&FMath::Abs(Stand-At.X)<2.9&&FMath::Abs(-.6-At.Y)<3.,TEXT("SARO-GEN: one standing on the ground beside the box can start and fuel it"));
 }
 // SARO-GEN-RAMP-20261109: nothing on the way down off the truck ramp's sides for one on foot; the truck's lines and the pad stay.
 C(S47OpenLand::TruckRampSide(6.6,6.6,12.4,16.6)&&S47OpenLand::TruckRampSide(13,13,12.4,16.6),TEXT("SARO-RAMP: the truck ramp's west and east sides do not hold one on foot"));
 C(!S47OpenLand::TruckRampSide(6.6,6.6,3.4,12.4)&&!S47OpenLand::TruckRampSide(13,13,3.4,12.4)&&!S47OpenLand::TruckRampSide(6.6,6.6,3.4,16.6)&&!S47OpenLand::TruckRampSide(6.6,13,3.4,3.4)&&!S47OpenLand::TruckRampSide(6.15,6.45,16.75,17.05),
  TEXT("SARO-RAMP: the pad's sides, its north edge and the bollard at the ramp's foot still hold"));
 OK &= FS47FieldLogger::SelfTest(Lines);
 OK &= FS47FreeformCarry::SelfTest(Lines);return OK;
}

bool FS47FreeformWorld::LoggerAccess()const
{
 const auto* Session=M?M->Freeform.Get():nullptr;
 return Logger.bOwned||(Session&&Session->CaseFile().HasAccess(TEXT("access.fieldlogger")));
}
FVector FS47FreeformWorld::LoggerPoint(int32 Site)const
{
 // Both posts use the existing SARO grounds, separate from the service cabinet.
 FVector At=Site==0?FVector(8,0,5):FVector(34,0,-28);
 At.Y=US47Night::SaroFloorAt(M?M->Web.Get():nullptr,At.X,At.Z)+.8;return At;
}
FString FS47FreeformWorld::LoggerPreparationBlock(const FString& Action) const
{
 if(Action!=TEXT("tune")&&Action!=TEXT("take"))return TEXT("Unknown logger preparation action.");
 if(!Ready()||!LoggerAccess())return TEXT("Complete THE REPEATED INTERVAL to unlock the loan logger.");
 if(!Logger.bLoanIssued)return TEXT("Collect the loan kit at PARTS STORAGE first.");
 if(Logger.Site>=0)return TEXT("Retrieve the logger from its field site first.");
 if(Logger.bCheckedOut)return TEXT("Return the logger to CALIBRATION / REPAIR first.");
 if(M->Web->Area()!=TEXT("saro")||!Near(BenchAt))return TEXT("Use the CALIBRATION / REPAIR bench.");
 if(Payload!=0)return TEXT("Free both hands before preparing the logger.");
 if(Action==TEXT("tune"))return M->Freeform->ConsoleProfile()?FString():FString(TEXT("Select a SARO card in OPS / DAILY WORK. If work is finished, start an optional reference check."));
 if(bLoggerCalibrating)return TEXT("Wait for clock calibration to finish.");
 if(!Logger.bCalibrated||Logger.Battery<=0)return TEXT("Calibrate at this bench to recharge the logger.");
 if(Logger.Tapes.Num()>=256)return TEXT("Tape capacity is full. Retained originals can still be read here.");
 return FString();
}
bool FS47FreeformWorld::LoggerAction(const FString& Action,const FString& TapeId,const FString& OriginalId)
{
 if(Action==TEXT("tune")||Action==TEXT("take")){const FString Why=LoggerPreparationBlock(Action);if(!Why.IsEmpty()){if(Ready())Change(Why);return false;}}
 if(!Ready()||!LoggerAccess())return false;
 if(Action==TEXT("site0")||Action==TEXT("site1"))
 {
  const int32 Site=Action==TEXT("site0")?0:1;
  if(M->Web->Area()!=TEXT("saro")||!Near(LoggerPoint(Site)))return false;
  if(Logger.Site==Site)
  {
   if(Payload!=0){Change(TEXT("Free both hands before retrieving the logger and its original tape."));return false;}
   Logger.Finish(M->Freeform->Clock());if(!Logger.Retrieve())return false;
   Payload=2;Item=TEXT("fieldlogger");Contents.Reset();bParcelWasLoaded=false;if(Carry)Carry->Restore(nullptr,LoggerPoint(Site));
   Change(TEXT("CALIBRATION / REPAIR: Return the logger and sealed original tape at the workbench, then compare there."));return true;
  }
  if(Payload!=2||Item!=TEXT("fieldlogger")){Change(TEXT("Bring the calibrated logger here in your hands. First issue is at PARTS STORAGE; prepare and collect it at CALIBRATION / REPAIR."));return false;}
  if(!Logger.Start(Site,M->Freeform->Day(),M->Freeform->Clock())){Change(TEXT("CALIBRATION / REPAIR: Check logger calibration, battery and retained-tape capacity before deploying it."));return false;}
  Payload=0;Item.Empty();bParcelWasLoaded=false;LoggerJob.Empty();LoggerJobElapsed=0;
  Change(TEXT("Field logger recording for two hours. Its battery and clock run independently of the SARO receiver. Return to SARO for a simultaneous recording, then retrieve this original."));return true;
 }
 if(Action==TEXT("issue"))
 {
  if(Logger.bLoanIssued||Logger.Site>=0||Payload!=0||M->Web->Area()!=TEXT("saro")||!Near(DepotAt,2.6))return false;
  Logger.bLoanIssued=true;Logger.bCheckedOut=true;Logger.Checkouts=1;
  Payload=2;Item=TEXT("fieldlogger");Contents.Reset();bParcelWasLoaded=false;if(Carry)Carry->Restore(nullptr,DepotAt+FVector(0,1.05,0));if(M->UI)M->UI->Close();
  Change(TEXT("Loan fieldlogger01 issued. CALIBRATION / REPAIR: Carry this kit to the workbench and set it there to assemble and calibrate. Later collections and returns happen at that same bench."));return true;
 }
 if(!Logger.bLoanIssued||Logger.bCheckedOut||!Near(BenchAt)||M->Web->Area()!=TEXT("saro")||Payload!=0||Logger.Site>=0)return false;
 if(Action==TEXT("connect"))
 {
  if(!BenchJob.IsEmpty()||!Logger.Connect())return false;
  Change(Logger.Assembly==3?TEXT("CALIBRATION / REPAIR: Battery connected; run clock calibration before carrying the logger."):TEXT("CALIBRATION / REPAIR: Connection seated. Continue with the next labeled connection."));
 }
 else if(Action==TEXT("calibrate"))
 {
  if(Logger.Assembly!=3||!BenchJob.IsEmpty())return false;
  bLoggerCalibrating=true;Logger.Calibration=0;Logger.bCalibrated=false;if(M->UI)M->UI->Close();
  Change(TEXT("CALIBRATION / REPAIR: Stay at the powered bench for the automatic three-second clock reference. Logger reads 2.4 seconds ahead; the correction will travel with every original."));
 }
 else if(Action==TEXT("tune"))
 {
  const auto* P=M->Freeform->ConsoleProfile();if(!P)return false;Logger.Frequency=P->F;
  Change(FString::Printf(TEXT("Logger tuned to %.3f MHz from the selected SARO work card. This setting is retained when you change the station receiver."),Logger.Frequency));
 }
 else if(Action==TEXT("take"))
 {
  if(!Logger.bCalibrated||bLoggerCalibrating||Logger.Battery<=0||Logger.Tapes.Num()>=256||BenchJob.StartsWith(TEXT("ws:")))return false;
  Logger.bCheckedOut=true;Logger.Checkouts=FMath::Min(1000000,Logger.Checkouts+1);
  Payload=2;Item=TEXT("fieldlogger");Contents.Reset();bParcelWasLoaded=false;if(Carry)Carry->Restore(nullptr,BenchAt+FVector(0,1.05,0));if(M->UI)M->UI->Close();
  Change(FString(Logger.bOwned?TEXT("Field logger carried."):TEXT("Loan logger carried."))+TEXT(" Site A is on the generator service path; site B is at the east array edge. Cargo transport and setting down work as for the service case."));return true;
 }
 else if(Action==TEXT("compare"))
 {
  if(!M->UI)return false;TArray<S47Kit::FW> Rows;
  TArray<FString> FullOriginals;
  for(const auto& J:M->Freeform->Jobs())if(LoggerOriginalComplete(J))FullOriginals.Add(J.Id);
  if(!TapeId.IsEmpty()||!OriginalId.IsEmpty())
  {
   auto* Selected=Logger.Tapes.FindByPredicate([&](const FS47FieldLogger::FTape& T){return T.Id==TapeId;});
   if(!Selected||!Selected->bRetrieved||!FS47FieldLogger::EligibleOriginals(*Selected,FullOriginals).Contains(OriginalId)||(Selected->bCompared&&Selected->ComparedJob!=OriginalId))return false;
  }
  for(auto& T:Logger.Tapes)if(T.bRetrieved)
  {
   const auto Choices=FS47FieldLogger::EligibleOriginals(T,FullOriginals);
   FString Choice;
   if(!T.bCompared){if(T.Id==TapeId)Choice=OriginalId;else if(TapeId.IsEmpty()&&Choices.Num()==1)Choice=Choices[0];}
   const bool NewComparison=!T.bCompared&&!Choice.IsEmpty()&&FS47FieldLogger::ChooseOriginal(T,Choice,FullOriginals.Contains(Choice));
   const FString Text=Logger.Report(T)+(T.bCompared?TEXT("\nCOMPARISON: retained against SARO original ")+T.ComparedJob+TEXT(". Local noise and station signal remain separate measurements."):Choices.Num()>1?TEXT("\nCHOOSE A SARO ORIGINAL: more than one completed original has measured overlap. No comparison has been selected."):TEXT("\nCOMPARISON NOT ESTABLISHED: requires a full two-hour field window without gaps, a complete physically retained SARO original and at least five seconds of its actually observed matching acquisition."));
   Rows.Add(S47Kit::Paper(Text));
   if(!T.Levels.IsEmpty())Rows.Add(S47Kit::Button(TEXT("Listen to retained local noise / six-second compressed monitor"),[this,Levels=T.Levels]
   {
    if(!M||!M->Audio||!M->Audio->Ctx||!M->Audio->SfxBus)return;
    auto Ctx=M->Audio->Ctx;FS47AudioContext::FBatch Batch(*Ctx);auto Buffer=Ctx->CreateBuffer(1,48000,8000);float* Samples=Buffer->GetChannelData(0);FRandomStream Noise(4701986);
    for(int32 I=0;I<48000;++I){const double Envelope=Levels[FMath::Min(Levels.Num()-1,I*Levels.Num()/48000)];const double Fade=FMath::Min3(1.,I/160.,(48000-I)/160.);Samples[I]=float(Fade*(.18*Envelope*FMath::Sin(2*PI*60*I/8000.)+.025*Noise.FRandRange(-1.f,1.f)));}
    auto Source=Ctx->CreateBufferSource();Source->Buffer=Buffer;Source->Connect(M->Audio->SfxBus);Source->Start();
   }));
   if(!T.bCompared&&Choices.Num()>1)for(const auto& Job:Choices)
   {
    const auto* O=T.Overlaps.Find(Job);if(!O)continue;
    const FString When=O->First>=0?FS47FreeformCases::Clock(O->First)+TEXT(" to ")+FS47FreeformCases::Clock(O->Last):TEXT("legacy frame times unavailable");
    Rows.Add(S47Kit::Button(FString::Printf(TEXT("Compare with %s / %s / %.2f acquired seconds"),*Job,*When,O->Seconds),[this,Id=T.Id,Job]{LoggerAction(TEXT("compare"),Id,Job);},true));
   }
   if(NewComparison)M->Freeform->RecordEvidence(T.Id,Text);
  }
  if(Rows.IsEmpty())Rows.Add(S47Kit::Paper(TEXT("No physically returned field original. Deploy the logger, let it record, retrieve it and return it to this CALIBRATION / REPAIR bench, then compare here.")));
  M->UI->Open(S47Kit::Shell(M->UI,TEXT("FIELD ORIGINALS / LOCAL NOISE COMPARISON"),S47Kit::Col(Rows),880));Change();return true;
 }
 else return false;
 if(M->UI)M->UI->Close();return true;
}
void FS47FreeformWorld::StepLogger(double Dt,double WorldDelta)
{
 if(bLoggerCalibrating)
 {
  if(!Near(BenchAt)||M->Web->Area()!=TEXT("saro")||!BenchJob.IsEmpty()||(M->Freeform->PowerFault()&&!GeneratorRunning()))
  {bLoggerCalibrating=false;Logger.Calibration=0;Logger.bCalibrated=false;Change(TEXT("CALIBRATION / REPAIR: Clock calibration interrupted. Stay at the powered bench for the full reference test."));}
  else if(Logger.Calibrate(FMath::Min(Dt,.1)))
  {
   bLoggerCalibrating=false;Change(TEXT("CALIBRATION / REPAIR: Clock offset +2.4 s retained; battery charged for six hours. Tune from a work card, then collect the logger at this same bench and carry it to a field post."));
  }
  if(BenchLamp){BenchLamp.SetColor(S47Three::Hex(0xc8a046));BenchLamp.SetEmissive(S47Three::Hex(0xc8a046),float(.2+Logger.Calibration/3.));}
 }
 const auto* J=M->Freeform->CurrentJob();const FString Id=J?J->Id:FString();
 // Persisting the previous observed elapsed value is unnecessary: after Continue
 // the first frame rebaselines rather than claiming historical acquisition.
 const double Acquired=J&&Id==LoggerJob?FMath::Max(0.,J->Elapsed-LoggerJobElapsed):0;
 LoggerJob=Id;LoggerJobElapsed=J?J->Elapsed:0;
 const bool Was=Logger.bRecording;const auto* P=M->Freeform->ConsoleProfile();
 const double Now=M->Freeform->Clock();
 if(WorldDelta>0&&WorldDelta<=3&&GeneratorRunning()&&GeneratorSeconds<WorldDelta)
 {
  const double Fraction=GeneratorSeconds/WorldDelta;
  Logger.Sample(Now-WorldDelta+GeneratorSeconds,GeneratorSeconds,true,M->Freeform->PowerFault(),Id,Acquired*Fraction,P?P->F:0);
  Logger.Sample(Now,WorldDelta-GeneratorSeconds,false,M->Freeform->PowerFault(),Id,Acquired*(1-Fraction),P?P->F:0);
 }
 else Logger.Sample(Now,WorldDelta,GeneratorRunning(),M->Freeform->PowerFault(),Id,Acquired,P?P->F:0);
 if(Was&&!Logger.bRecording)Change(TEXT("Field logger recording window ended. The original remains at its field post until physically retrieved."));
}

// D45-20261110 (Tom's test of d45: the backup generator needs sound). No generator recording in the free or the bought sounds, so the
// truck's Jeep clips (engineStart, engineIdle, engineOff; Freesound, credited) played slower and through the housing, as the RV's
// generator (FS47Happenings) and Story's standby diesel (FS47Observing) do, placed at the control box behind the control room.
namespace { const TCHAR* const GenLoopKey=TEXT("ffGenerator"); constexpr double GenRate=.62,GenLevel=.34; }
void FS47FreeformWorld::GeneratorSound(EGenSound Kind)
{
 US47Audio* A=M&&IsValid(M->Audio)?M->Audio.Get():nullptr;
 if(!A||!A->Ctx.IsValid()||!M->Web||M->Web->Area()!=TEXT("saro"))return; // heard only at SARO; elsewhere the loop is already gone
 FS47Sound O;O.At=GeneratorAt;O.Lp=2400;O.Rate=.82f;O.Gain=.6f; // a bigger, slower engine than the truck's, through its housing
 switch(Kind)
 {
 case EGenSound::Start:A->Sfx(TEXT("engineStart"),O);GenLoopDelay=2.3/.82;break; // the loop takes over as the clip settles (2.3 s of it)
 case EGenSound::Dry: // the starter turns it twice (the clip's cranking, 0-1.4 s, before it catches) and it does not catch
  O.Dur=1.4f;A->Sfx(TEXT("engineStart"),O);O.When=1.9f;O.Rate=.76f;O.Dur=1.2f;A->Sfx(TEXT("engineStart"),O);break;
 case EGenSound::Off:O.Gain=.5f;A->Sfx(TEXT("engineOff"),O);A->Stop(GenLoopKey,.35f);bGenSound=false;GenSrc.Reset();GenGain.Reset();break;
 case EGenSound::Stall: // dry: it chugs and winds down (the rate falls as the CB's engine dies, S47CB.cpp)
  O.Gain=.5f;O.Rate=.72f;A->Sfx(TEXT("engineOff"),O);
  if(GenSrc.IsValid()){FS47AudioContext::FBatch B(*A->Ctx);const double T=A->Ctx->CurrentTime();GenSrc->PlaybackRate.CancelAndHoldAtTime(T);GenSrc->PlaybackRate.ExponentialRampToValueAtTime(.22,T+1.2);}
  A->Stop(GenLoopKey,.45f);bGenSound=false;GenSrc.Reset();GenGain.Reset();break;
 }
}
void FS47FreeformWorld::StepGeneratorSound(double Dt)
{
 US47Audio* A=M&&IsValid(M->Audio)?M->Audio.Get():nullptr;
 GenLoopDelay=FMath::Max(0.,GenLoopDelay-Dt);
 if(bGenSound) // stopped by someone else (a new audio context, a stop-all): start it again
 {
  bool bGone=!A||!A->Ctx.IsValid()||!GenSrc.IsValid();if(!bGone){FS47AudioContext::FBatch B(*A->Ctx);bGone=GenSrc->Ended();}
  if(bGone){bGenSound=false;GenSrc.Reset();GenGain.Reset();}
 }
 const bool bWant=A&&A->Ctx.IsValid()&&GeneratorRunning()&&M->Web&&M->Web->Area()==TEXT("saro");
 if(bWant&&!bGenSound)
 {
  FS47Sound O;O.At=GeneratorAt;O.Gain=float(GenLevel);O.Rate=float(GenRate);O.Lp=900;O.bLoop=true;O.When=float(GenLoopDelay);
  const FS47Play P=A->LoopSfx(GenLoopKey,TEXT("engineIdle"),O);bGenSound=P.IsValid();GenSrc=P.Src;GenGain=P.Gain;GenCoughIn=FMath::FRandRange(5.f,12.f);
  if(GenGain.IsValid()&&GenLoopDelay>0) // fades in under the start clip's tail, so the two engines meet without a seam
  {FS47AudioContext::FBatch B(*A->Ctx);const double T=A->Ctx->CurrentTime()+GenLoopDelay;GenGain->Gain.SetValueAtTime(0,T);GenGain->Gain.LinearRampToValueAtTime(GenLevel,T+.5);}
  GenLoopDelay=0;
 }
 else if(!bWant&&bGenSound){if(A)A->Stop(GenLoopKey,.8f);bGenSound=false;GenSrc.Reset();GenGain.Reset();}
 // the last twenty minutes in the tank: it hunts now and then, more often as it runs out
 if(bGenSound&&GenGain.IsValid()&&GeneratorSeconds<1200&&(GenCoughIn-=Dt)<=0)
 {
  FS47AudioContext::FBatch B(*A->Ctx);const double T=A->Ctx->CurrentTime();
  FS47AudioParam& G=GenGain->Gain;G.CancelAndHoldAtTime(T);G.LinearRampToValueAtTime(GenLevel*.1,T+.09);G.LinearRampToValueAtTime(GenLevel*1.15,T+.26);G.LinearRampToValueAtTime(GenLevel,T+.45);
  FS47AudioParam& R=GenSrc->PlaybackRate;R.CancelAndHoldAtTime(T);R.LinearRampToValueAtTime(GenRate*.72,T+.12);R.LinearRampToValueAtTime(GenRate,T+.4);
  GenCoughIn=FMath::FRandRange(2.5f,8.f)*(.3+GeneratorSeconds/1200.);
 }
}
void FS47FreeformWorld::StopGeneratorSound()
{
 const bool bWas=bGenSound;bGenSound=false;GenLoopDelay=GenCoughIn=0;GenSrc.Reset();GenGain.Reset();
 if(!bWas||IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)||!IsValid(M->Audio))return;
 M->Audio->Stop(GenLoopKey,.2f);
}
