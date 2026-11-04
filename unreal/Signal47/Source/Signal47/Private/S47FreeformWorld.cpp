#include "S47FreeformWorld.h"
#include "S47Freeform.h"
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
#include "S47Glow.h"
#include "S47Album.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CoreGlobals.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/GarbageCollection.h"

namespace
{
 // EASTWING-01: the service bench stands inside the east wing's calibration / repair room,
 // against the partition to parts storage (S47EastWing; the outdoor bench was retired).
 const FVector BenchAt(27.0,0.0,1.25);
 const FVector SampleAt(23.8,0,-3.8),DepotAt(28.8,0,-3.8),DeliveryAt(32.6,0,-4.15);
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
 const FVector GeneratorAt(5.93,1.35,.65);
 // Original S-03 CABLE JUNCTION front, unchanged ground placement and atlas.
 FVector ServiceAt() { return FVector(4.647,-.01,-39.4); }
 const TCHAR* Items[] = { TEXT("fuse"), TEXT("relay"), TEXT("donor"), TEXT("fuel"), TEXT("film") };
 int32 Price(const FString& Id) { return Id==TEXT("fuse")?5:Id==TEXT("relay")?25:Id==TEXT("donor")?15:(Id==TEXT("fuel")||Id==TEXT("film"))?10:0; }
 FString ItemTitle(const FString& Id) { return Id==TEXT("fuse")?TEXT("control fuse"):Id==TEXT("relay")?TEXT("drive relay"):Id==TEXT("donor")?TEXT("used controller / relay donor"):Id==TEXT("film")?TEXT("instant-film pack / 36 exposures"):Id==TEXT("fieldlogger")?TEXT("SARO loan field logger / fieldlogger01"):Id==TEXT("service_case")?TEXT("reusable SARO service case"):TEXT("six-hour fuel can"); }
 bool ValidItem(const FString& Id) { return Price(Id)>0 || Id==TEXT("battery") || Id==TEXT("fragment") || Id==TEXT("archived_fragment"); }
 // Cash is integer-valued: each completed metering interval delivers up to
 // one quarter of the shared tank unit and costs exactly one operating dollar.
 double PumpIncrement(double Tank,double Remaining) { return FMath::Max(0.,FMath::Min(.25,FMath::Min(4.-Tank,Remaining))); }
 double DayClock(double T) { return FMath::Fmod(FMath::Fmod(T,86400.)+86400.,86400.); }
}

FS47FreeformWorld::FS47FreeformWorld(US47Main* InM) : M(InM), Carry(MakeShared<FS47FreeformCarry>(InM)) {}
FS47FreeformWorld::~FS47FreeformWorld() { Stop(); Life.Reset(); }
bool FS47FreeformWorld::Ready() const { return bActive && M && M->Freeform && M->Freeform->Active() && M->Web && M->Player; }
int32 FS47FreeformWorld::InventoryCount(const FString& Id) const { return Stock.FindRef(Id); }
void FS47FreeformWorld::AddItem(const FString& Id,int32 Count) { if(ValidItem(Id)&&Count>0) { Stock.FindOrAdd(Id)=FMath::Clamp(Stock.FindRef(Id)+Count,0,99); Change(); } }
bool FS47FreeformWorld::Consume(const FString& Id) { int32* N=Stock.Find(Id); if(!N||*N<=0)return false;--*N;Change();return true; }
int32 FS47FreeformWorld::S03() const { if(M&&M->Web)for(int32 I=0;I<M->Web->DishCount();++I)if(M->Web->DishId(I)==TEXT("S-03"))return I;return INDEX_NONE; }

void FS47FreeformWorld::Begin(const S47J::FObj& O)
{
 Stop(); Logger.Load(S47J::Obj(O,TEXT("fieldlogger")));bLoggerCalibrating=false;LoggerJob.Empty();LoggerJobElapsed=0; bActive=true;Service=Feeder=LastFaultDay=EmergencyDay=Payload=0;Item.Empty();Contents.Reset();BenchJob.Empty();
 PowerFaultSerial=EmergencyFuseFault=0;bTrackedPowerFault=false;
 bGenerator=false;GeneratorSeconds=BenchSeconds=FeederSeconds=Settle=ServiceAz=ServiceEl=0;Fuel=3;LastClock=-1;Dropped=FVector::ZeroVector;
 Delivered=DeliveredByTruck=ServiceDeliveries=DonorTests=ServiceTests=0;bParcelWasLoaded=false;
 bPumping=false;PumpRemaining=PumpElapsed=PumpVolume=0;PumpPaid=0;LastPumpDisplay.Empty();
 Stock.Reset();Stock.Add(TEXT("fuse"),3);Stock.Add(TEXT("relay"),1);Stock.Add(TEXT("fuel"),2);Stock.Add(TEXT("donor"),0);Stock.Add(TEXT("battery"),0);Stock.Add(TEXT("film"),0);Stock.Add(TEXT("fragment"),0);Stock.Add(TEXT("archived_fragment"),0);
 if(O && S47J::Int(O,TEXT("v"))==1)
 {
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
    else if(!Contents.Contains(Item))for(const TCHAR* Key:Items)if(Contents.Contains(Key)){Item=Key;break;}
   }
  }
  bParcelWasLoaded=S47J::Bool(O,TEXT("parcelLoaded"),Payload==3);
  Delivered=FMath::Clamp(S47J::Int(O,TEXT("delivered")),0,1000000);DeliveredByTruck=FMath::Clamp(S47J::Int(O,TEXT("cargoDelivered")),0,1000000);
  ServiceDeliveries=FMath::Clamp(S47J::Int(O,TEXT("serviceDelivered")),0,1000000);DonorTests=FMath::Clamp(S47J::Int(O,TEXT("donorTests")),0,1000000);ServiceTests=FMath::Clamp(S47J::Int(O,TEXT("serviceTests")),0,1000000);
  bGenerator=S47J::Bool(O,TEXT("generator"));GeneratorSeconds=FMath::Clamp(S47J::Num(O,TEXT("generatorSeconds")),0.,43200.);
  Fuel=FMath::Clamp(S47J::Num(O,TEXT("fuel"),3),0.,4.);LastClock=S47J::Num(O,TEXT("lastClock"),-1);
  bPumping=S47J::Bool(O,TEXT("pumping"));PumpRemaining=FMath::Clamp(S47J::Num(O,TEXT("pumpRemaining")),0.,1.);
  PumpElapsed=FMath::Clamp(S47J::Num(O,TEXT("pumpElapsed")),0.,2.);PumpVolume=FMath::Clamp(S47J::Num(O,TEXT("pumpVolume")),0.,1.);PumpPaid=FMath::Clamp(S47J::Int(O,TEXT("pumpPaid")),0,4);
  if(PumpRemaining<.00001||Fuel>=3.99999)bPumping=false;
  ServiceAz=S47J::Num(O,TEXT("serviceAz"));ServiceEl=S47J::Num(O,TEXT("serviceEl"));Settle=FMath::Clamp(S47J::Num(O,TEXT("settle")),0.,1.);
  BenchJob=S47J::Str(O,TEXT("benchJob"));if(BenchJob!=TEXT("donor")&&BenchJob!=TEXT("fragment"))BenchJob.Empty();BenchSeconds=FMath::Clamp(S47J::Num(O,TEXT("benchSeconds")),0.,3.);
  FeederSeconds=FMath::Clamp(S47J::Num(O,TEXT("feederSeconds")),0.,3.);
  const auto P=S47J::Nums(O,TEXT("dropped"));if(P.Num()==3 && FMath::IsFinite(P[0])&&FMath::IsFinite(P[1])&&FMath::IsFinite(P[2]))Dropped=FVector(P[0],P[1],P[2]);else if(Payload==4){Payload=1;}
  const auto S=S47J::Obj(O,TEXT("stock"));if(S)for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("donor"),TEXT("fuel"),TEXT("battery"),TEXT("film"),TEXT("fragment"),TEXT("archived_fragment")})Stock.FindOrAdd(Id)=FMath::Clamp(S47J::Int(S,Id),0,99);
 }
 if(Logger.Site>=0&&Item==TEXT("fieldlogger")){Item.Empty();Payload=0;}
 if(Logger.Assembly!=3&&Item==TEXT("fieldlogger")){Item.Empty();Payload=0;}
 if(M&&M->Freeform)TrackPowerFault(M->Freeform->PowerFault());
 const auto RestoredCarry=MigrateBenchParcel(O,Dropped,Payload);
 Build();if(Carry)Carry->Restore(RestoredCarry,Payload==4?Dropped:FVector::ZeroVector);OverrideExisting();
 if(M&&M->Web)for(auto& C:M->Web->CollidersMutable(TEXT("saro")))if(C.Id==TEXT("ff:bench")||C.Id==TEXT("ff:sample")||C.Id==TEXT("ff:depot")||C.Id==TEXT("ff:delivery")||C.Id==TEXT("ff:service"))C.bEnabled=true;
 if(M&&M->Web&&S03()>=0)
 {
  M->Web->SetDishRemoteAvailable(S03(),!HasArrayFault());
  if(Service==7)M->Web->PointDish(S03(),ServiceAz+5,ServiceEl,2.2,true);
  if(Service==8)M->Web->PointDish(S03(),ServiceAz,ServiceEl,2.2,true);
 }
 Apply();
}

void FS47FreeformWorld::Stop()
{
 bActive=false;bPumping=false;if(Carry)Carry->Stop();
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
 for(auto& C:Web->CollidersMutable(TEXT("saro")))if(C.Id==TEXT("ff:bench")||C.Id==TEXT("ff:sample")||C.Id==TEXT("ff:depot")||C.Id==TEXT("ff:delivery")||C.Id==TEXT("ff:service"))C.bEnabled=false;
 if(S03()>=0)Web->SetDishRemoteAvailable(S03(),true);
 for(FS47Obj O:Parts)if(O.W==Web&&O)
 {
  O.SetVisible(false);
  if(const auto* N=Web->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))
   if(Live(C))C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }
}

S47J::FObj FS47FreeformWorld::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("eastWingAnchors"),1);S47J::Set(O,TEXT("fieldlogger"),Logger.Save());S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("service"),Service);S47J::Set(O,TEXT("feeder"),Feeder);
 S47J::Set(O,TEXT("faultDay"),LastFaultDay);S47J::Set(O,TEXT("emergencyDay"),EmergencyDay);S47J::Set(O,TEXT("payload"),Payload);S47J::Set(O,TEXT("item"),Item);
 auto C=S47J::New();if(Payload>0&&Price(Item)>0)for(const auto& Pair:Contents)S47J::Set(C,*Pair.Key,Pair.Value);S47J::Set(O,TEXT("contents"),C);
 S47J::Set(O,TEXT("powerFaultSerial"),PowerFaultSerial);S47J::Set(O,TEXT("emergencyFuseFault"),EmergencyFuseFault);S47J::Set(O,TEXT("trackedPowerFault"),bTrackedPowerFault);
 S47J::Set(O,TEXT("parcelLoaded"),bParcelWasLoaded);S47J::Set(O,TEXT("delivered"),Delivered);S47J::Set(O,TEXT("cargoDelivered"),DeliveredByTruck);S47J::Set(O,TEXT("serviceDelivered"),ServiceDeliveries);S47J::Set(O,TEXT("donorTests"),DonorTests);S47J::Set(O,TEXT("serviceTests"),ServiceTests);
 if(Carry&&Payload>0)S47J::Set(O,TEXT("carry"),Carry->Save());
 S47J::Set(O,TEXT("generator"),bGenerator);S47J::Set(O,TEXT("generatorSeconds"),GeneratorSeconds);S47J::Set(O,TEXT("fuel"),Fuel);S47J::Set(O,TEXT("lastClock"),LastClock);
 S47J::Set(O,TEXT("pumping"),bPumping);S47J::Set(O,TEXT("pumpRemaining"),PumpRemaining);S47J::Set(O,TEXT("pumpElapsed"),PumpElapsed);S47J::Set(O,TEXT("pumpVolume"),PumpVolume);S47J::Set(O,TEXT("pumpPaid"),PumpPaid);
 S47J::Set(O,TEXT("serviceAz"),ServiceAz);S47J::Set(O,TEXT("serviceEl"),ServiceEl);S47J::Set(O,TEXT("settle"),Settle);
 S47J::Set(O,TEXT("benchJob"),BenchJob);S47J::Set(O,TEXT("benchSeconds"),BenchSeconds);S47J::Set(O,TEXT("feederSeconds"),FeederSeconds);
 S47J::Set(O,TEXT("dropped"),TArray<double>{Dropped.X,Dropped.Y,Dropped.Z});auto S=S47J::New();for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("donor"),TEXT("fuel"),TEXT("battery"),TEXT("film"),TEXT("fragment"),TEXT("archived_fragment")})S47J::Set(S,Id,Stock.FindRef(Id));S47J::Set(O,TEXT("stock"),S);return O;
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
FVector FS47FreeformWorld::InteractionPoint(const FString& Id) const
{
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
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service")||Id==TEXT("field:S-03"))return ServiceAt();
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
 const double Volume=FMath::Min(1.,4.-Fuel);const int32 Cost=FMath::CeilToInt(Volume/.25-.00001);
 if(Volume<.00001){Change(TEXT("Service truck tank is full."));return false;}
 if(M->Freeform->Budget()<Cost){Change(FString::Printf(TEXT("This metered fill needs up to $%d of operating budget. No fuel delivered or charged."),Cost));return false;}
 bPumping=true;PumpRemaining=Volume;PumpElapsed=PumpVolume=0;PumpPaid=0;
 Change(TEXT("REGULAR connected to SARO truck. Each completed two-second interval delivers up to 0.25 tank units for $1. E stops; only delivered fuel is charged."));return true;
}
void FS47FreeformWorld::StopPump()
{
 if(!bPumping)return;bPumping=false;PumpRemaining=PumpElapsed=0;
 ApplyPump();Change(FString::Printf(TEXT("Nozzle returned. Delivered %.2f tank units / charged $%d. Tank %.2f / 4."),PumpVolume,PumpPaid,Fuel));
}
void FS47FreeformWorld::StepPump(double Dt)
{
 if(!bPumping)return;
 if(!PumpConditions()){StopPump();return;}
 PumpElapsed+=FMath::Clamp(Dt,0.,.1);if(PumpElapsed<2.)return;
 const double Amount=PumpIncrement(Fuel,PumpRemaining);
 if(Amount<.00001||!M->Freeform->Spend(1)){StopPump();return;}
 // Spend's autosave request is deferred; fuel and receipt are updated in this
 // same game-thread transaction before any snapshot can observe the new cash.
 Fuel=FMath::Min(4.,Fuel+Amount);PumpRemaining=FMath::Max(0.,PumpRemaining-Amount);PumpVolume+=Amount;++PumpPaid;PumpElapsed=0;
 if(bFuelHeld&&M->World){M->World->HoldTruck(0);bFuelHeld=false;}
 if(PumpRemaining<.00001||Fuel>=3.99999)StopPump();else{M->RequestAutosave();ApplyPump();}
}
FString FS47FreeformWorld::PumpDisplay() const
{
 if(bPumping)return FString::Printf(TEXT("SARO / REGULAR\nTANK %.2f / 4\nSALE $%d / %.2f\nNEXT %.2f / $1\n%.0f%% / E STOP"),Fuel,PumpPaid,PumpVolume,PumpIncrement(Fuel,PumpRemaining),FMath::Clamp(PumpElapsed/2.,0.,1.)*100.);
 const double Volume=FMath::Min(1.,4.-Fuel);return FString::Printf(TEXT("SARO / REGULAR\nTANK %.2f / 4\nUP TO %.2f / $%d\nE START / E STOP\nLAST $%d / %.2f"),Fuel,Volume,FMath::CeilToInt(Volume/.25-.00001),PumpPaid,PumpVolume);
}
void FS47FreeformWorld::ApplyPump()
{
 const FString Display=PumpDisplay();if(Display==LastPumpDisplay)return;LastPumpDisplay=Display;
 if(bTripoPump)
 {
  const FString Left=FString::Printf(TEXT("TANK\n%.2f OF 4\nSALE $%d"),Fuel,PumpPaid);
  const FString Right=FString::Printf(TEXT("FUEL $4/U\n%.2f SOLD\nE %s"),PumpVolume,bPumping?TEXT("STOP"):TEXT("START"));
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
  const double Width=Mesh->GetBounds().BoxExtent.X*.02;
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
  auto O=M->Web->ObjAt(M->Web->AdoptNode(C,773,Name));Parts.Add(O);M->Web->MarkUsable(O.I,TEXT("ff:service"));return O;
 };
 if(!CabinetBody)CabinetBody=Part(TEXT("FreeformOriginalS03Body"),BodyMesh,FVector::ZeroVector,{0,1,3});
 if(!Lid)Lid=Part(TEXT("FreeformOriginalS03Door"),DoorMesh,FVector(4.647,-.01,-39.72),{1,2});
 for(int32 K=0;K<4;++K)if(auto* C=CabinetOriginals[K].Component.Get())
 {C->SetStaticMesh(Trim[K]);C->SetMaterial(0,CabinetOriginals[K].Materials[0].Get());}
 bServiceCabinet=true;return true;
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
 if(M){if(!Message.IsEmpty()&&M->UI)M->UI->Toast(Message,5);M->RequestAutosave();}Apply();
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
FS47Obj FS47FreeformWorld::Model(const FString& Name,const TCHAR* Asset,const FVector& At,double Width,int32 Parent)
{
 const auto* P=M->Web->Node(Parent);auto* Mesh=LoadObject<UStaticMesh>(nullptr,Asset);if(!P||!P->Comp.IsValid()||!Mesh)return {};
 const FBoxSphereBounds B=Mesh->GetBounds();const double K=Width*100/FMath::Max(1.,B.BoxExtent.X*2.);
 auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(P->Comp.Get());C->SetStaticMesh(Mesh);
 C->SetWorldScale3D(FVector(K));const FVector Offset(B.Origin.X,B.Origin.Y,B.Origin.Z-B.BoxExtent.Z);C->SetWorldLocation(S47Three::ToUE(At)-Offset*K);
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
 for(const TCHAR* Raw:{TEXT("substation:journal"),TEXT("substation:service"),TEXT("substation:lamp"),TEXT("obs:generator"),TEXT("town:pumps"),TEXT("field:S-03")})
 {
  const FString Id(Raw);if(Overrides.Contains(Id))continue;auto* I=M->Inter->Get(Id);if(!I)continue;
  FOverride O;O.bLook=I->bLook;O.Label=I->Label;O.Examine=I->Examine;O.Use=I->Use;Overrides.Add(Id,MoveTemp(O));
  const FString Target=Id==TEXT("field:S-03")?TEXT("ff:service"):Id;
  if(Id==TEXT("field:S-03"))I->bLook=false;
  I->Label=[this,Guard,Target]{return Guard.IsValid()?Label(Target):FString();};
  I->Examine=I->Label;I->Use=[this,Guard,Target]{if(Guard.IsValid())Use(Target);};
 }
}

bool FS47FreeformWorld::Build()
{
 if(bBuilt){if(M&&M->Web){InstallPumpModel(M->Web->Handle(TEXT("world.legs.road.area.group")));InstallServiceCabinet();}return true;}if(!M||!M->Web||!M->Web->IsLoaded()||!M->Inter)return false;
 const int32 Saro=M->Web->Handle(TEXT("ext.group")),Road=M->Web->Handle(TEXT("world.legs.road.area.group"));if(Saro<0||Road<0)return false;
 // Reuse project tables at distinct physical workstations inside the east wing.
 // EASTWING-01: the relay/donor calibration bench matches the wing's green steel benches.
 Bench=Model(TEXT("FreeformServiceBench"),TEXT("/Game/EastWing/Props/Workshop/SM_EW_GreenBench.SM_EW_GreenBench"),BenchAt,1.8,Saro);
 const double BenchRaise=Bench?.0925:0.; // green bench worktop .9325 vs the archive table's .84
 if(Bench)
 {
  Model(TEXT("FreeformBenchMultimeter"),TEXT("/Game/EastWing/Props/Lab/SM_EW_Multimeter.SM_EW_Multimeter"),BenchAt+FVector(.18,.9325,.02),.2332,Saro);
  Model(TEXT("FreeformBenchSolderingIron"),TEXT("/Game/EastWing/Props/Lab/SM_EW_SolderingIron.SM_EW_SolderingIron"),BenchAt+FVector(.62,.9325,.04),.213,Saro);
 }
 else Bench=Model(TEXT("FreeformServiceBench"),TEXT("/Game/ArchiveProps/SM_ArchiveProps_WorkTable.SM_ArchiveProps_WorkTable"),BenchAt,1.8,Saro);
 if(!Bench)Bench=Box(TEXT("FreeformServiceBenchTop"),BenchAt+FVector(0,.78,0),FVector(1.8,.12,.75),0x514638,Saro);
 Bind(Bench,TEXT("ff:bench"));
 for(int32 I=0;I<2;++I)LoggerPosts[I]=Box(FString::Printf(TEXT("FreeformLoggerPost%d"),I),LoggerPoint(I)-FVector(0,.4,0),FVector(.14,.8,.14),0x777e68,Saro,I==0?TEXT("ff:logger-a"):TEXT("ff:logger-b"));
 LoggerBody=Box(TEXT("FreeformDeployedLogger"),LoggerPoint(0),FVector(.46,.32,.38),0x59635d,Saro,TEXT("ff:logger-device"));
 M->Web->CollidersMutable(TEXT("saro")).Add({BenchAt.X-.92,BenchAt.X+.92,BenchAt.Z-.4,BenchAt.Z+.4,TEXT("ff:bench"),true});
 // EASTWING-01: field samples are analysed at a green steel lab bench with its own scope and jars.
 SampleBench=Model(TEXT("FreeformSampleBench"),TEXT("/Game/EastWing/Props/Workshop/SM_EW_GreenBench.SM_EW_GreenBench"),SampleAt,1.8,Saro);
 if(SampleBench)
 {
  const double Top=.9325;
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
 M->Web->CollidersMutable(TEXT("saro")).Add({SampleAt.X-.92,SampleAt.X+.92,SampleAt.Z-.4,SampleAt.Z+.4,TEXT("ff:sample"),true});
 M->Web->CollidersMutable(TEXT("saro")).Add({DepotAt.X-.74,DepotAt.X+.74,DepotAt.Z-.34,DepotAt.Z+.31,TEXT("ff:depot"),true});
 M->Web->CollidersMutable(TEXT("saro")).Add({DeliveryAt.X-.6,DeliveryAt.X+.6,DeliveryAt.Z-.4,DeliveryAt.Z+.4,TEXT("ff:delivery"),true});
 // EASTWING-01: SARO RT-12 relay test set; its bezel window frames the live status lamp below.
 // Bounds centre sits .061 m ahead of the case (test leads), hence the z offset.
 BenchMeter=Bench?Model(TEXT("FreeformBenchMeter"),TEXT("/Game/EastWing/Props/Extras/SM_EW_RelayTester.SM_EW_RelayTester"),BenchAt+FVector(-.44,.9325,-.10+.061),.42,Saro):FS47Obj();
 if(BenchMeter)Bind(BenchMeter,TEXT("ff:bench"));
 else BenchMeter=Box(TEXT("FreeformBenchMeter"),BenchAt+FVector(-.44,1.02+BenchRaise,-.10),FVector(.42,.28,.27),0x555b4c,Saro,TEXT("ff:bench"));
 BenchLamp=Box(TEXT("FreeformBenchTestIndicator"),BenchAt+FVector(-.44,1.04+BenchRaise,.05),FVector(.28,.11,.015),0x5e8b61,Saro);
 // Base-normalised authored tray rests on the actual green worktop; visibility still follows the retained sample.
 BenchSample=Model(TEXT("FreeformBenchSampleHolder"),TEXT("/Game/EastWing/Props/Extras/SM_EW_SampleHolder.SM_EW_SampleHolder"),SampleAt+FVector(.03,.9325,.05),.15,Saro);
 if(!BenchSample)BenchSample=Box(TEXT("FreeformBenchSampleHolder"),SampleAt+FVector(.03,.955,.05),FVector(.15,.045,.12),0x9f9b89,Saro);
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
 Generator=Box(TEXT("FreeformBackupSelector"),GeneratorAt,FVector(.20,.22,.10),0x776044,Saro,TEXT("ff:generator"));
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
 if(FS47Obj Sign=Model(TEXT("FreeformBenchLabel"),TEXT("/Game/EastWing/Props/Extras/SM_EW_CalibrationSign.SM_EW_CalibrationSign"),BenchAt+FVector(.02,.9325,-.27),.42,Saro))M->Web->MarkUsable(Sign.I,TEXT("ff:bench"));
 else LabelPart(TEXT("FreeformBenchLabel"),TEXT("CALIBRATION / REPAIR\nDONOR / LOGGER TEST"),BenchAt+FVector(0,1.45,-.35),90,3.5,Saro);
 if(FS47Obj Sign=Model(TEXT("FreeformSampleLabel"),TEXT("/Game/EastWing/Props/Extras/SM_EW_SampleAnalysisSign.SM_EW_SampleAnalysisSign"),SampleAt+FVector(-.05,.9325,-.29),.42,Saro))M->Web->MarkUsable(Sign.I,TEXT("ff:sample"));
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
 if(BenchSample)BenchSample.SetVisible(bActive&&(InventoryCount(TEXT("fragment"))>0||InventoryCount(TEXT("archived_fragment"))>0||BenchJob==TEXT("fragment")));
 if(BenchLamp){const bool Testing=!BenchJob.IsEmpty()||bLoggerCalibrating;BenchLamp.SetColor(S47Three::Hex(Testing?0xc8a046:0x5e8b61));BenchLamp.SetEmissive(S47Three::Hex(Testing?0xc8a046:0x5e8b61),Testing?.2f+float((bLoggerCalibrating?Logger.Calibration:BenchSeconds)/3.)*.8f:.15f);}
 const bool Open=bActive&&Service>=4&&Service<=5;
 if(Lid){Lid.SetRotY(Open?-1.4:0);Lid.SetVisible(bActive&&bServiceCabinet);}
 if(bServiceCabinet)
 {
  const double A=Open?-1.4:0;const FVector D(-.017,.41,.54);
  FS47GlowView(M->Web,TEXT("world.field.glow.points")).SetPos(2,FVector(4.647,-.01,-39.72)+FVector(FMath::Cos(A)*D.X+FMath::Sin(A)*D.Z,D.Y,-FMath::Sin(A)*D.X+FMath::Cos(A)*D.Z));
 }
 if(ServiceText.IsValid()){ServiceText->SetVisibility(Open&&bServiceCabinet);}
 for(FS47Obj O:{Panel,Relay,Selector})if(O){O.SetVisible(Open&&bServiceCabinet);if(const auto* N=O.W->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(Open&&bServiceCabinet?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}

 if(Relay)Relay.SetColor(S47Three::Hex(Service>=5?0x739470:0xb19651));
 if(Selector)
 {
  Selector.SetColor(S47Three::Hex(HasArrayFault()?0xb54e31:0x4e8d5c));
  // AdoptNode retains position, not native scale. Rotate the actual component
  // so the fitted 4.5 cm selector cannot become a one-metre interaction blocker.
  if(const auto* N=Selector.W->Node(Selector.I))if(auto* C=N->Comp.Get())
   C->SetWorldRotation(FRotator(0,0,Service>=3&&Service<=8?FMath::RadiansToDegrees(.65):0));
 }
 if(FeederLamp)FeederLamp.SetColor(S47Three::Hex(M&&M->Freeform&&M->Freeform->PowerFault()?0xc57d32:0x58a769));
 if(FeederLever)FeederLever.SetRotX(Feeder>=2&&Feeder<5?.55:0);
 const bool Road=M&&M->World&&(M->World->Area==TEXT("road")||M->World->Area==TEXT("diner"));
 if(Carry)Carry->Step(0,bActive?Payload:0,Item,Payload==1?ParcelShopPoint():Payload==3?TruckPoint():Dropped);
 for(auto Pair:TArray<TPair<FS47Obj,bool>>{{CargoSaro,!Road},{CargoRoad,Road}})if(Pair.Key)
 {
  const bool Show=bActive&&Pair.Value&&M&&M->World&&!M->World->bDriving;if(const auto* N=Pair.Key.W->Node(Pair.Key.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())){C->SetWorldLocation(S47Three::ToUE(TruckPoint()));C->SetCollisionEnabled(Show?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}Pair.Key.SetVisible(Show);
 }
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
 Change(TEXT("One feeder-repair fuse labelled / $5 charged to budget or requisition debt. Collect the parcel, deliver it to SARO stock and repair the isolated feeder. Existing debt does not block this repair."));return true;
}
bool FS47FreeformWorld::Purchase(const FString& Id,bool Emergency)
{
 if(!AtShopCounter()||Price(Id)<=0||(Emergency?Payload!=0:!CanAddShopItem()))return false;
 if(InventoryCount(Id)+Contents.FindRef(Id)>=99){Change(TEXT("SARO stock plus this paid parcel has reached the 99-item storage limit for that supply."));return false;}
 if(!Emergency&&!ShopOpen()){Change(TEXT("The shop counter sells supplies 08:00–20:00. Essential credit requisitions remain available here after hours."));return false;}
 if(Emergency&&((Id==TEXT("donor")||Id==TEXT("film"))||EmergencyDay==M->Freeform->Day())){Change(TEXT("One essential emergency parcel per day; donor equipment is sold during opening hours."));return false;}
 if(!M->Freeform->Spend(Price(Id),Emergency)){Change(M->Freeform->FeedbackText());return false;}
 if(Emergency)EmergencyDay=M->Freeform->Day();if(Payload==0){Item=Id;Contents.Reset();}++Contents.FindOrAdd(Id);Payload=1;bParcelWasLoaded=false;Change(TEXT("Paid and added to your counter parcel. Add more supplies or close the list and collect the sealed parcel."));return true;
}
bool FS47FreeformWorld::CanAddShopItem() const
{
 return Payload==0||(Payload==1&&Price(Item)>0&&!Contents.IsEmpty());
}
FString FS47FreeformWorld::ParcelDescription() const
{
 if(Contents.IsEmpty())return Payload>0?ItemTitle(Item):FString(TEXT("empty"));
 TArray<FString> Lines;
 for(const TCHAR* Key:Items)if(const int32 Count=Contents.FindRef(Key);Count>0)Lines.Add(FString::Printf(TEXT("%d x %s"),Count,*ItemTitle(Key)));
 return FString::Join(Lines,TEXT(", "));
}
void FS47FreeformWorld::OpenShop()
{
 if(!AtShopCounter()||!M->UI)return;TArray<S47Kit::FW> Rows;
 TrackPowerFault(M->Freeform->PowerFault());
 Rows.Add(S47Kit::Paper(FString::Printf(TEXT("SUPPLIES / 08:00–20:00 / BUDGET $%d\nBuy items here, then close this list and collect your parcel.\nLoad the Jeep; unpack at GARAGE RECEIVING to add stock."),M->Freeform->Budget())));
 if(Payload>0)Rows.Add(S47Kit::Paper(TEXT("PAID PARCEL / ")+ParcelDescription()));
 for(const TCHAR* Id:Items){const FString Key(Id);Rows.Add(S47Kit::Button(FString::Printf(TEXT("%s / $%d"),*ItemTitle(Key),Price(Key)),[this,Key]{Purchase(Key);OpenShop();},false,CanAddShopItem()&&ShopOpen()&&InventoryCount(Key)+Contents.FindRef(Key)<99));}
 Rows.Add(S47Kit::Small(TEXT("Essential after-hours/credit requisition: one per day, charged against future work. Donor controllers require bench testing before harvesting their relay.")));
 for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("fuel")}){const FString Key(Id);Rows.Add(S47Kit::Button(FString(TEXT("Emergency "))+ItemTitle(Key),[this,Key]{if(Purchase(Key,true))M->UI->Close();},false,Payload==0&&EmergencyDay!=M->Freeform->Day()));}
 if(M->Freeform->PowerFault())
 {
  Rows.Add(S47Kit::Small(TEXT("ACTIVE FEEDER FAULT: one repair fuse may be charged to requisition debt even with an older balance or today's essential parcel already used. Use existing stock or a paid parcel first.")));
  Rows.Add(S47Kit::Button(TEXT("Request feeder-repair fuse / $5 / credit available"),[this]{if(RequestEmergencyFuse())M->UI->Close();},true,EmergencyFuseAvailable()));
 }
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
 if(!M||!M->World||M->World->bDriving||!Near(M->World->ServicePosition(),4.8)){Change(TEXT("Move closer to your parked service jeep to load or unload its cargo."));return true;}
 if(Payload==2){Payload=3;bParcelWasLoaded=true;Change(TEXT("Parcel secured in the service-truck cargo rack. It travels with the truck."));}
 else if(Payload==3){Payload=2;Change(TEXT("GARAGE RECEIVING: Parcel unloaded. Carry supplies to the receiving table to add them to stock. Return loan equipment at CALIBRATION / REPAIR."));}
 else
 {
  TArray<S47Kit::FW> R;R.Add(S47Kit::Paper(FString::Printf(TEXT("SERVICE TRUCK\nFUEL %.1f / 4\nCARGO: %s"),Fuel,Payload==3?*ParcelDescription():TEXT("empty"))));
  R.Add(S47Kit::Button(TEXT("Refuel from a stock can"),[this]{RefuelTruck();M->UI->Close();},true,InventoryCount(TEXT("fuel"))>0&&Fuel<3.99));
  R.Add(S47Kit::Button(TEXT("Request roadside return to SARO"),[this]{RoadsideAssist();M->UI->Close();},false,Fuel<=.001));
  M->UI->Open(S47Kit::Shell(M->UI,TEXT("SERVICE TRUCK / CARGO AND FUEL"),S47Kit::Col(R),800));
 }
 return true;
}
bool FS47FreeformWorld::Deposit(const FString& Station)
{
 const FVector At=Station==TEXT("ff:delivery")?DeliveryAt:Station==TEXT("ff:depot")?DepotAt:BenchAt;
 if(!Near(At,2.6))return true;
 if(Payload==2&&Item==TEXT("fieldlogger")&&Station!=TEXT("ff:depot"))
 {Change(TEXT("EQUIPMENT CHECKOUT: Return the loan logger and its original tape in PARTS STORAGE, then compare at CALIBRATION / REPAIR."));return true;}
 if(Payload==2&&Item==TEXT("service_case")&&Station!=TEXT("ff:bench"))
 {Change(TEXT("CALIBRATION / REPAIR: Return the service case at the bench."));return true;}
 if(Payload==2&&Station!=TEXT("ff:delivery")&&Item!=TEXT("fieldlogger")&&Item!=TEXT("service_case"))
 {Change(TEXT("GARAGE RECEIVING: Carry purchased supplies to the labelled receiving table to unpack them."));return true;}
 if(Payload==2&&Item==TEXT("fieldlogger")){Payload=0;Item.Empty();bParcelWasLoaded=false;Change(TEXT("EQUIPMENT CHECKOUT: Loan logger and original tape returned intact. CALIBRATION / REPAIR: Compare retained originals and prepare another deployment."));return true;}
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
  Change(TEXT("Delivered to SARO stock: ")+Description+TEXT(". The labelled parcel is unpacked once."));return true;
 }
 if(Station==TEXT("ff:depot"))OpenDepot();
 else if(Station==TEXT("ff:bench"))OpenWorkbench();
 else Change(TEXT("GARAGE RECEIVING: Unload the parked Jeep, then carry the parcel to this table. Stored parts are in PARTS STORAGE."));
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
 BenchJob=TEXT("donor");BenchSeconds=0;M->UI->Close();Change(TEXT("Donor controller connected to the test bench. The relay test takes three seconds."));
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
 TArray<S47Kit::FW> R;R.Add(S47Kit::Paper(FString::Printf(TEXT("SARO / EAST WING / CALIBRATION BENCH\nFUSES %d / RELAYS %d / DONORS %d / FUEL CANS %d / FILM %d\nGENERATOR %.1f h / TRUCK %.1f / 4\n%s"),InventoryCount(TEXT("fuse")),InventoryCount(TEXT("relay")),InventoryCount(TEXT("donor")),InventoryCount(TEXT("fuel")),InventoryCount(TEXT("film")),GeneratorHours(),Fuel,BenchJob.IsEmpty()?TEXT("Bench ready"):BenchJob==TEXT("fragment")?TEXT("Field sample comparison running"):TEXT("Controller relay test running"))));
 R.Add(S47Kit::Small(LoggerAccess()?TEXT("FIELD LOGGER AUTHORIZED / loan kit, two field sites, six-hour battery. Measures local electrical noise only."):TEXT("FIELD LOGGER LOCKED / complete a supported conclusion in The repeated interval.")));
 if(LoggerAccess())
 {
  R.Add(S47Kit::Paper(FString::Printf(TEXT("fieldlogger01 / connections %d/3 / battery %.1f h / %.3f MHz / calibration %s\nSITE A: generator service path (8, 5). SITE B: east array edge (34, -28).\nTwo-hour recording starts on placement. Return to SARO and acquire a matching-frequency original while it runs; return the logger at EQUIPMENT CHECKOUT in PARTS STORAGE, then compare its retained original here."),Logger.Assembly,Logger.Battery/3600.,Logger.Frequency,Logger.bCalibrated?TEXT("+2.4 s offset retained"):TEXT("required"))));
  const TCHAR* Connections[]={TEXT("Seat loan receiver in case"),TEXT("Connect shielded antenna lead"),TEXT("Connect charged loan battery")};
  if(Logger.Assembly<3)R.Add(S47Kit::Button(Connections[Logger.Assembly],[this]{LoggerAction(TEXT("connect"));},true));
  R.Add(S47Kit::Button(TEXT("Calibrate against bench clock / recharge battery"),[this]{LoggerAction(TEXT("calibrate"));},false,Logger.Assembly==3&&Logger.Site<0&&Payload==0&&!bLoggerCalibrating));
  R.Add(S47Kit::Button(TEXT("Tune logger to selected SARO work card"),[this]{LoggerAction(TEXT("tune"));},false,Logger.Site<0&&Payload==0));
  R.Add(S47Kit::Small(TEXT("EQUIPMENT CHECKOUT: Collect and return the prepared logger in PARTS STORAGE. Calibration, tuning and original comparison remain at this bench.")));
  R.Add(S47Kit::Button(TEXT("Read and compare retained field originals"),[this]{LoggerAction(TEXT("compare"));},false,Payload==0));
 }
 R.Add(S47Kit::Button(TEXT("Take reusable SARO service case"),[this]
 {
  if(!Ready()||!Near(BenchAt)||Payload!=0)return;Item=TEXT("service_case");Contents.Reset();Payload=2;bParcelWasLoaded=false;if(Carry)Carry->Restore(nullptr,BenchAt+FVector(0,1.05,0));M->UI->Close();Change(TEXT("SARO service case in hand. Set it down, carry it in the truck, or return it to this bench. It imposes no repair requirement."));
 },false,Payload==0));
 R.Add(S47Kit::Button(TEXT("Test donor controller and harvest compatible relay"),[this]{DonorTest();},true,InventoryCount(TEXT("donor"))>0&&BenchJob.IsEmpty()));
 R.Add(S47Kit::Small(TEXT("TEST LAB: Analyze field fragments. PARTS STORAGE: Inspect spare stock and load film. GARAGE RECEIVING: Unpack carried supplies.")));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / CALIBRATION AND REPAIR"),S47Kit::Col(R),880));
}
void FS47FreeformWorld::OpenSampleLab()
{
 if(!Ready()||!Near(SampleAt,2.6)||!M->UI)return;
 TArray<S47Kit::FW> R;
 R.Add(S47Kit::Paper(FString::Printf(TEXT("GENERAL TEST LAB\nFIELD FRAGMENTS %d / SEALED ARCHIVED ORIGINALS %d\n%s"),InventoryCount(TEXT("fragment")),InventoryCount(TEXT("archived_fragment")),BenchJob==TEXT("fragment")?TEXT("REFERENCE COMPARISON RUNNING"):TEXT("Originals retain their field provenance; analysis does not identify an origin."))));
 R.Add(S47Kit::Button(TEXT("Analyze field fragment and retain sealed original"),[this]{AnalyzeSample();},true,InventoryCount(TEXT("fragment"))>0&&BenchJob.IsEmpty()&&!M->Freeform->HasFinding(TEXT("sample-analysis"))));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / TEST LAB"),S47Kit::Col(R),880));
}
void FS47FreeformWorld::OpenDepot()
{
 if(!Ready()||!Near(DepotAt,2.6)||!M->UI)return;
 TArray<S47Kit::FW> R;
 R.Add(S47Kit::Paper(FString::Printf(TEXT("PARTS STORAGE\nFUSES %d / RELAYS %d / DONORS %d / FUEL CANS %d / FILM %d"),InventoryCount(TEXT("fuse")),InventoryCount(TEXT("relay")),InventoryCount(TEXT("donor")),InventoryCount(TEXT("fuel")),InventoryCount(TEXT("film")))));
 R.Add(S47Kit::Small(LoggerAccess()?TEXT("EQUIPMENT CHECKOUT / fieldlogger01: Calibrate and tune at CALIBRATION / REPAIR, then collect the same loan logger here. Return it here with its original tape before bench comparison."):TEXT("EQUIPMENT CHECKOUT LOCKED: Complete a supported conclusion in The repeated interval.")));
 if(LoggerAccess())R.Add(S47Kit::Button(TEXT("Check out calibrated field logger"),[this]{LoggerAction(TEXT("take"));},false,Logger.bCalibrated&&Logger.Site<0&&Payload==0&&!bLoggerCalibrating));
 R.Add(S47Kit::Small(TEXT("Small spares travel in your service pouch. GARAGE RECEIVING: Unpack purchased parcels. CALIBRATION / REPAIR: Test donors and prepare the loan field logger.")));
 R.Add(S47Kit::Button(TEXT("Load stocked film pack / 36 exposures"),[this]
 {
  if(!Ready()||!Near(DepotAt,2.6)||InventoryCount(TEXT("film"))<=0||!M->Night||!M->Night->Album)return;
  if(M->Night->Album->AddFilm(FS47Album::ROLL)){Consume(TEXT("film"));M->UI->Close();Change(TEXT("PARTS STORAGE: Fresh film loaded. Earlier photographs remain in the case album."));}
 },false,InventoryCount(TEXT("film"))>0));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / PARTS STORAGE"),S47Kit::Col(R),880));
}
void FS47FreeformWorld::RefuelTruck(){if(Ready()&&Near(TruckPoint(),3.3)&&Fuel<3.99&&Consume(TEXT("fuel"))){Fuel=FMath::Min(4.,Fuel+2.);Change(TEXT("Fuel can transferred to the service truck."));}}
void FS47FreeformWorld::RoadsideAssist()
{
 if(!Ready()||!M->World||Fuel>.001)return;
 if(M->World->bDriving)M->World->GetOutHere(true);
 M->World->Enter(TEXT("saro"));M->World->PlaceTruck(TEXT("saro"));M->Player->Place(16,9,0);
 // Two reserve units cover 24 km at the normal 1/12000-m consumption. SARO
 // and Caliche use separate scene origins; their 8-km rendering offset is
 // not a driven segment. This still leaves ample margin for the full return
 // supply journey and a feeder detour before stock fuel can be unpacked.
 Fuel=2.;M->Freeform->SpendTime(15*60);Change(TEXT("County roadside service returned truck and cargo to SARO with a 24-km emergency fuel reserve. Collect supplies in Caliche and refuel before the next long trip."));
 M->Freeform->RecordEvidence(TEXT("roadside"),TEXT("Roadside return used; the interrupted trip and cargo were retained."));
}

bool FS47FreeformWorld::TriggerServiceFault()
{
 if(!Ready()||HasArrayFault()||M->Freeform->PowerFault())return false;
 Service=1;Settle=0;LastFaultDay=M->Freeform->Day();if(S03()>=0)M->Web->SetDishRemoteAvailable(S03(),false);
 Change(TEXT("S-03 remote link failed its health poll. Inspect the S-03 CABLE JUNCTION cabinet beside the antenna, or select S-03 on the status PC. Other antennas remain available."));return true;
}
void FS47FreeformWorld::DiagnoseService(){if(Ready()&&Service==1){Service=2;Change(TEXT("S-03 command relay failed. At the S-03 CABLE JUNCTION cabinet: select LOCAL, open the hatch and replace one drive relay. Stock parts are carried in your service pouch; no upgrade is needed."));}}
FString FS47FreeformWorld::ServiceStatus() const
{
 const int32 I=S03();
 const bool Powered=M&&M->Freeform&&(!M->Freeform->PowerFault()||HasBackupPower());
 const bool Moving=I>=0&&(FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),M->Web->DishTargetAz(I)))>.1||FMath::Abs(M->Web->DishEl(I)-M->Web->DishTargetEl(I))>.1);
 const FString State=!Powered?TEXT("NO POWER / movement held"):I<0?TEXT("ENCODER UNAVAILABLE"):Service==1||Service==2?TEXT("FAULT / command relay failed"):Service>=3&&Service<=6?TEXT("LOCAL / remote isolated"):Service==7||Service==8?TEXT("LOCAL TEST / moving and settling"):Moving?TEXT("REMOTE / moving"):TEXT("REMOTE / ready");
 const FString Next=Service==0||Service==9?TEXT("Select a source and command tracking on the tracking PC."):Service==7||Service==8?Powered?TEXT("Watch the antenna turn out and return; REMOTE follows a passed test."):TEXT("Restore feeder power or start the fuelled backup generator; the test resumes."):Label(TEXT("ff:service"));
 const FString Encoder=I>=0?FString::Printf(TEXT("ENCODER %.1f / %.1f | TARGET %.1f / %.1f | %s"),M->Web->DishAz(I),M->Web->DishEl(I),M->Web->DishTargetAz(I),M->Web->DishTargetEl(I),M->Web->DishRemoteAvailable(I)?TEXT("REMOTE ENABLED"):TEXT("REMOTE DISABLED")):TEXT("ENCODER UNAVAILABLE");
 return FString::Printf(TEXT("S-03: %s\n%s\nLOCAL SERVICE: existing S-03 CABLE JUNCTION cabinet at ground level.\nDRIVE RELAYS IN POUCH: %d / required: %d\nNEXT: %s\n%s"),*State,*Encoder,InventoryCount(TEXT("relay")),HasArrayFault()&&Service<5?1:0,*Next,Service>=5?TEXT("Replacement already fitted. No further relay is consumed for closing, testing or Continue."):InventoryCount(TEXT("relay"))>0?TEXT("Stock relays can be fitted directly; no upgrade or service case is required."):TEXT("CALICHE GAS STATION: Buy a drive relay. GARAGE RECEIVING: Unload and unpack its parcel. CALIBRATION / REPAIR: Test donor controllers before use."));
}
bool FS47FreeformWorld::ServiceUse()
{
 if(!Near(InteractionPoint(TEXT("ff:service")),2.6)){Change(TEXT("Approach the S-03 CABLE JUNCTION cabinet beside the antenna. Use the hatch from ground level."));return true;}
 switch(Service)
 {
  case 0:case 9:Change(ServiceStatus());return true;
  case 1:DiagnoseService();return true;
  case 2:Service=3;Change(TEXT("S-03 set to LOCAL. Remote movement is isolated. Open the cover."));break;
  case 3:Service=4;Change(TEXT("Hatch open. The command relay is not closing; seat a compatible replacement from your stock."));break;
  case 4:if(!Consume(TEXT("relay"))){Change(TEXT("CALICHE GAS STATION: Buy a relay; your pouch is empty. GARAGE RECEIVING: Unpack its parcel. CALIBRATION / REPAIR: Test a donor before using its relay."));return true;}Service=5;Change(TEXT("Replacement relay seated. Close the cover before the movement test."));break;
  case 5:Service=6;Change(TEXT("Cover secured. Use the local control to test five degrees out and back."));break;
  case 6:if(M->Freeform->PowerFault()&&!HasBackupPower()){Change(TEXT("NO POWER: restore feeder supply or start the fuelled backup generator before the local test. The fitted relay is retained."));return true;}if(S03()<0){Change(TEXT("S-03 encoder unavailable. Test cannot start; the fitted relay is retained."));return true;}ServiceAz=M->Web->DishAz(S03());ServiceEl=M->Web->DishEl(S03());Service=7;Settle=0;M->Web->PointDish(S03(),ServiceAz+5,ServiceEl,2.2,true);Change(TEXT("Local test running: watch S-03 turn five degrees and return."));break;
  case 7:case 8:Change(ServiceStatus());break;
 }
 return true;
}
bool FS47FreeformWorld::GeneratorUse()
{
 if(!Near(GeneratorAt,2.9))return true;
 if(bGenerator){bGenerator=false;Change(TEXT("Backup generator stopped. Remaining tank fuel is retained."));return true;}
 if(GeneratorSeconds<=0){if(!Consume(TEXT("fuel"))){Change(TEXT("Generator tank empty. Bring a fuel can from the depot or Caliche."));return true;}GeneratorSeconds=6*3600.;}
 bGenerator=true;Change(TEXT("Backup generator running. Its local tank has six game-hours per full can."));return true;
}
bool FS47FreeformWorld::FeederUse()
{
 if(!Near(InteractionPoint(TEXT("ff:feeder")),3.0))return true;
 if(!M->Freeform->PowerFault()){Change(TEXT("SARO feeder voltage and local fuse are normal."));return true;}
 if(Feeder<=1){Feeder=2;Change(TEXT("Low supply confirmed at the feeder front. Open the enclosed isolation switch."));}
 else if(Feeder==2){Feeder=3;Change(TEXT("Feeder isolated. Replace the blown control fuse from your service stock."));}
 else if(Feeder==3){if(!Consume(TEXT("fuse"))){Change(TEXT("A control fuse is needed. Caliche's supply hatch offers a $5 feeder-repair fuse on credit even with existing debt; collect its parcel and unpack it at SARO GARAGE RECEIVING."));return true;}Feeder=4;Change(TEXT("Replacement fuse seated. Run the enclosed three-second test."));}
 else if(Feeder==4){if(FeederSeconds<=0){FeederSeconds=.001;Change(TEXT("Feeder test running. Watch the marked indicator settle before supply is restored."));}}
 return true;
}

bool FS47FreeformWorld::Use(const FString& Id)
{
 if(!Ready())return false;
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
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service")||Id==TEXT("field:S-03"))return ServiceUse();
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return GeneratorUse();
 if(Id==TEXT("ff:feeder")||Id==TEXT("substation:service")||Id==TEXT("substation:lamp")||Id==TEXT("substation:journal"))return FeederUse();
 return false;
}
FString FS47FreeformWorld::Label(const FString& Id) const
{
 if(!Ready())return FString();
 if(Id==TEXT("ff:logger-device"))return Logger.Site>=0?TEXT("Retrieve field logger and original tape"):FString();
 if(Id==TEXT("ff:logger-a")||Id==TEXT("ff:logger-b"))return !LoggerAccess()?FString():FString(Id==TEXT("ff:logger-a")?TEXT("FIELD SITE A / generator path / "):TEXT("FIELD SITE B / array east / "))+(Logger.Site==(Id==TEXT("ff:logger-a")?0:1)?TEXT("retrieve logger and original tape"):TEXT("place carried logger and start two-hour record"));
 if(Id==TEXT("ff:pump")||Id==TEXT("town:pumps"))
 {
  if(AtDiesel())return TEXT("DIESEL / service truck needs the red REGULAR pump");
  if(bTripoPump)return bPumping?TEXT("Stop pump"):TEXT("Use pump");
  if(bPumping)return FString::Printf(TEXT("Stop fuel / tank %.2f / 4 / delivered %.2f / $%d"),Fuel,PumpVolume,PumpPaid);
  const double Volume=FMath::Min(1.,4.-Fuel);return Volume<.00001?TEXT("REGULAR / service truck tank full"):FString::Printf(TEXT("REGULAR / add up to %.2f / $%d / tank %.2f of 4"),Volume,FMath::CeilToInt(Volume/.25-.00001),Fuel);
 }
 if(Id==TEXT("ff:carry"))return Payload==2?TEXT("Handle carried parcel / set down or drop"):FString();
 if(Id==TEXT("ff:shop"))return ShopOpen()?TEXT("Buy service supplies / collect a physical parcel"):TEXT("Supply hatch closed / essential requisition");
 if(Id==TEXT("ff:bench"))return Payload==2&&Item==TEXT("service_case")?TEXT("Return service case to calibration bench"):TEXT("CALIBRATION / REPAIR: Test donors and prepare field logger");
 if(Id==TEXT("ff:sample"))return TEXT("TEST LAB: Analyze field sample and inspect sealed originals");
 if(Id==TEXT("ff:depot"))return Payload==2&&Item==TEXT("fieldlogger")?TEXT("EQUIPMENT CHECKOUT: Return logger and original tape"):TEXT("PARTS STORAGE: Equipment checkout, spare stock and film");
 if(Id==TEXT("ff:delivery"))return Payload==2?TEXT("GARAGE RECEIVING: Unpack carried supplies into SARO stock"):TEXT("GARAGE RECEIVING: Delivery instructions");
 if(Id==TEXT("ff:parcel"))return Payload>0&&Payload!=2?FString(TEXT("Carry "))+ParcelDescription():FString();
 if(Id==TEXT("ff:cargo"))return Payload==2?TEXT("Secure carried parcel in truck"):Payload==3?TEXT("Unload parcel from truck"):TEXT("Truck fuel / cargo rack");
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return bGenerator?TEXT("Stop backup generator"):TEXT("Start / fuel backup generator");
 if(Id==TEXT("ff:feeder")||Id==TEXT("substation:service")||Id==TEXT("substation:lamp")||Id==TEXT("substation:journal"))return M->Freeform->PowerFault()?Feeder==4&&FeederSeconds>0?TEXT("Feeder test running"):Feeder==4?TEXT("Run feeder replacement-fuse test"):Feeder==3?TEXT("Replace isolated feeder control fuse"):Feeder==2?TEXT("Open enclosed feeder isolation switch"):TEXT("Measure SARO feeder supply"):TEXT("Check normal feeder supply");
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service")||Id==TEXT("field:S-03"))
 {
  if(Service>=6&&Service<=8&&M->Freeform->PowerFault()&&!HasBackupPower())return TEXT("S-03 NO POWER / restore feeder or generator");
  if(Service==4&&InventoryCount(TEXT("relay"))<=0)return TEXT("GARAGE RECEIVING: Unpack a drive relay for S-03");
  static const TCHAR* Labels[]={TEXT("Inspect healthy S-03 control"),TEXT("Diagnose S-03 failed relay / no kit required"),TEXT("Select S-03 LOCAL service"),TEXT("Open S-03 relay hatch"),TEXT("Seat compatible replacement relay"),TEXT("Close S-03 relay hatch"),TEXT("Run S-03 local movement test"),TEXT("Local test: outward movement"),TEXT("Local test: return movement"),TEXT("S-03 verified / REMOTE restored")};return Labels[FMath::Clamp(Service,0,9)];
 }
 return FString();
}

void FS47FreeformWorld::Step(double Dt)
{
 if(!Ready())return;if(!bBuilt)Build();
 TrackPowerFault(M->Freeform->PowerFault());
 if(Carry)Carry->Step(Dt,Payload,Item,Payload==1?ParcelShopPoint():Payload==3?TruckPoint():Dropped);
 const double Now=M->Freeform->Clock();if(LastClock<0)LastClock=Now;
 const double WorldDelta=FMath::Max(0.,Now-LastClock);LastClock=Now;
 if(M->Freeform->Held()){Apply();return;}
 StepLogger(Dt,WorldDelta);
 StepPump(Dt);
 if(bGenerator){GeneratorSeconds=FMath::Max(0.,GeneratorSeconds-WorldDelta);if(GeneratorSeconds<=0){bGenerator=false;Change(TEXT("Backup generator tank empty. Array work needs restored mains or another fuel can."));}}
 if(M->World&&M->World->bDriving)
 {
  Fuel=FMath::Max(0.,Fuel-FMath::Abs(M->World->DrivingSpeed())*FMath::Min(Dt,.2)/12000.);
  if(Fuel<=.001){M->World->HoldTruck(25.);bFuelHeld=true;}
  else if(bFuelHeld){M->World->HoldTruck(0);bFuelHeld=false;}
 }
 if((Service==7||Service==8)&&(!M->Freeform->PowerFault()||HasBackupPower()))
 {
  const int32 I=S03();if(I>=0){const double Target=ServiceAz+(Service==7?5.:0.);
   const bool On=FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),Target))<=.1&&FMath::Abs(M->Web->DishEl(I)-ServiceEl)<=.1;
   Settle=On?Settle+FMath::Min(Dt,.1):0;
   if(Settle>=1){Settle=0;if(Service==7){Service=8;M->Web->PointDish(I,ServiceAz,ServiceEl,2.2,true);}else{Service=9;ServiceTests=FMath::Min(1000000,ServiceTests+1);M->Web->SetDishRemoteAvailable(I,true);M->Freeform->RecordEvidence(TEXT("service-s03"),TEXT("S-03 replacement relay passed local five-degree out-and-back test. Remote pointing restored."));Change(TEXT("S-03 returned to its starting angle. Local test passed; REMOTE restored."));}}
  }
 }
 if(!BenchJob.IsEmpty())
 {
  BenchSeconds+=FMath::Min(Dt,.1);
  if(BenchSeconds>=3)
  {
   const FString Finished=BenchJob;BenchJob.Empty();BenchSeconds=0;
   if(Finished==TEXT("fragment"))
   {
    AddItem(TEXT("archived_fragment"));
    M->Freeform->RecordEvidence(TEXT("sample-analysis"),TEXT("SARO test-lab comparison of the retained field fragment. Provenance: original landing_trace field record and its photograph, where available, remain unchanged in the case archive. The fragment conducts differently from the stock relay/contact reference; this measurement alone does not identify its origin. Original sample sealed and retained in the test-lab archive."));
    Change(TEXT("TEST LAB: Comparison complete. Measurement retained; original fragment sealed in the archive. Origin remains unassigned."));
   }
   else
   {
    DonorTests=FMath::Min(1000000,DonorTests+1);AddItem(TEXT("relay"));M->Freeform->RecordEvidence(TEXT("donor-relay"),TEXT("Donor controller relay tested at SARO bench and harvested as a compatible spare."));
    Change(TEXT("Bench relay test passed. One compatible relay added to stock; donor casing retained for scrap."));
   }
  }
 }
 if(M->Freeform->PowerFault()&&(Feeder==0||Feeder==5))Feeder=1;
 if(!M->Freeform->PowerFault()&&Feeder>0&&Feeder<5){Feeder=0;FeederSeconds=0;}
 if(Feeder==4&&FeederSeconds>0){FeederSeconds+=FMath::Min(Dt,.1);if(FeederSeconds>=3){Feeder=5;FeederSeconds=0;M->Freeform->SetPowerFault(false);TrackPowerFault(false);M->Freeform->RecordEvidence(TEXT("feeder-repair"),TEXT("Feeder supply measured, isolated, control fuse replaced and enclosed test passed."));Change(TEXT("Feeder indicator steady. SARO mains supply restored."));}}
 // A bounded ordinary service need, not a constant global wear drain.
 if(M->Freeform->Day()>1&&M->Freeform->Day()%(M->Freeform->HasUpgrade(0)?6:3)==2&&LastFaultDay!=M->Freeform->Day()&&DayClock(Now)>=10*3600&&!HasArrayFault()&&!M->Freeform->PowerFault())TriggerServiceFault();
 Apply();
}

bool FS47FreeformWorld::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto C=[&](bool B,const TCHAR* T){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+T);};
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
 C(!RescueCopy.RequestEmergencyFuse()&&RescueCopy.EmergencyFuseFault==0&&RescueCopy.ParcelStage()==0,TEXT("A rescue request still requires the actual player at the supply hatch"));
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
 OK &= FS47FieldLogger::SelfTest(Lines);
 OK &= FS47FreeformCarry::SelfTest(Lines);return OK;
}

bool FS47FreeformWorld::LoggerAccess()const
{
 const auto* Session=M?M->Freeform.Get():nullptr;
 return Session&&Session->CaseFile().HasAccess(TEXT("access.fieldlogger"));
}
FVector FS47FreeformWorld::LoggerPoint(int32 Site)const
{
 // Both posts use the existing SARO grounds, separate from the service cabinet.
 FVector At=Site==0?FVector(8,0,5):FVector(34,0,-28);
 At.Y=US47Night::SaroFloorAt(M?M->Web.Get():nullptr,At.X,At.Z)+.8;return At;
}
bool FS47FreeformWorld::LoggerAction(const FString& Action)
{
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
   Change(TEXT("EQUIPMENT CHECKOUT: Return the logger and sealed original tape in PARTS STORAGE. CALIBRATION / REPAIR: Compare the retained original afterward."));return true;
  }
  if(Payload!=2||Item!=TEXT("fieldlogger")){Change(TEXT("Bring the calibrated loan logger here in your hands. Prepare it at CALIBRATION / REPAIR, then collect it at EQUIPMENT CHECKOUT in PARTS STORAGE."));return false;}
  if(!Logger.Start(Site,M->Freeform->Day(),M->Freeform->Clock())){Change(TEXT("Logger cannot start: check calibration, battery and retained-tape capacity at the bench."));return false;}
  Payload=0;Item.Empty();bParcelWasLoaded=false;LoggerJob.Empty();LoggerJobElapsed=0;
  Change(TEXT("Field logger recording for two hours. Its battery and clock run independently of the SARO receiver. Return to SARO for a simultaneous recording, then retrieve this original."));return true;
 }
 if(!(Action==TEXT("take")?Near(DepotAt,2.6):Near(BenchAt))||M->Web->Area()!=TEXT("saro")||Payload!=0||Logger.Site>=0)return false;
 if(Action==TEXT("connect"))
 {
  if(!BenchJob.IsEmpty()||!Logger.Connect())return false;
  Change(Logger.Assembly==3?TEXT("Battery connected: bench meter responds. Run the clock calibration before carrying the logger."):TEXT("Loan-kit connection seated. Continue with the next labelled connection."));
 }
 else if(Action==TEXT("calibrate"))
 {
  if(Logger.Assembly!=3||!BenchJob.IsEmpty())return false;
  bLoggerCalibrating=true;Logger.Calibration=0;Logger.bCalibrated=false;if(M->UI)M->UI->Close();
  Change(TEXT("Bench reference running for three seconds. Logger clock reads 2.4 seconds ahead; the correction will travel with every original."));
 }
 else if(Action==TEXT("tune"))
 {
  const auto* P=M->Freeform->ConsoleProfile();if(!P)return false;Logger.Frequency=P->F;
  Change(FString::Printf(TEXT("Logger tuned to %.3f MHz from the selected SARO work card. This setting is retained when you change the station receiver."),Logger.Frequency));
 }
 else if(Action==TEXT("take"))
 {
  if(!Logger.bCalibrated||bLoggerCalibrating||Logger.Battery<=0||Logger.Tapes.Num()>=256)return false;
  Payload=2;Item=TEXT("fieldlogger");Contents.Reset();bParcelWasLoaded=false;if(Carry)Carry->Restore(nullptr,DepotAt+FVector(0,1.05,0));if(M->UI)M->UI->Close();
  Change(TEXT("Loan logger carried. Site A is on the generator service path; site B is at the east array edge. Cargo transport and setting down work as for the service case."));return true;
 }
 else if(Action==TEXT("compare"))
 {
  if(!M->UI)return false;TArray<S47Kit::FW> Rows;
  for(auto& T:Logger.Tapes)if(T.bRetrieved)
  {
   const auto* J=M->Freeform->Jobs().FindByPredicate([&](const FS47FreeformSession::FJob& Job){return Job.Id==T.Job&&Job.Stage>=FS47FreeformSession::EJobStage::Recorded;});
   const bool Valid=J&&FS47FieldLogger::CompleteWindow(T);
   const FString Text=Logger.Report(T)+(Valid?TEXT("\nCOMPARISON: contemporaneous SARO original retained. Local noise and station signal remain separate measurements."):TEXT("\nCOMPARISON NOT ESTABLISHED: requires a full two-hour field window without gaps and at least five seconds of matching SARO acquisition. Early retrieval, sleep gaps, empty battery or different frequency cannot substitute."));
   Rows.Add(S47Kit::Paper(Text));
   if(!T.Levels.IsEmpty())Rows.Add(S47Kit::Button(TEXT("Listen to retained local noise / six-second compressed monitor"),[this,Levels=T.Levels]
   {
    if(!M||!M->Audio||!M->Audio->Ctx||!M->Audio->SfxBus)return;
    auto Ctx=M->Audio->Ctx;FS47AudioContext::FBatch Batch(*Ctx);auto Buffer=Ctx->CreateBuffer(1,48000,8000);float* Samples=Buffer->GetChannelData(0);FRandomStream Noise(4701986);
    for(int32 I=0;I<48000;++I){const double Envelope=Levels[FMath::Min(Levels.Num()-1,I*Levels.Num()/48000)];const double Fade=FMath::Min3(1.,I/160.,(48000-I)/160.);Samples[I]=float(Fade*(.18*Envelope*FMath::Sin(2*PI*60*I/8000.)+.025*Noise.FRandRange(-1.f,1.f)));}
    auto Source=Ctx->CreateBufferSource();Source->Buffer=Buffer;Source->Connect(M->Audio->SfxBus);Source->Start();
   }));
   if(!T.bCompared&&Valid){T.bCompared=true;M->Freeform->RecordEvidence(T.Id,Text);}
  }
  if(Rows.IsEmpty())Rows.Add(S47Kit::Paper(TEXT("No physically returned field original. Deploy the logger, let it record, retrieve it and return it at EQUIPMENT CHECKOUT in PARTS STORAGE, then compare here.")));
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
  {bLoggerCalibrating=false;Logger.Calibration=0;Logger.bCalibrated=false;Change(TEXT("Clock calibration interrupted. Stay at the powered, available SARO bench for the full reference test."));}
  else if(Logger.Calibrate(FMath::Min(Dt,.1)))
  {
   bLoggerCalibrating=false;Change(TEXT("Bench reference passed. Clock offset +2.4 s retained; loan battery charged for six hours. Tune from a work card, then carry to either field post."));
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
