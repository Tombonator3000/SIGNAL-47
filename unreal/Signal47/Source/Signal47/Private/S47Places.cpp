#include "S47Places.h"
#include "S47Audio.h"
#include "S47Calm.h"
#include "S47Balloon.h"
#include "S47FarmHouse.h"
#include "S47FarmSite.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
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
#include "S47Predecessor.h"
#include "S47OpenLand.h"
#include "S47Relief.h"
#include "S47Script.h"
#include "S47RoadTerrain.h"
#include "S47Three.h"
#include "S47TownUse.h"
#include "S47MotelNight.h"
#include "S47UI.h"
#include "S47FieldLife.h"
#include "S47Walker.h"
#include "SS47FieldMap.h"
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
// MAP-D62-20261112 (Tom, d62: a slip printed at 11:36 had its circle gone from the map at 12:00, when its line went cold): a slip's circle
// that has not been stood in stays on the sheet a day past the line's noon, so it is there every time the map opens until it is looked
// at. Its line still goes at noon, as the slip says. A slip with no time (an older save's) is never cold anyway.
static bool FS47PlacesLocalCircleOnSheet(const FS47Places::FBearing& B,double Now)
{
 if(B.bCar||!B.bRing)return false;
 if(FS47Places::Live(B,Now))return true;
 return !B.bVisited&&B.Until>=0&&Now<B.Until+86400.;
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
  // D48-20261110: the balloon's model where Tools/balloon imported it (FS47Balloon draws every piece then, and the place's "look closer"
  // with it); otherwise the rubber, the chute and the cord as basic shapes. The instrument box with its beacon is always FS47Balloon's.
  if(Kind==TEXT("balloon"))return FS47Balloon::HaveModel()?TArray<FPart>{}:TArray<FPart>{{Ball,{0,.1,0},{3,.16,2.2},Pale,20,0},{Ball,{1.8,.08,1.2},{1.6,.1,.9},Pale,70,0},{Cube,{-.8,.02,-.6},{1.9,.015,.015},Dark,38,0}};
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
  // T16-17 (→ M1): the old house's two things, on its own floor (S47FarmHouse::PadY), x east and z south as ever: the plan's x runs
  // west and its y north (S47FarmHouse.cpp). The tally book on the kitchen table where the imported house has its table (build_house.py:
  // plan 1.55 to 2.55 by 6.75 to 7.75, top at .76), clear of its cup and paper; the block house has no table, so one in basic shapes.
  if(Kind==TEXT("tally book"))
  {
   TArray<FPart> P={{Cube,{-.2,.775,.25},{.3,.03,.22},0x2f3a2c,8,0},{Cube,{-.21,.792,.25},{.27,.004,.19},Pale,8,0},{Cube,{-.02,.766,.34},{.14,.008,.008},Flag,32,0}};
   if(!S47FarmHouse::HasModel()){P.Add({Cube,{0,.74,0},{1,.04,1},Wood,0,0});for(int32 I=0;I<4;++I)P.Add({Cube,{(I&1)?.45:-.45,.36,(I&2)?.45:-.45},{.045,.72,.045},Wood,0,0});}
   return P;
  }
  // A kitchen chair carried into the back room and set square to the north window (road +z), between the steel shelves and clear of
  // the hall door's swing; a coffee can beside it; a strip of pencilled times on the window ledge (the sill at .9, plan 8.0 by .35).
  if(Kind==TEXT("chair at the window"))
  {
   TArray<FPart> P={{Cube,{0,.45,0},{.42,.04,.42},Wood,0,0},{Cube,{0,.72,-.2},{.42,.5,.04},Wood,0,0},{Cyl,{.4,.09,-.1},{.16,.18,.16},Rust,0,0},{Cube,{.3,.905,.65},{.3,.01,.06},Dark,0,0}};
   for(int32 I=0;I<4;++I)P.Add({Cube,{(I&1)?.18:-.18,.22,(I&2)?.18:-.18},{.035,.44,.035},Wood,0,0});
   return P;
  }
  return {};
 }
 // D48-20261110 (D46-18, Tom: at the windmill and "nothing there"): what the first day shows there, from shapes the place already
 // uses. The head turned toward the bearing Az (compass degrees; the wheel and the vane are the parts 6 and 7 of the windmill), a bright
 // new wire down from the vane, two tyre tracks that stop short of the tower, a ring of flattened brush. Returns where the traces begin.
 int32 MillTraces(TArray<FPart>& P,double Az)
 {
  const uint32 Wire=0xe9e4d2,Track=0x3b3126,Flat=0xa89868,Bare=0x9c8b66;
  const double Rad=FMath::DegreesToRadians(Az),Sx=FMath::Sin(Rad),Sz=-FMath::Cos(Rad); // d: toward the bearing, x east and z south
  const auto Along=[Sx,Sz](double Out,double Side,double Y,const FVector& Size,uint32 Colour,float Yaw)->FPart{return {Cube,{Sx*Out+Sz*-Side,Y,Sz*Out+Sx*Side},Size,Colour,Yaw,0};};
  if(P.Num()>7)
  {
   P[6].At=FVector(Sx*.2,9.9,Sz*.2);P[6].Yaw=float(Az);
   P[7].At=FVector(-Sx*.9,9.9,-Sz*.9);P[7].Yaw=float(Az);
  }
  const int32 From=P.Num();
  // the new wire: a bright line down from the tail to the tower leg, and bright turns round the vane's root and the leg
  P.Add(Along(-1.6,0,7.9,{.03,4.1,.03},Wire,float(Az)));
  P.Add(Along(-.5,0,10.3,{.035,.035,1.7},Wire,float(Az)));
  P.Add(Along(-.2,0,9.9,{.16,.16,.16},Wire,float(Az)));
  P.Add({Cube,{-1.1,6.6,-1.1},{.2,.1,.2},Wire,0,0});
  // a ring of flattened brush round the tower, laid all one way, and the bare ground it left
  P.Add({Cyl,{0,.03,0},{16,.06,16},Bare,0,0});
  for(int32 I=0;I<18;++I){const double A=I*UE_DOUBLE_PI/9;P.Add({Cube,{8.2*FMath::Cos(A),.07,8.2*FMath::Sin(A)},{2.2,.07,.55},Flat,float(FMath::RadiansToDegrees(A)+90.),0});}
  // two tyre tracks coming in from the SARO side, stopping all at once about eight metres short of the tower
  for(int32 Side=-1;Side<=1;Side+=2)P.Add(Along(-17,Side*.95,.05,{.28,.1,18},Track,float(Az)));
  P.Add(Along(-8,0,.04,{3.2,.08,.5},Track,float(Az)));
  return From;
 }
 FLinearColor Tone(uint32 Hex){return S47Three::Hex(Hex);}
 const TCHAR* const Motel=TEXT("motel");
}

const TArray<FS47Places::FDef>& FS47Places::Defs()
{
 static const TArray<FDef> Table={
  {TEXT("windmill"),TEXT("the windmill on the Baca lease"),TEXT("windmill"),170,720,TEXT("A stock windmill with its brake on and no stock near it. Someone has wired the tail vane so the wheel faces a little east of north-northeast. The wire is new. The pump rod is rusted solid."),TEXT("All steel; the needle follows the tower and nothing else.")},
  {TEXT("tank"),TEXT("the water tank on the old grade"),TEXT("tank"),-190,1010,TEXT("A corrugated stock tank, a third full. The water has a skin on it and nothing drinks here: no prints at the rim in any direction, and it is the only water for miles. Rivets on the old riser carry a railroad's initials and an 1890s date. The rails were never laid; the grade runs west and stops."),TEXT("The needle swings as you walk around the tank, as it should. It takes longer to settle on the far side.")},
  {TEXT("mine"),TEXT("the Esperanza mine on the west side"),TEXT("mine"),-560,1340,TEXT("The Esperanza mine, the deepest working of the silver years. An adit with its boards pulled off and laid in a neat stack. Cold air comes out of it at a steady rate. Old survey tags on the timbers, the dates weathered off. Nobody has mined anything here since."),TEXT("Ore in the tailings pulls the needle; the heading wanders. It wanders less at the mouth than it should.")},
  {TEXT("balloon"),TEXT("the balloon remains east of the highway"),TEXT("balloon"),300,1270,TEXT("Neoprene and a parachute of orange cloth, and a cardboard instrument box with a weather service return label. It has been out here a while. Ordinary, as far as it goes. The label asks the finder to mail it in, and the box is empty."),TEXT("Nothing. A cardboard box, a wire and some rubber.")},
  {TEXT("cow"),TEXT("the dead cow by the fence"),TEXT("cow"),110,1590,TEXT("A dead cow with a brand from the Alderete place, a long way from it. No scavengers have touched her and the flies have not found her. She has been dead a few days by the smell. The ground under her is dry."),TEXT("The needle dips over her, about as much as for a belt buckle. There is a tag in her ear and that may be all it is.")},
  {TEXT("stake"),TEXT("the survey stake off the track"),TEXT("stake"),-130,650,TEXT("A hub stake with orange flagging, freshly driven, and a witness stake beside it. Written on it in grease pencil: an arrow, and a bearing of 026. There is no line of stakes. There is just this one."),TEXT("A nail in the hub moves the needle a hair. Nothing else.")},
  {TEXT("shack"),TEXT("the line shack"),TEXT("shack"),440,930,TEXT("A line camp, one room, with a cot frame and a calendar for 1971. Coffee in a can, gone solid. Someone has been keeping a tally on the door frame in pencil, in fours and a stroke. The last group stops at three."),TEXT("Stove, nails, a bed frame. The needle has plenty to say and none of it is anything.")},
  {TEXT("trough"),TEXT("the salt lick and trough"),TEXT("trough"),240,1740,TEXT("A rusted trough and a block of stock salt, licked hollow on one side only. The hoof prints all stop about ten feet short of the trough, and go back the way they came."),TEXT("Iron in the trough. The needle settles on it from any side.")},
  {TEXT("cairn"),TEXT("the cairn on the rise"),TEXT("cairn"),-320,430,TEXT("A stack of stones with a post in the top, the kind a survey party leaves. From here you can see SARO and the farm both. The stones are newer than the post. Someone has rebuilt it recently, carefully, facing the array."),TEXT("A steady reading, nothing to speak of. It stays steady for longer than you would think a needle could.")},
  // PT-SAUCER-ROCK-20261109: on the souvenir man's map, and now on the land too, where he drew it (FS47TownUse::SaucerMap).
  {TEXT("saucer_rock"),TEXT("Saucer Rock"),TEXT("saucer rock"),-760,2240,TEXT("A pillar of pale rock taller than two men with a wide flat cap on it, a cup and saucer. Initials cut in the stem back to 1951, a fire ring, cans. Round its foot someone has set a ring of stones and swept the sand inside it smooth. Nothing has crossed it."),TEXT("The cap pulls the needle a few degrees. Walk round it and the pull follows the rock, as it should.")},
  // T16-17 (→ M1, "the farmhouse is an empty shell"): two things inside the old house, at the plan's (2.05, 7.25) and (8.3, 1.0) in
  // the land's metres (S47FarmHouse::Point; the self-test holds them to it). The kitchen near the front door, the back room by the back
  // door: neither is a dead end with one way out (AGENTS.md, «Fast regel: undersøkelse, sniking, jakt og flukt i bygg»).
  {TEXT("kitchen"),TEXT("the kitchen of the old house"),TEXT("tally book"),101.95,217.75,TEXT("A ranch tally book on the kitchen table, years of mornings in one careful hand: the windmill pumping or not, the level in the stock tank. In the last pages the hand gets smaller and the water is gone from them. Only the line shack and the tank, each with a clock time after dark, and nothing said of why."),TEXT("The stove and the icebox pull it about. Nothing of its own.")},
  {TEXT("backroom"),TEXT("the back room of the old house"),TEXT("chair at the window"),95.7,224,TEXT("Someone carried a kitchen chair in here and set it square to the north window, looking out over the pasture. The seat has less dust on it than the shelves. Pencilled on the sill, a column of clock times, every one after midnight. Beside the chair, a coffee can of cigarette ends."),TEXT("Steel shelves on both walls. The needle sits between them and says nothing of its own.")},
  {Motel,TEXT("the Sierra Motor Court"),Motel,0,0,TEXT("Sierra Motor Court office. The night bell works and the key rack has more keys than the lot has cars. The card on the counter says rooms are eight dollars, and quiet."),TEXT("Nothing but the bell.")},
 };
 return Table;
}
const FS47Places::FDef* FS47Places::Def(const FString& Id){return Defs().FindByPredicate([&](const FDef& D){return Id==D.Id;});}
bool FS47Places::bHasFound(const FString& Id) const{return M&&M->Freeform&&M->Freeform->HasFinding(TEXT("place_")+Id);}
bool FS47Places::Checked(const FBearing& B) const
{
 if(B.bVisited)return true;
 if(!Def(B.Target)||!bHasFound(B.Target))return false;
 // T16-17 (→ M1): a place already found, pointed at for the trace the next morning leaves there: checked once that trace is kept
 if(M&&M->Freeform&&!DayTrace(B.Target,M->Freeform->Seed(),B.Day+1).IsEmpty())return M->Freeform->HasFinding(FString::Printf(TEXT("place_trace_d%d"),B.Day+1));
 return true;
}
FString FS47Places::TracePlace(int32 Seed,int32 Day)
{
 for(const FDef& D:Defs())if(FString(D.Id)!=Motel&&!DayTrace(D.Id,Seed,Day).IsEmpty())return D.Id;
 return FString();
}
double FS47Places::AzimuthOf(double FromX,double FromZ,double ToX,double ToZ)
{
 const double A=FMath::RadiansToDegrees(FMath::Atan2(ToX-FromX,-(ToZ-FromZ)));
 return A<0?A+360:A;
}
bool FS47Places::ReadFix(const S47J::FObj& O,FBearing& B)
{
 if(!O.IsValid())return false;
 B=FBearing();
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
  if(From.Num()!=2||!FMath::IsFinite(From[0])||!FMath::IsFinite(From[1])||!S47OpenLand::InRim(From[0],From[1]))return false;
  B.FromX=From[0];B.FromZ=From[1];
 }
 else
 {
  // D45-20261110: what the slip points at and its circle, both on the land; an older save has neither (the wedge alone).
  const TArray<double> To=S47J::Nums(O,TEXT("to")),Ring=S47J::Nums(O,TEXT("ring"));
  if(To.Num()==2&&FMath::IsFinite(To[0])&&FMath::IsFinite(To[1])&&S47OpenLand::InRim(To[0],To[1])){B.bTo=true;B.ToX=To[0];B.ToZ=To[1];}
  if(Ring.Num()==3&&FMath::IsFinite(Ring[0])&&FMath::IsFinite(Ring[1])&&FMath::IsFinite(Ring[2])&&S47OpenLand::InRim(Ring[0],Ring[1])&&Ring[2]>50&&Ring[2]<=600)
  {B.bRing=true;B.RingX=Ring[0];B.RingZ=Ring[1];B.RingR=Ring[2];B.bVisited=S47J::Bool(O,TEXT("visited"));}
 }
 return FMath::IsFinite(B.Az)&&FMath::IsFinite(B.Spread)&&B.Az>=0&&B.Az<=360&&B.Spread>0&&B.Spread<=45;
}
S47J::FObj FS47Places::WriteFix(const FBearing& B)
{
 auto X=S47J::New();S47J::Set(X,TEXT("day"),B.Day);S47J::Set(X,TEXT("az"),B.Az);S47J::Set(X,TEXT("spread"),B.Spread);S47J::Set(X,TEXT("target"),B.Target);
 if(B.Clock>=0)S47J::Set(X,TEXT("clock"),B.Clock);
 if(B.Until>=0)S47J::Set(X,TEXT("until"),B.Until);
 if(B.bCar){S47J::Set(X,TEXT("car"),true);S47J::Set(X,TEXT("from"),TArray<double>{FMath::RoundToDouble(B.FromX*10)/10,FMath::RoundToDouble(B.FromZ*10)/10});}
 // D45-20261110: new keys, written only when there is something (an older save reads as before)
 if(B.bTo)S47J::Set(X,TEXT("to"),TArray<double>{FMath::RoundToDouble(B.ToX*10)/10,FMath::RoundToDouble(B.ToZ*10)/10});
 if(B.bRing){S47J::Set(X,TEXT("ring"),TArray<double>{FMath::RoundToDouble(B.RingX*10)/10,FMath::RoundToDouble(B.RingZ*10)/10,FMath::RoundToDouble(B.RingR*10)/10});if(B.bVisited)S47J::Set(X,TEXT("visited"),true);}
 return X;
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
  if(!V||V->Type!=EJson::Object)continue;FBearing B;
  if(ReadFix(V->AsObject(),B)&&Fixes.Num()<MaxFixes){Fixes.Add(B);if(!B.bCar)LastFixDay=FMath::Max(LastFixDay,B.Day);}
 }
 // LOOP-SLIP-IN-PACKET-20261113: a discrepancy's bearing waiting for its packet (an older save has none: its slip is on the printer)
 Held.Reset();
 for(const auto& V:S47J::Arr(Saved,TEXT("held")))
 {
  if(!V||V->Type!=EJson::Object||Held.Num()>=MaxHeld)continue;const auto O=V->AsObject();FHeldFix H;
  const auto Fix=S47J::Obj(O,TEXT("fix"));H.Info.Job=S47J::Str(O,TEXT("job")).Left(64);
  if(H.Info.Job.IsEmpty()||!Fix.IsValid()||!ReadFix(Fix,H.B)||H.B.bCar)continue;
  H.Info.Branches=FMath::Clamp(S47J::Int(O,TEXT("branches")),0,27);H.Info.Steps=FMath::Clamp(S47J::Int(O,TEXT("steps")),0,2);H.Info.Why=uint8(FMath::Clamp(S47J::Int(O,TEXT("why")),0,7));
  H.Info.bExtra=S47J::Bool(O,TEXT("extra"));H.Info.bSame=S47J::Bool(O,TEXT("same"));H.Info.bOwnMarks=S47J::Bool(O,TEXT("own"));
  const TArray<double> Mk=S47J::Nums(O,TEXT("marks"));
  if(H.Info.bOwnMarks&&Mk.Num()==2&&FMath::IsFinite(Mk[0])&&FMath::IsFinite(Mk[1])&&Mk[0]>=0&&Mk[1]>Mk[0]&&Mk[1]<=18){H.Info.MarkIn=Mk[0];H.Info.MarkOut=Mk[1];}else H.Info.bOwnMarks=false;
  Held.Add(H);LastFixDay=FMath::Max(LastFixDay,H.B.Day);
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
  for(const FBearing& B:Fixes)List.Add(MakeShared<FJsonValueObject>(WriteFix(B)));
  O->SetArrayField(TEXT("fixes"),List);
 }
 if(!Held.IsEmpty())
 {
  // LOOP-SLIP-IN-PACKET-20261113 (optional; an older build ignores it)
  TArray<TSharedPtr<FJsonValue>> List;
  for(const FHeldFix& H:Held)
  {
   auto X=S47J::New();S47J::Set(X,TEXT("job"),H.Info.Job);X->SetObjectField(TEXT("fix"),WriteFix(H.B));S47J::Set(X,TEXT("branches"),H.Info.Branches);
   if(H.Info.Steps>0)S47J::Set(X,TEXT("steps"),H.Info.Steps);
   if(H.Info.Why)S47J::Set(X,TEXT("why"),int32(H.Info.Why));
   if(H.Info.bExtra)S47J::Set(X,TEXT("extra"),true);
   if(H.Info.bSame)S47J::Set(X,TEXT("same"),true);
   if(H.Info.bOwnMarks){S47J::Set(X,TEXT("own"),true);S47J::Set(X,TEXT("marks"),TArray<double>{H.Info.MarkIn,H.Info.MarkOut});}
   List.Add(MakeShared<FJsonValueObject>(X));
  }
  O->SetArrayField(TEXT("held"),List);
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
 TArray<FString> Left;for(const FDef& D:Defs())if(!AtSaro(D.Id)&&!bHasFound(D.Id)&&!HeardOf.Contains(D.Id))Left.Add(D.Id);
 if(Left.IsEmpty())return FString();
 const int32 Seed=M&&M->Freeform?M->Freeform->Seed():47;
 return Left[int32(FS47NightRhythm::Roll(Seed,Salt,0x504c4143u)*Left.Num())%Left.Num()];
}
FString FS47Places::ArcPlace(int32 Seed)
{
 // D45-20261110 (D45-26): the windmill while it stands on the land; else one with a survey tag, by the seed (Saucer Rock has none).
 TArray<FString> Fit;
 for(const FDef& D:Defs())
 {
  const FString Id=D.Id;
  if(AtSaro(Id)||Id==TEXT("cairn")||Id==TEXT("saucer_rock")||!S47OpenLand::InRim(D.X,D.Z,60))continue;
  if(Id==TEXT("windmill"))return Id;
  Fit.Add(Id);
 }
 if(Fit.IsEmpty())return FString();
 return Fit[int32(FS47NightRhythm::Roll(Seed,1,0x41524350u)*Fit.Num())%Fit.Num()];
}
double FS47Places::ArcBearing(int32 Seed,double FromX,double FromZ,double ToX,double ToZ,double Spread)
{
 const double Most=FMath::Max(0.,.45*Spread);
 const double Off=FMath::Clamp((double(FS47NightRhythm::Roll(Seed,1,0x41524342u))-.5)*10.,-Most,Most);
 return Whole(AzimuthOf(FromX,FromZ,ToX,ToZ)+Off);
}
bool FS47Places::Spot(const FString& Id,double& OutX,double& OutZ) const
{
 const FBuilt* B=Built.FindByPredicate([&](const FBuilt& X){return X.Id==Id;});
 if(B){OutX=B->X;OutZ=B->Z;return true;}
 const FDef* D=Def(Id);if(!D)return false;OutX=D->X;OutZ=D->Z;return true;
}
bool FS47Places::Stand(const FString& Id,FVector& OutWorld,bool& bOutSeated) const
{
 // D48-20261110: where a built place stands with its ground (Seat), for what else is drawn there (FS47Balloon).
 const FBuilt* B=Built.FindByPredicate([&](const FBuilt& X){return X.Id==Id;});if(!B)return false;
 const FVector Road=S47Legs::ORIGINS(TEXT("road"));
 if(InOldHouse(B->Id)){OutWorld=S47OpenLand::FromRoad(TEXT("saro"),B->X,B->Z);OutWorld.Y=B->Y;bOutSeated=B->bSeated;return true;} // T16-17: the land's metres on SARO's ground
 OutWorld=B->Id==Motel?FVector(B->X,B->Y,B->Z):FVector(Road.X+B->X,B->Y,Road.Z+B->Z);bOutSeated=B->bSeated;return true;
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
  const bool bMotel=FString(D.Id)==Motel,bSaro=AtSaro(D.Id);
  // T16-17 (→ M1): the old house's two stand inside it, where the table says (Defs), on SARO's ground: neither branch below
  if(bMotel)
  {
   const FS47Box2& Park=S47Legs::MOTEL_PARK();const FVector At=FVector(Park.MinX-1.2,0,Park.MinZ+2);
   B.X=At.X;B.Z=At.Z;
  }
  else if(!InOldHouse(D.Id))
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
  TArray<FPart> Shapes=PartsOf(D.Kind);
  // D48-20261110: on the first days the windmill shows what was done to it, its head turned toward the bearing from SARO
  if(M->Freeform&&Traced(D.Id,M->Freeform->Seed(),M->Freeform->Day()))
  {
   double Sx=0,Sz=0;SaroRoad(Sx,Sz);
   B.TraceFrom=MillTraces(Shapes,ArcBearing(M->Freeform->Seed(),Sx,Sz,B.X,B.Z,SpreadFor(0)));
  }
  for(int32 Index=0;Index<Shapes.Num();++Index)
  {
   const FPart& P=Shapes[Index];
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
    B.Hit=M->Web->ObjAt(M->Web->AdoptNode(C,bSaro?SaroRoot:RoadRoot,UseId(D.Id)));bFirst=false;
   }
   else if(FString(D.Kind)!=TEXT("cow"))
   {
    // The whole thing can be pointed at, not just its first leg.
    C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);C->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
    const FS47Obj More=M->Web->ObjAt(M->Web->AdoptNode(C,bSaro?SaroRoot:RoadRoot,Name));if(More)M->Web->MarkUsable(More.I,UseId(D.Id));
   }
   B.Parts.Add(C);B.Local.Add(P.At);
  }
  if(B.Hit)
  {
   const FString Id=D.Id;
   FS47Interactable I;I.Id=UseId(D.Id);I.Object=B.Hit;I.Range=bSaro?2.6:6;
   // D48-20261110 (D46-18): the windmill on the first days says what to do, and is reached from well off (the whole wheel and tower and
   // the ground round it are the target)
   const bool bMill=Id==TEXT("windmill");if(bMill)I.Range=14;
   I.Label=[this,Guard,Id,bMill]{
    if(!Guard.IsValid())return FString();
    if(bMill&&M&&M->Freeform&&Traced(Id,M->Freeform->Seed(),M->Freeform->Day())&&!bHasFound(Id))return FString(TEXT("Look at the windmill"));
    return FString::Printf(TEXT("%s / look closer"),Def(Id)->Title);};
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
 // T16-17 (→ M1): the old house's floor is level and known (S47FarmHouse::PadY); its two things stand on it in SARO's area
 if(InOldHouse(B.Id))
 {
  const FVector OnSaro=S47OpenLand::FromRoad(TEXT("saro"),B.X,B.Z);
  B.Y=S47FarmHouse::PadY();B.bSeated=true;
  for(int32 I=0;I<B.Parts.Num();++I)if(auto* C=B.Parts[I].Get())C->SetWorldLocation(S47Three::ToUE(FVector(OnSaro.X,B.Y,OnSaro.Z)+B.Local[I]));
  return;
 }
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
  if(InOldHouse(B.Id))continue; // T16-17: seated once, never moves
  const FVector World=B.Id==Motel?FVector(B.X,0,B.Z):FVector(Road.X+B.X,0,Road.Z+B.Z);
  const double Away=FVector2D(M->Player->Pos.X-World.X,M->Player->Pos.Z-World.Z).Size();
  if(!B.bSeated&&Away<300)Seat(B);
  // D48-20261110 (D46-18): the traces are for the first days; after them the windmill stands as it was. The head stays as it is turned.
  if(B.TraceFrom>=0)
  {
   const bool bOn=M->Freeform->Day()<=2;
   if(bOn!=B.bTraceOn){B.bTraceOn=bOn;for(int32 I=B.TraceFrom;I<B.Parts.Num();++I)if(auto* C=B.Parts[I].Get())C->SetVisibility(bOn);}
   // arriving at the first one: a thought of the player's own, once (not once found: then the record has it)
   if(bOn&&!bMillThought&&Away<40&&!bHasFound(B.Id)&&M->UI)
   {
    bMillThought=true;
    M->UI->Thought(TEXT("Why would anyone tie it to point there?"),5.f);
    UE_LOG(LogTemp,Display,TEXT("S47: windmill: the first day's traces, the player is near"));
   }
  }
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
 StepSlips(Dt);
}
void FS47Places::StepSlips(double Dt)
{
 // D45-20261110: a live slip's circle is checked once the player (on foot or at the wheel) stands inside it; NOW ticks it off.
 SlipIn-=Dt;if(SlipIn>0)return;SlipIn=.5;
 double Px=0,Pz=0,Facing=0;FString Area;bool bWheel=false;
 if(!S47FieldMap::Me(M,Px,Pz,Facing,Area,bWheel))return;
 const double Now=M->Freeform->Clock();
 for(FBearing& B:Fixes)
  if(!B.bCar&&B.bRing&&!B.bVisited&&FS47PlacesLocalCircleOnSheet(B,Now)&&FVector2D::Distance(FVector2D(Px,Pz),FVector2D(B.RingX,B.RingZ))<B.RingR)
  {
   B.bVisited=true;
   if(M->UI)M->UI->Toast(TEXT("BEARING SLIP / You are inside the circle on your map. Look around here."),5);
   UE_LOG(LogTemp,Display,TEXT("S47: bearing slip day %d: the player is inside its circle at %.0f %.0f"),B.Day,Px,Pz);
   M->RequestAutosave();
  }
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
  if(bHasFound(B.Id)||AtSaro(B.Id))continue;
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
 Rows.Add(S47Kit::Small(bFirst?TEXT("Kept as a field record: what you found here, and when."):TEXT("Already in your field records."))); // D45-20261110: plain words
 auto* F=M->Freeform.Get();
 // T16-17 (→ M3/M7): a page of Reyes's notebook left here, when its turn has come (S47PredecessorBook.cpp).
 if(F->Predecessor)F->Predecessor->PlaceRows(D.Id,Rows);
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
 else if(F->HasKit(TEXT("cot"))&&!InOldHouse(D.Id)) // T16-17: no cot, trip camera or salt in the old house
 {
  Rows.Add(S47Kit::Small(TEXT("The cot is in the truck. Out here, anything close by will wake you.")));
  Rows.Add(S47Kit::Button(TEXT("Put up the cot here until morning"),[this,F,Next]{if(M->UI)M->UI->Close(true);F->RestAt(Next,FS47FreeformSession::ERest::Field);},true));
 }
 if(F->TripCamera&&F->TripCamera->CanSet()&&!AtSaro(D.Id))
 {
  const FString Where=D.Title;
  Rows.Add(S47Kit::Button(TEXT("Leave the trip camera here until morning"),[this,F,Where]{F->TripCamera->SetHere(Where,false);if(M->UI)M->UI->Close(true);}));
 }
 // Salt or range cubes from the Feed & Seed, set out here, and what came to them (TOWN-USE-20261108, S47TownUse).
 if(F->TownUse&&!AtSaro(D.Id))F->TownUse->LureRows(D.Id,Rows);
 M->UI->Open(S47Kit::Shell(M->UI,FString(D.Title).ToUpper(),S47Kit::Col(Rows),760));
}
bool FS47Places::Use(const FString& Id)
{
 const FDef* D=Def(Id);if(!D||!M||!M->UI||!M->Freeform)return false;auto& F=*M->Freeform;
 const FString Key=TEXT("place_")+Id;const bool bFirst=!F.HasFinding(Key);
 if(bFirst)
 {
  FString Page=FString(D->Finding);if(!Page.Contains(TEXT("records")))Page+=TEXT(" This records what is here.");
  // D48-20261110 (D46-18): what the first day shows there, in the record: the new wire, tracks that stop, brush laid flat
  if(Traced(Id,F.Seed(),F.Day()))
  {
   // the finding's own words for where the wheel faces are the turned head's: toward the bearing from SARO
   double Sx=0,Sz=0,Mx=0,Mz=0;SaroRoad(Sx,Sz);
   if(Spot(Id,Mx,Mz))Page.ReplaceInline(TEXT("a little east of north-northeast"),*CompassWord(ArcBearing(F.Seed(),Sx,Sz,Mx,Mz,SpreadFor(0))));
   Page+=TEXT(" Tire tracks come in from the array's side and stop all at once, short of the tower. The brush is laid flat in a ring around it, and the wire on the tail is bright and clean.");
  }
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
// N1110 (rhythm, item 12): by day, the place may carry the night's one unexplained trace; said once a day, kept once.
 bool bNews=!Extra.IsEmpty();
 {
  const double Hd=FMath::Fmod(F.Clock(),86400.)/3600.;const FString Trace=DayTrace(Id,F.Seed(),F.Day());
  if(!Trace.IsEmpty()&&Hd>=6.&&Hd<19.)
  {
   bNews=true;
   Extra+=(Extra.IsEmpty()?FString():TEXT("\n"))+Trace;
   F.RecordEvidence(FString::Printf(TEXT("place_trace_d%d"),F.Day()),FString::Printf(TEXT("At %s: %s"),D->Title,*Trace));
  }
 }
 // T16-17 (→ M1): the tally book names two places out on the land, and puts them on the map as the radio's would (a question mark)
 if(Id==TEXT("kitchen"))
 {
  int32 Marked=0;
  for(const TCHAR* Named:{TEXT("tank"),TEXT("shack")})if(!bHasFound(Named)&&!HeardOf.Contains(Named)){Hear(Named);++Marked;}
  if(Marked>0)Extra+=(Extra.IsEmpty()?FString():TEXT("\n"))+FString::Printf(TEXT("The book names %s out on the land. %s pencilled on your map (%s) with a question mark."),Marked==1?TEXT("a place"):TEXT("two places"),Marked==1?TEXT("It is"):TEXT("They are"),*FS47PlacesLocalMapKey());
 }
 // T16-17 (→ M1): the back room after something was near the farm in the last two nights (FS47Happenings::StirredNear, the needle's
 // own history), by daylight and once looked at before: the dust says somebody sat there again. A record for the visitors' case
 // (its key ends _daylight_dN: FS47FreeformCases::LineForFinding), kept once a day.
 if(Id==TEXT("backroom")&&!bFirst)
 {
  const double Hd=FMath::Fmod(F.Clock(),86400.)/3600.;double Hx=0,Hz=0;
  if(Hd>=6.&&Hd<19.&&F.Happenings&&Spot(Id,Hx,Hz)&&F.Happenings->StirredNear(Hx,Hz))
  {
   bNews=true;
   Extra+=(Extra.IsEmpty()?FString():TEXT("\n"))+BackRoomAgain();
   F.RecordEvidence(FString::Printf(TEXT("%s_daylight_d%d"),*Key,F.Day()),FString::Printf(TEXT("The back room of the old house, by daylight. %s"),*BackRoomAgain()));
  }
 }
 if(F.HasKit(TEXT("magnetometer")))
 {
  // LOOP-4-20261109 (HELHETLIG_LOKKE step 4): the needle has a history. Where something happened near the place in the last two
  // nights it will not settle, whatever any bearing says; otherwise the place's own reading. The unsettled needle is a record of OUT
  // ON THE LAND, once a day a place (FS47FreeformCases::LineForFinding).
  double Px=0,Pz=0;const bool bStirred=F.Happenings&&Spot(Id,Px,Pz)&&F.Happenings->StirredNear(Px,Pz);
  Extra+=(Extra.IsEmpty()?FString():TEXT("\n"))+TEXT("MAGNETOMETER / ")+NeedleReading(D->Needle,bStirred);bNews|=bStirred;
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
 // T16-17 (→ M1, "the places are reading stations"): a second look with nothing new says so, and what can change that (Saucer Rock
 // always has its hour to tell)
 // T16-17 (→ M3/M7): a page of Reyes's notebook lying here now is something new (FS47Predecessor::PlaceRows shows it).
 bNews|=F.Predecessor&&F.Predecessor->PlaceNews(Id);
 if(!bFirst&&!bNews&&Id!=Motel&&Id!=TEXT("saucer_rock"))Extra+=(Extra.IsEmpty()?FString():TEXT("\n"))+NothingNew(InOldHouse(Id));
 Panel(*D,bFirst,Extra);return true;
}
FString FS47Places::DayTrace(const FString& Id,int32 Seed,int32 Day)
{
 if(Day<2)return FString();
 TArray<const FDef*> Open;
 for(const FDef& D:Defs())if(FCString::Strcmp(D.Id,TEXT("cairn"))!=0&&FCString::Strcmp(D.Id,TEXT("saucer_rock"))!=0&&!InOldHouse(D.Id))Open.Add(&D); // (T16-17: the house's two are not the land's, and the days' picks stay as they were)
 if(Open.IsEmpty())return FString();
 const int32 Pick=int32(FS47NightRhythm::Roll(Seed,Day,0x54524331u)*Open.Num())%Open.Num();
 if(Id!=Open[Pick]->Id)return FString();
 switch((Day+int32(FS47NightRhythm::Roll(Seed,0,0x54524332u)*6.f))%6)
 {
 case 0:return TEXT("A line of bare tracks comes across the dirt here and stops in the open. Nothing leads away from the last one.");
 case 1:return TEXT("A circle of grass about the size of a table has died here, brown and dry to the root. The grass around it is green.");
 case 2:return TEXT("The wire of the fence nearby has been cut clean, both ends bright. There are no marks from pliers on either one.");
 case 3:return TEXT("A dozen birds sit on the rail and the wire, and every one of them faces the same way. They do not turn their heads when you come near.");
 case 4:return TEXT("A patch of sand has been fused glassy in a ring about the size of a plate. It is still smooth, and nothing else near it is burned.");
 default:return TEXT("A cow stands alone on the open ground, facing the station. She does not move when you come near.");
 }
}
FString FS47Places::BackRoomAgain()
{
 return TEXT("Since you were last here the dust has been walked through: from the hall door to the chair and back. The chair is turned a little further toward the window, and there is one more time pencilled on the sill.");
}
FString FS47Places::NothingNew(bool bHouse)
{
 return bHouse?TEXT("Nothing has changed here since you last looked. After a night with something near the farm, look again by daylight.")
  :TEXT("Nothing new here since you last looked. A bearing slip, or a night's trace in daylight, can bring you back.");
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
 // D45-20261110 (Tom: "for rart skrevet"): the direction in plain words, its compass point, and when it runs out; no degree sign.
 const FString Az=FString::Printf(TEXT("%03.0f"),Whole(B.Az));
 const FString Spread=FString::Printf(TEXT("%.0f"),FMath::Clamp(FMath::RoundToDouble(B.Spread),1.,45.));
 const FString Way=S47FieldMap::Compass(Whole(B.Az)).ToUpper();
 return B.Clock>=0?FString::Printf(TEXT("DIRECTION %s DEGREES (%s), GIVE OR TAKE %s / LOGGED %s / GOOD UNTIL NOON"),*Az,*Way,*Spread,*SlipClock(B.Clock))
  :FString::Printf(TEXT("DIRECTION %s DEGREES (%s), GIVE OR TAKE %s"),*Az,*Way,*Spread);
}
void FS47Places::RingOf(FBearing& B,float Roll)
{
 // D45-20261110: the circle is where to look, not the spot: the target inside it, off its middle by the roll.
 B.bRing=false;if(!B.bTo)return;
 const double R=RingFor(B.Spread);
 const FVector2D To(B.ToX,B.ToZ);
 const double Turn=double(Roll)*2*UE_DOUBLE_PI,Off=R*.45*FMath::Frac(double(Roll)*7.31);
 FVector2D C=To+FVector2D(FMath::Cos(Turn),FMath::Sin(Turn))*Off;
 const auto Good=[&B,R](const FVector2D& P){return S47OpenLand::InRim(P.X,P.Y,R+20)&&!S47Relief::HillStops(P.X,P.Y,5)&&InSector(B,P.X,P.Y);};
 for(int32 I=0;I<6&&!Good(C);++I)C=FMath::Lerp(C,To,.5);
 B.RingR=R;
 if(!Good(C))
 {
  // the target itself is on the land (the places and event places are, by their own self-tests): a smaller circle there
  C=To;
  const double Room=FMath::Min(FMath::Min(To.X-S47OpenLand::RimX0,S47OpenLand::RimX1-To.X),FMath::Min(To.Y-S47OpenLand::RimZ0,S47OpenLand::RimZ1-To.Y));
  B.RingR=FMath::Clamp(Room-20,60.,R);
 }
 B.RingX=C.X;B.RingZ=C.Y;B.bRing=true;
}
void FS47Places::ArcRing(FBearing& B)
{
 // D48-20261110 (D46-18): the first day's circle is about 150 m and centred on the place; only where the rim leaves no room, smaller
 if(!B.bTo)return;
 const double Room=FMath::Min(FMath::Min(B.ToX-S47OpenLand::RimX0,S47OpenLand::RimX1-B.ToX),FMath::Min(B.ToZ-S47OpenLand::RimZ0,S47OpenLand::RimZ1-B.ToZ));
 B.RingX=B.ToX;B.RingZ=B.ToZ;B.RingR=FMath::Clamp(Room-20,60.,150.);B.bRing=true;
}
FString FS47Places::CompassWord(double Az){return S47FieldMap::Compass(Az);}
FString FS47Places::SlipWhere(const FBearing& B)
{
 double Sx=0,Sz=0;SaroRoad(Sx,Sz);
 if(!B.bRing&&!B.bTo)return S47FieldMap::Compass(B.Az)+TEXT(" of SARO");
 const double X=B.bRing?B.RingX:B.ToX,Z=B.bRing?B.RingZ:B.ToZ;
 const double Metres=FVector2D::Distance(FVector2D(Sx,Sz),FVector2D(X,Z)),Mi=Metres/1609.344;
 // yards under half a mile, in fifties (never a 47); miles to a tenth above it
 const FString Far=Mi<.5?FString::Printf(TEXT("%d yards"),FMath::Max(50,FMath::RoundToInt(Metres/.9144/50)*50))
  :FString::Printf(TEXT("%.1f"),Mi)==TEXT("1.0")?FString(TEXT("1 mile")):FString::Printf(TEXT("%.1f miles"),Mi);
 return FString::Printf(TEXT("%s %s of SARO"),*Far,*S47FieldMap::Compass(AzimuthOf(Sx,Sz,X,Z)));
}
FString FS47Places::SlipNote(const FBearing& B,int32 Branches)
{
 // D45-20261110: what "a branch mended" means, said once on the paper, and where the circle is
 const FString Arms=Branches<=0?FString(TEXT("NO BRANCH OF THE ARRAY IS REPAIRED YET, SO THE AREA IS WIDE."))
  :Branches==1?FString(TEXT("ONE BRANCH OF THE ARRAY IS REPAIRED, SO THE AREA IS NARROWER."))
  :FString::Printf(TEXT("%d BRANCHES OF THE ARRAY ARE REPAIRED, SO THE AREA IS NARROWER."),Branches);
 const FString Map=FS47PlacesLocalMapKey().ToUpper();
 return Arms+TEXT(" A BRANCH IS ONE ARM OF THE ANTENNA ARRAY. EVERY BRANCH YOU REPAIR MAKES THE AREA SMALLER.\n")
  +(B.bRing?FString::Printf(TEXT("THE AREA TO SEARCH IS CIRCLED ON YOUR MAP (%s): %s. DRIVE OUT AND LOOK BEFORE NOON."),*Map,*SlipWhere(B).ToUpper())
   :FString::Printf(TEXT("THE DIRECTION IS PENCILED ON YOUR MAP (%s) UNTIL NOON."),*Map));
}
TArray<TPair<FString,bool>> FS47Places::SlipRows(double Now) const
{
 // D45-20261110 (D45-7): the slip on the list, the way WORTH A LOOK is (FS47FreeformDirector::MysteryTrailRows). Only a slip with a
 // noon: an older save's bearing (no time) is never cold and would stay on the list for good.
 TArray<TPair<FString,bool>> Out;
 for(int32 I=Fixes.Num()-1;I>=0&&Out.Num()<2;--I)
 {
  const FBearing& B=Fixes[I];if(B.bCar||B.Until<0||!Live(B,Now))continue;
  const bool bDone=Checked(B);
  Out.Add({B.bRing?FString::Printf(TEXT("BEARING SLIP / New circle on your map, %s. Drive out and look before noon."),*SlipWhere(B))
   :FString::Printf(TEXT("BEARING SLIP / New line on your map, %s. Drive out along it and look before noon."),*SlipWhere(B)),bDone});
 }
 // D48-20261110 (D46-18): the windmill found on the first day: the wire was put there on purpose, and the bearing may come back tonight
 for(const FBearing& B:Fixes)
  if(!B.bCar&&B.Until>=0&&Live(B,Now)&&B.Day==1&&B.Target==TEXT("windmill")&&FreeSlip(1,B.Clock)&&bHasFound(B.Target))
  {Out.Add({TEXT("SARO / Somebody wired that windmill on purpose. The bearing may come back tonight."),false});break;}
 return Out;
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
FS47Places::FBearing FS47Places::StationFix(int32 Day,double Clock,int32 Branches,double FromX,double FromZ,double ToX,double ToZ,float Roll,const FString& Target,float RingRoll)
{
 // Somewhere in the sector, not on its middle line: the roll moves the line up to half the spread either way.
 FBearing B;B.Day=Day;B.Spread=SpreadFor(Branches);B.Az=Whole(AzimuthOf(FromX,FromZ,ToX,ToZ)+(double(Roll)-.5)*B.Spread);B.Target=Target;
 B.Clock=FMath::Max(0.,Clock);B.Until=ColdAt(B.Clock);B.FromX=FromX;B.FromZ=FromZ;
 // D45-20261110: the place itself is kept (never shown exactly) and the circle round it, on the land
 if(FMath::IsFinite(ToX)&&FMath::IsFinite(ToZ)&&S47OpenLand::InRim(ToX,ToZ)){B.bTo=true;B.ToX=ToX;B.ToZ=ToZ;RingOf(B,RingRoll);}
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
  if(AtSaro(B.Id))continue;
  const bool bKept=bHasFound(B.Id);
  if(!bKept&&!HeardOf.Contains(B.Id))continue;
  FS47MapPin P;P.East=B.X;P.South=B.Z-SaroZ;P.Label=Def(B.Id)?FString(Def(B.Id)->Kind):B.Id;P.bHeard=!bKept;
  for(const int32 I:Shown)P.bInSector|=InSector(Fixes[I],B.X,B.Z);
  View.Pins.Add(P);
 }
 // MAP-D62-20261112: a slip's circle not yet stood in, its line gone cold, stays on the sheet (FS47PlacesLocalCircleOnSheet), paler,
 // without its wedge; at most two, older than the live ones, so the newest unlooked circle is still the last.
 {
  TArray<int32> Kept;
  for(int32 I=Fixes.Num()-1;I>=0&&Kept.Num()<2;--I)if(!Shown.Contains(I)&&!Live(Fixes[I],Now)&&FS47PlacesLocalCircleOnSheet(Fixes[I],Now)&&!Checked(Fixes[I]))Kept.Insert(I,0);
  for(const int32 I:Kept)
  {
   const FBearing& B=Fixes[I];
   S47FieldMap::FSlipCircle Ring;Ring.X=B.RingX;Ring.Z=B.RingZ;Ring.R=B.RingR;Ring.Where=SlipWhere(B);Ring.Fade=.3f;
   Ring.Label=FString::Printf(TEXT("Day %d slip"),B.Day);if(B.Clock>=0)Ring.Label+=TEXT(", ")+SlipClock(B.Clock);
   S47FieldMap::AddSlipCircle(View,Ring);
  }
 }
 for(const int32 I:Shown)
 {
  const FBearing& B=Fixes[I];
  // D45-20261110: "Day 3 slip, 21:40", not "D3" (it read as a grid square). The pin board leaves out what starts with D: still so.
  // FIX10-9 (→ M3): a line pencilled from a SARO window (FS47Happenings, the figure on the rise; Target WindowTarget) is the eye's, not the truck's.
  FS47MapSector S;S.Az=B.Az;S.Spread=B.Spread;S.Label=!B.bCar?FString::Printf(TEXT("Day %d slip"),B.Day):B.Target==WindowTarget?FString::Printf(TEXT("Day %d window"),B.Day):FString::Printf(TEXT("Day %d truck"),B.Day);
  // NIGHT-20261110: the pencilled wedge carries the hour the slip was taken (as the slip has it); an older bearing has none.
  if(B.Clock>=0)S.Label+=TEXT(", ")+SlipClock(B.Clock);
  S.bFrom=B.bCar;S.FromX=B.FromX;S.FromZ=B.FromZ;
  // The pencil pales as it cools, never quite away while it is live.
  S.Fade=float(.3+.7*Freshness(B,Now));
  View.Sectors.Add(S);
  // D45-20261110: the slip's circle on the sheet, with the wedge's own label, ticked once stood in (or its place found)
  if(!B.bCar&&B.bRing)
  {
   S47FieldMap::FSlipCircle Ring;Ring.X=B.RingX;Ring.Z=B.RingZ;Ring.R=B.RingR;Ring.Label=S.Label;Ring.Where=SlipWhere(B);Ring.Fade=S.Fade;
   Ring.bChecked=Checked(B);
   S47FieldMap::AddSlipCircle(View,Ring);
  }
 }
 for(int32 A=0;A<Shown.Num();++A)for(int32 C=A+1;C<Shown.Num();++C)
 {
  FVector2D At;
  if(Cross(Fixes[Shown[A]],Fixes[Shown[C]],At)&&S47OpenLand::InRim(At.X,At.Y))View.Crossings.Add(At);
 }
}
// D45-20261110 (Tom: the slip read strangely): the field record and the toast in plain words, the same for a registration and the week's
// case: which way, give or take how much, when, that it is a direction only, and that the circle on the map is where to look.
static FString FS47PlacesLocalSlipRecord(const FString& Lead,const FS47Places::FBearing& B,double Clock)
{
 return FString::Printf(TEXT("%s: about %03.0f degrees from SARO (%s), give or take %.0f, at %s. It only gives a direction; it doesn't say how far out it is or what it is. %s"),
  *Lead,B.Az,*S47FieldMap::Compass(B.Az),B.Spread,*FS47Places::SlipClock(Clock),
  B.bRing?*FString::Printf(TEXT("The area to search is circled on the map, %s, until noon."),*FS47Places::SlipWhere(B)):TEXT("It's penciled on the map until noon."));
}
static FString FS47PlacesLocalSlipToast(const FS47Places::FBearing& B,bool bSlip)
{
 return FString::Printf(TEXT("%sThe array picked up something about %03.0f degrees from the station (%s), give or take %.0f. %s (%s)."),bSlip?TEXT("PRINTER / Bearing slip printing. "):TEXT(""),
  B.Az,*S47FieldMap::Compass(B.Az),B.Spread,B.bRing?TEXT("It's circled on your map"):TEXT("It's penciled on your map"),*FS47PlacesLocalMapKey());
}
static FString FS47PlacesLocalMended(int32 Branches)
{
 return Branches<=0?FString(TEXT("With no branch of the array repaired, the array still picked up something away from where the dishes were pointed, though only roughly"))
  :FString::Printf(TEXT("With %d %s of the array repaired, the array picked up something away from where the dishes were pointed"),Branches,Branches==1?TEXT("branch"):TEXT("branches"));
}
FString FS47Places::CarefulLine(const FHeld& H)
{
 // LOOP-CAREFUL-PAYS-20261113: what made the area narrower, in the terminal's words; with neither, how it could have been
 const FString Who=(H.Why&3)==3?FString(TEXT("HAND FIX / MARKS TIGHT")):(H.Why&1)?FString(TEXT("HAND FIX")):(H.Why&2)?FString(TEXT("MARKS TIGHT")):FString();
 if(Who.IsEmpty()&&H.Branches>=4)return FString(); // nothing could have made it narrower
 if(Who.IsEmpty())return (H.Why&4)?FString(TEXT("A SKY FIX MADE BY HAND AND MARKS SET CLOSE ROUND THE EXTRA TICKS EACH MAKE THE AREA ONE STEP NARROWER."))
  :FString(TEXT("A SKY FIX MADE BY HAND MAKES THE AREA ONE STEP NARROWER."));
 if(H.Steps<=0)return Who+TEXT(": THE AREA IS ALREADY AS NARROW AS THE ARRAY GIVES.");
 return Who+(H.Steps>=2?TEXT(": AREA NARROWER BY TWO STEPS."):TEXT(": AREA NARROWER BY ONE STEP."));
}
FString FS47Places::TieLine(const FHeld& H)
{
 // LOOP-SLIP-IN-PACKET-20261113: the bearing and this take. Only when the take was odd because of what is out tonight and the slip
 // points at that same thing does it say they belong together, in the operator's own marks if he set them. Never what it is.
 if(H.bSame&&H.bOwnMarks)
 {
  // (read as digits: 14.7 or 4.7 says a 47 as well as 47.0 does; REVIEW-20261113: the shared rule, S47Script::TapeSeconds)
  const FString In=S47Script::TapeSeconds(H.MarkIn),Out=S47Script::TapeSeconds(H.MarkOut);
  const FString Digits=(In+TEXT(" ")+Out).Replace(TEXT("."),TEXT(""));
  if(!Digits.Contains(TEXT("47"))&&In!=TEXT("8.15")&&Out!=TEXT("8.15"))
   return FString::Printf(TEXT("THE EXTRA TICKS BETWEEN YOUR MARK IN AT %s S AND YOUR MARK OUT AT %s S CAME IN ON THIS BEARING."),*In,*Out);
  return TEXT("THE EXTRA TICKS BETWEEN YOUR MARK IN AND YOUR MARK OUT CAME IN ON THIS BEARING.");
 }
 if(H.bSame)return TEXT("THE EXTRA TICKS ON THIS TAKE CAME IN ON THIS BEARING.");
 return TEXT("LOGGED AS THIS TAKE WAS REGISTERED. IT DOES NOT SAY THE TWO ARE THE SAME.");
}
FString FS47Places::PacketSection(const FBearing& B,const FHeld& H)
{
 // LOOP-SLIP-IN-PACKET-20261113 (→ M3): the slip's own words as the packet's last section, under a plain head
 TArray<FString> L={TEXT("OFF-BEAM BEARING"),SlipLine(B),
  TEXT("THE ARRAY PICKED UP SOMETHING AWAY FROM WHERE THE DISHES WERE POINTED. IT GIVES A DIRECTION ONLY. IT DOES NOT SAY HOW FAR OUT IT IS OR WHAT IT IS.")};
 if(H.bExtra)L.Add(S47Dossier::ExtraSlipWords());
 L.Add(SlipNote(B,H.Branches));
 if(const FString Careful=CarefulLine(H);!Careful.IsEmpty())L.Add(Careful);
 L.Add(TieLine(H));
 return FString::Join(L,TEXT("\n"));
}
FString FS47Places::PacketSection(const FString& Job) const
{
 const FHeldFix* H=Held.FindByPredicate([&Job](const FHeldFix& X){return X.Info.Job==Job;});
 return H?PacketSection(H->B,H->Info):FString();
}
bool FS47Places::ReleasePacket(const FString& Job,FString* OutPage,int32* OutDay,bool* bOutExtra,double AtClock)
{
 const int32 I=Held.IndexOfByPredicate([&Job](const FHeldFix& X){return X.Info.Job==Job;});
 if(I==INDEX_NONE)return false;
 FHeldFix H=Held[I];Held.RemoveAt(I);
 // REVIEW-20261113: a packet fetched after the bearing's noon brings it live until the next noon, not already cold
 const double Now=AtClock>=0?AtClock:M&&M->Freeform?M->Freeform->Clock():-1;
 if(Now>=0&&H.B.Until>=0&&H.B.Until<=Now+3600.)H.B.Until=FMath::Max(H.B.Until,ColdAt(Now));
 if(OutPage)*OutPage=PacketSection(H.B,H.Info);
 if(OutDay)*OutDay=H.B.Day;
 if(bOutExtra)*bOutExtra=H.Info.bExtra;
 // on the map now (as live as its noon allows), and in the field record and the toast as a slip's would have been at the registration
 Fixes.Add(H.B);while(Fixes.Num()>MaxFixes)Fixes.RemoveAt(0);
 if(M&&M->Freeform)
 {
  const FString ExtraWhy=H.Info.bExtra?S47Dossier::ExtraToast()+TEXT(" "):FString();
  M->Freeform->RecordEvidence(FString::Printf(TEXT("bearing_d%d%s"),H.B.Day,H.Info.bExtra?TEXT("_more"):TEXT("")),ExtraWhy+FS47PlacesLocalSlipRecord(FS47PlacesLocalMended(H.Info.Branches),H.B,H.B.Clock>=0?H.B.Clock:M->Freeform->Clock()));
  if(M->UI)M->UI->Toast(ExtraWhy+TEXT("ANOMALY PACKET / Its last page is the bearing. ")+FS47PlacesLocalSlipToast(H.B,false),6);
 }
 UE_LOG(LogTemp,Display,TEXT("S47: bearing held for %s released with its packet: %.0f within %.0f toward %s"),*Job,H.B.Az,H.B.Spread,*H.B.Target);
 if(M)M->RequestAutosave();
 return true;
}
bool FS47Places::AfterRegistered(bool bAnomaly,const FTake* Packet)
{
 if(!M||!M->Freeform||!M->Freeform->RulesLive()||!M->Freeform->Field)return false;
 auto& F=*M->Freeform;
 const auto& Wear=F.Field->ArrayWear();
 const int32 Branches=Wear.bActive?Wear.Restored.Num():0;
 // LOOP-1-20261109: a discrepancy points out even with no branch mended (wide); routine work only through a mended branch.
 if(!Gives(bAnomaly,Branches))return false;
 const double Now=F.Clock();const int32 Night=FS47FreeformSession::NightOf(Now);
 // At most one thing a night at SARO points out at the land: the Director holds it (director.json point_out_per_night).
 // D45-20261110 (D45-26): on the first day a slip in daylight (FreeSlip), or any slip on the first night while the array's burst is still
 // to come (FS47Happenings::FirstBurstOwed), is once that day and leaves the night's one pointing-out to the burst.
 const bool bFree=FreeSlip(Night,Now)||(F.Happenings&&F.Happenings->FirstBurstOwed(Night));
 // LOOP-DOSSIER-GIVES-20261113 (→ M3, M4): a file of the unknown brought to MARKS gives one more than that, once (FS47Dossier::Grant):
 // the night's own is spent, and this is the extra. Never on the first day's free slips, never on the big night (DossierExtraOpen, and
 // BigOwns); the tiers and the target order are as ever. Only this slip takes it: the burst and the lights still see the night spent.
 // PT-FEEDER-20261109: the big night's night is its own (FS47Happenings::BigOwns): it points, nothing else does, before it or after.
 const bool bSpent=bFree?LastFixDay==F.Day():(F.Director?!F.Director->MayPointOut(Night):LastFixDay==F.Day());
 const int32 Allowed=SlipAllowed(bFree,bSpent,F.DossierExtraOpen(Night),F.Happenings&&F.Happenings->BigOwns(Night));
 if(Allowed==0)return false;
 const bool bExtra=Allowed==2;
 // The extra slip keeps the same order of what it points at, past whatever tonight's slips from SARO have pointed at already.
 // (LOOP-SLIP-IN-PACKET-20261113: a bearing still held for its packet counts as pointed at)
 const auto Fresh=[this,&F,bExtra](const FString& Name){return !bExtra||(!Fixes.ContainsByPredicate([&F,&Name](const FBearing& Had){return !Had.bCar&&Had.Day==F.Day()&&Had.Target==Name;})
  &&!Held.ContainsByPredicate([&F,&Name](const FHeldFix& Had){return Had.B.Day==F.Day()&&Had.B.Target==Name;}));};
 double SaroX=0,SaroZ=0;SaroRoad(SaroX,SaroZ);
 // What is off the main beam tonight: the marks, if there are any and their tier allows it at this hour (director.json: before
 // midnight only what can be explained away); otherwise something out on the land nobody has looked at.
 FString Target;double ToX=0,ToZ=0;bool bHave=false;
 if(F.FarmSite&&F.FarmSite->Armed())
 {
  const FString Why=F.FarmSite->FieldVisit()?TEXT("field_visit"):F.FarmSite->PlaceId()==TEXT("farm")?TEXT("farm_lights"):TEXT("town_lights");
  if((!F.Director||F.Director->MayPointAt(Why,Now,Night))&&Fresh(F.FarmSite->PlaceTitle()))
  {
   const FVector C=F.FarmSite->Centre();const FVector Road=S47Legs::ORIGINS(TEXT("road"));
   Target=F.FarmSite->PlaceTitle();
   if(F.FarmSite->PlaceId()==TEXT("farm"))bHave=S47OpenLand::RoadOf(TEXT("saro"),C.X,C.Z,ToX,ToZ);
   else{ToX=C.X-Road.X;ToZ=C.Z-Road.Z;bHave=true;}
  }
 }
 // CHAIN-1-20261109: then what is really out there tonight (FS47Happenings::OutTonight): what has come and nobody has been out to,
 // or the night's lead at the place drawn for it at dusk, which may be any place on the list of event places. Its tier is asked
 // the same way. Only with neither, as before: a place on the land nobody has looked at. Nothing is made up for a quiet night.
 const bool bMarks=bHave;bool bTonight=false,bAhead=false;
 if(!bHave&&F.Happenings)
 {
  FString Event,Title;double X=0,Z=0;
  if(F.Happenings->OutTonight(Night,Event,Title,X,Z,bAhead)&&(!F.Director||F.Director->MayPointAt(Event,Now,Night))&&Fresh((Title.IsEmpty()?Event:Title).Left(64)))
  {
   Target=(Title.IsEmpty()?Event:Title).Left(64);ToX=X;ToZ=Z;bHave=bTonight=true;
  }
  else bAhead=false;
 }
 // D45-20261110 (D45-26): on the first night the line the night's burst will come back on (ArcPlace), inside the rim.
 bool bArc=false;
 if(!bHave&&Night==1){Target=ArcPlace(F.Seed());bHave=bArc=!Target.IsEmpty()&&Fresh(Target)&&Spot(Target,ToX,ToZ)&&S47OpenLand::InRim(ToX,ToZ);}
 if(!bHave){Target=NextUnknown(F.Day());bHave=!Target.IsEmpty()&&Fresh(Target)&&Spot(Target,ToX,ToZ);}
 // T16-17 (→ M1, "the bearings run out"): every place found or heard of, the slip still points: at the place where the next
 // morning's trace will stand (DayTrace; the day turns at 08:00, so whether it is taken before midnight or after, that is Day+1).
 // The drive is the same as ever: be there by daylight before noon and look closer. Nothing new is saved.
 bool bTrace=false;
 if(!bHave){Target=TracePlace(F.Seed(),F.Day()+1);bHave=bTrace=!Target.IsEmpty()&&Fresh(Target)&&Spot(Target,ToX,ToZ)&&S47OpenLand::InRim(ToX,ToZ);}
 if(!bHave)return false;
 // LOOP-CAREFUL-PAYS-20261113 (→ M3, M2): a discrepancy worked with care (a sky fix by hand, marks tight round the ticks) narrows the
 // search one step each, on the mended branches' own ladder (SpreadFor, and with it RingFor and RingOf), never past the narrowest.
 const bool bPacket=Packet&&!Packet->Job.IsEmpty();
 const int32 Steps=bPacket?StepsTaken(Branches,CarefulSteps(Packet->bHandFix,Packet->bTightMarks)):0;
 const double Spread=SpreadFor(Branches+Steps);
 FBearing B=StationFix(F.Day(),Now,Branches+Steps,SaroX,SaroZ,ToX,ToZ,FS47NightRhythm::Roll(F.Seed(),F.Day(),bExtra?0x4245415bu:0x4245415au),Target,FS47NightRhythm::Roll(F.Seed(),F.Day(),bExtra?0x52494e48u:0x52494e47u));
 if(bArc){B.Az=ArcBearing(F.Seed(),SaroX,SaroZ,ToX,ToZ,Spread);RingOf(B,FS47NightRhythm::Roll(F.Seed(),F.Day(),0x52494e47u));ArcRing(B);} // the line's own number; D48-20261110: a small circle on the place
 // The night's pointing-out is spent here, at the registration, as ever (LOOP-SLIP-IN-PACKET-20261113: a slip held for its packet too).
 LastFixDay=F.Day();
 if(F.Director&&!bFree&&!bExtra)F.Director->NotePointOut(Night);
 if(bExtra)F.DossierSpendExtra(Night); // once: it is printed
 if(bAhead)F.Happenings->PointedTonight(Night);
 if(bPacket)
 {
  // LOOP-SLIP-IN-PACKET-20261113 (→ M3): the last page of the job's packet; the map, the record and the toast when it is collected
  FHeldFix H;H.B=B;H.Info.Job=Packet->Job.Left(64);H.Info.Branches=Branches;H.Info.Steps=Steps;H.Info.Why=uint8((Packet->bHandFix?1:0)|(Packet->bTightMarks?2:0)|(Packet->bTicks?4:0));H.Info.bExtra=bExtra;
  // the take was odd because of what is out tonight, and the slip points at that same thing (not at marks standing somewhere else)
  H.Info.bSame=Packet->bOut&&bTonight;
  H.Info.bOwnMarks=Packet->bOwnMarks&&Packet->MarkIn>=0&&Packet->MarkOut>Packet->MarkIn;H.Info.MarkIn=H.Info.bOwnMarks?Packet->MarkIn:-1;H.Info.MarkOut=H.Info.bOwnMarks?Packet->MarkOut:-1;
  Held.RemoveAll([&H](const FHeldFix& X){return X.Info.Job==H.Info.Job;});
  Held.Add(H);while(Held.Num()>MaxHeld)Held.RemoveAt(0);
  UE_LOG(LogTemp,Display,TEXT("S47: bearing: %.0f within %.0f toward %s held for the packet of %s, %d branches, %d careful steps (why %d), same %d, night %d"),B.Az,Spread,*Target,*H.Info.Job,Branches,Steps,int32(H.Info.Why),H.Info.bSame?1:0,Night);
  M->RequestAutosave();
  return true;
 }
 Fixes.Add(B);while(Fixes.Num()>MaxFixes)Fixes.RemoveAt(0);
 // LOOP-DOSSIER-GIVES-20261113: the extra slip is a record and a paper of its own, and says why there is one, in the station's words
 const FString ExtraWhy=bExtra?S47Dossier::ExtraToast()+TEXT(" "):FString();
 F.RecordEvidence(FString::Printf(TEXT("bearing_d%d%s"),F.Day(),bExtra?TEXT("_more"):TEXT("")),ExtraWhy+FS47PlacesLocalSlipRecord(FS47PlacesLocalMended(Branches),B,Now));
 // The slip on the OPS printer, collected there like the result sheets (behind any job's paper).
 const bool bSlip=F.QueueBearingSlip(B.Day,SlipLine(B),bExtra?S47Dossier::ExtraSlipWords()+TEXT("\n")+SlipNote(B,Branches):SlipNote(B,Branches),bExtra);
 if(M->UI)M->UI->Toast(ExtraWhy+FS47PlacesLocalSlipToast(B,bSlip),6);
 UE_LOG(LogTemp,Display,TEXT("S47: bearing: %.0f within %.0f toward %s (%s), %d branches, %s, night %d; circle %.0f %.0f r%.0f, on the land %d%s"),B.Az,Spread,*Target,SlipPoints(bMarks,bTonight,true)==1?TEXT("marks"):SlipPoints(bMarks,bTonight,true)==2?(bAhead?TEXT("tonight's, still to come"):TEXT("tonight's, out there now")):bArc?TEXT("the first night's line"):SlipPoints(false,false,false,bTrace)==4?TEXT("where the morning's trace will be"):TEXT("a place not looked at"),Branches,bAnomaly?TEXT("a discrepancy"):TEXT("routine"),Night,B.RingX,B.RingZ,B.RingR,B.bRing&&S47OpenLand::InRim(B.RingX,B.RingZ,B.RingR)?1:0,bFree?TEXT("; the night's pointing-out left for the burst"):TEXT(""));
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
 const FBearing B=StationFix(F.Day(),Now,Branches,SaroX,SaroZ,ToX,ToZ,FS47NightRhythm::Roll(F.Seed(),F.Day(),0x5745454bu),Target,FS47NightRhythm::Roll(F.Seed(),F.Day(),0x52494e47u));
 Fixes.Add(B);while(Fixes.Num()>MaxFixes)Fixes.RemoveAt(0);
 LastFixDay=F.Day();
 if(F.Director)F.Director->NotePointOut(Night);
 F.RecordEvidence(FString::Printf(TEXT("bearing_d%d"),F.Day()),FS47PlacesLocalSlipRecord(TEXT("Tonight the array picked up something on its own, away from where the dishes were pointed"),B,Now));
 const bool bSlip=F.QueueBearingSlip(B.Day,SlipLine(B),SlipNote(B,Branches));
 if(M->UI)M->UI->Toast(FS47PlacesLocalSlipToast(B,bSlip),6);
 UE_LOG(LogTemp,Display,TEXT("S47: bearing: %.0f within %.0f toward %s, %d branches, the week's case, night %d; circle %.0f %.0f r%.0f"),B.Az,Spread,*Target,Branches,Night,B.RingX,B.RingZ,B.RingR);
 M->RequestAutosave();
 return true;
}
bool FS47Places::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s places: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 TSet<FString> Ids;bool bWords=true,bParts=true;
 for(const FDef& D:Defs()){Ids.Add(D.Id);bWords&=FCString::Strlen(D.Finding)>40&&FCString::Strlen(D.Finding)<320&&FCString::Strlen(D.Needle)>4&&!FString(D.Finding).Contains(TEXT("alien"));bParts&=PartsOf(D.Kind).Num()>0||(FString(D.Id)==TEXT("balloon")&&FS47Balloon::HaveModel());} // (D48: the balloon's model draws the place itself)
 Check(Ids.Num()==Defs().Num()&&Defs().Num()>=9&&Defs().Num()<=13,TEXT("eight to ten places on the land besides the motel and the old house's two, each once"));
 // T16-17 (→ M1, "the farmhouse is an empty shell"): two things inside the old house, where the plan has them, inside its walls, the
 // kitchen's by the front door and the back room's by the back door, on SARO's ground; never on the map, never a slip's or a cot's.
 {
  const FDef* Kitchen=Def(TEXT("kitchen"));const FDef* BackRoom=Def(TEXT("backroom"));
  const FVector Kp=S47FarmHouse::Point(2.05,7.25),Bp=S47FarmHouse::Point(8.3,1.);
  bool bHouse=Kitchen&&BackRoom&&FMath::IsNearlyEqual(Kitchen->X,Kp.X,.01)&&FMath::IsNearlyEqual(Kitchen->Z,Kp.Z,.01)&&FMath::IsNearlyEqual(BackRoom->X,Bp.X,.01)&&FMath::IsNearlyEqual(BackRoom->Z,Bp.Z,.01)
   &&S47FarmHouse::Inside(Kp.X,Kp.Z)&&S47FarmHouse::Inside(Bp.X,Bp.Z)&&InOldHouse(TEXT("kitchen"))&&InOldHouse(TEXT("backroom"))&&AtSaro(TEXT("motel"))&&!InOldHouse(TEXT("motel"))&&!AtSaro(TEXT("windmill"));
  bool bOffLand=true;for(int32 Sd=1;Sd<=200;++Sd){const FString A=ArcPlace(Sd);bOffLand&=!AtSaro(A);for(int32 Day=1;Day<=12;++Day)bOffLand&=!InOldHouse(TracePlace(Sd,Day))&&DayTrace(TEXT("kitchen"),Sd,Day).IsEmpty()&&DayTrace(TEXT("backroom"),Sd,Day).IsEmpty();}
  const FString AgainLine=BackRoomAgain(),OnLand=NothingNew(false),AtHome=NothingNew(true);
  bool bHouseWords=true;for(const FString& W:{AgainLine,OnLand,AtHome,FString(Kitchen?Kitchen->Finding:TEXT("")),FString(BackRoom?BackRoom->Finding:TEXT(""))})bHouseWords&=!W.IsEmpty()&&S47Script::LintText(W).IsEmpty()&&!W.Contains(TEXT("47"))&&!W.ToLower().Contains(TEXT("taken"))&&!W.ToLower().Contains(TEXT("alien"));
  Check(bHouse&&bOffLand&&bHouseWords&&OnLand!=AtHome,TEXT("T16-17: the old house has two things to look at, inside its walls where the plan has them, off the land's map and its traces, in plain words"));
 }
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
 // LOOP-DOSSIER-GIVES-20261113: MARKS's one more slip comes only once the night's own is spent and an extra is open; never for the first
 // day's free slips, and nothing at all on the big night.
 Check(SlipAllowed(false,false,false,false)==1&&SlipAllowed(false,false,true,false)==1&&SlipAllowed(false,true,false,false)==0&&SlipAllowed(false,true,true,false)==2
  &&SlipAllowed(true,true,true,false)==0&&SlipAllowed(true,false,true,false)==1&&SlipAllowed(false,false,true,true)==0&&SlipAllowed(false,true,true,true)==0&&SlipAllowed(false,false,false,true)==0,
  TEXT("LOOP-DOSSIER-GIVES: one more bearing slip only past the night's own, with a file's extra open; never the first day's free slip, never on the big night"));
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
  Check(Gives(true,0)&&!Gives(false,0)&&Gives(false,2)&&B.Spread==40&&B.Az==90&&Line==TEXT("DIRECTION 090 DEGREES (EAST), GIVE OR TAKE 40 / LOGGED 19:52 / GOOD UNTIL NOON")&&InSector(B,Sx+400,Sz+200)&&!InSector(B,Sx,Sz+400),
   TEXT("a discrepancy gives a wide bearing and its slip with no branch mended, routine work needs a mended branch, and a place in the sector is in it"));
  // Cold by noon: the next noon, the pencil paling until then, and off the map once it is cold; an older one (no time) never is.
  FS47Places Map(nullptr);Map.Begin(nullptr);Map.Fixes.Add(B);
  FS47MapView Fresh,Later,Cold;Map.FillMapAt(Fresh,Clock+60);Map.FillMapAt(Later,ColdAt(Clock)-3600);Map.FillMapAt(Cold,ColdAt(Clock)+1);
  FBearing Old=B;Old.Clock=Old.Until=-1;Old.bTo=Old.bRing=false; // (D45-20261110: an older save has no circle either)
  Check(ColdAt(Clock)==4*86400.+12*3600.&&ColdAt(4*86400.+1*3600.)==4*86400.+12*3600.&&Live(B,ColdAt(Clock)-1)&&!Live(B,ColdAt(Clock))&&Live(Old,1e9)
   &&Fresh.Sectors.Num()==1&&Later.Sectors.Num()==1&&Fresh.Sectors[0].Fade>Later.Sectors[0].Fade&&Later.Sectors[0].Fade>=.3f&&Cold.Sectors.IsEmpty(),
   TEXT("a bearing is cold by the next noon: it pales on the map until then and is off it after"));
  // NIGHT-20261110: the wedge on the map carries the hour of its slip; an older bearing (no clock) keeps its bare label.
  FS47MapView Bare;FS47Places Plain(nullptr);Plain.Begin(nullptr);Plain.Fixes.Add(Old);Plain.FillMapAt(Bare,1e6);
  Check(Fresh.Sectors[0].Label.EndsWith(SlipClock(Clock))&&Bare.Sectors.Num()==1&&!Bare.Sectors[0].Label.Contains(TEXT(":")),TEXT("the pencilled wedge carries the hour of its slip, an older one has none"));
  // Story's numbers never on the paper: no 47 in a degree or a clock, no 8.15 or 20.15.
  bool bClean=true;
  for(int32 D=0;D<360;++D){FBearing X=B;X.Az=D;const FString L=SlipLine(X);bClean&=!L.Contains(TEXT("47"))&&Whole(D)!=47&&Whole(D)!=147&&Whole(D)!=247&&Whole(D)!=347;}
  for(int32 Min=0;Min<24*60;++Min){const FString C=SlipClock(Min*60.);bClean&=C.Len()==5&&!C.Contains(TEXT("47"))&&C!=TEXT("08:15")&&C!=TEXT("20:15");}
  Check(bClean&&SlipClock(20*3600.+15*60.)==TEXT("20:16")&&SlipClock(19*3600.+47*60.)==TEXT("19:48"),TEXT("the slip never prints a 47, 08:15 or 20:15: a degree or a minute on"));
  // D45-20261110: the slip in plain words: the compass point, no degree or plus-minus sign (plain ASCII), what a branch is, the circle.
  {
   bool bAscii=true;const FString Paper=Line+SlipLine(Old)+SlipNote(B,0)+SlipNote(B,1)+SlipNote(B,3)+SlipNote(Old,0);
   for(const TCHAR Ch:Paper)bAscii&=Ch>=9&&Ch<127;
   Check(bAscii&&SlipLine(Old)==TEXT("DIRECTION 090 DEGREES (EAST), GIVE OR TAKE 40")&&SlipNote(B,0).Contains(TEXT("A BRANCH IS ONE ARM OF THE ANTENNA ARRAY"))&&SlipNote(B,0).Contains(TEXT("CIRCLED ON YOUR MAP"))
    &&SlipNote(B,0).Contains(TEXT("BEFORE NOON"))&&SlipNote(B,2).Contains(TEXT("2 BRANCHES"))&&!SlipNote(Old,0).Contains(TEXT("CIRCLED"))&&S47Script::LintText(Paper).IsEmpty()&&!Paper.Contains(TEXT("47")),
    TEXT("the slip says the direction and its compass point, give or take, when it was logged and that it is good until noon, what a branch is, and where the circle is, in plain ASCII"));
  }
  // D45-20261110 (Tom's d45 map, the mark far south of the land): every slip from SARO has a circle that holds its target, inside the
  // rim and with its middle in the sector; the more branches, the smaller. For every place, a few far out near the rim, every roll.
  {
   TArray<FVector2D> Targets;for(const FDef& D:Defs())if(!AtSaro(D.Id))Targets.Add(FVector2D(D.X,D.Z));
   Targets.Append({FVector2D(S47OpenLand::RimX0+160,1500),FVector2D(S47OpenLand::RimX1-160,-1200),FVector2D(300,S47OpenLand::RimZ1-160),FVector2D(-560,1340)});
   bool bRinged=true,bHolds=true,bLand=true,bSector=true;
   for(const FVector2D& T:Targets)for(int32 Br=0;Br<=4;++Br)for(int32 K=0;K<=10;++K)
   {
    const FBearing R=StationFix(3,20*3600.,Br,Sx,Sz,T.X,T.Y,float(K)/10.f,TEXT("t"),float(10-K)/10.f);
    bRinged&=R.bTo&&R.bRing&&R.RingR>=60&&R.RingR<=RingFor(R.Spread);
    bHolds&=FVector2D::Distance(FVector2D(R.RingX,R.RingZ),T)<=R.RingR*.45+1;
    bLand&=S47OpenLand::InRim(R.RingX,R.RingZ,R.RingR);
    bSector&=InSector(R,R.RingX,R.RingZ);
   }
   Check(bRinged&&bHolds&&bLand&&bSector&&RingFor(40)==450&&RingFor(4)==140&&RingFor(20)<RingFor(40)&&RingFor(7)<RingFor(12),
    TEXT("a slip's circle holds what it points at, stays on the land inside the rim, has its middle in the sector, and shrinks with every branch"));
   // LOOP-CAREFUL-PAYS-20261113 (→ M3, M2): a hand fix and tight marks take a step each off the same ladder, never past the narrowest,
   // and the narrowed circle still holds its target on the land inside the sector.
   {
    bool bLadder=CarefulSteps(true,true)==2&&CarefulSteps(true,false)==1&&CarefulSteps(false,true)==1&&CarefulSteps(false,false)==0
     &&StepsTaken(0,2)==2&&StepsTaken(3,2)==1&&StepsTaken(4,2)==0&&StepsTaken(9,1)==0&&SpreadFor(0+StepsTaken(0,2))==12&&RingFor(SpreadFor(2))==240&&SpreadFor(0+StepsTaken(0,1))==20;
    bool bNarrowHolds=true;
    for(int32 Br=0;Br<=5;++Br)for(int32 St=0;St<=2;++St)
    {
     const int32 Taken=StepsTaken(Br,St);
     bLadder&=Taken>=0&&Taken<=St&&SpreadFor(Br+Taken)==SpreadFor(FMath::Min(FMath::Min(Br,4)+St,4))&&SpreadFor(Br+Taken)<=SpreadFor(Br);
     for(const FVector2D& T:Targets)for(int32 K=0;K<=10;K+=5)
     {
      const FBearing R=StationFix(3,20*3600.,Br+Taken,Sx,Sz,T.X,T.Y,float(K)/10.f,TEXT("t"),float(10-K)/10.f);
      bNarrowHolds&=R.bRing&&R.RingR<=RingFor(R.Spread)&&FVector2D::Distance(FVector2D(R.RingX,R.RingZ),T)<=R.RingR*.45+1&&S47OpenLand::InRim(R.RingX,R.RingZ,R.RingR)&&InSector(R,R.RingX,R.RingZ);
     }
    }
    Check(bLadder&&bNarrowHolds,TEXT("LOOP-CAREFUL-PAYS: a hand fix and tight marks each take one step off the spread and the circle (40/20/12/7/4, 450 to 140 m), clamped at the narrowest; the target stays inside the circle"));
   }
   // LOOP-SLIP-IN-PACKET-20261113 (→ M3): the packet's last section in the slip's plain words, the reasons and the tie to the take;
   // held for its packet it is off the map and off the slip rows, kept over a Continue, and on the map once the packet is collected.
   {
    const FBearing P=StationFix(4,Clock,2,Sx,Sz,Sx+500,Sz,.5f,TEXT("windmill"));
    FHeld H;H.Job=TEXT("FF-D0004-J2");H.Branches=0;H.Steps=2;H.Why=1|2|4;H.bSame=true;H.bOwnMarks=true;H.MarkIn=9.;H.MarkOut=13.;
    const FString Full=PacketSection(P,H);
    FHeld Loose=H;Loose.MarkOut=14.7;const FString NoNumber=PacketSection(P,Loose);
    FHeld Other=H;Other.bSame=false;const FString Apart=PacketSection(P,Other);
    FHeld Plain0=H;Plain0.Why=4;Plain0.Steps=0;Plain0.bOwnMarks=false;const FString Teach=PacketSection(P,Plain0);
    FHeld Instr=Plain0;Instr.Why=0;Instr.bSame=false;const FString TeachSky=PacketSection(P,Instr);
    FHeld Full4=H;Full4.Branches=4;Full4.Steps=0;Full4.Why=1|4;const FString Narrowest=PacketSection(P,Full4);
    FHeld None4=Full4;None4.Why=4;const FString Silent=PacketSection(P,None4);
    const FString All=Full+NoNumber+Apart+Teach+TeachSky+Narrowest+Silent;
    bool bPlain=S47Script::LintText(All).IsEmpty()&&!All.Contains(TEXT("47"))&&!All.ToLower().Contains(TEXT("taken"))&&!All.Contains(TEXT(" C "))&&!All.ToLower().Contains(TEXT("alien"));
    for(const TCHAR Ch:All)bPlain&=Ch>=9&&Ch<127;
    const bool bFull=Full.StartsWith(TEXT("OFF-BEAM BEARING\n"))&&Full.Contains(SlipLine(P))&&Full.Contains(TEXT("GOOD UNTIL NOON"))&&Full.Contains(TEXT("CIRCLED ON YOUR MAP"))&&Full.Contains(TEXT("DIRECTION ONLY"))
     &&Full.Contains(TEXT("HAND FIX / MARKS TIGHT: AREA NARROWER BY TWO STEPS."))&&Full.Contains(TEXT("BETWEEN YOUR MARK IN AT 9.0 S AND YOUR MARK OUT AT 13.0 S CAME IN ON THIS BEARING."));
    const bool bSaid=NoNumber.Contains(TEXT("BETWEEN YOUR MARK IN AND YOUR MARK OUT CAME IN ON THIS BEARING."))&&Apart.Contains(TEXT("IT DOES NOT SAY THE TWO ARE THE SAME"))&&!Apart.Contains(TEXT("CAME IN ON THIS BEARING"))
     &&Teach.Contains(TEXT("MARKS SET CLOSE ROUND THE EXTRA TICKS"))&&TeachSky.Contains(TEXT("A SKY FIX MADE BY HAND MAKES"))&&!TeachSky.Contains(TEXT("MARKS SET"))
     &&Narrowest.Contains(TEXT("HAND FIX: THE AREA IS ALREADY AS NARROW AS THE ARRAY GIVES."))&&!Silent.Contains(TEXT("HAND FIX"))&&!Silent.Contains(TEXT("A SKY FIX MADE BY HAND"));
    Check(bPlain&&bFull&&bSaid,TEXT("LOOP-SLIP-IN-PACKET: the packet's last page is OFF-BEAM BEARING with the direction, spread, circle and noon, why it is narrower, and the tie to the take in his own marks; never a 47 or what it is"));
    FS47Places Hold(nullptr);Hold.Begin(nullptr);FHeldFix Hf;Hf.B=P;Hf.Info=H;Hold.Held.Add(Hf);
    FS47Places HoldAgain(nullptr);HoldAgain.Begin(S47J::Read(S47J::Write(Hold.Save())));
    FS47MapView Waiting,Out;HoldAgain.FillMapAt(Waiting,Clock+60);
    const bool bWaits=HoldAgain.HeldFor(H.Job)&&!HoldAgain.HeldFor(TEXT("FF-D0004-J1"))&&HoldAgain.Bearings().IsEmpty()&&Waiting.Sectors.IsEmpty()&&S47FieldMap::SlipCircles(Waiting).IsEmpty()&&HoldAgain.SlipRows(Clock+60).IsEmpty()
     &&HoldAgain.PacketSection(H.Job).StartsWith(TEXT("OFF-BEAM BEARING"))&&HoldAgain.PacketSection(H.Job).Contains(SlipLine(P))&&HoldAgain.PacketSection(TEXT("FF-D0004-J1")).IsEmpty();
    FString Page;int32 PageDay=0;bool bPageExtra=true;
    const bool bOut=HoldAgain.ReleasePacket(H.Job,&Page,&PageDay,&bPageExtra)&&!HoldAgain.ReleasePacket(H.Job);
    HoldAgain.FillMapAt(Out,Clock+60);
    const bool bShown=bOut&&Page.StartsWith(TEXT("OFF-BEAM BEARING"))&&PageDay==4&&!bPageExtra&&!HoldAgain.HeldFor(H.Job)&&HoldAgain.Bearings().Num()==1&&Out.Sectors.Num()==1&&S47FieldMap::SlipCircles(Out).Num()==1
     &&HoldAgain.SlipRows(Clock+60).Num()==1&&FS47Places(nullptr).HeldCount()==0;
    {
     // REVIEW-20261113: the packet left on the printer past the bearing's noon: collected at 14:00, the bearing is live (to the next
     // noon), on the map and on NOW, not cold as it arrives
     FS47Places Late(nullptr);Late.Begin(nullptr);FHeldFix Lf=Hf;Late.Held.Add(Lf);
     const double After=P.Until+2*3600.;
     FS47MapView LateMap;const bool bLateOut=Late.ReleasePacket(H.Job,nullptr,nullptr,nullptr,After);Late.FillMapAt(LateMap,After+60);
     Check(bLateOut&&Late.Bearings().Num()==1&&Late.Bearings()[0].Until>After&&Late.Bearings()[0].Until==ColdAt(After)&&LateMap.Sectors.Num()==1&&Late.SlipRows(After+60).Num()==1,
      TEXT("REVIEW: a packet collected after its bearing's noon brings the bearing live to the next noon: on the map and on NOW, never cold on arrival"));
    }
    FS47Places OldSave(nullptr);OldSave.Begin(S47J::Read(S47J::Write(Map.Save())));
    Check(bWaits&&bShown&&OldSave.HeldCount()==0&&!Map.Save()->HasField(TEXT("held")),TEXT("LOOP-SLIP-IN-PACKET: held for its packet the bearing is off the map and NOW, kept over a Continue, then on the map with its circle once the packet is collected; an older save has none"));
   }
   // The circle on the sheet with the wedge's label; the old sheet's label is "Day 4 slip, 19:52", never "D4" (read as a grid square).
   const TArray<S47FieldMap::FSlipCircle> Circles=S47FieldMap::SlipCircles(Fresh);
   Check(B.bRing&&Circles.Num()==1&&Circles[0].Label==Fresh.Sectors[0].Label&&Fresh.Sectors[0].Label==TEXT("Day 4 slip, 19:52")&&FMath::IsNearlyEqual(Circles[0].R,B.RingR)&&!Circles[0].bChecked&&Circles[0].Where.EndsWith(TEXT("of SARO"))
    &&S47FieldMap::SlipCircles(Bare).IsEmpty(),TEXT("the map carries the live slip's circle with its day and hour; an older bearing has none"));
   // MAP-D62-20261112 (Tom, d62: the circle gone when the map was opened again after noon): the circle not yet stood in stays on the
   // sheet without its wedge, paler, for a day past the line's noon, and every opening builds it again; stood in, it goes with the line.
   {
    const TArray<S47FieldMap::FSlipCircle> Stays=S47FieldMap::SlipCircles(Cold);
    FS47MapView Again,Gone,Done;Map.FillMapAt(Again,ColdAt(Clock)+3600);Map.FillMapAt(Gone,ColdAt(Clock)+86400+1);
    FS47Places Looked(nullptr);Looked.Begin(nullptr);FBearing Seen=B;Seen.bVisited=true;Looked.Fixes.Add(Seen);Looked.FillMapAt(Done,ColdAt(Clock)+1);
    Check(Cold.Sectors.IsEmpty()&&Stays.Num()==1&&Stays[0].Label==Circles[0].Label&&!Stays[0].bChecked&&Stays[0].Fade<Circles[0].Fade&&FMath::IsNearlyEqual(Stays[0].R,B.RingR)
     &&S47FieldMap::SlipCircles(Again).Num()==1&&S47FieldMap::SlipCircles(Gone).IsEmpty()&&S47FieldMap::SlipCircles(Done).IsEmpty()&&Done.Sectors.IsEmpty(),
     TEXT("a slip's circle not yet looked at stays on the map after its line goes cold at noon, on every opening, for a day; looked at, it goes with the line"));
   }
   // The slip on NOW: one row, plain words, ticked once stood in, the same words either way, gone at noon; the truck's line is no slip.
   FS47Places Rows(nullptr);Rows.Begin(nullptr);Rows.Fixes.Add(B);
   FBearing Truck=B;Truck.bCar=true;Truck.bRing=false;Rows.Fixes.Add(Truck);
   FBearing Ancient=Old;Ancient.bTo=Ancient.bRing=false;Rows.Fixes.Insert(Ancient,0);
   const TArray<TPair<FString,bool>> Open=Rows.SlipRows(Clock+60);
   Rows.Fixes[1].bVisited=true;
   const TArray<TPair<FString,bool>> Seen=Rows.SlipRows(Clock+120);
   Check(Open.Num()==1&&!Open[0].Value&&Open[0].Key.StartsWith(TEXT("BEARING SLIP / New circle on your map, "))&&Open[0].Key.EndsWith(TEXT("of SARO. Drive out and look before noon."))&&S47Script::LintText(Open[0].Key).IsEmpty()
    &&Seen.Num()==1&&Seen[0].Value&&Seen[0].Key==Open[0].Key&&Rows.SlipRows(ColdAt(Clock)+1).IsEmpty(),
    TEXT("NOW has a row for the live slip, in plain words, ticked off once its circle is stood in, the words the same, gone at noon; none for the truck's line or an older bearing"));
   // Saved with the bearing and back; a bearing from an older save has no circle and still loads (see the next block).
   FS47Places Again(nullptr);Again.Begin(S47J::Read(S47J::Write(Rows.Save())));
   const FBearing* Kept=Again.Bearings().FindByPredicate([](const FBearing& X){return !X.bCar&&X.bRing;});
   Check(Kept&&Kept->bTo&&Kept->bVisited&&FMath::IsNearlyEqual(Kept->RingX,B.RingX,.11)&&FMath::IsNearlyEqual(Kept->RingZ,B.RingZ,.11)&&FMath::IsNearlyEqual(Kept->RingR,B.RingR,.11)&&FMath::IsNearlyEqual(Kept->ToX,B.ToX,.11)
    &&Again.Bearings().FilterByPredicate([](const FBearing& X){return X.bRing;}).Num()==1,TEXT("a slip's circle, its target and whether it was stood in survive a Continue"));
  }
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
  {
   // FIX10-9 (→ M3): a figure seen from a SARO window is a line from SARO on the map, labelled as the window's, and it crosses a truck's.
   FS47Places Pane(nullptr);Pane.Begin(nullptr);
   const bool bPane=Pane.AddCarFix(Sx,Sz,110,WindowTarget,Clock,4)&&Pane.AddCarFix(Tx,Tz,45,TEXT("omen:a"),Clock+60,4);
   FS47MapView PaneView;Pane.FillMapAt(PaneView,Clock+120);
   const bool bLabel=PaneView.Sectors.ContainsByPredicate([](const FS47MapSector& X){return X.Label.StartsWith(TEXT("Day 4 window"));})&&PaneView.Sectors.ContainsByPredicate([](const FS47MapSector& X){return X.Label.StartsWith(TEXT("Day 4 truck"));});
   Check(bPane&&bLabel&&Pane.Bearings()[0].bCar&&FMath::IsNearlyEqual(Pane.Bearings()[0].FromX,Sx,.11)&&!Dosed(Pane.Bearings()[0],Clock+60),
    TEXT("FIX10-9: a figure seen from a SARO window is a line from SARO on the map, \"Day N window\", beside a truck's (no dose: no place)"));
  }
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
  Check(Back.Heard(TEXT("mine"))&&!Back.Heard(TEXT("nowhere"))&&Back.Bearings().Num()==1&&Was.Az==31.&&Was.Clock<0&&Was.Until<0&&!Was.bCar&&Was.FromX==Sx&&Was.FromZ==Sz&&Live(Was,1e9)&&!Was.bRing&&!Was.bTo&&Back.SlipRows(1e9).IsEmpty(),
   TEXT("heard and bearings survive a Continue, an older bearing is from SARO and never cold, and nonsense is dropped"));
  // A bearing from the truck with no place to stand on is nonsense too.
  auto Bad=S47J::Clone(O);auto Car=S47J::Clone(Fix);S47J::Set(Car,TEXT("car"),true);TArray<TSharedPtr<FJsonValue>> L2;L2.Add(MakeShared<FJsonValueObject>(Car));Bad->SetArrayField(TEXT("fixes"),L2);
  FS47Places Dropped(nullptr);Dropped.Begin(S47J::Read(S47J::Write(Bad)));
  Check(Dropped.Bearings().IsEmpty(),TEXT("a bearing from the truck without where it stood is dropped"));
 }
 {
  // CHAIN-1-20261109: what the slip points at: marks first, then what is out there tonight, then a place nobody has looked at.
  Check(SlipPoints(true,true,true)==1&&SlipPoints(false,true,true)==2&&SlipPoints(false,false,true)==3&&SlipPoints(false,false,false)==0,
   TEXT("the bearing slip points at tonight's own thing before a place nobody has looked at, and at nothing when there is neither"));
  // T16-17 (→ M1, "the bearings run out"): with every place known, the slip points where the next morning's trace stands: one place a
  // day from day 2, the one DayTrace names, never the motel, the cairn or Saucer Rock; last in line, after a place nobody has looked at.
  {
   bool bOne=true,bSome=false;
   for(int32 Sd=1;Sd<=60;++Sd)for(int32 Day=2;Day<=30;++Day)
   {
    const FString P=TracePlace(Sd,Day);
    if(P.IsEmpty())continue;
    bSome=true;bOne&=!DayTrace(P,Sd,Day).IsEmpty()&&P!=TEXT("motel")&&P!=TEXT("cairn")&&P!=TEXT("saucer_rock")&&Def(P)!=nullptr;
   }
   Check(bOne&&bSome&&TracePlace(5,1).IsEmpty()&&SlipPoints(false,false,false,true)==4&&SlipPoints(false,false,true,true)==3&&SlipPoints(false,true,false,true)==2,
    TEXT("T16-17: once every place is known, the slip points at where the next morning's trace will be, after any place nobody has looked at"));
  }
  // A slip toward an event place is a bearing like any other (its target a title, not one of the ten): it is kept over a Continue,
  // goes cold by noon, and gives the counter no dose of its own.
  double Sx=0,Sz=0;SaroRoad(Sx,Sz);
  const FBearing Out=StationFix(5,19.5*3600.,0,Sx,Sz,Sx+900,Sz-1200,.5f,TEXT("the dry wash"));
  Check(Out.Target==TEXT("the dry wash")&&InSector(Out,Sx+900,Sz-1200)&&!Dosed(Out,Out.Clock)&&Live(Out,Out.Clock)&&!Live(Out,Out.Until+1),
   TEXT("a slip toward tonight's place holds the place in its sector, is live until noon, and is not one of the ten for the counter"));
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
 {
  // D45-20261110 (D45-26): the first night's line. The windmill, on the land, never the cairn; one bearing to it for the noon slip and
  // the burst whatever their spread (no branch to two mended), the same for a seed, inside each sector, never Story's number.
  double Sx=0,Sz=0;SaroRoad(Sx,Sz);
  const FDef* Mill=Def(TEXT("windmill"));
  bool bPlace=Mill!=nullptr,bOne=true,bInside=true,bNumber=true;
  for(int32 Sd=1;Sd<=400&&Mill;++Sd)
  {
   const FString P=ArcPlace(Sd);
   bPlace&=P==TEXT("windmill")&&S47OpenLand::InRim(Mill->X,Mill->Z,60);
   const double Slip=ArcBearing(Sd,Sx,Sz,Mill->X,Mill->Z,SpreadFor(0)),Burst=ArcBearing(Sd,Sx,Sz,Mill->X,Mill->Z,18.);
   bOne&=Slip==Burst&&Slip==ArcBearing(Sd,Sx,Sz,Mill->X,Mill->Z,SpreadFor(1))&&Slip==ArcBearing(Sd,Sx,Sz,Mill->X,Mill->Z,SpreadFor(2));
   FBearing Line;Line.Az=Slip;Line.Spread=SpreadFor(2);Line.FromX=Sx;Line.FromZ=Sz;
   bInside&=InSector(Line,Mill->X,Mill->Z);
   const FString Said=FString::Printf(TEXT("%03.0f"),Slip);
   bNumber&=!Said.Contains(TEXT("47"))&&Slip>=0&&Slip<360;
  }
  Check(bPlace&&bOne&&bInside&&bNumber,TEXT("the first night's line: the windmill, on the land, one bearing for the slip and the burst that holds it, and no 47"));
  // In daylight on the first day a slip leaves the night's pointing-out for the dark; at dusk, after midnight, or another day it does not.
  Check(FreeSlip(1,12.5*3600.)&&FreeSlip(1,8*3600.)&&!FreeSlip(1,18*3600.)&&!FreeSlip(1,22*3600.)&&!FreeSlip(1,86400.+7*3600.)&&!FreeSlip(2,86400.+12*3600.),
   TEXT("only the first day's daylight slip is free of the night's one pointing-out"));
  // D48-20261110 (D46-18): day 1 at the windmill. The circle is small (150 m at most) and on the place whatever the roll or the branches;
  // the traces are there on the first two days and not after; the head is turned toward the bearing; the NOW line comes with the
  // finding, only for the first day's slip, and is plain.
  if(Mill)
  {
   bool bRing=true;
   for(int32 Br=0;Br<=4;++Br)for(int32 K=0;K<=10;++K)
   {
    FBearing R=StationFix(1,12.5*3600.,Br,Sx,Sz,Mill->X,Mill->Z,float(K)/10.f,TEXT("windmill"),float(10-K)/10.f);
    ArcRing(R);
    bRing&=R.bRing&&R.RingR>=60&&R.RingR<=150&&FMath::IsNearlyEqual(R.RingX,Mill->X)&&FMath::IsNearlyEqual(R.RingZ,Mill->Z)&&S47OpenLand::InRim(R.RingX,R.RingZ,R.RingR)&&InSector(R,R.RingX,R.RingZ);
   }
   TArray<FPart> Mine=PartsOf(Mill->Kind);const int32 Plain=Mine.Num();const int32 From=MillTraces(Mine,90.);
   bool bHead=Plain>7&&From==Plain&&Mine.Num()>Plain+20&&FMath::IsNearlyEqual(Mine[6].At.X,.2,.001)&&FMath::IsNearlyEqual(Mine[7].At.X,-.9,.001)&&FMath::IsNearlyEqual(Mine[7].Yaw,90.f);
   Check(bRing&&bHead&&Traced(TEXT("windmill"),47,1)&&Traced(TEXT("windmill"),47,2)&&!Traced(TEXT("windmill"),47,3)&&!Traced(TEXT("tank"),47,1),
    TEXT("day 1 at the windmill: a small circle on the place, the head turned toward the bearing, wire and tracks and flattened brush there for the first two days only"));
   FBearing Slip=StationFix(1,12.5*3600.,0,Sx,Sz,Mill->X,Mill->Z,.5f,TEXT("windmill"));ArcRing(Slip);
   FS47Places Quiet(nullptr);Quiet.Begin(nullptr);Quiet.Fixes.Add(Slip);
   const FString Line=TEXT("SARO / Somebody wired that windmill on purpose. The bearing may come back tonight.");
   Check(Quiet.SlipRows(12.6*3600.).Num()==1&&S47Script::LintText(Line).IsEmpty()&&!Line.Contains(TEXT("47")),
    TEXT("the slip is the only NOW row until the windmill is found, and the line that follows the finding is plain"));
  }
 }
 // N1110 (rhythm, item 12): one place a day, none on day 1, plain words, the four kinds all come, the same each time it is asked
 {
  int32 Days=0;bool bOne=true,bPlain=true;TSet<FString> Kinds;
  for(int32 Day=2;Day<=60;++Day)
  {
   int32 Here=0;
   for(const FDef& D:Defs()){const FString T=DayTrace(D.Id,47,Day);if(!T.IsEmpty()){++Here;Kinds.Add(T);bPlain&=S47Script::LintText(T).IsEmpty()&&!T.Contains(TEXT("taken"))&&T==DayTrace(D.Id,47,Day);}}
   bOne&=Here==1;Days+=Here;
  }
  bool bDayOne=true;for(const FDef& D:Defs())bDayOne&=DayTrace(D.Id,47,1).IsEmpty();
  Check(bOne&&Days==59&&bDayOne&&bPlain&&Kinds.Num()==6,TEXT("N1110: from day 2 one place a day carries one unexplained trace in plain words, six kinds, none on day 1"));
 }
 OK&=FS47Calm::SelfTest(Lines);
 return OK;
}
