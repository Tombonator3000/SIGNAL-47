#include "S47Places.h"
#include "S47Audio.h"
#include "S47FarmSite.h"
#include "S47Freeform.h"
#include "S47FreeformObservation.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Kit.h"
#include "S47LeftGear.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Relief.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47TownUse.h"
#include "S47MotelNight.h"
#include "S47UI.h"
#include "S47FieldLife.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"
#include "UObject/GarbageCollection.h"

namespace
{
 // Offset and size in metres on the place's own ground: x east, y up, z south. A part's centre, as the basic shapes have it.
 struct FPart{const TCHAR* Mesh;FVector At,Size;uint32 Colour;float Yaw,Roll;};
 const TCHAR* const Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
 const TCHAR* const Cyl=TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
 const TCHAR* const Ball=TEXT("/Engine/BasicShapes/Sphere.Sphere");
 const TCHAR* const Cone=TEXT("/Engine/BasicShapes/Cone.Cone");
 const TCHAR* const CowBody=TEXT("/Game/Signal47/Art/Cows/SM_Cow_Alive_Body.SM_Cow_Alive_Body");
 const TCHAR* const CowHead=TEXT("/Game/Signal47/Art/Cows/SM_Cow_Alive_Head.SM_Cow_Alive_Head");
 // PT-SAUCER-ROCK-20261109: Poly Haven's boulders (CC0, Tools/freemodels, cooked with /Game/Free): pivot at the middle of the base,
 // in centimetres, so a scale of 1 is the rock as it was scanned and the part's height is where its base sits.
 const TCHAR* const RockA=TEXT("/Game/Free/Desert/namaqualand_boulder_04/StaticMeshes/namaqualand_boulder_04.namaqualand_boulder_04"); // 2.5 x 2.5 x 1.9 m
 const TCHAR* const RockB=TEXT("/Game/Free/Desert/namaqualand_boulder_02/StaticMeshes/namaqualand_boulder_02.namaqualand_boulder_02"); // 2.5 x 1.2 x .9 m
 const TCHAR* const RockC=TEXT("/Game/Free/Desert/namaqualand_boulder_03/StaticMeshes/namaqualand_boulder_03.namaqualand_boulder_03"); // 2.4 x 3.1 x 1.5 m
 const TCHAR* const RockD=TEXT("/Game/Free/Desert/namaqualand_boulder_05/StaticMeshes/namaqualand_boulder_05.namaqualand_boulder_05"); // 1.4 x .7 x .5 m
 const TCHAR* const RockE=TEXT("/Game/Free/Desert/namaqualand_rocks_01/StaticMeshes/namaqualand_rocks_01.namaqualand_rocks_01");     // a spill of stones
 bool Have(const TCHAR* Path){return FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path)));}
 TArray<FPart> PartsOf(const FString& Kind)
 {
  const uint32 Steel=0x8b8e8a,Rust=0x7a4b2e,Wood=0x5b4733,Dark=0x151311,Pale=0xd8d4c6,Flag=0xd0641e,Stone=0x8f8778,Dirt=0x7c6348;
  if(Kind==TEXT("windmill"))return {{Cyl,{2.8,.4,1.2},{1.8,.8,1.8},Rust,0,0},{Cube,{-1.1,4.5,-1.1},{.09,9,.09},Steel,0,7},{Cube,{1.1,4.5,-1.1},{.09,9,.09},Steel,0,-7},{Cube,{-1.1,4.5,1.1},{.09,9,.09},Steel,0,7},{Cube,{1.1,4.5,1.1},{.09,9,.09},Steel,0,-7},{Cube,{0,9,0},{.9,.08,.9},Wood,0,0},{Cyl,{0,9.9,-.2},{2.6,.1,2.6},Rust,0,90},{Cube,{0,9.9,.9},{.05,.7,1.5},Steel,0,0}};
  if(Kind==TEXT("tank"))return {{Cyl,{0,1.3,0},{7,2.6,7},Steel,0,0},{Cube,{4,.3,0},{1.4,.6,.5},Rust,0,0},{Cube,{0,2.7,0},{7.2,.05,.3},Rust,30,0}};
  if(Kind==TEXT("mine"))return {{Cube,{-1,1.2,0},{.25,2.4,.25},Wood,0,0},{Cube,{1,1.2,0},{.25,2.4,.25},Wood,0,0},{Cube,{0,2.45,0},{2.5,.28,.28},Wood,0,0},{Cube,{0,1.1,.5},{1.8,2.2,1},Dark,0,0},{Cone,{-4.5,.7,2.5},{6.5,1.4,5},Dirt,0,0},{Cone,{4.5,.9,2},{7.5,1.8,6},Dirt,0,0},{Cube,{0,.1,-1.5},{.9,.1,3},Rust,0,0}};
  if(Kind==TEXT("balloon"))return {{Ball,{0,.1,0},{3,.16,2.2},Pale,20,0},{Ball,{1.8,.08,1.2},{1.6,.1,.9},Pale,70,0},{Cube,{-1.6,.12,-1.3},{.2,.22,.14},Pale,15,8},{Cube,{-.8,.02,-.6},{1.9,.015,.015},Dark,38,0}};
  if(Kind==TEXT("cow"))return {{CowBody,{0,.42,0},{1,1,1},0,30,86},{CowHead,{0,.42,0},{1,1,1},0,30,86}};
  if(Kind==TEXT("stake"))return {{Cube,{0,.65,0},{.05,1.3,.05},Wood,0,2},{Cube,{.13,1.15,0},{.26,.13,.012},Flag,0,0},{Cube,{.6,.04,.3},{.04,.08,.04},Wood,0,0}};
  if(Kind==TEXT("shack"))return {{Cube,{0,1.2,0},{4.5,2.4,3.2},Wood,0,0},{Cube,{0,2.6,0},{5,.12,3.8},Rust,0,8},{Cube,{2.26,1,.2},{.05,2,.9},Dark,0,0},{Cube,{0,1.5,1.61},{.8,.7,.03},Dark,0,0}};
  if(Kind==TEXT("trough"))return {{Cube,{0,.32,0},{3.2,.64,.9},Rust,0,0},{Cube,{0,.62,0},{3,.03,.7},Dark,0,0},{Cube,{2.6,.2,.8},{.4,.4,.4},Pale,12,0},{Cube,{-3,.6,-1},{.1,1.2,.1},Wood,0,5}};
  if(Kind==TEXT("cairn"))return {{Ball,{0,.3,0},{1.5,.7,1.3},Stone,0,0},{Ball,{.2,.8,.1},{1,.55,.9},Stone,40,0},{Ball,{0,1.2,0},{.6,.4,.6},Stone,10,0},{Cube,{.05,1.65,0},{.05,.7,.05},Wood,0,6}};
  // PT-SAUCER-ROCK-20261109 (Tom: at Saucer Rock and nothing there). A pillar of pale rock with a wide flat cap balanced on it, a
  // cup and saucer, seen from the track by day and as a shape against the sky at night; a ring of stones round its foot with the
  // sand swept smooth inside it, a fire ring, cans, and the hand-lettered board the tourists come for. The scanned boulders where
  // the free models are in, basic shapes where they are not. The first part is the one that is used.
  if(Kind==TEXT("saucer rock"))
  {
   const uint32 Sand=0xb9a27f,Can=0xa8a39a,Ash=0x2a2622;
   TArray<FPart> P;
   if(Have(RockA)&&Have(RockB))P={{RockA,{0,-.3,0},{.85,1.75,.85},0,20,0},{RockB,{.25,2.75,-.1},{2.3,.8,3.8},0,35,4}};
   else P={{Cyl,{0,1.45,0},{2.1,3.3,2.1},Stone,0,0},{Ball,{.25,3.2,-.1},{5.8,.75,4.8},Stone,35,4}};
   P.Append({{Cyl,{0,.0,0},{8.2,.03,8.2},Sand,0,0},
    {RockC,{-5.6,-.2,2.8},{.55,.5,.55},0,70,0},{RockD,{4.6,0,-3.4},{1,1,1},0,140,0},{RockE,{-2.8,0,-4.9},{1,1,1},0,15,0},
    {Cube,{6.8,.7,-1.2},{.08,1.4,.08},Wood,0,3},{Cube,{6.8,1.25,-1.17},{.7,.28,.03},Pale,0,3},
    {Cyl,{6,.01,3.8},{1,.02,1},Ash,0,0},{Cyl,{6.3,.06,4.6},{.066,.12,.066},Can,0,0},{Cyl,{5.2,.035,3.1},{.066,.12,.066},Can,0,90}});
   // the ring round its foot: twelve stones, set by hand
   for(int32 I=0;I<12;++I){const double A=I*UE_DOUBLE_PI/6;P.Add({Ball,{4.2*FMath::Cos(A),.1,4.2*FMath::Sin(A)},{.38,.24,.32},Stone,float(I*37%180),0});}
   return P;
  }
  // The Sierra Motor Court's office, as far as a player needs it: a bell and a key rack on the wall by the door.
  if(Kind==TEXT("motel"))return {{Cube,{0,1.05,0},{.5,.7,.06},Wood,0,0},{Cube,{0,1.5,0},{.14,.14,.14},Rust,0,0}};
  return {};
 }
 FLinearColor Tone(uint32 Hex){return S47Three::Hex(Hex);}
 const TCHAR* const Motel=TEXT("motel");
}

const TArray<FS47Places::FDef>& FS47Places::Defs()
{
 static const TArray<FDef> Table={
  {TEXT("windmill"),TEXT("the windmill on the Baca lease"),TEXT("windmill"),170,720,TEXT("A stock windmill with its brake on and no stock near it. Someone has wired the tail vane so the wheel faces a little east of north-northeast. The wire is new. The pump rod is rusted solid."),TEXT("All steel; the needle follows the tower and nothing else.")},
  {TEXT("tank"),TEXT("the water tank on the old grade"),TEXT("tank"),-190,1010,TEXT("A corrugated stock tank, a third full. The water has a skin on it and nothing drinks here: no prints at the rim in any direction, and it is the only water for miles."),TEXT("The needle swings as you walk around the tank, as it should. It takes longer to settle on the far side.")},
  {TEXT("mine"),TEXT("the old mine on the west side"),TEXT("mine"),-560,1340,TEXT("An adit with its boards pulled off and laid in a neat stack. Cold air comes out of it at a steady rate. Old survey tags on the timbers, the dates weathered off. Nobody has mined anything here since well before that."),TEXT("Ore in the tailings pulls the needle; the heading wanders. It wanders less at the mouth than it should.")},
  {TEXT("balloon"),TEXT("the balloon remains east of the highway"),TEXT("balloon"),300,1270,TEXT("Neoprene and a parachute of orange cloth, and a cardboard instrument box with a weather service return label. It has been out here a while. Ordinary, as far as it goes. The label asks the finder to mail it in, and the box is empty."),TEXT("Nothing. A cardboard box, a wire and some rubber.")},
  {TEXT("cow"),TEXT("the dead cow by the fence"),TEXT("cow"),110,1590,TEXT("A dead cow with a brand from the Alderete place, a long way from it. No scavengers have touched her and the flies have not found her. She has been dead a few days by the smell. The ground under her is dry."),TEXT("The needle dips over her, about as much as for a belt buckle. There is a tag in her ear and that may be all it is.")},
  {TEXT("stake"),TEXT("the survey stake off the track"),TEXT("stake"),-130,650,TEXT("A hub stake with orange flagging, freshly driven, and a witness stake beside it. Written on it in grease pencil: an arrow, and a bearing of 026. There is no line of stakes. There is just this one."),TEXT("A nail in the hub moves the needle a hair. Nothing else.")},
  {TEXT("shack"),TEXT("the line shack"),TEXT("shack"),440,930,TEXT("A line camp, one room, with a cot frame and a calendar for 1971. Coffee in a can, gone solid. Someone has been keeping a tally on the door frame in pencil, in fours and a stroke. The last group stops at three."),TEXT("Stove, nails, a bed frame. The needle has plenty to say and none of it is anything.")},
  {TEXT("trough"),TEXT("the salt lick and trough"),TEXT("trough"),240,1740,TEXT("A rusted trough and a block of stock salt, licked hollow on one side only. The hoof prints all stop about ten feet short of the trough, and go back the way they came."),TEXT("Iron in the trough. The needle settles on it from any side.")},
  {TEXT("cairn"),TEXT("the cairn on the rise"),TEXT("cairn"),-320,430,TEXT("A stack of stones with a post in the top, the kind a survey party leaves. From here you can see SARO and the farm both. The stones are newer than the post. Someone has rebuilt it recently, carefully, facing the array."),TEXT("A steady reading, nothing to speak of. It stays steady for longer than you would think a needle could.")},
  // PT-SAUCER-ROCK-20261109: on the souvenir man's map, and now on the land too, where he drew it (FS47TownUse::SaucerMap).
  {TEXT("saucer_rock"),TEXT("Saucer Rock"),TEXT("saucer rock"),-760,2240,TEXT("A pillar of pale rock taller than two men with a wide flat cap on it, a cup and saucer. Initials cut in the stem back to 1951, a fire ring, cans. Round its foot someone has set a ring of stones and swept the sand inside it smooth. Nothing has crossed it."),TEXT("The cap pulls the needle a few degrees. Walk round it and the pull follows the rock, as it should.")},
  {Motel,TEXT("the Sierra Motor Court"),Motel,0,0,TEXT("Sierra Motor Court office. The night bell works and the key rack has more keys than the lot has cars. The card on the counter says rooms are eight dollars, and quiet."),TEXT("Nothing but the bell.")},
 };
 return Table;
}
const FS47Places::FDef* FS47Places::Def(const FString& Id){return Defs().FindByPredicate([&](const FDef& D){return Id==D.Id;});}
bool FS47Places::bHasFound(const FString& Id) const{return M&&M->Freeform&&M->Freeform->HasFinding(TEXT("place_")+Id);}
double FS47Places::AzimuthOf(double FromX,double FromZ,double ToX,double ToZ)
{
 const double A=FMath::RadiansToDegrees(FMath::Atan2(ToX-FromX,-(ToZ-FromZ)));
 return A<0?A+360:A;
}
void FS47Places::Begin(const S47J::FObj& Saved)
{
 Stop();Life=MakeShared<uint8>(0);HeardOf.Reset();Fixes.Reset();LastFixDay=0;NeedleIn=0;Marks.Reset();
 for(const FString& H:S47J::Strs(Saved,TEXT("heard")))if(Def(H))HeardOf.Add(H);
 {
  // The player's own marks on the map: x and z, two numbers each, inside the rim.
  const TArray<double> Xz=S47J::Nums(Saved,TEXT("marks"));
  for(int32 I=0;I+1<Xz.Num()&&Marks.Num()<MaxMarks;I+=2)if(FMath::IsFinite(Xz[I])&&FMath::IsFinite(Xz[I+1])&&S47OpenLand::InRim(Xz[I],Xz[I+1]))Marks.Add(FVector2D(Xz[I],Xz[I+1]));
 }
 for(const auto& V:S47J::Arr(Saved,TEXT("fixes")))
 {
  if(!V||V->Type!=EJson::Object)continue;const auto O=V->AsObject();FBearing B;
  B.Day=FMath::Max(0,S47J::Int(O,TEXT("day")));B.Az=S47J::Num(O,TEXT("az"));B.Spread=S47J::Num(O,TEXT("spread"));B.Target=S47J::Str(O,TEXT("target"));
  if(FMath::IsFinite(B.Az)&&FMath::IsFinite(B.Spread)&&B.Az>=0&&B.Az<=360&&B.Spread>0&&B.Spread<=45){Fixes.Add(B);LastFixDay=FMath::Max(LastFixDay,B.Day);}
 }
 if(!M||!IsValid(M->Web.Get()))return;
 if(Scene.Get()!=M->Web.Get()){bBuilt=false;Built.Reset();}
 Scene=M->Web;Build();
}
void FS47Places::Stop()
{
 Life.Reset();
}
S47J::FObj FS47Places::Save() const
{
 auto O=S47J::New();
 if(!HeardOf.IsEmpty()){TArray<FString> H=HeardOf.Array();H.Sort();S47J::Set(O,TEXT("heard"),H);}
 if(!Fixes.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> List;
  for(const FBearing& B:Fixes){auto X=S47J::New();S47J::Set(X,TEXT("day"),B.Day);S47J::Set(X,TEXT("az"),B.Az);S47J::Set(X,TEXT("spread"),B.Spread);S47J::Set(X,TEXT("target"),B.Target);List.Add(MakeShared<FJsonValueObject>(X));}
  O->SetArrayField(TEXT("fixes"),List);
 }
 if(!Marks.IsEmpty()){TArray<double> Xz;for(const FVector2D& Mk:Marks){Xz.Add(FMath::RoundToDouble(Mk.X*10)/10);Xz.Add(FMath::RoundToDouble(Mk.Y*10)/10);}S47J::Set(O,TEXT("marks"),Xz);}
 return O;
}
bool FS47Places::ToggleMark(double X,double Z,double Within)
{
 if(!FMath::IsFinite(X)||!FMath::IsFinite(Z)||!S47OpenLand::InRim(X,Z))return false;
 // On one already there: rubbed out. Otherwise a new one, the last number.
 int32 Near=INDEX_NONE;double Best=FMath::Max(1.,Within);
 for(int32 I=0;I<Marks.Num();++I){const double D=FVector2D::Distance(Marks[I],FVector2D(X,Z));if(D<=Best){Best=D;Near=I;}}
 if(Near!=INDEX_NONE){Marks.RemoveAt(Near);UE_LOG(LogTemp,Display,TEXT("S47: map mark rubbed out at %.0f %.0f, %d left"),X,Z,Marks.Num());}
 else
 {
  if(Marks.Num()>=MaxMarks)return false;
  Marks.Add(FVector2D(X,Z));UE_LOG(LogTemp,Display,TEXT("S47: map mark %d at %.0f %.0f"),Marks.Num(),X,Z);
 }
 if(M)M->RequestAutosave();
 return true;
}
void FS47Places::Hear(const FString& Id)
{
 if(!Def(Id)||HeardOf.Contains(Id))return;
 HeardOf.Add(Id);if(M)M->RequestAutosave();
 UE_LOG(LogTemp,Display,TEXT("S47: heard about %s: on the map with a question mark"),*Id);
}
FString FS47Places::NextUnknown(int32 Salt) const
{
 TArray<FString> Left;for(const FDef& D:Defs())if(FString(D.Id)!=Motel&&!bHasFound(D.Id)&&!HeardOf.Contains(D.Id))Left.Add(D.Id);
 if(Left.IsEmpty())return FString();
 const int32 Seed=M&&M->Freeform?M->Freeform->Seed():47;
 return Left[int32(FS47NightRhythm::Roll(Seed,Salt,0x504c4143u)*Left.Num())%Left.Num()];
}
bool FS47Places::Spot(const FString& Id,double& OutX,double& OutZ) const
{
 const FBuilt* B=Built.FindByPredicate([&](const FBuilt& X){return X.Id==Id;});
 if(B){OutX=B->X;OutZ=B->Z;return true;}
 const FDef* D=Def(Id);if(!D)return false;OutX=D->X;OutZ=D->Z;return true;
}
bool FS47Places::Build()
{
 if(bBuilt)return true;
 if(!M||!IsValid(M->Web.Get())||!IsValid(M->Inter))return false;
 const int32 RoadRoot=M->Web->Handle(TEXT("world.legs.road.area.group")),SaroRoot=M->Web->Handle(TEXT("ext.group"));if(RoadRoot<0||SaroRoot<0)return false;
 auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Web/Base/M_Web_Lit_Opaque.M_Web_Lit_Opaque"));if(!Base)return false;
 const TWeakPtr<uint8> Guard=Life;
 for(const FDef& D:Defs())
 {
  FBuilt B;B.Id=D.Id;B.X=D.X;B.Z=D.Z;
  const bool bMotel=FString(D.Id)==Motel;
  if(bMotel)
  {
   const FS47Box2& Park=S47Legs::MOTEL_PARK();const FVector At=FVector(Park.MinX-1.2,0,Park.MinZ+2);
   B.X=At.X;B.Z=At.Z;
  }
  else
  {
   // Off the slope and out of the sites, as near the planned spot as the land allows.
   bool bClear=false;
   for(double R=0;R<=260&&!bClear;R+=26)for(int32 I=0;I<(R>0?8:1)&&!bClear;++I)
   {
    const double X=D.X+FMath::Cos(I*UE_DOUBLE_PI/4)*R,Z=D.Z+FMath::Sin(I*UE_DOUBLE_PI/4)*R;
    if(!S47OpenLand::InRim(X,Z,20)||S47OpenLand::InSaroCore(X,Z,15)||S47OpenLand::InStationCore(X,Z,15)||FMath::Abs(X)<16||S47Relief::HillStops(X,Z,5))continue;
    B.X=X;B.Z=Z;bClear=true;
   }
   UE_LOG(LogTemp,Display,TEXT("S47: place %s stands at road %.0f %.0f%s"),D.Id,B.X,B.Z,bClear?TEXT(""):TEXT(" (no clear ground found: as planned)"));
  }
  bool bFirst=true;
  for(const FPart& P:PartsOf(D.Kind))
  {
   if(!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(P.Mesh))))continue;
   auto* Mesh=LoadObject<UStaticMesh>(nullptr,P.Mesh);if(!Mesh)continue;
   const FString Name=FString::Printf(TEXT("S47Place_%s_%d"),D.Id,B.Parts.Num());
   auto* C=NewObject<UStaticMeshComponent>(M->Web.Get(),*Name);C->SetMobility(EComponentMobility::Movable);C->SetupAttachment(M->Web->GetRootComponent());C->SetStaticMesh(Mesh);C->SetCastShadow(true);
   if(P.Colour!=0)
   {
    auto* Mat=UMaterialInstanceDynamic::Create(Base,M->Web.Get());Mat->SetVectorParameterValue(TEXT("Color"),Tone(P.Colour));Mat->SetScalarParameterValue(TEXT("Roughness"),.92f);Mat->SetScalarParameterValue(TEXT("NativeLighting"),1);C->SetMaterial(0,Mat);
   }
   // The cow's own meshes are metres already, in Unreal's terms; basic shapes are a metre to a side.
   C->SetWorldScale3D(FVector(P.Size.X,P.Size.Z,P.Size.Y));
   // The cow is on her side about her own length, which in those meshes is the y axis.
   C->SetWorldRotation(FString(D.Kind)==TEXT("cow")?FRotator(P.Roll,P.Yaw,0):FRotator(0,P.Yaw,P.Roll));
   C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();
   if(bFirst)
   {
    C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
    B.Hit=M->Web->ObjAt(M->Web->AdoptNode(C,bMotel?SaroRoot:RoadRoot,UseId(D.Id)));bFirst=false;
   }
   else if(FString(D.Kind)!=TEXT("cow"))
   {
    // The whole thing can be pointed at, not just its first leg.
    C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
    const FS47Obj More=M->Web->ObjAt(M->Web->AdoptNode(C,bMotel?SaroRoot:RoadRoot,Name));if(More)M->Web->MarkUsable(More.I,UseId(D.Id));
   }
   B.Parts.Add(C);B.Local.Add(P.At);
  }
  if(B.Hit)
  {
   const FString Id=D.Id;
   FS47Interactable I;I.Id=UseId(D.Id);I.Object=B.Hit;I.Range=bMotel?2.6:6;
   I.Label=[this,Guard,Id]{return Guard.IsValid()?FString::Printf(TEXT("%s / look closer"),Def(Id)->Title):FString();};
   I.Use=[this,Guard,Id]{if(Guard.IsValid())Use(Id);};
   I.Examine=I.Label;M->Inter->Add(I);M->Web->MarkUsable(B.Hit.I,I.Id);
  }
  Seat(B);Built.Add(B);
 }
 bBuilt=true;return true;
}
void FS47Places::Seat(FBuilt& B)
{
 if(!M||!IsValid(M->Web.Get()))return;
 const bool bMotel=B.Id==Motel;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 const FVector World=bMotel?FVector(B.X,0,B.Z):FVector(Road.X+B.X,0,Road.Z+B.Z);
 double Y=bMotel?S47Legs::SaroFloorAt(B.X,B.Z):Road.Y+S47RoadTerrain::SurfaceY(B.X,B.Z);
 // What the player will really stand on, once they are near enough for it to be measured. Not a line trace: that finds the top
 // of a bush as readily as the ground, and the place then hangs in the air.
 const bool bSameLand=M->Player&&M->Player->Floor&&M->Web->Area()==(bMotel?FString(TEXT("saro")):M->Web->Area())&&(bMotel||M->Web->Area()!=TEXT("saro"));
 if(bSameLand)
 {
  const double Floor=M->Player->Floor(World.X,World.Z);
  if(FMath::IsFinite(Floor)){Y=Floor;B.bSeated=true;}
 }
 B.Y=Y;
 for(int32 I=0;I<B.Parts.Num();++I)if(auto* C=B.Parts[I].Get())C->SetWorldLocation(S47Three::ToUE(FVector(World.X,Y,World.Z)+B.Local[I]));
}
bool FS47Places::Drifts(const FString& Id){return Id==TEXT("stake")||Id==TEXT("cairn")||Id==TEXT("trough");}
FVector2D FS47Places::DriftOf(const FString& Id,int32 Seed,int32 Day)
{
 if(!Drifts(Id))return FVector2D::ZeroVector;
 FVector2D Sum=FVector2D::ZeroVector;const uint32 Salt=FCrc::StrCrc32(*Id);
 // Every few days, a few metres. It never wanders far from where it stood.
 for(int32 D=2;D<=Day;++D)if(FS47NightRhythm::Roll(Seed,D,Salt)<.2f)
 {
  const float A=FS47NightRhythm::Roll(Seed,D,Salt+1)*6.2831853f,R=2.f+5.f*FS47NightRhythm::Roll(Seed,D,Salt+2);
  Sum+=FVector2D(FMath::Cos(A),FMath::Sin(A))*double(R);
 }
 return Sum.ClampAxes(-18.,18.);
}
void FS47Places::Step(double Dt)
{
 if(!M||!M->Freeform||!M->Player||!M->Web)return;
 if(!bBuilt&&!Build())return;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 for(FBuilt& B:Built)
 {
  const FVector World=B.Id==Motel?FVector(B.X,0,B.Z):FVector(Road.X+B.X,0,Road.Z+B.Z);
  const double Away=FVector2D(M->Player->Pos.X-World.X,M->Player->Pos.Z-World.Z).Size();
  if(!B.bSeated&&Away<300)Seat(B);
  // Not while anyone is near enough to see it go.
  if(Drifts(B.Id)&&B.ShiftDay!=M->Freeform->Day()&&Away>200)
  {
   const FVector2D Then=B.Shift,Now=DriftOf(B.Id,M->Freeform->Seed(),M->Freeform->Day());
   B.ShiftDay=M->Freeform->Day();
   if(!Now.Equals(Then,.01))
   {
    B.Shift=Now;B.X+=Now.X-Then.X;B.Z+=Now.Y-Then.Y;B.bSeated=false;
    UE_LOG(LogTemp,Display,TEXT("S47: place %s has shifted %.1f m since it first stood there"),*B.Id,Now.Size());
   }
  }
 }
 StepNeedle(Dt);
}
void FS47Places::StepNeedle(double Dt)
{
 auto& F=*M->Freeform;
 if(!F.HasKit(TEXT("magnetometer"))||(M->World&&M->World->bDriving)||M->ModalOpen())return;
 NeedleIn-=Dt;if(NeedleIn>0)return;NeedleIn=5;
 // What a needle on foot can tell: something off the ordinary, and which way. It says nothing of what.
 const FVector Foot=M->Player->Pos;const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 double Best=90;FVector Toward=FVector::ZeroVector;FString What;
 for(const FBuilt& B:Built)
 {
  if(bHasFound(B.Id)||B.Id==Motel)continue;
  const FVector At(Road.X+B.X,Foot.Y,Road.Z+B.Z);const double D=FVector2D(At.X-Foot.X,At.Z-Foot.Z).Size();
  if(D<Best){Best=D;Toward=At;What=B.Id;}
 }
 if(F.FarmSite&&F.FarmSite->Armed()&&!F.FarmSite->Raked()){const FVector C=F.FarmSite->Centre();const double D=FVector2D(C.X-Foot.X,C.Z-Foot.Z).Size();if(D<Best){Best=D;Toward=C;What=TEXT("marks");}}
 if(F.Gear)for(const auto& L:F.Gear->Items()){const double D=FVector2D(L.At.X-Foot.X,L.At.Z-Foot.Z).Size();if(D<Best){Best=D;Toward=L.At;What=L.Item;}}
 if(What.IsEmpty())return;
 const int32 Strength=Best<20?3:Best<50?2:1;
 if(M->Audio)M->Audio->Beep(600.f+Strength*220.f,.07f,.05f);
 if(M->UI)M->UI->Toast(FString::Printf(TEXT("NEEDLE / %s, %s."),Strength==3?TEXT("hard over"):Strength==2?TEXT("steady pull"):TEXT("a small lean"),*FS47FreeformObservation::Direction(Foot,Toward)),3);
 UE_LOG(LogTemp,Display,TEXT("S47: magnetometer: %s at %.0f m"),*What,Best);
}
void FS47Places::Panel(const FDef& D,bool bFirst,const FString& Extra)
{
 if(!M||!M->UI||!M->Freeform)return;
 TArray<S47Kit::FW> Rows;
 Rows.Add(S47Kit::Text(D.Finding));
 if(!Extra.IsEmpty())Rows.Add(S47Kit::Text(Extra));
 Rows.Add(S47Kit::Small(bFirst?TEXT("Kept as a field record. It says what is here, not why."):TEXT("Already in your field records.")));
 auto* F=M->Freeform.Get();
 const double Next=(FMath::FloorToDouble((F->Clock()-8*3600)/86400.)+1)*86400.+8*3600;
 if(FString(D.Id)==Motel)
 {
  Rows.Add(S47Kit::Small(TEXT("A locked door and a bed. What goes on out on the land, you hear about in the morning. A motel has its own nights.")));
  Rows.Add(S47Kit::Small(TEXT("Nothing is saved at the motel. The game saves again once you are back at SARO.")));
  // MOTEL-SLEEP-20261108: the night's own may wake the player at its hour (S47MotelNight). MOTEL-PAY-20261109: woken early (the big
  // night wakes a room too) and back at the desk, it is the room already paid for tonight, not a second one (FS47MotelNight::TakeRoom).
  const bool bPaid=F->MotelNight&&F->MotelNight->RoomPaid(F->Clock());
  Rows.Add(S47Kit::Button(bPaid?FString(TEXT("Back to your room until morning / paid")):FString::Printf(TEXT("Take a room until morning / $%d"),F->MotelPrice()),[this,F,Next]
  {
   if(M->UI)M->UI->Close(true);
   if(F->MotelNight){F->MotelNight->TakeRoom(Next);return;}
   F->RestAt(Next,FS47FreeformSession::ERest::Motel);
  },true,TAttribute<bool>::CreateLambda([F]{return (F->MotelNight&&F->MotelNight->RoomPaid(F->Clock()))||F->Budget()>=F->MotelPrice();})));
 }
 else if(F->HasKit(TEXT("cot")))
 {
  Rows.Add(S47Kit::Small(TEXT("The cot is in the truck. Out here, anything close by will wake you.")));
  Rows.Add(S47Kit::Button(TEXT("Put up the cot here until morning"),[this,F,Next]{if(M->UI)M->UI->Close(true);F->RestAt(Next,FS47FreeformSession::ERest::Field);},true));
 }
 if(F->TripCamera&&F->TripCamera->CanSet()&&FString(D.Id)!=Motel)
 {
  const FString Where=D.Title;
  Rows.Add(S47Kit::Button(TEXT("Leave the trip camera here until morning"),[this,F,Where]{F->TripCamera->SetHere(Where,false);if(M->UI)M->UI->Close(true);}));
 }
 // Salt or range cubes from the Feed & Seed, set out here, and what came to them (TOWN-USE-20261108, S47TownUse).
 if(F->TownUse&&FString(D.Id)!=Motel)F->TownUse->LureRows(D.Id,Rows);
 M->UI->Open(S47Kit::Shell(M->UI,FString(D.Title).ToUpper(),S47Kit::Col(Rows),760));
}
bool FS47Places::Use(const FString& Id)
{
 const FDef* D=Def(Id);if(!D||!M||!M->UI||!M->Freeform)return false;auto& F=*M->Freeform;
 const FString Key=TEXT("place_")+Id;const bool bFirst=!F.HasFinding(Key);
 if(bFirst)
 {
  FString Page=FString(D->Finding);if(!Page.Contains(TEXT("records")))Page+=TEXT(" This records what is here.");
  F.RecordEvidence(Key,Page);
  UE_LOG(LogTemp,Display,TEXT("S47: place found: %s"),*Id);
  M->RequestAutosave();
 }
 FString Extra;
 if(!bFirst&&Drifts(Id)&&!DriftOf(Id,F.Seed(),F.Day()).IsNearlyZero(.5))
 {
  // A second visit: it is the player's own record that says it was somewhere else.
  const FString Moved=FString::Printf(TEXT("%s_moved_d%d"),*Key,F.Day());
  Extra=TEXT("It is not where you noted it. Your record puts it some paces from here, and nothing shows it was dragged.");
  F.RecordEvidence(Moved,FString::Printf(TEXT("Not where I recorded it: %s, some paces off. No drag marks that I can see."),D->Title));
 }
 if(F.HasKit(TEXT("magnetometer")))
 {
  Extra+=(Extra.IsEmpty()?FString():TEXT("\n"))+TEXT("MAGNETOMETER / ")+FString(D->Needle);
  F.RecordEvidence(Key+TEXT("_needle"),FString::Printf(TEXT("Magnetometer at %s. %s"),D->Title,D->Needle));
 }
 // PT-SAUCER-ROCK-20261109: Saucer Rock by the hour, as the nights go (18 to 24 a hint that has another reason, after midnight
 // something far off, after two the sign of a visit): what is there when nothing is happening, kept once a night.
 if(Id==TEXT("saucer_rock"))
 {
  const double Hour=FMath::Fmod(F.Clock(),86400.)/3600.;
  const int32 Tier=SaucerTier(Hour);
  const FString Now=SaucerAt(Tier,F.TownUse&&F.TownUse->HasMap());
  if(!Now.IsEmpty())
  {
   Extra+=(Extra.IsEmpty()?FString():TEXT("\n"))+Now;
   if(Tier==2||Tier==3)F.RecordEvidence(FString::Printf(TEXT("%s_night%d_t%d"),*Key,FS47FreeformSession::NightOf(F.Clock()),Tier),FString::Printf(TEXT("Saucer Rock, after midnight. %s"),*Now));
  }
 }
 Panel(*D,bFirst,Extra);return true;
}
int32 FS47Places::SaucerTier(double Hour)
{
 if(!FMath::IsFinite(Hour))return 0;
 Hour=FMath::Fmod(FMath::Fmod(Hour,24.)+24.,24.);
 // Critics, round 8: the cap's "last of the light" from just after sunset (18:27) to 19:30 only; the rest of the evening its own
 // (tier 4); before 18:30 still the day's board.
 return Hour>=19.5?4:Hour>=18.5?1:Hour<2?2:Hour<FS47NightRhythm::EndHour?3:0;
}
FString FS47Places::SaucerAt(int32 Tier,bool bMap)
{
 switch(Tier)
 {
  case 1:return TEXT("The cap keeps the last of the light a while after the land has gone dark. Inside the swept ring the sand is colder to the hand than outside it. Shade, most of the day. That would do it.");
  case 2:return TEXT("Low over the ridge to the west, a light that does not move. Look straight at it and it is not there; look a little aside and it is. It is still there when you turn to go.");
  // (critics, round 8: true on a first visit as on a later one: the tracks one has just made coming in)
  case 3:return TEXT("The sand inside the ring is swept smooth. Behind you, the tracks you made coming in are already gone, and there are no others, going in or coming out.");
  case 4:return TEXT("The wind drops off inside the swept ring, as if the stones stood higher than they do. Out of the wind the sand stays cold to the hand. That would do it.");
  default:return bMap?TEXT("The souvenir man's map has it about right, for once. The board on the post says SAUCER ROCK in house paint, and under it, smaller, NO CAMPING."):TEXT("The board on the post says SAUCER ROCK in house paint, and under it, smaller, NO CAMPING.");
 }
}
void FS47Places::FillMap(FS47MapView& View) const
{
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
 for(const FBuilt& B:Built)
 {
  if(B.Id==Motel)continue;
  const bool bKept=bHasFound(B.Id);
  if(!bKept&&!HeardOf.Contains(B.Id))continue;
  FS47MapPin P;P.East=B.X;P.South=B.Z-SaroZ;P.Label=Def(B.Id)?FString(Def(B.Id)->Kind):B.Id;P.bHeard=!bKept;View.Pins.Add(P);
 }
 (void)Road;
 const int32 From=FMath::Max(0,Fixes.Num()-3);
 for(int32 I=From;I<Fixes.Num();++I){FS47MapSector S;S.Az=Fixes[I].Az;S.Spread=Fixes[I].Spread;S.Label=FString::Printf(TEXT("D%d"),Fixes[I].Day);View.Sectors.Add(S);}
}
bool FS47Places::AfterRegistered()
{
 if(!M||!M->Freeform||!M->Freeform->RulesLive()||!M->Freeform->Field)return false;
 auto& F=*M->Freeform;
 const auto& Wear=F.Field->ArrayWear();
 const int32 Branches=Wear.bActive?Wear.Restored.Num():0;
 if(Branches<=0||LastFixDay==F.Day())return false;
 double SaroX=0,SaroZ=0;if(!S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ))return false;
 // What is off the main beam tonight: the marks, if there are any; otherwise something out on the land nobody has looked at.
 FString Target;double ToX=0,ToZ=0;bool bHave=false;
 if(F.FarmSite&&F.FarmSite->Armed())
 {
  const FVector C=F.FarmSite->Centre();const FVector Road=S47Legs::ORIGINS(TEXT("road"));
  Target=F.FarmSite->PlaceTitle();
  if(F.FarmSite->PlaceId()==TEXT("farm"))bHave=S47OpenLand::RoadOf(TEXT("saro"),C.X,C.Z,ToX,ToZ);
  else{ToX=C.X-Road.X;ToZ=C.Z-Road.Z;bHave=true;}
 }
 if(!bHave){Target=NextUnknown(F.Day());bHave=!Target.IsEmpty()&&Spot(Target,ToX,ToZ);}
 if(!bHave)return false;
 const double Spread=SpreadFor(Branches);
 const double Err=(FS47NightRhythm::Roll(F.Seed(),F.Day(),0x4245415au)-.5)*Spread;
 double Az=FMath::Fmod(AzimuthOf(SaroX,SaroZ,ToX,ToZ)+Err+360.,360.);
 FBearing B;B.Day=F.Day();B.Az=Az;B.Spread=Spread;B.Target=Target;Fixes.Add(B);if(Fixes.Num()>12)Fixes.RemoveAt(0);
 LastFixDay=F.Day();
 F.RecordEvidence(FString::Printf(TEXT("bearing_d%d"),F.Day()),FString::Printf(TEXT("With %d mended %s the array resolves something off the main beam tonight: about %03.0f degrees from SARO, within %.0f. Penciled in on the map. It says which way, not how far and not what."),Branches,Branches==1?TEXT("branch"):TEXT("branches"),Az,Spread));
 if(M->UI)M->UI->Toast(FString::Printf(TEXT("Something off the main beam: about %03.0f degrees, give or take %.0f. It's on your map (TAB)."),Az,Spread),6);
 UE_LOG(LogTemp,Display,TEXT("S47: bearing: %.1f within %.0f toward %s, %d branches"),Az,Spread,*Target,Branches);
 M->RequestAutosave();
 return true;
}
bool FS47Places::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s places: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 TSet<FString> Ids;bool bWords=true,bParts=true;
 for(const FDef& D:Defs()){Ids.Add(D.Id);bWords&=FCString::Strlen(D.Finding)>40&&FCString::Strlen(D.Finding)<320&&FCString::Strlen(D.Needle)>4&&!FString(D.Finding).Contains(TEXT("alien"));bParts&=PartsOf(D.Kind).Num()>0;}
 Check(Ids.Num()==Defs().Num()&&Defs().Num()>=9&&Defs().Num()<=11,TEXT("eight to ten places besides the motel, each once"));
 // PT-SAUCER-ROCK-20261109: Saucer Rock is on the land where the souvenir man's map has it, with something to see at any hour.
 {
  const FDef* Rock=Def(TEXT("saucer_rock"));
  bool bTiers=SaucerTier(12)==0&&SaucerTier(18.2)==0&&SaucerTier(19)==1&&SaucerTier(20)==4&&SaucerTier(0.5)==2&&SaucerTier(3)==3&&SaucerTier(7)==0&&SaucerTier(-1)==4;
  for(int32 T=0;T<5;++T)bTiers&=!SaucerAt(T,false).IsEmpty()&&!SaucerAt(T,false).ToLower().Contains(TEXT("alien"))&&!SaucerAt(T,false).ToLower().Contains(TEXT("saucer people"));
  Check(Rock&&FVector2D::Distance(FVector2D(Rock->X,Rock->Z),FVector2D(-760,2240))<1&&PartsOf(Rock->Kind).Num()>=10&&bTiers,TEXT("Saucer Rock stands where the map has it, a rock with a ring and a board, and says something by day, at dusk, in the evening, after midnight and after two"));
 }
 Check(bWords&&bParts,TEXT("each has a prop and one thing to find, and none says what it is"));
 Check(FMath::IsNearlyEqual(AzimuthOf(0,0,0,-10),0.)&&FMath::IsNearlyEqual(AzimuthOf(0,0,10,0),90.)&&FMath::IsNearlyEqual(AzimuthOf(0,0,0,10),180.)&&FMath::IsNearlyEqual(AzimuthOf(0,0,-10,0),270.),TEXT("a bearing is degrees from north; road-local z runs south"));
 Check(SpreadFor(0)==0&&SpreadFor(1)>SpreadFor(2)&&SpreadFor(2)>SpreadFor(3)&&SpreadFor(3)>SpreadFor(4)&&SpreadFor(4)>0,TEXT("every mended branch narrows the sector, and none is exact"));
 {
  bool bStable=true,bNear=true,bMoves=false,bStill=true;
  for(int32 Day=1;Day<=120;++Day)
  {
   const FVector2D A=DriftOf(TEXT("stake"),47,Day);bStable&=A==DriftOf(TEXT("stake"),47,Day);bNear&=FMath::Abs(A.X)<=18&&FMath::Abs(A.Y)<=18;bMoves|=A.Size()>1;
   bStill&=DriftOf(TEXT("tank"),47,Day).IsZero()&&DriftOf(TEXT("mine"),47,Day).IsZero();
  }
  Check(bStable&&bNear&&bMoves&&bStill,TEXT("small things shift a little over the days, the same way in any Continue; what could not be carried stays"));
 }
 FS47Places None(nullptr);None.Begin(nullptr);Check(None.Save()->Values.Num()==0&&None.Bearings().IsEmpty(),TEXT("an older save has none of it and writes nothing"));
 auto O=S47J::New();S47J::Set(O,TEXT("heard"),TArray<FString>{TEXT("mine"),TEXT("nowhere")});
 auto Fix=S47J::New();S47J::Set(Fix,TEXT("day"),3);S47J::Set(Fix,TEXT("az"),31.);S47J::Set(Fix,TEXT("spread"),12.);S47J::Set(Fix,TEXT("target"),TEXT("mine"));
 TArray<TSharedPtr<FJsonValue>> List;List.Add(MakeShared<FJsonValueObject>(Fix));O->SetArrayField(TEXT("fixes"),List);
 FS47Places Back(nullptr);Back.Begin(S47J::Read(S47J::Write(O)));
 Check(Back.Heard(TEXT("mine"))&&!Back.Heard(TEXT("nowhere"))&&Back.Bearings().Num()==1&&Back.Bearings()[0].Az==31.,TEXT("heard and bearings survive a Continue, and nonsense is dropped"));
 {
  // The player's own marks on the map: one down, another, the first rubbed out by a touch near it, and the rest kept over a Continue.
  FS47Places Pencil(nullptr);Pencil.Begin(nullptr);
  const bool bDown=Pencil.ToggleMark(120,640,10)&&Pencil.ToggleMark(-300,900,10)&&Pencil.MapMarks().Num()==2;
  const bool bRubbed=Pencil.ToggleMark(124,643,10)&&Pencil.MapMarks().Num()==1&&Pencil.MapMarks()[0].Equals(FVector2D(-300,900));
  const bool bOff=!Pencil.ToggleMark(99999,0,10);
  FS47Places Kept(nullptr);Kept.Begin(S47J::Read(S47J::Write(Pencil.Save())));
  Check(bDown&&bRubbed&&bOff&&Kept.MapMarks().Num()==1&&Kept.MapMarks()[0].Equals(FVector2D(-300,900),.11),TEXT("pencil marks go down, rub out where they are touched, stay on the land and survive a Continue"));
 }
 return OK;
}
