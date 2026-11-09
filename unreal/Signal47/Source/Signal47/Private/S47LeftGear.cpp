#include "S47LeftGear.h"
#include "S47Audio.h"
#include "S47FieldCamera.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47Inter.h"
#include "S47Main.h"
#include "S47Three.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/GarbageCollection.h"

namespace
{
 const TCHAR* const Kinds[]={TEXT("radio"),TEXT("camera")};
 FVector PropSize(const FString& Item){return Item==TEXT("radio")?FVector(.09,.3,.07):FVector(.15,.1,.1);} // metres, three axes: x, y up, z
}

FString FS47LeftGear::NameOf(const FString& Item){return Item==TEXT("radio")?TEXT("field set"):Item==TEXT("camera")?TEXT("camera"):Item;}

void FS47LeftGear::Begin(const S47J::FObj& Saved)
{
 Stop();Life=MakeShared<uint8>(0);Left.Reset();Place=FLeft();bPlace=false;
 if(const auto P=S47J::Obj(Saved,TEXT("place")))
 {
  Place.Area=S47J::Str(P,TEXT("area"));Place.Title=S47J::Str(P,TEXT("title"));Place.Clock=S47J::Num(P,TEXT("clock"));
  const auto A=S47J::Nums(P,TEXT("at"));if(A.Num()==3)Place.At=FVector(A[0],A[1],A[2]);
  bPlace=!Place.Area.IsEmpty()&&FMath::IsFinite(Place.Clock)&&A.Num()==3&&!Place.At.ContainsNaN();
  if(!bPlace)Place=FLeft();
 }
 for(const auto& V:S47J::Arr(Saved,TEXT("left")))
 {
  if(!V||V->Type!=EJson::Object)continue;const auto O=V->AsObject();
  FLeft L;L.Item=S47J::Str(O,TEXT("item"));L.Area=S47J::Str(O,TEXT("area"));L.Title=S47J::Str(O,TEXT("title"));L.Clock=S47J::Num(O,TEXT("clock"));
  const auto A=S47J::Nums(O,TEXT("at"));if(A.Num()==3)L.At=FVector(A[0],A[1],A[2]);
  const bool bKnown=L.Item==Kinds[0]||L.Item==Kinds[1];
  if(!bKnown||L.Area.IsEmpty()||!FMath::IsFinite(L.Clock)||A.Num()!=3||L.At.ContainsNaN()||IsLeft(L.Item))continue;
  Left.Add(L);
 }
 if(M&&IsValid(M->Web.Get())&&Scene.Get()!=M->Web.Get()){bBuilt=false;Slots.Reset();}
 if(M&&IsValid(M->Web.Get())&&Build())
 {
  Scene=M->Web;
  const TWeakPtr<uint8> Guard=Life;
  if(IsValid(M->Inter))for(FSlot& S:Slots)
  {
   const FString Item=S.Item;
   FS47Interactable I;I.Id=TEXT("ff:left:")+Item;I.Object=S.SaroObj;I.Range=2.8;
   I.Label=[this,Guard,Item]{return Guard.IsValid()&&IsLeft(Item)?FString::Printf(TEXT("Your %s / pick it up"),*NameOf(Item)):FString();};
   I.Use=[this,Guard,Item]{if(Guard.IsValid())Fetch(Item);};
   I.Examine=I.Label;
   M->Inter->Add(I);
   if(S.SaroObj)M->Web->MarkUsable(S.SaroObj.I,I.Id);
   if(S.RoadObj)M->Web->MarkUsable(S.RoadObj.I,I.Id);
  }
  Layout();
 }
}
void FS47LeftGear::Stop()
{
 Life.Reset();
 if(!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(Scene.Get()))
  for(FSlot& S:Slots)for(auto* C:{S.Saro.Get(),S.Road.Get()})if(C){C->SetVisibility(false);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
}
S47J::FObj FS47LeftGear::Save() const
{
 auto O=S47J::New();
 if(bPlace){auto P=S47J::New();S47J::Set(P,TEXT("area"),Place.Area);S47J::Set(P,TEXT("title"),Place.Title);S47J::Set(P,TEXT("clock"),Place.Clock);S47J::Set(P,TEXT("at"),TArray<double>{Place.At.X,Place.At.Y,Place.At.Z});S47J::Set(O,TEXT("place"),P);}
 if(Left.IsEmpty())return O;
 TArray<TSharedPtr<FJsonValue>> List;
 for(const FLeft& L:Left)
 {
  auto X=S47J::New();S47J::Set(X,TEXT("item"),L.Item);S47J::Set(X,TEXT("area"),L.Area);S47J::Set(X,TEXT("title"),L.Title);S47J::Set(X,TEXT("clock"),L.Clock);S47J::Set(X,TEXT("at"),TArray<double>{L.At.X,L.At.Y,L.At.Z});
  List.Add(MakeShared<FJsonValueObject>(X));
 }
 O->SetArrayField(TEXT("left"),List);return O;
}
bool FS47LeftGear::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web.Get()))return false;
 const int32 SaroRoot=M->Web->Handle(TEXT("ext.group")),RoadRoot=M->Web->Handle(TEXT("world.legs.road.area.group"));if(SaroRoot<0||RoadRoot<0)return false;
 auto* Box=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Box||!Base)return false;
 for(const TCHAR* Kind:Kinds)
 {
  FSlot S;S.Item=Kind;
  const auto Prop=[&](int32 Parent,const FString& Name,FS47Obj& Out)
  {
   auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web.Get());Mat->SetVectorParameterValue(TEXT("Color"),FString(Kind)==Kinds[0]?FLinearColor(.05f,.07f,.04f):FLinearColor(.03f,.03f,.03f));Mat->SetScalarParameterValue(TEXT("Roughness"),.6f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
   auto* C=NewObject<UStaticMeshComponent>(M->Web.Get(),*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Box);C->SetMaterial(0,Mat);C->SetCastShadow(true);
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);C->RegisterComponent();
   Out=M->Web->ObjAt(M->Web->AdoptNode(C,Parent,Name));
   return C;
  };
  S.Saro=Prop(SaroRoot,FString::Printf(TEXT("ff:left:%s"),Kind),S.SaroObj);
  S.Road=Prop(RoadRoot,FString::Printf(TEXT("ff:left:%s:road"),Kind),S.RoadObj);
  Slots.Add(S);
 }
 bBuilt=true;return true;
}
bool FS47LeftGear::Here(const FLeft& L) const
{
 if(!M||!M->Web)return false;
 const FString A=M->Web->Area();
 const auto Town=[](const FString& X){return X==TEXT("road")||X==TEXT("diner");};
 return A==L.Area||(Town(A)&&Town(L.Area));
}
void FS47LeftGear::Layout()
{
 if(!bBuilt)return;
 for(FSlot& S:Slots)
 {
  const FLeft* L=Left.FindByPredicate([&](const FLeft& X){return X.Item==S.Item;});
  const bool bSaroSide=L&&(L->Area==TEXT("saro"));
  for(auto* C:{S.Saro.Get(),S.Road.Get()})
  {
   if(!C)continue;
   const bool bThis=L&&Here(*L)&&(C==S.Saro.Get())==bSaroSide;
   C->SetVisibility(bThis);C->SetHiddenInGame(!bThis);C->SetCollisionEnabled(bThis?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
   if(bThis)
   {
    const FVector Size=PropSize(S.Item);
    C->SetWorldLocation(S47Three::ToUE(L->At+FVector(0,Size.Y*.5+.02,0)));C->SetWorldRotation(FRotator(0,float(FCrc::StrCrc32(*S.Item)%180),12));
    C->SetWorldScale3D(FVector(Size.X,Size.Z,Size.Y)); // The cube is a metre on each side: UE's x, y, z.
   }
  }
 }
}
TArray<FString> FS47LeftGear::Leave(int32 Count,const FString& Area,const FVector& At,const FString& Title,double Clock)
{
 TArray<FString> Out;if(!M)return Out;
 const auto Drop=[&](const TCHAR* Item)
 {
  FLeft L;L.Item=Item;L.Area=Area;L.At=At+FVector(FMath::FRandRange(-1.2f,1.2f),0,FMath::FRandRange(-1.2f,1.2f));L.Title=Title;L.Clock=Clock;
  if(!IsLeft(L.Item)){Left.Add(L);Out.Add(L.Item);}
 };
 if(Count>=1&&M->FieldRadio&&M->FieldRadio->Owned()){M->FieldRadio->Lose();Drop(Kinds[0]);}
 if(Count>=2&&M->FieldCamera&&M->FieldCamera->bHave){M->FieldCamera->Raise(false);M->FieldCamera->bHave=false;Drop(Kinds[1]);}
 Layout();
 for(const FString& Item:Out)UE_LOG(LogTemp,Display,TEXT("S47: left behind: %s at %s, area %s"),*Item,*At.ToCompactString(),*Area);
 return Out;
}
void FS47LeftGear::MarkPlace(const FString& Area,const FVector& At,const FString& Title,double Clock)
{
 if(Area.IsEmpty()||At.ContainsNaN()||!FMath::IsFinite(Clock))return;
 Place=FLeft();Place.Area=Area;Place.At=At;Place.Title=Title;Place.Clock=Clock;bPlace=true;
 UE_LOG(LogTemp,Display,TEXT("S47: left behind: nothing, the place kept at %s, area %s"),*At.ToCompactString(),*Area);
}
void FS47LeftGear::GiveBack(const FLeft& L,bool bToHand)
{
 if(!M)return;
 if(L.Item==Kinds[0]&&M->FieldRadio)M->FieldRadio->Recover(bToHand);
 if(L.Item==Kinds[1]&&M->FieldCamera&&bToHand)M->FieldCamera->bHave=true;
}
bool FS47LeftGear::Fetch(const FString& Item)
{
 const int32 Index=Left.IndexOfByPredicate([&](const FLeft& L){return L.Item==Item;});
 if(Index==INDEX_NONE||!M||!M->Player||!Here(Left[Index]))return false;
 if(FVector2D(M->Player->Pos.X-Left[Index].At.X,M->Player->Pos.Z-Left[Index].At.Z).Size()>3.5)return false;
 const FLeft L=Left[Index];Left.RemoveAt(Index);GiveBack(L,true);Layout();
 if(M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.5f});
 if(M->UI)M->UI->Toast(FString::Printf(TEXT("Your %s, in the dirt where you stood. It still works."),*NameOf(Item)),5);
 UE_LOG(LogTemp,Display,TEXT("S47: left behind: %s fetched"),*Item);
 if(M->Freeform)M->Freeform->LostTimeTrace(1);
 bPlace=false; // the place has been looked at, whatever else happened there
 M->RequestAutosave();return true;
}
void FS47LeftGear::Step(double Dt)
{
 if(!M||!M->Freeform||!M->Web)return;
 const double Clock=M->Freeform->Clock();
 // Nothing stays lost for good: somebody finds it and brings it in. Back on the desk or the bench, for the player to take.
 for(int32 I=Left.Num()-1;I>=0;--I)if(Clock-Left[I].Clock>HandedInAfter)
 {
  const FLeft L=Left[I];Left.RemoveAt(I);GiveBack(L,false);Layout();
  M->Freeform->RecordEvidence(FString::Printf(TEXT("gear-handed-in-%s-d%d"),*L.Item,M->Freeform->Day()),FString::Printf(TEXT("A county car left a SARO %s at the gate, picked up off %s. It is back where it is kept. Nobody said anything else about it."),*NameOf(L.Item),*L.Title));
  if(M->UI)M->UI->Toast(FString::Printf(TEXT("Somebody has handed in your %s at the gate. It is back where it is kept."),*NameOf(L.Item)),6);
  UE_LOG(LogTemp,Display,TEXT("S47: left behind: %s handed in"),*L.Item);
  M->RequestAutosave();
 }
 // Gone back to where it happened, in daylight (critic round 5): the trace, once.
 if(bPlace&&M->Player&&Here(Place))
 {
  const double Hour=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;
  if(PlaceLooked(Hour,FVector2D(M->Player->Pos.X-Place.At.X,M->Player->Pos.Z-Place.At.Z).Size(),Clock-Place.Clock))
  {
   bPlace=false;M->Freeform->LostTimeTrace(1);
   if(M->UI)M->UI->Toast(FString::Printf(TEXT("This is where it happened, at %s. Noted in your field records (TAB)."),*Place.Title),6);
   UE_LOG(LogTemp,Display,TEXT("S47: left behind: the place gone back to in daylight"));
   M->RequestAutosave();
  }
 }
 // The area shown changes under it; cheap enough to follow.
 static_cast<void>(Dt);
 if(AnyLeft())Layout();
}
FString FS47LeftGear::TaskLine() const
{
 if(Left.IsEmpty())return FString();
 TArray<FString> Names;for(const FLeft& L:Left)Names.Add(NameOf(L.Item));
 return FString::Printf(TEXT("LEFT BEHIND / Your %s, at %s. Go and get it, or wait for somebody to hand it in."),*FString::Join(Names,TEXT(" and ")),*Left[0].Title);
}
bool FS47LeftGear::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s left gear: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 FS47LeftGear None(nullptr);None.Begin(nullptr);
 Check(None.Save()->Values.Num()==0&&!None.AnyLeft()&&None.TaskLine().IsEmpty(),TEXT("an older save or an untaken player has nothing out there and writes no field"));
 const auto Make=[](const TCHAR* Item,const TCHAR* Area,double Clock)->TSharedPtr<FJsonValue>
 {
  auto X=S47J::New();S47J::Set(X,TEXT("item"),Item);S47J::Set(X,TEXT("area"),Area);S47J::Set(X,TEXT("title"),TEXT("the church"));S47J::Set(X,TEXT("clock"),Clock);S47J::Set(X,TEXT("at"),TArray<double>{1,2,3});
  return MakeShared<FJsonValueObject>(X);
 };
 auto Saved=S47J::New();TArray<TSharedPtr<FJsonValue>> List={Make(Kinds[0],TEXT("road"),100),Make(Kinds[1],TEXT("road"),100),Make(TEXT("ghost"),TEXT("road"),100),Make(Kinds[0],TEXT("road"),200)};Saved->SetArrayField(TEXT("left"),List);
 FS47LeftGear Two(nullptr);Two.Begin(S47J::Read(S47J::Write(Saved)));
 Check(Two.IsLeft(Kinds[0])&&Two.IsLeft(Kinds[1])&&Two.Items().Num()==2&&Two.Items()[0].At.Equals(FVector(1,2,3))&&Two.Items()[0].Clock==100,TEXT("what lies out there survives a Continue, one of each, and nonsense in a save is dropped"));
 Check(Two.TaskLine().Contains(TEXT("field set and camera"))&&Two.TaskLine().Contains(TEXT("the church")),TEXT("the task says what and where"));
 FS47LeftGear Back(nullptr);Back.Begin(S47J::Read(S47J::Write(Two.Save())));Check(S47J::Write(Back.Save())==S47J::Write(Two.Save()),TEXT("saving it twice changes nothing"));
 // Critic round 5: a taking with nothing left behind keeps its place, and only a look in daylight, after waking, counts.
 FS47LeftGear Spot(nullptr);Spot.Begin(nullptr);Spot.MarkPlace(TEXT("road"),FVector(4,5,6),TEXT("the church"),90000);
 FS47LeftGear SpotBack(nullptr);SpotBack.Begin(S47J::Read(S47J::Write(Spot.Save())));
 Check(SpotBack.PlaceMarked()&&!SpotBack.AnyLeft()&&SpotBack.TaskLine().IsEmpty()&&S47J::Write(SpotBack.Save())==S47J::Write(Spot.Save()),TEXT("the place of a taking survives a Continue, with nothing to fetch"));
 Check(PlaceLooked(10,3,6*3600)&&!PlaceLooked(2,3,6*3600)&&!PlaceLooked(10,PlaceReach+1,6*3600)&&!PlaceLooked(10,0,60),TEXT("the place counts when gone back to in daylight, not at night, from afar, or on waking"));
 return OK;
}
