#include "S47FreeformWorld.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47UI.h"
#include "S47Kit.h"
#include "S47Three.h"
#include "S47Substation.h"
#include "S47Audio.h"
#include "S47Night.h"
#include "S47Album.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
 const FVector BenchAt(21.6,-.57,6.2);
 const FVector GeneratorAt(5.93,1.35,.65);
 const FVector CabinetLocal(0,.62,.29);
 const TCHAR* Items[] = { TEXT("fuse"), TEXT("relay"), TEXT("donor"), TEXT("fuel"), TEXT("film") };
 int32 Price(const FString& Id) { return Id==TEXT("fuse")?5:Id==TEXT("relay")?25:Id==TEXT("donor")?15:(Id==TEXT("fuel")||Id==TEXT("film"))?10:0; }
 FString ItemTitle(const FString& Id) { return Id==TEXT("fuse")?TEXT("control fuse"):Id==TEXT("relay")?TEXT("drive relay"):Id==TEXT("donor")?TEXT("used controller / relay donor"):Id==TEXT("film")?TEXT("instant-film pack / 36 exposures"):TEXT("six-hour fuel can"); }
 bool ValidItem(const FString& Id) { return Price(Id)>0 || Id==TEXT("battery") || Id==TEXT("fragment") || Id==TEXT("archived_fragment"); }
 double DayClock(double T) { return FMath::Fmod(FMath::Fmod(T,86400.)+86400.,86400.); }
}

FS47FreeformWorld::FS47FreeformWorld(US47Main* InM) : M(InM) {}
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
 Stock.Reset();Stock.Add(TEXT("fuse"),3);Stock.Add(TEXT("relay"),1);Stock.Add(TEXT("fuel"),2);Stock.Add(TEXT("donor"),0);Stock.Add(TEXT("battery"),0);Stock.Add(TEXT("film"),0);Stock.Add(TEXT("fragment"),0);Stock.Add(TEXT("archived_fragment"),0);
 if(O && S47J::Int(O,TEXT("v"))==1)
 {
  Service=FMath::Clamp(S47J::Int(O,TEXT("service")),0,9);Feeder=FMath::Clamp(S47J::Int(O,TEXT("feeder")),0,5);
  LastFaultDay=FMath::Max(0,S47J::Int(O,TEXT("faultDay")));EmergencyDay=FMath::Max(0,S47J::Int(O,TEXT("emergencyDay")));
  Payload=FMath::Clamp(S47J::Int(O,TEXT("payload")),0,4);Item=S47J::Str(O,TEXT("item"));if(Price(Item)<=0){Payload=0;Item.Empty();}
  bGenerator=S47J::Bool(O,TEXT("generator"));GeneratorSeconds=FMath::Clamp(S47J::Num(O,TEXT("generatorSeconds")),0.,43200.);
  Fuel=FMath::Clamp(S47J::Num(O,TEXT("fuel"),3),0.,4.);LastClock=S47J::Num(O,TEXT("lastClock"),-1);
  ServiceAz=S47J::Num(O,TEXT("serviceAz"));ServiceEl=S47J::Num(O,TEXT("serviceEl"));Settle=FMath::Clamp(S47J::Num(O,TEXT("settle")),0.,1.);
  BenchJob=S47J::Str(O,TEXT("benchJob"));if(BenchJob!=TEXT("donor")&&BenchJob!=TEXT("fragment"))BenchJob.Empty();BenchSeconds=FMath::Clamp(S47J::Num(O,TEXT("benchSeconds")),0.,3.);
  FeederSeconds=FMath::Clamp(S47J::Num(O,TEXT("feederSeconds")),0.,3.);
  const auto P=S47J::Nums(O,TEXT("dropped"));if(P.Num()==3 && FMath::IsFinite(P[0])&&FMath::IsFinite(P[1])&&FMath::IsFinite(P[2]))Dropped=FVector(P[0],P[1],P[2]);else if(Payload==4){Payload=1;}
  const auto S=S47J::Obj(O,TEXT("stock"));if(S)for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("donor"),TEXT("fuel"),TEXT("battery"),TEXT("film"),TEXT("fragment"),TEXT("archived_fragment")})Stock.FindOrAdd(Id)=FMath::Clamp(S47J::Int(S,Id),0,99);
 }
 Build();OverrideExisting();
 if(M&&M->Web)for(auto& C:M->Web->CollidersMutable(TEXT("saro")))if(C.Id==TEXT("ff:bench"))C.bEnabled=true;
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
 bActive=false;
 if(M&&M->Inter)for(auto& Entry:Overrides)if(auto* I=M->Inter->Get(Entry.Key)){I->Label=MoveTemp(Entry.Value.Label);I->Use=MoveTemp(Entry.Value.Use);I->Examine=MoveTemp(Entry.Value.Examine);}Overrides.Reset();
 if(bFuelHeld&&M&&M->World)M->World->HoldTruck(0);bFuelHeld=false;
 if(M&&M->Web)for(auto& C:M->Web->CollidersMutable(TEXT("saro")))if(C.Id==TEXT("ff:bench"))C.bEnabled=false;
 if(M&&M->Web&&S03()>=0)M->Web->SetDishRemoteAvailable(S03(),true);
 for(const FS47Obj& O:Parts)if(O){O.SetVisible(false);if(const auto* N=O.W->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
}

S47J::FObj FS47FreeformWorld::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);S47J::Set(O,TEXT("service"),Service);S47J::Set(O,TEXT("feeder"),Feeder);
 S47J::Set(O,TEXT("faultDay"),LastFaultDay);S47J::Set(O,TEXT("emergencyDay"),EmergencyDay);S47J::Set(O,TEXT("payload"),Payload);S47J::Set(O,TEXT("item"),Item);
 S47J::Set(O,TEXT("generator"),bGenerator);S47J::Set(O,TEXT("generatorSeconds"),GeneratorSeconds);S47J::Set(O,TEXT("fuel"),Fuel);S47J::Set(O,TEXT("lastClock"),LastClock);
 S47J::Set(O,TEXT("serviceAz"),ServiceAz);S47J::Set(O,TEXT("serviceEl"),ServiceEl);S47J::Set(O,TEXT("settle"),Settle);
 S47J::Set(O,TEXT("benchJob"),BenchJob);S47J::Set(O,TEXT("benchSeconds"),BenchSeconds);S47J::Set(O,TEXT("feederSeconds"),FeederSeconds);
 S47J::Set(O,TEXT("dropped"),TArray<double>{Dropped.X,Dropped.Y,Dropped.Z});auto S=S47J::New();for(const TCHAR* Id:{TEXT("fuse"),TEXT("relay"),TEXT("donor"),TEXT("fuel"),TEXT("battery"),TEXT("film"),TEXT("fragment"),TEXT("archived_fragment")})S47J::Set(S,Id,Stock.FindRef(Id));S47J::Set(O,TEXT("stock"),S);return O;
}

bool FS47FreeformWorld::Near(const FVector& At,double Range) const
{
 return Ready() && (!M->World||!M->World->bDriving) && FVector2D(M->Player->Pos.X-At.X,M->Player->Pos.Z-At.Z).Size()<=Range && FMath::Abs(M->Player->Pos.Y-At.Y)<3.;
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
 if(Id==TEXT("ff:shop"))return ShopPoint();
 if(Id==TEXT("ff:bench")||Id==TEXT("ff:depot"))return BenchAt+FVector(0,.90,0);
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return GeneratorAt;
 if(Id==TEXT("ff:feeder"))return FS47Substation::Site(FVector(5.52,-9.04,1.30));
 if(Id==TEXT("ff:service"))
 {
  if(M&&M->Web){const FS47Obj C=M->Web->Obj(TEXT("yard.objs.motorBus"));if(C)return C.LocalToWorld(CabinetLocal+FVector(0,0,.07));}
  return FVector(11.95,.62,-11);
 }
 if(Id==TEXT("ff:cargo"))return TruckPoint();
 if(Id==TEXT("ff:parcel"))return Payload==1?ShopPoint()+FVector(.55,-.25,.2):Payload==3?TruckPoint():Payload==4?Dropped:M&&M->Player?M->Player->Pos+FVector(0,.8,-.6):FVector::ZeroVector;
 return FVector::ZeroVector;
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
 I.Examine=[this,Guard,Id]{return Guard.IsValid()?Label(Id):FString();};M->Inter->Add(I);M->Web->MarkUsable(Object.I,Id);
}

void FS47FreeformWorld::OverrideExisting()
{
 if(!M||!M->Inter||!bActive)return;
 const TWeakPtr<uint8> Guard=Life;
 for(const TCHAR* Raw:{TEXT("substation:journal"),TEXT("substation:service"),TEXT("substation:lamp"),TEXT("obs:generator")})
 {
  const FString Id(Raw);if(Overrides.Contains(Id))continue;auto* I=M->Inter->Get(Id);if(!I)continue;
  FOverride O;O.Label=I->Label;O.Examine=I->Examine;O.Use=I->Use;Overrides.Add(Id,MoveTemp(O));
  I->Label=[this,Guard,Id]{return Guard.IsValid()?Label(Id):FString();};
  I->Examine=I->Label;I->Use=[this,Guard,Id]{if(Guard.IsValid())Use(Id);};
 }
}

bool FS47FreeformWorld::Build()
{
 if(bBuilt)return true;if(!M||!M->Web||!M->Web->IsLoaded()||!M->Inter)return false;
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
 const FS47Obj Cabinet=M->Web->Obj(TEXT("yard.objs.motorBus"));
 if(Cabinet)
 {
  const FVector At=Cabinet.LocalToWorld(CabinetLocal);
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
 };
 LabelPart(TEXT("FreeformGasLabel"),TEXT("SERVICE SUPPLIES\n08:00 - 20:00"),ShopPoint()+FVector(.10,0,0),0,3.1,Road);
 LabelPart(TEXT("FreeformBenchLabel"),TEXT("SARO SERVICE BAY\nTEST / PARTS / STOCK"),BenchAt+FVector(0,.55,.42),90,3.5,Saro);
 bBuilt=Bench&&Shop&&Panel&&FeederPanel;Apply();return bBuilt;
}

void FS47FreeformWorld::Apply()
{
 if(!bBuilt)return;for(const FS47Obj& O:Parts)if(O){O.SetVisible(bActive);if(const auto* N=O.W->Node(O.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(bActive&&C->IsA<UStaticMeshComponent>()?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}
 if(BenchSample)BenchSample.SetVisible(bActive&&(InventoryCount(TEXT("fragment"))>0||InventoryCount(TEXT("archived_fragment"))>0||BenchJob==TEXT("fragment")));
 if(BenchLamp){const bool Testing=!BenchJob.IsEmpty();BenchLamp.SetColor(S47Three::Hex(Testing?0xc8a046:0x5e8b61));BenchLamp.SetEmissive(S47Three::Hex(Testing?0xc8a046:0x5e8b61),Testing?.2f+float(BenchSeconds/3.)*.8f:.15f);}
 const bool Open=bActive&&Service>=4&&Service<=5;
 if(Lid){const FVector At=InteractionPoint(TEXT("ff:service"))+FVector(-.015,0,0);if(const auto* N=Lid.W->Node(Lid.I))if(auto* C=N->Comp.Get())C->SetWorldLocation(S47Three::ToUE(At+FVector(0,0,Open?.38:0)));Lid.SetVisible(bActive);}
 if(Relay)Relay.SetColor(S47Three::Hex(Service>=5?0x739470:0xb19651));
 if(Selector)Selector.SetColor(S47Three::Hex(HasArrayFault()?0xb54e31:0x4e8d5c));
 if(FeederLamp)FeederLamp.SetColor(S47Three::Hex(M&&M->Freeform&&M->Freeform->PowerFault()?0xc57d32:0x58a769));
 if(FeederLever)FeederLever.SetRotX(Feeder>=2&&Feeder<5?.55:0);
 const bool Road=M&&M->World&&(M->World->Area==TEXT("road")||M->World->Area==TEXT("diner"));
 FVector Position=InteractionPoint(TEXT("ff:parcel"));
 if(Payload==2&&M&&M->Player)
 {
  const FVector Eye=M->Player->Pos+FVector(0,1.0,0),Fwd=S47Three::ToThree(M->Player->ForwardUE()*100.);Position=Eye+FVector(Fwd.X,0,Fwd.Z).GetSafeNormal()*.65;
  // A carried package stops before opaque scenery instead of being pushed through it.
  FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(FreeformCarry),true);
  if(const auto* N=ParcelSaro.W->Node(ParcelSaro.I))if(N->Comp.IsValid())Q.AddIgnoredComponent(Cast<UPrimitiveComponent>(N->Comp.Get()));
  if(const auto* N=ParcelRoad.W->Node(ParcelRoad.I))if(N->Comp.IsValid())Q.AddIgnoredComponent(Cast<UPrimitiveComponent>(N->Comp.Get()));
  if(M->Web->GetWorld()->LineTraceSingleByChannel(Hit,S47Three::ToUE(Eye),S47Three::ToUE(Position),ECC_Visibility,Q))Position=S47Three::ToThree(Hit.Location)+(Eye-Position).GetSafeNormal()*.12;
 }
 for(const auto& Pair:TArray<TPair<FS47Obj,bool>>{{ParcelSaro,!Road},{ParcelRoad,Road}})if(Pair.Key)
 {
  const bool Show=bActive&&Payload>0&&Pair.Value;if(const auto* N=Pair.Key.W->Node(Pair.Key.I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get())){C->SetWorldLocation(S47Three::ToUE(Position));C->SetCollisionEnabled(Show&&Payload!=2?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);}Pair.Key.SetVisible(Show);
 }
 for(const auto& Pair:TArray<TPair<FS47Obj,bool>>{{CargoSaro,!Road},{CargoRoad,Road}})if(Pair.Key)
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
 if(Emergency)EmergencyDay=M->Freeform->Day();Item=Id;Payload=1;Change(TEXT("Paid and labelled. Collect the physical parcel beside the supply hatch."));return true;
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

bool FS47FreeformWorld::ParcelUse()
{
 if(!Payload||Payload==2)return true;if(!Near(InteractionPoint(TEXT("ff:parcel")),2.8))return true;
 Payload=2;Change(TEXT("Parcel carried. Use the truck cargo rack to load it, or the SARO bench to unpack it."));return true;
}
bool FS47FreeformWorld::CargoUse()
{
 if(!Near(TruckPoint(),3.3))return true;
 if(Payload==2){Payload=3;Change(TEXT("Parcel secured in the service-truck cargo rack. It travels with the truck."));}
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
 if(Payload==2){const FString Delivered=Item;Payload=0;Item.Empty();AddItem(Delivered,1);Change(FString(TEXT("Delivered to SARO stock: "))+ItemTitle(Delivered)+TEXT(". The labelled parcel is unpacked once."));return true;}
 OpenWorkbench();return true;
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
 M->World->Enter(TEXT("saro"));M->World->PlaceTruck(TEXT("saro"));M->Player->Place(16,9,0);Fuel=.25;M->Freeform->SpendTime(15*60);Change(TEXT("County roadside service returned truck and cargo to SARO. A small reserve permits the next fuel run."));
 M->Freeform->RecordEvidence(TEXT("roadside"),TEXT("Roadside return used; the interrupted trip and cargo were retained."));
}

bool FS47FreeformWorld::TriggerServiceFault()
{
 if(!Ready()||HasArrayFault()||M->Freeform->PowerFault())return false;
 Service=1;Settle=0;LastFaultDay=M->Freeform->Day();if(S03()>=0)M->Web->SetDishRemoteAvailable(S03(),false);
 Change(TEXT("S-03 remote link failed its health poll. Diagnose it on the left status terminal; unaffected antennas remain available."));return true;
}
void FS47FreeformWorld::DiagnoseService(){if(Ready()&&Service==1){Service=2;Change(TEXT("S-03 command relay does not close. Isolate the local selector, open the hatch and replace the compatible relay."));}}
bool FS47FreeformWorld::ServiceUse()
{
 if(!Near(InteractionPoint(TEXT("ff:service")),2.9))return true;
 switch(Service)
 {
  case 0:case 9:Change(TEXT("S-03 relay and local movement are in service. No replacement is required."));return true;
  case 1:if(M->Freeform->HasUpgrade(2)){DiagnoseService();Change(TEXT("Field test kit confirms a failed command relay here. Isolate LOCAL before opening the hatch."));}else Change(TEXT("Read S-03's fault on the status terminal before opening its service hatch."));return true;
  case 2:Service=3;Change(TEXT("S-03 set to LOCAL. Remote movement is isolated. Open the cover."));break;
  case 3:Service=4;Change(TEXT("Hatch open. The command relay is not closing; seat a compatible replacement from your stock."));break;
  case 4:if(!Consume(TEXT("relay"))){Change(TEXT("No compatible relay in SARO stock. A new relay or a tested donor controller is available at Caliche."));return true;}Service=5;Change(TEXT("Replacement relay seated. Close the cover before the movement test."));break;
  case 5:Service=6;Change(TEXT("Cover secured. Use the local control to test five degrees out and back."));break;
  case 6:if(S03()<0)return true;ServiceAz=M->Web->DishAz(S03());ServiceEl=M->Web->DishEl(S03());Service=7;Settle=0;M->Web->PointDish(S03(),ServiceAz+5,ServiceEl,2.2,true);Change(TEXT("Local test running: watch S-03 turn five degrees and return."));break;
  case 7:case 8:Change(TEXT("The actual local movement test is still running."));break;
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
 if(Id==TEXT("ff:shop"))return ShopOpen()?TEXT("Buy service supplies / collect a physical parcel"):TEXT("Supply hatch closed / essential requisition");
 if(Id==TEXT("ff:bench")||Id==TEXT("ff:depot"))return Payload==2?TEXT("Unpack carried parcel into SARO stock"):TEXT("Use service bench and inspect stock");
 if(Id==TEXT("ff:parcel"))return Payload>0&&Payload!=2?FString(TEXT("Carry "))+ItemTitle(Item):FString();
 if(Id==TEXT("ff:cargo"))return Payload==2?TEXT("Secure carried parcel in truck"):Payload==3?TEXT("Unload parcel from truck"):TEXT("Truck fuel / cargo rack");
 if(Id==TEXT("ff:generator")||Id==TEXT("obs:generator"))return bGenerator?TEXT("Stop backup generator"):TEXT("Start / fuel backup generator");
 if(Id==TEXT("ff:feeder")||Id==TEXT("substation:service")||Id==TEXT("substation:lamp")||Id==TEXT("substation:journal"))return M->Freeform->PowerFault()?Feeder==4&&FeederSeconds>0?TEXT("Feeder test running"):Feeder==4?TEXT("Run feeder replacement-fuse test"):Feeder==3?TEXT("Replace isolated feeder control fuse"):Feeder==2?TEXT("Open enclosed feeder isolation switch"):TEXT("Measure SARO feeder supply"):TEXT("Check normal feeder supply");
 if(Id==TEXT("ff:service")||Id==TEXT("obs:service"))
 {
  static const TCHAR* Labels[]={TEXT("Inspect healthy S-03 control"),TEXT("S-03 fault / read status terminal first"),TEXT("Select S-03 LOCAL service"),TEXT("Open S-03 relay hatch"),TEXT("Seat compatible replacement relay"),TEXT("Close S-03 relay hatch"),TEXT("Run S-03 local movement test"),TEXT("Local test: outward movement"),TEXT("Local test: return movement"),TEXT("S-03 verified / REMOTE restored")};return Labels[FMath::Clamp(Service,0,9)];
 }
 return FString();
}

void FS47FreeformWorld::Step(double Dt)
{
 if(!Ready())return;if(!bBuilt)Build();
 const double Now=M->Freeform->Clock();if(LastClock<0)LastClock=Now;
 const double WorldDelta=FMath::Max(0.,Now-LastClock);LastClock=Now;
 if(M->Freeform->Held()){Apply();return;}
 if(bGenerator){GeneratorSeconds=FMath::Max(0.,GeneratorSeconds-WorldDelta);if(GeneratorSeconds<=0){bGenerator=false;Change(TEXT("Backup generator tank empty. Array work needs restored mains or another fuel can."));}}
 if(M->World&&M->World->bDriving)
 {
  Fuel=FMath::Max(0.,Fuel-FMath::Abs(M->World->DrivingSpeed())*FMath::Min(Dt,.2)/12000.);
  if(Fuel<=.001){M->World->HoldTruck(25.);bFuelHeld=true;}
  else if(bFuelHeld){M->World->HoldTruck(0);bFuelHeld=false;}
 }
 if(Service==7||Service==8)
 {
  const int32 I=S03();if(I>=0){const double Target=ServiceAz+(Service==7?5.:0.);
   const bool On=FMath::Abs(FMath::FindDeltaAngleDegrees(M->Web->DishAz(I),Target))<=.1&&FMath::Abs(M->Web->DishEl(I)-ServiceEl)<=.1;
   Settle=On?Settle+FMath::Min(Dt,.1):0;
   if(Settle>=1){Settle=0;if(Service==7){Service=8;M->Web->PointDish(I,ServiceAz,ServiceEl,2.2,true);}else{Service=9;M->Web->SetDishRemoteAvailable(I,true);M->Freeform->RecordEvidence(TEXT("service-s03"),TEXT("S-03 replacement relay passed local five-degree out-and-back test. Remote pointing restored."));Change(TEXT("S-03 returned to its starting angle. Local test passed; REMOTE restored."));}}
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
    AddItem(TEXT("relay"));M->Freeform->RecordEvidence(TEXT("donor-relay"),TEXT("Donor controller relay tested at SARO bench and harvested as a compatible spare."));
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
 W.Item=TEXT("fuel");W.Payload=3;W.Service=4;W.Fuel=.7;W.BenchJob=TEXT("donor");W.BenchSeconds=1.2;W.GeneratorSeconds=7200;W.bGenerator=true;
 const auto Save=S47J::Read(S47J::Write(W.Save()));FS47FreeformWorld R(nullptr);R.Begin(Save);
 C(R.Payload==3&&R.Item==TEXT("fuel")&&R.Service==4&&R.Fuel==.7,TEXT("Freeform cargo and opened service survive a serialized restore"));
 C(R.BenchJob==TEXT("donor")&&R.BenchSeconds==1.2&&R.GeneratorSeconds==7200&&R.bGenerator,TEXT("Freeform in-progress donor and generator preserve consumed inputs"));
 C(!R.Purchase(TEXT("relay"))&&R.Payload==3,TEXT("Freeform purchase requires actual world and cannot duplicate loaded parcel"));
 W.AddItem(TEXT("fragment"),1);W.AddItem(TEXT("archived_fragment"),1);W.BenchJob=TEXT("fragment");W.BenchSeconds=1.7;
 FS47FreeformWorld Sample(nullptr);Sample.Begin(S47J::Read(S47J::Write(W.Save())));
 C(Sample.InventoryCount(TEXT("fragment"))==1&&Sample.InventoryCount(TEXT("archived_fragment"))==1&&Sample.BenchJob==TEXT("fragment")&&Sample.BenchSeconds==1.7,TEXT("Freeform field fragments, sealed originals and unfinished analysis persist"));
 C(Price(TEXT("fragment"))==0&&Price(TEXT("archived_fragment"))==0,TEXT("Freeform field samples cannot be purchased or loaned as shop stock"));
 return OK;
}
