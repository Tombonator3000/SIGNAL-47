#include "S47Happenings.h"
#include "S47Audio.h"
#include "S47CB.h"
#include "S47Docs.h"
#include "S47Drive.h"
#include "S47EventPlaces.h"
#include "S47FarmSite.h"
#include "S47FieldCamera.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformWorld.h"
#include "S47Glimpses.h"
#include "S47Herd.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47Night.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Rhythm.h"
#include "S47RoadTerrain.h"
#include "S47Seen.h"
#include "S47SkyVisitors.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectGlobals.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47Happen,Log,All);

// Everything below keeps three's terms (metres, y up, z south) as the rest of the game does; road-local is the land's one frame
// (S47OpenLand), and a site is drawn in whichever area the player is in.
namespace S47Hap
{
 const TCHAR* const Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
 const TCHAR* const Cyl=TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
 const TCHAR* const Ball=TEXT("/Engine/BasicShapes/Sphere.Sphere");
 const TCHAR* const Cone=TEXT("/Engine/BasicShapes/Cone.Cone");
 const TCHAR* const BaseMaterial=TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque");
 const TCHAR* const CowBody=TEXT("/Game/Signal47/Art/Cows/SM_Cow_Alive_Body.SM_Cow_Alive_Body");
 const TCHAR* const CowHead=TEXT("/Game/Signal47/Art/Cows/SM_Cow_Alive_Head.SM_Cow_Alive_Head");
 // Tom's own Tripo models (TRIPO-NEW-20261108), not downloaded yet: each its one path here, and what stands in for it meanwhile.
 // Change a path when the model is in /Game/Licensed; nothing else has to move. A humorous one with neither skips itself.
 const TCHAR* const RvModel=TEXT("/Game/Licensed/TripoNew/SM_Tripo_Motorhome.SM_Tripo_Motorhome"); // stand-in: basic shapes (RvParts)
 const TCHAR* const ConvertibleModel=TEXT("/Game/Licensed/TripoNew/SM_Tripo_PinkConvertible.SM_Tripo_PinkConvertible");
 const TCHAR* const ConvertibleStandIn=TEXT("/Game/Free/Vehicles/el_alamo_78/StaticMeshes/el_alamo_78.el_alamo_78"); // painted pink
 const TCHAR* const SingerModel=TEXT("/Game/Licensed/TripoNew/SM_Tripo_Singer.SM_Tripo_Singer");
 const TCHAR* const SingerStandIn=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_souvenir_man.SM_Tripo_souvenir_man"); // in white
 const TCHAR* const HairyModel=TEXT("/Game/Licensed/TripoNew/SM_Tripo_Bigfoot.SM_Tripo_Bigfoot");
 const TCHAR* const HairyStandIn=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_shadow_man.SM_Tripo_shadow_man"); // brown, a head taller
 const TCHAR* const ZetModel=TEXT("/Game/Licensed/TripoNew/SM_Tripo_Zet.SM_Tripo_Zet"); // stand-in: basic shapes (ZetParts)
 const TCHAR* const HitcherModel=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_radio_woman.SM_Tripo_radio_woman");
 const TCHAR* const DriverModel=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_patron.SM_Tripo_patron");
 const TCHAR* const RigModel=TEXT("/Game/Licensed/TripoFleet/heavy_truck/SM_TripoFleet_HeavyTruck.SM_TripoFleet_HeavyTruck");
 const TCHAR* const RigStandIn=TEXT("/Game/Free/Vehicles/muteki_taiga/StaticMeshes/muteki_taiga.muteki_taiga");

 bool Exists(const TCHAR* Path){return Path&&*Path&&FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path)));}
 const TCHAR* Either(const TCHAR* Own,const TCHAR* StandIn){return Exists(Own)?Own:Exists(StandIn)?StandIn:nullptr;}
 double HourOf(double Clock){return FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;}
 FString ClockText(double Clock){const int32 S=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));return FString::Printf(TEXT("%02d:%02d"),S/3600,(S/60)%60);}
 FString Compass(double Az)
 {
  static const TCHAR* const Names[]={TEXT("north"),TEXT("north-east"),TEXT("east"),TEXT("south-east"),TEXT("south"),TEXT("south-west"),TEXT("west"),TEXT("north-west")};
  return Names[int32(FMath::FloorToDouble((FMath::Fmod(Az+360.,360.)+22.5)/45.))%8];
 }
 uint32 SaltOf(const FString& Id){return FCrc::StrCrc32(*Id);}
 constexpr uint32 Bit(ES47PlaceTrait T){return uint32(T);}
 // What the stores and the land call them in the log: never a name, never Story's far side.
 const TCHAR* const FarSide[]={TEXT("forty-seven"),TEXT("forty seven"),TEXT("47"),TEXT("tomas"),TEXT("tomás"),TEXT("silhouette"),TEXT("cable"),TEXT("kinski"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("abduct"),TEXT("8.15"),TEXT("the event"),TEXT("elvis"),TEXT("bigfoot"),TEXT("sasquatch"),TEXT("heisenberg"),TEXT("cadillac"),TEXT("dale")};
 bool Clean(const FString& Text){const FString L=Text.ToLower();for(const TCHAR* W:FarSide)if(L.Contains(W))return false;return true;}

 // A piece of something drawn: three's terms from the thing's own foot, before its yaw. Size: metres for the basic shapes, a scale for
 // a model. Colour 0 keeps a model's own materials.
 struct FPiece{const TCHAR* Mesh;FVector At;FVector Size;double Yaw,Pitch,Roll;uint32 Colour;float Glow;bool bHit;};
 struct FLampDef{FVector At;uint32 Colour;float Lumens;float Radius;};

 // Seven head standing close in a ring, every one facing -z (the herd's own way round: S47Herd, yaw 0 faces -z).
 TArray<FPiece> CattleParts()
 {
  TArray<FPiece> Out;
  for(int32 I=0;I<7;++I)
  {
   const double A=I*2.*UE_DOUBLE_PI/7.+.3,R=4.6+.5*FMath::Sin(I*2.3);
   const FVector C(FMath::Cos(A)*R,0,FMath::Sin(A)*R);
   const double Lean=.05*FMath::Sin(I*1.7);
   Out.Add({CowBody,C,FVector(1),Lean,0,0,0,0,true});
   Out.Add({CowHead,C+S47Herd::NECK,FVector(1),Lean,.07,0,0,0,true});
  }
  return Out;
 }
 // Scorched ground about nine yards across, four marks pressed into it, dead beetles and a lizard inside, and the brush burnt brown.
 TArray<FPiece> RingParts()
 {
  const uint32 Burnt=0x1b1712,Ash=0x5a544a,Pale=0xb8ad94,Bug=0x16140f,Dry=0x6b4a2a;
  TArray<FPiece> Out={
   {Cyl,FVector(0,.012,0),FVector(8.6,.02,8.6),0,0,0,Burnt,0,true},
   {Cyl,FVector(0,.018,0),FVector(6.4,.02,6.4),0,0,0,Pale,0,true},
   {Cyl,FVector(.3,.022,-.2),FVector(2.2,.015,2.2),0,0,0,Ash,0,false},
  };
  const FVector Feet[4]={FVector(-2.1,0,-1.8),FVector(2.1,0,-1.9),FVector(2.2,0,2.0),FVector(-2.0,0,2.1)};
  for(int32 I=0;I<4;++I)Out.Add({Cube,Feet[I]+FVector(0,.02,0),FVector(.55,.03,.75),.15*I,0,0,Burnt,0,true});
  for(int32 I=0;I<9;++I){const double A=I*.83+.4,R=.9+(I%4)*.55;Out.Add({Ball,FVector(FMath::Cos(A)*R,.03,FMath::Sin(A)*R),FVector(.045,.025,.06),A,0,0,Bug,0,false});}
  Out.Add({Cube,FVector(1.3,.03,.6),FVector(.05,.03,.22),.7,0,0,Bug,0,false});
  for(int32 I=0;I<6;++I){const double A=I*1.05+.2,R=3.3+(I%2)*.4;Out.Add({Cone,FVector(FMath::Cos(A)*R,.25,FMath::Sin(A)*R),FVector(.6,.5,.6),A,0,.12,Dry,0,false});}
  return Out;
 }
 // An old camper from basic shapes until Tom's model is in: cream with a brown stripe, its windows lit from inside.
 TArray<FPiece> RvParts()
 {
  const uint32 Cream=0xd8ccb0,Brown=0x6a4a2e,Dark=0x1c1a18,Lit=0xffd59a,Chrome=0x9a9890;
  return {
   {Cube,FVector(0,1.75,.4),FVector(2.4,2.5,6.2),0,0,0,Cream,0,true},
   {Cube,FVector(0,1.05,-3.25),FVector(2.3,1.5,1.5),0,0,0,Cream,0,true},
   {Cube,FVector(0,.95,-3.95),FVector(2.2,.55,.12),0,0,0,Chrome,0,false},
   {Cube,FVector(0,1.65,-3.62),FVector(2.0,.6,.6),0,-.5,0,Dark,0,false},
   {Cube,FVector(0,1.45,.4),FVector(2.42,.22,6.22),0,0,0,Brown,0,false},
   {Cube,FVector(1.21,2.15,-.9),FVector(.02,.55,1.1),0,0,0,Lit,3.5f,false},
   {Cube,FVector(1.21,2.15,1.6),FVector(.02,.55,1.0),0,0,0,Lit,3.5f,false},
   {Cube,FVector(-1.21,2.15,.2),FVector(.02,.55,1.6),0,0,0,Lit,3.5f,false},
   {Cube,FVector(1.215,1.4,.45),FVector(.02,1.8,.7),0,0,0,Brown,0,true},
   {Cube,FVector(1.22,1.95,.45),FVector(.02,.35,.4),0,0,0,Lit,2.5f,false},
   {Cyl,FVector(1.0,.42,-2.9),FVector(.84,.3,.84),0,0,UE_DOUBLE_PI/2,Dark,0,false},
   {Cyl,FVector(-1.0,.42,-2.9),FVector(.84,.3,.84),0,0,UE_DOUBLE_PI/2,Dark,0,false},
   {Cyl,FVector(1.0,.42,2.1),FVector(.84,.3,.84),0,0,UE_DOUBLE_PI/2,Dark,0,false},
   {Cyl,FVector(-1.0,.42,2.1),FVector(.84,.3,.84),0,0,UE_DOUBLE_PI/2,Dark,0,false},
  };
 }
 // A small grey man in a short-sleeved shirt and a hat, from basic shapes until Tom's Zet is in.
 TArray<FPiece> ZetParts()
 {
  const uint32 Grey=0x8c938d,Shirt=0xc8743a,Trousers=0x3a3d44,Black=0x050505,Straw=0xc9b27a;
  return {
   {Cyl,FVector(-.09,.3,0),FVector(.1,.6,.1),0,0,0,Trousers,0,true},
   {Cyl,FVector(.09,.3,0),FVector(.1,.6,.1),0,0,0,Trousers,0,true},
   {Cube,FVector(0,.78,0),FVector(.36,.38,.22),0,0,0,Shirt,0,true},
   {Cyl,FVector(-.24,.74,0),FVector(.07,.42,.07),0,0,.15,Grey,0,false},
   {Cyl,FVector(.24,.74,0),FVector(.07,.42,.07),0,0,-.15,Grey,0,false},
   {Ball,FVector(0,1.14,0),FVector(.34,.42,.34),0,0,0,Grey,0,true},
   {Ball,FVector(-.075,1.16,.15),FVector(.09,.05,.04),0,0,-.3,Black,0,false},
   {Ball,FVector(.075,1.16,.15),FVector(.09,.05,.04),0,0,.3,Black,0,false},
   {Cyl,FVector(0,1.36,0),FVector(.44,.02,.44),0,0,0,Straw,0,false},
   {Cyl,FVector(0,1.42,0),FVector(.26,.12,.26),0,0,0,Straw,0,false},
  };
 }
 TArray<FPiece> HandsetParts()
 {
  const uint32 Wood=0x5b4733,Black=0x101010;
  return {
   {Cube,FVector(0,.75,0),FVector(.12,1.5,.12),0,0,0,Wood,0,true},
   {Cube,FVector(.1,1.42,0),FVector(.04,.02,.02),0,0,0,Black,0,false},
   {Cube,FVector(.13,1.2,0),FVector(.012,.4,.012),0,0,0,Black,0,false},
   {Cube,FVector(.13,.98,0),FVector(.06,.11,.035),0,0,0,Black,0,true},
  };
 }
 // The passing over the truck, and the lights that come down: lamps, each with its own point light.
 TArray<FPiece> GlowParts(double Size,int32 Count,double Spread,uint32 Colour,float Glow)
 {
  TArray<FPiece> Out;
  for(int32 I=0;I<Count;++I)
  {
   const double A=I*2.*UE_DOUBLE_PI/FMath::Max(1,Count);
   Out.Add({Ball,Count>1?FVector(FMath::Cos(A)*Spread,0,FMath::Sin(A)*Spread):FVector::ZeroVector,FVector(Size),0,0,0,Colour,Glow,false});
  }
  return Out;
 }
}

// What is drawn for one kind in one area: built once and kept, shown and posed as the night needs it.
struct FS47Happenings::FProp
{
 FString Kind,Area,UseId;
 struct FPart{TWeakObjectPtr<UStaticMeshComponent> C;FVector At=FVector::ZeroVector,Size=FVector(1);double Yaw=0,Pitch=0,Roll=0;bool bHit=false;};
 TArray<FPart> Parts;
 struct FLamp{TWeakObjectPtr<UPointLightComponent> L;FVector At=FVector::ZeroVector;float Lumens=0;};
 TArray<FLamp> Lamps;
 TArray<FS47Obj> Hits;
 FVector Foot=FVector::ZeroVector;double Yaw=0;bool bShown=false,bPosed=false;
};

FS47Happenings::FS47Happenings(US47Main* Main):M(Main){}
// The scene owns what was drawn, and may be gone already: nothing in the world is touched here.
FS47Happenings::~FS47Happenings(){Life.Reset();Alive.Reset();}

const TArray<FString>& FS47Happenings::Ids()
{
 static const TArray<FString> L={TEXT("car_dies"),TEXT("clock_ahead"),TEXT("cattle_ring"),TEXT("cb_stranded"),TEXT("cb_echo"),TEXT("landing"),TEXT("hitchhiker"),TEXT("stranded_rv"),TEXT("humor")};
 return L;
}
bool FS47Happenings::IsDanger(const FString& Id){return Id==TEXT("cb_echo")||Id==TEXT("hitchhiker");}

// ---------------------------------------------------------------- saving
void FS47Happenings::Begin(const S47J::FObj& Saved)
{
 Stop();Life=MakeShared<uint8>(0);
 Now=FActive();Sites.Reset();LastDays.Reset();Deeds.Reset();Plan.Reset();Says.Reset();GeigerNoted.Reset();
 Serial=0;HumorNight=-100;EchoFrom=0;LateNight=0;PlanNight=0;AheadNight=0;AheadSeconds=0;bLate=false;bEchoArmed=false;HumorKind.Empty();LastArea.Empty();
 DimLeft=0;bDimmed=false;Dose=0;Needle=0;TickDebt=0;GeigerUpFor=0;GeigerNoteIn=0;bGeigerUp=false;bGeigerHeld=false;bDevGeiger=false;
 if(Saved.IsValid())
 {
  const S47J::FObj Last=S47J::Obj(Saved,TEXT("last"));
  for(const FString& Id:Ids()){const int32 D=S47J::Int(Last,*Id,0);if(D>=1&&D<100000)LastDays.Add(Id,D);}
  Serial=FMath::Clamp(S47J::Int(Saved,TEXT("serial")),0,1000000);
  HumorNight=S47J::Int(Saved,TEXT("humor"),-100);
  bEchoArmed=S47J::Bool(Saved,TEXT("echo"));EchoFrom=FMath::Max(0,S47J::Int(Saved,TEXT("echoFrom")));
  AheadNight=FMath::Max(0,S47J::Int(Saved,TEXT("aheadNight")));AheadSeconds=FMath::Clamp(S47J::Num(Saved,TEXT("ahead")),0.,7200.);
  for(const FString& D:S47J::Strs(Saved,TEXT("deeds")))if(!D.IsEmpty()&&D.Len()<200&&Deeds.Num()<6)Deeds.Add(D);
  for(const auto& V:S47J::Arr(Saved,TEXT("sites")))
  {
   if(!V.IsValid()||V->Type!=EJson::Object)continue;
   const S47J::FObj O=V->AsObject();
   FSite S;S.Kind=S47J::Str(O,TEXT("kind"));S.Variant=S47J::Str(O,TEXT("variant"));S.Place=S47J::Str(O,TEXT("place"));S.Title=S47J::Str(O,TEXT("title"));
   S.Night=S47J::Int(O,TEXT("night"));S.Serial=S47J::Int(O,TEXT("serial"));S.X=S47J::Num(O,TEXT("x"));S.Z=S47J::Num(O,TEXT("z"));S.Yaw=S47J::Num(O,TEXT("yaw"));
   S.Az=S47J::Num(O,TEXT("az"),-1);S.Spread=S47J::Num(O,TEXT("spread"));S.Until=S47J::Num(O,TEXT("until"));
   S.bFound=S47J::Bool(O,TEXT("found"));S.bDone=S47J::Bool(O,TEXT("done"));S.bHeard=S47J::Bool(O,TEXT("heard"));
   const bool bKind=S.Kind==TEXT("landing")||S.Kind==TEXT("truck")||S.Kind==TEXT("rv")||S.Kind==TEXT("cattle");
   if(bKind&&S.Night>0&&FMath::IsFinite(S.X)&&FMath::IsFinite(S.Z)&&FMath::Abs(S.X)<1e5&&FMath::Abs(S.Z)<1e5&&FMath::IsFinite(S.Until)&&Sites.Num()<16)Sites.Add(S);
  }
 }
 if(M&&IsValid(M->Web.Get())&&Scene.Get()!=M->Web.Get()){Props.Reset();Scene=M->Web;}
}

S47J::FObj FS47Happenings::Save() const
{
 auto O=S47J::New();
 if(!LastDays.IsEmpty()){auto L=S47J::New();for(const auto& KV:LastDays)S47J::Set(L,*KV.Key,KV.Value);S47J::Set(O,TEXT("last"),L);}
 if(Serial>0)S47J::Set(O,TEXT("serial"),Serial);
 if(HumorNight>0)S47J::Set(O,TEXT("humor"),HumorNight);
 if(bEchoArmed){S47J::Set(O,TEXT("echo"),true);S47J::Set(O,TEXT("echoFrom"),EchoFrom);}
 if(AheadSeconds>0){S47J::Set(O,TEXT("aheadNight"),AheadNight);S47J::Set(O,TEXT("ahead"),AheadSeconds);}
 if(!Deeds.IsEmpty())S47J::Set(O,TEXT("deeds"),Deeds);
 if(!Sites.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> List;
  for(const FSite& S:Sites)
  {
   auto X=S47J::New();S47J::Set(X,TEXT("kind"),S.Kind);if(!S.Variant.IsEmpty())S47J::Set(X,TEXT("variant"),S.Variant);if(!S.Place.IsEmpty())S47J::Set(X,TEXT("place"),S.Place);S47J::Set(X,TEXT("title"),S.Title);
   S47J::Set(X,TEXT("night"),S.Night);S47J::Set(X,TEXT("serial"),S.Serial);S47J::Set(X,TEXT("x"),S.X);S47J::Set(X,TEXT("z"),S.Z);S47J::Set(X,TEXT("yaw"),S.Yaw);
   S47J::Set(X,TEXT("az"),S.Az);S47J::Set(X,TEXT("spread"),S.Spread);S47J::Set(X,TEXT("until"),S.Until);
   if(S.bFound)S47J::Set(X,TEXT("found"),true);if(S.bDone)S47J::Set(X,TEXT("done"),true);if(S.bHeard)S47J::Set(X,TEXT("heard"),true);
   List.Add(MakeShared<FJsonValueObject>(X));
  }
  O->SetArrayField(TEXT("sites"),List);
 }
 return O;
}

void FS47Happenings::Stop()
{
 Life.Reset();
 if(bDimmed&&IsValid(M)&&IsValid(M->World)&&!IsEngineExitRequested()){M->World->Trouble.Dim=1;}
 bDimmed=false;DimLeft=0;
 Now=FActive();Says.Reset();
 if(!IsEngineExitRequested()&&!IsGarbageCollecting())HideAll();
}

// ---------------------------------------------------------------- what the rhythm reads
bool FS47Happenings::Live(const FString& Id) const
{
 if(!Now.Id.IsEmpty()&&Now.Id==Id)return true;
 const int32 Tonight=M&&M->Freeform?FS47FreeformSession::NightOf(M->Freeform->Clock()):0;
 if(Id==TEXT("clock_ahead"))return AheadSeconds>0&&AheadNight==Tonight;
 if(Id==TEXT("cattle_ring"))return SiteOf(TEXT("cattle"))!=nullptr;
 if(Id==TEXT("stranded_rv"))return SiteOf(TEXT("rv"))!=nullptr;
 return false;
}
const FString& FS47Happenings::Ground() const{return Now.Id;}
bool FS47Happenings::HumorTonight() const{return M&&M->Freeform&&HumorNight==FS47FreeformSession::NightOf(M->Freeform->Clock());}
void FS47Happenings::FillRhythm(FS47RhythmNow& Out) const
{
 for(const auto& KV:LastDays)Out.LastDays.Add(FName(*KV.Key),KV.Value);
 if(!Now.Id.IsEmpty()&&Out.Busy.IsNone())Out.Busy=FName(*Now.Id);
 Out.bHumorTonight=HumorTonight();
}
double FS47Happenings::DashAhead() const{return M&&M->Freeform&&AheadNight==FS47FreeformSession::NightOf(M->Freeform->Clock())?AheadSeconds:0.;}
FString FS47Happenings::DashClock(const US47Main* Main,const FString& Usual)
{
 if(!Main||!Main->IsFreeform()||!Main->Freeform||!Main->Freeform->Active()||!Main->Freeform->Happenings||Usual.IsEmpty())return Usual;
 const double Ahead=Main->Freeform->Happenings->DashAhead();
 return Ahead>0?S47Text::ClockText(FMath::Fmod(Main->Freeform->Clock()+Ahead,86400.),false):Usual;
}
const FS47Happenings::FSite* FS47Happenings::SiteOf(const FString& Kind) const
{
 for(int32 I=Sites.Num()-1;I>=0;--I)if(Sites[I].Kind==Kind)return &Sites[I];
 return nullptr;
}
FS47Happenings::FSite* FS47Happenings::SiteOf(const FString& Kind)
{
 for(int32 I=Sites.Num()-1;I>=0;--I)if(Sites[I].Kind==Kind)return &Sites[I];
 return nullptr;
}
bool FS47Happenings::HasSite(const FString& Kind) const{return SiteOf(Kind)!=nullptr;}
bool FS47Happenings::SiteHere(const FString& Kind,FVector& Out) const
{
 const FSite* S=SiteOf(Kind);
 return S&&WorldOf(S->X,S->Z,Out);
}

void FS47Happenings::FillMap(FS47MapView& View) const
{
 double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
 for(const FSite& S:Sites)
 {
  if(S.Kind==TEXT("landing")&&S.Az>=0)
  {
   FS47MapSector Sec;Sec.Az=S.Az;Sec.Spread=S.Spread;Sec.Label=FString::Printf(TEXT("L%d"),S.Night);View.Sectors.Add(Sec);
   if(S.bFound){FS47MapPin P;P.East=S.X;P.South=S.Z-SaroZ;P.Label=TEXT("ring");View.Pins.Add(P);}
  }
  else if((S.Kind==TEXT("truck")||S.Kind==TEXT("rv"))&&(S.bHeard||S.bFound)&&!S.bDone)
  {
   FS47MapPin P;P.East=S.X;P.South=S.Z-SaroZ;P.Label=S.Kind==TEXT("truck")?TEXT("CB"):TEXT("camper");P.bHeard=!S.bFound;View.Pins.Add(P);
  }
 }
}

// ---------------------------------------------------------------- the world
bool FS47Happenings::Driving() const{return M&&M->World&&M->World->bDriving;}
bool FS47Happenings::Outdoors() const{return M&&M->Freeform&&!(M->Freeform->Taken&&M->Freeform->Taken->Indoors());}
bool FS47Happenings::RoadOfWorld(const FVector& W,double& X,double& Z) const
{
 return M&&M->Web&&S47OpenLand::RoadOf(M->Web->Area(),W.X,W.Z,X,Z);
}
bool FS47Happenings::PlayerRoad(double& X,double& Z) const
{
 if(!M||!M->Player)return false;
 const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr;
 return RoadOfWorld(D?D->Pos:M->Player->Pos,X,Z);
}
bool FS47Happenings::WorldOf(double RoadX,double RoadZ,FVector& Out) const
{
 if(!M||!M->Web)return false;
 const FString Leg=M->Web->Area();
 if(Leg.IsEmpty()||Leg==TEXT("roswell"))return false;
 Out=S47OpenLand::FromRoad(Leg,RoadX,RoadZ);
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 double Y=Road.Y+S47RoadTerrain::SurfaceY(RoadX,RoadZ);
 if((Leg==TEXT("saro")||Leg==TEXT("motel"))&&S47OpenLand::InSaroCore(RoadX,RoadZ))Y=S47Legs::SaroFloorAt(Out.X,Out.Z);
 // What the player will stand on, once near enough for it to be measured (as FS47Places seats its props).
 if(M->Player&&M->Player->Floor&&FVector2D::Distance(FVector2D(Out.X,Out.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z))<350)
 {
  const double Fl=M->Player->Floor(Out.X,Out.Z);
  if(FMath::IsFinite(Fl))Y=Fl;
 }
 Out.Y=Y;return true;
}
bool FS47Happenings::OnHighway(double& RoadX,double& RoadZ,double& Speed) const
{
 if(!Driving())return false;
 const FS47DriveController* D=M->World->CabDrive();if(!D)return false;
 Speed=D->Speed;
 return RoadOfWorld(D->Pos,RoadX,RoadZ)&&FMath::Abs(RoadX)<9&&RoadZ>-2400&&RoadZ<4200;
}
bool FS47Happenings::CanHearCb() const
{
 if(!M||!M->Web)return false;
 if(Driving())return true;
 if(M->FieldRadio&&M->FieldRadio->Owned())return true;
 // The base set in the control room.
 return M->Web->Area()==TEXT("saro")&&!Outdoors();
}
FVector FS47Happenings::Eye() const{return M&&M->Night?M->Night->GlimpseIn(true).Cam.Pos:FVector::ZeroVector;}
bool FS47Happenings::Seen(const FVector& At,double Range,double Deg) const
{
 if(!M||!M->Night)return false;
 return S47Facing(M->Night->GlimpseIn(true).Cam,At,Range,Deg);
}
FString FS47Happenings::DirectionFrom(const FVector& From,const FVector& To) const
{
 return S47Hap::Compass(FMath::RadiansToDegrees(FMath::Atan2(To.X-From.X,-(To.Z-From.Z))));
}
int32 FS47Happenings::Branches() const
{
 if(!M||!M->Freeform||!M->Freeform->Field)return 0;
 const auto& Wear=M->Freeform->Field->ArrayWear();
 return Wear.bActive?Wear.Restored.Num():0;
}

// The one door to where a night's thing happens (TODO ANYWHERE-20261108): the place catalogue when it is there, otherwise the land's own
// places. Same seed, night, kind and salt: the same answer.
FS47Happenings::FSpot FS47Happenings::Where(const FString& Kind,uint32 All,uint32 Any,uint32 Not,double MinM,double MaxM,float FarShare,uint32 Salt,bool bSameArea) const
{
 FSpot S;
 auto* F=M?M->Freeform.Get():nullptr;if(!F)return S;
 if(F->EventPlaces)
 {
  FS47PlaceNeed N;N.Kind=Kind;N.All=ES47PlaceTrait(All);N.Any=ES47PlaceTrait(Any);N.Not=ES47PlaceTrait(Not);N.MinM=MinM;N.MaxM=MaxM;N.FarShare=FarShare;N.bSameArea=bSameArea;
  const FS47PlacePick P=F->EventPlaces->Pick(N,Salt);
  if(P.IsSet())if(const FS47EventPlace* E=F->EventPlaces->Find(P.Id))
  {
   S.Place=E->Id;S.Title=E->Title;S.X=E->X;S.Z=E->Z;S.bSet=true;
   UE_LOG(LogS47Happen,Display,TEXT("S47: happening %s: place %s (%s), %.0f m, roll %.3f%s"),*Kind,*E->Id,*E->Title,P.DistanceM,P.Roll,P.bFar?TEXT(", far"):TEXT(""));
   return S;
  }
 }
 // Until the catalogue answers: open land only, from FS47Places (never the motel), as near the need as it allows.
 const uint32 Land=S47Hap::Bit(ES47PlaceTrait::Open)|S47Hap::Bit(ES47PlaceTrait::Far)|S47Hap::Bit(ES47PlaceTrait::Wash)|S47Hap::Bit(ES47PlaceTrait::Farm);
 if(((All|Any)&Land)==0||!F->Places)return S;
 double Px=0,Pz=0;const bool bPlayer=PlayerRoad(Px,Pz);
 TArray<const FS47Places::FDef*> Fit;
 for(const FS47Places::FDef& D:FS47Places::Defs())
 {
  if(FString(D.Kind)==TEXT("motel"))continue;
  double X=D.X,Z=D.Z;F->Places->Spot(D.Id,X,Z);
  const double Far=bPlayer?FVector2D::Distance(FVector2D(X,Z),FVector2D(Px,Pz)):1000.;
  if((MinM>0&&Far<MinM)||(MaxM>0&&Far>MaxM))continue;
  Fit.Add(&D);
 }
 if(Fit.IsEmpty())return S;
 const FS47Places::FDef* D=Fit[int32(FS47NightRhythm::Roll(F->Seed(),FS47FreeformSession::NightOf(F->Clock()),S47Hap::SaltOf(Kind)^Salt)*Fit.Num())%Fit.Num()];
 S.Place=FString();S.Title=D->Title;S.X=D->X;S.Z=D->Z;F->Places->Spot(D->Id,S.X,S.Z);S.bSet=true;
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening %s: no catalogue answer, the land's own place %s"),*Kind,D->Id);
 return S;
}
void FS47Happenings::UsedPlace(const FString& Place,const FString& Kind) const
{
 if(!Place.IsEmpty()&&M&&M->Freeform&&M->Freeform->EventPlaces)M->Freeform->EventPlaces->Used(Place,Kind);
}

// ---------------------------------------------------------------- what is drawn
FS47Happenings::FProp* FS47Happenings::FindProp(const FString& Kind) const
{
 const FString Here=M&&M->Web?M->Web->Area():FString();
 for(const auto& P:Props)if(P->Kind==Kind&&P->Area==Here)return P.Get();
 return nullptr;
}
FS47Happenings::FProp* FS47Happenings::Prop(const FString& Kind)
{
 if(FProp* Have=FindProp(Kind))return Have;
 if(!M||!IsValid(M->Web.Get())||!M->Web->IsLoaded()||!IsValid(M->Inter)||IsEngineExitRequested()||IsGarbageCollecting())return nullptr;
 AS47Web* W=M->Web.Get();const FString Here=W->Area();
 using S47Hap::FPiece;using S47Hap::FLampDef;using S47Hap::Exists;using S47Hap::Either;using S47Hap::Cube;using S47Hap::Cyl;using S47Hap::BaseMaterial;
 using S47Hap::RvModel;using S47Hap::RigModel;using S47Hap::RigStandIn;using S47Hap::DriverModel;using S47Hap::HitcherModel;using S47Hap::ConvertibleModel;using S47Hap::ConvertibleStandIn;
 using S47Hap::SingerModel;using S47Hap::SingerStandIn;using S47Hap::HairyModel;using S47Hap::HairyStandIn;using S47Hap::ZetModel;
 using S47Hap::CattleParts;using S47Hap::RingParts;using S47Hap::RvParts;using S47Hap::HandsetParts;using S47Hap::GlowParts;using S47Hap::ZetParts;
 TArray<FPiece> Spec;TArray<FLampDef> Lamps;FString UseId;
 if(Kind==TEXT("cattle")){Spec=CattleParts();UseId=TEXT("ff:hap:cattle");}
 else if(Kind==TEXT("ring")){Spec=RingParts();UseId=TEXT("ff:hap:ring");}
 else if(Kind==TEXT("rv"))
 {
  if(Exists(RvModel))Spec.Add({RvModel,FVector::ZeroVector,FVector(1),0,0,0,0,0,true});else Spec=RvParts();
  Lamps.Add({FVector(0,2.1,.3),0xffcf8a,9000.f,1400.f});UseId=TEXT("ff:hap:rv");
 }
 else if(Kind==TEXT("rig"))
 {
  if(const TCHAR* Rig=Either(RigModel,RigStandIn))Spec.Add({Rig,FVector::ZeroVector,FVector(1),0,0,0,0,0,true});
  else Spec.Add({Cube,FVector(0,1.6,0),FVector(2.5,3.2,8.5),0,0,0,0x7a2c22,0,true});
  Lamps.Add({FVector(1.3,1.1,-3.2),0xffa23a,2500.f,700.f});Lamps.Add({FVector(-1.3,1.1,-3.2),0xffa23a,2500.f,700.f});
  UseId=TEXT("ff:hap:truck");
 }
 else if(Kind==TEXT("driver")){if(!Exists(DriverModel))return nullptr;Spec.Add({DriverModel,FVector::ZeroVector,FVector(1),0,0,0,0,0,true});UseId=TEXT("ff:hap:truck");}
 else if(Kind==TEXT("handset")){Spec=HandsetParts();UseId=TEXT("ff:hap:truck");}
 else if(Kind==TEXT("hitcher")){if(!Exists(HitcherModel))return nullptr;Spec.Add({HitcherModel,FVector::ZeroVector,FVector(1),0,0,0,0,0,true});UseId=TEXT("ff:hap:hitcher");}
 else if(Kind==TEXT("passer")){Spec=GlowParts(3.2,1,0,0xe8f4ea,40.f);Spec.Add({Cyl,FVector(0,-.4,0),FVector(9,.35,9),0,0,0,0x2a2d2e,0,false});Lamps.Add({FVector(0,-3,0),0xd9f2e4,1500000.f,16000.f});}
 else if(Kind==TEXT("lander")){Spec=GlowParts(7,3,7,0xfff1d6,60.f);Lamps.Add({FVector(0,-2,0),0xffe6c2,3000000.f,30000.f});}
 else if(Kind==TEXT("car"))
 {
  const TCHAR* Car=Either(ConvertibleModel,ConvertibleStandIn);if(!Car)return nullptr;
  Spec.Add({Car,FVector::ZeroVector,FVector(1),0,0,0,Car==ConvertibleModel?0u:0xe58fb4u,0,true});UseId=TEXT("ff:hap:humor");
 }
 else if(Kind==TEXT("singer"))
 {
  const TCHAR* Man=Either(SingerModel,SingerStandIn);if(!Man)return nullptr;
  Spec.Add({Man,FVector::ZeroVector,FVector(1),0,0,0,Man==SingerModel?0u:0xeeeae0u,0,true});UseId=TEXT("ff:hap:humor");
 }
 else if(Kind==TEXT("hairy"))
 {
  const TCHAR* Big=Either(HairyModel,HairyStandIn);if(!Big)return nullptr;
  Spec.Add({Big,FVector::ZeroVector,Big==HairyModel?FVector(1):FVector(1.3),0,0,0,Big==HairyModel?0u:0x3d2b1cu,0,false});
 }
 else if(Kind==TEXT("geiger"))
 {
  // Local to the instrument: x right, y up, z toward the eye. The needle is last (PoseGeiger turns it).
  const uint32 Olive=0x3c4636,Face=0xe6dfc4,Red=0xa8322a,Black=0x101010,Probe=0x2a2a2a;
  Spec={{Cube,FVector(0,0,0),FVector(.15,.09,.07),0,0,0,Olive,0,false},{Cube,FVector(0,.008,.0362),FVector(.112,.058,.003),0,0,0,Face,.6f,false},
   {Cube,FVector(.036,.03,.0378),FVector(.026,.007,.002),0,0,-.7,Red,.4f,false},{Cyl,FVector(-.1,-.02,0),FVector(.026,.13,.026),0,0,UE_DOUBLE_PI/2,Probe,0,false}};
  for(int32 I=0;I<5;++I){const double A=FMath::Lerp(1.05,-1.05,I/4.);Spec.Add({Cube,FVector(-FMath::Sin(A)*.043,-.018+FMath::Cos(A)*.043,.0378),FVector(.002,.007,.002),0,0,A,Black,0,false});}
  Spec.Add({Cyl,FVector(0,-.018,.039),FVector(.008,.004,.008),0,UE_DOUBLE_PI/2,0,Black,0,false});
  Spec.Add({Cube,FVector(0,0,.0392),FVector(.0025,.042,.002),0,0,0,Black,0,false});
 }
 else if(Kind==TEXT("zet"))
 {
  if(Exists(ZetModel))Spec.Add({ZetModel,FVector::ZeroVector,FVector(1),0,0,0,0,0,true});else Spec=ZetParts();
  UseId=TEXT("ff:hap:humor");
 }
 else return nullptr;
 auto* Base=LoadObject<UMaterialInterface>(nullptr,BaseMaterial);
 const int32 Parent=Here==TEXT("saro")||Here==TEXT("motel")?W->Handle(TEXT("ext.group")):Here==TEXT("road")||Here==TEXT("diner")?W->Handle(TEXT("world.legs.road.area.group")):-1;
 auto Made=MakeUnique<FProp>();FProp& P=*Made;P.Kind=Kind;P.Area=Here;P.UseId=UseId;
 TMap<uint64,UMaterialInstanceDynamic*> Paint;
 for(const FPiece& Pc:Spec)
 {
  if(!Exists(Pc.Mesh))continue;
  auto* Mesh=LoadObject<UStaticMesh>(nullptr,Pc.Mesh,nullptr,LOAD_NoWarn|LOAD_Quiet);if(!Mesh)continue;
  const FName Name=MakeUniqueObjectName(W,UStaticMeshComponent::StaticClass(),FName(*FString::Printf(TEXT("S47Hap_%s_%s_%d"),*Kind,*Here,P.Parts.Num())));
  auto* C=NewObject<UStaticMeshComponent>(W,Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(W->GetRootComponent());C->SetStaticMesh(Mesh);
  C->SetCastShadow(Pc.Glow<=0&&Kind!=TEXT("geiger"));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);
  if((Pc.Colour!=0||Pc.Glow>0)&&Base)
  {
   const uint64 Key=(uint64(Pc.Colour)<<32)|uint64(FMath::RoundToInt(Pc.Glow*100.f));
   UMaterialInstanceDynamic* Mat=Paint.FindRef(Key);
   if(!Mat)
   {
    Mat=UMaterialInstanceDynamic::Create(Base,W);
    Mat->SetVectorParameterValue(TEXT("Color"),S47Three::Hex(Pc.Colour));Mat->SetScalarParameterValue(TEXT("Roughness"),Kind==TEXT("car")?.35f:.88f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);
    Mat->SetVectorParameterValue(TEXT("Emissive"),S47Three::Hex(Pc.Colour));Mat->SetScalarParameterValue(TEXT("EmissivePower"),Pc.Glow);
    Paint.Add(Key,Mat);
   }
   for(int32 I=0;I<FMath::Max(1,C->GetNumMaterials());++I)C->SetMaterial(I,Mat);
  }
  C->SetVisibility(false);C->RegisterComponent();
  FProp::FPart Part;Part.C=C;Part.At=Pc.At;Part.Size=Pc.Size;Part.Yaw=Pc.Yaw;Part.Pitch=Pc.Pitch;Part.Roll=Pc.Roll;Part.bHit=Pc.bHit&&!UseId.IsEmpty();
  if(Part.bHit&&Parent>=0)
  {
   C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
   const FS47Obj Hit=W->ObjAt(W->AdoptNode(C,Parent,Name.ToString()));
   if(Hit){W->MarkUsable(Hit.I,UseId);W->SetVisible(Hit.I,false);P.Hits.Add(Hit);}
  }
  P.Parts.Add(Part);
 }
 for(const FLampDef& L:Lamps)
 {
  auto* Lc=NewObject<UPointLightComponent>(W,MakeUniqueObjectName(W,UPointLightComponent::StaticClass(),FName(*FString::Printf(TEXT("S47HapLamp_%s_%s"),*Kind,*Here)))) ;
  Lc->SetupAttachment(W->GetRootComponent());Lc->SetMobility(EComponentMobility::Movable);Lc->SetIntensityUnits(ELightUnits::Lumens);Lc->SetIntensity(0);
  Lc->SetAttenuationRadius(L.Radius);Lc->SetCastShadows(false);Lc->SetLightColor(S47Three::Hex(L.Colour));Lc->SetVisibility(false);Lc->RegisterComponent();
  FProp::FLamp La;La.L=Lc;La.At=L.At;La.Lumens=L.Lumens;P.Lamps.Add(La);
 }
 if(P.Parts.IsEmpty()&&P.Lamps.IsEmpty()){UE_LOG(LogS47Happen,Warning,TEXT("S47: happening prop %s: nothing could be loaded"),*Kind);return nullptr;}
 if(!UseId.IsEmpty()&&!P.Hits.IsEmpty())
 {
  const TWeakPtr<uint8> Guard=Alive;
  FS47Interactable I;I.Id=UseId;I.Object=P.Hits[0];I.Range=UseId==TEXT("ff:hap:truck")||UseId==TEXT("ff:hap:rv")?4.5:3.2;
  I.Label=[this,Guard,UseId]()->FString
  {
   if(!Guard.IsValid()||!M||!M->Freeform)return FString();
   if(UseId==TEXT("ff:hap:ring"))return SiteOf(TEXT("landing"))?TEXT("Scorched ground / look closer"):FString();
   if(UseId==TEXT("ff:hap:rv"))return SiteOf(TEXT("rv"))?TEXT("Camper / look inside"):FString();
   if(UseId==TEXT("ff:hap:cattle"))return SiteOf(TEXT("cattle"))?TEXT("The cattle / look closer"):FString();
   if(UseId==TEXT("ff:hap:hitcher"))return Now.Id==TEXT("hitchhiker")&&Now.Stage<=1?TEXT("The woman / ask where she's going"):FString();
   if(UseId==TEXT("ff:hap:humor"))return Now.Id!=TEXT("humor")?FString():Now.Variant==TEXT("car")?TEXT("Pink convertible / look"):Now.Variant==TEXT("singer")?TEXT("The singer / listen"):Now.Variant==TEXT("pump")?TEXT("The small man / talk"):FString();
   if(UseId==TEXT("ff:hap:truck"))
   {
    const FSite* S=SiteOf(TEXT("truck"));if(!S||S->bDone)return FString();
    return S->Variant==TEXT("wrong")?TEXT("Handset on a fence post / look"):S->Variant==TEXT("empty")?TEXT("Stuck rig / look in the cab"):TEXT("Stuck rig / help dig him out (half an hour)");
   }
   return FString();
  };
  I.Use=[this,Guard,UseId]
  {
   if(!Guard.IsValid())return;
   if(UseId==TEXT("ff:hap:ring"))UseSite(TEXT("landing"));
   else if(UseId==TEXT("ff:hap:rv"))UseSite(TEXT("rv"));
   else if(UseId==TEXT("ff:hap:truck"))UseSite(TEXT("truck"));
   else if(UseId==TEXT("ff:hap:cattle"))UseCattle();
   else if(UseId==TEXT("ff:hap:hitcher"))UseHitcher();
   else if(UseId==TEXT("ff:hap:humor"))UseHumor();
  };
  I.Examine=I.Label;
  M->Inter->Add(I);
 }
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening prop %s built in %s: %d parts, %d lamps%s"),*Kind,*Here,P.Parts.Num(),P.Lamps.Num(),Parent<0&&!UseId.IsEmpty()?TEXT(", nothing to use in this area"):TEXT(""));
 Props.Add(MoveTemp(Made));
 return Props.Last().Get();
}
void FS47Happenings::PoseProp(FProp& P,const FVector& Foot,double Yaw)
{
 if(P.bPosed&&P.Foot.Equals(Foot,.01)&&FMath::IsNearlyEqual(P.Yaw,Yaw,.001))return;
 P.Foot=Foot;P.Yaw=Yaw;P.bPosed=true;
 const FQuat Turn=S47Three::Euler(0,Yaw,0);
 for(FProp::FPart& Pt:P.Parts)
 {
  UStaticMeshComponent* C=Pt.C.Get();if(!IsValid(C))continue;
  const FQuat Q=Turn*S47Three::Euler(Pt.Pitch,Pt.Yaw,Pt.Roll,TEXT("YXZ"));
  double E[16];S47Three::Compose(Foot+Turn.RotateVector(Pt.At),Q,Pt.Size,E);
  C->SetWorldTransform(S47Three::MatToUE(E));
 }
 for(FProp::FLamp& L:P.Lamps)if(UPointLightComponent* Lc=L.L.Get())Lc->SetWorldLocation(S47Three::ToUE(Foot+Turn.RotateVector(L.At)));
}
void FS47Happenings::ShowProp(FProp& P,bool bOn)
{
 if(!M||!IsValid(M->Web.Get()))return;
 const bool bHere=bOn&&M->Web->Area()==P.Area;
 if(P.bShown==bHere)return;
 P.bShown=bHere;
 for(const FS47Obj& H:P.Hits)if(H.W==M->Web.Get()&&H)M->Web->SetVisible(H.I,bHere);
 for(FProp::FPart& Pt:P.Parts)if(UStaticMeshComponent* C=Pt.C.Get())
 {
  C->SetVisibility(bHere);
  C->SetCollisionEnabled(bHere&&Pt.bHit?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
 for(FProp::FLamp& L:P.Lamps)if(UPointLightComponent* Lc=L.L.Get()){Lc->SetVisibility(bHere);Lc->SetIntensity(bHere?L.Lumens:0.f);}
}
void FS47Happenings::HideAll()
{
 if(!M||!IsValid(M->Web.Get()))return;
 for(const auto& P:Props)if(P){P->bShown=true;ShowProp(*P,false);}
}

// ---------------------------------------------------------------- the night
double FS47Happenings::HourOfNight(double Clock) const{const double H=S47Hap::HourOf(Clock);return H<12?H+24:H;}
void FS47Happenings::PlanTonight(int32 Night)
{
 Plan.Reset();PlanNight=Night;bLate=false;HumorKind.Empty();
 auto* F=M?M->Freeform.Get():nullptr;if(!F)return;
 const int32 Seed=F->Seed();
 // The night's chance from director.json (per_night), so that it can be set without a build; these numbers when it has none.
 const auto Draw=[&](const TCHAR* Id,uint32 Salt,int32 FirstNight,float Usual,double From,double Span,float AtLeast=0.f)
 {
  if(Night<FirstNight)return;
  const float Chance=FMath::Max(AtLeast,M->Rhythm?M->Rhythm->PerNight(FName(Id),Usual):Usual);
  if(Chance>0&&FS47NightRhythm::Roll(Seed,Night,Salt)<Chance)Plan.Add(Id,From+Span*FS47NightRhythm::Roll(Seed,Night,Salt+1));
 };
 // A first landing is all but sure by the fourth night, so that the counter has something to find.
 const bool bNoLanding=!Sites.ContainsByPredicate([](const FSite& S){return S.Kind==TEXT("landing");})&&!LastDays.Contains(TEXT("landing"));
 // Numbers for feel, from TODO NIGHT31-20261108 (Tom, 8 November): something most nights, the dangerous ones rare.
 Draw(TEXT("car_dies"),0x43415244u,1,.45f,21.,6.5);
 Draw(TEXT("cattle_ring"),0x4341544cu,2,.32f,21.5,4.);
 Draw(TEXT("humor"),0x48554d52u,3,.5f,20.5,4.5);
 // A calm night has no lights, no bodies and no calls that lead anywhere (FS47NightRhythm::Quiet).
 if(!F->QuietNight(Night))
 {
  Draw(TEXT("cb_stranded"),0x43425354u,2,.28f,20.,4.5);
  Draw(TEXT("landing"),0x4c414e44u,2,.24f,22.,5.,bNoLanding&&Night>=4?.6f:0.f);
  Draw(TEXT("hitchhiker"),0x48495443u,3,.2f,25.,4.);
  Draw(TEXT("stranded_rv"),0x52564e54u,3,.14f,19.5,3.);
  if(bEchoArmed&&Night>EchoFrom&&Night<=EchoFrom+3)Draw(TEXT("cb_echo"),0x4543484fu,1,.4f,25.,3.);
 }
 TArray<FString> Parts;for(const auto& KV:Plan)Parts.Add(FString::Printf(TEXT("%s %s"),*KV.Key,*S47Hap::ClockText(FMath::Fmod(KV.Value,24.)*3600.)));
 UE_LOG(LogS47Happen,Display,TEXT("S47: happenings night %d drawn: %s"),Night,Parts.IsEmpty()?TEXT("none"):*FString::Join(Parts,TEXT(", ")));
}
bool FS47Happenings::Due(const FString& Id) const
{
 const double* From=Plan.Find(Id);if(!From||!M||!M->Freeform)return false;
 const double H=HourOfNight(M->Freeform->Clock());
 const int32* Last=LastDays.Find(Id);
 return H>=*From&&H<30&&!(Last&&*Last==M->Freeform->Day());
}
bool FS47Happenings::MayStart(const FString& Id)
{
 return !M||!M->Rhythm||M->Rhythm->Allow(FName(*Id));
}
void FS47Happenings::Say(const FString& Text,double In,float Secs){FSay S;S.Text=Text;S.In=In;S.Secs=Secs;Says.Add(S);}
void FS47Happenings::StepSays(double Dt)
{
 if(Says.IsEmpty())return;
 Says[0].In-=Dt;
 if(Says[0].In>0)return;
 const FSay S=Says[0];Says.RemoveAt(0);
 if(M&&M->UI)M->UI->Toast(S.Text,S.Secs);
}
void FS47Happenings::Deed(const FString& Text)
{
 Deeds.Remove(Text);Deeds.Add(Text);
 while(Deeds.Num()>5)Deeds.RemoveAt(0);
}
void FS47Happenings::Cb(const TArray<FString>& Lines,const FString& Voice,bool bWrong)
{
 if(!M)return;
 if(!M->CB.IsValid()&&IsValid(M->Audio))M->CB=MakeShared<FS47CB>(M->Audio);
 TArray<FS47CBLine> Call;for(const FString& L:Lines){FS47CBLine C;C.Text=L;C.Who=Voice;C.By=bWrong?TEXT("CB"):TEXT("CB, a driver");Call.Add(C);}
 TOptional<FVector> At;if(!Driving()&&M->Player)At=M->Player->Pos+FVector(.6,1.1,-.4);
 const TWeakPtr<uint8> Guard=Life;
 const double Took=M->CB.IsValid()&&!M->CB->bBusy?M->CB->Call(Call,At,[this,Guard](const FS47CBLine& Line,double Secs){if(Guard.IsValid()&&M&&M->UI)M->UI->Toast(TEXT("CB: ")+Line.Text,float(Secs)+1.5f);}):0.;
 if(Took>0)return;
 // No sound to play it with: the words, one after another.
 double In=0;for(const FString& L:Lines){Say(TEXT("CB: ")+L,In,float(S47CB::SecsOf(L))+1.5f);In=S47CB::SecsOf(L)+.6;}
}

bool FS47Happenings::StartEvent(const FString& Id,const FString& Variant,bool bForced,FString& Why)
{
 auto* F=M?M->Freeform.Get():nullptr;
 if(!F||!F->Active()||!M->IsFreeform()){Why=TEXT("Freeform only");return false;}
 if(!bForced&&F->InTutorial()){Why=TEXT("the first shift");return false;}
 const bool bGround=Id!=TEXT("clock_ahead")&&Id!=TEXT("cattle_ring")&&Id!=TEXT("stranded_rv");
 if(bGround&&!Now.Id.IsEmpty()){Why=FString::Printf(TEXT("%s is on"),*Now.Id);return false;}
 bool bOk=false;
 if(Id==TEXT("car_dies"))bOk=BeginCar(bForced,Why);
 else if(Id==TEXT("clock_ahead"))bOk=BeginAhead(bForced,Why);
 else if(Id==TEXT("cattle_ring"))bOk=BeginCattle(bForced,Why);
 else if(Id==TEXT("cb_stranded"))bOk=BeginStranded(Variant,bForced,Why);
 else if(Id==TEXT("cb_echo"))bOk=BeginEcho(bForced,Why);
 else if(Id==TEXT("landing"))bOk=BeginLanding(bForced,Why);
 else if(Id==TEXT("hitchhiker"))bOk=BeginHitcher(bForced,Why);
 else if(Id==TEXT("stranded_rv"))bOk=BeginRv(bForced,Why);
 else if(Id==TEXT("humor"))bOk=BeginHumor(Variant,bForced,Why);
 else Why=TEXT("no such happening");
 if(!bOk)return false;
 LastDays.Add(Id,F->Day());++Serial;
 if(M->Rhythm)M->Rhythm->Began(FName(*Id));
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening start %s%s day %d %s in %s%s"),*Id,Now.Id==Id&&!Now.Variant.IsEmpty()?*FString::Printf(TEXT(" (%s)"),*Now.Variant):TEXT(""),F->Day(),*S47Hap::ClockText(F->Clock()),*M->Web->Area(),bForced?TEXT(", F10"):TEXT(""));
 M->RequestAutosave();
 return true;
}
void FS47Happenings::EndEvent(bool bEvidence)
{
 if(Now.Id.IsEmpty())return;
 const FString Id=Now.Id;
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening end %s after %.0f s%s"),*Id,Now.Age,bEvidence?TEXT(", kept"):TEXT(""));
 for(const TCHAR* K:{TEXT("passer"),TEXT("lander"),TEXT("hitcher"),TEXT("car"),TEXT("singer"),TEXT("hairy"),TEXT("zet")})for(const auto& P:Props)if(P&&P->Kind==K)ShowProp(*P,false);
 if(M&&M->Audio)M->Audio->Stop(TEXT("hapEngine"),.6f);
 Now=FActive();
 if(M&&M->Rhythm)M->Rhythm->Ended(FName(*Id));
 if(M)M->RequestAutosave();
}

void FS47Happenings::Step(double Dt)
{
 auto* F=M?M->Freeform.Get():nullptr;
 if(!F||!F->Active()||!M->IsFreeform()||!M->Web||!M->Player||!Life.IsValid())return;
 if(Scene.Get()!=M->Web.Get()){Props.Reset();Scene=M->Web;}
 if(F->Held()){StepGeiger(0);return;}
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 if(Night!=PlanNight)PlanTonight(Night);
 if(AheadSeconds>0&&AheadNight!=Night){AheadSeconds=0;UE_LOG(LogS47Happen,Display,TEXT("S47: happening clock_ahead over: the dash agrees again"));}
 const FString Area=M->Web->Area();
 if(Area!=LastArea){LastArea=Area;HideAll();for(const auto& P:Props)if(P)P->bPosed=false;}
 StepSays(Dt);
 if(!Now.Id.IsEmpty())StepEvent(Dt);
 StepSites(Dt);
 // The night by its own rules: only where a real game is played (or a test asks for them), never in the first shift.
 if(F->RulesLive()&&!F->InTutorial())
 {
  static const TCHAR* const Order[]={TEXT("hitchhiker"),TEXT("car_dies"),TEXT("cb_echo"),TEXT("cb_stranded"),TEXT("landing"),TEXT("humor"),TEXT("cattle_ring"),TEXT("stranded_rv")};
  for(const TCHAR* Id:Order)
  {
   if(!Due(Id))continue;
   const bool bGround=FString(Id)!=TEXT("cattle_ring")&&FString(Id)!=TEXT("stranded_rv");
   if(bGround&&!Now.Id.IsEmpty())continue;
   FString Why;
   if(StartEvent(Id,FString(),false,Why))break;
  }
  StepLate();
 }
 StepGeiger(Dt);
}

// Tom, 8 November: something every night, also between eleven and one. If nothing has come by half past twelve, the best of these that
// fits where the player is now comes, still asking the rhythm.
void FS47Happenings::StepLate()
{
 auto* F=M->Freeform.Get();
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 const double H=HourOfNight(F->Clock());
 if(H<23||H>=25.5||LateNight==Night)return;
 if(!bLate)
 {
  const bool bOthers=(F->Director&&F->Director->Busy())||(M->Rhythm&&!M->Rhythm->ReadNow().Busy.IsNone())||(M->SkyVisitors&&M->SkyVisitors->FormationData().bActive)||!Now.Id.IsEmpty();
  bool bToday=false;for(const auto& KV:LastDays)bToday|=KV.Value==F->Day()&&KV.Key!=TEXT("clock_ahead");
  if(bOthers||(bToday&&H>=23))bLate=true;
 }
 if(bLate){LateNight=Night;return;}
 if(H<24.5)return;
 LateNight=Night;
 // A calm night (FS47NightRhythm::Quiet) has no lights and no calls that lead anywhere: only the small ones then.
 const bool bQuiet=F->QuietNight(Night);
 for(const TCHAR* Id:{TEXT("car_dies"),TEXT("cattle_ring"),TEXT("landing"),TEXT("cb_stranded"),TEXT("stranded_rv")})
 {
  if(LastDays.FindRef(Id)==F->Day())continue;
  if(bQuiet&&FString(Id)!=TEXT("car_dies")&&FString(Id)!=TEXT("cattle_ring"))continue;
  FString Why;
  if(StartEvent(Id,FString(),false,Why)){UE_LOG(LogS47Happen,Display,TEXT("S47: happenings: nothing between 23:00 and 00:30 on night %d, so %s"),Night,Id);return;}
 }
 UE_LOG(LogS47Happen,Display,TEXT("S47: happenings: nothing between 23:00 and 00:30 on night %d, and nothing of these fits here"),Night);
}

void FS47Happenings::StepEvent(double Dt)
{
 Now.Age+=Dt;
 if(Now.Id==TEXT("car_dies"))StepCar(Dt);
 else if(Now.Id==TEXT("landing"))StepLanding(Dt);
 else if(Now.Id==TEXT("hitchhiker"))StepHitcher(Dt);
 else if(Now.Id==TEXT("humor"))StepHumor(Dt);
 else if(Now.Id==TEXT("cb_stranded")||Now.Id==TEXT("cb_echo"))
 {
  if(Now.Id==TEXT("cb_echo")&&Driving()&&Now.Stage==0&&Now.Age>2.5){Now.Stage=1;M->World->Trouble.Start(ES47Trouble::Flicker,[]{FS47TroubleOpts O;O.Len=2.2;return O;}());}
  if(Now.Age>=Now.Len)EndEvent(true);
 }
 else EndEvent(false);
}

void FS47Happenings::StepSites(double Dt)
{
 static_cast<void>(Dt);
 auto* F=M->Freeform.Get();
 const double Clock=F->Clock();
 // What has had its time goes, but never in front of the player.
 for(int32 I=Sites.Num()-1;I>=0;--I)
 {
  FSite& S=Sites[I];
  if(S.Until>0&&Clock>=S.Until)
  {
   FVector At;double Away=1e9;
   if(WorldOf(S.X,S.Z,At))Away=FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
   if(Away>250||(S.Kind==TEXT("cattle")&&!Seen(At,400,50)))
   {
    UE_LOG(LogS47Happen,Display,TEXT("S47: happening site %s at %s gone"),*S.Kind,*S.Title);
    Sites.RemoveAt(I);M->RequestAutosave();continue;
   }
  }
 }
 PlaceSites();
 // Noticed from where the player is.
 for(FSite& S:Sites)
 {
  FVector At;if(!WorldOf(S.X,S.Z,At))continue;
  const double Away=FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
  const double H=HourOfNight(Clock);
  if(S.Kind==TEXT("cattle")&&!S.bFound&&Away<160&&Seen(At+FVector(0,1,0),170,34))
  {
   S.bFound=true;
   const FString Way=S47Hap::Compass(S.Az);
   const FSite* Ring=SiteOf(TEXT("landing"));
   const bool bToward=Ring&&Ring->Night>=S.Night-3&&S.Variant==TEXT("landing");
   if(M->UI)M->UI->Toast(FString::Printf(TEXT("Cattle out in the dark, standing in a ring, every head turned the same way: %s."),*Way),6);
   F->RecordEvidence(FString::Printf(TEXT("cattle_ring_d%d"),S.Night),FString::Printf(TEXT("At %s, out by %s, seven head of cattle stood close in a ring, every one of them facing %s, about %03.0f degrees. Not a sound from them, and none of them grazing.%s This records what I saw."),*S47Hap::ClockText(Clock),*S.Title,*Way,S.Az,bToward?TEXT(" That is the way the lights came down."):TEXT("")));
   Deed(FString::Printf(TEXT("You stood and looked at the cattle out by %s."),*S.Title));
   UE_LOG(LogS47Happen,Display,TEXT("S47: happening cattle_ring seen at %.0f m, facing %03.0f%s"),Away,S.Az,bToward?TEXT(" toward the landing"):TEXT(""));
   M->RequestAutosave();
  }
  if(S.Kind==TEXT("rv")&&!S.bHeard&&!S.bFound&&(H>=19.5&&H<30))
  {
   if(Outdoors()&&Away<1100&&Seen(At+FVector(0,2,0),1150,26))
   {
    S.bHeard=true;
    if(M->UI)M->UI->Toast(TEXT("Far out on the flat, a camper with every window lit."),6);
    UE_LOG(LogS47Happen,Display,TEXT("S47: happening stranded_rv seen at %.0f m"),Away);
    M->RequestAutosave();
   }
   else if(CanHearCb()&&Now.Id.IsEmpty()&&FS47NightRhythm::Roll(F->Seed(),int32(Clock/600.),0x52564342u)<.25f)
   {
    S.bHeard=true;
    Cb({FString::Printf(TEXT("Somebody's got a camper sitting way out past %s with every light on. Two nights now. Not my business."),*S.Title)},TEXT("man"),false);
    UE_LOG(LogS47Happen,Display,TEXT("S47: happening stranded_rv heard of on the CB"));
    M->RequestAutosave();
   }
  }
 }
}

// Every lasting thing, drawn where it stands when the player is near enough to see it, in this area's terms.
void FS47Happenings::PlaceSites()
{
 const FString Area=M->Web->Area();
 const bool bLand=Area==TEXT("road")||Area==TEXT("diner");
 TSet<FString> Want;
 for(const FSite& S:Sites)
 {
  FVector At;if(!WorldOf(S.X,S.Z,At))continue;
  const double Away=FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
  const bool bSameLand=bLand||S47OpenLand::LegFor(Area,S.X,S.Z)==Area;
  if(S.Kind==TEXT("rv")&&Away<2500)
  {
   // Its windows can be seen from far off; the camper itself only from the land.
   if(FProp* P=Prop(TEXT("rv"))){PoseProp(*P,At,S.Yaw);ShowProp(*P,bSameLand||Away>900);Want.Add(TEXT("rv"));}
   continue;
  }
  if(!bSameLand||Away>900)continue;
  const auto Show=[&](const TCHAR* Kind,const FVector& Foot,double Yaw){if(FProp* P=Prop(Kind)){PoseProp(*P,Foot,Yaw);ShowProp(*P,true);Want.Add(Kind);}};
  if(S.Kind==TEXT("landing"))Show(TEXT("ring"),At,S.Yaw);
  else if(S.Kind==TEXT("cattle"))Show(TEXT("cattle"),At,S.Yaw);
  else if(S.Kind==TEXT("truck")&&!S.bDone)
  {
   if(S.Variant==TEXT("wrong"))Show(TEXT("handset"),At,S.Yaw);
   else
   {
    Show(TEXT("rig"),At,S.Yaw);
    if(S.Variant==TEXT("real"))
    {
     const FQuat Turn=S47Three::Euler(0,S.Yaw,0);FVector Man=At+Turn.RotateVector(FVector(2.6,0,-1.5));
     if(M->Player&&M->Player->Floor){const double Fl=M->Player->Floor(Man.X,Man.Z);if(FMath::IsFinite(Fl))Man.Y=Fl;}
     Show(TEXT("driver"),Man,S.Yaw+1.2);
    }
    // The hazard lamps.
    if(FProp* Rig=FindProp(TEXT("rig")))for(FProp::FLamp& L:Rig->Lamps)if(UPointLightComponent* Lc=L.L.Get())Lc->SetIntensity(FMath::Fmod(M->T,1.1)<.55?L.Lumens:0.f);
   }
  }
 }
 for(const TCHAR* K:{TEXT("ring"),TEXT("cattle"),TEXT("rig"),TEXT("driver"),TEXT("handset"),TEXT("rv")})if(!Want.Contains(K))if(FProp* P=FindProp(K))ShowProp(*P,false);
}

// ---------------------------------------------------------------- car_dies: something goes over the truck
bool FS47Happenings::BeginCar(bool bForced,FString& Why)
{
 const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr;
 if(!D){Why=TEXT("not driving");return false;}
 if(!bForced&&(FMath::Abs(D->Speed)<6||!D->bEngine)){Why=TEXT("not at speed");return false;}
 const double H=S47Hap::HourOf(M->Freeform->Clock());
 if(!bForced&&!(H>=20.5||H<5)){Why=TEXT("not in the dark");return false;}
 if(!bForced&&!MayStart(TEXT("car_dies"))){Why=TEXT("the rhythm says not now");return false;}
 if(!Prop(TEXT("passer"))){Why=TEXT("nothing to draw it with");return false;}
 Now=FActive();Now.Id=TEXT("car_dies");Now.Len=9;Now.bForced=bForced;Now.Serial=Serial+1;
 Now.At=D->Pos;Now.Dir=FVector(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading));
 Now.Clock=M->Freeform->Clock();
 return true;
}
void FS47Happenings::StepCar(double Dt)
{
 static_cast<void>(Dt);
 const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr;
 const FVector Truck=D?D->Pos:Now.At;
 if(D)Now.Dir=FVector(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading));
 // From behind and above, low over the cab, and on ahead: about six seconds.
 const double T=FMath::Clamp(Now.Age/6.,0.,1.);
 const FVector Side=FVector(-Now.Dir.Z,0,Now.Dir.X);
 const FVector At=Truck+Now.Dir*FMath::Lerp(-240.,320.,T)+Side*FMath::Lerp(-18.,24.,T)+FVector(0,FMath::Lerp(90.,55.,T)-30.*FMath::Sin(T*UE_DOUBLE_PI),0);
 if(FProp* P=Prop(TEXT("passer"))){PoseProp(*P,At,0);ShowProp(*P,Now.Age<6.5);}
 auto& Tr=M->World->Trouble;
 if(Now.Stage==0&&Now.Age>1.9)
 {
  Now.Stage=1;if(D)Tr.Start(ES47Trouble::Flicker,[]{FS47TroubleOpts O;O.Len=.8;return O;}());
  if(M->Audio){FS47Sound S;S.Gain=.5f;S.Rate=.42f;S.Lp=380;S.At=At;M->Audio->Sfx(TEXT("thudSoft"),S);}
 }
 if(Now.Stage==1&&Now.Age>2.7)
 {
  Now.Stage=2;Tr.Dim=.02;bDimmed=true;
  if(D)Tr.Start(ES47Trouble::Sputter,[]{FS47TroubleOpts O;O.Len=1.6;return O;}());
  if(M->Audio)M->Audio->Boom(.35f);
  if(M->UI)M->UI->Toast(TEXT("Something goes over, low. The lights die, and the radio with them."),4);
 }
 if(Now.Stage==2&&Now.Age>6.4)
 {
  Now.Stage=3;Tr.Dim=1;bDimmed=false;
  if(D)Tr.Start(ES47Trouble::Flicker,[]{FS47TroubleOpts O;O.Len=.5;return O;}());
 }
 if(Now.Stage==3&&Now.Age>7.4)
 {
  Now.Stage=4;
  if(M->UI)M->UI->Toast(TEXT("The lights come back. The engine never quite quit."),4);
  auto* F=M->Freeform.Get();
  F->RecordEvidence(FString::Printf(TEXT("car_dies_d%d"),F->Day()),FString::Printf(TEXT("On the road at %s the headlights, the dash and the radio went dead for about four seconds while something went over the truck, low. The engine coughed and kept running. Nothing on the road after, and nothing in the sky. This records what happened."),*S47Hap::ClockText(Now.Clock)));
  Deed(TEXT("You were on the road when your lights went out."));
 }
 if(Now.Age>=Now.Len)
 {
  if(bDimmed){Tr.Dim=1;bDimmed=false;}
  const bool bForced=Now.bForced;
  EndEvent(true);
  // Half the time the dash clock is not right afterwards.
  auto* F=M->Freeform.Get();
  if(bForced||FS47NightRhythm::Roll(F->Seed(),F->Day(),0x41484541u)<.5f){FString Why;StartEvent(TEXT("clock_ahead"),FString(),bForced,Why);}
 }
}

// ---------------------------------------------------------------- clock_ahead: forty minutes ahead after a passing
bool FS47Happenings::BeginAhead(bool bForced,FString& Why)
{
 if(!Driving()&&!bForced){Why=TEXT("not in the truck");return false;}
 if(!bForced&&!MayStart(TEXT("clock_ahead"))){Why=TEXT("the rhythm says not now");return false;}
 auto* F=M->Freeform.Get();
 AheadSeconds=40*60;AheadNight=FS47FreeformSession::NightOf(F->Clock());
 const double Clock=F->Clock();
 Say(FString::Printf(TEXT("The dash clock says %s. Your own watch says %s."),*S47Hap::ClockText(Clock+AheadSeconds),*S47Hap::ClockText(Clock)),bForced?.5:6,6);
 F->RecordEvidence(FString::Printf(TEXT("clock_ahead_d%d"),F->Day()),FString::Printf(TEXT("After the lights went over, the clock on the dash read %s when the station's said %s: forty minutes ahead. It kept time from there, forty minutes out, until the morning. This records what I could compare."),*S47Hap::ClockText(Clock+AheadSeconds),*S47Hap::ClockText(Clock)));
 Deed(TEXT("You set your watch by the dash, and it was wrong."));
 return true;
}

// ---------------------------------------------------------------- cattle_ring
bool FS47Happenings::BeginCattle(bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 const FString Area=M->Web->Area();
 // Out on the land: the road's ground, or SARO's own past the fence (the farm lies in SARO's area).
 const bool bLand=Area==TEXT("road")||Area==TEXT("diner")||(Area==TEXT("saro")&&FS47FarmSite::OutOnFoot(Area,FVector2D(M->Player->Pos.X,M->Player->Pos.Z).Size(),Driving(),!Outdoors()));
 if(!bLand&&!bForced){Why=TEXT("not out on the land");return false;}
 if(!Outdoors()&&!bForced){Why=TEXT("indoors");return false;}
 if(SiteOf(TEXT("cattle"))){Why=TEXT("a ring is standing already");return false;}
 using S47Hap::Bit;
 const FSpot S=Where(TEXT("cattle_ring"),0,Bit(ES47PlaceTrait::Farm)|Bit(ES47PlaceTrait::Open),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors)|Bit(ES47PlaceTrait::Array),bForced?0:70,bForced?0:420,0.f,Serial,!bForced);
 double X=0,Z=0;
 if(S.bSet){X=S.X;Z=S.Z;}
 else if(bForced&&PlayerRoad(X,Z)){X+=40;}
 else{Why=TEXT("no open ground near");return false;}
 // Off the yard and the buildings, out in the open: thirty metres from the place, on its own side.
 const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x52494e47u)*2*UE_DOUBLE_PI;
 if(S.bSet){X+=FMath::Cos(A)*30;Z+=FMath::Sin(A)*30;}
 if(bForced)
 {
  // F10: forty metres ahead of the player, so that the line can be seen at once.
  double Px=0,Pz=0;
  if(PlayerRoad(Px,Pz)&&M->Player){X=Px-FMath::Sin(M->Player->Yaw)*45;Z=Pz-FMath::Cos(M->Player->Yaw)*45;}
 }
 if(!bForced&&!MayStart(TEXT("cattle_ring"))){Why=TEXT("the rhythm says not now");return false;}
 if(!Prop(TEXT("cattle"))){Why=TEXT("no cow models");return false;}
 FSite C;C.Kind=TEXT("cattle");C.Place=S.Place;C.Title=S.bSet?S.Title:TEXT("the open ground");C.Night=F->Day();C.Serial=Serial+1;C.X=X;C.Z=Z;
 C.Until=F->Clock()+35*60;
 // Toward the last lights that came down, when there are any; otherwise wherever tonight has them looking.
 const FSite* Ring=SiteOf(TEXT("landing"));
 if(Ring&&Ring->Night>=F->Day()-3){C.Az=FS47Places::AzimuthOf(X,Z,Ring->X,Ring->Z);C.Variant=TEXT("landing");}
 else C.Az=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x53544152u)*360.;
 // The herd's yaw: 0 faces -z (north); a heading of Az degrees east of north.
 C.Yaw=-FMath::DegreesToRadians(C.Az);
 Sites.Add(C);UsedPlace(S.Place,TEXT("cattle_ring"));
 return true;
}
void FS47Happenings::UseCattle()
{
 const FSite* S=SiteOf(TEXT("cattle"));if(!S||!M->UI)return;
 auto* F=M->Freeform.Get();
 const FString Page=FString::Printf(TEXT("They do not move off when you walk in among them. Every eye is on something you cannot see, about %03.0f degrees, %s. One of them is shaking. There is no wind, and no smell of anything but cattle."),S->Az,*S47Hap::Compass(S->Az));
 F->RecordEvidence(FString::Printf(TEXT("cattle_ring_d%d"),S->Night),FString::Printf(TEXT("Seven head of cattle out by %s stood close in a ring, all facing about %03.0f degrees, %s, and did not move off when I walked in among them. This records what I saw."),*S->Title,S->Az,*S47Hap::Compass(S->Az)));
 Deed(FString::Printf(TEXT("You walked in among the cattle out by %s."),*S->Title));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("CATTLE IN A RING"),S47Kit::Col({S47Kit::Text(Page),S47Kit::Small(TEXT("Kept as a field record. Which way, not why."))}),760));
}

// ---------------------------------------------------------------- cb_stranded
bool FS47Happenings::BeginStranded(const FString& Variant,bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 if(!CanHearCb()&&!bForced){Why=TEXT("no CB to hear it on: the truck, the control room or the field radio");return false;}
 if(const FSite* Old=SiteOf(TEXT("truck")))if(!Old->bDone){Why=TEXT("a driver is still waiting out there");return false;}
 using S47Hap::Bit;
 FSpot S=Where(TEXT("cb_stranded"),Bit(ES47PlaceTrait::Far),Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Road)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors),bForced?0:450,0,1.f,Serial);
 if(!S.bSet)S=Where(TEXT("cb_stranded"),0,Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors),bForced?0:450,0,1.f,Serial+7);
 if(!S.bSet){Why=TEXT("nowhere far out to be stuck");return false;}
 if(!bForced&&!MayStart(TEXT("cb_stranded"))){Why=TEXT("the rhythm says not now");return false;}
 FString Kind=Variant;
 if(Kind.IsEmpty())
 {
  const float R=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x56415249u);
  Kind=R<.12f?TEXT("wrong"):R<.45f?TEXT("empty"):TEXT("real");
 }
 Now=FActive();Now.Id=TEXT("cb_stranded");Now.Variant=Kind;Now.bForced=bForced;Now.Place=S.Place;Now.Title=S.Title;Now.Serial=Serial+1;
 const bool bWrong=Kind==TEXT("wrong");
 TArray<FString> Lines={
  FString::Printf(TEXT("Break one-nine, break one-nine. Anybody out toward %s?"),*S.Title),
  TEXT("I've got a rig down in the sand to the axles and no shovel worth the name."),
  bWrong?TEXT("I'll pay for the trouble. I'm not going anywhere. Come on back. Come on back."):TEXT("I'll pay for the trouble. I'm not going anywhere. Come on back.")};
 Cb(Lines,bWrong?TEXT("roswell"):TEXT("man"),bWrong);
 double Secs=0;for(const FString& L:Lines)Secs+=S47CB::SecsOf(L)+.6;
 Now.Len=Secs+3;
 Say(TEXT("On your map: where the driver said he is (TAB)."),Secs+1,5);
 // His rig is a few metres off the place itself, beside it.
 const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x52494753u)*2*UE_DOUBLE_PI;
 FSite T;T.Kind=TEXT("truck");T.Variant=Kind;T.Place=S.Place;T.Title=S.Title;T.Night=F->Day();T.Serial=Serial+1;T.X=S.X+FMath::Cos(A)*14;T.Z=S.Z+FMath::Sin(A)*14;T.Yaw=A+.4;T.bHeard=true;
 // Until noon tomorrow.
 T.Until=(FMath::FloorToDouble((F->Clock()-8*3600)/86400.)+1)*86400.+12*3600.;
 Sites.Add(T);UsedPlace(S.Place,TEXT("cb_stranded"));
 if(bWrong){bEchoArmed=true;EchoFrom=F->Day();}
 return true;
}

bool FS47Happenings::BeginEcho(bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 if(!CanHearCb()){Why=TEXT("no CB to hear it on");return false;}
 if(!bForced&&!MayStart(TEXT("cb_echo"))){Why=TEXT("the rhythm says not now");return false;}
 const FString Did=Deeds.IsEmpty()?FString(TEXT("You were out on the road at night.")):Deeds[int32(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x44454544u)*Deeds.Num())%Deeds.Num()];
 Now=FActive();Now.Id=TEXT("cb_echo");Now.bForced=bForced;Now.bDanger=true;Now.Serial=Serial+1;
 const TArray<FString> Lines={Did,TEXT("You did. Come on back.")};
 if(M->Audio){FS47Sound S;S.Gain=.4f;S.Dest=TEXT("radio");M->Audio->Sfx(M->Audio->Or(TEXT("radio_crackle_short"),TEXT("alienRadio")),S);}
 Cb(Lines,TEXT("roswell"),true);
 double Secs=0;for(const FString& L:Lines)Secs+=S47CB::SecsOf(L)+.6;
 Now.Len=Secs+4;
 F->RecordEvidence(FString::Printf(TEXT("cb_echo_d%d"),F->Day()),FString::Printf(TEXT("At %s the CB opened by itself on nineteen and a voice said, slowly, what I had done: \"%s\" It had it right. When I keyed back there was nobody on the channel. This records what was said."),*S47Hap::ClockText(F->Clock()),*Did));
 bEchoArmed=false;
 return true;
}

// ---------------------------------------------------------------- landing
bool FS47Happenings::BeginLanding(bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 const bool bAtSaro=M->Web->Area()==TEXT("saro");
 if(!bForced&&!Outdoors()&&!bAtSaro){Why=TEXT("indoors, away from the array");return false;}
 using S47Hap::Bit;
 FSpot S=Where(TEXT("landing"),0,Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors)|Bit(ES47PlaceTrait::Array)|Bit(ES47PlaceTrait::People),bForced?250:350,2800,.3f,Serial);
 if(!S.bSet){Why=TEXT("no open ground for it");return false;}
 if(!bForced&&!MayStart(TEXT("landing"))){Why=TEXT("the rhythm says not now");return false;}
 if(!Prop(TEXT("lander"))){Why=TEXT("nothing to draw it with");return false;}
 // On open ground a little off the place's own point, never on top of a building.
 const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x4c4e4441u)*2*UE_DOUBLE_PI;
 const double X=S.X+FMath::Cos(A)*45,Z=S.Z+FMath::Sin(A)*45;
 Now=FActive();Now.Id=TEXT("landing");Now.Place=S.Place;Now.Title=S.Title;Now.X=X;Now.Z=Z;Now.Len=26;Now.bForced=bForced;Now.Serial=Serial+1;Now.Clock=F->Clock();
 double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
 const int32 B=Branches();
 const double Spread=B>0?FS47Places::SpreadFor(B):25.;
 const double Err=(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x4c42455au)-.5)*Spread;
 FSite R;R.Kind=TEXT("landing");R.Place=S.Place;R.Title=S.Title;R.Night=F->Day();R.Serial=Serial+1;R.X=X;R.Z=Z;R.Yaw=A;R.Spread=Spread;
 R.Az=-1;R.Until=F->Clock()+3*86400.;
 Now.Yaw=FMath::Fmod(FS47Places::AzimuthOf(SaroX,SaroZ,X,Z)+Err+360.,360.);
 // Nothing on the map until it has been seen or logged (StepLanding).
 Sites.Add(R);UsedPlace(S.Place,TEXT("landing"));
 return true;
}
void FS47Happenings::StepLanding(double Dt)
{
 static_cast<void>(Dt);
 FVector Ground;
 const bool bHere=WorldOf(Now.X,Now.Z,Ground);
 // Down slowly over twenty seconds from two hundred and fifty metres, a pulse on the ground, and out.
 const double T=FMath::Clamp(Now.Age/20.,0.,1.);
 const double Up=FMath::Lerp(260.,5.,1.-FMath::Square(1.-T));
 const FVector At=Ground+FVector(FMath::Sin(Now.Age*.4)*6,Up,0);
 if(FProp* P=Prop(TEXT("lander")))
 {
  PoseProp(*P,At,Now.Age*.3);
  const bool bOn=bHere&&(Now.Age<20||(Now.Age<23.5&&FMath::Fmod(Now.Age,.6)<.35));
  ShowProp(*P,bOn);
 }
 if(bHere&&!Now.bSeen&&Seen(At,4500,38))
 {
  Now.bSeen=true;
  UE_LOG(LogS47Happen,Display,TEXT("S47: happening landing seen at %.0f m"),FVector::Dist(Eye(),At));
 }
 if(Now.Age<Now.Len)return;
 auto* F=M->Freeform.Get();
 FSite* R=nullptr;for(FSite& S:Sites)if(S.Kind==TEXT("landing")&&S.Serial==Now.Serial)R=&S;
 const bool bLogged=M->Web->Area()==TEXT("saro")&&Branches()>0;
 if(R&&(Now.bSeen||bLogged||Now.bForced))
 {
  R->Az=Now.Yaw;
  const FString Way=bHere?DirectionFrom(Eye(),Ground):S47Hap::Compass(Now.Yaw);
  if(M->UI)M->UI->Toast(Now.bSeen?FString::Printf(TEXT("Out to the %s, lights come down slowly behind the low ground and go out. The bearing is on your map (TAB)."),*Way):FString(TEXT("The array's monitor kicks: something low on the horizon, coming down. The bearing is on your map (TAB).")),7);
  F->RecordEvidence(FString::Printf(TEXT("landing_seen_d%d"),R->Night),FString::Printf(TEXT("At %s lights came down out on the land, about %03.0f degrees from SARO, give or take %.0f, and went out on the ground. This records which way, not what."),*S47Hap::ClockText(Now.Clock),R->Az,R->Spread));
  Deed(TEXT("You watched the lights come down and did not look away."));
 }
 EndEvent(Now.bSeen||bLogged);
}
void FS47Happenings::UseSite(const FString& Kind)
{
 FSite* S=SiteOf(Kind);if(!S||!M->UI||!M->Freeform)return;
 auto* F=M->Freeform.Get();
 TArray<S47Kit::FW> Rows;FString Title;
 const bool bFirst=!S->bFound;S->bFound=true;
 if(Kind==TEXT("landing"))
 {
  Title=TEXT("SCORCHED GROUND");
  Rows.Add(S47Kit::Text(TEXT("A ring of scorched ground about nine yards across, the caliche baked pale under the black. Pressed into it, four marks, square-sided and deeper on the north side, as if something heavy had stood here on four feet. Inside the ring the beetles lie dead on their backs and the creosote is brown on the side that faces the middle. Outside it nothing is touched.")));
  if(GeigerBuilt())Rows.Add(S47Kit::Text(TEXT("COUNTER / It climbs from the edge of the ring and pegs in the middle.")));
  const FString Key=FString::Printf(TEXT("lights_trace_landing_d%d"),S->Night);
  if(!F->HasFinding(Key))F->RecordEvidence(Key,FString::Printf(TEXT("A scorched ring with four marks pressed into it out by %s, dead insects and burnt brush inside it and nothing touched outside. This records what is here."),*S->Title));
  if(S->Az<0){double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);S->Az=FS47Places::AzimuthOf(SaroX,SaroZ,S->X,S->Z);S->Spread=4;}
  Deed(FString::Printf(TEXT("You walked round the burnt ring out by %s."),*S->Title));
 }
 else if(Kind==TEXT("rv"))
 {
  Title=TEXT("THE CAMPER");
  FString Page=TEXT("An old camper a long way from any road, every light on and the battery nearly flat. The door is not locked. Nobody. A sharp chemical smell, like a school lab shut up for the summer, and glassware drying on a towel. On the dinette a college chemistry book lies open face down, the margins full of small, careful handwriting; a short-brimmed hat on the dash. Tyre tracks come in. None go out.");
  if(bFirst&&F->Places)
  {
   const FString Next=F->Places->NextUnknown(F->Day()+S->Serial);
   if(const FS47Places::FDef* D=Next.IsEmpty()?nullptr:FS47Places::Def(Next))
   {
    F->Places->Hear(Next);
    Page+=FString::Printf(TEXT(" Tucked in the book, a page of pencilled bearings with one place circled: %s. It is on your map."),D->Title);
   }
  }
  Rows.Add(S47Kit::Text(Page));
  const FString Key=FString::Printf(TEXT("rv_d%d"),S->Night);
  if(!F->HasFinding(Key))F->RecordEvidence(Key,FString::Printf(TEXT("A camper stood out by %s with every light on and nobody in it: a chemical smell, glassware, a chemistry book full of notes, tracks coming in and none going out. This records what is here."),*S->Title));
  Deed(TEXT("You went into the camper with every light on."));
 }
 else if(Kind==TEXT("truck"))
 {
  if(S->bDone)return;
  if(S->Variant==TEXT("real"))
  {
   Title=TEXT("STUCK RIG");
   F->SpendTime(30*60.);F->Earn(15);
   FString Lead=TEXT("says the CB has been strange out this way lately, and will not say how.");
   if(F->Places)
   {
    const FString Next=F->Places->NextUnknown(F->Day()+S->Serial*3);
    if(const FS47Places::FDef* D=Next.IsEmpty()?nullptr:FS47Places::Def(Next)){F->Places->Hear(Next);Lead=FString::Printf(TEXT("says he saw a light sitting out by %s a few nights back. He did not go and look. It is on your map."),D->Title);}
   }
   Rows.Add(S47Kit::Text(TEXT("He has been at it with a tyre iron. Between the two of you, his jack and a board off the trailer, it comes out in half an hour. He gives you fifteen dollars, will not hear of less, and ")+Lead));
   F->RecordEvidence(FString::Printf(TEXT("cb_helped_d%d"),S->Night),FString::Printf(TEXT("Helped a driver dig his rig out of the sand by %s, after his call on the CB. Paid $15 for the trouble. This records what happened."),*S->Title));
   Deed(TEXT("You dug a man's rig out of the sand."));
   S->bDone=true;
  }
  else if(S->Variant==TEXT("empty"))
  {
   Title=TEXT("EMPTY RIG");
   Rows.Add(S47Kit::Text(TEXT("The rig is down to the axles with the engine idling, the cab light on and the CB set to nineteen, hissing. A thermos on the dash, still warm. Nobody. His boot prints go twenty paces from the door and stop, as if he had stood there a while and then not.")));
   F->RecordEvidence(FString::Printf(TEXT("cb_empty_d%d"),S->Night),FString::Printf(TEXT("The driver who called for help by %s was not with his rig. Engine idling, the CB on, his prints going twenty paces from the door and stopping. This records what is here."),*S->Title));
   Deed(TEXT("You looked in the cab of an empty rig."));
  }
  else
  {
   Title=TEXT("NO RIG");
   Rows.Add(S47Kit::Text(TEXT("There is no rig. There are no tyre tracks and no boot prints. On a fence post at head height somebody has hung a CB handset by its cord. The cord has been cut clean, and the handset is warm.")));
   F->RecordEvidence(FString::Printf(TEXT("cb_wrong_d%d"),S->Night),FString::Printf(TEXT("Nobody by %s, where the call said. No rig, no tracks: a CB handset hung on a fence post by a cut cord, still warm. This records what is here."),*S->Title));
   Deed(TEXT("You took a handset down off a fence post."));
   S->bDone=true;
  }
 }
 if(Rows.IsEmpty())return;
 Rows.Add(S47Kit::Small(bFirst?TEXT("Kept as a field record. It says what is here, not why."):TEXT("Already in your field records.")));
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening site %s (%s) looked at%s"),*Kind,*S->Variant,bFirst?TEXT(", first time"):TEXT(""));
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Col(Rows),760));
 M->RequestAutosave();
}

// ---------------------------------------------------------------- hitchhiker
bool FS47Happenings::BeginHitcher(bool bForced,FString& Why)
{
 double X=0,Z=0,Speed=0;
 if(!OnHighway(X,Z,Speed)){Why=TEXT("not on the highway");return false;}
 if(!bForced&&FMath::Abs(Speed)<8){Why=TEXT("not at speed");return false;}
 if(!S47Hap::Exists(S47Hap::HitcherModel)){Why=TEXT("no figure for her in this build");return false;}
 if(!bForced&&!MayStart(TEXT("hitchhiker"))){Why=TEXT("the rhythm says not now");return false;}
 const FS47DriveController* D=M->World->CabDrive();
 const FVector Fwd(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading));
 const double Dir=Fwd.Z<0?-1.:1.; // road-local z runs south: north-bound is -z
 Now=FActive();Now.Id=TEXT("hitchhiker");Now.bForced=bForced;Now.bDanger=true;Now.Serial=Serial+1;Now.Clock=M->Freeform->Clock();
 // On the right-hand shoulder, a long way ahead, facing the truck.
 Now.X=-Dir*5.2;Now.Z=Z+Dir*(bForced?110.:170.);Now.Yaw=Dir<0?0.:UE_DOUBLE_PI;
 Now.Len=240;
 if(!Prop(TEXT("hitcher"))){Now=FActive();Why=TEXT("no figure for her in this area");return false;}
 return true;
}
void FS47Happenings::StepHitcher(double Dt)
{
 auto* F=M->Freeform.Get();
 FVector Her;const bool bHere=WorldOf(Now.X,Now.Z,Her);
 FProp* P=Prop(TEXT("hitcher"));
 const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr;
 const FVector Truck=D?D->Pos:M->Player->Pos;
 const double Away=bHere?FVector2D::Distance(FVector2D(Her.X,Her.Z),FVector2D(Truck.X,Truck.Z)):1e9;
 const FVector Fwd=D?FVector(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading)):FVector(0,0,-1);
 const double Ahead=bHere?FVector::DotProduct(Her-Truck,Fwd):0;
 const auto Finish=[&](const FString& Toast,const FString& Page,const FString& Did)
 {
  if(M->UI)M->UI->Toast(Toast,6);
  F->RecordEvidence(FString::Printf(TEXT("hitcher_d%d"),F->Day()),Page);
  Deed(Did);
  if(M->Audio){FS47Sound S;S.Gain=.25f;S.Rate=.6f;S.Lp=900;M->Audio->Sfx(TEXT("doorCreak1"),S);}
  EndEvent(true);
 };
 switch(Now.Stage)
 {
 case 0: // standing by the road, her hand out
  if(P){PoseProp(*P,Her,Now.Yaw);ShowProp(*P,bHere);}
  if(D&&Away<20&&FMath::Abs(D->Speed)<1.5)
  {
   Now.Stage=1;Now.Age=0;
   if(M->UI)M->UI->Toast(TEXT("She looks at you through the window. She asked with her hand; she does not say where to."),6);
   UE_LOG(LogS47Happen,Display,TEXT("S47: happening hitchhiker: the truck stopped for her"));
  }
  else if(D&&Ahead<-22&&Away<120)
  {
   // Passed without stopping: in the mirror a moment, and then not.
   Now.Stage=4;Now.Age=0;
   UE_LOG(LogS47Happen,Display,TEXT("S47: happening hitchhiker: passed"));
  }
  else if(Now.Age>Now.Len||(!Driving()&&Now.Age>30))EndEvent(false);
  break;
 case 1: // the truck stopped beside her
  if(P){PoseProp(*P,Her,Now.Yaw);ShowProp(*P,bHere);}
  if(Now.Age>7||Now.bSeen)
  {
   Now.Stage=2;Now.Age=0;
   if(P)ShowProp(*P,false);
   if(M->Audio){FS47Sound S;S.Gain=.4f;S.At=Truck+FVector(1,1,0);M->Audio->Sfx(TEXT("doorMetal"),S);}
   if(M->UI)M->UI->Toast(TEXT("The passenger door opens and shuts. She sits looking ahead. Nothing you say gets an answer."),6);
   Now.At=Truck;
  }
  else if(D&&FMath::Abs(D->Speed)>4&&Away>25){Now.Stage=4;Now.Age=0;}
  break;
 case 2: // beside you, as far as you can tell, until the mirror
  if(P)ShowProp(*P,false);
  if((D&&FVector2D::Distance(FVector2D(Truck.X,Truck.Z),FVector2D(Now.At.X,Now.At.Z))>320)||Now.Age>45)
   Finish(TEXT("In the mirror the seat beside you is empty. The lock is still down."),FString::Printf(TEXT("At %s a woman by the highway asked for a ride with her hand and never spoke. She got in. Further on she was not there, and the door had not opened. A little road dust on the seat, and a damp mark the shape of a hand on the window. This records what happened."),*S47Hap::ClockText(Now.Clock)),TEXT("You stopped for the woman by the road."));
  break;
 case 4: // gone in the mirror
  if(P){PoseProp(*P,Her,Now.Yaw+(Now.Age>.4?UE_DOUBLE_PI:0.));ShowProp(*P,bHere&&Now.Age<1.3);}
  if(Now.Age>2.2)Finish(TEXT("In the mirror the shoulder is empty. There is nothing out there for anyone to stand behind."),FString::Printf(TEXT("At %s a woman stood by the highway with her hand out. I did not stop. In the mirror, a moment later, there was nobody on the shoulder and nowhere for anybody to go. This records what I saw."),*S47Hap::ClockText(Now.Clock)),TEXT("You drove past the woman by the road."));
  break;
 default:EndEvent(false);break;
 }
 static_cast<void>(Dt);
}
void FS47Happenings::UseHitcher()
{
 if(Now.Id!=TEXT("hitchhiker")||Now.Stage>1)return;
 if(M->UI)M->UI->Toast(TEXT("She does not answer. She does not seem to have heard."),4);
 Now.bSeen=true;
}

// ---------------------------------------------------------------- stranded_rv
bool FS47Happenings::BeginRv(bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 if(const FSite* Old=SiteOf(TEXT("rv"))){Why=FString::Printf(TEXT("the camper from night %d is still out there"),Old->Night);return false;}
 if(!bForced&&LastDays.FindRef(TEXT("stranded_rv"))>F->Day()-6){Why=TEXT("too soon after the last one");return false;}
 using S47Hap::Bit;
 FSpot S=Where(TEXT("stranded_rv"),Bit(ES47PlaceTrait::Far),Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::People)|Bit(ES47PlaceTrait::Indoors),bForced?0:600,0,1.f,Serial);
 if(!S.bSet)S=Where(TEXT("stranded_rv"),0,Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors),bForced?0:600,0,1.f,Serial+5);
 if(!S.bSet){Why=TEXT("nowhere far out enough");return false;}
 if(!bForced&&!MayStart(TEXT("stranded_rv"))){Why=TEXT("the rhythm says not now");return false;}
 const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x52565941u)*2*UE_DOUBLE_PI;
 FSite R;R.Kind=TEXT("rv");R.Place=S.Place;R.Title=S.Title;R.Night=F->Day();R.Serial=Serial+1;R.X=S.X+FMath::Cos(A)*35;R.Z=S.Z+FMath::Sin(A)*35;R.Yaw=A;
 R.Until=F->Clock()+2*86400.;
 Sites.Add(R);UsedPlace(S.Place,TEXT("stranded_rv"));
 if(CanHearCb()&&Now.Id.IsEmpty())
 {
  Sites.Last().bHeard=true;
  Cb({FString::Printf(TEXT("Somebody's got a camper sitting way out past %s with every light on. Two nights now. Not my business."),*S.Title)},TEXT("man"),false);
 }
 return true;
}

// ---------------------------------------------------------------- humor
bool FS47Happenings::BeginHumor(const FString& Variant,bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 if(!bForced&&Night-HumorNight<7){Why=TEXT("once a week at most");return false;}
 if(!bForced&&F->Director&&F->Director->DangerTonight()>0){Why=TEXT("not on a night with a danger");return false;}
 // Where: the diner's lot, the motel's pool, the pump in Caliche, or ahead on the highway.
 FString Kind=Variant;
 double Px=0,Pz=0;const bool bPlayer=PlayerRoad(Px,Pz);
 const auto Near=[&](const TCHAR* Region,const TCHAR* IdPart,double Within,FSpot& Out)
 {
  if(!F->EventPlaces||!bPlayer)return false;
  for(const FS47EventPlace& E:F->EventPlaces->All())
  {
   if(!(E.Region==Region||(IdPart&&E.Id.Contains(IdPart))))continue;
   if(FVector2D::Distance(FVector2D(E.X,E.Z),FVector2D(Px,Pz))>Within&&Within>0)continue;
   Out.Place=E.Id;Out.Title=E.Title;Out.X=E.X;Out.Z=E.Z;Out.bSet=true;return true;
  }
  return false;
 };
 FSpot S;double Hx=0,Hz=0,Speed=0;const bool bRoad=OnHighway(Hx,Hz,Speed);
 const double Reach=bForced?0:180;
 if(Kind.IsEmpty())
 {
  if(bRoad&&FMath::Abs(Speed)>8)Kind=TEXT("crossing");
  else if(Near(TEXT("diner"),TEXT("diner"),Reach,S))Kind=TEXT("car");
  else if(Near(TEXT("motel"),TEXT("motel"),Reach,S))Kind=TEXT("singer");
  else if(Near(TEXT("__none"),TEXT("gas"),Reach,S))Kind=TEXT("pump");
  else{Why=TEXT("not at the diner, the motel, the pump or on the highway");return false;}
 }
 else if(Kind==TEXT("crossing")){if(!bRoad){Why=TEXT("not on the highway");return false;}}
 else
 {
  // F10 names the kind and stands the player where it is wanted: the place, when the catalogue has it, is only for its name.
  const bool bThere=Kind==TEXT("car")?Near(TEXT("diner"),TEXT("diner"),Reach,S):Kind==TEXT("singer")?Near(TEXT("motel"),TEXT("motel"),Reach,S):Kind==TEXT("pump")?Near(TEXT("__none"),TEXT("gas"),Reach,S):false;
  if(Kind!=TEXT("car")&&Kind!=TEXT("singer")&&Kind!=TEXT("pump")){Why=TEXT("car, singer, crossing or pump");return false;}
  if(!bThere&&!bForced){Why=FString::Printf(TEXT("not near the place for %s"),*Kind);return false;}
 }
 const TCHAR* PropKind=Kind==TEXT("car")?TEXT("car"):Kind==TEXT("singer")?TEXT("singer"):Kind==TEXT("crossing")?TEXT("hairy"):TEXT("zet");
 if(!Prop(PropKind)){Why=FString::Printf(TEXT("the model for %s is missing, so it skips itself"),*Kind);UE_LOG(LogS47Happen,Display,TEXT("S47: happening humor %s skipped: no model"),*Kind);return false;}
 if(!bForced&&!MayStart(TEXT("humor"))){Why=TEXT("the rhythm says not now");return false;}
 Now=FActive();Now.Id=TEXT("humor");Now.Variant=Kind;Now.bForced=bForced;Now.Place=S.Place;Now.Title=S.Title;Now.Serial=Serial+1;Now.Clock=F->Clock();
 if(Kind==TEXT("crossing"))
 {
  const FS47DriveController* D=M->World->CabDrive();
  const FVector Fwd(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading));
  const double Dir=Fwd.Z<0?-1.:1.;
  Now.X=Hx;Now.Z=Hz+Dir*75;Now.Len=9;Now.Yaw=Dir;
 }
 else
 {
  // Out in front of the place, where it can be seen from the lot.
  const double A=FS47NightRhythm::Roll(F->Seed(),Night,0x48554d41u)*2*UE_DOUBLE_PI;
  Now.X=S.X+FMath::Cos(A)*(Kind==TEXT("pump")?3.:8.);Now.Z=S.Z+FMath::Sin(A)*(Kind==TEXT("pump")?3.:8.);Now.Yaw=A;
  Now.Len=20*60;
  if(bForced&&bPlayer&&M->Player){Now.X=Px-FMath::Sin(M->Player->Yaw)*(Kind==TEXT("car")?9.:5.);Now.Z=Pz-FMath::Cos(M->Player->Yaw)*(Kind==TEXT("car")?9.:5.);}
 }
 HumorNight=Night;
 UsedPlace(S.Place,TEXT("humor"));
 if(Kind==TEXT("car"))
 {
  Say(TEXT("A pink convertible at the diner, top down, the engine running. Nobody in it."),1,6);
  FVector At;if(M->Audio&&WorldOf(Now.X,Now.Z,At)){FS47Sound O;O.Gain=.18f;O.At=At;O.bLoop=true;M->Audio->LoopSfx(TEXT("hapEngine"),TEXT("engineIdle"),O);}
 }
 else if(Kind==TEXT("singer"))Say(TEXT("Somebody by the motel pool is singing, low, to nobody."),1,6);
 else if(Kind==TEXT("pump"))Say(TEXT("A small grey man in a short-sleeved shirt and a hat is standing at the pump."),1,6);
 return true;
}
void FS47Happenings::StepHumor(double Dt)
{
 static_cast<void>(Dt);
 auto* F=M->Freeform.Get();
 FVector At;const bool bHere=WorldOf(Now.X,Now.Z,At);
 const TCHAR* Kind=Now.Variant==TEXT("car")?TEXT("car"):Now.Variant==TEXT("singer")?TEXT("singer"):Now.Variant==TEXT("crossing")?TEXT("hairy"):TEXT("zet");
 FProp* P=Prop(Kind);
 if(Now.Variant==TEXT("crossing"))
 {
  // From the right-hand side to the left in the lights, a stop in the middle to look, and on into the dark.
  const double Side=-Now.Yaw; // the right-hand shoulder, as for the woman
  const double T=Now.Age<2.2?Now.Age/2.2*.5:Now.Age<3.6?.5:FMath::Min(1.,.5+(Now.Age-3.6)/2.2*.5);
  FVector Walk;WorldOf(FMath::Lerp(Side*9.,-Side*9.,T),Now.Z,Walk);
  const double Facing=Now.Age>=2.2&&Now.Age<3.6?(Now.Yaw<0?0.:UE_DOUBLE_PI):(Side>0?-UE_DOUBLE_PI/2:UE_DOUBLE_PI/2);
  if(P){PoseProp(*P,Walk,Facing);ShowProp(*P,bHere&&Now.Age<6.2);}
  if(Now.Stage==0&&Now.Age>6.4)
  {
   Now.Stage=1;
   if(M->UI)M->UI->Toast(TEXT("Something tall and hairy crosses in your lights, stops in the middle of the road, looks at the truck as if it had been caught at something, and goes on into the dark."),7);
   F->RecordEvidence(FString::Printf(TEXT("humor_crossing_d%d"),F->Day()),FString::Printf(TEXT("At %s something tall and hairy crossed the highway in the headlights, stopped in the middle to look at the truck, and went on. It seemed embarrassed. I am writing that down as I saw it."),*S47Hap::ClockText(Now.Clock)));
   Deed(TEXT("You slowed for something crossing the road."));
  }
  if(Now.Age>=Now.Len)EndEvent(true);
  return;
 }
 if(P)
 {
  double Facing=Now.Yaw;
  if(Now.Variant!=TEXT("car")&&bHere){const FVector To=M->Player->Pos-At;Facing=FMath::Atan2(To.X,To.Z);}
  PoseProp(*P,At,Facing);ShowProp(*P,bHere);
 }
 const double Away=bHere?FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z)):1e9;
 if(Now.Stage==0&&Away<12&&bHere)Now.Stage=1;
 // Gone once the player has had their look and gone off, or when its time is up; never in front of them.
 if((Now.Stage>=1&&Away>130)||(Now.Age>Now.Len&&(Away>130||!Seen(At,160,40))))EndEvent(Now.Stage>=2);
}
void FS47Happenings::UseHumor()
{
 if(Now.Id!=TEXT("humor")||!M->UI)return;
 auto* F=M->Freeform.Get();
 const bool bFirst=Now.Stage<2;Now.Stage=2;
 TArray<S47Kit::FW> Rows;FString Title;
 if(Now.Variant==TEXT("car"))
 {
  Title=TEXT("PINK CONVERTIBLE");
  Rows.Add(S47Kit::Text(TEXT("Top down, engine running, the radio on low to something with a lot of horns. Nobody in it, and nobody in the diner who owns it. The keys hang from a ring shaped like a guitar. Fuzzy dice. The seats are warm.")));
  if(bFirst)F->RecordEvidence(FString::Printf(TEXT("humor_car_d%d"),F->Day()),FString::Printf(TEXT("At %s a pink convertible stood running outside the diner with nobody in it and nobody inside who owned it. It was gone when I next looked. This records what I saw."),*S47Hap::ClockText(F->Clock())));
  Deed(TEXT("You looked into the pink car at the diner."));
 }
 else if(Now.Variant==TEXT("singer"))
 {
  Title=TEXT("BY THE POOL");
  Rows.Add(S47Kit::Text(TEXT("A big man in a white suit with a high collar and dark glasses, at night, by a pool with no water in it. He sings to the deep end, slowly, and waves when he sees you. He says the pool sounds better empty. He thanks you for listening and presses five dollars on you before you can say no.")));
  if(bFirst){F->Earn(5);F->RecordEvidence(FString::Printf(TEXT("humor_singer_d%d"),F->Day()),FString::Printf(TEXT("At %s a man in a white suit and dark glasses sang to the empty motel pool, waved, and gave me five dollars for listening. This records what happened."),*S47Hap::ClockText(F->Clock())));}
  Deed(TEXT("You listened to the man singing at the empty pool."));
 }
 else if(Now.Variant==TEXT("pump"))
 {
  Title=TEXT("AT THE PUMP");
  Rows.Add(S47Kit::Text(TEXT("He is very polite. He would like to buy some water, please, if you have any. He holds out a ten-dollar bill so new it squeaks.")));
  const TWeakPtr<uint8> Guard=Life;
  const auto Done=[this,Guard](bool bSold)
  {
   if(!Guard.IsValid()||!M||!M->Freeform||!M->UI)return;
   auto* S=M->Freeform.Get();
   if(bSold&&S->Consume(TEXT("water")))S->Earn(10);else bSold=false;
   M->UI->Close(true);
   M->UI->Toast(bSold?TEXT("He thanks you, tips his hat, and walks off behind the station with the can. There is nothing behind the station."):TEXT("He tips his hat anyway and walks off behind the station. There is nothing behind the station."),7);
   S->RecordEvidence(FString::Printf(TEXT("humor_pump_d%d"),S->Day()),FString::Printf(TEXT("At %s a small grey man in a short-sleeved shirt and a hat wanted to buy water at the pump%s. He walked off behind the station, where there is nothing. This records what happened."),*S47Hap::ClockText(S->Clock()),bSold?TEXT(", and paid ten dollars for a can"):TEXT("")));
   Deed(bSold?TEXT("You sold the small man at the pump a can of water."):TEXT("You told the small man at the pump you had no water."));
   if(Now.Id==TEXT("humor"))EndEvent(true);
  };
  Rows.Add(S47Kit::Button(TEXT("Sell him a can of water ($10)"),[Done]{Done(true);},true,TAttribute<bool>::CreateLambda([this,Guard]{return Guard.IsValid()&&M&&M->Freeform&&M->Freeform->InventoryCount(TEXT("water"))>0;})));
  Rows.Add(S47Kit::Button(TEXT("Tell him you have none"),[Done]{Done(false);}));
 }
 if(Rows.IsEmpty())return;
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening humor %s looked at"),*Now.Variant);
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Col(Rows),720));
 M->RequestAutosave();
}

// ---------------------------------------------------------------- the counter (GEIGER-20261108)
bool FS47Happenings::GeigerBuilt() const
{
 return bDevGeiger||(M&&M->Freeform&&M->Freeform->Field&&M->Freeform->Field->Workshop().IsBuilt(TEXT("geiger")));
}
FString FS47Happenings::GeigerBench(const US47Main* Main)
{
 return Main&&Main->Freeform&&Main->Freeform->HasKit(TEXT("geiger_tube"))?FString():FString(TEXT("the Geiger-Muller tube from the stores (OPS)"));
}
float FS47Happenings::DoseAt(const FVector& Foot,FString& Near,double& NearM) const
{
 float Sum=.015f;Near.Empty();NearM=1e9;
 const FString Area=M&&M->Web?M->Web->Area():FString();
 if(Area.IsEmpty()||Area==TEXT("roswell")||!M->Freeform)return Sum;
 const auto Add=[&](const FVector& At,float Strength,double R0,const FString& What)
 {
  const double D=FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(Foot.X,Foot.Z));
  Sum+=Strength/float(1.+FMath::Square(D/R0));
  if(D<NearM){NearM=D;Near=What;}
 };
 const auto Road=[&](double X,double Z){return S47OpenLand::FromRoad(Area,X,Z);};
 // Where something came down: hot in the middle of the ring.
 for(const FSite& S:Sites)if(S.Kind==TEXT("landing"))Add(Road(S.X,S.Z),1.1f,5.,FString::Printf(TEXT("landing:%d"),S.Serial));
 auto* F=M->Freeform.Get();
 // Marks left at a site by a visit, as the magnetometer has them (FS47Places::StepNeedle): in the frame the player stands in.
 if(F->FarmSite&&F->FarmSite->Armed())Add(F->FarmSite->Centre(),.35f,8.,FString::Printf(TEXT("marks:%d"),F->Day()));
 // The marked ground at SARO's east yard while it lies there.
 if(F->Director&&F->Director->ActiveEvent()==TEXT("landing_trace")&&Area==TEXT("saro"))Add(FVector(12,0,-12),.4f,4.,FString::Printf(TEXT("yard:%d"),F->Day()));
 // Places a bearing has pointed at: a little, and steady.
 if(F->Places)for(const FS47Places::FBearing& B:F->Places->Bearings())
 {
  double X=0,Z=0;
  if(FS47Places::Def(B.Target)&&F->Places->Spot(B.Target,X,Z))Add(Road(X,Z),.22f,10.,TEXT("place:")+B.Target);
 }
 return FMath::Min(Sum,1.5f);
}
void FS47Happenings::StepGeiger(double Dt)
{
 if(!M||!M->Freeform||!M->Player)return;
 const bool bCan=GeigerBuilt()&&Dt>0&&!Driving()&&!M->ModalOpen()&&M->Mode==US47Main::EMode::Play&&!M->bPausedByMenu&&!(M->FieldCamera&&M->FieldCamera->bRaised);
 if(!bCan){Dose=0;GeigerUpFor=0;PoseGeiger(false);return;}
 FString What;double NearM=0;
 Dose=DoseAt(M->Player->Pos,What,NearM);
 // A tube on a dry cell: the odd tick at background, a rattle over burnt ground, a roar where it pegs.
 const double Rate=FMath::Min(70.,.6+55.*Dose);
 TickDebt+=Rate*Dt;int32 Ticks=0;
 while(TickDebt>=1&&Ticks<10)
 {
  TickDebt-=1;++Ticks;
  if(M->Audio)M->Audio->Beep(2200.f+FMath::FRand()*1800.f,.0025f,.035f+.03f*FMath::Min(1.f,Dose),FMath::FRand()*float(Dt),TEXT("square"));
 }
 TickDebt=FMath::Min(TickDebt,3.);
 const float Want=FMath::Clamp(float(FMath::LogX(10.,1.+60.*Dose)/FMath::LogX(10.,61.)),0.f,1.f);
 Needle+=(Want-Needle)*FMath::Min(1.f,float(Dt)*5.f)+(FMath::FRand()-.5f)*.025f*(1.f+Dose);
 Needle=FMath::Clamp(Needle,0.f,Dose>.85f?1.f:.97f);
 if(Dose>.07f||bGeigerHeld)GeigerUpFor=4;else GeigerUpFor-=Dt;
 PoseGeiger(GeigerUpFor>0);
 GeigerNoteIn-=Dt;
 if(Dose>.12f&&GeigerNoteIn<=0&&M->UI){M->UI->Toast(Dose>.85f?TEXT("COUNTER / Pegged."):TEXT("COUNTER / It's climbing."),3);GeigerNoteIn=Dose>.85f?20.:40.;}
 // What it has read is kept, once each.
 if(Dose<.5f||What.IsEmpty())return;
 auto* F=M->Freeform.Get();
 FString Key,Page;
 if(What.StartsWith(TEXT("landing:")))
 {
  const int32 N=FCString::Atoi(*What.RightChop(8));const FSite* Ring=nullptr;for(const FSite& X:Sites)if(X.Serial==N)Ring=&X;
  Key=FString::Printf(TEXT("geiger_landing_%d"),N);
  Page=FString::Printf(TEXT("The counter at the scorched ring out by %s: it climbs from the edge and pegs in the middle; twenty paces out it is back to the odd tick. This records what it read."),Ring?*Ring->Title:TEXT("the land"));
 }
 else if(What.StartsWith(TEXT("marks:"))){Key=TEXT("geiger_")+What.Replace(TEXT(":"),TEXT("_d"));Page=TEXT("The counter over the marks: well above background, highest in the middle of them. This records what it read.");}
 else if(What.StartsWith(TEXT("yard:"))){Key=TEXT("geiger_")+What.Replace(TEXT(":"),TEXT("_d"));Page=TEXT("The counter over the marked ground in the east yard: above background, falling off within a few paces. This records what it read.");}
 else if(What.StartsWith(TEXT("place:")))
 {
  const FString Id=What.RightChop(6);const FS47Places::FDef* D=FS47Places::Def(Id);
  Key=TEXT("geiger_place_")+Id;
  Page=FString::Printf(TEXT("The counter at %s, where a bearing pointed: a little above background, and steady. This records what it read."),D?D->Title:TEXT("a place"));
 }
 if(Key.IsEmpty()||F->HasFinding(Key))return;
 F->RecordEvidence(Key,Page);
 UE_LOG(LogS47Happen,Display,TEXT("S47: counter reads %.2f at %s, %.0f m: kept as %s"),Dose,*What,NearM,*Key);
 M->RequestAutosave();
}
// Held up in the lower left of the view while it climbs: a case, a cream dial with a red end and a needle. Nothing on the HUD.
void FS47Happenings::PoseGeiger(bool bShow)
{
 if(!M||!M->Player||!IsValid(M->Web.Get()))return;
 FProp* P=bShow?Prop(TEXT("geiger")):FindProp(TEXT("geiger"));
 bGeigerUp=bShow&&P;
 if(!P)return;
 if(!bShow){ShowProp(*P,false);return;}
 const FS47Cam Cam=FS47Cam::Of(M->Player);
 const FVector Base=Cam.Pos+Cam.Forward*.40-Cam.Up*.22-Cam.Right*.17;
 const FQuat Q=Cam.Quaternion()*S47Three::Euler(-.45,.3,0,TEXT("YXZ"));
 const int32 Last=P->Parts.Num()-1;
 for(int32 I=0;I<P->Parts.Num();++I)
 {
  FProp::FPart& Pt=P->Parts[I];UStaticMeshComponent* C=Pt.C.Get();if(!IsValid(C))continue;
  FVector Local=Pt.At;FQuat Turn=S47Three::Euler(Pt.Pitch,Pt.Yaw,Pt.Roll,TEXT("YXZ"));
  if(I==Last)
  {
   // The needle, about its pivot low on the dial: full left at nothing, full right where it pegs.
   const double A=FMath::Lerp(1.05,-1.05,double(Needle));
   const FQuat R=S47Three::Euler(0,0,A,TEXT("XYZ"));
   Local=FVector(0,-.018,.0392)+R.RotateVector(FVector(0,.021,0));Turn=R;
  }
  double E[16];S47Three::Compose(Base+Q.RotateVector(Local),Q*Turn,Pt.Size,E);
  C->SetWorldTransform(S47Three::MatToUE(E));
 }
 ShowProp(*P,true);
}

// ---------------------------------------------------------------- F10
bool FS47Happenings::DevStart(const FString& Id,const FString& Variant,FString& Why)
{
 if(!Ids().Contains(Id)){Why=TEXT("no such happening");return false;}
 const bool bGround=Id!=TEXT("clock_ahead")&&Id!=TEXT("cattle_ring")&&Id!=TEXT("stranded_rv");
 if(bGround&&!Now.Id.IsEmpty())EndEvent(false);
 if(Id==TEXT("cattle_ring"))Sites.RemoveAll([](const FSite& S){return S.Kind==TEXT("cattle");});
 if(Id==TEXT("stranded_rv"))Sites.RemoveAll([](const FSite& S){return S.Kind==TEXT("rv");});
 if(Id==TEXT("cb_stranded"))Sites.RemoveAll([](const FSite& S){return S.Kind==TEXT("truck");});
 if(Id==TEXT("cb_echo")&&Deeds.IsEmpty())Deed(TEXT("You stood out in the yard and looked at the antennas."));
 if(Id==TEXT("landing"))Sites.RemoveAll([](const FSite& S){return S.Kind==TEXT("landing");});
 if(!StartEvent(Id,Variant,true,Why))return false;
 // "ring": what a landing leaves, without waiting for it to come down; its bearing on the map as if it had been seen.
 if(Id==TEXT("landing")&&Variant==TEXT("ring")){Now.bSeen=true;Now.Age=Now.Len;StepLanding(0);}
 return true;
}
bool FS47Happenings::LiveAt(FVector& Out) const
{
 if(Now.Id==TEXT("hitchhiker")||Now.Id==TEXT("humor")||Now.Id==TEXT("landing"))return WorldOf(Now.X,Now.Z,Out);
 if(Now.Id==TEXT("car_dies")){Out=Now.At;return true;}
 return false;
}
void FS47Happenings::DevClear(bool bSites)
{
 if(!Now.Id.IsEmpty())EndEvent(false);
 if(bDimmed&&M&&M->World){M->World->Trouble.Dim=1;bDimmed=false;}
 Says.Reset();
 if(bSites)Sites.Reset();
 HideAll();
}

// ---------------------------------------------------------------- self-test
bool FS47Happenings::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s happenings: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 // Every one of them is in the rhythm's file, under its own system, and the dangerous ones wait past the first night.
 FS47RhythmData D;FString Why;
 FString Text;const bool bFile=FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Director/director.json")));
 const bool bParsed=bFile&&FS47Rhythm::Parse(Text,D,Why);
 if(bParsed)
 {
  bool bAll=true,bDanger=true,bSystem=true;FString Missing;
  for(const FString& Id:Ids())
  {
   const FS47RhythmEvent* E=D.Find(FName(*Id));
   if(!E){bAll=false;Missing+=Id+TEXT(" ");continue;}
   bSystem&=E->System==TEXT("night");
   bDanger&=(E->Level==ES47RhythmLevel::Danger)==IsDanger(Id)&&(!IsDanger(Id)||E->FromDay>=2);
  }
  Check(bAll&&bSystem,TEXT("every one is in director.json under the night system ")+Missing);
  Check(bDanger,TEXT("the woman by the road and the voice that repeats you are dangers, and never on the first night"));
  const FS47RhythmEvent* H=D.Find(TEXT("humor"));
  Check(H&&H->MinNights>=7&&H->Level!=ES47RhythmLevel::Danger,TEXT("humour at most once a week, and it is no danger"));
 }
 else Lines.Add(TEXT("PASS happenings: no director.json beside this build to read (packaged runs read the embedded one)"));
 // Words: English, short, nothing named, nothing from Story's far side.
 const TArray<FString> Said={
  TEXT("Something goes over, low. The lights die, and the radio with them."),
  TEXT("Break one-nine, break one-nine. Anybody out toward the wash?"),
  TEXT("I've got a rig down in the sand to the axles and no shovel worth the name."),
  TEXT("Somebody's got a camper sitting way out past the wash with every light on. Two nights now. Not my business."),
  TEXT("She looks at you through the window. She asked with her hand; she does not say where to."),
  TEXT("In the mirror the seat beside you is empty. The lock is still down."),
  TEXT("A small grey man in a short-sleeved shirt and a hat is standing at the pump."),
  TEXT("Something tall and hairy crosses in your lights, stops in the middle of the road, looks at the truck as if it had been caught at something, and goes on into the dark."),
  TEXT("A big man in a white suit with a high collar and dark glasses, at night, by a pool with no water in it."),
  TEXT("On the dinette a college chemistry book lies open face down, the margins full of small, careful handwriting; a short-brimmed hat on the dash."),
  TEXT("You did. Come on back.")};
 bool bClean=true;for(const FString& S:Said)bClean&=S47Hap::Clean(S)&&S.Len()<260;
 Check(bClean,TEXT("what they say names nobody and keeps to the outside"));
 // The models: Tom's own paths are one constant each, and every humorous one has a stand-in or skips itself.
 Check(FString(S47Hap::RvModel).StartsWith(TEXT("/Game/Licensed/"))&&FString(S47Hap::ConvertibleModel).StartsWith(TEXT("/Game/Licensed/"))&&FString(S47Hap::SingerModel).StartsWith(TEXT("/Game/Licensed/"))&&FString(S47Hap::HairyModel).StartsWith(TEXT("/Game/Licensed/"))&&FString(S47Hap::ZetModel).StartsWith(TEXT("/Game/Licensed/")),TEXT("Tom's own models stay out of Git, under /Game/Licensed, one path each"));
 Check(S47Hap::RvParts().Num()>8&&S47Hap::ZetParts().Num()>6&&S47Hap::RingParts().Num()>10&&S47Hap::CattleParts().Num()==14,TEXT("what stands in until they come is there"));
 // The compass, and a save round trip.
 Check(S47Hap::Compass(0)==TEXT("north")&&S47Hap::Compass(91)==TEXT("east")&&S47Hap::Compass(181)==TEXT("south")&&S47Hap::Compass(-89)==TEXT("west")&&S47Hap::Compass(359)==TEXT("north"),TEXT("a heading in words"));
 {
  FS47Happenings A(nullptr);A.Begin(nullptr);
  Check(A.Save()->Values.Num()==0&&A.Sites.IsEmpty(),TEXT("an older save has none of it and writes nothing"));
  FSite S;S.Kind=TEXT("landing");S.Title=TEXT("the wash");S.Night=4;S.Serial=3;S.X=120;S.Z=-440;S.Az=31;S.Spread=12;S.Until=9e5;A.Sites.Add(S);
  FSite Bad;Bad.Kind=TEXT("nonsense");Bad.Night=2;A.Sites.Add(Bad);
  A.LastDays.Add(TEXT("landing"),4);A.Deeds.Add(TEXT("You walked round the burnt ring out by the wash."));A.bEchoArmed=true;A.EchoFrom=4;A.HumorNight=3;
  FS47Happenings B(nullptr);B.Begin(S47J::Read(S47J::Write(A.Save())));
  Check(B.Sites.Num()==1&&B.Sites[0].Az==31.&&B.Sites[0].Title==TEXT("the wash")&&B.LastDays.FindRef(TEXT("landing"))==4&&B.Deeds.Num()==1&&B.bEchoArmed&&B.EchoFrom==4&&B.HumorNight==3,TEXT("sites, days, deeds and the armed voice survive a Continue, and nonsense is dropped"));
 }
 // The draws are the seed's: the same night draws the same, and something most nights.
 {
  int32 Nights=0,Some=0;
  for(int32 Seed=1;Seed<=40;++Seed)for(int32 Night=2;Night<=31;++Night)
  {
   ++Nights;bool bAny=false;
   for(const auto& Pr:TArray<TPair<uint32,float>>{{0x43415244u,.45f},{0x4341544cu,.32f},{0x43425354u,.28f},{0x4c414e44u,.24f}})bAny|=FS47NightRhythm::Roll(Seed,Night,Pr.Key)<Pr.Value;
   Some+=bAny?1:0;
  }
  Check(Some>Nights*6/10,FString::Printf(TEXT("something of these is drawn on %d of %d nights"),Some,Nights));
 }
 return OK;
}
