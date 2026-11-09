#include "S47Places.h"
#include "S47Audio.h"
#include "S47FarmSite.h"
#include "S47Freeform.h"
#include "S47FreeformDirector.h"
#include "S47FreeformObservation.h"
#include "S47FreeformWorld.h"
#include "S47Happenings.h"
#include "S47Inter.h"
#include "S47Keys.h"
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

// CRIT9: the key that opens the notebook at its map, as the player has it (the pad's button with a gamepad, a rebound key), never a
// fixed one; the notebook's own key when the map has none.
static FString FS47PlacesLocalMapKey()
{
 const FString Map=S47Keys::Said(TEXT("map"));
 return Map.IsEmpty()?S47Keys::Said(TEXT("notebook")):Map;
}

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
  // LOOP-1-20261109: when it was taken, when it goes cold, and from where; an older save has none of them (no time, never cold,
  // from SARO), so the save needs no new version.
  B.Clock=S47J::Num(O,TEXT("clock"),-1);B.Until=S47J::Num(O,TEXT("until"),-1);
  if(!FMath::IsFinite(B.Clock)||B.Clock<0)B.Clock=-1;
  if(!FMath::IsFinite(B.Until)||B.Until<0||(B.Clock>=0&&B.Until<=B.Clock))B.Until=-1;
  B.bCar=S47J::Bool(O,TEXT("car"));SaroRoad(B.FromX,B.FromZ);
  if(B.bCar)
  {
   const TArray<double> From=S47J::Nums(O,TEXT("from"));
   if(From.Num()!=2||!FMath::IsFinite(From[0])||!FMath::IsFinite(From[1])||!S47OpenLand::InRim(From[0],From[1]))continue;
   B.FromX=From[0];B.FromZ=From[1];
  }
  if(FMath::IsFinite(B.Az)&&FMath::IsFinite(B.Spread)&&B.Az>=0&&B.Az<=360&&B.Spread>0&&B.Spread<=45&&Fixes.Num()<MaxFixes){Fixes.Add(B);if(!B.bCar)LastFixDay=FMath::Max(LastFixDay,B.Day);}
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
  for(const FBearing& B:Fixes)
  {
   auto X=S47J::New();S47J::Set(X,TEXT("day"),B.Day);S47J::Set(X,TEXT("az"),B.Az);S47J::Set(X,TEXT("spread"),B.Spread);S47J::Set(X,TEXT("target"),B.Target);
   if(B.Clock>=0)S47J::Set(X,TEXT("clock"),B.Clock);
   if(B.Until>=0)S47J::Set(X,TEXT("until"),B.Until);
   if(B.bCar){S47J::Set(X,TEXT("car"),true);S47J::Set(X,TEXT("from"),TArray<double>{FMath::RoundToDouble(B.FromX*10)/10,FMath::RoundToDouble(B.FromZ*10)/10});}
   List.Add(MakeShared<FJsonValueObject>(X));
  }
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
  // LOOP-4-20261109 (HELHETLIG_LOKKE step 4): the needle has a history. Where something happened near the place in the last two
  // nights it will not settle, whatever any bearing says; otherwise the place's own reading. The unsettled needle is a record of OUT
  // ON THE LAND, once a day a place (FS47FreeformCases::LineForFinding).
  double Px=0,Pz=0;const bool bStirred=F.Happenings&&Spot(Id,Px,Pz)&&F.Happenings->StirredNear(Px,Pz);
  Extra+=(Extra.IsEmpty()?FString():TEXT("\n"))+TEXT("MAGNETOMETER / ")+NeedleReading(D->Needle,bStirred);
  F.RecordEvidence(Key+TEXT("_needle"),FString::Printf(TEXT("Magnetometer at %s. %s"),D->Title,D->Needle));
  if(bStirred)F.RecordEvidence(FString::Printf(TEXT("%s_unsettled_d%d"),*Key,F.Day()),FString::Printf(TEXT("Magnetometer at %s: the needle will not settle. It keeps swinging slowly across the heading long after I stand still, the way it does in a magnetic storm."),D->Title));
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
FString FS47Places::NeedleReading(const FString& Needle,bool bStirred)
{
 if(!bStirred)return Needle;
 return Needle+TEXT(" Then it will not settle. Long after you stand still it keeps swinging, slowly, across the heading, the way it does in a magnetic storm.");
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
void FS47Places::SaroRoad(double& X,double& Z){X=0;Z=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,X,Z);}
double FS47Places::Whole(double Az)
{
 // As S47Happenings' Bearing: whole degrees, and never Story's number (47, 147 ...): a degree on is as good a bearing.
 if(!FMath::IsFinite(Az))return 0;
 int32 D=FMath::RoundToInt(FMath::Fmod(FMath::Fmod(Az,360.)+360.,360.))%360;
 if(FString::FromInt(D).Contains(TEXT("47")))D=(D+1)%360;
 return double(D);
}
FString FS47Places::SlipClock(double Clock)
{
 int32 S=FMath::IsFinite(Clock)?int32(FMath::Fmod(FMath::Max(0.,Clock),86400.)):0;
 FString T;
 for(int32 Try=0;Try<4;++Try)
 {
  T=FString::Printf(TEXT("%02d:%02d"),(S/3600)%24,(S/60)%60);
  if(!T.Contains(TEXT("47"))&&T!=TEXT("08:15")&&T!=TEXT("20:15"))break;
  S+=60;
 }
 return T;
}
double FS47Places::ColdAt(double Clock)
{
 const double Noon=12*3600.;
 return (FMath::FloorToDouble((FMath::Max(0.,Clock)-Noon)/86400.)+1)*86400.+Noon;
}
double FS47Places::Freshness(const FBearing& B,double Now)
{
 if(B.Until<0)return 1;
 if(Now>=B.Until)return 0;
 if(B.Clock<0||B.Until<=B.Clock)return 1;
 return FMath::Clamp((B.Until-Now)/(B.Until-B.Clock),0.,1.);
}
FString FS47Places::SlipLine(const FBearing& B)
{
 const FString Az=FString::Printf(TEXT("%03.0f"),Whole(B.Az));
 const FString Spread=FString::Printf(TEXT("%.0f"),FMath::Clamp(FMath::RoundToDouble(B.Spread),1.,45.));
 return B.Clock>=0?FString::Printf(TEXT("BEARING ABOUT %s° ±%s° / %s / COLD BY NOON"),*Az,*Spread,*SlipClock(B.Clock))
  :FString::Printf(TEXT("BEARING ABOUT %s° ±%s°"),*Az,*Spread);
}
bool FS47Places::InSector(const FBearing& B,double X,double Z)
{
 if(FVector2D::Distance(FVector2D(B.FromX,B.FromZ),FVector2D(X,Z))<1)return false;
 return FMath::Abs(FMath::FindDeltaAngleDegrees(B.Az,AzimuthOf(B.FromX,B.FromZ,X,Z)))<=B.Spread;
}
bool FS47Places::Cross(const FBearing& A,const FBearing& B,FVector2D& Out)
{
 // From one station two bearings never cross: it has to be two places (the truck and SARO, or the truck twice).
 const FVector2D Pa(A.FromX,A.FromZ),Pb(B.FromX,B.FromZ);
 if(FVector2D::Distance(Pa,Pb)<50)return false;
 const auto Dir=[](double Az){const double R=FMath::DegreesToRadians(Az);return FVector2D(FMath::Sin(R),-FMath::Cos(R));}; // z runs south
 const FVector2D Da=Dir(A.Az),Db=Dir(B.Az);
 const double Den=FVector2D::CrossProduct(Da,Db);
 if(FMath::Abs(Den)<.02)return false; // within a degree or so of parallel: no point to it
 const FVector2D W=Pb-Pa;
 const double Ta=FVector2D::CrossProduct(W,Db)/Den,Tb=FVector2D::CrossProduct(W,Da)/Den;
 if(Ta<=0||Tb<=0||Ta>20000||Tb>20000)return false;
 Out=Pa+Da*Ta;return true;
}
bool FS47Places::AddCarFix(double FromX,double FromZ,double Az,const FString& Target,double Clock,int32 Day)
{
 if(!FMath::IsFinite(FromX)||!FMath::IsFinite(FromZ)||!FMath::IsFinite(Az)||!FMath::IsFinite(Clock)||Target.IsEmpty()||!S47OpenLand::InRim(FromX,FromZ))return false;
 FBearing B;B.Day=FMath::Max(0,Day);B.Az=Whole(Az);B.Spread=CarSpread;B.Target=Target.Left(64);B.Clock=FMath::Max(0.,Clock);B.Until=ColdAt(B.Clock);B.bCar=true;B.FromX=FromX;B.FromZ=FromZ;
 // The newest line from the truck toward the same thing: still near where it was taken, this one goes in its place.
 for(int32 I=Fixes.Num()-1;I>=0;--I)
 {
  const FBearing& Was=Fixes[I];
  if(!Was.bCar||Was.Target!=B.Target||!Live(Was,B.Clock))continue;
  if(FVector2D::Distance(FVector2D(Was.FromX,Was.FromZ),FVector2D(FromX,FromZ))<CarApart){Fixes[I]=B;if(M)M->RequestAutosave();return false;}
  break;
 }
 Fixes.Add(B);
 // Three lines from the truck toward one thing at most, and the list as long as it was: the truck's oldest go first, so that a
 // drive does not push the station's own bearings off.
 int32 Same=0;
 for(int32 I=Fixes.Num()-1;I>=0;--I)if(Fixes[I].bCar&&Fixes[I].Target==B.Target&&++Same>3)Fixes.RemoveAt(I);
 while(Fixes.Num()>MaxFixes){const int32 Car=Fixes.IndexOfByPredicate([](const FBearing& X){return X.bCar;});Fixes.RemoveAt(Car!=INDEX_NONE?Car:0);}
 UE_LOG(LogTemp,Display,TEXT("S47: bearing from the truck at %.0f %.0f: %.0f within %.0f toward %s"),FromX,FromZ,B.Az,B.Spread,*B.Target);
 if(M)M->RequestAutosave();
 return true;
}
FS47Places::FBearing FS47Places::StationFix(int32 Day,double Clock,int32 Branches,double FromX,double FromZ,double ToX,double ToZ,float Roll,const FString& Target)
{
 // Somewhere in the sector, not on its middle line: the roll moves the line up to half the spread either way.
 FBearing B;B.Day=Day;B.Spread=SpreadFor(Branches);B.Az=Whole(AzimuthOf(FromX,FromZ,ToX,ToZ)+(double(Roll)-.5)*B.Spread);B.Target=Target;
 B.Clock=FMath::Max(0.,Clock);B.Until=ColdAt(B.Clock);B.FromX=FromX;B.FromZ=FromZ;
 return B;
}
void FS47Places::FillMap(FS47MapView& View) const{FillMapAt(View,M&&M->Freeform?M->Freeform->Clock():0.);}
void FS47Places::FillMapAt(FS47MapView& View,double Now) const
{
 // The live bearings, newest last, at most six: a cold one is off the map (LOOP-1-20261109: cold by noon).
 TArray<int32> Shown;
 for(int32 I=Fixes.Num()-1;I>=0&&Shown.Num()<6;--I)if(Live(Fixes[I],Now))Shown.Insert(I,0);
 double SaroX=0,SaroZ=0;SaroRoad(SaroX,SaroZ);
 for(const FBuilt& B:Built)
 {
  if(B.Id==Motel)continue;
  const bool bKept=bHasFound(B.Id);
  if(!bKept&&!HeardOf.Contains(B.Id))continue;
  FS47MapPin P;P.East=B.X;P.South=B.Z-SaroZ;P.Label=Def(B.Id)?FString(Def(B.Id)->Kind):B.Id;P.bHeard=!bKept;
  for(const int32 I:Shown)P.bInSector|=InSector(Fixes[I],B.X,B.Z);
  View.Pins.Add(P);
 }
 for(const int32 I:Shown)
 {
  const FBearing& B=Fixes[I];
  FS47MapSector S;S.Az=B.Az;S.Spread=B.Spread;S.Label=B.bCar?FString::Printf(TEXT("D%d truck"),B.Day):FString::Printf(TEXT("D%d"),B.Day);
  S.bFrom=B.bCar;S.FromX=B.FromX;S.FromZ=B.FromZ;
  // The pencil pales as it cools, never quite away while it is live.
  S.Fade=float(.3+.7*Freshness(B,Now));
  View.Sectors.Add(S);
 }
 for(int32 A=0;A<Shown.Num();++A)for(int32 C=A+1;C<Shown.Num();++C)
 {
  FVector2D At;
  if(Cross(Fixes[Shown[A]],Fixes[Shown[C]],At)&&S47OpenLand::InRim(At.X,At.Y))View.Crossings.Add(At);
 }
}
bool FS47Places::AfterRegistered(bool bAnomaly)
{
 if(!M||!M->Freeform||!M->Freeform->RulesLive()||!M->Freeform->Field)return false;
 auto& F=*M->Freeform;
 const auto& Wear=F.Field->ArrayWear();
 const int32 Branches=Wear.bActive?Wear.Restored.Num():0;
 // LOOP-1-20261109: a discrepancy points out even with no branch mended (wide); routine work only through a mended branch.
 if(!Gives(bAnomaly,Branches))return false;
 const double Now=F.Clock();const int32 Night=FS47FreeformSession::NightOf(Now);
 // At most one thing a night at SARO points out at the land: the Director holds it (director.json point_out_per_night).
 if(F.Director?!F.Director->MayPointOut(Night):LastFixDay==F.Day())return false;
 // PT-FEEDER-20261109: the big night's night is its own (FS47Happenings::BigOwns): it points, nothing else does, before it or after.
 if(F.Happenings&&F.Happenings->BigOwns(Night))return false;
 double SaroX=0,SaroZ=0;SaroRoad(SaroX,SaroZ);
 // What is off the main beam tonight: the marks, if there are any and their tier allows it at this hour (director.json: before
 // midnight only what can be explained away); otherwise something out on the land nobody has looked at.
 FString Target;double ToX=0,ToZ=0;bool bHave=false;
 if(F.FarmSite&&F.FarmSite->Armed())
 {
  const FString Why=F.FarmSite->FieldVisit()?TEXT("field_visit"):F.FarmSite->PlaceId()==TEXT("farm")?TEXT("farm_lights"):TEXT("town_lights");
  if(!F.Director||F.Director->MayPointAt(Why,Now,Night))
  {
   const FVector C=F.FarmSite->Centre();const FVector Road=S47Legs::ORIGINS(TEXT("road"));
   Target=F.FarmSite->PlaceTitle();
   if(F.FarmSite->PlaceId()==TEXT("farm"))bHave=S47OpenLand::RoadOf(TEXT("saro"),C.X,C.Z,ToX,ToZ);
   else{ToX=C.X-Road.X;ToZ=C.Z-Road.Z;bHave=true;}
  }
 }
 if(!bHave){Target=NextUnknown(F.Day());bHave=!Target.IsEmpty()&&Spot(Target,ToX,ToZ);}
 if(!bHave)return false;
 const double Spread=SpreadFor(Branches);
 const FBearing B=StationFix(F.Day(),Now,Branches,SaroX,SaroZ,ToX,ToZ,FS47NightRhythm::Roll(F.Seed(),F.Day(),0x4245415au),Target);
 Fixes.Add(B);while(Fixes.Num()>MaxFixes)Fixes.RemoveAt(0);
 LastFixDay=F.Day();
 if(F.Director)F.Director->NotePointOut(Night);
 const FString Mended=Branches<=0?FString(TEXT("With no branch mended the array still resolves something off the main beam, broadly"))
  :FString::Printf(TEXT("With %d mended %s the array resolves something off the main beam tonight"),Branches,Branches==1?TEXT("branch"):TEXT("branches"));
 F.RecordEvidence(FString::Printf(TEXT("bearing_d%d"),F.Day()),FString::Printf(TEXT("%s: about %03.0f degrees from SARO, within %.0f, at %s. Penciled in on the map, and cold by noon. It says which way, not how far and not what."),*Mended,B.Az,Spread,*SlipClock(Now)));
 // The slip on the OPS printer, collected there like the result sheets (behind any job's paper).
 const bool bSlip=F.QueueBearingSlip(B.Day,SlipLine(B),Branches<=0?FString(TEXT("NO BRANCH MENDED: A WIDE SECTOR."))
  :FString::Printf(TEXT("%d %s MENDED."),Branches,Branches==1?TEXT("BRANCH"):TEXT("BRANCHES")));
 if(M->UI)M->UI->Toast(FString::Printf(TEXT("%sSomething off the main beam: about %03.0f degrees, give or take %.0f. It's on your map (%s)."),bSlip?TEXT("PRINTER / A bearing slip. "):TEXT(""),B.Az,Spread,*FS47PlacesLocalMapKey()),6);
 UE_LOG(LogTemp,Display,TEXT("S47: bearing: %.0f within %.0f toward %s, %d branches, %s, night %d"),B.Az,Spread,*Target,Branches,bAnomaly?TEXT("a discrepancy"):TEXT("routine"),Night);
 M->RequestAutosave();
 return true;
}
bool FS47Places::PointAt(const FString& Target,double ToX,double ToZ)
{
 if(!M||!M->Freeform||!M->Freeform->RulesLive()||!M->Freeform->Field||Target.IsEmpty())return false;
 auto& F=*M->Freeform;
 const auto& Wear=F.Field->ArrayWear();
 const int32 Branches=Wear.bActive?Wear.Restored.Num():0;
 const double Now=F.Clock();const int32 Night=FS47FreeformSession::NightOf(Now);
 if(F.Director?!F.Director->MayPointOut(Night):LastFixDay==F.Day())return false;
 double SaroX=0,SaroZ=0;SaroRoad(SaroX,SaroZ);
 const double Spread=SpreadFor(Branches);
 const FBearing B=StationFix(F.Day(),Now,Branches,SaroX,SaroZ,ToX,ToZ,FS47NightRhythm::Roll(F.Seed(),F.Day(),0x5745454bu),Target);
 Fixes.Add(B);while(Fixes.Num()>MaxFixes)Fixes.RemoveAt(0);
 LastFixDay=F.Day();
 if(F.Director)F.Director->NotePointOut(Night);
 F.RecordEvidence(FString::Printf(TEXT("bearing_d%d"),F.Day()),FString::Printf(TEXT("The array resolves something off the main beam tonight, by itself: about %03.0f degrees from SARO, within %.0f, at %s. Penciled in on the map, and cold by noon. It says which way, not how far and not what."),B.Az,Spread,*SlipClock(Now)));
 const bool bSlip=F.QueueBearingSlip(B.Day,SlipLine(B),Branches<=0?FString(TEXT("NO BRANCH MENDED: A WIDE SECTOR."))
  :FString::Printf(TEXT("%d %s MENDED."),Branches,Branches==1?TEXT("BRANCH"):TEXT("BRANCHES")));
 if(M->UI)M->UI->Toast(FString::Printf(TEXT("%sSomething off the main beam: about %03.0f degrees, give or take %.0f. It's on your map (%s)."),bSlip?TEXT("PRINTER / A bearing slip. "):TEXT(""),B.Az,Spread,*FS47PlacesLocalMapKey()),6);
 UE_LOG(LogTemp,Display,TEXT("S47: bearing: %.0f within %.0f toward %s, %d branches, the week's case, night %d"),B.Az,Spread,*Target,Branches,Night);
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
 // LOOP-1-20261109: with no branch mended the sector is wide (±40), never none; every mended branch narrows it.
 Check(SpreadFor(0)==40&&SpreadFor(0)>SpreadFor(1)&&SpreadFor(1)>SpreadFor(2)&&SpreadFor(2)>SpreadFor(3)&&SpreadFor(3)>SpreadFor(4)&&SpreadFor(4)>0,TEXT("no branch mended is a wide sector, every mended branch narrows it, and none is exact"));
 {
  bool bStable=true,bNear=true,bMoves=false,bStill=true;
  for(int32 Day=1;Day<=120;++Day)
  {
   const FVector2D A=DriftOf(TEXT("stake"),47,Day);bStable&=A==DriftOf(TEXT("stake"),47,Day);bNear&=FMath::Abs(A.X)<=18&&FMath::Abs(A.Y)<=18;bMoves|=A.Size()>1;
   bStill&=DriftOf(TEXT("tank"),47,Day).IsZero()&&DriftOf(TEXT("mine"),47,Day).IsZero();
  }
  Check(bStable&&bNear&&bMoves&&bStill,TEXT("small things shift a little over the days, the same way in any Continue; what could not be carried stays"));
 }
 {
  // LOOP-1-20261109: a registered discrepancy gives a bearing and its slip with no branch mended; routine work still needs one.
  double Sx=0,Sz=0;SaroRoad(Sx,Sz);
  const double Clock=3*86400.+19*3600.+52*60.; // day 4, 19:52
  const FBearing B=StationFix(4,Clock,0,Sx,Sz,Sx+500,Sz,.5f,TEXT("windmill"));
  const FString Line=SlipLine(B);
  Check(Gives(true,0)&&!Gives(false,0)&&Gives(false,2)&&B.Spread==40&&B.Az==90&&Line==TEXT("BEARING ABOUT 090° ±40° / 19:52 / COLD BY NOON")&&InSector(B,Sx+400,Sz+200)&&!InSector(B,Sx,Sz+400),
   TEXT("a discrepancy gives a wide bearing and its slip with no branch mended, routine work needs a mended branch, and a place in the sector is in it"));
  // Cold by noon: the next noon, the pencil paling until then, and off the map once it is cold; an older one (no time) never is.
  FS47Places Map(nullptr);Map.Begin(nullptr);Map.Fixes.Add(B);
  FS47MapView Fresh,Later,Cold;Map.FillMapAt(Fresh,Clock+60);Map.FillMapAt(Later,ColdAt(Clock)-3600);Map.FillMapAt(Cold,ColdAt(Clock)+1);
  FBearing Old=B;Old.Clock=Old.Until=-1;
  Check(ColdAt(Clock)==4*86400.+12*3600.&&ColdAt(4*86400.+1*3600.)==4*86400.+12*3600.&&Live(B,ColdAt(Clock)-1)&&!Live(B,ColdAt(Clock))&&Live(Old,1e9)
   &&Fresh.Sectors.Num()==1&&Later.Sectors.Num()==1&&Fresh.Sectors[0].Fade>Later.Sectors[0].Fade&&Later.Sectors[0].Fade>=.3f&&Cold.Sectors.IsEmpty(),
   TEXT("a bearing is cold by the next noon: it pales on the map until then and is off it after"));
  // Story's numbers never on the paper: no 47 in a degree or a clock, no 8.15 or 20.15.
  bool bClean=true;
  for(int32 D=0;D<360;++D){FBearing X=B;X.Az=D;const FString L=SlipLine(X);bClean&=!L.Contains(TEXT("47"))&&Whole(D)!=47&&Whole(D)!=147&&Whole(D)!=247&&Whole(D)!=347;}
  for(int32 Min=0;Min<24*60;++Min){const FString C=SlipClock(Min*60.);bClean&=C.Len()==5&&!C.Contains(TEXT("47"))&&C!=TEXT("08:15")&&C!=TEXT("20:15");}
  Check(bClean&&SlipClock(20*3600.+15*60.)==TEXT("20:16")&&SlipClock(19*3600.+47*60.)==TEXT("19:48"),TEXT("the slip never prints a 47, 08:15 or 20:15: a degree or a minute on"));
  // The direction finder: a line from where the truck stands, into Fixes; one near the last toward the same thing replaces it;
  // two from different places cross, and the crossing is on the map. Two from SARO never cross.
  FS47Places Truck(nullptr);Truck.Begin(nullptr);
  const double Tx=Sx+200,Tz=Sz+900;
  const bool bFirst=Truck.AddCarFix(Tx,Tz,45,TEXT("omen:a"),Clock,4);
  const bool bSame=!Truck.AddCarFix(Tx+40,Tz-30,44,TEXT("omen:a"),Clock+120,4)&&Truck.Bearings().Num()==1&&Truck.Bearings()[0].Az==44;
  const bool bSecond=Truck.AddCarFix(Tx+1000,Tz,315,TEXT("omen:a"),Clock+600,4)&&Truck.Bearings().Num()==2;
  FVector2D At;const bool bCross=Cross(Truck.Bearings()[0],Truck.Bearings()[1],At);
  FS47MapView Two;Truck.FillMapAt(Two,Clock+700);
  const FBearing SaroA=StationFix(4,Clock,2,Sx,Sz,Sx+500,Sz,.5f,TEXT("mine")),SaroB=StationFix(5,Clock+86400,2,Sx,Sz,Sx,Sz+500,.5f,TEXT("mine"));
  FVector2D Never;
  Check(bFirst&&bSame&&bSecond&&Truck.Bearings()[0].bCar&&Truck.Bearings()[0].FromX==Tx+40&&bCross&&FVector2D::Distance(At,FVector2D(Tx+520,Tz-520))<60
   &&Two.Sectors.Num()==2&&Two.Sectors[0].bFrom&&Two.Crossings.Num()==1&&!Cross(SaroA,SaroB,Never),
   TEXT("the direction finder's bearing is from the truck, in the fixes, and two from different places cross on the map; two from SARO never do"));
  // A save keeps the new fields; an older bearing (no clock, from SARO) reads as before.
  FS47Places Kept(nullptr);Kept.Begin(S47J::Read(S47J::Write(Truck.Save())));
  const FBearing& K=Kept.Bearings()[0];
  // LOOP-4-20261109: the counter ticks at a bearing's place only while it is live; cold by noon, no dose. An older bearing (never
  // cold) still gives one; a target that is no place gives none.
  FBearing ToWindmill=B;ToWindmill.Target=TEXT("windmill");FBearing Nowhere=ToWindmill;Nowhere.Target=TEXT("omen:a");FBearing Older=ToWindmill;Older.Clock=Older.Until=-1;
  Check(Dosed(ToWindmill,Clock+60)&&!Dosed(ToWindmill,ColdAt(Clock))&&!Dosed(ToWindmill,ColdAt(Clock)+86400)&&Dosed(Older,1e9)&&!Dosed(Nowhere,Clock+60),
   TEXT("a cold bearing gives the counter no dose at its place; a live one does"));
  // The needle: the place's own reading when nothing happened near it, and one that will not settle when something did.
  const FString Calm=NeedleReading(TEXT("All steel; the needle follows the tower and nothing else."),false),Stirred=NeedleReading(TEXT("All steel; the needle follows the tower and nothing else."),true);
  Check(Calm==TEXT("All steel; the needle follows the tower and nothing else.")&&!Calm.Contains(TEXT("settle"))&&Stirred.StartsWith(Calm)&&Stirred.Contains(TEXT("will not settle"))&&!Stirred.ToLower().Contains(TEXT("alien")),
   TEXT("the needle is calm with nothing near and will not settle after something happened near"));
  Check(Kept.Bearings().Num()==2&&K.bCar&&FMath::IsNearlyEqual(K.FromX,Tx+40,.11)&&FMath::IsNearlyEqual(K.FromZ,Tz-30,.11)&&FMath::IsNearlyEqual(K.Clock,Clock+120,.5)&&FMath::IsNearlyEqual(K.Until,ColdAt(Clock),.5),
   TEXT("a bearing's clock, its noon and the truck's place survive a Continue"));
 }
 FS47Places None(nullptr);None.Begin(nullptr);Check(None.Save()->Values.Num()==0&&None.Bearings().IsEmpty(),TEXT("an older save has none of it and writes nothing"));
 auto O=S47J::New();S47J::Set(O,TEXT("heard"),TArray<FString>{TEXT("mine"),TEXT("nowhere")});
 auto Fix=S47J::New();S47J::Set(Fix,TEXT("day"),3);S47J::Set(Fix,TEXT("az"),31.);S47J::Set(Fix,TEXT("spread"),12.);S47J::Set(Fix,TEXT("target"),TEXT("mine"));
 TArray<TSharedPtr<FJsonValue>> List;List.Add(MakeShared<FJsonValueObject>(Fix));O->SetArrayField(TEXT("fixes"),List);
 FS47Places Back(nullptr);Back.Begin(S47J::Read(S47J::Write(O)));
 {
  double Sx=0,Sz=0;SaroRoad(Sx,Sz);const FBearing& Was=Back.Bearings()[0];
  Check(Back.Heard(TEXT("mine"))&&!Back.Heard(TEXT("nowhere"))&&Back.Bearings().Num()==1&&Was.Az==31.&&Was.Clock<0&&Was.Until<0&&!Was.bCar&&Was.FromX==Sx&&Was.FromZ==Sz&&Live(Was,1e9),
   TEXT("heard and bearings survive a Continue, an older bearing is from SARO and never cold, and nonsense is dropped"));
  // A bearing from the truck with no place to stand on is nonsense too.
  auto Bad=S47J::Clone(O);auto Car=S47J::Clone(Fix);S47J::Set(Car,TEXT("car"),true);TArray<TSharedPtr<FJsonValue>> L2;L2.Add(MakeShared<FJsonValueObject>(Car));Bad->SetArrayField(TEXT("fixes"),L2);
  FS47Places Dropped(nullptr);Dropped.Begin(S47J::Read(S47J::Write(Bad)));
  Check(Dropped.Bearings().IsEmpty(),TEXT("a bearing from the truck without where it stood is dropped"));
 }
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
