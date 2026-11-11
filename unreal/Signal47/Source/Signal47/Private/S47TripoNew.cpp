#include "S47TripoNew.h"
#include "S47EyeGlint.h"
#include "S47Image.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"

namespace S47TripoNewLocal
{
 const TCHAR* const Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
 const TCHAR* const TallStandIn=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_tall_one.SM_Tripo_tall_one");
}

const TArray<S47TripoNew::FModel>& S47TripoNew::Models()
{
 using S47TripoNewLocal::Cube;using S47TripoNewLocal::TallStandIn;
 // Sizes as Tools/triponew/prepare.py makes them. The stand-ins are S47Happenings' own where it has one.
 static const TArray<FModel> All={
  {TEXT("singer"),TEXT("Big man in a white suit (never named)"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Singer.SM_Tripo_Singer"),
   TEXT("/Game/Licensed/TripoNPC/SM_Tripo_souvenir_man.SM_Tripo_souvenir_man"),EKind::Figure,1.83,0xb9b2a4u},
  {TEXT("bigfoot"),TEXT("Something tall and hairy (never named)"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Bigfoot.SM_Tripo_Bigfoot"),
   TEXT("/Game/Licensed/TripoNPC/SM_Tripo_shadow_man.SM_Tripo_shadow_man"),EKind::Figure,2.45,0x3d2b1cu},
  {TEXT("zet"),TEXT("Zet: small, gray, shirt and hat"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Zet.SM_Tripo_Zet"),Cube,EKind::Alien,1.40,0x8c938du},
  {TEXT("nordic"),TEXT("Nordic: tall, pale, fair hair"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Nordic.SM_Tripo_Nordic"),TallStandIn,EKind::Alien,1.96,0xc8c8c8u},
  {TEXT("cosmic"),TEXT("Cosmic: very tall, thin, blue-green"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Cosmic.SM_Tripo_Cosmic"),TallStandIn,EKind::Alien,2.30,0x2f7a78u},
  {TEXT("motorhome"),TEXT("Old motorhome, cream and brown"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Motorhome.SM_Tripo_Motorhome"),Cube,EKind::Vehicle,7.3,0xd8ccb0u},
  {TEXT("convertible"),TEXT("Pink convertible with fins"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_PinkConvertible.SM_Tripo_PinkConvertible"),
   TEXT("/Game/Free/Vehicles/el_alamo_78/StaticMeshes/el_alamo_78.el_alamo_78"),EKind::Vehicle,5.7,0xe58fb4u},
  // PT-WALKIE-20261109 (Tom): his better walkie, the one on the SARO desk (DeskWalkie).
  {TEXT("walkie"),TEXT("Walkie-talkie, Tom's better one: the SARO desk walkie"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Walkie.SM_Tripo_Walkie"),Cube,EKind::Prop,.34,0x1c1d1cu},
  // "pakke tripo3d-1" on Tom's desktop, imported as SM_Tripo_Parcel: a Geiger counter's prototype, the counter in the hand (Geiger).
  {TEXT("geiger"),TEXT("Geiger counter prototype (Tom's \"pakke tripo3d-1\"), held in the hand"),TEXT("/Game/Licensed/TripoNew/SM_Tripo_Parcel.SM_Tripo_Parcel"),Cube,EKind::Prop,.31,0x3c4636u},
 };
 return All;
}

const S47TripoNew::FModel* S47TripoNew::Find(const FString& Id)
{
 for(const FModel& M:Models())if(Id==M.Id)return &M;
 return nullptr;
}

const S47TripoNew::FModel* S47TripoNew::DeskWalkie(){return Find(TEXT("walkie"));}
const S47TripoNew::FModel* S47TripoNew::Geiger(){return Find(TEXT("geiger"));}

// ---------------------------------------------------------------- the Cosmic one's light (PT-NORDIC-COSMIC-20261109)
namespace S47TripoNewLocal
{
 // Both made by the web import (Tools/webexport/import_web.py) and in every build: the unlit additive master (Color, Map with its
 // MapU/MapV transform, Brightness) and the soft round glow point of the glints (Color, Brightness).
 const TCHAR* const OverlayBase=TEXT("/Game/Web/Base/M_Web_Unlit_Add.M_Web_Unlit_Add");
 const TCHAR* const StarBase=TEXT("/Game/Web/Base/M_WebGlow.M_WebGlow");
 constexpr int32 FieldSize=128,FieldStars=110,BodyStars=12;
 // The star field's repeats across the body's map, and how fast it drifts (repeats a second): about one star's width in two.
 constexpr double Tile=2.6,DriftU=.0021,DriftV=-.0013;
 // How bright the overlay is on average (it breathes about this), and a star point's.
 constexpr double Glow=.95,StarGlow=2.4;
 // A star point's size, metres across.
 constexpr double StarM=.035;
 // The field: a faint blue-green light everywhere, small white stars in it, each with a softer blue edge. sRGB, as the map is read.
 UTexture2D* StarField()
 {
  static TWeakObjectPtr<UTexture2D> Have;
  if(Have.IsValid())return Have.Get();
  constexpr int32 N=FieldSize;
  TArray<FColor> Px;Px.Init(FColor(52,118,132,255),N*N);
  FRandomStream R(0x434f534du);
  const auto Put=[&](int32 X,int32 Y,const FColor& C){FColor& P=Px[((Y+N)%N)*N+(X+N)%N];P=FColor(FMath::Max(P.R,C.R),FMath::Max(P.G,C.G),FMath::Max(P.B,C.B),255);};
  for(int32 K=0;K<FieldStars;++K)
  {
   const int32 X=R.RandRange(0,N-1),Y=R.RandRange(0,N-1);
   const bool bBig=R.FRand()<.22f;
   Put(X,Y,FColor(255,255,255,255));
   const FColor Edge=bBig?FColor(170,215,235,255):FColor(110,170,190,255);
   Put(X+1,Y,Edge);Put(X-1,Y,Edge);Put(X,Y+1,Edge);Put(X,Y-1,Edge);
   if(bBig){const FColor Far(80,145,165,255);Put(X+1,Y+1,Far);Put(X-1,Y-1,Far);Put(X+1,Y-1,Far);Put(X-1,Y+1,Far);}
  }
  UTexture2D* T=S47Image::FromPixels(Px,N,N);
  Have=T;
  return T;
 }
}

bool S47TripoNew::IsCosmic(const FString& AssetName){return AssetName==TEXT("SM_Tripo_Cosmic");}

double S47TripoNew::CosmicBreath(double Age)
{
 // A slow breath, about seven seconds, with a slower swell under it: from three quarters to a little over full, never dark.
 return .86+.12*FMath::Sin(Age*2.*UE_DOUBLE_PI/7.)+.05*FMath::Sin(Age*2.*UE_DOUBLE_PI/19.+1.3);
}
double S47TripoNew::CosmicTwinkle(double Age,float Phase)
{
 // Each star on its own clock: mostly lit, now and then a brief flare, now and then near gone.
 const double A=Age*(.9+.5*Phase)+Phase*17.;
 const double S=.55+.3*FMath::Sin(A*2.3)+.15*FMath::Sin(A*5.1+Phase*3.);
 return FMath::Clamp(S,.1,1.);
}

bool S47TripoNew::AddCosmicGlow(UObject* Outer,UStaticMeshComponent* Body,FCosmicGlow& Out)
{
 using namespace S47TripoNewLocal;
 const UStaticMesh* Mesh=Body?Body->GetStaticMesh():nullptr;
 if(!Outer||!Mesh||!IsCosmic(Mesh->GetName()))return false;
 Out=FCosmicGlow();Out.Body=Body;
 if(UMaterialInterface* Base=LoadObject<UMaterialInterface>(nullptr,OverlayBase,nullptr,LOAD_NoWarn|LOAD_Quiet))
  if(UTexture2D* Field=StarField())
  {
   UMaterialInstanceDynamic* D=UMaterialInstanceDynamic::Create(Base,Outer);
   D->SetVectorParameterValue(TEXT("Color"),FLinearColor(1.f,1.f,1.f,1.f));
   D->SetTextureParameterValue(TEXT("Map"),Field);
   D->SetScalarParameterValue(TEXT("UseMap"),1.f);
   D->SetVectorParameterValue(TEXT("MapU"),FLinearColor(float(Tile),0.f,0.f,0.f));
   D->SetVectorParameterValue(TEXT("MapV"),FLinearColor(0.f,float(Tile),0.f,0.f));
   D->SetScalarParameterValue(TEXT("Opacity"),1.f);
   D->SetScalarParameterValue(TEXT("Brightness"),float(Glow*CosmicBreath(0)));
   Body->SetOverlayMaterial(D);
   Out.Skin=D;
  }
 // A few brighter points on and about the body, from the knees to the head, close in round its middle: those in front of it
 // shine, those behind it are hidden by it, and they move round slowly.
 if(UMaterialInterface* Glint=LoadObject<UMaterialInterface>(nullptr,StarBase,nullptr,LOAD_NoWarn|LOAD_Quiet))
 {
  const FBox B=Mesh->GetBoundingBox();
  const FVector C=B.GetCenter(),E=B.GetExtent();
  const double Reach=FMath::Max(4.,FMath::Min(E.X,E.Y)*.55);
  FRandomStream R(0x53544152u);
  for(int32 K=0;K<BodyStars;++K)
  {
   const double A=R.FRand()*2.*UE_DOUBLE_PI,H=FMath::Lerp(B.Min.Z+E.Z*.35,B.Max.Z-E.Z*.04,double(R.FRand()));
   const FVector At(C.X+FMath::Cos(A)*Reach*(.7+.3*R.FRand()),C.Y+FMath::Sin(A)*Reach*(.7+.3*R.FRand()),H);
   UMaterialInstanceDynamic* D=UMaterialInstanceDynamic::Create(Glint,Outer);
   D->SetVectorParameterValue(TEXT("Color"),FLinearColor(.82f,.94f,1.f,1.f));
   D->SetScalarParameterValue(TEXT("Brightness"),float(StarGlow*CosmicTwinkle(0,R.FRand())));
   UMaterialBillboardComponent* S=NewObject<UMaterialBillboardComponent>(Outer);
   S->SetupAttachment(Body);
   const float Half=float(StarM*50.);
   S->AddElement(D,nullptr,false,Half,Half,nullptr);
   S->SetCastShadow(false);S->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   S->RegisterComponent();
   S->SetRelativeLocation(At);
   S->SetVisibility(Body->IsVisible());
   Out.Stars.Add(S);Out.StarAt.Add(At);Out.Phase.Add(R.FRand());
  }
 }
 UE_LOG(LogTemp,Display,TEXT("S47: the Cosmic one shines: overlay %s, %d star points"),Out.Skin.IsValid()?TEXT("on"):TEXT("missing"),Out.Stars.Num());
 return Out.Skin.IsValid()||!Out.Stars.IsEmpty();
}

void S47TripoNew::StepCosmicGlow(FCosmicGlow& G,double Dt)
{
 using namespace S47TripoNewLocal;
 UStaticMeshComponent* Body=G.Body.Get();
 if(!IsValid(Body)||!Body->IsVisible()||!FMath::IsFinite(Dt))return;
 G.Age+=FMath::Clamp(Dt,0.,.25);
 const double Breath=CosmicBreath(G.Age);
 if(UMaterialInstanceDynamic* D=G.Skin.Get())
 {
  D->SetScalarParameterValue(TEXT("Brightness"),float(Glow*Breath));
  // The stars drift: the field slides slowly across the body's map (wrapped, so it never runs out).
  const double U=FMath::Fmod(G.Age*DriftU,1.),V=FMath::Fmod(G.Age*DriftV,1.);
  D->SetVectorParameterValue(TEXT("MapU"),FLinearColor(float(Tile),0.f,float(U),0.f));
  D->SetVectorParameterValue(TEXT("MapV"),FLinearColor(0.f,float(Tile),float(V),0.f));
 }
 for(int32 K=0;K<G.Stars.Num();++K)
 {
  UMaterialBillboardComponent* S=G.Stars[K].Get();
  if(!IsValid(S)||!G.StarAt.IsValidIndex(K)||!G.Phase.IsValidIndex(K))continue;
  // Round the body's axis, slowly, each at its own pace, and a little up and down.
  const FVector Home=G.StarAt[K];
  const double Turn=G.Age*(.05+.04*G.Phase[K])*(K%2?1.:-1.);
  const double Cs=FMath::Cos(Turn),Sn=FMath::Sin(Turn);
  const FVector2D Off(Home.X,Home.Y);
  S->SetRelativeLocation(FVector(Off.X*Cs-Off.Y*Sn,Off.X*Sn+Off.Y*Cs,Home.Z+4.*FMath::Sin(G.Age*.3+G.Phase[K]*6.)));
  if(S->Elements.Num())if(UMaterialInstanceDynamic* D=Cast<UMaterialInstanceDynamic>(S->Elements[0].Material))
   D->SetScalarParameterValue(TEXT("Brightness"),float(StarGlow*Breath*CosmicTwinkle(G.Age,G.Phase[K])));
 }
}

bool S47TripoNew::Exists(const TCHAR* ObjectPath)
{
 return ObjectPath&&*ObjectPath&&FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(ObjectPath)));
}

const TCHAR* S47TripoNew::Resolve(const FModel& Model,bool* bOwn)
{
 const bool bHave=Exists(Model.Path);
 if(bOwn)*bOwn=bHave;
 if(bHave)return Model.Path;
 return Exists(Model.StandIn)?Model.StandIn:S47TripoNewLocal::Cube;
}

bool S47TripoNew::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;auto Check=[&](bool Pass,const TCHAR* Text){Lines.Add(FString(Pass?TEXT("PASS "):TEXT("FAIL "))+Text);OK&=Pass;};
 bool bPrivate=true,bNamed=true,bSized=true;
 for(const FModel& M:Models())
 {
  const FString P(M.Path);
  bPrivate&=P.StartsWith(TEXT("/Game/Licensed/TripoNew/SM_Tripo_"));
  const FString Asset=FPackageName::ObjectPathToObjectName(P);
  bNamed&=!Asset.IsEmpty()&&P.EndsWith(TEXT("/")+Asset+TEXT(".")+Asset);
  bSized&=M.SizeM>.1&&M.SizeM<10.;
 }
 Check(Models().Num()==9&&bPrivate,TEXT("Tripo new: the nine models, every one under /Game/Licensed/TripoNew (out of Git)"));
 Check(bNamed&&bSized,TEXT("Tripo new: each path names its own asset, and each has a size"));
 // S47Happenings waits for these five by name (S47Hap::RvModel ...): the same names here.
 Check(Find(TEXT("singer"))&&FString(Find(TEXT("singer"))->Path).EndsWith(TEXT("SM_Tripo_Singer"))&&FString(Find(TEXT("bigfoot"))->Path).EndsWith(TEXT("SM_Tripo_Bigfoot"))
  &&FString(Find(TEXT("zet"))->Path).EndsWith(TEXT("SM_Tripo_Zet"))&&FString(Find(TEXT("motorhome"))->Path).EndsWith(TEXT("SM_Tripo_Motorhome"))
  &&FString(Find(TEXT("convertible"))->Path).EndsWith(TEXT("SM_Tripo_PinkConvertible")),TEXT("Tripo new: the humor events' five paths are the ones imported"));
 // PT-WALKIE-20261109 (Tom): the walkie on the desk is his better walkie; "pakke tripo3d-1" is the Geiger counter in the hand, and
 // nothing is called a parcel any more.
 Check(DeskWalkie()&&FString(DeskWalkie()->Path).EndsWith(TEXT("SM_Tripo_Walkie"))&&DeskWalkie()->Kind==EKind::Prop&&DeskWalkieHeightM>.2&&DeskWalkieHeightM<.45,
  TEXT("Tripo new: the desk walkie is Tom's better one (SM_Tripo_Walkie), at a hand-held height"));
 Check(Geiger()&&FString(Geiger()->Path).EndsWith(TEXT("SM_Tripo_Parcel"))&&Geiger()->Kind==EKind::Prop&&GeigerHeightM>.1&&GeigerHeightM<.3&&!Find(TEXT("parcel")),
  TEXT("Tripo new: the Geiger counter is Tom's prototype (\"pakke tripo3d-1\", SM_Tripo_Parcel), at a hand-held height, its entry no longer a parcel"));
 // PT-NORDIC-COSMIC-20261109: only the Cosmic body shines; it breathes and its stars twinkle, never out, never a lamp.
 {
  bool bBreath=true,bTwinkle=true;double Lo=9,Hi=-9;
  for(double A=0;A<60;A+=.05){const double B=CosmicBreath(A);Lo=FMath::Min(Lo,B);Hi=FMath::Max(Hi,B);bBreath&=FMath::IsFinite(B)&&B>.6&&B<1.1;
   const double T=CosmicTwinkle(A,float(FMath::Fmod(A*.37,1.)));bTwinkle&=T>=.1&&T<=1.;}
  Check(IsCosmic(TEXT("SM_Tripo_Cosmic"))&&!IsCosmic(TEXT("SM_Tripo_Nordic"))&&!IsCosmic(TEXT("SM_Tripo_Zet"))&&bBreath&&bTwinkle&&Hi-Lo>.15,
   *FString::Printf(TEXT("Tripo new: the Cosmic one alone shines, breathing %.2f to %.2f, its stars twinkling; the Nordic does not"),Lo,Hi));
  FCosmicGlow None;
  Check(!AddCosmicGlow(nullptr,nullptr,None)&&None.Stars.IsEmpty(),TEXT("Tripo new: no body, no glow"));
 }
 // Without the private models (a build from Git alone) each still resolves to something drawable.
 FModel Missing=Models()[0];Missing.Path=TEXT("/Game/Licensed/TripoNew/SM_Tripo_NotThere.SM_Tripo_NotThere");Missing.StandIn=TEXT("/Game/Nowhere/SM_Gone.SM_Gone");
 bool bOwn=true;const TCHAR* Got=Resolve(Missing,&bOwn);
 Check(!bOwn&&Got&&FString(Got)==TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Tripo new: a missing model and a missing stand-in fall back to a basic shape, not to nothing"));
 bool bAll=true;for(const FModel& M:Models())bAll&=Resolve(M)!=nullptr;
 Check(bAll,TEXT("Tripo new: every model resolves to something in this build"));
 // Zet's big black eyes have the dot (S47EyeGlint::ForStatic): either side of the middle, ahead of the pivot, up in the head.
 const S47EyeGlint::FEyes* Zet=S47EyeGlint::ForStatic(TEXT("SM_Tripo_Zet"));
 const double ZetCm=Find(TEXT("zet"))->SizeM*100.;
 Check(Zet&&Zet->Left.X>2.&&Zet->Right.X<-2.&&Zet->Left.Y>5.&&Zet->Right.Y>5.&&Zet->Left.Z>ZetCm*.75&&Zet->Left.Z<ZetCm&&FMath::Abs(Zet->Left.Z-Zet->Right.Z)<3.,
  TEXT("Tripo new: Zet's eyes measured for the glint, in the head, one each side"));
 // The Nordic's eyes are a man's and the Cosmic one has none: no dot on either, nor on the singer or the hairy one.
 Check(!S47EyeGlint::ForStatic(TEXT("SM_Tripo_Nordic"))&&!S47EyeGlint::ForStatic(TEXT("SM_Tripo_Cosmic"))&&!S47EyeGlint::ForStatic(TEXT("SM_Tripo_Singer"))
  &&!S47EyeGlint::ForStatic(TEXT("SM_Tripo_Bigfoot")),TEXT("Tripo new: no glint where there is no black eye (Nordic, Cosmic, the singer, the hairy one)"));
 return OK;
}
