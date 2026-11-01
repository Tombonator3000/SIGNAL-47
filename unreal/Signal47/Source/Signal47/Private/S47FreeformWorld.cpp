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
 const FVector BenchAt(24.0,-.57,6.2);
 const FVector GeneratorAt(5.93,1.35,.65);
 // West of S-03's four plinths; reachable from the graded ground, not its elevated controller.
 FVector ServiceAt() { return FVector(5.8,S47GroundCover::SaroLand(5.8,-42.)+1.25,-42.); }
 const TCHAR* Items[] = { TEXT("fuse"), TEXT("relay"), TEXT("donor"), TEXT("fuel"), TEXT("film") };
 int32 Price(const FString& Id) { return Id==TEXT("fuse")?5:Id==TEXT("relay")?25:Id==TEXT("donor")?15:(Id==TEXT("fuel")||Id==TEXT("film"))?10:0; }
 FString ItemTitle(const FString& Id) { return Id==TEXT("fuse")?TEXT("control fuse"):Id==TEXT("relay")?TEXT("drive relay"):Id==TEXT("donor")?TEXT("used controller / relay donor"):Id==TEXT("film")?TEXT("instant-film pack / 36 exposures"):Id==TEXT("service_case")?TEXT("reusable SARO service case"):TEXT("six-hour fuel can"); }
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
 Stop(); bActive=true;Service=Feeder=LastFaultDay=EmergencyDay=Payload=0;Item.Empty();BenchJob.Empty();
 bGenerator=false;GeneratorSeconds=BenchSeconds=FeederSeconds=Settle=ServiceAz=ServiceEl=0;Fuel=3;LastClock=-1;Dropped=FVector::ZeroVector;
 Delivered=DeliveredByTruck=ServiceDeliveries=DonorTests=ServiceTests=0;bParcelWasLoaded=false;
 bPumping=false;PumpRemaining=PumpElapsed=PumpVolume=0;PumpPaid=0;LastPumpDisplay.Empty();
 Stock.Reset();Stock.Add(TEXT("fuse"),3);Stock.Add(TEXT("relay"),1);Stock.Add(TEXT("fuel"),2);Stock.Add(TEXT("donor"),0);Stock.Add(TEXT("battery"),0);Stock.Add(TEXT("film"),0);Stock.Add(TEXT("fragment"),0);Stock.Add(TEXT("archived_fragment"),0);
 if(O && S47J::Int(O,TEXT("v"))==1)
 {
  Service=FMath::Clamp(S47J::Int(O,TEXT("service")),0,9);Feeder=FMath::Clamp(S47J::Int(O,TEXT("feeder")),0,5);
  LastFaultDay=FMath::Max(0,S47J::Int(O,TEXT("faultDay")));EmergencyDay=FMath::Max(0,S47J::Int(O,TEXT("emergencyDay")));
  Payload=FMath::Clamp(S47J::Int(O,TEXT("payload")),0,4);Item=S47J::Str(O,TEXT("item"));if(Price(Item)<=0&&Item!=TEXT("service_case")){Payload=0;Item.Empty();}
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
 Build();if(Carry)Carry->Restore(S47J::Obj(O,TEXT("carry")),Payload==4?Dropped:FVector::ZeroVector);OverrideExisting();
 if(M&&M->Web)for(auto& C:M->Web->CollidersMutable(TEXT("saro")))if(C.Id==TEXT("ff:bench")||C.Id==TEXT("ff:service"))C.bEnabled=true;
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
 {I->Label=MoveTemp(Entry.Value.Label);I->Use=MoveTemp(Entry.Value.Use);I->Examine=MoveTemp(Entry.Value.Examine);}
 Overrides.Reset();
 if(bFuelHeld&&Live(M->World.Get()))M->World->HoldTruck(0);bFuelHeld=false;
 AS47Web* Web=M->Web.Get();
 if(!Live(Web)||Web->IsActorBeingDestroyed())return;
 RestorePumpModel();
 for(auto& C:Web->CollidersMutable(TEXT("saro")))if(C.Id==TEXT("ff:bench")||C.Id==TEXT("ff:service"))C.bEnabled=false;
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
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("service"),Service);S47J::Set(O,TEXT("feeder"),Feeder);
 S47J::Set(O,TEXT("faultDay"),LastFaultDay);S47J::Set(O,TEXT("emergencyDay"),EmergencyDay);S47J::Set(O,TEXT("payload"),Payload);S47J::Set(O,TEXT("item"),Item);
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
 if(M&&M->Web){const FS47Obj P=M->Web->Obj(TEXT("world.town.proxies.gasDoor"));if(P)return P.WorldPosition()+FVector(1.0,-.65,0);}
 return FVector(7984.1,5.87,2067.2);
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
 if(Id==TEXT("ff:pump")||Id==TEXT("town:pumps"))return PumpPoint();
 if(Id==TEXT("ff:shop"))return ShopPoint();
 if(Id==TEXT("ff:bench")||Id==TEXT("ff:depot"))return BenchAt+FVector(0,.90,0);
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return GeneratorAt;
 if(Id==TEXT("ff:feeder"))return FS47Substation::Site(FVector(5.52,-9.04,1.30));
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service"))return ServiceAt()+FVector(-.07,0,0);
 if(Id==TEXT("ff:cargo"))return TruckPoint();
 if(Id==TEXT("ff:parcel"))return Payload==1?ShopPoint()+FVector(.55,-.25,.2):Payload==3?TruckPoint():Payload==4?ParcelPosition():M&&M->Player?FVector(M->Player->Pos.X,M->Player->FloorY+1.,M->Player->Pos.Z):FVector::ZeroVector;
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
 for(const TCHAR* Raw:{TEXT("substation:journal"),TEXT("substation:service"),TEXT("substation:lamp"),TEXT("obs:generator"),TEXT("town:pumps")})
 {
  const FString Id(Raw);if(Overrides.Contains(Id))continue;auto* I=M->Inter->Get(Id);if(!I)continue;
  FOverride O;O.Label=I->Label;O.Examine=I->Examine;O.Use=I->Use;Overrides.Add(Id,MoveTemp(O));
  I->Label=[this,Guard,Id]{return Guard.IsValid()?Label(Id):FString();};
  I->Examine=I->Label;I->Use=[this,Guard,Id]{if(Guard.IsValid())Use(Id);};
 }
}

bool FS47FreeformWorld::Build()
{
 if(bBuilt){if(M&&M->Web)InstallPumpModel(M->Web->Handle(TEXT("world.legs.road.area.group")));return true;}if(!M||!M->Web||!M->Web->IsLoaded()||!M->Inter)return false;
 const int32 Saro=M->Web->Handle(TEXT("ext.group")),Road=M->Web->Handle(TEXT("world.legs.road.area.group"));if(Saro<0||Road<0)return false;
 // Reuse the existing archive-table model, fitted to the accessible facade walk.
 // The sign wing has no authored portal, so its solid walls remain intact.
 Bench=Model(TEXT("FreeformServiceBench"),TEXT("/Game/ArchiveProps/SM_ArchiveProps_WorkTable.SM_ArchiveProps_WorkTable"),BenchAt,1.8,Saro);
 if(!Bench)Bench=Box(TEXT("FreeformServiceBenchTop"),BenchAt+FVector(0,.78,0),FVector(1.8,.12,.75),0x514638,Saro);
 Bind(Bench,TEXT("ff:bench"));
 M->Web->CollidersMutable(TEXT("saro")).Add({BenchAt.X-.92,BenchAt.X+.92,BenchAt.Z-.4,BenchAt.Z+.4,TEXT("ff:bench"),true});
 Depot=Box(TEXT("FreeformDepotBin"),BenchAt+FVector(.55,.95,0),FVector(.52,.24,.38),0x687260,Saro,TEXT("ff:depot"));
 BenchMeter=Box(TEXT("FreeformBenchMeter"),BenchAt+FVector(-.44,1.02,-.10),FVector(.42,.28,.27),0x555b4c,Saro,TEXT("ff:bench"));
 BenchLamp=Box(TEXT("FreeformBenchTestIndicator"),BenchAt+FVector(-.44,1.04,.05),FVector(.28,.11,.015),0x5e8b61,Saro);
 BenchSample=Box(TEXT("FreeformBenchSampleHolder"),BenchAt+FVector(.03,.90,.05),FVector(.15,.045,.12),0x9f9b89,Saro);
 M->Web->MarkUsable(BenchLamp.I,TEXT("ff:bench"));M->Web->MarkUsable(BenchSample.I,TEXT("ff:bench"));
 Shop=Box(TEXT("FreeformGasSupplyHatch"),ShopPoint(),FVector(.18,.50,.72),0x807146,Road,TEXT("ff:shop"));
 Box(TEXT("FreeformGasSupplySupport"),ShopPoint()+FVector(0,-.40,0),FVector(.12,.30,.45),0x534f43,Road);
 ParcelSaro=Box(TEXT("FreeformParcelSaro"),BenchAt+FVector(0,1.0,0),FVector(.46,.32,.38),0x9f8250,Saro);
 ParcelRoad=Box(TEXT("FreeformParcelRoad"),ShopPoint()+FVector(.55,-.25,.2),FVector(.46,.32,.38),0x9f8250,Road);
 // Both visible parcel representations share one authoritative transaction/state.
 Bind(ParcelSaro,TEXT("ff:parcel"));M->Web->MarkUsable(ParcelRoad.I,TEXT("ff:parcel"));
 CargoSaro=Box(TEXT("FreeformCargoSaro"),TruckPoint(),FVector(.62,.12,.52),0x383e37,Saro);
 CargoRoad=Box(TEXT("FreeformCargoRoad"),TruckPoint(),FVector(.62,.12,.52),0x383e37,Road);
 Bind(CargoSaro,TEXT("ff:cargo"),3.0);M->Web->MarkUsable(CargoRoad.I,TEXT("ff:cargo"));
 Generator=Box(TEXT("FreeformBackupSelector"),GeneratorAt,FVector(.20,.22,.10),0x776044,Saro,TEXT("ff:generator"));
 {
  const FVector At=ServiceAt();
  M->Web->CollidersMutable(TEXT("saro")).Add({At.X-.06,At.X+.29,At.Z-.32,At.Z+.32,TEXT("ff:service"),true});
  Box(TEXT("FreeformS03Stand"),At+FVector(.10,-.60,0),FVector(.14,1.30,.24),0x566056,Saro);
  Box(TEXT("FreeformS03Housing"),At+FVector(.15,0,0),FVector(.25,.44,.62),0x566056,Saro,TEXT("ff:service"));
  Panel=Box(TEXT("FreeformS03Panel"),At,FVector(.025,.34,.50),0x333e36,Saro,TEXT("ff:service"));
  Lid=Box(TEXT("FreeformS03Lid"),At+FVector(-.025,0,0),FVector(.025,.34,.50),0x637060,Saro);
  Relay=Box(TEXT("FreeformS03Relay"),At+FVector(-.045,0,0),FVector(.05,.09,.13),0xb19651,Saro);
  Selector=Box(TEXT("FreeformS03Selector"),At+FVector(-.05,-.11,-.18),FVector(.045,.09,.05),0xab4931,Saro);
  for(const FS47Obj& O:{Lid,Relay,Selector})M->Web->MarkUsable(O.I,TEXT("ff:service"));
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
 Box(TEXT("FreeformS03Sign"),ServiceAt()+FVector(.04,.55,0),FVector(.04,.55,1.05),0x26352b,Saro,TEXT("ff:service"));
 // Keep one interaction origin on the hatch, including when aiming at the sign.
 Bind(Panel,TEXT("ff:service"));
 LabelPart(TEXT("FreeformS03Text"),TEXT("S-03 / LOCAL SERVICE"),ServiceAt()+FVector(.01,.55,0),180,3.5,Saro);
 LabelPart(TEXT("FreeformGasLabel"),TEXT("SERVICE SUPPLIES\n08:00 - 20:00"),ShopPoint()+FVector(.10,0,0),0,3.1,Road);
 LabelPart(TEXT("FreeformBenchLabel"),TEXT("SARO SERVICE BAY\nTEST / PARTS / STOCK"),BenchAt+FVector(0,1.45,-.35),90,3.5,Saro);
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
 if(BenchSample)BenchSample.SetVisible(bActive&&(InventoryCount(TEXT("fragment"))>0||InventoryCount(TEXT("archived_fragment"))>0||BenchJob==TEXT("fragment")));
 if(BenchLamp){const bool Testing=!BenchJob.IsEmpty();BenchLamp.SetColor(S47Three::Hex(Testing?0xc8a046:0x5e8b61));BenchLamp.SetEmissive(S47Three::Hex(Testing?0xc8a046:0x5e8b61),Testing?.2f+float(BenchSeconds/3.)*.8f:.15f);}
 const bool Open=bActive&&Service>=4&&Service<=5;
 if(Lid){const FVector At=InteractionPoint(TEXT("ff:service"))+FVector(-.015,0,0);if(const auto* N=Lid.W->Node(Lid.I))if(auto* C=N->Comp.Get())C->SetWorldLocation(S47Three::ToUE(At+FVector(0,0,Open?.38:0)));Lid.SetVisible(bActive);}
 if(ServiceText.IsValid()){const FString Display=FString(TEXT("S-03 / LOCAL SERVICE\n"))+Label(TEXT("ff:service"));if(Display!=LastServiceDisplay){ServiceText->SetText(FText::FromString(Display));LastServiceDisplay=Display;}}
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
 if(Carry)Carry->Step(0,bActive?Payload:0,Item,Payload==1?ShopPoint()+FVector(.55,-.25,.2):Payload==3?TruckPoint():Dropped);
 for(auto Pair:TArray<TPair<FS47Obj,bool>>{{CargoSaro,!Road},{CargoRoad,Road}})if(Pair.Key)
 {
  const bool Show=bActive&&Pair.Value&&M&&M->World&&!M->World->bDriving;if(const auto* N=Pair.Key.W->Node(Pair.Key.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())){C->SetWorldLocation(S47Three::ToUE(TruckPoint()));C->SetCollisionEnabled(Show?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}Pair.Key.SetVisible(Show);
 }
}

bool FS47FreeformWorld::Purchase(const FString& Id,bool Emergency)
{
 if(!Ready()||!Near(ShopPoint())||Price(Id)<=0||Payload!=0)return false;
 if(!Emergency&&!ShopOpen()){Change(TEXT("The supply window opens 08:00–20:00. The marked emergency hatch handles essential requisitions."));return false;}
 if(Emergency&&((Id==TEXT("donor")||Id==TEXT("film"))||EmergencyDay==M->Freeform->Day())){Change(TEXT("One essential emergency parcel per day; donor equipment is sold during opening hours."));return false;}
 if(!M->Freeform->Spend(Price(Id),Emergency)){Change(TEXT("Not enough operating budget. An essential emergency requisition is available once per day."));return false;}
 if(Emergency)EmergencyDay=M->Freeform->Day();Item=Id;Payload=1;bParcelWasLoaded=false;Change(TEXT("Paid and labelled. Collect the physical parcel beside the supply hatch."));return true;
}
void FS47FreeformWorld::OpenShop()
{
 if(!Ready()||!M->UI||!Near(ShopPoint()))return;TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Paper(FString::Printf(TEXT("CALICHE SERVICE SUPPLIES / 08:00–20:00\nOPERATING BUDGET $%d\nOne physical parcel at a time. Carry it to the truck, load it, and unload at the SARO service bench."),M->Freeform->Budget())));
 for(const TCHAR* Id:Items){const FString Key(Id);Rows.Add(S47Kit::Button(FString::Printf(TEXT("%s / $%d"),*ItemTitle(Key),Price(Key)),[this,Key]{if(Purchase(Key)){M->UI->Close();}},false,Payload==0&&ShopOpen()));}
 Rows.Add(S47Kit::Small(TEXT("Essential after-hours/credit requisition: one per day, charged against future work. Donor controllers require bench testing before harvesting their relay.")));
 for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("fuel")}){const FString Key(Id);Rows.Add(S47Kit::Button(FString(TEXT("Emergency "))+ItemTitle(Key),[this,Key]{if(Purchase(Key,true))M->UI->Close();},false,Payload==0&&EmergencyDay!=M->Freeform->Day()));}
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("CALICHE / SUPPLY HATCH"),S47Kit::Col(Rows),880));
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
 TArray<S47Kit::FW> R;R.Add(S47Kit::Paper(ItemTitle(Item)+TEXT("\nHeld in both hands. The package remains part of the same paid delivery.")));
 R.Add(S47Kit::Button(TEXT("Set down on a clear surface"),[this]{M->UI->Close();DropParcel(false);},true));
 R.Add(S47Kit::Button(TEXT("Drop from your hands"),[this]{M->UI->Close();DropParcel(true);}));
 R.Add(S47Kit::Small(TEXT("A heavier package slows walking and trails a turn. Walls stop it. Loading the truck secures it for travel.")));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("CARRIED PARCEL"),S47Kit::Col(R),800));
}

bool FS47FreeformWorld::ParcelUse()
{
 if(!Payload||Payload==2)return true;if(!Near(InteractionPoint(TEXT("ff:parcel")),2.8))return true;
 Payload=2;Change(TEXT("Parcel carried. Load it at the truck or unpack at SARO. Use parcel handling to set it down or drop it safely."));return true;
}
bool FS47FreeformWorld::CargoUse()
{
 if(!M||!M->World||M->World->bDriving||!Near(M->World->ServicePosition(),4.8)){Change(TEXT("Move closer to your parked service jeep to load or unload its cargo."));return true;}
 if(Payload==2){Payload=3;bParcelWasLoaded=true;Change(TEXT("Parcel secured in the service-truck cargo rack. It travels with the truck."));}
 else if(Payload==3){Payload=2;Change(TEXT("Parcel unloaded. Carry it to the SARO service bench to add its contents to stock."));}
 else
 {
  TArray<S47Kit::FW> R;R.Add(S47Kit::Paper(FString::Printf(TEXT("SERVICE TRUCK\nFUEL %.1f / 4\nCARGO: %s"),Fuel,Payload==3?*ItemTitle(Item):TEXT("empty"))));
  R.Add(S47Kit::Button(TEXT("Refuel from a stock can"),[this]{RefuelTruck();M->UI->Close();},true,InventoryCount(TEXT("fuel"))>0&&Fuel<3.99));
  R.Add(S47Kit::Button(TEXT("Request roadside return to SARO"),[this]{RoadsideAssist();M->UI->Close();},false,Fuel<=.001));
  M->UI->Open(S47Kit::Shell(M->UI,TEXT("SERVICE TRUCK / CARGO AND FUEL"),S47Kit::Col(R),800));
 }
 return true;
}
bool FS47FreeformWorld::Deposit()
{
 if(!Near(BenchAt,3.0))return true;
 if(Payload==2&&Item==TEXT("service_case")){Payload=0;Item.Empty();bParcelWasLoaded=false;Change(TEXT("Reusable service case returned to the bench. Stock and budget are unchanged."));return true;}
 if(Payload==2){const FString Part=Item;Delivered=FMath::Min(1000000,Delivered+1);if(bParcelWasLoaded){DeliveredByTruck=FMath::Min(1000000,DeliveredByTruck+1);if(Part==TEXT("relay")||Part==TEXT("donor"))ServiceDeliveries=FMath::Min(1000000,ServiceDeliveries+1);}bParcelWasLoaded=false;Payload=0;Item.Empty();AddItem(Part,1);Change(FString(TEXT("Delivered to SARO stock: "))+ItemTitle(Part)+TEXT(". The labelled parcel is unpacked once."));return true;}
 OpenWorkbench();return true;
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
 if(!Ready()||!Near(BenchAt)||!BenchJob.IsEmpty()||M->Freeform->HasFinding(TEXT("sample-analysis"))||InventoryCount(TEXT("fragment"))<=0)return false;
 if(!Consume(TEXT("fragment")))return false;
 BenchJob=TEXT("fragment");BenchSeconds=0;M->UI->Close();Change(TEXT("Field fragment placed in the bench test holder. Reference and conductivity comparison running."));return true;
}
void FS47FreeformWorld::OpenWorkbench()
{
 if(!Ready()||!Near(BenchAt)||!M->UI)return;
 TArray<S47Kit::FW> R;R.Add(S47Kit::Paper(FString::Printf(TEXT("SARO / EXTERIOR SERVICE BAY\nFUSES %d / RELAYS %d / DONORS %d / FUEL CANS %d / FILM %d\nGENERATOR %.1f h / TRUCK %.1f / 4\n%s"),InventoryCount(TEXT("fuse")),InventoryCount(TEXT("relay")),InventoryCount(TEXT("donor")),InventoryCount(TEXT("fuel")),InventoryCount(TEXT("film")),GeneratorHours(),Fuel,BenchJob.IsEmpty()?TEXT("Bench ready"):BenchJob==TEXT("fragment")?TEXT("Field sample comparison running"):TEXT("Controller relay test running"))));
 R.Add(S47Kit::Button(TEXT("Take reusable SARO service case"),[this]
 {
  if(!Ready()||!Near(BenchAt)||Payload!=0)return;Item=TEXT("service_case");Payload=2;bParcelWasLoaded=false;if(Carry)Carry->Restore(nullptr,BenchAt+FVector(0,1.05,0));M->UI->Close();Change(TEXT("SARO service case in hand. Set it down, carry it in the truck, or return it to this bench. It imposes no repair requirement."));
 },false,Payload==0));
 R.Add(S47Kit::Button(TEXT("Test donor controller and harvest compatible relay"),[this]{DonorTest();},true,InventoryCount(TEXT("donor"))>0&&BenchJob.IsEmpty()));
 R.Add(S47Kit::Small(FString::Printf(TEXT("FIELD FRAGMENTS %d / SEALED ARCHIVED SAMPLES %d"),InventoryCount(TEXT("fragment")),InventoryCount(TEXT("archived_fragment")))));
 R.Add(S47Kit::Button(TEXT("Analyze field fragment and retain sealed original"),[this]{AnalyzeSample();},false,InventoryCount(TEXT("fragment"))>0&&BenchJob.IsEmpty()&&!M->Freeform->HasFinding(TEXT("sample-analysis"))));
 R.Add(S47Kit::Button(TEXT("Load stocked instant-film pack / 36 exposures"),[this]
 {
  if(!Ready()||!Near(BenchAt)||InventoryCount(TEXT("film"))<=0||!M->Night||!M->Night->Album)return;
  if(M->Night->Album->AddFilm(FS47Album::ROLL)){Consume(TEXT("film"));M->UI->Close();Change(TEXT("Fresh instant film loaded. Earlier photographs remain in the case album."));}
 },false,InventoryCount(TEXT("film"))>0));
 R.Add(S47Kit::Small(TEXT("The test bench, stock and transport are physical. A purchased parcel must be carried or unloaded here before its part can be used. Small stock parts fit in your service pouch.")));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("SARO / SERVICE BENCH AND DEPOT"),S47Kit::Col(R),880));
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
 Change(TEXT("S-03 remote link failed its health poll. Inspect the marked LOCAL SERVICE box at the west foot of S-03, or select S-03 on the status PC. Other antennas remain available."));return true;
}
void FS47FreeformWorld::DiagnoseService(){if(Ready()&&Service==1){Service=2;Change(TEXT("S-03 command relay failed. At the marked box on the west foot of S-03: select LOCAL, open the hatch and replace one drive relay. Stock parts are carried in your service pouch; no upgrade is needed."));}}
FString FS47FreeformWorld::ServiceStatus() const
{
 const int32 I=S03();
 const bool Powered=M&&M->Freeform&&(!M->Freeform->PowerFault()||HasBackupPower());
 const bool Moving=I>=0&&(FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),M->Web->DishTargetAz(I)))>.1||FMath::Abs(M->Web->DishEl(I)-M->Web->DishTargetEl(I))>.1);
 const FString State=!Powered?TEXT("NO POWER / movement held"):I<0?TEXT("ENCODER UNAVAILABLE"):Service==1||Service==2?TEXT("FAULT / command relay failed"):Service>=3&&Service<=6?TEXT("LOCAL / remote isolated"):Service==7||Service==8?TEXT("LOCAL TEST / moving and settling"):Moving?TEXT("REMOTE / moving"):TEXT("REMOTE / ready");
 const FString Next=Service==0||Service==9?TEXT("Select a source and command tracking on the tracking PC."):Service==7||Service==8?Powered?TEXT("Watch the antenna turn out and return; REMOTE follows a passed test."):TEXT("Restore feeder power or start the fuelled backup generator; the test resumes."):Label(TEXT("ff:service"));
 const FString Encoder=I>=0?FString::Printf(TEXT("ENCODER %.1f / %.1f | TARGET %.1f / %.1f | %s"),M->Web->DishAz(I),M->Web->DishEl(I),M->Web->DishTargetAz(I),M->Web->DishTargetEl(I),M->Web->DishRemoteAvailable(I)?TEXT("REMOTE ENABLED"):TEXT("REMOTE DISABLED")):TEXT("ENCODER UNAVAILABLE");
 return FString::Printf(TEXT("S-03: %s\n%s\nLOCAL SERVICE: marked box at the west foot of S-03, at ground level.\nDRIVE RELAYS IN POUCH: %d / required: %d\nNEXT: %s\n%s"),*State,*Encoder,InventoryCount(TEXT("relay")),HasArrayFault()&&Service<5?1:0,*Next,Service>=5?TEXT("Replacement already fitted. No further relay is consumed for closing, testing or Continue."):InventoryCount(TEXT("relay"))>0?TEXT("Stock relays can be fitted directly; no upgrade or service case is required."):TEXT("Buy a drive relay at Caliche supplies. Unload/carry the parcel to SARO SERVICE BAY by the sign building and unpack at the bench. Test donor controllers there before use."));
}
bool FS47FreeformWorld::ServiceUse()
{
 if(!Near(InteractionPoint(TEXT("ff:service")),2.6)){Change(TEXT("Approach the marked LOCAL SERVICE box at the west foot of S-03. Use the hatch from ground level."));return true;}
 switch(Service)
 {
  case 0:case 9:Change(ServiceStatus());return true;
  case 1:DiagnoseService();return true;
  case 2:Service=3;Change(TEXT("S-03 set to LOCAL. Remote movement is isolated. Open the cover."));break;
  case 3:Service=4;Change(TEXT("Hatch open. The command relay is not closing; seat a compatible replacement from your stock."));break;
  case 4:if(!Consume(TEXT("relay"))){Change(TEXT("No drive relay in your stock pouch. Buy a relay at Caliche service supplies; carry/unload its parcel at the SARO SERVICE BAY bench by the sign building and unpack it. A donor must also be tested there."));return true;}Service=5;Change(TEXT("Replacement relay seated. Close the cover before the movement test."));break;
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
 else if(Feeder==3){if(!Consume(TEXT("fuse"))){Change(TEXT("A control fuse is needed. Use the Caliche emergency supply hatch if necessary."));return true;}Feeder=4;Change(TEXT("Replacement fuse seated. Run the enclosed three-second test."));}
 else if(Feeder==4){if(FeederSeconds<=0){FeederSeconds=.001;Change(TEXT("Feeder test running. Watch the marked indicator settle before supply is restored."));}}
 return true;
}

bool FS47FreeformWorld::Use(const FString& Id)
{
 if(!Ready())return false;
 if(Id==TEXT("ff:pump")||Id==TEXT("town:pumps"))
 {
  if(AtDiesel()){Change(TEXT("DIESEL is incompatible with the SARO service truck. Use the red REGULAR pump on the same island."));return true;}
  if(bPumping)StopPump();else StartPump();return true;
 }
 if(Id==TEXT("ff:carry")){if(Payload!=2)return false;OpenCarryMenu();return true;}
 if(Id==TEXT("ff:shop")){OpenShop();return true;}
 if(Id==TEXT("ff:bench")||Id==TEXT("ff:depot"))return Deposit();
 if(Id==TEXT("ff:parcel"))return ParcelUse();
 if(Id==TEXT("ff:cargo"))return CargoUse();
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service"))return ServiceUse();
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return GeneratorUse();
 if(Id==TEXT("ff:feeder")||Id==TEXT("substation:service")||Id==TEXT("substation:lamp")||Id==TEXT("substation:journal"))return FeederUse();
 return false;
}
FString FS47FreeformWorld::Label(const FString& Id) const
{
 if(!Ready())return FString();
 if(Id==TEXT("ff:pump")||Id==TEXT("town:pumps"))
 {
  if(AtDiesel())return TEXT("DIESEL / service truck needs the red REGULAR pump");
  if(bTripoPump)return bPumping?TEXT("Stop pump"):TEXT("Use pump");
  if(bPumping)return FString::Printf(TEXT("Stop fuel / tank %.2f / 4 / delivered %.2f / $%d"),Fuel,PumpVolume,PumpPaid);
  const double Volume=FMath::Min(1.,4.-Fuel);return Volume<.00001?TEXT("REGULAR / service truck tank full"):FString::Printf(TEXT("REGULAR / add up to %.2f / $%d / tank %.2f of 4"),Volume,FMath::CeilToInt(Volume/.25-.00001),Fuel);
 }
 if(Id==TEXT("ff:carry"))return Payload==2?TEXT("Handle carried parcel / set down or drop"):FString();
 if(Id==TEXT("ff:shop"))return ShopOpen()?TEXT("Buy service supplies / collect a physical parcel"):TEXT("Supply hatch closed / essential requisition");
 if(Id==TEXT("ff:bench")||Id==TEXT("ff:depot"))return Payload==2?(Item==TEXT("service_case")?TEXT("Return reusable SARO service case"):TEXT("Unpack carried parcel into SARO stock")):TEXT("Use service bench and inspect stock");
 if(Id==TEXT("ff:parcel"))return Payload>0&&Payload!=2?FString(TEXT("Carry "))+ItemTitle(Item):FString();
 if(Id==TEXT("ff:cargo"))return Payload==2?TEXT("Secure carried parcel in truck"):Payload==3?TEXT("Unload parcel from truck"):TEXT("Truck fuel / cargo rack");
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return bGenerator?TEXT("Stop backup generator"):TEXT("Start / fuel backup generator");
 if(Id==TEXT("ff:feeder")||Id==TEXT("substation:service")||Id==TEXT("substation:lamp")||Id==TEXT("substation:journal"))return M->Freeform->PowerFault()?Feeder==4&&FeederSeconds>0?TEXT("Feeder test running"):Feeder==4?TEXT("Run feeder replacement-fuse test"):Feeder==3?TEXT("Replace isolated feeder control fuse"):Feeder==2?TEXT("Open enclosed feeder isolation switch"):TEXT("Measure SARO feeder supply"):TEXT("Check normal feeder supply");
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service"))
 {
  if(Service>=6&&Service<=8&&M->Freeform->PowerFault()&&!HasBackupPower())return TEXT("S-03 NO POWER / restore feeder or generator");
  if(Service==4&&InventoryCount(TEXT("relay"))<=0)return TEXT("S-03 needs drive relay / unpack at SARO bench");
  static const TCHAR* Labels[]={TEXT("Inspect healthy S-03 control"),TEXT("Diagnose S-03 failed relay / no kit required"),TEXT("Select S-03 LOCAL service"),TEXT("Open S-03 relay hatch"),TEXT("Seat compatible replacement relay"),TEXT("Close S-03 relay hatch"),TEXT("Run S-03 local movement test"),TEXT("Local test: outward movement"),TEXT("Local test: return movement"),TEXT("S-03 verified / REMOTE restored")};return Labels[FMath::Clamp(Service,0,9)];
 }
 return FString();
}

void FS47FreeformWorld::Step(double Dt)
{
 if(!Ready())return;if(!bBuilt)Build();
 if(Carry)Carry->Step(Dt,Payload,Item,Payload==1?ShopPoint()+FVector(.55,-.25,.2):Payload==3?TruckPoint():Dropped);
 const double Now=M->Freeform->Clock();if(LastClock<0)LastClock=Now;
 const double WorldDelta=FMath::Max(0.,Now-LastClock);LastClock=Now;
 if(M->Freeform->Held()){Apply();return;}
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
    M->Freeform->RecordEvidence(TEXT("sample-analysis"),TEXT("SARO bench comparison of the retained field fragment. Provenance: original landing_trace field record and its photograph, where available, remain unchanged in the case archive. The fragment conducts differently from the stock relay/contact reference; this measurement alone does not identify its origin. Original sample sealed and retained in the service-bay archive."));
    Change(TEXT("Sample comparison complete. Measurement retained; original fragment sealed in the archive. Origin remains unassigned."));
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
 if(Feeder==4&&FeederSeconds>0){FeederSeconds+=FMath::Min(Dt,.1);if(FeederSeconds>=3){Feeder=5;FeederSeconds=0;M->Freeform->SetPowerFault(false);M->Freeform->RecordEvidence(TEXT("feeder-repair"),TEXT("Feeder supply measured, isolated, control fuse replaced and enclosed test passed."));Change(TEXT("Feeder indicator steady. SARO mains supply restored."));}}
 // A bounded ordinary service need, not a constant global wear drain.
 if(M->Freeform->Day()>1&&M->Freeform->Day()%(M->Freeform->HasUpgrade(0)?6:3)==2&&LastFaultDay!=M->Freeform->Day()&&DayClock(Now)>=10*3600&&!HasArrayFault()&&!M->Freeform->PowerFault())TriggerServiceFault();
 Apply();
}

bool FS47FreeformWorld::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto C=[&](bool B,const TCHAR* T){OK&=B;Lines.Add(FString(B?TEXT("PASS "):TEXT("FAIL "))+T);};
 FS47FreeformWorld W(nullptr);W.Begin();C(W.InventoryCount(TEXT("fuse"))==3&&W.InventoryCount(TEXT("relay"))==1&&W.InventoryCount(TEXT("fuel"))==2,TEXT("Freeform depot starts with bounded essential spares"));
 W.AddItem(TEXT("relay"),200);C(W.InventoryCount(TEXT("relay"))==99,TEXT("Freeform stock is bounded"));for(int32 I=0;I<99;++I)W.Consume(TEXT("relay"));C(!W.Consume(TEXT("relay"))&&W.InventoryCount(TEXT("relay"))==0,TEXT("Freeform cannot consume nonexistent stock"));
 W.Item=TEXT("fuel");W.Payload=3;W.bParcelWasLoaded=true;W.Delivered=4;W.DeliveredByTruck=3;W.ServiceDeliveries=2;W.DonorTests=1;W.ServiceTests=2;W.Service=4;W.Fuel=.7;W.BenchJob=TEXT("donor");W.BenchSeconds=1.2;W.GeneratorSeconds=7200;W.bGenerator=true;
 const auto Save=S47J::Read(S47J::Write(W.Save()));FS47FreeformWorld R(nullptr);R.Begin(Save);
 C(R.DeliveredParcels()==4&&R.CargoDeliveries()==3&&R.ServiceSupplyDeliveries()==2&&R.DonorTestsCompleted()==1&&R.ServiceCompletions()==2&&R.bParcelWasLoaded,TEXT("Freeform delivery and service counters survive JSON without repeating actions"));
 C(R.Payload==3&&R.Item==TEXT("fuel")&&R.Service==4&&R.Fuel==.7,TEXT("Freeform cargo and opened service survive a serialized restore"));
 C(R.BenchJob==TEXT("donor")&&R.BenchSeconds==1.2&&R.GeneratorSeconds==7200&&R.bGenerator,TEXT("Freeform in-progress donor and generator preserve consumed inputs"));
 C(!R.Purchase(TEXT("relay"))&&R.Payload==3,TEXT("Freeform purchase requires actual world and cannot duplicate loaded parcel"));
 W.AddItem(TEXT("fragment"),1);W.AddItem(TEXT("archived_fragment"),1);W.BenchJob=TEXT("fragment");W.BenchSeconds=1.7;
 FS47FreeformWorld Sample(nullptr);Sample.Begin(S47J::Read(S47J::Write(W.Save())));
 C(Sample.InventoryCount(TEXT("fragment"))==1&&Sample.InventoryCount(TEXT("archived_fragment"))==1&&Sample.BenchJob==TEXT("fragment")&&Sample.BenchSeconds==1.7,TEXT("Freeform field fragments, sealed originals and unfinished analysis persist"));
 C(Price(TEXT("fragment"))==0&&Price(TEXT("archived_fragment"))==0,TEXT("Freeform field samples cannot be purchased or loaned as shop stock"));
 W.Item=TEXT("service_case");W.Payload=4;W.Dropped=FVector(24,.45,8);
 auto Legacy=W.Save();Legacy->RemoveField(TEXT("carry"));FS47FreeformWorld Case(nullptr);Case.Begin(S47J::Read(S47J::Write(Legacy)));
 C(Case.ParcelItem()==TEXT("service_case")&&Case.ParcelStage()==4&&Case.ParcelPosition().Equals(W.Dropped,.0001)&&Case.InventoryCount(TEXT("service_case"))==0,TEXT("Reusable case keeps one saved identity and old dropped saves retain their position without becoming stock"));
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
 C(FMath::IsNearlyEqual(W.InteractionPoint(TEXT("ff:service")).Z,-42.)&&FMath::Abs(W.InteractionPoint(TEXT("ff:service")).X-9.5)<4.&&FMath::IsNearlyEqual(W.InteractionPoint(TEXT("ff:service")).Y-S47GroundCover::SaroLand(5.8,-42.),1.25),TEXT("S-03 service anchor is beside the actual antenna and within standing reach above graded ground"));
 OK &= FS47FreeformCarry::SelfTest(Lines);return OK;
}
