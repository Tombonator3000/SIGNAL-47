#include "S47RoomLights.h"
#include "S47Main.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Walker.h"
#include "S47Inter.h"
#include "S47Story.h"
#include "S47UI.h"
#include "S47Audio.h"
#include "S47Three.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

namespace
{
 struct FSwitch { const TCHAR* Room; const TCHAR* Title; const TCHAR* Parent; FVector At; double Normal; bool bSideWall = false; };
 // Existing wall faces, outside door sweeps. Metres in the gameplay coordinate system.
 // Control/corridor are opposite faces of the same wall; each must be reached from its room.
 const FSwitch Switches[] = {
  {TEXT("control"),TEXT("CONTROL ROOM"),TEXT("room.group"),FVector(-3.25,1.35,4.475),-1},
  {TEXT("corridor"),TEXT("CORRIDOR"),TEXT("annex.interior"),FVector(-3.25,1.35,4.83),1},
  {TEXT("records"),TEXT("RECORDS"),TEXT("annex.interior"),FVector(-4.75,1.35,6.825),1},
  // East lining is x=1.78; keep the plate clear of the departure notice on the front wall.
  {TEXT("office"),TEXT("OPERATIONS"),TEXT("annex.interior"),FVector(1.765,1.35,7.70),-1,true}
 };
 FVector Facing(int32 I) { return Switches[I].bSideWall?FVector(Switches[I].Normal,0,0):FVector(0,0,Switches[I].Normal); }
 FString IdOf(int32 I) { return FString(TEXT("roomlight:"))+Switches[I].Room; }
 int32 IndexOf(const FString& Id)
 {
  for(int32 I=0;I<UE_ARRAY_COUNT(Switches);++I)if(Id==Switches[I].Room||Id==IdOf(I))return I;
  return INDEX_NONE;
 }
}
FS47RoomLights::~FS47RoomLights(){Stop();Life.Reset();}
void FS47RoomLights::Begin(const S47J::FObj& Saved)
{
 bActive=true;Retry=0;
 for(int32 I=0;I<UE_ARRAY_COUNT(Switches);++I)
  Enabled[I]=!Saved||S47J::Int(Saved,TEXT("v"))!=1?true:S47J::Bool(Saved,Switches[I].Room,true);
 Build();Apply();
}
S47J::FObj FS47RoomLights::Save() const
{
 auto O=S47J::New();S47J::Set(O,TEXT("v"),1);
 for(int32 I=0;I<UE_ARRAY_COUNT(Switches);++I)S47J::Set(O,Switches[I].Room,Enabled[I]);
 return O;
}
bool FS47RoomLights::RoomEnabled(const FString& Room) const
{
 const int32 I=IndexOf(Room);return !bActive||I<0||Enabled[I];
}
FVector FS47RoomLights::InteractionPoint(const FString& Room) const
{
 const int32 I=IndexOf(Room);return I<0?FVector::ZeroVector:Switches[I].At+Facing(I)*.028;
}
FVector FS47RoomLights::ApproachPoint(const FString& Room, double Distance) const
{
 const int32 I=IndexOf(Room);return I<0?FVector::ZeroVector:InteractionPoint(Room)+Facing(I)*Distance;
}
bool FS47RoomLights::Available(int32 I) const
{
 return I>=0&&I<UE_ARRAY_COUNT(Switches)&&bActive&&bBuilt&&M&&M->Mode==US47Main::EMode::Play&&M->Web&&M->World&&M->World->Area==TEXT("saro")&&!M->World->bDriving&&M->Web->Obj(Switches[I].Parent).Shown();
}
FString FS47RoomLights::Label(const FString& Id) const
{
 const int32 I=IndexOf(Id);if(!Available(I))return FString();
 return FString::Printf(TEXT("%s ceiling light / switch %s"),Switches[I].Title,Enabled[I]?TEXT("OFF"):TEXT("ON"));
}
bool FS47RoomLights::Use(const FString& Id)
{
 const int32 I=IndexOf(Id);if(!Available(I)||!M->Player||!M->Inter)return false;
 if(M->bPausedByMenu||M->ModalOpen()||(M->Story&&M->Story->IsCinematic()))return false;
 const FVector Eye=S47Three::ToThree(M->Player->EyeUE());
 // Distance plus actual eye ray prevents remote calls and use through the shared wall.
 if(FVector::DotProduct(Eye-Switches[I].At,Facing(I))<=0||FVector::Dist(Eye,InteractionPoint(Id))>2.2||M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE())!=IdOf(I))return false;
 Enabled[I]=!Enabled[I];Apply();
 if(M->Audio){FS47Sound S;S.At=InteractionPoint(Id);S.Gain=.45f;M->Audio->Sfx(M->Audio->Or(TEXT("small_switch"),TEXT("switch")),S);}
 if(M->UI)M->UI->Toast(FString::Printf(TEXT("%s ceiling switch %s.%s"),Switches[I].Title,Enabled[I]?TEXT("ON"):TEXT("OFF"),M->Story&&M->Story->bDark?TEXT(" Supply is off; your switch setting is retained."):TEXT(" Equipment power is unchanged.")),3);
 M->RequestAutosave();return true;
}
bool FS47RoomLights::Build()
{
 if(bBuilt)return true;
 if(!M||!M->Web||!M->Web->IsLoaded()||!M->Inter)return false;
 for(const auto& S:Switches)if(!M->Web->Obj(S.Parent))return false;
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));
 if(!Mesh||!Base)return false;
 const TWeakPtr<uint8> Guard=Life;
 for(int32 I=0;I<UE_ARRAY_COUNT(Switches);++I)
 {
  const auto& S=Switches[I];const FS47Obj Parent=M->Web->Obj(S.Parent);
  auto Box=[&](const FString& Name,const FVector& At,const FVector& Size,uint32 Color)
  {
   auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web);Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Color));Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);Mat->SetScalarParameterValue(TEXT("Roughness"),.7f);
   auto* C=NewObject<UStaticMeshComponent>(M->Web,*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->Node(Parent.I)->Comp.Get());C->SetStaticMesh(Mesh);C->SetMaterial(0,Mat);
   C->SetWorldLocation(S47Three::ToUE(At));C->SetWorldScale3D(S47Three::DirToUE(Size));C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();
   FS47Obj O=M->Web->ObjAt(M->Web->AdoptNode(C,Parent.I,Name));Parts.Add(O);M->Web->MarkUsable(O.I,IdOf(I));return O;
  };
  Plates[I]=Box(FString(TEXT("RoomLightPlate_"))+S.Room,S.At,S.bSideWall?FVector(.022,.15,.09):FVector(.09,.15,.022),0xc3baa5);
  Levers[I]=Box(FString(TEXT("RoomLightLever_"))+S.Room,InteractionPoint(S.Room),S.bSideWall?FVector(.026,.055,.025):FVector(.025,.055,.026),0xe0d8c5);
  auto* Text=NewObject<UTextRenderComponent>(M->Web);Text->SetupAttachment(M->Web->Node(Parent.I)->Comp.Get());Text->SetMobility(EComponentMobility::Movable);Text->SetText(FText::FromString(FString(S.Title)+TEXT("\nCEILING")));Text->SetTextRenderColor(FColor(212,203,179));Text->SetWorldSize(1.6f);Text->SetHorizontalAlignment(EHTA_Center);Text->SetVerticalAlignment(EVRTA_TextCenter);Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);Text->RegisterComponent();Text->SetWorldLocation(S47Three::ToUE(S.At+FVector(0,.14,0)+Facing(I)*.016));Text->SetWorldRotation(FRotator(0,S.bSideWall?180:S.Normal*90,0));
  Parts.Add(M->Web->ObjAt(M->Web->AdoptNode(Text,Parent.I,FString(TEXT("RoomLightLabel_"))+S.Room)));
  FS47Interactable Item;Item.Id=IdOf(I);Item.Object=Plates[I];Item.Range=2.2;
  Item.Label=[this,Guard,I]{return Guard.IsValid()?Label(IdOf(I)):FString();};
  Item.Use=[this,Guard,I]{if(Guard.IsValid())Use(IdOf(I));};
  Item.Examine=[this,Guard,I]{return Guard.IsValid()&&Available(I)?FString::Printf(TEXT("%s ceiling switch is %s. This controls room lighting only; computers and instruments have separate power."),Switches[I].Title,Enabled[I]?TEXT("ON"):TEXT("OFF")):FString();};M->Inter->Add(Item);
 }
 bBuilt=true;return true;
}
void FS47RoomLights::Apply()
{
 if(!bBuilt||!M||!M->Web)return;
 for(int32 I=0;I<UE_ARRAY_COUNT(Switches);++I)
 {
  const bool Show=Available(I);
  for(int32 K=I*3;K<I*3+3;++K)if(Parts.IsValidIndex(K)&&Parts[K])
  {
   Parts[K].SetVisible(Show);
   if(const auto* N=M->Web->Node(Parts[K].I))if(auto* C=Cast<UPrimitiveComponent>(N->Comp.Get()))C->SetCollisionEnabled(Show&&C->IsA<UStaticMeshComponent>()?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
  }
  // Adopted Web nodes do not retain the native mesh scale; avoid SetRotX here.
  if(const auto* N=M->Web->Node(Levers[I].I))if(auto* C=N->Comp.Get())C->SetWorldRotation(Switches[I].bSideWall?FRotator((Enabled[I]?-22.:22.)*Switches[I].Normal,0,0):FRotator(0,0,(Enabled[I]?-22.:22.)*Switches[I].Normal));
 }
}
void FS47RoomLights::Step(double Dt)
{
 if(!bActive)return;
 if(!bBuilt){Retry-=FMath::Max(0.,Dt);if(Retry<=0){Build();Retry=1.;}}
 Apply();
}
void FS47RoomLights::Stop()
{
 bActive=false;
 if(IsEngineExitRequested()||IsGarbageCollecting()||!IsValid(M)||M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)||!IsValid(M->Web.Get()))return;
 Apply();
}


bool FS47RoomLights::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool Good,const TCHAR* Message){OK&=Good;Lines.Add(FString(Good?TEXT("PASS "):TEXT("FAIL "))+Message);};
 FS47RoomLights L(nullptr);L.Begin();
 Check(L.RoomEnabled(TEXT("control"))&&L.RoomEnabled(TEXT("corridor"))&&L.RoomEnabled(TEXT("records"))&&L.RoomEnabled(TEXT("office")),TEXT("Legacy room lighting defaults to all switches on"));
 bool AllRestored=true,AllRejected=true;
 for(int32 Mask=0;Mask<16;++Mask)
 {
  for(int32 I=0;I<4;++I)L.Enabled[I]=(Mask&(1<<I))!=0;
  FS47RoomLights Restored(nullptr);Restored.Begin(S47J::Read(S47J::Write(L.Save())));
  bool Same=true;for(int32 I=0;I<4;++I)Same&=Restored.RoomEnabled(Switches[I].Room)==L.Enabled[I];
  AllRestored&=Same;AllRejected&=!Restored.Use(TEXT("roomlight:control"));
 }
 Check(AllRestored,TEXT("All 16 independent room-switch combinations survive JSON Continue"));
 Check(AllRejected,TEXT("Room switch cannot be operated without an actual reachable world interaction"));
 auto Partial=S47J::New();S47J::Set(Partial,TEXT("v"),1);S47J::Set(Partial,TEXT("control"),false);L.Begin(Partial);
 Check(!L.RoomEnabled(TEXT("control"))&&L.RoomEnabled(TEXT("office")),TEXT("Partial legacy room settings preserve saved off and default absent rooms on"));
 L.Stop();Check(L.RoomEnabled(TEXT("control")),TEXT("Stopped room session releases its lighting mask"));
 L.Begin();Check(L.RoomEnabled(TEXT("control")),TEXT("New case resets the previous case's room switches"));
 return OK;
}
