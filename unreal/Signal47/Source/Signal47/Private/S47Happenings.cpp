#include "S47Happenings.h"
#include "S47Album.h"
#include "S47Ambience.h"
#include "S47Audio.h"
#include "S47CB.h"
#include "S47Docs.h"
#include "S47Drive.h"
#include "S47EyeGlint.h"
#include "S47EventPlaces.h"
#include "S47Visitors.h"
#include "S47FarmLights.h"
#include "S47FarmSite.h"
#include "S47FieldCamera.h"
#include "S47FieldRadio.h"
#include "S47Flashlight.h"
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
#include "S47Omen.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Radio.h"
#include "S47Rhythm.h"
#include "S47RoadGlimpse.h"
#include "S47RoadTerrain.h"
#include "S47Seen.h"
#include "S47SkyVisitors.h"
#include "S47Taken.h"
#include "S47Three.h"
#include "S47Town.h"
#include "S47TownLayer.h"
#include "S47TripoNew.h"
#include "S47Truck.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include "S47Story.h"
#include "S47Theme.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "CoreGlobals.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
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
 // Standing, in one muted tone in the headlights (the radio woman sits, so she would not do).
 const TCHAR* const HitcherModel=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_waitress.SM_Tripo_waitress");
 const TCHAR* const DriverModel=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_patron.SM_Tripo_patron");
 const TCHAR* const RigModel=TEXT("/Game/Licensed/TripoFleet/heavy_truck/SM_TripoFleet_HeavyTruck.SM_TripoFleet_HeavyTruck");
 const TCHAR* const RigStandIn=TEXT("/Game/Free/Vehicles/muteki_taiga/StaticMeshes/muteki_taiga.muteki_taiga");

 bool Exists(const TCHAR* Path){return Path&&*Path&&FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(FString(Path)));}
 const TCHAR* Either(const TCHAR* Own,const TCHAR* StandIn){return Exists(Own)?Own:Exists(StandIn)?StandIn:nullptr;}
 double HourOf(double Clock){return FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;}
 // Due: from its hour until it is gone, and never after six. Stale: in the night's own hours, an hour past its time.
 bool DueAt(double H,double From,double Until){return H>=From&&H<FMath::Min(30.,Until);}
 bool StaleAt(double H,double From){return H>=19&&H<29.5&&H>=From+1.;}
 FString ClockText(double Clock){const int32 S=int32(FMath::Fmod(FMath::Max(0.,Clock),86400.));return FString::Printf(TEXT("%02d:%02d"),S/3600,(S/60)%60);}
 FString Compass(double Az)
 {
  static const TCHAR* const Names[]={TEXT("north"),TEXT("north-east"),TEXT("east"),TEXT("south-east"),TEXT("south"),TEXT("south-west"),TEXT("west"),TEXT("north-west")};
  return Names[int32(FMath::FloorToDouble((FMath::Fmod(Az+360.,360.)+22.5)/45.))%8];
 }
 uint32 SaltOf(const FString& Id){return FCrc::StrCrc32(*Id);}
 // Where the player is, as an event asks for it (FS47Happenings::SituationNow, NeedMask).
 constexpr uint32 NeedDrive=1,NeedFoot=2,NeedIndoors=4,NeedSaro=8,NeedTown=16,NeedMotel=32,NeedSmall=64;
 // Near: it happens at the player, seen or heard where they stand (the reserve between 23 and 01 takes only these).
 constexpr uint32 NeedNear=128;
 // Near enough to the player to count as near them, metres (the tally, and the reserve's "something near").
 constexpr double NearByM=400;
 // What does not hold the ground: lasting things out on the land, the clock, a word on the radio.
 bool Background(const FString& Id)
 {
  static const TCHAR* const L[]={TEXT("clock_ahead"),TEXT("cattle_ring"),TEXT("stranded_rv"),TEXT("carcass"),TEXT("house_lit"),TEXT("cinema_film"),TEXT("missing_person"),TEXT("truck_tracks"),TEXT("hood_warm"),TEXT("am_caller")};
  for(const TCHAR* X:L)if(Id==X)return true;
  return false;
 }
 // The site each lasting one leaves behind it.
 const TCHAR* SiteKindOf(const FString& Id)
 {
  return Id==TEXT("cattle_ring")?TEXT("cattle"):Id==TEXT("stranded_rv")?TEXT("rv"):Id==TEXT("carcass")?TEXT("carcass"):Id==TEXT("house_lit")?TEXT("houselit")
   :Id==TEXT("cinema_film")?TEXT("cinema"):Id==TEXT("missing_person")?TEXT("missing"):Id==TEXT("truck_tracks")?TEXT("tracks"):TEXT("");
 }
 // A bearing in whole degrees, 0 to 359, that never reads as Story's number (47, 147 ...): a degree on is as good a bearing.
 double Bearing(double Az)
 {
  int32 D=FMath::RoundToInt(FMath::Fmod(FMath::Fmod(Az,360.)+360.,360.))%360;
  if(FString::FromInt(D).Contains(TEXT("47")))D=(D+1)%360;
  return double(D);
 }
 constexpr uint32 Bit(ES47PlaceTrait T){return uint32(T);}
 // What the stores and the land call them in the log: never a name, never Story's far side.
 const TCHAR* const FarSide[]={TEXT("forty-seven"),TEXT("forty seven"),TEXT("47"),TEXT("tomas"),TEXT("tomás"),TEXT("silhouette"),TEXT("cable"),TEXT("kinski"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),TEXT("abduct"),TEXT("8.15"),TEXT("the event"),TEXT("elvis"),TEXT("bigfoot"),TEXT("sasquatch"),TEXT("heisenberg"),TEXT("cadillac"),TEXT("thermos"),TEXT("cut clean"),TEXT("cleanly cut"),TEXT("this records")};
// As whole words: a name of Story's (Magdalena is a town), makes and brands nobody here names, and British spelling in an American 1986.
const TCHAR* const NotSaid[]={TEXT("dale"),TEXT("chevy"),TEXT("chevrolet"),TEXT("ford"),TEXT("dodge"),TEXT("gmc"),TEXT("jeep"),TEXT("winnebago"),TEXT("coke"),TEXT("pepsi"),TEXT("tyre"),TEXT("tyres"),TEXT("windscreen"),TEXT("grey"),TEXT("programme"),TEXT("colour")};
bool WholeWord(const FString& L,const TCHAR* W)
{
 const int32 N=FCString::Strlen(W);
 for(int32 At=L.Find(W,ESearchCase::CaseSensitive);At!=INDEX_NONE;At=L.Find(W,ESearchCase::CaseSensitive,ESearchDir::FromStart,At+1))
 {
  const bool bBefore=At>0&&FChar::IsAlpha(L[At-1]),bAfter=At+N<L.Len()&&FChar::IsAlpha(L[At+N]);
  if(!bBefore&&!bAfter)return true;
 }
 return false;
}
bool Clean(const FString& Text)
{
 const FString L=Text.ToLower();
 for(const TCHAR* W:FarSide)if(L.Contains(W))return false;
 for(const TCHAR* W:NotSaid)if(WholeWord(L,W))return false;
 return true;
}
// What may carry on after a Continue: it stands where it stood and needs nothing else of the world. The rest can be drawn again.
bool Resumable(const FString& Id,const FString& Variant)
{
 return Id==TEXT("landing")||Id==TEXT("horizon_figure")||Id==TEXT("dust_devil")||Id==TEXT("room6_tv")||Id==TEXT("phone_empty")||(Id==TEXT("humor")&&Variant!=TEXT("crossing"));
}
// The records the night's events keep, by the start of their keys (FS47Happenings::IsNightFinding). Humour and pay are not among them.
const TCHAR* const NightKeys[]={TEXT("cattle_ring_d"),TEXT("car_dies_d"),TEXT("clock_ahead_d"),TEXT("cb_echo_d"),TEXT("cb_empty_d"),TEXT("cb_wrong_d"),TEXT("landing_seen_d"),TEXT("landing_logged_d"),TEXT("hitcher_d"),TEXT("rv_d"),
 TEXT("carcass_d"),TEXT("house_lit_d"),TEXT("horizon_figure_d"),TEXT("cinema_film_d"),TEXT("room6_tv_d"),TEXT("blackout_d"),TEXT("array_burst_d"),TEXT("hood_warm_d"),TEXT("truck_tracks_d"),
 TEXT("animals_flee_d"),TEXT("car_light_d"),TEXT("dust_devil_d"),TEXT("missing_d"),TEXT("dogs_hush_d"),TEXT("phone_empty_d"),TEXT("geiger_landing_"),TEXT("geiger_trace_"),TEXT("geiger_sample_"),
 TEXT("figure_near_d"),TEXT("lights_trace_bignight_d"),TEXT("big_felt_d"),TEXT("road_glimpse_d"),TEXT("road_tracks_d"),
 TEXT("answer_d")}; // LOOP-3-20261109: the answer on the monitor channel (S47Answer) is a night record of OUT ON THE LAND too
// What a line may not say (the text critic, 8 November): the answer for the player, and the motifs another system owns. One line.
const TCHAR* const Concludes[]={TEXT("nobody in it"),TEXT("nobody lives there"),TEXT("nobody with it"),TEXT("it had it right"),TEXT("seemed embarrassed"),TEXT("nothing behind the station"),TEXT("looks like your handwriting"),TEXT("the way the lights came down"),TEXT("as if a film were running"),TEXT("does not seem to have heard"),TEXT("nothing out there for anyone")};
bool Concluding(const FString& Text){const FString L=Text.ToLower();for(const TCHAR* W:Concludes)if(L.Contains(W))return true;return false;}

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
  // Seen 8 November: a pale inner disc read as a white platform and the brush as traffic cones. Ground tones, and brush as low clumps.
  const uint32 Burnt=0x15110e,Ash=0x332f2b,Pale=0x6a604f,Bug=0x16140f,Dry=0x3a2e22;
  TArray<FPiece> Out={
   {Cyl,FVector(0,.012,0),FVector(8.6,.02,8.6),0,0,0,Burnt,0,true},
   {Cyl,FVector(0,.018,0),FVector(6.4,.02,6.4),0,0,0,Pale,0,true},
   {Cyl,FVector(.3,.022,-.2),FVector(2.2,.015,2.2),0,0,0,Ash,0,false},
  };
  const FVector Feet[4]={FVector(-2.1,0,-1.8),FVector(2.1,0,-1.9),FVector(2.2,0,2.0),FVector(-2.0,0,2.1)};
  for(int32 I=0;I<4;++I)Out.Add({Cube,Feet[I]+FVector(0,.02,0),FVector(.55,.03,.75),.15*I,0,0,Burnt,0,true});
  for(int32 I=0;I<9;++I){const double A=I*.83+.4,R=.9+(I%4)*.55;Out.Add({Ball,FVector(FMath::Cos(A)*R,.03,FMath::Sin(A)*R),FVector(.045,.025,.06),A,0,0,Bug,0,false});}
  Out.Add({Cube,FVector(1.3,.03,.6),FVector(.05,.03,.22),.7,0,0,Bug,0,false});
  for(int32 I=0;I<6;++I){const double A=I*1.05+.2,R=3.3+(I%2)*.4;Out.Add({Ball,FVector(FMath::Cos(A)*R,.12,FMath::Sin(A)*R),FVector(.8,.3,.7),A,0,0,Dry,0,false});}
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
 // A small gray man in a short-sleeved shirt and a hat, from basic shapes until Tom's Zet is in.
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
 // Unease (URO.md): what a few of them need drawn. Basic shapes; nothing here is a reward or a thing to use.
 TArray<FPiece> ChairParts()
 {
  const uint32 Wood=0x4a3a2a;
  TArray<FPiece> Out={{Cube,FVector(0,.45,0),FVector(.44,.04,.42),0,0,0,Wood,0,false},{Cube,FVector(0,.72,.2),FVector(.44,.5,.04),0,-.08,0,Wood,0,false}};
  for(const FVector2D L:{FVector2D(-.19,-.18),FVector2D(.19,-.18),FVector2D(-.19,.18),FVector2D(.19,.18)})Out.Add({Cube,FVector(L.X,.22,L.Y),FVector(.035,.44,.035),0,0,0,Wood,0,false});
  return Out;
 }
 TArray<FPiece> StareParts()
 {
  TArray<FPiece> Out;
  const FVector At[4]={FVector(-3.2,0,.4),FVector(-.9,0,-.6),FVector(1.6,0,.3),FVector(3.8,0,-.4)};
  for(const FVector& C:At){Out.Add({CowBody,C,FVector(1),0,0,0,0,0,false});Out.Add({CowHead,C+S47Herd::NECK,FVector(1),0,.08,0,0,0,false});}
  return Out;
 }
 // A coyote on its side in the lane.
 TArray<FPiece> CarcassParts()
 {
  const uint32 Fur=0x6b5a44,Dark=0x2a231c;
  return {{Ball,FVector(0,.16,0),FVector(.34,.3,.95),0,0,1.4,Fur,0,false},{Ball,FVector(0,.14,-.58),FVector(.22,.2,.3),0,0,1.2,Fur,0,false},
   {Cone,FVector(.05,.2,-.72),FVector(.06,.12,.06),0,0,0,Dark,0,false},{Cube,FVector(.1,.08,.62),FVector(.07,.07,.45),0,.3,0,Fur,0,false},
   {Cube,FVector(.22,.06,.15),FVector(.05,.05,.42),0,0,.9,Dark,0,false},{Cube,FVector(.22,.06,-.25),FVector(.05,.05,.4),0,0,.9,Dark,0,false}};
 }
 // A lit window far off: warm glass in a dark frame.
 TArray<FPiece> WindowParts()
 {
  return {{Cube,FVector(0,0,0),FVector(1.1,.9,.05),0,0,0,0xffc27a,9.f,false},{Cube,FVector(0,0,-.03),FVector(1.25,1.05,.04),0,0,0,0x1a1612,0,false}};
 }
 // The rest of the 31: what a few of them need drawn.
 TArray<FPiece> PhoneParts(){return {{Cube,FVector(0,0,0),FVector(.22,.3,.12),0,0,0,0x2b2b2b,0,true},{Cube,FVector(0,.05,.07),FVector(.06,.2,.05),0,0,0,0x101010,0,true}};}
 // A cow on her side in a ring of flattened grass.
 TArray<FPiece> DeadCowParts()
 {
  return {{Cyl,FVector(0,.01,0),FVector(7,.02,7),0,0,0,0x4f4535,0,false},{CowBody,FVector(0,.42,0),FVector(1),0,0,1.5,0,0,true},{CowHead,FVector(0,.42,0)+FVector(0,-.3,-.74),FVector(1),0,0,1.5,0,0,true}};
 }
 TArray<FPiece> RabbitParts()
 {
  TArray<FPiece> Out;
  for(int32 I=0;I<12;++I){const double X=(I%4)*1.6-2.4+(I%3)*.4,Z=(I/4)*2.1-2.;Out.Add({Ball,FVector(X,.16,Z),FVector(.2,.22,.42),0,-.2,0,0x5d5040,0,false});Out.Add({Ball,FVector(X,.3,Z-.22),FVector(.12,.14,.14),0,0,0,0x5d5040,0,false});}
  return Out;
 }
 // Dust turning in a column: rings of it, wider and fainter going up.
 TArray<FPiece> DevilParts()
 {
  TArray<FPiece> Out;
  for(int32 I=0;I<9;++I){const double Y=.4+I*1.3,R=.9+I*.32;Out.Add({Cyl,FVector(FMath::Sin(I*.9)*.3,Y,FMath::Cos(I*.9)*.3),FVector(R,.5,R*.85),I*.7,.08*FMath::Sin(I*1.3),0,0x8a7a5e,0,false});}
  return Out;
 }
 // Small bare prints in a ring round the truck, stopping.
 TArray<FPiece> TrackParts()
 {
  TArray<FPiece> Out;
  for(int32 I=0;I<22;++I){const double A=I*2.*UE_DOUBLE_PI/26.,R=3.7+(I%2)*.25;Out.Add({Ball,FVector(FMath::Cos(A)*R,.015,FMath::Sin(A)*R),FVector(.11,.02,.24),-A,0,0,0x2a241c,0,false});}
  return Out;
 }
 // GLIMPSE-ROAD-20261109: what crossed the road far ahead leaves prints going off the far shoulder into the brush, along +z from the
 // edge: the tall one's long and narrow, a long stride apart; the low one's small and round, in fours, close together.
 TArray<FPiece> RoadTrackParts(bool bLow)
 {
  TArray<FPiece> Out;const uint32 Print=0x2a241c;
  if(bLow){for(int32 I=0;I<7;++I)for(int32 K=0;K<4;++K)Out.Add({Ball,FVector((K%2)*.2-.1,.015,.2+I*.75+(K/2)*.28),FVector(.08,.02,.09),0,0,0,Print,0,false});}
  else for(int32 I=0;I<6;++I)Out.Add({Ball,FVector((I%2)*.26-.13,.015,.3+I*1.15),FVector(.1,.02,.32),0,0,0,Print,0,false});
  return Out;
 }
 // The figure on the rise or on foot: the shade as before, or Tom's Nordic or Cosmic (its Variant, S47RoadGlimpse::FigureFor).
 FString ShadeKind(const FString& Body){return Body.IsEmpty()?FString(TEXT("shade")):TEXT("glimpse_")+Body;}
 const TCHAR* const PickupModel=TEXT("/Game/Free/Vehicles/pickup_c10_1966/StaticMeshes/pickup_c10_1966.pickup_c10_1966");
 const TCHAR* const DarkCarModel=TEXT("/Game/Free/Vehicles/fairheaven_lt80/StaticMeshes/fairheaven_lt80.fairheaven_lt80");
 struct FUneaseDef{const TCHAR* Id;const TCHAR* Title;};
 // In URO.md's order: its number is the index plus one.
 const FUneaseDef UneaseDefs[]={
  {TEXT("car_alarm"),TEXT("The truck's alarm goes off")},{TEXT("knock"),TEXT("Three knocks on the door")},{TEXT("name"),TEXT("Your name, behind you")},
  {TEXT("steps"),TEXT("Steps that stop when you stop")},{TEXT("dog_cut"),TEXT("A dog cut off mid-bark")},{TEXT("tap"),TEXT("A tap running")},
  {TEXT("radio_song"),TEXT("The radio finds an old song")},{TEXT("phone_breath"),TEXT("The phone, and breathing")},{TEXT("child"),TEXT("Something like a laugh, far out")},
  {TEXT("hum_stops"),TEXT("The hum stops")},{TEXT("window_out"),TEXT("A far window goes dark")},{TEXT("chair"),TEXT("A chair has moved")},
  {TEXT("car_door"),TEXT("The truck's cab is lit when you come back")},{TEXT("shadow_door"),TEXT("A shadow under the door")},{TEXT("handprint"),TEXT("A handprint inside the windshield")},
  {TEXT("lamp_spot"),TEXT("The lamp flickers at one spot")},{TEXT("clock_stops"),TEXT("The wall clock stands still")},{TEXT("photo_figure"),TEXT("A figure on the next photograph")},
  {TEXT("mirror"),TEXT("The mirror, a moment early")},{TEXT("note"),TEXT("A note in your own hand")},{TEXT("stare"),TEXT("Cattle stand and look at SARO")},
  {TEXT("roadkill"),TEXT("A dead animal on the road, then not")},{TEXT("lamps_out"),TEXT("Caliche's lamps go out one by one")},{TEXT("dark_car"),TEXT("A car without lights behind you")},
  {TEXT("birds"),TEXT("All the birds go up at once")},
 };
 // M7, the big night (S47BigNight.cpp). A column of pale light some nine hundred metres high, a brighter core in it, and the ground
 // lit white where it stands; the parts are placed from its foot so that it grows up off the land as its height is scaled.
 TArray<FPiece> BigColumnParts()
 {
  return {{Cyl,FVector(0,450,0),FVector(44,900,44),0,0,0,0xcfdcf4,22.f,false},{Cyl,FVector(0,450,0),FVector(14,900,14),0,0,0,0xffffff,80.f,false},
   {Cyl,FVector(0,.6,0),FVector(70,1.2,70),0,0,0,0xe6eeff,18.f,false}};
 }
 // A dozen small lights close together, at uneven heights, that rise as one.
 TArray<FPiece> BigRiseParts()
 {
  TArray<FPiece> Out;
  for(int32 I=0;I<12;++I){const double A=I*2.4+.3,R=22.+(I%4)*14.;Out.Add({Ball,FVector(FMath::Cos(A)*R,(I%3)*12.-12.,FMath::Sin(A)*R),FVector(16),0,0,0,0xfff2d8,80.f,false});}
  return Out;
 }
 // What it leaves. Glassed ground: a dark disc gone to glass in a ring of pale ash, and small bare prints round it twice and off.
 TArray<FPiece> BigMarkColumn()
 {
  const uint32 Ash=0x4e483e,Glass=0x08090a,Crack=0x2a2824,Print=0x2b251d;
  TArray<FPiece> Out={{Cyl,FVector(0,.006,0),FVector(6.6,.01,6.6),0,0,0,Ash,0,true},{Cyl,FVector(0,.012,0),FVector(2.1,.012,2.1),0,0,0,Glass,0,true}};
  for(int32 I=0;I<5;++I)Out.Add({Cube,FVector(0,.02,0),FVector(.025,.004,1.9-.2*I),I*.63,0,0,Crack,0,false});
  for(int32 I=0;I<26;++I){const double A=I*2.*UE_DOUBLE_PI/13.,R=2.55+(I/13)*.32;Out.Add({Ball,FVector(FMath::Cos(A)*R,.02,FMath::Sin(A)*R),FVector(.1,.02,.22),-A,0,0,Print,0,false});}
  for(int32 I=0;I<10;++I)Out.Add({Ball,FVector((I%2)*.28-.14,.02,-3.4-I*.8),FVector(.1,.02,.22),0,0,0,Print,0,false});
  return Out;
 }
 // The furrow: burnt along its bottom, a lip of thrown caliche either side, deepest at its end with nothing there, the brush laid flat.
 TArray<FPiece> BigMarkFall()
 {
  const uint32 Floor=0x16120e,Lip=0x4e4234,Pit=0x0c0a09,Brush=0x33291e;
  TArray<FPiece> Out={{Cube,FVector(0,.008,9),FVector(1.2,.016,18),0,0,0,Floor,0,false},{Ball,FVector(.95,0,9),FVector(.7,.22,18.4),0,0,0,Lip,0,false},
   {Ball,FVector(-.95,0,9),FVector(.7,.22,18.4),0,0,0,Lip,0,false},{Cyl,FVector(0,.01,0),FVector(2.6,.014,2.6),0,0,0,Pit,0,true}};
  for(int32 I=0;I<12;++I){const double Side=I%2?1.:-1.,Z=1.5+I*1.4;Out.Add({Ball,FVector(Side*(2.4+(I%3)*.5),.01,Z),FVector(1.5,.06,.55),Side*.5,0,0,Brush,0,false});}
  return Out;
 }
 // Stones turned over: a ring of them dark side up round a middle swept bare, and cattle tracks running out of it every way.
 TArray<FPiece> BigMarkRise()
 {
  const uint32 Bare=0x5e5444,Stone=0x231e18,Track=0x2a241c;
  TArray<FPiece> Out={{Cyl,FVector(0,.006,0),FVector(6.2,.008,6.2),0,0,0,Bare,0,true}};
  for(int32 I=0;I<28;++I){const double A=I*.9+.2,R=3.3+(I%5)*.45;Out.Add({Ball,FVector(FMath::Cos(A)*R,.02,FMath::Sin(A)*R),FVector(.28,.08,.22),A,0,0,Stone,0,false});}
  for(int32 D=0;D<8;++D)for(int32 I=0;I<5;++I){const double A=D*UE_DOUBLE_PI/4.+.2,R=5.8+I*.9;Out.Add({Ball,FVector(FMath::Cos(A)*R,.02,FMath::Sin(A)*R),FVector(.16,.02,.2),-A,0,0,Track,0,false});}
  return Out;
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
 // A road glimpse's body: the truck's lights at its distance, on lighting channel 1 that only the body is on (FS47Happenings::FarBeam).
 TWeakObjectPtr<USpotLightComponent> FarBeam;
 TArray<FS47Obj> Hits;
 // The dot in each big black eye, on a part whose mesh has its eyes measured (Zet, S47EyeGlint): children of that part, so that they
 // move and hide with it (ShowProp sets its children too).
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Glints;
 // PT-NORDIC-COSMIC-20261109: the Cosmic one's light on a part that is its body (S47TripoNew::AddCosmicGlow), stepped while shown.
 TArray<S47TripoNew::FCosmicGlow> Cosmic;
 FVector Foot=FVector::ZeroVector;double Yaw=0;bool bShown=false,bPosed=false;
};

namespace S47Hap
{
 // Sounds the recordings lack, made once from a fixed seed (FS47Happenings::Synth): a whisper of two breaths, a thin far laugh, a flock
 // of wings going up, a two-note truck alarm. Mono, -1 to 1. Each is meant to be heard, not explained.
 struct FReso
 {
  // A two-pole resonator: one formant.
  double A1=0,A2=0,G=0,Y1=0,Y2=0;
  void Set(double Freq,double Width,double Hz){const double Pole=FMath::Exp(-UE_DOUBLE_PI*Width/Hz);A1=2*Pole*FMath::Cos(2*UE_DOUBLE_PI*Freq/Hz);A2=-Pole*Pole;G=1-Pole;}
  double Step(double X){const double Y=G*X+A1*Y1+A2*Y2;Y2=Y1;Y1=Y;return Y;}
 };
 void Normalise(TArray<float>& S,float Peak)
 {
  float Top=0;for(const float V:S)Top=FMath::Max(Top,FMath::Abs(V));
  if(Top>1e-6f)for(float& V:S)V*=Peak/Top;
 }
 TArray<float> Make(const FString& Name,float Hz)
 {
  TArray<float> S;
  if(Hz<8000)return S;
  FRandomStream Rng(int32(FCrc::StrCrc32(*Name)));
  const auto Noise=[&Rng]{return Rng.FRandRange(-1.f,1.f);};
  if(Name==TEXT("whisper"))
  {
   // Two unvoiced syllables, the first falling, the second round, with a little hiss between: a word, or not.
   const int32 Count=int32(Hz*.8);S.SetNumZeroed(Count);
   FReso F1,F2,F3;double Hp=0,Prev=0;
   for(int32 I=0;I<Count;++I)
   {
    const double T=I/double(Hz);
    const bool bSecond=T>=.36;
    const double In=bSecond?(T-.36)/.32:(T-.04)/.28;
    const double Env=In<0||In>1?0.:FMath::Sin(UE_DOUBLE_PI*FMath::Pow(In,.7));
    F1.Set(bSecond?480:FMath::Lerp(720.,520.,FMath::Clamp(In,0.,1.)),180,Hz);F2.Set(bSecond?950:FMath::Lerp(1850.,1500.,FMath::Clamp(In,0.,1.)),240,Hz);F3.Set(2600,400,Hz);
    const double X=Noise();
    double V=(F1.Step(X)*1.0+F2.Step(X)*.8+F3.Step(X)*.35)*Env;
    // The hiss between them.
    const double Sh=T>.31&&T<.39?FMath::Sin(UE_DOUBLE_PI*(T-.31)/.08):0.;
    Hp=.92*(Hp+X-Prev);Prev=X;V+=Hp*Sh*.18;
    S[I]=float(V);
   }
   Normalise(S,.6f);
  }
  else if(Name==TEXT("laugh"))
  {
   // Five short "ha"s, high and thin, each a breath then a voice, falling a little; far off it is a laugh, a bird, or wind.
   const int32 Count=int32(Hz*1.25);S.SetNumZeroed(Count);
   FReso F1,F2,F3;F1.Set(980,160,Hz);F2.Set(1650,200,Hz);F3.Set(2900,300,Hz);
   double Phase=0;
   for(int32 I=0;I<Count;++I)
   {
    const double T=I/double(Hz);
    const int32 K=FMath::Min(4,int32(T/.19));const double In=(T-K*.19)/.19;
    const double Amp=FMath::Pow(.82,K);
    const double Pitch=FMath::Lerp(390.,330.,T/1.25)*(1+.02*FMath::Sin(T*31.));
    Phase+=Pitch/Hz;Phase-=FMath::FloorToDouble(Phase);
    const double Voice=In>.12&&In<.62?FMath::Sin(UE_DOUBLE_PI*(In-.12)/.5):0.;
    const double Breath=In<.16?FMath::Sin(UE_DOUBLE_PI*In/.16):0.;
    const double Src=(2*Phase-1)*Voice*.9+Noise()*(Breath*.6+Voice*.08);
    S[I]=float((F1.Step(Src)+F2.Step(Src)*.7+F3.Step(Src)*.3)*Amp);
   }
   Normalise(S,.6f);
  }
  else if(Name==TEXT("wings"))
  {
   // A dozen birds off the ground at once: each a run of wingbeats, quick and fading, all of it in the air's hiss.
   const int32 Count=int32(Hz*1.9);S.SetNumZeroed(Count);
   for(int32 B=0;B<12;++B)
   {
    const double T0=Rng.FRandRange(0.f,.28f),Beat=Rng.FRandRange(9.f,16.f),Len=Rng.FRandRange(.7f,1.5f),Gain=Rng.FRandRange(.5f,1.f);
    double Lo=0,Hi=0,Prev=0;const double ALo=1-FMath::Exp(-2*UE_DOUBLE_PI*Rng.FRandRange(2500.f,4200.f)/Hz);
    for(int32 I=int32(T0*Hz);I<Count&&I<int32((T0+Len)*Hz);++I)
    {
     const double T=I/double(Hz)-T0;
     const double Flap=FMath::Pow(FMath::Abs(FMath::Sin(UE_DOUBLE_PI*Beat*T)),4.);
     const double Fade=FMath::Exp(-2.2*T/Len);
     const double X=Noise();Lo+=ALo*(X-Lo);Hi=.95*(Hi+Lo-Prev);Prev=Lo;
     S[I]+=float(Hi*Flap*Fade*Gain);
    }
   }
   Normalise(S,.7f);
  }
  else if(Name==TEXT("alarm"))
  {
   // Two notes, over and over: a horn's square, softened, one loop of both.
   const int32 Count=int32(Hz*.84);S.SetNumZeroed(Count);
   double Phase=0,Lo=0;const double A=1-FMath::Exp(-2*UE_DOUBLE_PI*2200./Hz);
   for(int32 I=0;I<Count;++I)
   {
    const double T=I/double(Hz);
    Phase+=(T<.42?440.:540.)/Hz;Phase-=FMath::FloorToDouble(Phase);
    const double Sq=Phase<.5?1.:-1.;Lo+=A*(Sq-Lo);
    const double Ramp=FMath::Min(1.,FMath::Min(FMath::Fmod(T,.42),.42-FMath::Fmod(T,.42))/.006);
    S[I]=float(Lo*1.6/(1.+FMath::Abs(Lo*1.6))*Ramp);
   }
   Normalise(S,.55f);
  }
  else if(Name==TEXT("low"))
  {
   // A cow's low, far off: a voice that rises and falls through two formants, an open "m" closing at the end.
   const int32 Count=int32(Hz*1.7);S.SetNumZeroed(Count);
   FReso F1,F2,F3;double Phase=0;
   for(int32 I=0;I<Count;++I)
   {
    const double T=I/double(Hz),In=T/1.7;
    const double Pitch=In<.35?FMath::Lerp(105.,148.,In/.35):FMath::Lerp(148.,96.,(In-.35)/.65);
    Phase+=Pitch/Hz;Phase-=FMath::FloorToDouble(Phase);
    const double Open=FMath::Sin(UE_DOUBLE_PI*FMath::Clamp(In*1.15,0.,1.));
    F1.Set(FMath::Lerp(320.,620.,Open),140,Hz);F2.Set(FMath::Lerp(820.,1050.,Open),180,Hz);F3.Set(2350,300,Hz);
    const double Env=FMath::Min(1.,T/.16)*FMath::Min(1.,(1.7-T)/.42);
    const double Src=(2*Phase-1)*.9+Noise()*.12;
    S[I]=float((F1.Step(Src)+F2.Step(Src)*.55+F3.Step(Src)*.12)*Env);
   }
   Normalise(S,.6f);
  }
  else if(Name==TEXT("bell"))
  {
   // A cowbell knocked twice as she shifts: a dull clank, the partials of a bent sheet of iron, gone quickly.
   const int32 Count=int32(Hz*1.3);S.SetNumZeroed(Count);
   const double Parts[5][2]={{540,1.},{812,.7},{1271,.45},{1718,.3},{2390,.18}};
   for(const double Strike:{0.,.42})
   {
    const double Gain=Strike>0?.6:1.;
    for(int32 I=int32(Strike*Hz);I<Count;++I)
    {
     const double T=I/double(Hz)-Strike;
     double V=0;for(const auto& P:Parts)V+=P[1]*FMath::Sin(2*UE_DOUBLE_PI*P[0]*T)*FMath::Exp(-T*(6.+P[0]/400.));
     if(T<.012)V+=Noise()*(1.-T/.012)*.8;
     S[I]+=float(V*Gain);
    }
   }
   Normalise(S,.5f);
  }
  else if(Name==TEXT("projector"))
  {
   // A projector through a wall: the claw at twenty-four a second, the motor's hum, the fan. One second, to loop.
   const int32 Count=int32(Hz);S.SetNumZeroed(Count);
   double Lo=0,Band=0,Prev=0;const double ALo=1-FMath::Exp(-2*UE_DOUBLE_PI*1800./Hz);
   for(int32 I=0;I<Count;++I)
   {
    const double T=I/double(Hz);
    const double Claw=FMath::Fmod(T*24.,1.);
    const double X=Noise();Lo+=ALo*(X-Lo);Band=.9*(Band+Lo-Prev);Prev=Lo;
    S[I]=float((Claw<.08?Band*(1.-Claw/.08)*1.4:0.)+FMath::Sin(2*UE_DOUBLE_PI*120.*T)*.12+FMath::Sin(2*UE_DOUBLE_PI*240.*T)*.05+Band*.18);
   }
   Normalise(S,.45f);
  }
  else if(Name==TEXT("croon"))
  {
   // A man humming low and slow by an empty pool: six notes with a slide between, a little vibrato, no words.
   const int32 Count=int32(Hz*3.6);S.SetNumZeroed(Count);
   const double Notes[]={196.,246.9,293.7,246.9,220.,196.};
   FReso F1,F2,F3;F1.Set(330,120,Hz);F2.Set(820,160,Hz);F3.Set(2500,350,Hz);
   double Phase=0;
   for(int32 I=0;I<Count;++I)
   {
    const double T=I/double(Hz);
    const int32 K=FMath::Min(5,int32(T/.6));const double In=(T-K*.6)/.6;
    const double From=Notes[FMath::Max(0,K-1)],To=Notes[K];
    const double Pitch=FMath::Lerp(From,To,FMath::Min(1.,In/.18))*(1+.012*FMath::Sin(2*UE_DOUBLE_PI*5.4*T));
    Phase+=Pitch/Hz;Phase-=FMath::FloorToDouble(Phase);
    const double Env=FMath::Min(1.,T/.25)*FMath::Min(1.,(3.6-T)/.5);
    const double Src=(2*Phase-1)*.8+Noise()*.03;
    S[I]=float((F1.Step(Src)+F2.Step(Src)*.4+F3.Step(Src)*.08)*Env);
   }
   Normalise(S,.5f);
  }
  else if(Name==TEXT("tick"))
  {
   // Metal cooling: a few small ticks, uneven, a ring to each.
   const int32 Count=int32(Hz*2.4);S.SetNumZeroed(Count);
   for(const double At:{.05,.61,.83,1.52,2.1})
   {
    const double F=Rng.FRandRange(2600.f,3900.f);
    for(int32 I=int32(At*Hz);I<Count&&I<int32((At+.09)*Hz);++I){const double T=I/double(Hz)-At;S[I]+=float(FMath::Sin(2*UE_DOUBLE_PI*F*T)*FMath::Exp(-T*70.)+(T<.002?Noise()*.6:0.));}
   }
   Normalise(S,.5f);
  }
  else if(Name==TEXT("lowtone"))
  {
   // M7, the big night: a low note up out of the ground, two tones three cycles apart that beat, and one an octave over. One
   // second, whole cycles of each, to loop without a seam.
   const int32 Count=int32(Hz);S.SetNumZeroed(Count);
   for(int32 I=0;I<Count;++I)
   {
    const double T=I/double(Hz);
    S[I]=float(FMath::Sin(2*UE_DOUBLE_PI*52.*T)*1.+FMath::Sin(2*UE_DOUBLE_PI*55.*T)*.8+FMath::Sin(2*UE_DOUBLE_PI*104.*T)*.3);
   }
   Normalise(S,.7f);
  }
  return S;
 }
}

FS47Happenings::FS47Happenings(US47Main* Main):M(Main){}
FString FS47Happenings::Synth(const TCHAR* Name)
{
 const FString Key=FString(TEXT("hap:"))+Name;
 US47Audio* A=M&&IsValid(M->Audio)?M->Audio.Get():nullptr;
 if(!A||!A->Ctx.IsValid()||A->Buf.Contains(Key))return Key;
 const float Hz=A->Ctx->SampleRate;
 const TArray<float> S=S47Hap::Make(Name,Hz);
 if(S.IsEmpty())return Key;
 TSharedRef<FS47AudioBuffer> B=A->Ctx->CreateBuffer(1,S.Num(),Hz);
 FMemory::Memcpy(B->GetChannelData(0),S.GetData(),S.Num()*sizeof(float));
 A->Buf.Add(Key,B);
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening sound %s made, %.2f s"),*Key,S.Num()/Hz);
 return Key;
}
// CRIT6-20261109: what a happening pays from the field (the pickup's reward, the geiger jar, the stuck rig) goes under the week's cap
// (FS47FreeformSession::EarnSide), and what is said says what came of it: the same words as the photo credit, the souvenir copies,
// 1420's tips and the sky-watch van.
namespace S47HapPay
{
 // Empty when it was paid in full.
 FString Short(int32 Paid,int32 Offered,const TCHAR* What)
 {
  if(Paid>=Offered)return FString();
  return Paid>0?FString::Printf(TEXT("FIELD MONEY / $%d of the $%d for %s: with that, this week's field money is used up."),Paid,Offered,What)
   :FString::Printf(TEXT("FIELD MONEY / Nothing for %s: this week's field money is used up."),What);
 }
 FString ReportCb(int32 Paid,const FString& Named)
 {
  return FString::Printf(TEXT("County copies, the blue half-ton by %s. I'll send a unit when I've got one. His family put up twenty dollars for word of that truck%s"),*Named,
   Paid>0?TEXT("; it'll come through your station."):TEXT("."));
 }
 FString ReportFound(int32 Paid,const FString& Title)
 {
  const FString Head=FString::Printf(TEXT("Called in the missing man's pickup out by %s to the county. They are sending a unit. "),*Title);
  return Head+(Paid>=20?FString(TEXT("His family's twenty dollars for word of it came through the station."))
   :Paid>0?FString::Printf(TEXT("$%d of his family's twenty dollars came through the station; this week's field money took no more."),Paid)
   :FString(TEXT("His family's twenty dollars stayed with them: this week's field money was used up.")));
 }
 FString JarFound(int32 Paid,const FString& Title)
 {
  return FString::Printf(TEXT("A jar of soil from the middle of the scorched ring out by %s, where the counter pegged. Sent to Dr. Ward; %s"),*Title,
   *(Paid>0?FString::Printf(TEXT("$%d for it on the sheet."),Paid):FString(TEXT("this week's field money is used up: no payment."))));
 }
 FString JarToast(int32 Paid)
 {
  return Paid>0?FString::Printf(TEXT("COUNTER / Pegged. A jar of the soil goes in the kit: Dr. Ward pays $%d for it."),Paid)
   :FString(TEXT("COUNTER / Pegged. A jar of the soil goes in the kit for Dr. Ward. This week's field money is used up: no payment."));
 }
 // The start of the stuck rig's page; the lead that follows starts "says ...".
 FString RigPaid(int32 Paid)
 {
  return Paid>=15?FString(TEXT("He counts fifteen dollars into your hand, and "))
   :Paid>0?FString::Printf(TEXT("He counts fifteen dollars into your hand. This week's field money has room for $%d of it, and you give him the rest back. He "),Paid)
   :FString(TEXT("He holds out fifteen dollars. This week's field money is used up, so you wave it off. He "));
 }
 FString RigFound(int32 Paid,const FString& Title)
 {
  return FString::Printf(TEXT("Helped a driver dig his rig out of the sand by %s, after his call on the CB. %s"),*Title,
   *(Paid>0?FString::Printf(TEXT("Paid $%d for the trouble."),Paid):FString(TEXT("Nothing paid: this week's field money was used up."))));
 }
}
// The missing man's pickup, noted, is called in at the next CB or the station's phone: the county sends a unit, and his people's
// twenty dollars for word of it comes through the station (FS47FreeformSession::SheriffTrust counts it), inside the week's cap.
void FS47Happenings::StepReport()
{
 if(!Now.Id.IsEmpty()||!M->Freeform||M->ModalOpen())return;
 FSite* S=nullptr;for(FSite& X:Sites)if(X.Kind==TEXT("missing")&&X.Variant==TEXT("noted"))S=&X;
 if(!S||!CanHearCb()||(M->CB.IsValid()&&M->CB->bBusy))return;
 auto* F=M->Freeform.Get();
 S->Variant=TEXT("reported");S->bDone=true;
 const FString Named=Spoken(S->Title);
 if(M->UI)M->UI->Toast(FString::Printf(TEXT("You call it in on channel nine: the blue pickup by %s."),*Named),5);
 const int32 Paid=F->EarnSide(20,TEXT("pickup reward"));
 Cb({S47HapPay::ReportCb(Paid,Named)},TEXT("woman"),false);
 if(M->UI){const FString Cap=S47HapPay::Short(Paid,20,TEXT("word of the pickup"));if(!Cap.IsEmpty())M->UI->Toast(Cap,5);}
 F->RecordEvidence(FString::Printf(TEXT("missing_reported_d%d"),S->Night),S47HapPay::ReportFound(Paid,S->Title));
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening missing_person: called in to the county, $%d"),Paid);
 M->RequestAutosave();
}
// The scene owns what was drawn, and may be gone already: nothing in the world is touched here.
FS47Happenings::~FS47Happenings(){Life.Reset();Alive.Reset();}

const TArray<FString>& FS47Happenings::Ids()
{
 static const TArray<FString> L={TEXT("car_dies"),TEXT("clock_ahead"),TEXT("cattle_ring"),TEXT("cb_stranded"),TEXT("cb_echo"),TEXT("landing"),TEXT("hitchhiker"),TEXT("stranded_rv"),TEXT("humor"),
  // The rest of NATTHENDELSER-31 (2, 6, 10, 12, 14 to 22, 24 to 26): Tom 8 November, all 31.
  TEXT("dogs_hush"),TEXT("phone_empty"),TEXT("carcass"),TEXT("house_lit"),TEXT("horizon_figure"),TEXT("cinema_film"),TEXT("room6_tv"),TEXT("town_blackout"),
  TEXT("array_burst"),TEXT("am_caller"),TEXT("hood_warm"),TEXT("truck_tracks"),TEXT("animals_flee"),TEXT("car_light"),TEXT("dust_devil"),TEXT("missing_person"),
  // On foot, a danger (Tom's critics, 8 November: M1 had none for a player on foot).
  TEXT("figure_near"),
  // GLIMPSE-ROAD-20261109: at the wheel, something crosses the road far ahead in the lights (S47RoadGlimpse).
  TEXT("road_glimpse"),
  // M7: the first big night, once in a game on night 2 or 3 (S47BigNight.cpp).
  TEXT("big_night")};
 return L;
}
bool FS47Happenings::IsDanger(const FString& Id){return Id==TEXT("cb_echo")||Id==TEXT("hitchhiker")||Id==TEXT("car_light")||Id==TEXT("figure_near");}

// ---------------------------------------------------------------- saving
void FS47Happenings::Begin(const S47J::FObj& Saved)
{
 Stop();Life=MakeShared<uint8>(0);
 Now=FActive();Sites.Reset();Past.Reset();LastDays.Reset();Deeds.Reset();Plan.Reset();Says.Reset();GeigerNoted.Reset();TryAt.Reset();LateTryClock=0;
 HoodNight=0;Uneasy=FUneasy();UneaseOnce.Reset();UneaseDone.Reset();UneasePool.Reset();UneaseAt.Reset();UneaseNight=UneaseWant=DangerSeen=0;DangerClock=-1e9;UneaseTry=0;OutsideSince=TruckAwaySince=-1;bWasDriving=bWasIndoors=bChairUp=false;
 Serial=0;HumorNight=-100;EchoFrom=0;LateNight=0;PlanNight=0;AheadNight=0;AheadSeconds=0;bLate=false;bEchoArmed=false;HumorKind.Empty();LastArea.Empty();
 MetTotal=0;HumorSeen.Reset();GhostNight=0;UneaseLast.Reset();Cues.Reset();LateAt=24.5;bLateHush=false;ReturnedAt=-1e9;ReturnedAway=0;
 Glimpse=S47RoadGlimpse::FPlan();Paced=0;NearK=1;DangerAt=-1;Tally=FTally();NoticedSerials.Reset();RefusedSaid.Reset();LateDrawSaid.Reset();LoopsOn.Reset();SoundNext=0;HeldTips.Reset();FigureSeen.Reset();FigureSeenFor=0;SwapTry=0;RumorTry=0;
 DimLeft=0;bDimmed=false;Dose=0;Needle=0;TickDebt=0;GeigerUpFor=0;GeigerNoteIn=0;bGeigerUp=false;bGeigerHeld=false;bDevGeiger=false;bGeigerAway=false;
 ResetBig();
 if(Saved.IsValid())
 {
  LoadBig(S47J::Obj(Saved,TEXT("big")));
  const S47J::FObj Last=S47J::Obj(Saved,TEXT("last"));
  for(const FString& Id:Ids()){const int32 D=S47J::Int(Last,*Id,0);if(D>=1&&D<100000)LastDays.Add(Id,D);}
  Serial=FMath::Clamp(S47J::Int(Saved,TEXT("serial")),0,1000000);
  HumorNight=S47J::Int(Saved,TEXT("humor"),-100);
  bEchoArmed=S47J::Bool(Saved,TEXT("echo"));EchoFrom=FMath::Max(0,S47J::Int(Saved,TEXT("echoFrom")));
  AheadNight=FMath::Max(0,S47J::Int(Saved,TEXT("aheadNight")));AheadSeconds=FMath::Clamp(S47J::Num(Saved,TEXT("ahead")),0.,7200.);
  for(const FString& D:S47J::Strs(Saved,TEXT("deeds")))if(!D.IsEmpty()&&D.Len()<200&&Deeds.Num()<6)Deeds.Add(D);
  for(const FString& U:S47J::Strs(Saved,TEXT("uneaseOnce")))if(U==TEXT("name")||U==TEXT("note"))UneaseOnce.AddUnique(U);
  UneaseNight=FMath::Max(0,S47J::Int(Saved,TEXT("uneaseNight")));
  LateNight=FMath::Max(0,S47J::Int(Saved,TEXT("lateNight")));bLate=S47J::Bool(Saved,TEXT("late"));
  MetTotal=FMath::Clamp(S47J::Int(Saved,TEXT("met")),0,10000);GhostNight=FMath::Max(0,S47J::Int(Saved,TEXT("ghostNight")));
  for(const FString& H:S47J::Strs(Saved,TEXT("humorSeen")))if((H==TEXT("car")||H==TEXT("singer")||H==TEXT("crossing")||H==TEXT("pump"))&&!HumorSeen.Contains(H))HumorSeen.Add(H);
  {
   const S47J::FObj U=S47J::Obj(Saved,TEXT("uneaseLast"));
   if(U.IsValid())for(const auto& KV:U->Values){const int32 I=FCString::Atoi(*KV.Key);double N=0;if(I>=0&&I<UneaseCount()&&KV.Value.IsValid()&&KV.Value->TryGetNumber(N)&&N>0&&N<100000)UneaseLast.Add(I,int32(N));}
  }
  for(const double U:S47J::Nums(Saved,TEXT("uneaseDone")))if(U>=0&&U<UneaseCount()&&UneaseDone.Num()<4)UneaseDone.AddUnique(int32(U));
  // LOOP-4-20261109: what stood out there and has gone, x, z and night in threes.
  {const TArray<double> P=S47J::Nums(Saved,TEXT("past"));for(int32 I=0;I+2<P.Num()&&Past.Num()<MostPast;I+=3)if(FMath::IsFinite(P[I])&&FMath::IsFinite(P[I+1])&&FMath::Abs(P[I])<1e5&&FMath::Abs(P[I+1])<1e5&&P[I+2]>=1&&P[I+2]<100000)Past.Add({P[I],P[I+1],int32(P[I+2])});}
  for(const auto& V:S47J::Arr(Saved,TEXT("sites")))
  {
   if(!V.IsValid()||V->Type!=EJson::Object)continue;
   const S47J::FObj O=V->AsObject();
   FSite S;S.Kind=S47J::Str(O,TEXT("kind"));S.Variant=S47J::Str(O,TEXT("variant"));S.Place=S47J::Str(O,TEXT("place"));S.Title=S47J::Str(O,TEXT("title"));
   S.Night=S47J::Int(O,TEXT("night"));S.Serial=S47J::Int(O,TEXT("serial"));S.X=S47J::Num(O,TEXT("x"));S.Z=S47J::Num(O,TEXT("z"));S.Yaw=S47J::Num(O,TEXT("yaw"));
   S.Az=S47J::Num(O,TEXT("az"),-1);S.Spread=S47J::Num(O,TEXT("spread"));S.Until=S47J::Num(O,TEXT("until"));
   S.bFound=S47J::Bool(O,TEXT("found"));S.bDone=S47J::Bool(O,TEXT("done"));S.bHeard=S47J::Bool(O,TEXT("heard"));
   // Before a landing's bearing was set as it came down, a bearing on it meant it was known.
   S.bKnown=O->HasField(TEXT("known"))?S47J::Bool(O,TEXT("known")):S.Az>=0;
   const bool bKind=S.Kind==TEXT("landing")||S.Kind==TEXT("truck")||S.Kind==TEXT("rv")||S.Kind==TEXT("cattle")||S.Kind==TEXT("carcass")||S.Kind==TEXT("houselit")||S.Kind==TEXT("cinema")||S.Kind==TEXT("missing")||S.Kind==TEXT("tracks")||S.Kind==TEXT("burst")||S.Kind==TEXT("bigmark")||S.Kind==TEXT("roadtracks");
   if(bKind&&S.Night>0&&FMath::IsFinite(S.X)&&FMath::IsFinite(S.Z)&&FMath::Abs(S.X)<1e5&&FMath::Abs(S.Z)<1e5&&FMath::IsFinite(S.Until)&&Sites.Num()<16)Sites.Add(S);
  }
  // What was on when it was saved, if it can carry on (S47Hap::Resumable): otherwise it may be drawn again.
  const S47J::FObj Was=S47J::Obj(Saved,TEXT("now"));
  if(Was.IsValid())
  {
   FActive A;A.Id=S47J::Str(Was,TEXT("id"));A.Variant=S47J::Str(Was,TEXT("variant"));A.Place=S47J::Str(Was,TEXT("place")).Left(64);A.Title=S47J::Str(Was,TEXT("title")).Left(120);
   A.Stage=FMath::Clamp(S47J::Int(Was,TEXT("stage")),0,9);A.Serial=S47J::Int(Was,TEXT("serial"));
   A.Age=S47J::Num(Was,TEXT("age"));A.Len=S47J::Num(Was,TEXT("len"));A.X=S47J::Num(Was,TEXT("x"));A.Z=S47J::Num(Was,TEXT("z"));A.Yaw=S47J::Num(Was,TEXT("yaw"));A.Clock=S47J::Num(Was,TEXT("clock"));
   A.At=FVector(S47J::Num(Was,TEXT("ax")),S47J::Num(Was,TEXT("ay")),S47J::Num(Was,TEXT("az")));A.bSeen=S47J::Bool(Was,TEXT("seen"));A.bMet=S47J::Bool(Was,TEXT("met"));A.bDanger=IsDanger(A.Id);
   const bool bFinite=FMath::IsFinite(A.Age)&&FMath::IsFinite(A.Len)&&FMath::IsFinite(A.X)&&FMath::IsFinite(A.Z)&&!A.At.ContainsNaN()&&A.Len>0&&A.Len<86400&&FMath::Abs(A.X)<1e5&&FMath::Abs(A.Z)<1e5;
   // A figure on the rise saved with Tom's Nordic or Cosmic, loaded in a build without them (the private TripoNew models are kept
   // out of Git): the shade stands there instead, as a new one would be drawn (FigureFor); without the shade's body either, it is
   // not carried on at all, so that nothing unseen is seen (critic round 7).
   bool bBody=true;
   if(A.Id==TEXT("horizon_figure"))
   {
    if(!A.Variant.IsEmpty()){const TCHAR* P=S47RoadGlimpse::PathOf(A.Variant);if(!P||!S47Hap::Exists(P))A.Variant.Reset();}
    bBody=S47Hap::Exists(S47Hap::HairyStandIn)||!A.Variant.IsEmpty();
   }
   if(Ids().Contains(A.Id)&&S47Hap::Resumable(A.Id,A.Variant)&&bFinite&&bBody){A.Age=FMath::Clamp(A.Age,0.,A.Len);Now=A;}
  }
 }
 if(M&&IsValid(M->Web.Get())&&Scene.Get()!=M->Web.Get()){Props.Reset();Scene=M->Web;}
 if(M)Sight.Init(M);
 // The room's television, carried on: its sound again.
 if(Now.Id==TEXT("room6_tv")&&M&&IsValid(M->Audio)&&!IsEngineExitRequested())M->Audio->TvHiss(Now.At);
 // The counter's own key (S47Keys "counter"), once: through the session, so that a new one finds it too.
 if(M&&!M->Keys.ContainsByPredicate([](const US47Main::FKeyHandler& K){return K.Action==FName(TEXT("counter"));}))
 {
  US47Main* Owner=M;
  M->OnAction(TEXT("counter"),[Owner]{return IsValid(Owner)&&Owner->IsFreeform()&&Owner->Freeform&&Owner->Freeform->Happenings&&Owner->Freeform->Happenings->ToggleCounter();});
 }
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
 if(!UneaseOnce.IsEmpty())S47J::Set(O,TEXT("uneaseOnce"),UneaseOnce);
 if(!UneaseDone.IsEmpty()){TArray<double> Done;for(const int32 U:UneaseDone)Done.Add(U);S47J::Set(O,TEXT("uneaseNight"),UneaseNight);S47J::Set(O,TEXT("uneaseDone"),Done);}
 if(!UneaseLast.IsEmpty()){auto U=S47J::New();for(const auto& KV:UneaseLast)S47J::Set(U,*FString::FromInt(KV.Key),KV.Value);S47J::Set(O,TEXT("uneaseLast"),U);}
 if(LateNight>0){S47J::Set(O,TEXT("lateNight"),LateNight);if(bLate)S47J::Set(O,TEXT("late"),true);}
 if(MetTotal>0)S47J::Set(O,TEXT("met"),MetTotal);
 if(GhostNight>0)S47J::Set(O,TEXT("ghostNight"),GhostNight);
 if(!HumorSeen.IsEmpty())S47J::Set(O,TEXT("humorSeen"),HumorSeen);
 if(const S47J::FObj Big=SaveBig())S47J::Set(O,TEXT("big"),Big);
 if(!Past.IsEmpty()){TArray<double> P;for(const FStir& X:Past){P.Add(FMath::RoundToDouble(X.X*10)/10);P.Add(FMath::RoundToDouble(X.Z*10)/10);P.Add(X.Night);}S47J::Set(O,TEXT("past"),P);} // LOOP-4-20261109
 if(!Now.Id.IsEmpty()&&!Now.bForced&&S47Hap::Resumable(Now.Id,Now.Variant))
 {
  auto N=S47J::New();
  S47J::Set(N,TEXT("id"),Now.Id);if(!Now.Variant.IsEmpty())S47J::Set(N,TEXT("variant"),Now.Variant);if(!Now.Place.IsEmpty())S47J::Set(N,TEXT("place"),Now.Place);if(!Now.Title.IsEmpty())S47J::Set(N,TEXT("title"),Now.Title);
  S47J::Set(N,TEXT("stage"),Now.Stage);S47J::Set(N,TEXT("serial"),Now.Serial);S47J::Set(N,TEXT("age"),Now.Age);S47J::Set(N,TEXT("len"),Now.Len);
  S47J::Set(N,TEXT("x"),Now.X);S47J::Set(N,TEXT("z"),Now.Z);S47J::Set(N,TEXT("yaw"),Now.Yaw);S47J::Set(N,TEXT("clock"),Now.Clock);
  S47J::Set(N,TEXT("ax"),Now.At.X);S47J::Set(N,TEXT("ay"),Now.At.Y);S47J::Set(N,TEXT("az"),Now.At.Z);
  if(Now.bSeen)S47J::Set(N,TEXT("seen"),true);if(Now.bMet)S47J::Set(N,TEXT("met"),true);
  S47J::Set(O,TEXT("now"),N);
 }
 if(!Sites.IsEmpty())
 {
  TArray<TSharedPtr<FJsonValue>> List;
  for(const FSite& S:Sites)
  {
   auto X=S47J::New();S47J::Set(X,TEXT("kind"),S.Kind);if(!S.Variant.IsEmpty())S47J::Set(X,TEXT("variant"),S.Variant);if(!S.Place.IsEmpty())S47J::Set(X,TEXT("place"),S.Place);S47J::Set(X,TEXT("title"),S.Title);
   S47J::Set(X,TEXT("night"),S.Night);S47J::Set(X,TEXT("serial"),S.Serial);S47J::Set(X,TEXT("x"),S.X);S47J::Set(X,TEXT("z"),S.Z);S47J::Set(X,TEXT("yaw"),S.Yaw);
   S47J::Set(X,TEXT("az"),S.Az);S47J::Set(X,TEXT("spread"),S.Spread);S47J::Set(X,TEXT("until"),S.Until);
   if(S.bFound)S47J::Set(X,TEXT("found"),true);if(S.bDone)S47J::Set(X,TEXT("done"),true);if(S.bHeard)S47J::Set(X,TEXT("heard"),true);
   S47J::Set(X,TEXT("known"),S.bKnown);
   List.Add(MakeShared<FJsonValueObject>(X));
  }
  O->SetArrayField(TEXT("sites"),List);
 }
 return O;
}

void FS47Happenings::Stop()
{
 Life.Reset();
 // The big night's hold on SARO's power, the truck's lamps and the motors is let go first, whatever else is.
 EndBigEffects();
 // At process exit the object array may already be gone: IsValid(M) itself faults there (seen in the Reyes and journey runs
 // after the test passed). Only plain state is reset then.
 if(IsEngineExitRequested()||GExitPurge){Uneasy=FUneasy();bChairUp=false;bDimmed=false;DimLeft=0;LoopsOn.Reset();Now=FActive();Says.Reset();return;}
 if(bDimmed&&IsValid(M)&&IsValid(M->World)&&!IsEngineExitRequested()){M->World->Trouble.Dim=1;}
 if(!Now.Id.IsEmpty()&&IsValid(M)&&IsValid(M->Audio)&&!IsEngineExitRequested())M->Audio->Stop(TEXT("hapEngine"),.2f);
 if(Uneasy.I>=0&&IsValid(M)&&!IsEngineExitRequested()&&!IsGarbageCollecting())EndUneasy();
 Uneasy=FUneasy();bChairUp=false;
 bDimmed=false;DimLeft=0;
 if(IsValid(M)&&IsValid(M->Audio)&&!IsEngineExitRequested())for(const FString& K:LoopsOn)M->Audio->Stop(K,.3f);
 LoopsOn.Reset();
 if(!Now.Id.IsEmpty()&&IsValid(M)&&!IsEngineExitRequested()&&!IsGarbageCollecting())PutAway(Now.Id);
 Now=FActive();Says.Reset();
 if(!IsEngineExitRequested()&&!IsGarbageCollecting())HideAll();
}

// ---------------------------------------------------------------- what the rhythm reads
bool FS47Happenings::Live(const FString& Id) const
{
 if(!Now.Id.IsEmpty()&&Now.Id==Id)return true;
 const int32 Tonight=M&&M->Freeform?FS47FreeformSession::NightOf(M->Freeform->Clock()):0;
 if(Id==TEXT("clock_ahead"))return AheadSeconds>0&&AheadNight==Tonight;
 if(const TCHAR* Kind=S47Hap::SiteKindOf(Id);*Kind)return SiteOf(Kind)!=nullptr;
 return false;
}
const FString& FS47Happenings::Ground() const{return Now.Id;}
bool FS47Happenings::HumorTonight() const{return M&&M->Freeform&&HumorNight==FS47FreeformSession::NightOf(M->Freeform->Clock());}
void FS47Happenings::FillRhythm(FS47RhythmNow& Out) const
{
 for(const auto& KV:LastDays)Out.LastDays.Add(FName(*KV.Key),KV.Value);
 // Something humorous at a lot holds nobody else back: it waits for the player, harmless, and goes.
 if(!Now.Id.IsEmpty()&&Now.Id!=TEXT("humor")&&Out.Busy.IsNone())Out.Busy=FName(*Now.Id);
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
  if(S.Kind==TEXT("landing")&&S.Az>=0&&S.bKnown)
  {
   FS47MapSector Sec;Sec.Az=S.Az;Sec.Spread=S.Spread;Sec.Label=FString::Printf(TEXT("L%d"),S.Night);View.Sectors.Add(Sec);
   if(S.bFound){FS47MapPin P;P.East=S.X;P.South=S.Z-SaroZ;P.Label=TEXT("ring");View.Pins.Add(P);}
  }
  else if((S.Kind==TEXT("truck")||S.Kind==TEXT("rv")||S.Kind==TEXT("carcass")||S.Kind==TEXT("missing"))&&(S.bHeard||S.bFound)&&!S.bDone)
  {
   FS47MapPin P;P.East=S.X;P.South=S.Z-SaroZ;P.Label=S.Kind==TEXT("truck")?TEXT("CB"):S.Kind==TEXT("rv")?TEXT("camper"):S.Kind==TEXT("carcass")?TEXT("cow"):TEXT("pickup");P.bHeard=!S.bFound;View.Pins.Add(P);
  }
  // The big night's mark: the array's bearing to it once known, and the place in pencil until it is found.
  else if(S.Kind==TEXT("bigmark")&&(S.bKnown||S.bFound))
  {
   if(S.Az>=0){FS47MapSector Sec;Sec.Az=S.Az;Sec.Spread=S.Spread;Sec.Label=FString::Printf(TEXT("N%d"),S.Night);View.Sectors.Add(Sec);}
   FS47MapPin P;P.East=S.X;P.South=S.Z-SaroZ;P.Label=TEXT("mark");P.bHeard=!S.bFound;View.Pins.Add(P);
  }
  else if(S.Kind==TEXT("burst")&&S.Az>=0)
  {
   FS47MapSector Sec;Sec.Az=S.Az;Sec.Spread=S.Spread;Sec.Label=FString::Printf(TEXT("B%d"),S.Night);View.Sectors.Add(Sec);
  }
  // The cattle seen in a ring: where they stood and which way their heads were turned, in the player's pencil. The map can be
  // laid against a landing's bearing; nothing says they agree.
  else if(S.Kind==TEXT("cattle")&&S.bFound)
  {
   FS47MapPin P;P.East=S.X;P.South=S.Z-SaroZ;P.Label=FString::Printf(TEXT("cattle %.0f"),S.Az);View.Pins.Add(P);
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
bool FS47Happenings::OnHighway(double& RoadX,double& RoadZ,double& TruckSpeed) const
{
 if(!Driving())return false;
 const FS47DriveController* D=M->World->CabDrive();if(!D)return false;
 TruckSpeed=D->Speed;
 return RoadOfWorld(D->Pos,RoadX,RoadZ)&&FMath::Abs(RoadX)<9&&RoadZ>-2400&&RoadZ<4200;
}
bool FS47Happenings::CanHearCb() const{return FS47FieldRadio::CbAtHand(M);}
FVector FS47Happenings::Eye() const{return M&&M->Night?M->Night->GlimpseIn(true).Cam.Pos:FVector::ZeroVector;}
bool FS47Happenings::Seen(const FVector& At,double Range,double Deg) const
{
 if(!M||!M->Night)return false;
 return S47Facing(M->Night->GlimpseIn(true).Cam,At,Range,Deg);
}
bool FS47Happenings::LitBy(const FLightOn& L)
{
 if(L.bOwn)return true;
 // Daylight and dusk: the land is lit.
 if(L.Hour>=6.2&&L.Hour<19.6)return true;
 // The lamp's beam, within its reach and its cone (FS47VisitAttention::BeamReach is 25 m).
 if(L.bLamp&&L.LampM<=28&&L.LampDeg<=20)return true;
 // The truck's lights, ahead of it and within their throw.
 if(L.bHeadlights&&L.CarM<=80&&L.CarDeg<=24)return true;
 // Caliche's street lamps.
 if(L.bTown&&L.TownM<=45)return true;
 // Against the sky: what stands with its top above the eye, dark on the stars.
 return L.bSky;
}
bool FS47Happenings::SeenLit(const FVector& At,double Range,double Deg,uint32 How,const TCHAR* Own) const
{
 if(!M||!M->Night||!M->Freeform||!M->World||!Seen(At,Range,Deg))return false;
 const FS47Cam Cam=M->Night->GlimpseIn(true).Cam;
 FLightOn L;L.Hour=S47Hap::HourOf(M->Freeform->Clock());L.bOwn=(How&LitOwn)!=0;
 const FVector To=At-Cam.Pos;const double Dist=To.Size();
 const auto Off=[](const FVector& Dir,const FVector& V){return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Dir.GetSafeNormal(),V.GetSafeNormal()),-1.,1.)));};
 if(!L.bOwn)
 {
  const FS47Flashlight* Lamp=M->Night->Flashlight.Get();
  L.bLamp=Lamp&&Lamp->bOn&&!Lamp->Dead()&&!Driving();L.LampM=Dist;L.LampDeg=Off(Cam.Forward,To);
  if(const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr)
  {
   const FVector Ahead(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading)),Flat(At.X-D->Pos.X,0,At.Z-D->Pos.Z);
   L.bHeadlights=M->World->Trouble.Dim>.5;L.CarM=Flat.Size();L.CarDeg=Off(Ahead,Flat);
  }
  L.bTown=(SituationNow()&S47Hap::NeedTown)!=0;L.TownM=Dist;
  L.bSky=(How&LitSky)!=0&&At.Y>Cam.Pos.Y+.4&&Dist<320;
 }
 if(!LitBy(L))return false;
 // Nothing solid between: the thing itself is not in its own way. From the cab the line starts past the glass and the pillars.
 Sight.IgnoredComponents.Reset();
 if(Own)if(const FProp* P=FindProp(Own))for(const FProp::FPart& Pt:P->Parts)if(Pt.C.IsValid())Sight.IgnoredComponents.Emplace(Pt.C.Get());
 // The line ends a little short of the point, on the near side of the wall a window or a door is set in.
 const FVector From=Driving()?Cam.Pos+Cam.Forward*3.:Cam.Pos;
 const bool bClear=Sight.ClearSight(From,At-(At-From).GetSafeNormal()*FMath::Min(1.2,Dist*.05));
 Sight.IgnoredComponents.Reset();
 return bClear;
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
   UE_LOG(LogS47Happen,Verbose,TEXT("S47: happening %s: place %s (%s), %.0f m, roll %.3f%s"),*Kind,*E->Id,*E->Title,P.DistanceM,P.Roll,P.bFar?TEXT(", far"):TEXT(""));
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
 UE_LOG(LogS47Happen,Verbose,TEXT("S47: happening %s: no catalogue answer, the land's own place %s"),*Kind,D->Id);
 return S;
}
// MILITARY-20261108: the morning after, they got to a landing's ring first and went over the ground (S47Visitors::Cleared).
static bool S47HapRakedSite(const US47Main* M,const FString& Place,int32 Night)
{
 return M&&M->Freeform&&M->Freeform->Visitors&&!Place.IsEmpty()&&M->Freeform->Visitors->Cleared(Place,Night);
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
 else if(Kind==TEXT("hitcher")){if(!Exists(HitcherModel))return nullptr;Spec.Add({HitcherModel,FVector::ZeroVector,FVector(1),0,0,0,0x6a6672u,0,true});UseId=TEXT("ff:hap:hitcher");}
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
  Spec.Add({Man,FVector::ZeroVector,FVector(1),0,0,0,Man==SingerModel?0u:0xb9b2a4u,0,true});UseId=TEXT("ff:hap:humor");
 }
 else if(Kind==TEXT("hairy"))
 {
  const TCHAR* Big=Either(HairyModel,HairyStandIn);if(!Big)return nullptr;
  Spec.Add({Big,FVector::ZeroVector,Big==HairyModel?FVector(1):FVector(1.3),0,0,0,Big==HairyModel?0u:0x3d2b1cu,0,false});
 }
 else if(Kind==TEXT("phone")){Spec=S47Hap::PhoneParts();UseId=TEXT("ff:hap:phone");}
 else if(Kind==TEXT("deadcow")){Spec=S47Hap::DeadCowParts();UseId=TEXT("ff:hap:carcass");}
 else if(Kind==TEXT("litwin")){Spec=S47Hap::WindowParts();Lamps.Add({FVector(0,0,.6),0xffc27a,2500.f,900.f});}
 else if(Kind==TEXT("cinemalight"))Lamps.Add({FVector(0,1.5,0),0xc8d4ff,6000.f,1600.f});
 else if(Kind==TEXT("tvlight"))Lamps.Add({FVector(0,0,0),0x9fb4ff,1500.f,700.f});
 else if(Kind==TEXT("dome"))Lamps.Add({FVector(0,0,0),0xffd9a0,900.f,260.f});
 else if(Kind==TEXT("strip")){Spec.Add({Cube,FVector(0,.008,0),FVector(.88,.016,.03),0,0,0,0xffe2b0,30.f,false});Lamps.Add({FVector(0,.05,.12),0xffd9a8,140.f,130.f});}
 else if(Kind==TEXT("shade")){if(!Exists(S47Hap::HairyStandIn))return nullptr;Spec.Add({S47Hap::HairyStandIn,FVector::ZeroVector,FVector(1),0,0,0,0x0c0c0eu,0,false});}
 else if(Kind==TEXT("rabbits"))Spec=S47Hap::RabbitParts();
 else if(Kind==TEXT("devil"))Spec=S47Hap::DevilParts();
 else if(Kind==TEXT("tracks"))Spec=S47Hap::TrackParts();
 else if(Kind==TEXT("pickup")){if(!Exists(S47Hap::PickupModel))return nullptr;Spec.Add({S47Hap::PickupModel,FVector::ZeroVector,FVector(1),0,0,0,0x2c4a7au,0,true});UseId=TEXT("ff:hap:missing");}
 else if(Kind==TEXT("chair"))Spec=S47Hap::ChairParts();
 // M7, the big night (S47BigNight.cpp): what stands out there for a few seconds, and what it leaves.
 else if(Kind==TEXT("bigglow_column")){Spec=S47Hap::BigColumnParts();Lamps.Add({FVector(0,40,0),0xdfe8ff,80000000.f,250000.f});}
 else if(Kind==TEXT("bigglow_fall")){Spec=S47Hap::GlowParts(24,1,0,0xf4fff2,90.f);Lamps.Add({FVector(0,0,0),0xe8f6ff,40000000.f,200000.f});}
 else if(Kind==TEXT("bigglow_rise")){Spec=S47Hap::BigRiseParts();Lamps.Add({FVector(0,0,0),0xfff0d2,20000000.f,150000.f});}
 else if(Kind==TEXT("bigflash"))Lamps.Add({FVector(0,0,0),0xeaf4ff,120000000.f,300000.f});
 else if(Kind==TEXT("bigmark_column")){Spec=S47Hap::BigMarkColumn();UseId=TEXT("ff:hap:bigmark");}
 else if(Kind==TEXT("bigmark_fall")){Spec=S47Hap::BigMarkFall();UseId=TEXT("ff:hap:bigmark");}
 else if(Kind==TEXT("bigmark_rise")){Spec=S47Hap::BigMarkRise();UseId=TEXT("ff:hap:bigmark");}
 else if(Kind==TEXT("stare"))Spec=S47Hap::StareParts();
 else if(Kind==TEXT("carcass"))Spec=S47Hap::CarcassParts();
 else if(Kind==TEXT("window")){Spec=S47Hap::WindowParts();Lamps.Add({FVector(0,0,.6),0xffc27a,1800.f,700.f});}
 else if(Kind==TEXT("darkcar")){if(!Exists(S47Hap::DarkCarModel))return nullptr;Spec.Add({S47Hap::DarkCarModel,FVector::ZeroVector,FVector(1),0,0,0,0x17171au,0,false});}
 // GLIMPSE-ROAD-20261109: the bodies a glimpse or a figure may have (S47RoadGlimpse::PathOf), each its own prop; the shade's stand-in
 // in its near-black, the rest as they come. Tom's Nordic and Cosmic have no eye's dot (S47EyeGlint::ForStatic); the Cosmic one shines
 // all over, with stars in it (S47TripoNew::AddCosmicGlow, below where the parts are made).
 else if(Kind.StartsWith(TEXT("glimpse_")))
 {
  const TCHAR* Body=S47RoadGlimpse::PathOf(Kind.Mid(8));if(!Body||!Exists(Body))return nullptr;
  Spec.Add({Body,FVector::ZeroVector,FVector(1),0,0,0,Kind==TEXT("glimpse_shadow")?0x0c0c0eu:0u,0,false});
 }
 else if(Kind==TEXT("roadtracks_low")||Kind==TEXT("roadtracks_tall"))Spec=S47Hap::RoadTrackParts(Kind==TEXT("roadtracks_low"));
 else if(Kind==TEXT("geiger"))
 {
  // Local to the instrument: x right, y up, z toward the eye. The needle is last (PoseGeiger turns it about its At, its pivot).
  const uint32 Olive=0x3c4636,Face=0xe6dfc4,Red=0xa8322a,Black=0x101010,Probe=0x2a2a2a;
  // PT-WALKIE-20261109 (Tom): his prototype ("pakke tripo3d-1", S47TripoNew::Geiger) is the case when this build has it, standing up
  // with its face to the eye and the dial on that face; else the drawn case with its probe, as before. -S47TripoNewOff: the drawn one.
  const S47TripoNew::FModel* Own=S47TripoNew::Geiger();
  UStaticMesh* Case=Own&&Exists(Own->Path)&&!FParse::Param(FCommandLine::Get(),TEXT("S47TripoNewOff"))?LoadObject<UStaticMesh>(nullptr,Own->Path,nullptr,LOAD_NoWarn|LOAD_Quiet):nullptr;
  // The middle of the case's face, where the dial goes, and the dial's size against the drawn case's.
  FVector Front(0,0,.035);double Dial=1;
  if(Case)
  {
   // Its bounds in its own centimetres (x out of its back, y across, z up, the face toward -x), at a hand-held height and turned a
   // quarter about up so that the face looks at the eye: the face's upper middle, where the dial goes, at the instrument's middle.
   const FBox B=Case->GetBoundingBox();const FVector C=B.GetCenter(),E=B.GetExtent();
   const double S=S47TripoNew::GeigerHeightM*100./FMath::Max(1.,E.Z*2.),K=S/100.,Up=E.Z*.22;
   Spec.Add({Own->Path,FVector(-K*C.Y,-K*(C.Z+Up),K*C.X),FVector(S),UE_DOUBLE_PI/2,0,0,Olive,0,false});
   Front=FVector(0,0,K*E.X);Dial=FMath::Clamp(K*E.Y*2.*.6/.112,.35,1.);
  }
  else
  {
   Spec.Add({Cube,FVector(0,0,0),FVector(.15,.09,.07),0,0,0,Olive,0,false});
   Spec.Add({Cyl,FVector(-.1,-.02,0),FVector(.026,.13,.026),0,0,UE_DOUBLE_PI/2,Probe,0,false});
  }
  // The dial as the drawn case has it, Dial of its size: the cream face, its red end, five ticks, the pivot, and the needle last.
  const auto On=[&Front,Dial](double X,double Y,double Z){return Front+FVector(X*Dial,Y*Dial,Z);};
  Spec.Add({Cube,On(0,.008,.0012),FVector(.112*Dial,.058*Dial,.003),0,0,0,Face,.6f,false});
  Spec.Add({Cube,On(.036,.03,.0028),FVector(.026*Dial,.007*Dial,.002),0,0,-.7,Red,.4f,false});
  for(int32 I=0;I<5;++I){const double A=FMath::Lerp(1.05,-1.05,I/4.);Spec.Add({Cube,On(-FMath::Sin(A)*.043,-.018+FMath::Cos(A)*.043,.0028),FVector(.002,.007*Dial,.002),0,0,A,Black,0,false});}
  Spec.Add({Cyl,On(0,-.018,.004),FVector(.008*Dial,.004,.008*Dial),0,UE_DOUBLE_PI/2,0,Black,0,false});
  Spec.Add({Cube,On(0,-.018,.0042),FVector(.0025,.042*Dial,.002),0,0,0,Black,0,false});
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
  // Zet's dots (Tom, 9 November): any part whose mesh has its eyes measured gets them, hidden until the part is shown.
  S47EyeGlint::AddStaticIfKnown(W,C,P.Glints);
  // The Cosmic one's light and stars (PT-NORDIC-COSMIC-20261109), children of its body: hidden with it until it is shown.
  if(S47TripoNew::IsCosmic(Mesh->GetName())){S47TripoNew::FCosmicGlow Glow;if(S47TripoNew::AddCosmicGlow(W,C,Glow))P.Cosmic.Add(Glow);}
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
 // Critic round 7: the truck's spots end at 45 m (S47Truck, attenuation 4500), a road glimpse crosses at 60 to 120 m. A body that can
 // be lit (not the shade's near-black) is also on lighting channel 1, and its own spot there, from the truck's lamps, gives it what
 // their beam would give at that distance (FarBeam): the figure shows pale in the far throw, the road and land around it stay dark.
 if(Kind.StartsWith(TEXT("glimpse_"))&&Kind!=TEXT("glimpse_shadow")&&!P.Parts.IsEmpty())
 {
  for(FProp::FPart& Pt:P.Parts)if(UStaticMeshComponent* C=Pt.C.Get())C->SetLightingChannels(true,true,false);
  auto* Sp=NewObject<USpotLightComponent>(W,MakeUniqueObjectName(W,USpotLightComponent::StaticClass(),FName(*FString::Printf(TEXT("S47HapFarBeam_%s_%s"),*Kind,*Here))));
  Sp->SetupAttachment(W->GetRootComponent());Sp->SetMobility(EComponentMobility::Movable);Sp->SetIntensityUnits(ELightUnits::Candelas);Sp->SetIntensity(0);
  Sp->SetLightingChannels(false,true,false);Sp->SetCastShadows(false);Sp->SetIndirectLightingIntensity(0);Sp->SetVolumetricScatteringIntensity(0);
  Sp->SetAffectTranslucentLighting(false);Sp->SetLightColor(FLinearColor(1.f,.9f,.72f));Sp->SetVisibility(false);Sp->RegisterComponent();
  P.FarBeam=Sp;
 }
 if(P.Parts.IsEmpty()&&P.Lamps.IsEmpty()){UE_LOG(LogS47Happen,Warning,TEXT("S47: happening prop %s: nothing could be loaded"),*Kind);return nullptr;}
 if(!UseId.IsEmpty()&&!P.Hits.IsEmpty())
 {
  const TWeakPtr<uint8> Guard=Alive;
  FS47Interactable I;I.Id=UseId;I.Object=P.Hits[0];I.Range=UseId==TEXT("ff:hap:truck")||UseId==TEXT("ff:hap:rv")||UseId==TEXT("ff:hap:missing")?4.5:UseId==TEXT("ff:hap:carcass")?4.:UseId==TEXT("ff:hap:bigmark")?6.:3.2;
  I.Label=[this,Guard,UseId]()->FString
  {
   if(!Guard.IsValid()||!M||!M->Freeform)return FString();
   if(UseId==TEXT("ff:hap:ring"))return SiteOf(TEXT("landing"))?TEXT("Scorched ground / look closer"):FString();
   if(UseId==TEXT("ff:hap:rv"))return SiteOf(TEXT("rv"))?TEXT("Camper / knock"):FString();
   if(UseId==TEXT("ff:hap:cattle"))return SiteOf(TEXT("cattle"))?TEXT("The cattle / look closer"):FString();
   if(UseId==TEXT("ff:hap:hitcher"))return Now.Id==TEXT("hitchhiker")&&Now.Stage<=1?TEXT("The woman / ask where she's going"):FString();
   if(UseId==TEXT("ff:hap:humor"))return Now.Id!=TEXT("humor")?FString():Now.Variant==TEXT("car")?TEXT("Pink convertible / look"):Now.Variant==TEXT("singer")?TEXT("The singer / listen"):Now.Variant==TEXT("pump")?TEXT("The small man / talk"):FString();
   if(UseId==TEXT("ff:hap:phone"))return Now.Id==TEXT("phone_empty")&&Now.Stage==0?TEXT("The phone / answer it"):FString();
   if(UseId==TEXT("ff:hap:carcass"))return SiteOf(TEXT("carcass"))?TEXT("Dead cow / look closer"):FString();
   if(UseId==TEXT("ff:hap:bigmark"))return SiteOf(TEXT("bigmark"))?TEXT("Marked ground / look closer"):FString();
   if(UseId==TEXT("ff:hap:missing")){const FSite* S=SiteOf(TEXT("missing"));return S&&!S->bDone?TEXT("Blue pickup / look inside"):FString();}
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
   else if(UseId==TEXT("ff:hap:phone"))UsePhone();
   else if(UseId==TEXT("ff:hap:carcass"))UseSite(TEXT("carcass"));
   else if(UseId==TEXT("ff:hap:missing"))UseSite(TEXT("missing"));
   else if(UseId==TEXT("ff:hap:bigmark"))UseBig();
  };
  I.Examine=I.Label;
  M->Inter->Add(I);
 }
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening prop %s built in %s: %d parts, %d lamps%s"),*Kind,*Here,P.Parts.Num(),P.Lamps.Num(),Parent<0&&!UseId.IsEmpty()?TEXT(", nothing to use in this area"):TEXT(""));
 Props.Add(MoveTemp(Made));
 return Props.Last().Get();
}
void FS47Happenings::PoseProp(FProp& P,const FVector& Foot,double Yaw,const FQuat& Tilt)
{
 if(P.bPosed&&P.Foot.Equals(Foot,.01)&&FMath::IsNearlyEqual(P.Yaw,Yaw,.001)&&Tilt.Equals(FQuat::Identity,1e-4))return;
 P.Foot=Foot;P.Yaw=Yaw;P.bPosed=true;
 const FQuat Turn=Tilt*S47Three::Euler(0,Yaw,0);
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
  // With its children: the eye's dots on it (P.Glints) show and hide with the body, and S47EyeGlint::Step reads that.
  C->SetVisibility(bHere,true);
  C->SetCollisionEnabled(bHere&&Pt.bHit?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
 }
 for(FProp::FLamp& L:P.Lamps)if(UPointLightComponent* Lc=L.L.Get()){Lc->SetVisibility(bHere);Lc->SetIntensity(bHere?L.Lumens:0.f);}
 // The far beam is the road glimpse's to turn on (StepMore road_glimpse); it never outlasts the body.
 if(!bHere)if(USpotLightComponent* Sp=P.FarBeam.Get()){Sp->SetIntensity(0);Sp->SetVisibility(false);}
}
// The big night's drawn things: posed with each part's place and size times Scale (a column that grows up off the ground), the
// lamps at LampK of their strength. Not PoseProp's pose: its next call poses it again from scratch.
bool FS47Happenings::ShowBig(const FString& Kind,const FVector& Foot,double Yaw,const FVector& Scale,float LampK,bool bOn)
{
 FProp* P=bOn?Prop(Kind):FindProp(Kind);
 if(!P)return false;
 if(!bOn){ShowProp(*P,false);return true;}
 const FQuat Turn=S47Three::Euler(0,Yaw,0);
 for(FProp::FPart& Pt:P->Parts)
 {
  UStaticMeshComponent* C=Pt.C.Get();if(!IsValid(C))continue;
  const FQuat Q=Turn*S47Three::Euler(Pt.Pitch,Pt.Yaw,Pt.Roll,TEXT("YXZ"));
  double E[16];S47Three::Compose(Foot+Turn.RotateVector(Pt.At*Scale),Q,Pt.Size*Scale,E);
  C->SetWorldTransform(S47Three::MatToUE(E));
 }
 for(FProp::FLamp& L:P->Lamps)if(UPointLightComponent* Lc=L.L.Get())Lc->SetWorldLocation(S47Three::ToUE(Foot+Turn.RotateVector(L.At*Scale)));
 P->bPosed=false;
 ShowProp(*P,true);
 for(FProp::FLamp& L:P->Lamps)if(UPointLightComponent* Lc=L.L.Get())Lc->SetIntensity(L.Lumens*FMath::Clamp(LampK,0.f,1.f));
 return true;
}
void FS47Happenings::HideAll()
{
 if(!M||!IsValid(M->Web.Get()))return;
 for(const auto& P:Props)if(P){P->bShown=true;ShowProp(*P,false);}
}

// ---------------------------------------------------------------- the night
// From eight in the morning, when the night is drawn (NightOf turns there), to eight the next: before the evening the hour is
// under nineteen, after midnight over twenty-four. Until 8 November this counted from noon, and every draw redrawn in the morning
// was given an hour of 32 to 36 that never came (critics: all 22 morning swaps in m1cfield, none of them started).
double FS47Happenings::NightHour(double Clock){const double H=S47Hap::HourOf(Clock);return H<8?H+24:H;}
double FS47Happenings::HourOfNight(double Clock) const{return NightHour(Clock);}
double FS47Happenings::NightPace() const{return M&&M->Freeform&&M->Freeform->Director?FMath::Max(1.,M->Freeform->Director->TestPaceNow()):1.;}
void FS47Happenings::PlanTonight(int32 Night)
{
 CloseTally(Night);
 Plan.Reset();PlanNight=Night;bLate=false;HumorKind.Empty();DangerAt=-1;RefusedSaid.Reset();LateDrawSaid.Reset();
 auto* F=M?M->Freeform.Get():nullptr;if(!F)return;
 const int32 Seed=F->Seed();
 // This week's row of the curve: how much each level is worth, how near things come, how likely the night's danger is.
 const FS47RhythmWeek Week=M->Rhythm?M->Rhythm->Data().Week(Night):FS47RhythmWeek();
 NearK=FMath::Clamp(Week.Near,.2,1.5);
 // Tonight's hour for the reserve between 23 and 01 (StepLate), and whether it is a hush instead.
 LateAt=23.5+1.33*FS47NightRhythm::Roll(Seed,Night,0x4c415445u);bLateHush=FS47NightRhythm::Roll(Seed,Night,0x48555348u)<.25f;
 // What unease left in the room or on the film is not carried into another night.
 if(bChairUp){if(FProp* P=FindProp(TEXT("chair")))ShowProp(*P,false);bChairUp=false;}
 if(GhostNight>0&&GhostNight<Night){if(M->Night&&M->Night->Album)M->Night->Album->bForceGhost=false;GhostNight=0;}
 // The night's chance from director.json (per_night, times this week's worth of its level) and its hours (draw.at), so that they can
 // be set without a build; these numbers when it has none. The hours climb through the night: small things in the evening,
 // what can be seen from eleven, what comes near after midnight, toward their hours.
 const auto Draw=[&](const TCHAR* Id,uint32 Salt,int32 FirstNight,float Usual,double From,double Span,float AtLeast=0.f)
 {
  if(Night<FirstNight)return;
  const float Chance=FMath::Max(AtLeast,M->Rhythm?M->Rhythm->PerNight(FName(Id),Usual,Night):Usual);
  double A=From,B=From+Span;if(M->Rhythm)M->Rhythm->DrawHours(FName(Id),A,B);
  if(Chance>0&&FS47NightRhythm::Roll(Seed,Night,Salt)<Chance)Plan.Add(Id,A+(B-A)*FS47NightRhythm::Roll(Seed,Night,Salt+1));
 };
 // A first landing is all but sure by the fourth night, so that the counter has something to find.
 // Until one has been noticed: a landing nobody saw does not count.
 const bool bNoLanding=!Sites.ContainsByPredicate([](const FSite& S){return S.Kind==TEXT("landing")&&(S.bKnown||S.bFound);});
 // Numbers for feel, from TODO NIGHT31-20261108 (Tom, 8 November): something most nights, the dangerous ones rare.
 Draw(TEXT("car_dies"),0x43415244u,1,.45f,21.,5.5);
 Draw(TEXT("cattle_ring"),0x4341544cu,2,.32f,22.,5.);
 // Humour at the end of the night, from five, when nothing more is coming anyway: a relief, never a sign (krit, 8 November).
 Draw(TEXT("humor"),0x48554d52u,7,.5f,29.,.8);
 Draw(TEXT("dogs_hush"),0x444f4753u,1,.3f,20.5,5.);
 Draw(TEXT("phone_empty"),0x50484f4eu,2,.2f,21.,4.);
 Draw(TEXT("animals_flee"),0x414e494du,1,.25f,21.,5.);
 Draw(TEXT("am_caller"),0x414d4341u,2,.25f,20.,6.);
 Draw(TEXT("town_blackout"),0x424c4b4fu,2,.12f,22.,2.5);
 Draw(TEXT("dust_devil"),0x44555354u,2,.15f,20.,3.5);
 Draw(TEXT("hood_warm"),0x484f4f44u,2,.3f,22.,6.);
 // A calm night has no lights, no bodies and no calls that lead anywhere (FS47NightRhythm::Quiet).
 const bool bCalm=F->QuietNight(Night);
 if(!bCalm)
 {
  Draw(TEXT("carcass"),0x43415243u,3,.12f,20.,6.);
  Draw(TEXT("house_lit"),0x484f5553u,2,.25f,21.5,5.5);
  Draw(TEXT("horizon_figure"),0x484f525au,2,.2f,24.5,4.);
  // GLIMPSE-ROAD-20261109: something across the road far ahead, in the glimpse tier's hours, at most once a week (director.json).
  Draw(TEXT("road_glimpse"),0x52474c4du,3,.3f,24.,1.75);
  Draw(TEXT("cinema_film"),0x43494e45u,2,.15f,22.,3.);
  Draw(TEXT("room6_tv"),0x524f4f4du,3,.1f,23.,4.);
  Draw(TEXT("array_burst"),0x42555253u,2,.25f,21.5,6.5);
  Draw(TEXT("truck_tracks"),0x5452434bu,3,.15f,23.,5.);
  Draw(TEXT("missing_person"),0x4d495353u,4,.1f,20.,4.);
  Draw(TEXT("cb_stranded"),0x43425354u,2,.28f,20.,4.5);
  Draw(TEXT("landing"),0x4c414e44u,2,.24f,22.5,5.,bNoLanding&&Night>=4?.6f:0.f);
  Draw(TEXT("stranded_rv"),0x52564e54u,3,.14f,19.5,3.);
 }
 // The night's one danger of these (Tom's critics, 8 November: none of M1's was ever met, and none on foot): a slot at an hour in
 // their hours, kept until five, and taken by the first of them that fits where the player is then. How likely the slot is grows
 // week by week (curve "danger"); which shapes it may take tonight is each one's own per_night. M7: the same slot is the indoor
 // systems' too (FS47Rhythm::DangerSlotOpen): a player who keeps inside meets it at a door, at the window, or in the yard.
 DangerAt=FS47Rhythm::DangerSlotHour(Seed,Night,Week.Danger,F->BodyStartHour(Night),bCalm);
 if(DangerAt>0)
 {
  const bool bEcho=bEchoArmed&&Night>EchoFrom&&Night<=EchoFrom+3;
  for(const TCHAR* Id:{TEXT("figure_near"),TEXT("hitchhiker"),TEXT("car_light"),TEXT("cb_echo")})
  {
   if(FString(Id)==TEXT("cb_echo")&&!bEcho)continue;
   // The shape's own number, not the curve's: the slot's chance has the week's danger in it already.
   const float Shape=M->Rhythm?M->Rhythm->PerNight(FName(Id),.6f,-1):.6f;
   if(FS47NightRhythm::Roll(Seed,Night,S47Hap::SaltOf(Id)^0x53484150u)<Shape)Plan.Add(Id,DangerAt);
  }
  // The night leads up to it, closer each time: the dogs go quiet, then the animals run past (a chain on one night, as the critics
  // asked). Where the player cannot hear them they give their place to what fits (StepSwap).
  if(!Plan.Contains(TEXT("dogs_hush")))Plan.Add(TEXT("dogs_hush"),DangerAt-1.1);
  if(!Plan.Contains(TEXT("animals_flee"))&&Night>=4)Plan.Add(TEXT("animals_flee"),DangerAt-.55);
 }
 TArray<FString> Parts;for(const auto& KV:Plan)Parts.Add(FString::Printf(TEXT("%s %s"),*KV.Key,*S47Hap::ClockText(FMath::Fmod(KV.Value,24.)*3600.)));
 UE_LOG(LogS47Happen,Display,TEXT("S47: happenings night %d drawn: %s; reserve %s%s; week from day %d: danger %.2f, near %.2f%s"),Night,Parts.IsEmpty()?TEXT("none"):*FString::Join(Parts,TEXT(", ")),*S47Hap::ClockText(FMath::Fmod(LateAt,24.)*3600.),bLateHush?TEXT(" (a hush)"):TEXT(""),Week.FromDay,Week.Danger,NearK,DangerAt>0?*FString::Printf(TEXT(", the danger's slot %s"),*S47Hap::ClockText(FMath::Fmod(DangerAt,24.)*3600.)):TEXT(""));
 PlanUnease(Night);
 PlanCues(Night);
}
// Before each danger drawn tonight, one or two small things twenty to forty minutes ahead of it, the second closer; before what can
// be seen or comes near, now and then one far off. On some other nights the same with nothing after, so that they promise nothing.
void FS47Happenings::PlanCues(int32 Night)
{
 Cues.Reset();
 auto* F=M->Freeform.Get();const int32 Seed=F->Seed();
 bool bAny=false;
 if(DangerAt>0)
 {
  const double Lead=(20.+20.*FS47NightRhythm::Roll(Seed,Night,0x43554531u))/60.;
  FCue Far;Far.Hour=DangerAt-Lead;Far.Kind=0;Cues.Add(Far);
  if(FS47NightRhythm::Roll(Seed,Night,0x43554532u)<.6f){FCue Close;Close.Hour=DangerAt-Lead*.4;Close.Kind=1;Cues.Add(Close);}
  bAny=true;
 }
 for(const auto& KV:Plan)
 {
  const FS47RhythmEvent* E=M->Rhythm?M->Rhythm->Data().Find(FName(*KV.Key)):nullptr;
  if(!E||IsDanger(KV.Key)||(E->Level!=ES47RhythmLevel::Observation&&E->Level!=ES47RhythmLevel::Presence))continue;
  if(FS47NightRhythm::Roll(Seed,Night,S47Hap::SaltOf(KV.Key)^0x43554536u)>=.45f)continue;
  FCue Far;Far.Hour=KV.Value-(10.+15.*FS47NightRhythm::Roll(Seed,Night,S47Hap::SaltOf(KV.Key)^0x43554537u))/60.;Far.Kind=0;Cues.Add(Far);
 }
 if(!bAny&&FS47NightRhythm::Roll(Seed,Night,0x43554533u)<.3f)
 {
  const double At=24.+4.*FS47NightRhythm::Roll(Seed,Night,0x43554534u);
  FCue Far;Far.Hour=At;Cues.Add(Far);
  if(FS47NightRhythm::Roll(Seed,Night,0x43554535u)<.5f){FCue Close;Close.Hour=At+.25;Close.Kind=1;Cues.Add(Close);}
 }
 Cues.Sort([](const FCue& A,const FCue& B){return A.Hour<B.Hour;});
}
namespace S47HapOmen
{
 // M2: a forewarning goes through the one owner of the night's signs (FS47Omen::Post), so that the crickets, the animals and the gear
 // answer it as they answer everything else, and each may miss it. Out at Dist metres (in a random way, or at the place given): near
 // enough for the night to go quiet (FS47Soundscape::HushReach), too far for the lamp or the player's strain to feel it yet.
 void Forewarn(US47Main* M,const FString& Id,const FVector& Eye,const FVector* At,double Dist,double Severity,double Seconds)
 {
  if(!M||!M->Omen)return;
  FS47OmenSource S;S.Id=Id;S.Severity=Severity;S.bComing=true;
  if(At)S.At=*At;
  else{const double A=FMath::FRand()*2*UE_DOUBLE_PI;S.At=Eye+FVector(FMath::Cos(A)*Dist,0,FMath::Sin(A)*Dist);}
  S.Reach=FMath::Max(1.,FVector2D::Distance(FVector2D(Eye.X,Eye.Z),FVector2D(S.At.X,S.At.Z))*.8);
  M->Omen->Post(S,Seconds);
 }
}
void FS47Happenings::StepCues()
{
 US47Audio* A=M->Audio.Get();
 // What a cue dipped comes back.
 if(ToneBack>=0&&M->T>=ToneBack)
 {
  ToneBack=-1;
  if(A&&Uneasy.I!=9&&Uneasy.I!=16)A->HoldRoomTone(1.f,.8f);
  if(M->Night&&M->Night->Flashlight&&Uneasy.I!=15)M->Night->Flashlight->Flicker(0,TEXT("cue"));
 }
 const double H=HourOfNight(M->Freeform->Clock());
 for(FCue& C:Cues)
 {
  if(C.bDone||H<C.Hour)continue;
  C.bDone=true;
  // Slept through, or something else had the night: gone, not made up for.
  if(H>C.Hour+.5||!Now.Id.IsEmpty())continue;
  if(C.Kind==0)
  {
   // Far off first: the crickets go quiet a while.
   S47HapOmen::Forewarn(M,TEXT("happen:cue_far"),Eye(),nullptr,130,.45,45);
  }
  else if(Driving())M->World->Trouble.Start(ES47Trouble::Flicker,[]{FS47TroubleOpts O;O.Len=.6;return O;}());
  else if(!Outdoors()){if(A){FS47Sound O;O.Gain=.12f;O.Rate=.8f;A->Sfx(TEXT("switch"),O);A->HoldRoomTone(.2f,.15f);}ToneBack=M->T+1.4;}
  else if(M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn){M->Night->Flashlight->Flicker(.7,TEXT("cue"));ToneBack=M->T+1.1;}
  else S47HapOmen::Forewarn(M,TEXT("happen:cue_near"),Eye(),nullptr,70,.45,20);
  UE_LOG(LogS47Happen,Display,TEXT("S47: happenings cue %s at %s"),C.Kind==0?TEXT("far"):TEXT("near"),*S47Hap::ClockText(M->Freeform->Clock()));
 }
}
bool FS47Happenings::Due(const FString& Id) const
{
 const double* From=Plan.Find(Id);if(!From||!M||!M->Freeform)return false;
 // An hour the night never reaches is a fault, said once, not a draw that quietly never comes.
 if(*From>=30){if(!LateDrawSaid.Contains(Id)){LateDrawSaid.Add(Id);UE_LOG(LogS47Happen,Warning,TEXT("S47: happening %s drawn for an hour the night never reaches (%.2f): it cannot start"),*Id,*From);}return false;}
 const double H=HourOfNight(M->Freeform->Clock());
 const int32* Last=LastDays.Find(Id);
 // The night's danger waits in its slot until five for something that fits; the rest have an hour and a half.
 return S47Hap::DueAt(H,*From,IsDanger(Id)?DangerUntil:*From+DrawWindow)&&!(Last&&*Last==M->Freeform->Day());
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
void FS47Happenings::Deed(const FString& Text,bool bEcho)
{
 // Humour and F10 are never said back (cb_echo): only what the player really did.
 if(!bEcho||Now.bForced)return;
 Deeds.Remove(Text);Deeds.Add(Text);
 while(Deeds.Num()>5)Deeds.RemoveAt(0);
}
void FS47Happenings::Cb(const TArray<FString>& Lines,const FString& Voice,bool bWrong)
{
 if(!M||Lines.IsEmpty())return;
 static_cast<void>(bWrong);
 // CB-TRUCK-20261109: the CB is under the truck's dash and nowhere else. Away from it, a call waits a while for the player to sit in
 // the truck (StepTips), as a stranger's tip does; it is never put on the screen as a CB line out of the truck.
 if(!CanHearCb())
 {
  FHeldTip Held;Held.Lines=Lines;Held.Voice=Voice;Held.Until=Paced+240;
  if(HeldTips.Num()>=3)HeldTips.RemoveAt(0);
  HeldTips.Add(Held);
  return;
 }
 // The set's own sound and its HUD line, from the dash.
 if(FS47FieldRadio::CbSay(M,Lines,Voice))return;
 // MSG-ONE-20261109: Dale's voice on the walkie: the call waits for him (StepTips), as one away from the CB does, not as text over him.
 if(FS47FieldRadio::DaleOnAir(M)){FHeldTip Held;Held.Lines=Lines;Held.Voice=Voice;Held.Until=Paced+240;if(HeldTips.Num()>=3)HeldTips.RemoveAt(0);HeldTips.Add(Held);return;}
 // Busy, or no sound to play it with: the words, one after another.
 double In=0;for(const FString& L:Lines){Say(TEXT("CB: ")+L,In,float(S47CB::SecsOf(L))+1.5f);In+=S47CB::SecsOf(L)+.6;}
}
void FS47Happenings::Tip(const TArray<FString>& Lines,const FString& Voice,const FString& Passed)
{
 // CB-WALKIE: on the CB a stranger says it himself, in his own words; away from any CB Dale passes the gist on over the walkie.
 // With neither, it waits a while here for one of them: the gist is never read out over the CB in a driver's voice (the text critic).
 if(CanHearCb()){Cb(Lines,Voice,false);return;}
 if(M&&M->FieldRadio.IsValid()&&M->FieldRadio->Owned()&&!Passed.IsEmpty()){FS47FieldRadio::Report(M,Passed);return;}
 FHeldTip H;H.Lines=Lines;H.Voice=Voice;H.Passed=Passed;H.Until=Paced+240;
 if(HeldTips.Num()>=3)HeldTips.RemoveAt(0);
 HeldTips.Add(H);
}
void FS47Happenings::StepTips()
{
 if(HeldTips.IsEmpty()||!M||M->ModalOpen())return;
 if(HeldTips[0].Until<Paced){HeldTips.RemoveAt(0);return;}
 if(CanHearCb()&&!(M->CB.IsValid()&&M->CB->bBusy)&&!FS47FieldRadio::DaleOnAir(M)){const FHeldTip H=HeldTips[0];HeldTips.RemoveAt(0);Cb(H.Lines,H.Voice,false);}
 else if(M->FieldRadio.IsValid()&&M->FieldRadio->Owned()&&!HeldTips[0].Passed.IsEmpty()){const FHeldTip H=HeldTips[0];HeldTips.RemoveAt(0);FS47FieldRadio::Report(M,H.Passed);}
}
FString FS47Happenings::Spoken(const FString& Title)
{
 // A sign's capitals are not what a driver calls a place (STATION 01 in the catalogue).
 if(Title.Contains(TEXT("STATION")))return TEXT("the old survey station up the track");
 bool bCaps=Title.Len()>3;for(const TCHAR C:Title)if(FChar::IsLower(C)){bCaps=false;break;}
 if(bCaps)return Title.ToLower();
 // Nobody on nineteen says the whole of "the salt lick and trough": the first of it does (the text critic, 8 November).
 FString Short=Title;int32 And=Short.Find(TEXT(" and "));
 if(And>4)Short=Short.Left(And);
 return Short;
}
void FS47Happenings::Noticed(const FString& Id,double Dist,const TCHAR* How,int32 Ser)
{
 if(!M||!M->Freeform)return;
 const bool bNear=Dist<0||Dist<S47Hap::NearByM;
 const double H=HourOfNight(M->Freeform->Clock());
 // Between eleven and one only what came near counts: a light far off or a word on the radio does not (StepLate).
 if(H>=23&&H<25&&bNear)bLate=true;
 const int32 Key=Ser>0?Ser:Now.Serial;
 if(Key<=0||NoticedSerials.Contains(Key))return;
 NoticedSerials.Add(Key);
 const bool bForced=Ser<=0&&Now.bForced;
 if(!bForced){++Tally.NNoticed;if(bNear)++Tally.NNear;if(Dist<1e5)Tally.Nearest=FMath::Min(Tally.Nearest,FMath::Max(0.,Dist));}
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening %s noticed day %d %s %s by %s%s"),Id.IsEmpty()?*Now.Id:*Id,M->Freeform->Day(),*S47Hap::ClockText(M->Freeform->Clock()),Dist<0?TEXT("at the player"):Dist>=1e5?TEXT("far off"):*FString::Printf(TEXT("at %.0f m"),Dist),How?How:TEXT("here"),bForced?TEXT(", F10"):TEXT(""));
}
void FS47Happenings::CloseTally(int32 NewNight)
{
 if(Tally.Night>0&&Tally.Night!=NewNight)
 {
  UE_LOG(LogS47Happen,Display,TEXT("S47: happenings night %d summary: %d started, %d noticed, %d of them near (under %.0f m), %d dangers met, nearest %s, %d refusals"),Tally.Night,Tally.NStarted,Tally.NNoticed,Tally.NNear,S47Hap::NearByM,Tally.NMet,Tally.Nearest<1e8?*FString::Printf(TEXT("%.0f m"),Tally.Nearest):TEXT("none"),Tally.NRefused);
 }
 if(Tally.Night!=NewNight){Tally=FTally();Tally.Night=NewNight;NoticedSerials.Reset();}
}
void FS47Happenings::Refused(const FString& Id,const FString& Why)
{
 // Each reason a draw is turned away for, once a night, so that a draw never dies without the log saying why.
 const FString Key=Id+TEXT("|")+Why;
 if(RefusedSaid.Contains(Key))return;
 RefusedSaid.Add(Key,Why);++Tally.NRefused;
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening %s refused day %d %s: %s"),*Id,M&&M->Freeform?M->Freeform->Day():0,M&&M->Freeform?*S47Hap::ClockText(M->Freeform->Clock()):TEXT("--:--"),*Why);
}
FVector FS47Happenings::PlayerAt() const
{
 if(!M||!M->Player)return FVector::ZeroVector;
 if(const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr)return D->Pos;
 return M->Player->Pos;
}
double FS47Happenings::AwayFrom(const FVector& At) const{const FVector P=PlayerAt();return FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(P.X,P.Z));}
double FS47Happenings::AwayNow(const FString& Id) const
{
 FVector At;
 if(Now.Id==Id&&LiveAt(At))return AwayFrom(At);
 if(const TCHAR* Kind=S47Hap::SiteKindOf(Id);*Kind)
 {
  const FSite* S=SiteOf(Kind);if(!S)return -1;
  if(WorldOf(S->X,S->Z,At))return AwayFrom(At);
  // On another ground: road-local, as far as the land goes.
  double Px=0,Pz=0;if(PlayerRoad(Px,Pz))return FVector2D::Distance(FVector2D(S->X,S->Z),FVector2D(Px,Pz));
 }
 return -1;
}
bool FS47Happenings::TakePointOut(FS47FreeformDirector* Director,int32 Night)
{
 if(!Director)return true;
 if(!Director->MayPointOut(Night))return false;
 Director->NotePointOut(Night);return true;
}
bool FS47Happenings::Stirred(const TArray<FStir>& Near,double X,double Z,int32 Day)
{
 for(const FStir& S:Near)if(S.Night<=Day&&S.Night>=Day-StirNights&&FVector2D::Distance(FVector2D(S.X,S.Z),FVector2D(X,Z))<=StirReach)return true;
 return false;
}
bool FS47Happenings::StirredNear(double X,double Z) const
{
 if(!M||!M->Freeform)return false;
 TArray<FStir> Near=Past;
 for(const FSite& S:Sites)Near.Add({S.X,S.Z,S.Night});
 return Stirred(Near,X,Z,M->Freeform->Day());
}
bool FS47Happenings::IsNightFinding(const FString& Id)
{
 for(const TCHAR* K:S47Hap::NightKeys)if(Id.StartsWith(K))return true;
 return false;
}
FString FS47Happenings::NightFindingKind(const FString& Id)
{
 for(const TCHAR* K:S47Hap::NightKeys)if(Id.StartsWith(K)){FString Kind=K;Kind.RemoveFromEnd(TEXT("_d"));Kind.RemoveFromEnd(TEXT("_"));return Kind;}
 return FString();
}
FString FS47Happenings::RestBlock() const
{
 if(Now.Id==TEXT("big_night"))return TEXT("Not now. Not with every light in the place doing that.");
 if(!Now.bDanger||!Now.bMet)return FString();
 if(Now.Id==TEXT("car_light"))return TEXT("Not with that light still over the truck.");
 if(Now.Id==TEXT("hitchhiker"))return TEXT("Not with her out there.");
 if(Now.Id==TEXT("figure_near"))return TEXT("Not with that out there in the dark.");
 return TEXT("Not with that voice on the set.");
}
bool FS47Happenings::HoldsTaking() const{return !Now.Id.IsEmpty()&&Now.bDanger&&Now.Watch>Now.Age;}
void FS47Happenings::MeetDanger()
{
 if(Now.bMet||!Now.bDanger||Now.Id.IsEmpty())return;
 Now.bMet=true;
 if(!Now.bForced){++MetTotal;++Tally.NMet;}
 if(M&&M->Rhythm)M->Rhythm->Began(FName(*Now.Id));
 const double Away=AwayNow(Now.Id);
 Noticed(Now.Id,Away,TEXT("meeting it"));
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening %s met after %.0f s, %s: the night's danger"),*Now.Id,Now.Age,Away<0?TEXT("at the player"):*FString::Printf(TEXT("%.0f m from the player"),Away));
}
// Out on foot while it watches: the light comes down over the player, and that is this meeting (S47Taken::RulesAllow, SameMeeting).
void FS47Happenings::StepWatch()
{
 if(Now.Id.IsEmpty()||Now.Watch<=Now.Age||Now.Stage>=7||!M->Freeform)return;
 auto* F=M->Freeform.Get();
 if(Driving()||!Outdoors()||!F->Taken||F->Taken->Busy()||!M->Player)return;
 // The light over the figure comes down where it stood: run far enough from there and it does not reach (StepFigure ends it).
 if(Now.Id==TEXT("figure_near")&&AwayFrom(Now.At)>75)return;
 const FVector Above(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 MeetDanger();
 if(F->Taken->BeginOver(ES47Taker::Roswell,Now.Title.IsEmpty()?FString(TEXT("the road")):Now.Title,Above))
 {
  Now.Stage=8;
  UE_LOG(LogS47Happen,Display,TEXT("S47: happening %s: out on foot under it, the light comes down"),*Now.Id);
 }
 else
 {
  // The rules would not have it (or it could not be): the light goes up and away instead.
  Now.Stage=7;Now.Watch=-1;
  UE_LOG(LogS47Happen,Display,TEXT("S47: happening %s: out on foot under it, and it goes up"),*Now.Id);
 }
}
void FS47Happenings::PutAway(const FString& Id)
{
 static const TCHAR* const What[][2]={{TEXT("car_dies"),TEXT("passer")},{TEXT("landing"),TEXT("lander")},{TEXT("car_light"),TEXT("lander")},{TEXT("cb_echo"),TEXT("lander")},{TEXT("hitchhiker"),TEXT("hitcher")},
  {TEXT("humor"),TEXT("car")},{TEXT("humor"),TEXT("singer")},{TEXT("humor"),TEXT("hairy")},{TEXT("humor"),TEXT("zet")},{TEXT("phone_empty"),TEXT("phone")},{TEXT("horizon_figure"),TEXT("shade")},
  {TEXT("room6_tv"),TEXT("tvlight")},{TEXT("animals_flee"),TEXT("rabbits")},{TEXT("dust_devil"),TEXT("devil")},{TEXT("figure_near"),TEXT("shade")},{TEXT("figure_near"),TEXT("lander")},
  {TEXT("big_night"),TEXT("bigglow_column")},{TEXT("big_night"),TEXT("bigglow_fall")},{TEXT("big_night"),TEXT("bigglow_rise")},{TEXT("big_night"),TEXT("bigflash")}};
 if(M&&IsValid(M->Web.Get()))for(const auto& W:What)if(Id==W[0])for(const auto& P:Props)if(P&&P->Kind==W[1])ShowProp(*P,false);
 // The body a figure or a glimpse was given (glimpse_*), whichever it was.
 if((Id==TEXT("horizon_figure")||Id==TEXT("figure_near")||Id==TEXT("road_glimpse"))&&M&&IsValid(M->Web.Get()))for(const auto& P:Props)if(P&&P->Kind.StartsWith(TEXT("glimpse_")))ShowProp(*P,false);
 if(Id==TEXT("room6_tv")&&M&&IsValid(M->Audio))M->Audio->TvHiss({});
}

bool FS47Happenings::StartEvent(const FString& Id,const FString& Variant,bool bForced,FString& Why)
{
 auto* F=M?M->Freeform.Get():nullptr;
 if(!F||!F->Active()||!M->IsFreeform()){Why=TEXT("Freeform only");return false;}
 if(!bForced&&F->InTutorial()){Why=TEXT("the first shift");return false;}
 const bool bGround=!S47Hap::Background(Id);
 if(bGround&&!Now.Id.IsEmpty()){Why=FString::Printf(TEXT("%s is on"),*Now.Id);return false;}
 bool bOk=false;const int32 SitesBefore=Sites.Num();
 if(Id==TEXT("car_dies"))bOk=BeginCar(bForced,Why);
 else if(Id==TEXT("clock_ahead"))bOk=BeginAhead(bForced,Why);
 else if(Id==TEXT("cattle_ring"))bOk=BeginCattle(bForced,Why);
 else if(Id==TEXT("cb_stranded"))bOk=BeginStranded(Variant,bForced,Why);
 else if(Id==TEXT("cb_echo"))bOk=BeginEcho(bForced,Why);
 else if(Id==TEXT("landing"))bOk=BeginLanding(bForced,Why);
 else if(Id==TEXT("hitchhiker"))bOk=BeginHitcher(bForced,Why);
 else if(Id==TEXT("stranded_rv"))bOk=BeginRv(bForced,Why);
 else if(Id==TEXT("humor"))bOk=BeginHumor(Variant,bForced,Why);
 else if(Id==TEXT("figure_near"))bOk=BeginFigure(bForced,Why);
 else if(Id==TEXT("big_night"))bOk=BeginBig(Variant,bForced,Why);
 else bOk=BeginMore(Id,Variant,bForced,Why);
 if(!bOk)return false;
 ++Serial;TryAt.Remove(Id);
 // Its start on the night's clock, for what is measured in the night's minutes.
 if(Now.Id==Id&&Now.Clock<=0)Now.Clock=F->Clock();
 if(!bForced)++Tally.NStarted;
 // What stays out on the land is saved from now, so its day is too. What holds the ground counts its day when it is over
 // (EndEvent): one cut short by a Continue may be drawn again (or carries on, S47Hap::Resumable).
 const FSite* Made=Sites.Num()>SitesBefore?&Sites.Last():nullptr;
 if(Now.Id!=Id||bForced)LastDays.Add(Id,F->Day());
 // A danger is counted when the player meets it (MeetDanger), not when it is drawn.
 if(M->Rhythm&&!IsDanger(Id))M->Rhythm->Began(FName(*Id));
 // Between 23 and 01 only what was noticed near counts (StepLate): these are heard or felt as they start, the others when seen.
 // A word on the radio is noticed, but far: it does not count as something near.
 const double Away=AwayNow(Id);
 static const TCHAR* const AtOnce[]={TEXT("car_dies"),TEXT("clock_ahead"),TEXT("dogs_hush"),TEXT("town_blackout"),TEXT("hood_warm"),TEXT("truck_tracks"),TEXT("animals_flee")};
 for(const TCHAR* A:AtOnce)if(Id==A)Noticed(Id,Id==TEXT("dogs_hush")?Away:-1.,Id==TEXT("dogs_hush")?TEXT("sound"):TEXT("here"),Serial);
 if(Id==TEXT("cb_stranded")||Id==TEXT("array_burst")||Id==TEXT("am_caller"))Noticed(Id,1e6,TEXT("the radio"),Serial);
 if(Made&&Made->bHeard)Noticed(Id,1e6,TEXT("the radio"),Made->Serial);
 if(Id==TEXT("phone_empty")&&Away>=0&&Away<60)Noticed(Id,Away,TEXT("sound"),Serial);
 // Where it is: the place it stands at or was heard of, for the log (the catalogue logs its own draws).
 const FString AtTitle=Made?FString::Printf(TEXT("%s (%s)"),*Made->Title,*Made->Place):Now.Id==Id&&!Now.Title.IsEmpty()?FString::Printf(TEXT("%s (%s)"),*Now.Title,*Now.Place):FString(TEXT("where the player is"));
 const FString& PlaceId=Made?Made->Place:Now.Place;
 if(!PlaceId.IsEmpty()&&F->EventPlaces)FS47EventPlaces::LogPick(Id,F->EventPlaces->Find(PlaceId),TEXT("night event"));
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening start %s%s day %d %s in %s at %s, %s%s"),*Id,Now.Id==Id&&!Now.Variant.IsEmpty()?*FString::Printf(TEXT(" (%s)"),*Now.Variant):TEXT(""),F->Day(),*S47Hap::ClockText(F->Clock()),*M->Web->Area(),*AtTitle,Away<0?TEXT("no distance"):*FString::Printf(TEXT("%.0f m from the player"),Away),bForced?TEXT(", F10"):TEXT(""));
 M->RequestAutosave();
 return true;
}
void FS47Happenings::EndEvent(bool bEvidence)
{
 if(Now.Id.IsEmpty())return;
 const FString Id=Now.Id;const bool bDanger=Now.bDanger,bMet=Now.bMet,bForced=Now.bForced;
 const double Away=AwayNow(Id);const bool bNoticed=NoticedSerials.Contains(Now.Serial);
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening end %s after %.0f s (%.0f min of the night)%s%s, %s, %s"),*Id,Now.Age,M&&M->Freeform&&Now.Clock>0?(M->Freeform->Clock()-Now.Clock)/60.:0.,bEvidence?TEXT(", kept"):TEXT(""),bDanger?(bMet?TEXT(", met"):TEXT(", never met")):TEXT(""),bNoticed?TEXT("noticed"):TEXT("not noticed"),Away<0?TEXT("no distance"):*FString::Printf(TEXT("%.0f m from the player"),Away));
 PutAway(Id);
 if(Id==TEXT("big_night"))EndBigEffects();
 if(M&&M->Audio)M->Audio->Stop(TEXT("hapEngine"),.6f);
 // The truck's lights are never left out by an event cut short.
 if(bDimmed&&M&&M->World){M->World->Trouble.Dim=1;bDimmed=false;}
 if(M&&M->Freeform&&!bForced)LastDays.Add(Id,M->Freeform->Day());
 // The voice is spent once it has been heard; unheard, it may still come.
 if(Id==TEXT("cb_echo")&&bMet)bEchoArmed=false;
 Now=FActive();
 // A danger nobody met is followed by the short rest, not the long one.
 if(M&&M->Rhythm)M->Rhythm->Ended(FName(*Id),!bDanger||bMet);
 if(M)M->RequestAutosave();
}

void FS47Happenings::Step(double Dt)
{
 auto* F=M?M->Freeform.Get():nullptr;
 if(!F||!F->Active()||!M->IsFreeform()||!M->Web||!M->Player||!M->World||!Life.IsValid())return;
 if(Scene.Get()!=M->Web.Get()){Props.Reset();Scene=M->Web;Sight.Init(M);LoopsOn.Reset();}
 // The Cosmic one breathes and its stars drift while it is out there.
 for(const auto& P:Props)if(P&&P->bShown)for(S47TripoNew::FCosmicGlow& G:P->Cosmic)S47TripoNew::StepCosmicGlow(G,Dt);
 if(F->Held()){StepGeiger(0);return;}
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 if(Night!=PlanNight)PlanTonight(Night);
 // What lasts is measured at the night's pace (NightPace): at x1 in play nothing changes.
 const double Pc=NightPace();Paced+=Dt*Pc;
 if(AheadSeconds>0&&AheadNight!=Night){AheadSeconds=0;UE_LOG(LogS47Happen,Display,TEXT("S47: happening clock_ahead over: the dash agrees again"));}
 const FString Area=M->Web->Area();
 if(Area!=LastArea){LastArea=Area;HideAll();for(const auto& P:Props)if(P)P->bPosed=false;}
 StepSays(Dt);
 if(!Now.Id.IsEmpty())StepEvent(Dt*Pc);
 StepSites(Dt);
 SiteSounds();
 StepTips();
 StepBigAfter();
 // The night by its own rules: only where a real game is played (or a test asks for them), never in the first shift.
 // The first big night asks for itself in the first shift too (it waits for the shift to end, and is never lost to it).
 if(F->RulesLive()||bBigDevNatural)StepBigDue();
 if(F->RulesLive()&&!F->InTutorial())
 {
  StepSwap();
  static const TCHAR* const Order[]={TEXT("figure_near"),TEXT("hitchhiker"),TEXT("cb_echo"),TEXT("car_light"),TEXT("car_dies"),TEXT("road_glimpse"),TEXT("cb_stranded"),TEXT("landing"),TEXT("humor"),TEXT("array_burst"),
   TEXT("dogs_hush"),TEXT("phone_empty"),TEXT("horizon_figure"),TEXT("room6_tv"),TEXT("town_blackout"),TEXT("animals_flee"),TEXT("dust_devil"),
   TEXT("cattle_ring"),TEXT("stranded_rv"),TEXT("carcass"),TEXT("house_lit"),TEXT("cinema_film"),TEXT("missing_person"),TEXT("truck_tracks"),TEXT("hood_warm"),TEXT("am_caller")};
  for(const TCHAR* Id:Order)
  {
   if(!Due(Id)||Paced<TryAt.FindRef(Id))continue;
   const bool bGround=!S47Hap::Background(FString(Id));
   // Nothing else, not even a word on the CB, while a danger holds the ground.
   if((bGround||Now.bDanger)&&!Now.Id.IsEmpty())continue;
   FString Why;
   if(StartEvent(Id,FString(),false,Why))break;
   // Not here, not now: said once for each reason, and asked again in a few seconds of the night's pace, not every frame.
   Refused(Id,Why);
   TryAt.Add(Id,Paced+3.);
  }
  StepLate();
  StepCues();
 }
 StepWatch();
 StepReport();
 StepUnease(Dt);
 StepGeiger(Dt);
}

// Tom, 8 November: something every night, also between eleven and one. If nothing has come by half past twelve, the best of these that
// fits where the player is now comes, still asking the rhythm.
void FS47Happenings::StepLate()
{
 auto* F=M->Freeform.Get();
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 const double H=HourOfNight(F->Clock());
 if(H<23||H>=25||LateNight==Night)return;
 // Something near the ground between eleven and one, of this or any other system, or a piece of unease. Lights in the sky or far
 // off do not count (Tom's coordinator, 8 November): the watch is for what comes close.
 if(!bLate)
 {
  static const TCHAR* const Sky[]={TEXT("flyover"),TEXT("response_lights"),TEXT("farm_lights"),TEXT("town_lights"),TEXT("dusk_orb"),TEXT("sky_formation"),TEXT("sky_high")};
  FName Busy=NAME_None;if(M->Rhythm)Busy=M->Rhythm->ReadNow().Busy;
  // Our own are counted only once noticed (Noticed), not because they hold the ground.
  if(!Now.Id.IsEmpty()&&Busy==FName(*Now.Id))Busy=NAME_None;
  if(F->Director&&F->Director->Busy()&&Busy.IsNone())Busy=FName(*F->Director->ActiveEvent());
  bool bSky=false;for(const TCHAR* S:Sky)bSky|=Busy==FName(S);
  bLate=!Busy.IsNone()&&!bSky;
 }
 if(bLate){LateNight=Night;return;}
 // From tonight's hour (half past eleven to ten to one), the first that fits where the player is now, in tonight's order, tried
 // again every ten minutes until one. Some nights it is a hush instead: the crickets stop, and nothing comes.
 if(H<LateAt||F->Clock()<LateTryClock)return;
 LateTryClock=F->Clock()+600;
 if(bLateHush)
 {
  S47HapOmen::Forewarn(M,TEXT("happen:late_hush"),Eye(),nullptr,150,.45,150);
  LateNight=Night;
  UE_LOG(LogS47Happen,Display,TEXT("S47: happenings: between 23:00 and 01:00 on night %d, a hush"),Night);
  return;
 }
 // A calm night (FS47NightRhythm::Quiet) has no lights and no calls that lead anywhere: only the small ones then.
 // Only what happens at the player, seen or heard where they stand (NeedNear): never a camper or a cinema kilometres off (critics).
 const bool bQuiet=F->QuietNight(Night);
 const uint32 Sit=SituationNow();
 for(const FString& Id:SwapOrder(Night))
 {
  if(LastDays.FindRef(Id)==F->Day()||!(NeedMask(Id)&Sit)||!(NeedMask(Id)&S47Hap::NeedNear))continue;
  if(bQuiet&&!(NeedMask(Id)&S47Hap::NeedSmall))continue;
  FString Why;
  if(StartEvent(Id,FString(),false,Why))
  {
   const double Away=AwayNow(Id);
   LateNight=Night;UE_LOG(LogS47Happen,Display,TEXT("S47: happenings: nothing near between 23:00 and 00:30 on night %d, so %s, %s"),Night,*Id,Away<0?TEXT("where the player is"):*FString::Printf(TEXT("%.0f m from the player"),Away));
   return;
  }
 }
 // Or a piece of unease, which needs no more than the player being somewhere.
 if(Uneasy.I<0&&!UneasePool.IsEmpty())for(const int32 I:UneasePool)
 {
  if(UneaseDone.Contains(I)||I==11||I==12||I==14||I==2||I==19)continue;
  FString Why;
  if(FireUnease(I,false,Why)){LateNight=Night;UE_LOG(LogS47Happen,Display,TEXT("S47: happenings: nothing near between 23:00 and 00:30 on night %d, so unease %d"),Night,I+1);return;}
 }
 if(H>=24.8)UE_LOG(LogS47Happen,Display,TEXT("S47: happenings: nothing between 23:00 and 01:00 on night %d, and nothing fits here"),Night);
}
// Where the player is, as the events ask for it: at the wheel, on foot outside, under a roof, at SARO, in Caliche, at the motel.
uint32 FS47Happenings::SituationNow() const
{
 using S47Hap::NeedDrive;using S47Hap::NeedFoot;using S47Hap::NeedIndoors;using S47Hap::NeedSaro;using S47Hap::NeedTown;using S47Hap::NeedMotel;using S47Hap::NeedSmall;
 uint32 Sit=0;
 if(Driving())Sit|=NeedDrive;else if(Outdoors())Sit|=NeedFoot;else Sit|=NeedIndoors;
 const FString Area=M&&M->Web?M->Web->Area():FString();
 if(Area==TEXT("saro"))Sit|=NeedSaro;
 double X=0,Z=0;
 if(PlayerRoad(X,Z))
 {
  if((Area==TEXT("road")||Area==TEXT("diner"))&&Z>1800&&Z<2450&&FMath::Abs(X)<320&&!Driving())Sit|=NeedTown;
  if(M->Freeform&&M->Freeform->EventPlaces)if(const FS47EventPlace* Mo=M->Freeform->EventPlaces->Find(TEXT("motel")))if(FVector2D::Distance(FVector2D(Mo->X,Mo->Z),FVector2D(X,Z))<160&&!Driving())Sit|=NeedMotel;
 }
 return Sit;
}
// What each of them needs of where the player is (SituationNow), any one of these bits. Small: it may come on a calm night.
uint32 FS47Happenings::NeedMask(const FString& Id)
{
 using S47Hap::NeedDrive;using S47Hap::NeedFoot;using S47Hap::NeedIndoors;using S47Hap::NeedSaro;using S47Hap::NeedTown;using S47Hap::NeedMotel;using S47Hap::NeedSmall;
 using S47Hap::NeedNear;
 if(Id==TEXT("car_dies")||Id==TEXT("hitchhiker")||Id==TEXT("car_light")||Id==TEXT("am_caller"))return NeedDrive|(Id==TEXT("car_dies")?NeedSmall|NeedNear:0u);
 if(Id==TEXT("cattle_ring")||Id==TEXT("animals_flee")||Id==TEXT("dust_devil"))return NeedFoot|NeedSmall|NeedNear;
 if(Id==TEXT("dogs_hush"))return NeedFoot|NeedTown|NeedSmall|NeedNear;
 if(Id==TEXT("horizon_figure")||Id==TEXT("figure_near"))return NeedFoot;
 if(Id==TEXT("landing"))return NeedFoot|NeedDrive|NeedSaro;
 // CB-TRUCK-20261109: the stuck driver and the voice come over the CB, and the CB is in the truck only. The missing man's word can
 // also come from Dale over the walkie at SARO.
 if(Id==TEXT("missing_person"))return NeedDrive|NeedSaro;
 if(Id==TEXT("cb_stranded")||Id==TEXT("cb_echo"))return NeedDrive;
 if(Id==TEXT("stranded_rv")||Id==TEXT("carcass")||Id==TEXT("cinema_film"))return NeedFoot|NeedDrive|NeedIndoors|NeedSaro|NeedTown;
 if(Id==TEXT("house_lit"))return NeedFoot|NeedDrive|NeedTown;
 if(Id==TEXT("phone_empty")||Id==TEXT("town_blackout"))return NeedTown|NeedNear|(Id==TEXT("town_blackout")?NeedSmall:0u);
 if(Id==TEXT("room6_tv"))return NeedMotel|NeedNear;
 if(Id==TEXT("array_burst"))return NeedSaro;
 if(Id==TEXT("humor"))return NeedTown|NeedMotel|NeedDrive;
 if(Id==TEXT("road_glimpse"))return NeedDrive;
 // What follows from something else (the clock, the hood, the tracks) is never drawn in another's place.
 return 0;
}
// Tonight's order for whatever stands in for a draw that cannot happen where the player is: from the seed, the same in any Continue.
TArray<FString> FS47Happenings::SwapOrder(int32 Night) const
{
 TArray<FString> Out;
 // Humour and the crossing far ahead are rare by the week, from the seed: never drawn in another's place.
 for(const FString& Id:Ids())if(NeedMask(Id)!=0&&!IsDanger(Id)&&Id!=TEXT("humor")&&Id!=TEXT("road_glimpse"))Out.Add(Id);
 const int32 Seed=M&&M->Freeform?M->Freeform->Seed():47;
 Out.Sort([Seed,Night](const FString& A,const FString& B){return FS47NightRhythm::Roll(Seed,Night,S47Hap::SaltOf(A)^0x53574150u)<FS47NightRhythm::Roll(Seed,Night,S47Hap::SaltOf(B)^0x53574150u);});
 return Out;
}
// A draw that cannot happen where the player is waits for its own turn; one that has waited an hour past its time, with the player
// still somewhere it cannot happen, gives its place to one that can. Same number of things in the night either way.
void FS47Happenings::StepSwap()
{
 auto* F=M->Freeform.Get();
 if(Paced<SwapTry)return;
 SwapTry=Paced+5;
 const int32 Night=FS47FreeformSession::NightOf(F->Clock());
 const double H=HourOfNight(F->Clock());
 // Only in the night's own hours: in the morning, when the night is drawn, nothing is stale yet (S47Hap::StaleAt).
 if(H<19||H>=29.5)return;
 const uint32 Sit=SituationNow();
 FString Stale;double StaleFrom=H;
 for(const auto& KV:Plan)
 {
  const uint32 Need=NeedMask(KV.Key);
  if(Need==0||IsDanger(KV.Key)||LastDays.FindRef(KV.Key)==F->Day()||!S47Hap::StaleAt(H,KV.Value)||(Need&Sit))continue;
  Stale=KV.Key;StaleFrom=KV.Value;break;
 }
 if(Stale.IsEmpty())return;
 const bool bQuiet=F->QuietNight(Night);
 for(const FString& Alt:SwapOrder(Night))
 {
  if(Plan.Contains(Alt)||LastDays.FindRef(Alt)==F->Day()||!(NeedMask(Alt)&Sit))continue;
  if(bQuiet&&!(NeedMask(Alt)&S47Hap::NeedSmall))continue;
  // From now, never from an hour gone by: it may start at once, and has its own hour and a half.
  Plan.Remove(Stale);Plan.Add(Alt,FMath::Max(H,StaleFrom));
  UE_LOG(LogS47Happen,Display,TEXT("S47: happenings night %d: %s cannot happen where the player is, so %s instead, from %s"),Night,*Stale,*Alt,*S47Hap::ClockText(F->Clock()));
  return;
 }
}

void FS47Happenings::StepEvent(double Dt)
{
 Now.Age+=Dt;
 if(Now.Id==TEXT("car_dies"))StepCar(Dt);
 else if(Now.Id==TEXT("landing"))StepLanding(Dt);
 else if(Now.Id==TEXT("hitchhiker"))StepHitcher(Dt);
 else if(Now.Id==TEXT("humor"))StepHumor(Dt);
 else if(Now.Id==TEXT("figure_near"))StepFigure(Dt);
 else if(Now.Id==TEXT("big_night"))StepBig(Dt);
 else if(Now.Id==TEXT("cb_stranded"))
 {
  if(Now.Age>=Now.Len)EndEvent(true);
 }
 else if(Now.Id==TEXT("cb_echo"))
 {
  // Heard, it is the night's danger. When the voice stops the set goes dead and for a while it is out there: in the truck the engine
  // and the lights die (CB-TRUCK-20261109: the CB is only in the truck). Out of the cab by then the night goes quiet, and at SARO
  // something knocks. Out on foot in that while, the light comes down.
  if(!Now.bMet&&Now.Age>1.2&&CanHearCb())MeetDanger();
  if(Now.Stage==0&&Now.Age>=Now.Yaw)
  {
   Now.Stage=1;Now.Watch=Now.Age+24;Now.Len=Now.Age+26;
   if(Driving()){FS47TroubleOpts O;O.bKey=true;M->World->Trouble.Start(ES47Trouble::Stall,O);M->World->Trouble.Dim=.05;bDimmed=true;}
   else if(US47Audio* A=M->Audio.Get())
   {
    if(A->Night)A->Night->Hush(40,.02,.2,3.);
    const FS47Obj EastDoor=M->Web->Area()==TEXT("saro")?M->Web->Obj(TEXT("room.objs.doorEast")):FS47Obj();
    if(EastDoor)for(int32 K=0;K<3;++K){FS47Sound O;O.At=EastDoor.WorldPosition()+FVector(0,1.3,0);O.Gain=.75f;O.Rate=1.1f;O.When=2.f+.42f*K;O.Lp=2200;A->Sfx(TEXT("thudSoft"),O);}
   }
  }
  if(Now.Stage==1&&bDimmed&&Now.Age>Now.Len-5){M->World->Trouble.Dim=1;bDimmed=false;}
  if(Now.Age>=Now.Len)EndEvent(true);
 }
 else if(!StepMore(Dt))EndEvent(false);
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
    Past.Add({S.X,S.Z,S.Night});while(Past.Num()>MostPast)Past.RemoveAt(0); // LOOP-4-20261109: the needle remembers it
    Sites.RemoveAt(I);M->RequestAutosave();continue;
   }
  }
 }
 PlaceSites();
 // Noticed from where the player is: seen in fact (SeenLit), or heard (SiteSounds). Nothing is told; what is kept is what was seen.
 for(FSite& S:Sites)
 {
  FVector At;if(!WorldOf(S.X,S.Z,At))continue;
  const double Away=AwayFrom(At);
  const double H=HourOfNight(Clock);
  // The ring of cattle: made out only in light, the lamp's or the truck's or the day's. Heard before that (SiteSounds).
  if(S.Kind==TEXT("cattle")&&!S.bFound&&Away<170&&SeenLit(At+FVector(0,1,0),170,34,LitGround,TEXT("cattle")))
  {
   S.bFound=true;
   Noticed(TEXT("cattle_ring"),Away,TEXT("sight"),S.Serial);
   const FString Way=S47Hap::Compass(S.Az);
   const FSite* Ring=SiteOf(TEXT("landing"));
   const bool bRing=Ring&&Ring->Night>=S.Night-3&&Ring->bKnown&&Ring->Az>=0;
   // What was there, in numbers: the way their heads were turned, and (when one is on the map) the bearing of the last lights.
   F->RecordEvidence(FString::Printf(TEXT("cattle_ring_d%d"),S.Night),FString::Printf(TEXT("At %s, out by %s: seven head of cattle close in a ring, heads all one way, about %.0f degrees (%s). A bell on one of them. None of them grazing.%s"),*S47Hap::ClockText(Clock),*S.Title,S.Az,*Way,bRing?*FString::Printf(TEXT(" The landing bearing on the map from night %d is %.0f degrees from SARO."),Ring->Night,Ring->Az):TEXT("")));
   Deed(FString::Printf(TEXT("You stood and looked at the cattle out by %s."),*S.Title));
   UE_LOG(LogS47Happen,Display,TEXT("S47: happening cattle_ring seen at %.0f m, facing %.0f%s"),Away,S.Az,S.Variant==TEXT("landing")?TEXT(", toward the landing"):TEXT(""));
   M->RequestAutosave();
  }
  // GLIMPSE-ROAD-20261109: the prints on the shoulder, found on foot close by and looked at, in the day or in the lamp.
  if(S.Kind==TEXT("roadtracks")&&!S.bFound&&Away<9&&!Driving()&&SeenLit(At+FVector(0,.1,0),10,40,LitGround,S.Variant==TEXT("low")?TEXT("roadtracks_low"):TEXT("roadtracks_tall")))
  {
   S.bFound=true;
   const double Hr=S47Hap::HourOf(Clock);
   Noticed(TEXT("road_glimpse"),Away,TEXT("sight"),S.Serial);
   F->RecordEvidence(FString::Printf(TEXT("road_tracks_d%d"),S.Night),S47RoadGlimpse::TracksRecord(S.Variant==TEXT("low"),S47Hap::ClockText(Clock),Hr>=6.2&&Hr<19.6));
   Deed(TEXT("You looked at the prints on the shoulder of the highway."));
   UE_LOG(LogS47Happen,Display,TEXT("S47: happening road_glimpse: the prints on the shoulder found at %.0f m (%s)"),Away,*S.Variant);
   M->RequestAutosave();
  }
  if(S.Kind==TEXT("rv")&&!S.bHeard&&!S.bFound&&(H>=19.5&&H<30))
  {
   // Its windows give their own light: seen from far off where nothing stands between.
   if(Outdoors()&&Away<1100&&SeenLit(At+FVector(0,2,0),1150,26,LitOwn,TEXT("rv")))
   {
    S.bHeard=true;Noticed(TEXT("stranded_rv"),Away,TEXT("sight"),S.Serial);
    UE_LOG(LogS47Happen,Display,TEXT("S47: happening stranded_rv seen at %.0f m"),Away);
    M->RequestAutosave();
   }
   else if(CanHearCb()&&Now.Id.IsEmpty()&&FS47NightRhythm::Roll(F->Seed(),int32(Clock/600.),0x52564342u)<.25f)
   {
    S.bHeard=true;Noticed(TEXT("stranded_rv"),1e6,TEXT("the radio"),S.Serial);
    Cb({FString::Printf(TEXT("Breaker one-nine. Who's camping way out past %s? Every light burning, plain as day from the blacktop. Party I wasn't asked to, I guess."),*Spoken(S.Title))},TEXT("old"),false);
    UE_LOG(LogS47Happen,Display,TEXT("S47: happening stranded_rv heard of on the CB"));
    M->RequestAutosave();
   }
  }
  if(S.Kind==TEXT("houselit")&&!S.bDone)
  {
   const FVector Win=At+FVector(0,1.7,0);
   if(!S.bFound&&Away>35&&Away<1300&&SeenLit(Win,1300,6,LitOwn,TEXT("litwin")))
   {
    S.bFound=true;Noticed(TEXT("house_lit"),Away,TEXT("sight"),S.Serial);
    F->RecordEvidence(FString::Printf(TEXT("house_lit_d%d"),S.Night),FString::Printf(TEXT("At %s a light was on in a window at %s."),*S47Hap::ClockText(Clock),*S.Title));
    UE_LOG(LogS47Happen,Display,TEXT("S47: happening house_lit seen at %.0f m"),Away);
    M->RequestAutosave();
   }
   if(Away<18)
   {
    // Out as the player comes up to it, with a screen door's slap somewhere at the back of the house.
    S.bDone=true;
    if(M->Audio){FS47Sound O;O.At=At+FVector(-3,1,4);O.Gain=.5f;O.Rate=1.15f;O.Lp=2600;M->Audio->Sfx(TEXT("thudSoft"),O);FS47Sound C=O;C.Gain=.25f;C.When=.18f;M->Audio->Sfx(TEXT("doorCreak0"),C);}
    if(S.bFound)F->RecordEvidence(FString::Printf(TEXT("house_lit_d%d_out"),S.Night),FString::Printf(TEXT("The light at %s went out as I came up to it. A door banged somewhere at the back."),*S.Title));
    Deed(FString::Printf(TEXT("You walked up to the lit window at %s."),*S.Title));
    M->RequestAutosave();
   }
  }
  // Seen from the street, the doors flickering, or found at them; heard through the doors before that (SiteSounds).
  if(S.Kind==TEXT("cinema")&&!S.bFound&&(Away<40||(Away<300&&SeenLit(At+FVector(0,2,0),320,18,LitOwn,TEXT("cinemalight")))))
  {
   S.bFound=true;Noticed(TEXT("cinema_film"),Away,Away<40?TEXT("sound"):TEXT("sight"),S.Serial);
   F->RecordEvidence(FString::Printf(TEXT("cinema_film_d%d"),S.Night),FString::Printf(TEXT("At %s the Plaza's doors were flickering blue-white, after the last show.%s"),*S47Hap::ClockText(Clock),Away<40?TEXT(" Through the doors, a projector running. The lobby lights were off."):TEXT("")));
   Deed(Away<40?TEXT("You listened at the cinema's doors."):TEXT("You saw the cinema's doors flicker from down the street."));
   M->RequestAutosave();
  }
  // What nobody saw or heard of comes back as talk the next evening: on the CB in the words of whoever saw it, from Dale otherwise.
  const bool bUnseenRing=S.Kind==TEXT("landing")&&!S.bKnown&&!S.bFound,bUnheardCow=S.Kind==TEXT("carcass")&&!S.bHeard&&!S.bFound;
  if((bUnseenRing||bUnheardCow)&&S.Night<F->Day()&&H>=20&&H<23&&Now.Id.IsEmpty()&&Uneasy.I<0&&Paced>=RumorTry)
  {
   RumorTry=Paced+40;
   const FString Named=Spoken(S.Title);
   if(bUnseenRing)
   {
    Tip({FString::Printf(TEXT("Anybody else see something come down out past %s last night? I sure did. Slow, like it was being let down on a rope. I'm not going out there to look, either."),*Named)},TEXT("man"),
     FString::Printf(TEXT("A driver says he saw lights come down out past %s last night. Nobody went to look."),*Named));
    S.bKnown=true;S.bHeard=true;S.Spread=FMath::Max(S.Spread,30.);
   }
   else
   {
    Tip({FString::Printf(TEXT("Lost a heifer out by %s night before last. Vet took one look at her and got back in his truck. I'd like to know what he saw."),*Named)},TEXT("old"),
     FString::Printf(TEXT("A rancher lost a heifer out by %s. The vet looked at her and would not say."),*Named));
    S.bHeard=true;
   }
   UE_LOG(LogS47Happen,Display,TEXT("S47: happening site %s at %s: talked of the next evening"),*S.Kind,*S.Title);
   M->RequestAutosave();
  }
 }
}

// What a lasting thing sounds like, from near enough to hear it: the herd's lowing and its bell, the camper's generator, the
// projector through the Plaza's doors. Started as the player comes near, stopped as they go (Tom's critics: sound, not a caption).
void FS47Happenings::SiteSounds()
{
 US47Audio* A=M->Audio.Get();if(!A)return;
 TSet<FString> Want;
 const bool bTime=Paced>=SoundNext;
 if(bTime)SoundNext=Paced+FMath::FRandRange(7.f,14.f);
 for(const FSite& S:Sites)
 {
  FVector At;if(!WorldOf(S.X,S.Z,At))continue;
  const double Away=AwayFrom(At);
  if(S.Kind==TEXT("cattle")&&Away<220&&bTime)
  {
   // Now and then one of them lows, and the bell knocks as she shifts; nearer, more of them.
   if(FMath::FRand()<(Away<90?.7f:.4f)){FS47Sound O;O.At=At+FVector(FMath::FRandRange(-4.f,4.f),1.2,FMath::FRandRange(-4.f,4.f));O.Gain=.55f;O.Rate=FMath::FRandRange(.88f,1.08f);A->Sfx(Synth(TEXT("low")),O);}
   if(FMath::FRand()<.6f){FS47Sound B;B.At=At+FVector(1,1,-1);B.Gain=.35f;B.When=FMath::FRandRange(.5f,2.f);A->Sfx(Synth(TEXT("bell")),B);}
   if(!S.bFound)Noticed(TEXT("cattle_ring"),Away,TEXT("sound"),S.Serial);
  }
  if(S.Kind==TEXT("rv")&&!S.bDone&&Away<160)
  {
   const FString Key=FString::Printf(TEXT("hapRv%d"),S.Serial);Want.Add(Key);
   if(!LoopsOn.Contains(Key)){FS47Sound O;O.At=At+FVector(-1.5,.6,2.5);O.Gain=.3f;O.Rate=.62f;O.Lp=900;O.bLoop=true;A->LoopSfx(Key,A->Or(TEXT("obsGenerator"),TEXT("engineIdle")),O);LoopsOn.Add(Key);}
  }
  if(S.Kind==TEXT("cinema")&&Away<120)
  {
   const FString Key=FString::Printf(TEXT("hapCinema%d"),S.Serial);Want.Add(Key);
   if(!LoopsOn.Contains(Key)){FS47Sound O;O.At=At+FVector(0,1.5,-3);O.Gain=.4f;O.Lp=3200;O.bLoop=true;A->LoopSfx(Key,Synth(TEXT("projector")),O);LoopsOn.Add(Key);}
  }
 }
 // The singer by the pool, while he sings.
 if(Now.Id==TEXT("humor")&&Now.Variant==TEXT("singer"))
 {
  FVector At;if(WorldOf(Now.X,Now.Z,At)&&AwayFrom(At)<140)
  {
   Want.Add(TEXT("hapSinger"));
   if(!LoopsOn.Contains(TEXT("hapSinger"))){FS47Sound O;O.At=At+FVector(0,1.6,0);O.Gain=.45f;O.bLoop=true;A->LoopSfx(TEXT("hapSinger"),Synth(TEXT("croon")),O);LoopsOn.Add(TEXT("hapSinger"));}
  }
 }
 for(auto It=LoopsOn.CreateIterator();It;++It)if(!Want.Contains(*It)){A->Stop(*It,.6f);It.RemoveCurrent();}
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
  // A lit window in an empty house, seen from a long way off, out when you come up to it.
  if(S.Kind==TEXT("houselit")&&!S.bDone&&Away<1500)
  {
   if(FProp* P=Prop(TEXT("litwin"))){PoseProp(*P,At+FVector(0,1.7,0),S.Yaw);ShowProp(*P,true);Want.Add(TEXT("litwin"));}
   continue;
  }
  // The cinema's doors, flickering as a film runs inside for nobody.
  if(S.Kind==TEXT("cinema")&&Away<1500)
  {
   if(FProp* P=Prop(TEXT("cinemalight")))
   {
    PoseProp(*P,At,0);ShowProp(*P,true);Want.Add(TEXT("cinemalight"));
    const float K=FMath::Fmod(M->T*7.3,1.)<.15?.2f:FMath::FRandRange(.65f,1.f);
    for(FProp::FLamp& L:P->Lamps)if(UPointLightComponent* Lc=L.L.Get())Lc->SetIntensity(L.Lumens*K);
   }
   continue;
  }
  if(!bSameLand||Away>900)continue;
  const auto Show=[&](const TCHAR* Kind,const FVector& Foot,double Yaw){if(FProp* P=Prop(Kind)){PoseProp(*P,Foot,Yaw);ShowProp(*P,true);Want.Add(Kind);}};
  // Gone over by the military: no ring, only their raked circle where it was (drawn by S47Visitors).
  if(S.Kind==TEXT("landing")&&S47HapRakedSite(M,S.Place,S.Night)){M->Freeform->Visitors->RakeAt(S.Place,S.Night,S.X,S.Z);continue;}
  if(S.Kind==TEXT("landing"))
  {
   // Laid on the slope, not a flat plate half in the air (seen 8 November): the ground's fall across the ring, measured near.
   FQuat Tilt=FQuat::Identity;
   if(M->Player&&M->Player->Floor&&Away<300)
   {
    const double Ev=M->Player->Floor(At.X+3,At.Z),Wv=M->Player->Floor(At.X-3,At.Z),Sv=M->Player->Floor(At.X,At.Z+3),Nv=M->Player->Floor(At.X,At.Z-3);
    if(FMath::IsFinite(Ev)&&FMath::IsFinite(Wv)&&FMath::IsFinite(Sv)&&FMath::IsFinite(Nv))
     Tilt=FQuat::FindBetweenNormals(FVector(0,1,0),FVector(-(Ev-Wv)/6.,1.,-(Sv-Nv)/6.).GetSafeNormal());
   }
   if(FProp* Ring=Prop(TEXT("ring"))){PoseProp(*Ring,At,S.Yaw,Tilt);ShowProp(*Ring,true);Want.Add(TEXT("ring"));}
  }
  else if(S.Kind==TEXT("cattle"))Show(TEXT("cattle"),At,S.Yaw);
  else if(S.Kind==TEXT("carcass"))Show(TEXT("deadcow"),At,S.Yaw);
  else if(S.Kind==TEXT("missing"))Show(TEXT("pickup"),At,S.Yaw);
  else if(S.Kind==TEXT("tracks"))Show(TEXT("tracks"),At,S.Yaw);
  // GLIMPSE-ROAD-20261109: the prints going off the shoulder where something crossed (S.Yaw: along them, away from the road).
  else if(S.Kind==TEXT("roadtracks"))Show(S.Variant==TEXT("low")?TEXT("roadtracks_low"):TEXT("roadtracks_tall"),At,S.Yaw);
  // The big night's mark, in its own shape (S47BigNight.cpp).
  else if(S.Kind==TEXT("bigmark"))
  {
   // Laid on the slope it is on, as the ring is: not a flat plate half in the air.
   FQuat Tilt=FQuat::Identity;
   if(M->Player&&M->Player->Floor&&Away<300)
   {
    const double Ev=M->Player->Floor(At.X+3,At.Z),Wv=M->Player->Floor(At.X-3,At.Z),Sv=M->Player->Floor(At.X,At.Z+3),Nv=M->Player->Floor(At.X,At.Z-3);
    if(FMath::IsFinite(Ev)&&FMath::IsFinite(Wv)&&FMath::IsFinite(Sv)&&FMath::IsFinite(Nv))
     Tilt=FQuat::FindBetweenNormals(FVector(0,1,0),FVector(-(Ev-Wv)/6.,1.,-(Sv-Nv)/6.).GetSafeNormal());
   }
   const FString Mark=TEXT("bigmark_")+S.Variant;
   if(FProp* P=Prop(Mark)){PoseProp(*P,At,S.Yaw,Tilt);ShowProp(*P,true);Want.Add(Mark);}
  }
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
 for(const TCHAR* K:{TEXT("ring"),TEXT("cattle"),TEXT("rig"),TEXT("driver"),TEXT("handset"),TEXT("rv"),TEXT("litwin"),TEXT("cinemalight"),TEXT("deadcow"),TEXT("pickup"),TEXT("tracks"),TEXT("roadtracks_low"),TEXT("roadtracks_tall"),TEXT("bigmark_column"),TEXT("bigmark_fall"),TEXT("bigmark_rise")})if(!Want.Contains(K))if(FProp* P=FindProp(K))ShowProp(*P,false);
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
 }
 if(Now.Stage==2&&Now.Age>6.4)
 {
  Now.Stage=3;Tr.Dim=1;bDimmed=false;
  if(D)Tr.Start(ES47Trouble::Flicker,[]{FS47TroubleOpts O;O.Len=.5;return O;}());
 }
 if(Now.Stage==3&&Now.Age>7.4)
 {
  Now.Stage=4;
  auto* F=M->Freeform.Get();
  F->RecordEvidence(FString::Printf(TEXT("car_dies_d%d"),F->Day()),FString::Printf(TEXT("On the road at %s the headlights, the dash and the radio went dead for about four seconds while a light went over the truck, low. The engine coughed and kept running. Nothing on the road after, and nothing in the sky."),*S47Hap::ClockText(Now.Clock)));
  Deed(TEXT("You were on the road when your lights went out."));
 }
 if(Now.Age>=Now.Len)
 {
  if(bDimmed){Tr.Dim=1;bDimmed=false;}
  // Half the time the dash clock is not right afterwards: part of the same passing, so before its rest begins.
  auto* F=M->Freeform.Get();
  if(Now.bForced||FS47NightRhythm::Roll(F->Seed(),F->Day(),0x41484541u)<.5f){FString Why;if(!StartEvent(TEXT("clock_ahead"),FString(),Now.bForced,Why)){UE_LOG(LogS47Happen,Display,TEXT("S47: happening clock_ahead after the passing refused: %s"),*Why);}}
  EndEvent(true);
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
 // Nothing is said: the dash itself reads ahead (FS47Happenings::DashClock), and the record has the two times.
 F->RecordEvidence(FString::Printf(TEXT("clock_ahead_d%d"),F->Day()),FString::Printf(TEXT("After the lights went over, the clock on the dash read %s when my watch said %s."),*S47Hap::ClockText(Clock+AheadSeconds),*S47Hap::ClockText(Clock)));
 Deed(TEXT("You drove on with the dash clock forty minutes fast."));
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
 // Nearer as the weeks go (the curve's "near"): a long way off at first, close by in the last week.
 const FSpot S=Where(TEXT("cattle_ring"),0,Bit(ES47PlaceTrait::Farm)|Bit(ES47PlaceTrait::Open),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors)|Bit(ES47PlaceTrait::Array),bForced?0:FMath::Max(35.,70.*NearK),bForced?0:FMath::Max(160.,420.*NearK),0.f,Serial,!bForced);
 double X=0,Z=0;
 if(S.bSet){X=S.X;Z=S.Z;}
 else if(bForced&&PlayerRoad(X,Z)){X+=40;}
 else{Why=TEXT("no open ground near");return false;}
 // Off the yard and the buildings, out in the open: thirty metres from the place, on its own side.
 const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x52494e47u)*2*UE_DOUBLE_PI;
 if(S.bSet){X+=FMath::Cos(A)*30;Z+=FMath::Sin(A)*30;}
 if(bForced)
 {
  // F10: fourteen metres ahead of the player, inside the lamp's reach, so that the line can be seen at once (45 m was too dark to see).
  if(M->Night&&M->Night->Flashlight){M->Night->Flashlight->bHave=true;M->Night->Flashlight->bOn=true;}
  double Px=0,Pz=0;
  if(PlayerRoad(Px,Pz)&&M->Player){X=Px-FMath::Sin(M->Player->Yaw)*14;Z=Pz-FMath::Cos(M->Player->Yaw)*14;}
 }
 if(!bForced&&!MayStart(TEXT("cattle_ring"))){Why=TEXT("the rhythm says not now");return false;}
 if(!Prop(TEXT("cattle"))){Why=TEXT("no cow models");return false;}
 FSite C;C.Kind=TEXT("cattle");C.Place=S.Place;C.Title=S.bSet?S.Title:TEXT("the open ground");C.Night=F->Day();C.Serial=Serial+1;C.X=X;C.Z=Z;
 C.Until=F->Clock()+50*60;
 // Toward the last lights that came down, when there are any; otherwise wherever tonight has them looking.
 const FSite* Ring=SiteOf(TEXT("landing"));
 if(Ring&&Ring->Night>=F->Day()-3){C.Az=S47Hap::Bearing(FS47Places::AzimuthOf(X,Z,Ring->X,Ring->Z));C.Variant=TEXT("landing");}
 else C.Az=S47Hap::Bearing(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x53544152u)*360.);
 // The herd's yaw: 0 faces -z (north); a heading of Az degrees east of north.
 C.Yaw=-FMath::DegreesToRadians(C.Az);
 Sites.Add(C);UsedPlace(S.Place,TEXT("cattle_ring"));
 return true;
}
void FS47Happenings::UseCattle()
{
 const FSite* S=SiteOf(TEXT("cattle"));if(!S||!M->UI)return;
 auto* F=M->Freeform.Get();
 const FString Page=FString::Printf(TEXT("They do not move off when you walk in among them. Every head points the same way, about %.0f degrees, %s, at nothing you can see. One of them is shaking. No wind. No smell but cattle."),S->Az,*S47Hap::Compass(S->Az));
 F->RecordEvidence(FString::Printf(TEXT("cattle_ring_d%d"),S->Night),FString::Printf(TEXT("Seven head of cattle out by %s stood close in a ring, all facing about %.0f degrees, %s, and did not move off when I walked in among them."),*S->Title,S->Az,*S47Hap::Compass(S->Az)));
 Deed(FString::Printf(TEXT("You walked in among the cattle out by %s."),*S->Title));
 M->UI->Open(S47Kit::Shell(M->UI,TEXT("CATTLE IN A RING"),S47Kit::Col({S47Kit::Text(Page),S47Kit::Small(TEXT("Kept as a field record."))}),760));
}

// ---------------------------------------------------------------- cb_stranded
bool FS47Happenings::BeginStranded(const FString& Variant,bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 if(!CanHearCb()&&!bForced){Why=TEXT("no CB to hear it on: only sitting in the truck");return false;}
 if(const FSite* Old=SiteOf(TEXT("truck")))if(!Old->bDone){Why=TEXT("a driver is still waiting out there");return false;}
 using S47Hap::Bit;
 FSpot S=Where(TEXT("cb_stranded"),Bit(ES47PlaceTrait::Far),Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Road)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors),bForced?0:450,0,1.f,Serial);
 if(!S.bSet)S=Where(TEXT("cb_stranded"),0,Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors),bForced?0:450,0,1.f,Serial+7);
 if(!S.bSet){Why=TEXT("nowhere far out to be stuck");return false;}
 if(!bForced&&!MayStart(TEXT("cb_stranded"))){Why=TEXT("the rhythm says not now");return false;}
 FString Kind=Variant;
 if(Kind.IsEmpty())
 {
  // The voice that is not right comes more often as the weeks go (the curve's danger): rare in the first.
  const float R=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x56415249u);
  const float Wrong=.04f+.2f*(M->Rhythm?M->Rhythm->Data().Week(F->Day()).Danger:.4f);
  Kind=R<Wrong?TEXT("wrong"):R<Wrong+.33f?TEXT("empty"):TEXT("real");
 }
 Now=FActive();Now.Id=TEXT("cb_stranded");Now.Variant=Kind;Now.bForced=bForced;Now.Place=S.Place;Now.Title=S.Title;Now.Serial=Serial+1;
 const bool bWrong=Kind==TEXT("wrong");
 TArray<FString> Lines={
  FString::Printf(TEXT("Break one-nine, break one-nine. Anybody out toward %s?"),*Spoken(S.Title)),
  TEXT("Got my rig sunk to the axles in sand out here, and no shovel worth a darn."),
  bWrong?TEXT("I'll pay for the trouble. I'm not going anywhere. Come on back. Come on back."):TEXT("I'll pay for the trouble. I'm not going anywhere. Come on back.")};
 Cb(Lines,bWrong?TEXT("roswell"):FS47NightRhythm::Roll(F->Seed(),F->Day(),0x564f4943u)<.5f?TEXT("man"):TEXT("old"),bWrong);
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
 if(!CanHearCb()){Why=TEXT("no CB to hear it on: only sitting in the truck");return false;}
 if(!bForced&&!MayStart(TEXT("cb_echo"))){Why=TEXT("the rhythm says not now");return false;}
 const FString Did=Deeds.IsEmpty()?FString(TEXT("You were out on the road at night.")):Deeds[int32(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x44454544u)*Deeds.Num())%Deeds.Num()];
 Now=FActive();Now.Id=TEXT("cb_echo");Now.bForced=bForced;Now.bDanger=true;Now.Serial=Serial+1;Now.Clock=F->Clock();
 const TArray<FString> Lines={Did,TEXT("You did. Come on back.")};
 if(M->Audio){FS47Sound S;S.Gain=.4f;S.Dest=TEXT("radio");M->Audio->Sfx(M->Audio->Or(TEXT("radio_crackle_short"),TEXT("alienRadio")),S);}
 Cb(Lines,TEXT("roswell"),true);
 double Secs=0;for(const FString& L:Lines)Secs+=S47CB::SecsOf(L)+.6;
 // Yaw holds how long the voice speaks; then the set goes dead (StepEvent).
 Now.Yaw=Secs+.5;Now.Len=Secs+30;
 F->RecordEvidence(FString::Printf(TEXT("cb_echo_d%d"),F->Day()),FString::Printf(TEXT("At %s the CB opened by itself on nineteen and a voice said, slowly: \"%s\" Then there was only hiss on the channel."),*S47Hap::ClockText(F->Clock()),*Did));
 return true;
}

// ---------------------------------------------------------------- landing
bool FS47Happenings::BeginLanding(bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 const bool bAtSaro=M->Web->Area()==TEXT("saro");
 if(!bForced&&!Outdoors()&&!bAtSaro){Why=TEXT("indoors, away from the array");return false;}
 using S47Hap::Bit;
 // Nearer as the weeks go: past the low ground at first, then in plain sight from where the player stands.
 FSpot S=Where(TEXT("landing"),0,Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors)|Bit(ES47PlaceTrait::Array)|Bit(ES47PlaceTrait::People),bForced?250:FMath::Max(180.,350.*NearK),FMath::Max(900.,2800.*NearK),.3f*float(NearK),Serial);
 if(!S.bSet){Why=TEXT("no open ground for it");return false;}
 if(!bForced&&!MayStart(TEXT("landing"))){Why=TEXT("the rhythm says not now");return false;}
 if(!Prop(TEXT("lander"))){Why=TEXT("nothing to draw it with");return false;}
 // On open ground a little off the place's own point, never on top of a building.
 const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x4c4e4441u)*2*UE_DOUBLE_PI;
 const double X=S.X+FMath::Cos(A)*45,Z=S.Z+FMath::Sin(A)*45;
 // Ten seconds of warning (StepLanding), then twenty coming down, a pulse, and out.
 Now=FActive();Now.Id=TEXT("landing");Now.Place=S.Place;Now.Title=S.Title;Now.X=X;Now.Z=Z;Now.Len=36;Now.bForced=bForced;Now.Serial=Serial+1;Now.Clock=F->Clock();
 double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
 // Without mended branches the array still logs it, only wide.
 const int32 B=Branches();
 const double Spread=B>0?FS47Places::SpreadFor(B):40.;
 const double Err=(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x4c42455au)-.5)*Spread;
 FSite R;R.Kind=TEXT("landing");R.Place=S.Place;R.Title=S.Title;R.Night=F->Day();R.Serial=Serial+1;R.X=X;R.Z=Z;R.Yaw=A;R.Spread=Spread;
 // Its bearing now, kept (and saved) at once; on the map only once it is known: seen, logged, talked of or found.
 R.Az=S47Hap::Bearing(FS47Places::AzimuthOf(SaroX,SaroZ,X,Z)+Err);R.bKnown=false;R.Until=F->Clock()+3*86400.;
 Now.Yaw=R.Az;
 // One ring at a time: the older one is weathered away when new lights come down.
 Sites.RemoveAll([](const FSite& Old){return Old.Kind==TEXT("landing");});
 Sites.Add(R);UsedPlace(S.Place,TEXT("landing"));
 return true;
}
void FS47Happenings::StepLanding(double Dt)
{
 static_cast<void>(Dt);
 FVector Ground;
 const bool bHere=WorldOf(Now.X,Now.Z,Ground);
 US47Audio* A=M->Audio.Get();
 // The warning: the crickets stop and a dog far off, then whatever runs on power falters. Nothing in the sky yet.
 if(Now.Stage==0&&Now.Age>1.5)
 {
  Now.Stage=1;
  S47HapOmen::Forewarn(M,TEXT("happen:landing_warning"),Eye(),bHere?&Ground:nullptr,160,.5,60);
  if(A&&bHere)A->Far(TEXT("dog1"),Ground+FVector(40,2,-30),.25f,.95f);
 }
 if(Now.Stage==1&&Now.Age>6)
 {
  Now.Stage=2;
  if(Driving())M->World->Trouble.Start(ES47Trouble::Flicker,[]{FS47TroubleOpts O;O.Len=.9;return O;}());
  else if(!Outdoors()){if(A){FS47Sound O;O.Gain=.12f;O.Rate=.8f;A->Sfx(TEXT("switch"),O);A->HoldRoomTone(.2f,.15f);}ToneBack=M->T+1.6;}
  else if(M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn){M->Night->Flashlight->Flicker(.7,TEXT("cue"));ToneBack=M->T+1.2;}
 }
 // Then down slowly over twenty seconds from two hundred and fifty metres, a pulse on the ground, and out.
 const double Down=Now.Age-10.;
 const double T=FMath::Clamp(Down/20.,0.,1.);
 const double Up=FMath::Lerp(260.,5.,1.-FMath::Square(1.-T));
 const FVector At=Ground+FVector(FMath::Sin(Now.Age*.4)*6,Up,0);
 if(FProp* P=Prop(TEXT("lander")))
 {
  PoseProp(*P,At,Now.Age*.3);
  const bool bOn=bHere&&Down>=0&&(Down<20||(Down<23.5&&FMath::Fmod(Down,.6)<.35));
  ShowProp(*P,bOn);
 }
 if(bHere&&Down>=0&&!Now.bSeen&&SeenLit(At,4500,38,LitOwn,TEXT("lander")))
 {
  Now.bSeen=true;Noticed(TEXT("landing"),AwayFrom(At),TEXT("sight"));
  UE_LOG(LogS47Happen,Display,TEXT("S47: happening landing seen at %.0f m"),FVector::Dist(Eye(),At));
 }
 if(Now.Age<Now.Len)return;
 auto* F=M->Freeform.Get();
 FSite* R=nullptr;for(FSite& S:Sites)if(S.Kind==TEXT("landing")&&S.Serial==Now.Serial)R=&S;
 // At SARO the array logs it whether or not anyone is at the monitor; it is said only to one in the building.
 const bool bLogged=M->Web->Area()==TEXT("saro");
 const bool bAtMonitor=bLogged&&FS47FieldRadio::InSaroBuilding(M);
 // LOOP-3-20261109: the array's own log has it, whoever is at SARO and whether or not anyone saw it: a time from out there can be
 // set against it (FS47FreeformSession::SameMinute). LOOP-4: logged with nobody watching, its bearing is the night's one pointing-out
 // at the land, or nothing on the map when the night has had it.
 if(F)F->NoteArrayMinute(Now.Clock,Now.Clock+FMath::Max(Now.Len,60.),TEXT("landing"));
 if(R&&(Now.bSeen||Now.bForced||(bLogged&&TakePointOut(F?F->Director.Get():nullptr,FS47FreeformSession::NightOf(Now.Clock)))))
 {
  R->bKnown=true;
  // Only what it means for the map: what came down was on the screen, or on the array's monitor with its beeps and the printer.
  if(bAtMonitor&&!Now.bSeen&&M->Audio)
  {
   for(int32 K=0;K<4;++K)M->Audio->Beep(1320.f,.06f,.05f,.18f*K,TEXT("square"));
   FS47Sound P;P.At=FVector(-4,1,2);P.Gain=.3f;P.When=.9f;const FS47Play Line=M->Audio->Sfx(TEXT("printer"),P);
   if(Line&&M->Audio->Ctx.IsValid())Line.Src->Stop(M->Audio->Ctx->CurrentTime()+3.);
  }
  if(M->UI&&(Now.bSeen||bAtMonitor))M->UI->Toast(Now.bSeen?FString(TEXT("Bearing on your map (TAB).")):FString(TEXT("ARRAY / A bearing logged, on your map (TAB).")),5);
  if(Now.bSeen||Now.bForced)F->RecordEvidence(FString::Printf(TEXT("landing_seen_d%d"),R->Night),FString::Printf(TEXT("At %s lights came down out on the land, about %.0f degrees from SARO, give or take %.0f, and went out on the ground."),*S47Hap::ClockText(Now.Clock),R->Az,R->Spread));
  else F->RecordEvidence(FString::Printf(TEXT("landing_logged_d%d"),R->Night),FString::Printf(TEXT("At %s the array logged something low on the horizon coming down, about %.0f degrees from SARO, give or take %.0f."),*S47Hap::ClockText(Now.Clock),R->Az,R->Spread));
  if(Now.bSeen)Deed(TEXT("You watched the lights come down."));
 }
 EndEvent(Now.bSeen||bLogged);
}
void FS47Happenings::UseSite(const FString& Kind)
{
 FSite* S=SiteOf(Kind);if(!S||!M->UI||!M->Freeform)return;
 auto* F=M->Freeform.Get();
 TArray<S47Kit::FW> PanelRows;FString Title;
 const bool bFirst=!S->bFound;S->bFound=true;
 if(Kind==TEXT("landing"))
 {
  Title=TEXT("SCORCHED GROUND");
  PanelRows.Add(S47Kit::Text(TEXT("A ring of scorched ground about nine yards across, the caliche baked pale under the black. Four marks are pressed into it, square-sided, each deeper on the north side. Inside the ring the beetles lie dead on their backs and the creosote is brown on the side that faces the middle. Outside it nothing is touched.")));
  if(GeigerBuilt())PanelRows.Add(S47Kit::Text(TEXT("COUNTER / It climbs from the edge of the ring and pegs in the middle.")));
  const FString Key=FString::Printf(TEXT("lights_trace_landing_d%d"),S->Night);
  if(!F->HasFinding(Key))F->RecordEvidence(Key,FString::Printf(TEXT("A scorched ring with four marks pressed into it out by %s, dead insects and burnt brush inside it and nothing touched outside."),*S->Title));
  // Found: its bearing from SARO is exact now, and on the map.
  {double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);S->Az=S47Hap::Bearing(FS47Places::AzimuthOf(SaroX,SaroZ,S->X,S->Z));S->Spread=4;S->bKnown=true;}
  Deed(FString::Printf(TEXT("You found the scorched ring out by %s."),*S->Title));
 }
 else if(Kind==TEXT("rv"))
 {
  Title=TEXT("THE CAMPER");
  // Somebody is in it, and will not come out (the empty-car motif belongs to the rig on the CB and the district wagon).
  FString Page=TEXT("An old camper a long way from any road, every light on and a generator running under it. You knock. The blind in the door moves an inch and drops back. A man's voice, polite and tired, says he is fine, thank you, and would you please go away. A sharp chemical smell comes out of the vent, like a school lab shut up for the summer. On the dash, a short-brimmed hat. Tire tracks come in across the flat; none go out.");
  if(bFirst&&F->Places)
  {
   const FString Next=F->Places->NextUnknown(F->Day()+S->Serial);
   if(const FS47Places::FDef* D=Next.IsEmpty()?nullptr:FS47Places::Def(Next))
   {
    F->Places->Hear(Next);
    Page+=FString::Printf(TEXT(" Under the wiper, a page torn from a notebook: penciled bearings in a small, careful hand, one place circled: %s. It is on your map."),D->Title);
   }
  }
  PanelRows.Add(S47Kit::Text(Page));
  const FString Key=FString::Printf(TEXT("rv_d%d"),S->Night);
  if(!F->HasFinding(Key))F->RecordEvidence(Key,FString::Printf(TEXT("A camper stood out by %s with every light on and a generator running. A man inside would not open the door and asked me to go. A chemical smell from the vent. Tracks in, none out."),*S->Title));
  Deed(TEXT("You knocked on the door of the camper with every light on."));
 }
 else if(Kind==TEXT("carcass"))
 {
  Title=TEXT("DEAD COW");
  PanelRows.Add(S47Kit::Text(TEXT("A cow on her side in a ring of flattened grass. One ear is gone, and an eye, and a strip of hide along the jaw. The edges are smooth, almost sealed. There is no blood on the ground. No tracks come in and none go out, not even hers. The flies have left her alone.")));
  if(GeigerBuilt())PanelRows.Add(S47Kit::Text(TEXT("COUNTER / A little over background beside her, and in the ring.")));
  const FString Key=FString::Printf(TEXT("carcass_d%d"),S->Night);
  if(!F->HasFinding(Key))F->RecordEvidence(Key,FString::Printf(TEXT("A dead cow out by %s in a ring of flattened grass: clean cuts, no blood, no tracks in or out."),*S->Title));
  Deed(FString::Printf(TEXT("You found the dead cow out by %s."),*S->Title));
 }
 else if(Kind==TEXT("missing"))
 {
  if(S->bDone)return;
  Title=TEXT("BLUE PICKUP");
  // Not another empty car with the keys in it: a flat, a jack, and a man who walked. The ordinary answer can be the true one.
  PanelRows.Add(S47Kit::Text(TEXT("His pickup, nosed into the bank of the wash, the left front tire flat and the jack still under the bumper. The cab is shut and cold. His hat on the dash. A cane is gone from the rack behind the seat. His boot prints, one dragging, go off down the bottom of the wash toward the county road.")));
  const FString Key=FString::Printf(TEXT("missing_d%d"),S->Night);
  if(!F->HasFinding(Key))F->RecordEvidence(Key,FString::Printf(TEXT("The missing man's pickup out by %s: nosed into the wash with a flat, the jack under it, his prints going off down the wash toward the county road."),*S->Title));
  Deed(TEXT("You found the missing man's pickup."));
  // Called in at the next CB or the station's phone (StepReport): the county is looking for him.
  if(S->Variant!=TEXT("reported")){S->Variant=TEXT("noted");PanelRows.Add(S47Kit::Small(TEXT("You take down the plate. The county wants to hear of it: a CB or the station's phone.")));}
 }
 else if(Kind==TEXT("truck"))
 {
  if(S->bDone)return;
  if(S->Variant==TEXT("real"))
  {
   Title=TEXT("STUCK RIG");
   F->SpendTime(30*60.);const int32 Paid=F->EarnSide(15,TEXT("stuck rig"));
   FString Lead=TEXT("says the CB has been strange out this way lately, and will not say how.");
   if(F->Places)
   {
    const FString Next=F->Places->NextUnknown(F->Day()+S->Serial*3);
    if(const FS47Places::FDef* D=Next.IsEmpty()?nullptr:FS47Places::Def(Next)){F->Places->Hear(Next);Lead=FString::Printf(TEXT("says he saw a light sitting out by %s a few nights back. He did not go and look. It is on your map."),D->Title);}
   }
   PanelRows.Add(S47Kit::Text(TEXT("He has been at it with a tire iron. Between the two of you, his jack and a board off the trailer, it comes out in half an hour. ")+S47HapPay::RigPaid(Paid)+Lead));
   F->RecordEvidence(FString::Printf(TEXT("cb_helped_d%d"),S->Night),S47HapPay::RigFound(Paid,S->Title));
   Deed(TEXT("You dug a man's rig out of the sand."));
   S->bDone=true;
  }
  else if(S->Variant==TEXT("empty"))
  {
   Title=TEXT("EMPTY RIG");
   PanelRows.Add(S47Kit::Text(TEXT("The rig is down to the axles with the engine idling, the cab light on and the CB set to nineteen, hissing. His jacket over the seat, a cigarette burned down to the filter in the ashtray. Nobody. His boot prints go out to the fence and along it a way, and then there is only the fence.")));
   F->RecordEvidence(FString::Printf(TEXT("cb_empty_d%d"),S->Night),FString::Printf(TEXT("The driver who called for help by %s was not with his rig. Engine idling, the CB on, his prints going out along the fence and no further."),*S->Title));
   Deed(TEXT("You looked in the cab of an empty rig."));
  }
  else
  {
   Title=TEXT("NO RIG");
   PanelRows.Add(S47Kit::Text(TEXT("There is no rig. No tire tracks, no boot prints. On a fence post at head height a CB handset hangs by its coiled cord, the end of it wound around the wire. The handset is warm.")));
   F->RecordEvidence(FString::Printf(TEXT("cb_wrong_d%d"),S->Night),FString::Printf(TEXT("Nobody by %s, where the call said. No rig, no tracks: a CB handset hanging on a fence post by its cord, still warm."),*S->Title));
   Deed(TEXT("You found a handset hanging on a fence post."));
   S->bDone=true;
  }
 }
 if(PanelRows.IsEmpty())return;
 PanelRows.Add(S47Kit::Small(bFirst?TEXT("Kept as a field record."):TEXT("Already in your field records.")));
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening site %s (%s) looked at%s"),*Kind,*S->Variant,bFirst?TEXT(", first time"):TEXT(""));
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Col(PanelRows),760));
 M->RequestAutosave();
}

// ---------------------------------------------------------------- hitchhiker
bool FS47Happenings::BeginHitcher(bool bForced,FString& Why)
{
 double X=0,Z=0,TruckSpeed=0;
 if(!OnHighway(X,Z,TruckSpeed)){Why=TEXT("not on the highway");return false;}
 if(!bForced&&FMath::Abs(TruckSpeed)<8){Why=TEXT("not at speed");return false;}
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
 // Nothing is said on the screen: she is there in the lights, beside you through the window, or not.
 const auto Finish=[&](const FString& Page,const FString& Did)
 {
  F->RecordEvidence(FString::Printf(TEXT("hitcher_d%d"),F->Day()),Page);
  Deed(Did);
  if(M->Audio){FS47Sound S;S.Gain=.25f;S.Rate=.6f;S.Lp=900;M->Audio->Sfx(TEXT("doorCreak1"),S);}
  EndEvent(true);
 };
 // Met: seen standing there in the truck's lights (not only looked toward in the dark), or stopped for.
 if(!Now.bMet&&(Now.Stage>=1||(bHere&&Away<230&&SeenLit(Her+FVector(0,1.6,0),240,28,LitGround,TEXT("hitcher")))))MeetDanger();
 // Beside the player in the cab: low in the passenger seat, as much of her as shows through the glass, looking ahead.
 const auto InSeat=[&]
 {
  FS47Truck* T=M->World->CabTruck();if(!P||!T)return false;
  PoseProp(*P,T->LocalToWorld(FVector(.42,-.24,.2)),T->Rotation.Y+UE_DOUBLE_PI);ShowProp(*P,true);return true;
 };
 switch(Now.Stage)
 {
 case 0: // standing by the road, her hand out
  if(P){PoseProp(*P,Her,Now.Yaw);ShowProp(*P,bHere);}
  if(D&&Away<20&&FMath::Abs(D->Speed)<1.5)
  {
   Now.Stage=1;Now.Age=0;
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
 case 1: // the truck stopped beside her: she turns to the cab and looks in
  if(P){const FVector To=Truck-Her;PoseProp(*P,Her,FMath::Atan2(To.X,To.Z));ShowProp(*P,bHere);}
  if(Now.Age>7||Now.bSeen)
  {
   Now.Stage=2;Now.Age=0;
   // The passenger door, the seat taking her weight, and there she is beside you.
   if(M->Audio){FS47Sound S;S.Gain=.4f;S.At=Truck+FVector(1,1,0);M->Audio->Sfx(TEXT("doorMetal"),S);FS47Sound Seat=S;Seat.Gain=.18f;Seat.Rate=.7f;Seat.When=.9f;Seat.Lp=700;M->Audio->Sfx(TEXT("thudSoft"),Seat);}
   if(!InSeat()&&P)ShowProp(*P,false);
   Now.At=Truck;
   // While she sits there, getting out is getting out under it.
   Now.Watch=Now.Age+45;Now.Title=TEXT("the highway");
  }
  else if(D&&FMath::Abs(D->Speed)>4&&Away>25){Now.Stage=4;Now.Age=0;}
  break;
 case 2: // beside you, until a moment you do not see: the engine dies, and the seat is empty
  if(!InSeat()&&P)ShowProp(*P,false);
  if(Now.Stage==2&&((D&&FVector2D::Distance(FVector2D(Truck.X,Truck.Z),FVector2D(Now.At.X,Now.At.Z))>320)||Now.Age>45))
  {
   // The price of stopping for her: the engine dies, and by the dash clock twenty minutes have gone that nobody drove.
   if(P)ShowProp(*P,false);
   F->SpendTime(20*60.);
   if(D){FS47TroubleOpts O;O.bKey=true;M->World->Trouble.Start(ES47Trouble::Stall,O);}
   Finish(FString::Printf(TEXT("At %s a woman by the highway thumbed a ride and never said a word. She got in. Farther on the engine quit, the seat beside me was empty with the lock still down, and the dash clock was twenty minutes on."),*S47Hap::ClockText(Now.Clock)),TEXT("You stopped for the woman by the road."));
  }
  break;
 case 4: // gone in the mirror
  if(P){PoseProp(*P,Her,Now.Yaw+(Now.Age>.4?UE_DOUBLE_PI:0.));ShowProp(*P,bHere&&Now.Age<1.3);}
  if(Now.Age>2.2)
  {
   // Once more, further on, on the same shoulder: then not again.
   double Rx=0,Rz=0;
   if(!Now.bDone&&D&&RoadOfWorld(D->Pos,Rx,Rz))
   {
    const double Dir=Fwd.Z<0?-1.:1.;
    Now.bDone=true;Now.Stage=0;Now.Age=0;Now.Len=50;Now.X=-Dir*5.;Now.Z=Rz+Dir*150.;
    UE_LOG(LogS47Happen,Display,TEXT("S47: happening hitchhiker: on the shoulder ahead again"));
    break;
   }
   Finish(FString::Printf(TEXT("At %s a woman stood by the highway with her hand out%s. I did not stop. In the mirror, a moment later, the shoulder was empty: flat ground both sides, no brush, no ditch."),*S47Hap::ClockText(Now.Clock),Now.bDone?TEXT(", and further on there she was again"):TEXT("")),TEXT("You drove past the woman by the road."));
  }
  break;
 case 7: case 8: // out of the truck while she sat in it (StepWatch): the light came down over the player, or she is simply not there
  if(P)ShowProp(*P,false);
  Finish(FString::Printf(TEXT("At %s a woman by the highway thumbed a ride and never said a word. She got in. When I got out of the truck the seat was empty%s."),*S47Hap::ClockText(Now.Clock),Now.Stage==8?TEXT(", and there was a light over me"):TEXT("")),TEXT("You got out of the truck while she sat beside you."));
  break;
 default:EndEvent(false);break;
 }
 static_cast<void>(Dt);
}
void FS47Happenings::UseHitcher()
{
 if(Now.Id!=TEXT("hitchhiker")||Now.Stage>1)return;
 if(M->UI)M->UI->Toast(TEXT("She does not answer."),3);
 Now.bSeen=true;
}

// ---------------------------------------------------------------- stranded_rv
bool FS47Happenings::BeginRv(bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 if(const FSite* Old=SiteOf(TEXT("rv"))){Why=FString::Printf(TEXT("the camper from night %d is still out there"),Old->Night);return false;}
 if(const int32* LastRv=LastDays.Find(TEXT("stranded_rv"));!bForced&&LastRv&&*LastRv>F->Day()-6){Why=TEXT("too soon after the last one");return false;}
 using S47Hap::Bit;
 const double MinM=bForced?0:FMath::Max(250.,600.*NearK);
 FSpot S=Where(TEXT("stranded_rv"),Bit(ES47PlaceTrait::Far),Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::People)|Bit(ES47PlaceTrait::Indoors),MinM,0,1.f,Serial);
 if(!S.bSet)S=Where(TEXT("stranded_rv"),0,Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors),MinM,0,1.f,Serial+5);
 if(!S.bSet){Why=TEXT("nowhere far out enough");return false;}
 if(!bForced&&!MayStart(TEXT("stranded_rv"))){Why=TEXT("the rhythm says not now");return false;}
 const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x52565941u)*2*UE_DOUBLE_PI;
 FSite R;R.Kind=TEXT("rv");R.Place=S.Place;R.Title=S.Title;R.Night=F->Day();R.Serial=Serial+1;R.X=S.X+FMath::Cos(A)*35;R.Z=S.Z+FMath::Sin(A)*35;R.Yaw=A;
 R.Until=F->Clock()+2*86400.;
 Sites.Add(R);UsedPlace(S.Place,TEXT("stranded_rv"));
 if(CanHearCb()&&Now.Id.IsEmpty())
 {
  Sites.Last().bHeard=true;
  Cb({FString::Printf(TEXT("Breaker one-nine. Who's camping way out past %s? Every light burning, plain as day from the blacktop. Party I wasn't asked to, I guess."),*Spoken(S.Title))},TEXT("old"),false);
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
 if(!bForced&&MetTotal<=0&&F->TimesTaken()<=0){Why=TEXT("nothing real has come yet");return false;}
 // The small man at the pump comes last, once two of the others have been seen.
 int32 Others=0;for(const FString& H:HumorSeen)Others+=H!=TEXT("pump")?1:0;
 const bool bPumpYet=bForced||Others>=2;
 // Where: the diner's lot, the motel's pool, the pump in Caliche, or ahead on the highway.
 FString Kind=Variant;
 double Px=0,Pz=0;const bool bPlayer=PlayerRoad(Px,Pz);
 // The place of that name in the catalogue and how far the player is from it (a very long way: none).
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
 FSpot S;double Hx=0,Hz=0,TruckSpeed=0;const bool bRoad=OnHighway(Hx,Hz,TruckSpeed);
 const double Reach=bForced?0:120;
 if(Kind.IsEmpty())
 {
  if(bRoad&&FMath::Abs(TruckSpeed)>8)Kind=TEXT("crossing");
  else
  {
   // On foot outside, at whichever of the three is nearest: the diner and the pump are sixty metres apart.
   if(!Outdoors()||Driving()){Why=TEXT("not outside on foot");return false;}
   const TCHAR* const Kinds[]={TEXT("car"),TEXT("singer"),TEXT("pump")};
   const TCHAR* const Regions[]={TEXT("diner"),TEXT("motel"),TEXT("__none")};
   const TCHAR* const IdParts[]={TEXT("diner"),TEXT("motel"),TEXT("gas")};
   double Best=1e9;
   for(int32 I=0;I<3;++I)
   {
    if(I==2&&!bPumpYet)continue;
    FSpot Try;if(!Near(Regions[I],IdParts[I],Reach,Try))continue;
    const double Gap=FVector2D::Distance(FVector2D(Try.X,Try.Z),FVector2D(Px,Pz));
    if(Gap<Best){Best=Gap;S=Try;Kind=Kinds[I];}
   }
   if(Kind.IsEmpty()){Why=TEXT("not at the diner, the motel, the pump or on the highway");return false;}
  }
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
  Now.Len=20*60; // of the night's clock (StepHumor), not of the player's seconds
  if(bForced&&bPlayer&&M->Player){Now.X=Px-FMath::Sin(M->Player->Yaw)*(Kind==TEXT("car")?9.:5.);Now.Z=Pz-FMath::Cos(M->Player->Yaw)*(Kind==TEXT("car")?9.:5.);}
 }
 HumorNight=Night;
 if(!bForced&&!HumorSeen.Contains(Kind))HumorSeen.Add(Kind);
 UsedPlace(S.Place,TEXT("humor"));
 // Nothing is said: the car idles where it stands, the singer is heard from the lot (SiteSounds), the small man stands in the
 // station's lights.
 if(Kind==TEXT("car")){FVector At;if(M->Audio&&WorldOf(Now.X,Now.Z,At)){FS47Sound O;O.Gain=.18f;O.At=At;O.bLoop=true;M->Audio->LoopSfx(TEXT("hapEngine"),TEXT("engineIdle"),O);}}
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
  FVector Stride;WorldOf(FMath::Lerp(Side*9.,-Side*9.,T),Now.Z,Stride);
  const double Facing=Now.Age>=2.2&&Now.Age<3.6?(Now.Yaw<0?0.:UE_DOUBLE_PI):(Side>0?-UE_DOUBLE_PI/2:UE_DOUBLE_PI/2);
  if(P){PoseProp(*P,Stride,Facing);ShowProp(*P,bHere&&Now.Age<6.2);}
  if(Now.Stage==0&&Now.Age>6.4)
  {
   Now.Stage=1;
   F->RecordEvidence(FString::Printf(TEXT("humor_crossing_d%d"),F->Day()),FString::Printf(TEXT("At %s something tall and hairy crossed the highway in the headlights, stopped in the middle to look at the truck, and went on, a little faster."),*S47Hap::ClockText(Now.Clock)));
   Deed(TEXT("You slowed for something crossing the road."),false);
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
 // Gone once the player has had their look and gone off, or twenty minutes of the night on; never in front of them.
 const bool bOver=F->Clock()-Now.Clock>Now.Len;
 if((Now.Stage>=1&&Away>130)||(bOver&&(Away>130||!Seen(At,160,40))))EndEvent(Now.Stage>=2);
}
void FS47Happenings::UseHumor()
{
 if(Now.Id!=TEXT("humor")||!M->UI)return;
 auto* F=M->Freeform.Get();
 const bool bFirst=Now.Stage<2;Now.Stage=2;
 TArray<S47Kit::FW> PanelRows;FString Title;
 if(Now.Variant==TEXT("car"))
 {
  Title=TEXT("PINK CONVERTIBLE");
  PanelRows.Add(S47Kit::Text(TEXT("Top down, engine running, the radio on low to something with a lot of horns. On the seat, a diner napkin with BACK IN FIVE, DON'T TOUCH THE RADIO on it in lipstick. The keys hang from a ring shaped like a guitar. Fuzzy dice.")));
  if(bFirst)F->RecordEvidence(FString::Printf(TEXT("humor_car_d%d"),F->Day()),FString::Printf(TEXT("At %s a pink convertible sat outside the diner, top down, engine running, a note on the seat in lipstick: back in five."),*S47Hap::ClockText(F->Clock())));
  Deed(TEXT("You looked into the pink car at the diner."),false);
 }
 else if(Now.Variant==TEXT("singer"))
 {
  Title=TEXT("BY THE POOL");
  PanelRows.Add(S47Kit::Text(TEXT("A big man in a white suit with a high collar and dark glasses, at night, by a pool with no water in it. He sings to the deep end, slowly, and waves when he sees you. He says the pool sounds better empty. He thanks you for listening and presses five dollars on you before you can say no.")));
  if(bFirst){F->Earn(5);F->RecordEvidence(FString::Printf(TEXT("humor_singer_d%d"),F->Day()),FString::Printf(TEXT("At %s a man in a white suit and dark glasses sang to the empty motel pool, waved, and gave me five dollars for listening."),*S47Hap::ClockText(F->Clock())));}
  Deed(TEXT("You listened to the man singing at the empty pool."),false);
 }
 else if(Now.Variant==TEXT("pump"))
 {
  Title=TEXT("AT THE PUMP");
  PanelRows.Add(S47Kit::Text(TEXT("He is very polite. He would like to buy some water, please, if you have any. He holds out a ten-dollar bill so new it squeaks.")));
  const TWeakPtr<uint8> Guard=Life;
  const auto Done=[this,Guard](bool bSold)
  {
   if(!Guard.IsValid()||!M||!M->Freeform||!M->UI)return;
   auto* S=M->Freeform.Get();
   if(bSold&&S->Consume(TEXT("water")))S->Earn(10);else bSold=false;
   M->UI->Close(true);
   M->UI->Toast(bSold?TEXT("He thanks you, tips his hat, and walks off behind the station with the can."):TEXT("He tips his hat anyway and walks off behind the station."),6);
   S->RecordEvidence(FString::Printf(TEXT("humor_pump_d%d"),S->Day()),FString::Printf(TEXT("At %s a small gray man in a short-sleeved shirt and a hat wanted to buy water at the pump%s. He walked off behind the station."),*S47Hap::ClockText(S->Clock()),bSold?TEXT(", and paid ten dollars for a can"):TEXT("")));
   Deed(bSold?TEXT("You sold the small man at the pump a can of water."):TEXT("You told the small man at the pump you had no water."),false);
   if(Now.Id==TEXT("humor"))EndEvent(true);
  };
  PanelRows.Add(S47Kit::Button(TEXT("Sell him a can of water ($10)"),[Done]{Done(true);},true,TAttribute<bool>::CreateLambda([this,Guard]{return Guard.IsValid()&&M&&M->Freeform&&M->Freeform->InventoryCount(TEXT("water"))>0;})));
  PanelRows.Add(S47Kit::Button(TEXT("Tell him you have none"),[Done]{Done(false);}));
 }
 if(PanelRows.IsEmpty())return;
 UE_LOG(LogS47Happen,Display,TEXT("S47: happening humor %s looked at"),*Now.Variant);
 M->UI->Open(S47Kit::Shell(M->UI,Title,S47Kit::Col(PanelRows),720));
 M->RequestAutosave();
}

// ---------------------------------------------------------------- figure_near: on foot, something that comes nearer
// Tom's critics, 8 November: M1 had no danger for a player on foot. Out in the dark in their hours, from the second week: a figure
// on the open ground, dead still. Seen (in the lamp, the day or against the sky), it is not there; the next time it stands nearer,
// off to one side, then close behind. Then a light comes down over where it stood, and whoever is still out there under it is taken
// (S47Taken, as under the light over the truck). The truck, a roof, or a hard run away is the way out, and it is kept as such.
// Stages: 0 far, 1 gone, 2 nearer, 3 gone, 4 close, 5 the light coming, 6 the light down, 7 it went up, 8 taken, 9 got away.
bool FS47Happenings::BeginFigure(bool bForced,FString& Why)
{
 auto* F=M->Freeform.Get();
 if(!bForced&&(Driving()||!Outdoors())){Why=TEXT("not out on foot");return false;}
 if(!S47Hap::Exists(S47Hap::HairyStandIn)){Why=TEXT("no figure for it in this build");return false;}
 if(!bForced&&!MayStart(TEXT("figure_near"))){Why=TEXT("the rhythm says not now");return false;}
 // Tom's Nordic or Cosmic some nights, when this build has them; the shade otherwise (S47RoadGlimpse::FigureFor, the seed's).
 const FString Body=S47RoadGlimpse::FigureFor(F->Seed(),FS47FreeformSession::NightOf(F->Clock()),0x46494731u,[](const TCHAR* Path){return S47Hap::Exists(Path);});
 if(!Prop(S47Hap::ShadeKind(Body))||!Prop(TEXT("lander"))){Why=TEXT("nothing to draw it with");return false;}
 const FS47Cam Cam=FS47Cam::Of(M->Player);
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const FVector Fwd=FVector(Cam.Forward.X,0,Cam.Forward.Z).GetSafeNormal(),Side(-Fwd.Z,0,Fwd.X);
 // Off to one side of where the player looks, a long way out; nearer in the last weeks.
 const double Far=bForced?60:FMath::Clamp(170.*NearK,95.,170.);
 const double Off=(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x46494731u)<.5f?-1.:1.)*.6;
 FVector At=Feet+(Fwd*FMath::Cos(Off)+Side*FMath::Sin(Off))*Far;double X=0,Z=0;
 if(!RoadOfWorld(At,X,Z)||!WorldOf(X,Z,At)){Why=TEXT("no ground there");return false;}
 Now=FActive();Now.Id=TEXT("figure_near");Now.Variant=Body;Now.bDanger=true;Now.bForced=bForced;Now.Serial=Serial+1;Now.Clock=F->Clock();
 Now.At=At;Now.X=X;Now.Z=Z;Now.Dir=Feet;Now.Len=300;Now.Yaw=0;
 Now.Title=TEXT("the open ground");
 if(F->EventPlaces)if(const FS47EventPlace* P=F->EventPlaces->Nearest(500)){Now.Place=P->Id;Now.Title=FString::Printf(TEXT("the open ground near %s"),*P->Title);}
 FigureSeen.Reset();FigureSeenFor=0;FigureStageSeen=-1;
 // Before anything shows: the crickets stop.
 S47HapOmen::Forewarn(M,TEXT("happen:figure_near"),Eye(),&Now.At,0,.6,45);
 UsedPlace(Now.Place,TEXT("figure_near"));
 return true;
}
void FS47Happenings::StepFigure(double Dt)
{
 auto* F=M->Freeform.Get();
 US47Audio* A=M->Audio.Get();
 const FString Shade=S47Hap::ShadeKind(Now.Variant);
 FProp* P=Prop(Shade);FProp* Light=Prop(TEXT("lander"));
 const FS47Cam Cam=FS47Cam::Of(M->Player);
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const FVector Fwd=FVector(Cam.Forward.X,0,Cam.Forward.Z).GetSafeNormal(),Side(-Fwd.Z,0,Fwd.X);
 const bool bOut=!Driving()&&Outdoors();
 const double Away=AwayFrom(Now.At);
 // Kept only when the player met it: what was seen, in order, or the steps when nothing was; then how it ended.
 const auto Record=[&](const FString& End)
 {
  if(!Now.bMet&&FigureSeen.IsEmpty())return;
  F->RecordEvidence(FString::Printf(TEXT("figure_near_d%d"),F->Day()),FString::Printf(TEXT("At %s, out on %s: %s%s"),*S47Hap::ClockText(Now.Clock),*Now.Title,FigureSeen.IsEmpty()?TEXT("I saw nothing. There were steps in the gravel, and then one close behind me."):*FString::Join(FigureSeen,TEXT(" ")),End.IsEmpty()?TEXT(""):*(TEXT(" ")+End)));
 };
 const auto Over=[&](bool bGot,const FString& End)
 {
  if(P)ShowProp(*P,false);if(Light)ShowProp(*Light,false);
  if(M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,TEXT("figure"));
  Record(End);
  if(bGot)Deed(TEXT("You got away from something out in the dark."));
  UE_LOG(LogS47Happen,Display,TEXT("S47: happening figure_near: %s, %.0f m from where it stood"),bGot?TEXT("got away"):TEXT("over"),Away);
  EndEvent(true);
 };
 // Out of it: into the truck, under a roof, or a long way from where it began. What the player did is the end of it.
 if(Now.Stage<6&&(!bOut||FVector2D::Distance(FVector2D(Feet.X,Feet.Z),FVector2D(Now.Dir.X,Now.Dir.Z))>320))
 {
  Now.Stage=9;
  Over(Now.bMet,Driving()?TEXT("I got into the truck and did not look back."):!Outdoors()?TEXT("I got under a roof and shut the door."):TEXT("I walked away fast and kept walking."));
  return;
 }
 if(Now.Age>Now.Len){Over(false,FString());return;}
 const auto Stand=[&](double Dist,double FromForward)
 {
  // Out of the view, at an angle from where the player looks now: they have to turn to find it.
  const FVector Dir=Fwd*FMath::Cos(FromForward)+Side*FMath::Sin(FromForward);
  FVector At=Feet+Dir*Dist;double X=0,Z=0;
  if(!RoadOfWorld(At,X,Z)||!WorldOf(X,Z,At))return false;
  Now.At=At;Now.X=X;Now.Z=Z;return true;
 };
 const bool bStanding=Now.Stage==0||Now.Stage==2||Now.Stage==4;
 if(P){const FVector To=Feet-Now.At;PoseProp(*P,Now.At,FMath::Atan2(To.X,To.Z));ShowProp(*P,bStanding);}
 if(bStanding)
 {
  // The lamp turned on it, or seen in fact: a moment, and it is not there. Nearer each time, so it comes the sooner.
  const bool bLamp=M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn&&Seen(Now.At+FVector(0,1,0),400,7);
  const bool bSeen=SeenLit(Now.At+FVector(0,1.6,0),340,30,LitGround|LitSky,*Shade);
  FigureSeenFor=bSeen||bLamp?FigureSeenFor+Dt:FMath::Max(0.,FigureSeenFor-Dt*.5);
  if(bSeen&&FigureStageSeen!=Now.Stage)
  {
   // What was there, where and how far: raw, in the order it came.
   FigureStageSeen=Now.Stage;
   const FString Way=DirectionFrom(Feet,Now.At);
   FigureSeen.Add(FigureSeen.IsEmpty()?FString::Printf(TEXT("a figure stood to the %s, about %.0f metres off, dead still."),*Way,Away):FString::Printf(TEXT("Then it stood to the %s, about %.0f metres off."),*Way,Away));
   Noticed(TEXT("figure_near"),Away,TEXT("sight"));
   MeetDanger();
  }
  if(Now.Stage==4&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(.75,TEXT("figure"));
  const double Wait=Now.Stage==0?45:Now.Stage==2?35:7;
  if(FigureSeenFor>.6||(bLamp&&FigureSeenFor>.2)||Now.Age-Now.Yaw>Wait||Away<(Now.Stage==4?6:14))
  {
   FigureSeenFor=0;
   if(Now.Stage==4)
   {
    // The light: high over where it stood, coming down. A few seconds to be somewhere else.
    Now.Stage=5;Now.Yaw=Now.Age;
    if(A){if(A->Night)A->Night->Hush(30,.02,.2,3.);A->Boom(.25f);}
    if(M->Night&&M->Night->Flashlight)M->Night->Flashlight->FlickerOut(1.2);
    UE_LOG(LogS47Happen,Display,TEXT("S47: happening figure_near: the light comes down, %.0f m from the player"),Away);
   }
   else{++Now.Stage;Now.Yaw=Now.Age;}
  }
  return;
 }
 if(Now.Stage==1||Now.Stage==3)
 {
  // Gone a while, then nearer: steps in the gravel off to one side first, then one step close behind.
  if(Now.Age-Now.Yaw<(Now.Stage==1?9:6))return;
  const double Dist=Now.Stage==1?FMath::Clamp(70.*NearK+30.,45.,70.):22.;
  const double Angle=Now.Stage==1?(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x46494732u)<.5f?-1.:1.)*1.9:UE_DOUBLE_PI*(.9+.2*FS47NightRhythm::Roll(F->Seed(),F->Day(),0x46494733u));
  if(!Stand(Dist,Angle))
  {
   if(Now.Stage==1)Over(false,FString());else{Now.Stage=9;Over(Now.bMet,TEXT("There was nothing behind me when I turned."));}
   return;
  }
  ++Now.Stage;Now.Yaw=Now.Age;
  if(A)
  {
   const int32 Steps=Now.Stage==2?3:1;
   for(int32 K=0;K<Steps;++K){FS47Sound O;O.At=Now.At+FVector(0,.1,0);O.Gain=Now.Stage==2?.35f:.5f;O.When=.55f*K;O.Lp=2400;A->Sfx(FString::Printf(TEXT("stepGravel%d"),(K+Now.Stage)%5),O);}
  }
  // Close behind, and heard: it has come for the player, the night's danger met (counted now, the rest after it, stress).
  if(Now.Stage==4)MeetDanger();
  return;
 }
 if(Now.Stage==5||Now.Stage==6)
 {
  // The light over where it stood, coming down from a hundred metres in six seconds, and standing there a few more.
  const double T=FMath::Clamp((Now.Age-Now.Yaw)/6.,0.,1.);
  if(Light){PoseProp(*Light,Now.At+FVector(0,FMath::Lerp(110.,14.,T),0),Now.Age*.2);ShowProp(*Light,true);}
  if(P)ShowProp(*P,false);
  if(Now.Stage==5&&T>=1)
  {
   // Still out on foot under it: it watches, and the player is taken (StepWatch). Far enough off already: got away.
   if(bOut&&Away<75){Now.Stage=6;Now.Watch=Now.Age+4;}
   else{Now.Stage=9;Over(true,TEXT("A light came down where it had stood. I was a long way off by then."));return;}
  }
  if(Now.Stage==6&&Now.Age>Now.Watch){Now.Stage=9;Over(true,TEXT("A light came down where it had stood. I was out from under it."));}
  return;
 }
 // Taken (8) or the rules would not have it and it went up (7): over.
 if(Now.Stage==7||Now.Stage==8)Over(false,Now.Stage==8?TEXT("A light came down over me."):TEXT("A light came down where it stood, and went up again."));
}

// ---------------------------------------------------------------- the rest of NATTHENDELSER-31
// 2 dogs, 6 a phone in an empty building, 10 a carcass, 12 a lit window in an empty house, 14 a figure on the rise, 15 the cinema,
// 16 room 6, 17 Caliche's lights, 18 the array's burst, 19 the caller on 1420, 20 the warm hood, 21 tracks round the truck,
// 22 animals running, 24 a light over the truck (a danger), 25 a dust devil, 26 the missing man. Each leaves a record or a lead.
namespace S47HapTiers
{
 // TIERS-20261109: the moment holds a glimpse but not yet a full meeting (S47NightTiers, on the night the clock belongs to).
 bool GlimpseOnly(double Clock){return S47NightTiers::AtClock(Clock,FS47FreeformSession::NightOf(Clock))==ES47Allowed::Glimpse;}
 // How near the player comes before a glimpse on the horizon is gone (a full night's figure waits until 90 m).
 constexpr double GlimpseGoneAt=130.;
}
bool FS47Happenings::BeginMore(const FString& Id,const FString& Variant,bool bForced,FString& Why)
{
 using namespace S47HapTiers;
 static_cast<void>(Variant);
 auto* F=M->Freeform.Get();
 const double Clock=F->Clock();
 const FS47Cam Cam=FS47Cam::Of(M->Player);
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const FVector Fwd=FVector(Cam.Forward.X,0,Cam.Forward.Z).GetSafeNormal(),Side(-Fwd.Z,0,Fwd.X);
 const bool bDrive=Driving(),bOut=Outdoors()&&!bDrive,bIn=!Outdoors()&&!bDrive;
 const FString Area=M->Web->Area();
 const auto No=[&Why](const TCHAR* Text){Why=Text;return false;};
 const auto Arm=[&](const TCHAR* Kind,double Len)
 {
  Now=FActive();Now.Id=Kind;Now.Len=Len;Now.bForced=bForced;Now.Serial=Serial+1;Now.Clock=Clock;
 };
 // The catalogue's place nearest the player with these traits, in this area: false when none is within reach.
 const auto Near=[&](ES47PlaceTrait Need,double Within,FString& Place,FString& Title,FVector& At)
 {
  if(!F->EventPlaces)return false;
  const FS47EventPlace* P=F->EventPlaces->Nearest(Within,Need);
  if(!P||!F->EventPlaces->EntryNow(P->Id,At))return false;
  Place=P->Id;Title=P->Title;return true;
 };
 const auto Ask=[&](const TCHAR* Kind){return bForced||MayStart(Kind);};
 using S47Hap::Bit;
 if(Id==TEXT("dogs_hush"))
 {
  if(!bOut&&!bForced)return No(TEXT("not outside"));
  FString Place,Title;FVector At;
  if(!Near(ES47PlaceTrait::Farm,450,Place,Title,At)&&!Near(ES47PlaceTrait::Town,450,Place,Title,At))
  {
   if(!bForced)return No(TEXT("no farm or town within earshot"));
   At=Feet+Fwd*200;Title=TEXT("the dark");
  }
  if(!Ask(TEXT("dogs_hush")))return No(TEXT("the rhythm says not now"));
  Arm(TEXT("dogs_hush"),10);Now.At=At;Now.Place=Place;Now.Title=Title;
  return true;
 }
 if(Id==TEXT("phone_empty"))
 {
  if(bDrive&&!bForced)return No(TEXT("in the truck"));
  FString Place,Title;FVector At;
  if(!Near(ES47PlaceTrait::Phone|ES47PlaceTrait::Town,bForced?2000:140,Place,Title,At))
  {
   if(!bForced)return No(TEXT("no phone in an empty building near"));
   At=Feet+Fwd*6;Title=TEXT("the building");
  }
  if(!Ask(TEXT("phone_empty")))return No(TEXT("the rhythm says not now"));
  if(!Prop(TEXT("phone")))return No(TEXT("nothing to draw it with"));
  Arm(TEXT("phone_empty"),60);Now.At=At+FVector(0,1.2,0);Now.Place=Place;Now.Title=Title;
  return true;
 }
 if(Id==TEXT("carcass"))
 {
  if(HasSite(TEXT("carcass"))){Why=TEXT("one lies out there already");return false;}
  FSpot S=Where(TEXT("carcass"),0,Bit(ES47PlaceTrait::Farm)|Bit(ES47PlaceTrait::Open),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors),bForced?0:FMath::Max(100.,250.*NearK),0,.5f*float(NearK),Serial);
  if(!S.bSet)return No(TEXT("no open range for it"));
  if(!Ask(TEXT("carcass")))return No(TEXT("the rhythm says not now"));
  const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x43524353u)*2*UE_DOUBLE_PI;
  FSite C;C.Kind=TEXT("carcass");C.Place=S.Place;C.Title=S.Title;C.Night=F->Day();C.Serial=Serial+1;C.X=S.X+FMath::Cos(A)*25;C.Z=S.Z+FMath::Sin(A)*25;C.Yaw=A;C.Until=Clock+2*86400.;
  Sites.Add(C);UsedPlace(S.Place,TEXT("carcass"));
  // Heard of on the CB, or from Dale over the walkie when the player is on foot; otherwise talked of the next evening (StepSites).
  if(Now.Id.IsEmpty()&&(CanHearCb()||(M->FieldRadio.IsValid()&&M->FieldRadio->Owned())))
  {
   Sites.Last().bHeard=true;
   Tip({FString::Printf(TEXT("Anybody on this channel know a vet who'll come out at night? Got a cow down out by %s and I don't like the look of her."),*Spoken(S.Title))},TEXT("old"),
    FString::Printf(TEXT("A rancher has a cow down out by %s and wants a vet. He does not like the look of her."),*Spoken(S.Title)));
  }
  return true;
 }
 if(Id==TEXT("house_lit"))
 {
  if(HasSite(TEXT("houselit"))){Why=TEXT("one is lit already");return false;}
  if(!bOut&&!bDrive&&!bForced)return No(TEXT("indoors"));
  if(!F->EventPlaces)return No(TEXT("no place catalogue"));
  // An empty house: windows, nobody living there, on this ground and in sight.
  const FS47EventPlace* Best=nullptr;double BestScore=-1;
  for(const FS47EventPlace& P:F->EventPlaces->All())
  {
   if(!P.Has(ES47PlaceTrait::Window)||!P.Has(ES47PlaceTrait::Indoors)||P.Has(ES47PlaceTrait::People))continue;
   // Critics, round 8: not the old house on the Alderete place while its own timer has its lamp on anyway (S47FarmLights: dusk to
   // 22:30); a light there then is nobody's mystery.
   if(P.Id==TEXT("oldhouse")&&S47FarmLights::HouseTimerOn(S47Hap::HourOf(Clock)))continue;
   FVector At;if(!F->EventPlaces->EntryNow(P.Id,At))continue;
   const FVector To(At.X-Feet.X,0,At.Z-Feet.Z);const double Far=To.Size();
   if(Far<(bForced?20:120)||Far>900)continue;
   const double Score=FVector::DotProduct(To/Far,Fwd)+FS47NightRhythm::Roll(F->Seed(),F->Day(),S47Hap::SaltOf(P.Id))*.5;
   if(Score>BestScore){BestScore=Score;Best=&P;}
  }
  if(!Best)return No(TEXT("no empty house in sight"));
  if(!Ask(TEXT("house_lit")))return No(TEXT("the rhythm says not now"));
  FSite H;H.Kind=TEXT("houselit");H.Place=Best->Id;H.Title=Best->Title;H.Night=F->Day();H.Serial=Serial+1;H.X=Best->EntryX;H.Z=Best->EntryZ;
  H.Yaw=FMath::Atan2(Best->X-Best->EntryX,Best->Z-Best->EntryZ)+UE_DOUBLE_PI;
  // Until half past five in the morning of this night.
  H.Until=FS47FreeformSession::NightOf(Clock)*86400.+5.5*3600.;
  Sites.Add(H);UsedPlace(Best->Id,TEXT("house_lit"));
  return true;
 }
 if(Id==TEXT("horizon_figure"))
 {
  if(!bOut&&!bForced)return No(TEXT("not out on foot"));
  if(!S47Hap::Exists(S47Hap::HairyStandIn))return No(TEXT("no figure for it in this build"));
  if(!Ask(TEXT("horizon_figure")))return No(TEXT("the rhythm says not now"));
  // Tom's Nordic or Cosmic on the rise some nights, when this build has them; the shade otherwise. F10's Variant: one of them, or shade.
  const FString Body=S47RoadGlimpse::FigureFor(F->Seed(),FS47FreeformSession::NightOf(Clock),0x484f525au,[](const TCHAR* Path){return S47Hap::Exists(Path);},Variant);
  if(!Prop(S47Hap::ShadeKind(Body)))return No(TEXT("nothing to draw it with"));
  // On the rise at the edge of the view, a long way off; nearer as the weeks go. Before the full tier (TIERS-20261109: a glimpse, from
  // midnight) it keeps farther off, never nearer than 170 m, and is gone sooner when the player comes (GlimpseGoneAt).
  const bool bGlimpse=GlimpseOnly(Clock);
  const double Off=bForced?.15:.5,Far=bForced?110:FMath::Max(FMath::Clamp(190.*NearK,100.,190.),bGlimpse?170.:0.);
  FVector At=Feet+(Fwd*FMath::Cos(Off)+Side*FMath::Sin(Off))*Far;
  double X=0,Z=0;
  if(!RoadOfWorld(At,X,Z)||!WorldOf(X,Z,At))return No(TEXT("no ground there"));
  Arm(TEXT("horizon_figure"),120);Now.Variant=Body;Now.At=At;Now.X=X;Now.Z=Z;
  return true;
 }
 if(Id==TEXT("road_glimpse"))
 {
  // GLIMPSE-ROAD-20261109 (S47RoadGlimpse): at the wheel on the highway, something crosses far ahead in the lights and is gone before
  // the truck comes up. The rhythm holds it to the glimpse tier and to once a week; which body, which side and how far are the seed's.
  double Hx=0,Hz=0,Speed=0;
  if(!OnHighway(Hx,Hz,Speed))return No(TEXT("not on the highway"));
  if(!bForced&&FMath::Abs(Speed)<8)return No(TEXT("the truck is hardly moving"));
  if(!bForced&&M->World->Trouble.Dim<=.5)return No(TEXT("the truck's lights are out"));
  if(!Ask(TEXT("road_glimpse")))return No(TEXT("the rhythm says not now"));
  const S47RoadGlimpse::FPlan Way=S47RoadGlimpse::PlanFor(F->Seed(),FS47FreeformSession::NightOf(Clock),Speed,[](const TCHAR* Path){return S47Hap::Exists(Path);},Variant);
  if(Way.Id.IsEmpty()||!Prop(TEXT("glimpse_")+Way.Id))return No(TEXT("no figure for it in this build"));
  // Which way the road runs ahead of the truck, road-local (z along the highway): its own heading, measured ten metres on.
  const FS47DriveController* D=M->World->CabDrive();
  const FVector Ahead(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading));
  double Dir=Ahead.Z<0?-1.:1.;
  {double Ax=0,Az=0;if(RoadOfWorld(D->Pos+Ahead*10.,Ax,Az)&&!FMath::IsNearlyEqual(Az,Hz,.5))Dir=Az<Hz?-1.:1.;}
  Arm(TEXT("road_glimpse"),S47RoadGlimpse::CrossSeconds(Way)+.5);
  Glimpse=Way;Now.Variant=Way.Id;Now.X=0;Now.Z=Hz+Dir*Way.AheadM;Now.Yaw=Dir;Now.Title=TEXT("the highway");
  WorldOf(Now.X,Now.Z,Now.At);
  return true;
 }
 if(Id==TEXT("cinema_film"))
 {
  if(HasSite(TEXT("cinema"))){Why=TEXT("it is running already");return false;}
  const FS47EventPlace* C=F->EventPlaces?F->EventPlaces->Find(TEXT("cinema")):nullptr;if(!C)return No(TEXT("no cinema in the catalogue"));
  if(!Ask(TEXT("cinema_film")))return No(TEXT("the rhythm says not now"));
  FSite S;S.Kind=TEXT("cinema");S.Place=C->Id;S.Title=C->Title;S.Night=F->Day();S.Serial=Serial+1;S.X=C->EntryX;S.Z=C->EntryZ;S.Until=Clock+45*60;
  Sites.Add(S);UsedPlace(C->Id,TEXT("cinema_film"));
  return true;
 }
 if(Id==TEXT("room6_tv"))
 {
  FString Place,Title;FVector At;
  if(!Near(ES47PlaceTrait::Lot|ES47PlaceTrait::Road,bForced?3000:160,Place,Title,At)||!Place.Contains(TEXT("motel")))
  {
   const FS47EventPlace* Mo=F->EventPlaces?F->EventPlaces->Find(TEXT("motel")):nullptr;
   if(!Mo||!F->EventPlaces->EntryNow(Mo->Id,At)||(!bForced&&FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(Feet.X,Feet.Z))>160))return No(TEXT("not at the motel"));
   Place=Mo->Id;Title=Mo->Title;
  }
  if(!Ask(TEXT("room6_tv")))return No(TEXT("the rhythm says not now"));
  if(!Prop(TEXT("tvlight")))return No(TEXT("nothing to draw it with"));
  Arm(TEXT("room6_tv"),180);Now.At=At+FVector(0,1,0);Now.Place=Place;Now.Title=Title;
  if(M->Audio)M->Audio->TvHiss(Now.At);
  return true;
 }
 if(Id==TEXT("town_blackout"))
 {
  double X=0,Z=0;
  const bool bTown=PlayerRoad(X,Z)&&Z>1800&&Z<2450&&FMath::Abs(X)<320&&(Area==TEXT("road")||Area==TEXT("diner"));
  if(!bTown&&!bForced)return No(TEXT("not in Caliche"));
  FS47Town* T=M->TownLayer?M->TownLayer->Town.Get():nullptr;if(!T)return No(TEXT("no town here"));
  if(!Ask(TEXT("town_blackout")))return No(TEXT("the rhythm says not now"));
  T->LampWave(M->T);
  if(M->Audio){M->Audio->Beep(60.f,40.f,.05f,1.f,TEXT("sine"));M->Audio->Beep(120.f,40.f,.025f,1.f,TEXT("square"));}
  F->RecordEvidence(FString::Printf(TEXT("blackout_d%d"),F->Day()),FString::Printf(TEXT("At %s every light in Caliche went out, one after another down the street, and the transformer hummed loud enough to hear across town. Then they came back."),*S47Hap::ClockText(Clock)));
  Deed(TEXT("You stood in Caliche when the lights went out."));
  Arm(TEXT("town_blackout"),30);
  return true;
 }
 if(Id==TEXT("array_burst"))
 {
  // Only at the monitor: in the SARO building, not out on the farm that shares its area.
  if(!bForced&&(bDrive||!FS47FieldRadio::InSaroBuilding(M)))return No(TEXT("not in the SARO building"));
  // LOOP-4-20261109: the night's one pointing-out at the land, shared with the bearing slip and a logged landing.
  const int32 PointNight=FS47FreeformSession::NightOf(Clock);
  if(!bForced&&F->Director&&!F->Director->MayPointOut(PointNight))return No(TEXT("the night has pointed out at the land already"));
  if(!Ask(TEXT("array_burst")))return No(TEXT("the rhythm says not now"));
  // Off the main beam, toward somewhere nobody has looked at yet: which way, never what.
  FString Target;double ToX=0,ToZ=0;bool bHave=false;
  if(F->Places){Target=F->Places->NextUnknown(F->Day()+Serial);bHave=!Target.IsEmpty()&&F->Places->Spot(Target,ToX,ToZ);}
  if(!bHave){const FSpot S=Where(TEXT("array_burst"),Bit(ES47PlaceTrait::Far),0,0,0,0,1.f,Serial);if(!S.bSet)return No(TEXT("nowhere for it to point"));Target=S.Place;ToX=S.X;ToZ=S.Z;}
  double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
  const int32 B=Branches();const double Spread=B>0?FS47Places::SpreadFor(B):18.;
  const double Az=S47Hap::Bearing(FS47Places::AzimuthOf(SaroX,SaroZ,ToX,ToZ)+(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x42525354u)-.5)*Spread);
  FSite S;S.Kind=TEXT("burst");S.Place=Target;S.Title=Target;S.Night=F->Day();S.Serial=Serial+1;S.X=ToX;S.Z=ToZ;S.Az=Az;S.Spread=Spread;S.Until=Clock+4*86400.;
  Sites.RemoveAll([](const FSite& Old){return Old.Kind==TEXT("burst");});Sites.Add(S);
  if(!bForced)TakePointOut(F->Director.Get(),PointNight);
  if(F->Places&&FS47Places::Def(Target))F->Places->Hear(Target);
  if(M->Audio)
  {
   for(int32 K=0;K<6;++K)M->Audio->Beep(1400.f,.05f,.06f,.12f*K,TEXT("square"));
   FS47Sound P;P.At=FVector(-4,1,2);P.Gain=.3f;P.When=1.f;const FS47Play Line=M->Audio->Sfx(TEXT("printer"),P);
   if(Line&&M->Audio->Ctx.IsValid())Line.Src->Stop(M->Audio->Ctx->CurrentTime()+3.);
  }
  Say(FString::Printf(TEXT("ARRAY / Off the main beam: about %.0f degrees. On your map (TAB)."),Az),.8,5);
  F->RecordEvidence(FString::Printf(TEXT("array_burst_d%d"),F->Day()),FString::Printf(TEXT("At %s the array caught a short burst off the main beam, about %.0f degrees from SARO, give or take %.0f. The printer ran a line by itself."),*S47Hap::ClockText(Clock),Az,Spread));
  Arm(TEXT("array_burst"),5);
  return true;
 }
 if(Id==TEXT("am_caller"))
 {
  if(!bDrive)return No(TEXT("not in the truck"));
  if(!bForced&&!(M->CarRadio&&M->CarRadio->Live()&&M->CarRadio->Station==TEXT("caliche")))return No(TEXT("the radio is not on 1420"));
  if(!M->Radio||!M->Story)return No(TEXT("no radio"));
  const auto* St=M->Radio->Stations.Find(TEXT("caliche"));if(!St||!St->IsValid())return No(TEXT("no 1420 here"));
  // What you saw, or did, last night, told back wrong: another place, another colour, another hour.
  FString Wrong=TEXT("one of the people from out at the dishes was standing on the roof of their truck by the water tank, at about four");
  if(const FSite* L=SiteOf(TEXT("landing")))
  {
   FString Other=TEXT("the old mine");
   if(F->EventPlaces){const TArray<FString> PlaceIds=FS47EventPlaces::Ids();if(PlaceIds.Num())Other=F->EventPlaces->TitleOf(PlaceIds[int32(FS47NightRhythm::Roll(F->Seed(),F->Day(),0x57524f4eu)*PlaceIds.Num())%PlaceIds.Num()]);}
   if(Other==L->Title)Other=TEXT("the old mine");
   Wrong=FString::Printf(TEXT("green lights came down out by %s around nine, three of them, and went back up"),*Other);
  }
  else if(!Deeds.IsEmpty())Wrong=TEXT("somebody from the dishes was out walking the highway with no flashlight, at about four in the morning");
  TArray<FS47RadioLine> Lines;
  Lines.Add(FS47RadioLine(FString::Printf(TEXT("Earl, I don't want to make trouble, but last night %s. I saw it myself."),*Wrong),TEXT("woman"),TEXT("a caller")));
  Lines.Add(FS47RadioLine(TEXT("Well. Lot of people seeing things this spring.")));
  if(!(*St)->Slot(Lines,M->Story->Clock()))return No(TEXT("not the caller's turn on the program"));
  if(F->Places){const FString Next=F->Places->NextUnknown(F->Day()*3+Serial);if(!Next.IsEmpty())F->Places->Hear(Next);}
  return true;
 }
 if(Id==TEXT("hood_warm")||Id==TEXT("truck_tracks"))
 {
  FS47Truck* T=M->World->TruckHere();if(!T||bDrive)return No(TEXT("not by the parked truck"));
  const double Gap=FVector2D::Distance(FVector2D(T->Position.X,T->Position.Z),FVector2D(Feet.X,Feet.Z));
  // Its own stamp, set as the player comes back within reach (StepUnease): a minute to come up to it, after twenty away.
  if(!bForced&&(Gap>9||Paced-ReturnedAt>60||ReturnedAway<20*60))return No(TEXT("not back at the truck after a while away"));
  double X=0,Z=0;RoadOfWorld(T->Position,X,Z);
  if(Id==TEXT("truck_tracks"))
  {
   if(!bForced&&(Area!=TEXT("road")||FMath::Abs(X)<20))return No(TEXT("the truck is not out on the sand"));
   if(!Ask(TEXT("truck_tracks")))return No(TEXT("the rhythm says not now"));
   FSite S;S.Kind=TEXT("tracks");S.Title=TEXT("the truck");S.Night=F->Day();S.Serial=Serial+1;S.X=X;S.Z=Z;S.Yaw=T->Rotation.Y;S.Until=Clock+8*3600.;S.bFound=true;
   Sites.RemoveAll([](const FSite& Old){return Old.Kind==TEXT("tracks");});Sites.Add(S);
   F->RecordEvidence(FString::Printf(TEXT("truck_tracks_d%d"),F->Day()),FString::Printf(TEXT("At %s, back at the truck: small bare prints all the way around it, close to the doors. They do not come from anywhere or go anywhere."),*S47Hap::ClockText(Clock)));
   Deed(TEXT("You found small prints all around the truck."));
   return true;
  }
  if(!Ask(TEXT("hood_warm")))return No(TEXT("the rhythm says not now"));
  HoodNight=F->Day();
  // The engine ticking as it cools, as it does just after a drive: the truck has been sitting a good while.
  if(M->Audio){FS47Sound O;O.At=T->Position+FVector(0,1,0);O.Gain=.55f;M->Audio->Sfx(Synth(TEXT("tick")),O);FS47Sound O2=O;O2.When=2.6f;O2.Gain=.4f;O2.Rate=.9f;M->Audio->Sfx(Synth(TEXT("tick")),O2);}
  F->RecordEvidence(FString::Printf(TEXT("hood_warm_d%d"),F->Day()),FString::Printf(TEXT("At %s the truck had been sitting a good while, and its engine was ticking as it cooled when I came back to it. The hood was warm to the hand."),*S47Hap::ClockText(Clock)));
  Deed(TEXT("You came back to the truck."));
  return true;
 }
 if(Id==TEXT("animals_flee"))
 {
  if(!bOut&&!bForced)return No(TEXT("not out on foot"));
  if(!Ask(TEXT("animals_flee")))return No(TEXT("the rhythm says not now"));
  if(!Prop(TEXT("rabbits")))return No(TEXT("nothing to draw it with"));
  // From behind on one side, past, and away on the other: all one way.
  Arm(TEXT("animals_flee"),7);
  Now.Dir=(Fwd*.6+Side*.8).GetSafeNormal();Now.At=Feet-Now.Dir*40+Fwd*6;
  return true;
 }
 if(Id==TEXT("car_light"))
 {
  FS47Truck* T=bDrive?M->World->CabTruck():nullptr;const FS47DriveController* D=bDrive?M->World->CabDrive():nullptr;
  if(!T||!D)return No(TEXT("not in the truck"));
  if(!Ask(TEXT("car_light")))return No(TEXT("the rhythm says not now"));
  if(!Prop(TEXT("lander")))return No(TEXT("nothing to draw it with"));
  Arm(TEXT("car_light"),24);Now.bDanger=true;Now.At=D->Pos;Now.Title=TEXT("the road");
  // Met as it comes down over the cab; for as long as it stands there, getting out is getting out under it (StepWatch).
  Now.Watch=Now.Len-1;MeetDanger();
  FS47TroubleOpts O;O.bKey=true;M->World->Trouble.Start(ES47Trouble::Stall,O);
  if(M->Audio)M->Audio->Boom(.5f);
  return true;
 }
 if(Id==TEXT("dust_devil"))
 {
  if(!bOut&&!bForced)return No(TEXT("not out on foot"));
  if(!Ask(TEXT("dust_devil")))return No(TEXT("the rhythm says not now"));
  if(!Prop(TEXT("devil")))return No(TEXT("nothing to draw it with"));
  const double Off=(FS47NightRhythm::Roll(F->Seed(),int32(Clock/60.),0x44445645u)-.5)*.8,Far=bForced?40:FMath::Clamp(90.*NearK,45.,90.);
  FVector At=Feet+(Fwd*FMath::Cos(Off)+Side*FMath::Sin(Off))*Far;double X=0,Z=0;
  if(!RoadOfWorld(At,X,Z)||!WorldOf(X,Z,At))return No(TEXT("no ground there"));
  Arm(TEXT("dust_devil"),120);Now.At=At;Now.X=X;Now.Z=Z;
  return true;
 }
 if(Id==TEXT("missing_person"))
 {
  if(HasSite(TEXT("missing"))){Why=TEXT("his pickup is still out there");return false;}
  FSpot S=Where(TEXT("missing_person"),Bit(ES47PlaceTrait::Far),Bit(ES47PlaceTrait::Open)|Bit(ES47PlaceTrait::Road)|Bit(ES47PlaceTrait::Wash),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro),bForced?0:FMath::Max(250.,500.*NearK),0,1.f,Serial);
  if(!S.bSet)return No(TEXT("nowhere far out"));
  if(!bForced&&!CanHearCb()&&!(M->FieldRadio.IsValid()&&M->FieldRadio->Owned()))return No(TEXT("nothing to hear the county on"));
  if(!Ask(TEXT("missing_person")))return No(TEXT("the rhythm says not now"));
  const double A=FS47NightRhythm::Roll(F->Seed(),F->Day(),0x4d495353u)*2*UE_DOUBLE_PI;
  FSite P;P.Kind=TEXT("missing");P.Place=S.Place;P.Title=S.Title;P.Night=F->Day();P.Serial=Serial+1;P.X=S.X+FMath::Cos(A)*18;P.Z=S.Z+FMath::Sin(A)*18;P.Yaw=A;P.Until=Clock+2*86400.;P.bHeard=true;
  Sites.Add(P);UsedPlace(S.Place,TEXT("missing_person"));
  Tip({TEXT("Breaker one-nine, y'all keep an eye out. My cousin's neighbor never made it home from Magdalena last night. Sixty years old, bad knee."),FString::Printf(TEXT("Blue half-ton. They figure he cut out toward %s. You see it, holler on nine for the county."),*Spoken(S.Title))},TEXT("woman"),
   FString::Printf(TEXT("The county is looking for a man from Magdalena who did not come home: a blue half-ton pickup, last seen headed toward %s."),*Spoken(S.Title)));
  return true;
 }
 Why=TEXT("no such happening");
 return false;
}
bool FS47Happenings::StepMore(double Dt)
{
 auto* F=M->Freeform.Get();
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const FString& Id=Now.Id;
 US47Audio* A=M->Audio.Get();
 if(Id==TEXT("dogs_hush"))
 {
  // Every dog at once, a few seconds, then all of them stop together.
  if(Now.Age<4.5&&Now.Age>=Now.Yaw&&A)
  {
   Now.Yaw=Now.Age+FMath::FRandRange(.25f,.6f);
   const FVector At=Now.At+FVector(FMath::FRandRange(-40.f,40.f),2,FMath::FRandRange(-40.f,40.f));
   A->Far(FMath::RandBool()?TEXT("dog0"):TEXT("dog1"),At,.3f,FMath::FRandRange(.9f,1.1f));
  }
  if(Now.Stage==0&&Now.Age>=4.5)
  {
   Now.Stage=1;if(A&&A->Night)A->Night->Hush(25,.03,.15,4.);
   F->RecordEvidence(FString::Printf(TEXT("dogs_hush_d%d"),F->Day()),FString::Printf(TEXT("At %s every dog at %s barked at once, and then all of them stopped together, in the middle of it. The crickets stopped after."),*S47Hap::ClockText(Now.Clock),*Now.Title));
   Deed(FString::Printf(TEXT("You listened to the dogs at %s stop."),*Now.Title));
  }
  if(Now.Age>=Now.Len)EndEvent(true);
  return true;
 }
 if(Id==TEXT("phone_empty"))
 {
  if(FProp* P=Prop(TEXT("phone"))){PoseProp(*P,Now.At,0);ShowProp(*P,Now.Stage==0);}
  if(Now.Stage==0&&Now.Age>=Now.Yaw&&A)
  {
   Now.Yaw=Now.Age+4;
   FS47Sound O;O.At=Now.At;O.Gain=.8f;const FS47Play R=A->Sfx(TEXT("phoneRing"),O);
   if(R&&A->Ctx.IsValid()){const double T0=A->Ctx->CurrentTime();R.Gain->Gain.SetValueAtTime(.8f,T0+1.7);R.Gain->Gain.LinearRampToValueAtTime(0,T0+1.85);R.Src->Stop(T0+1.9);}
  }
  if(Now.Age>=Now.Len)EndEvent(Now.Stage>0);
  return true;
 }
 if(Id==TEXT("horizon_figure"))
 {
  const FString Shade=S47Hap::ShadeKind(Now.Variant);
  FProp* P=Prop(Shade);FVector At;
  if(WorldOf(Now.X,Now.Z,At))Now.At=At;
  const FVector To=Feet-Now.At;
  if(P){PoseProp(*P,Now.At,FMath::Atan2(To.X,To.Z));ShowProp(*P,Now.Stage==0);}
  // The lamp turned on it is the player's own doing, seen or not; seeing it is seeing it: against the sky on its rise, or lit.
  const bool bLamp=M->Night&&M->Night->Flashlight&&M->Night->Flashlight->bOn&&Seen(Now.At+FVector(0,1,0),400,6);
  const double Away=FVector2D(To.X,To.Z).Size();
  if(!Now.bSeen&&SeenLit(Now.At+FVector(0,1.6,0),260,30,LitGround|LitSky,*Shade)){Now.bSeen=true;Noticed(TEXT("horizon_figure"),Away,TEXT("sight"));}
  if(Now.Stage==0&&(bLamp||Away<(S47HapTiers::GlimpseOnly(Now.Clock)?S47HapTiers::GlimpseGoneAt:90.)))
  {
   Now.Stage=1;Now.Len=Now.Age+1;
   if(Now.bSeen)F->RecordEvidence(FString::Printf(TEXT("horizon_figure_d%d"),F->Day()),FString::Printf(TEXT("At %s a figure stood on the rise to the %s, a long way off, dead still. Then there was nobody on the rise."),*S47Hap::ClockText(Now.Clock),*DirectionFrom(Feet,Now.At)));
  }
  if(Now.Age>=Now.Len)EndEvent(Now.Stage>0);
  return true;
 }
 if(Id==TEXT("road_glimpse"))
 {
  // Across from its shoulder to the other in the lights, far ahead (S47RoadGlimpse::CrossX: + toward the driver's right, which is
  // road-local -z's way round: -Now.Yaw), and gone at the far side or once the truck is near, whichever comes first.
  const FString Kind=TEXT("glimpse_")+Now.Variant;
  FProp* P=Prop(Kind);
  const double Right=-Now.Yaw;
  bool bOn=false,bStand=false;
  const double X=S47RoadGlimpse::CrossX(Glimpse,Now.Age,&bOn,&bStand)*Right;
  FVector At;const bool bHere=WorldOf(X,Now.Z,At);
  if(bHere)Now.At=At;
  const double Away=bHere?AwayFrom(At):1e9;
  const bool bShow=bHere&&bOn&&Now.Stage==0&&Away>S47RoadGlimpse::GoneWithinM;
  if(P&&bHere)
  {
   // Facing its way across; the tall one, in its stop on the crown, turned to the lights.
   double Yaw=0;FVector Next;
   if(bStand){const FVector To=PlayerAt()-At;Yaw=FMath::Atan2(To.X,To.Z);}
   else if(WorldOf(X-Glimpse.Side*Right,Now.Z,Next))Yaw=FMath::Atan2(Next.X-At.X,Next.Z-At.Z);
   PoseProp(*P,At+FVector(0,S47RoadGlimpse::Bob(Glimpse,Now.Age),0),Yaw);
  }
  if(P)ShowProp(*P,bShow);
  // In the truck's lights (their near throw, or the far reach straight ahead): its far beam, from the lamps, the pair's 2 x 4200 cd
  // (S47Truck PlaceHeadlights) as their cone has it at that bearing, in over 40 to 50 m where the spots themselves end.
  bool bBeam=false;
  const double Head=Glimpse.Body==S47RoadGlimpse::EBody::Low?.5:1.5;
  {
   FS47Truck* T=bShow&&Driving()?M->World->CabTruck():nullptr;
   const FS47DriveController* D=T?M->World->CabDrive():nullptr;
   double Cd=0;FVector Lamp=FVector::ZeroVector,Flat=FVector::ZeroVector;
   if(D)
   {
    const FVector Ahead(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading));Flat=FVector(At.X-D->Pos.X,0,At.Z-D->Pos.Z);
    const double Deg=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Ahead,Flat.GetSafeNormal()),-1.,1.)));
    bBeam=M->World->Trouble.Dim>.5&&T->Beam()>.01&&S47RoadGlimpse::InBeam(Flat.Size(),Deg);
    Lamp=T->LocalToWorld(FVector(0,.8435,-2.65));
    if(bBeam)Cd=2.*4200.*T->Beam()*(1.-FMath::SmoothStep(10.,21.,Deg))*FMath::SmoothStep(40.,50.,Flat.Size());
   }
   if(USpotLightComponent* Sp=P?P->FarBeam.Get():nullptr)
   {
    const bool bLit=Cd>1.;
    if(bLit)
    {
     const FVector Aim=At+FVector(0,Head*.6,0);const double Dist=FMath::Max(1.,(Aim-Lamp).Size());
     // Narrow, onto the body alone (nothing else is on its channel); its reach twice the distance, so the falloff is the lamps'.
     const float Cone=float(FMath::RadiansToDegrees(FMath::Atan(4./Dist)))+1.f;
     Sp->SetWorldLocationAndRotation(S47Three::ToUE(Lamp),(S47Three::ToUE(Aim)-S47Three::ToUE(Lamp)).Rotation());
     Sp->SetOuterConeAngle(Cone);Sp->SetInnerConeAngle(Cone*.75f);Sp->SetAttenuationRadius(float(Dist*200.));Sp->SetIntensity(float(Cd));
    }
    else Sp->SetIntensity(0);
    if(Sp->IsVisible()!=bLit)Sp->SetVisibility(bLit);
   }
  }
  // Seen: lit by that beam (the shade's near-black stays dark in it), glowing of itself (the Cosmic one), or against the sky, with
  // nothing between.
  if(bShow&&!Now.bSeen)
  {
   const bool bOwn=(bBeam&&Now.Variant!=TEXT("shadow"))||Now.Variant==TEXT("cosmic");
   if(SeenLit(At+FVector(0,Head,0),S47RoadGlimpse::BeamFarM+5,32,bOwn?LitOwn:(LitGround|LitSky),*Kind)){Now.bSeen=true;Noticed(TEXT("road_glimpse"),Away,TEXT("sight"));}
  }
  if(Now.Stage==0&&(!bOn||Away<=S47RoadGlimpse::GoneWithinM))
  {
   Now.Stage=1;Now.Len=FMath::Min(Now.Len,Now.Age+.1);
   if(Now.bSeen)
   {
    F->RecordEvidence(FString::Printf(TEXT("road_glimpse_d%d"),F->Day()),S47RoadGlimpse::Record(Glimpse,S47Hap::ClockText(Now.Clock)));
    Deed(S47RoadGlimpse::Deed(Glimpse));
   }
   // Some nights it leaves prints going off the far shoulder, there the next day (StepSites finds them, PlaceSites draws them).
   if(Glimpse.bTracks)
   {
    const double Off=-Glimpse.Side*Right;
    FSite S;S.Kind=TEXT("roadtracks");S.Variant=Glimpse.Body==S47RoadGlimpse::EBody::Low?TEXT("low"):TEXT("tall");S.Title=TEXT("the highway");
    S.Night=F->Day();S.Serial=Now.Serial;S.X=Off*6.5;S.Z=Now.Z;
    FVector A0,A1;S.Yaw=WorldOf(S.X,S.Z,A0)&&WorldOf(S.X+Off,S.Z,A1)?FMath::Atan2(A1.X-A0.X,A1.Z-A0.Z):0.;
    // Until eight in the evening of the day after.
    S.Until=FS47FreeformSession::NightOf(F->Clock())*86400.+20*3600.;
    Sites.RemoveAll([](const FSite& Old){return Old.Kind==TEXT("roadtracks");});Sites.Add(S);
   }
   UE_LOG(LogS47Happen,Display,TEXT("S47: happening road_glimpse (%s) %s, the truck %.0f m off at the end%s"),*Now.Variant,Now.bSeen?TEXT("seen"):TEXT("not seen"),Away,Glimpse.bTracks?TEXT(", prints left on the shoulder"):TEXT(""));
  }
  if(Now.Age>=Now.Len)EndEvent(Now.bSeen);
  return true;
 }
 if(Id==TEXT("room6_tv"))
 {
  if(FProp* P=Prop(TEXT("tvlight"))){PoseProp(*P,Now.At,0);ShowProp(*P,true);for(FProp::FLamp& L:P->Lamps)if(UPointLightComponent* Lc=L.L.Get())Lc->SetIntensity(L.Lumens*FMath::FRandRange(.4f,1.f));}
  if(Now.Stage==0&&FVector2D::Distance(FVector2D(Now.At.X,Now.At.Z),FVector2D(Feet.X,Feet.Z))<12)
  {
   // The door open a crack, the snow's light on it and its hiss (the room's television): nothing said.
   Now.Stage=1;Noticed(TEXT("room6_tv"),AwayFrom(Now.At),TEXT("sight"));
   F->RecordEvidence(FString::Printf(TEXT("room6_tv_d%d"),F->Day()),FString::Printf(TEXT("At %s the door of room 6 at the motel stood open a crack, the TV inside showing nothing but snow."),*S47Hap::ClockText(F->Clock())));
  }
  if(Now.Age>=Now.Len)EndEvent(Now.Stage>0);
  return true;
 }
 if(Id==TEXT("town_blackout")||Id==TEXT("array_burst"))
 {
  if(Now.Age>=Now.Len)EndEvent(true);
  return true;
 }
 if(Id==TEXT("animals_flee"))
 {
  // A dozen jackrabbits, all one way, flat out.
  if(FProp* P=Prop(TEXT("rabbits")))
  {
   const FVector At=Now.At+Now.Dir*(Now.Age*11.);double X=0,Z=0;FVector Ground=At;
   if(RoadOfWorld(At,X,Z))WorldOf(X,Z,Ground);
   PoseProp(*P,Ground,FMath::Atan2(Now.Dir.X,Now.Dir.Z));ShowProp(*P,Now.Age<6.5);
  }
  if(Now.Stage==0&&Now.Age>2.5)
  {
   Now.Stage=1;
   if(A){FS47Sound O;O.At=Feet+Now.Dir*3;O.Gain=.25f;O.Rate=1.4f;A->Sfx(TEXT("gust"),O);}
   F->RecordEvidence(FString::Printf(TEXT("animals_flee_d%d"),F->Day()),FString::Printf(TEXT("At %s jackrabbits, a dozen or more, went past me all one way, toward the %s, flat out, and not one looked at me."),*S47Hap::ClockText(Now.Clock),*DirectionFrom(Feet,Feet+Now.Dir*10)));
  }
  if(Now.Age>=Now.Len)EndEvent(true);
  return true;
 }
 if(Id==TEXT("car_light"))
 {
  // The light stands over where the truck died. Stay in the cab and it goes; get out under it and it comes down (StepWatch: stage 8),
  // or, when the night's rules will not have it, goes up (stage 7).
  FProp* P=Prop(TEXT("lander"));
  if(P){PoseProp(*P,Now.At+FVector(0,38,0),Now.Age*.2);ShowProp(*P,Now.Age<Now.Len-1&&Now.Stage<7);}
  if(Now.Stage<7){M->World->Trouble.Dim=.25;bDimmed=true;}
  if(Now.Age>=Now.Len||Now.Stage>=7)
  {
   M->World->Trouble.Dim=1;bDimmed=false;
   F->RecordEvidence(FString::Printf(TEXT("car_light_d%d"),F->Day()),FString::Printf(TEXT("At %s a white light stood over the truck on the road and the engine died under it.%s"),*S47Hap::ClockText(Now.Clock),
    Now.Stage==0?TEXT(" I stayed in the cab, and after about twenty seconds it went up."):Now.Stage==7?TEXT(" I got out under it, and it went up."):TEXT(" I got out under it.")));
   Deed(Now.Stage==0?TEXT("You sat in the truck under the light and did not get out."):TEXT("You got out of the truck under the light."));
   EndEvent(true);
  }
  return true;
 }
 if(Id==TEXT("dust_devil"))
 {
  FProp* P=Prop(TEXT("devil"));
  if(P){PoseProp(*P,Now.At,Now.Age*2.4);ShowProp(*P,Now.Stage<2);}
  const double Away=FVector2D::Distance(FVector2D(Now.At.X,Now.At.Z),FVector2D(Feet.X,Feet.Z));
  if(Now.Stage==0&&SeenLit(Now.At+FVector(0,3,0),200,20,LitGround|LitSky,TEXT("devil")))
  {
   Now.Stage=1;Noticed(TEXT("dust_devil"),Away,TEXT("sight"));
   F->RecordEvidence(FString::Printf(TEXT("dust_devil_d%d"),F->Day()),FString::Printf(TEXT("At %s a dust devil stood out on the flat to the %s, turning, and did not move. There was no wind at all."),*S47Hap::ClockText(Now.Clock),*DirectionFrom(Feet,Now.At)));
  }
  if(Now.Stage<2&&Away<22){Now.Stage=2;Now.Len=Now.Age+.5;if(A){FS47Sound O;O.At=Now.At;O.Gain=.4f;A->Sfx(TEXT("gust"),O);}Noticed(TEXT("dust_devil"),Away,TEXT("sound"));}
  if(Now.Age>=Now.Len)EndEvent(Now.Stage>0);
  return true;
 }
 static_cast<void>(Dt);
 return false;
}
void FS47Happenings::UsePhone()
{
 if(Now.Id!=TEXT("phone_empty")||Now.Stage>0||!M->Freeform)return;
 auto* F=M->Freeform.Get();
 Now.Stage=1;Now.Len=Now.Age+2;
 if(M->Audio){FS47Sound O;O.At=Now.At;O.Gain=.35f;M->Audio->Sfx(M->Audio->Or(TEXT("radio_static"),TEXT("lic:line_hush")),O);}
 Noticed(TEXT("phone_empty"),AwayFrom(Now.At),TEXT("the hand"));
 F->RecordEvidence(FString::Printf(TEXT("phone_empty_d%d"),F->Day()),FString::Printf(TEXT("At %s the phone at %s rang with nobody there to answer it. When I picked it up there was a hiss on the line, a long way off, and nothing else."),*S47Hap::ClockText(F->Clock()),*Now.Title));
 Deed(FString::Printf(TEXT("You picked up the phone at %s."),*Now.Title));
}

// ---------------------------------------------------------------- unease (URO.md, TODO UNEASE-20261108)
int32 FS47Happenings::UneaseCount(){return UE_ARRAY_COUNT(S47Hap::UneaseDefs);}
const TCHAR* FS47Happenings::UneaseId(int32 I){return I>=0&&I<UneaseCount()?S47Hap::UneaseDefs[I].Id:TEXT("");}
const TCHAR* FS47Happenings::UneaseTitle(int32 I){return I>=0&&I<UneaseCount()?S47Hap::UneaseDefs[I].Title:TEXT("");}
void FS47Happenings::PlanUnease(int32 Night)
{
 auto* F=M->Freeform.Get();
 if(UneaseNight!=Night){UneaseNight=Night;UneaseDone.Reset();}
 const int32 Seed=F->Seed();
 // Tonight's order, from the seed: the first two or three are wanted, the rest stand by for where the player is.
 UneasePool.Reset();for(int32 I=0;I<UneaseCount();++I)UneasePool.Add(I);
 // From the seed, and what came in the last three nights at the back: a repeat is a little different each time, and rarer.
 const auto Recent=[this,Night](int32 I){const int32* Last=UneaseLast.Find(I);return Last&&*Last<Night&&*Last>=Night-3;};
 UneasePool.Sort([Seed,Night,&Recent](int32 A,int32 B)
 {
  const bool RA=Recent(A),RB=Recent(B);if(RA!=RB)return !RA;
  return FS47NightRhythm::Roll(Seed,Night,0x55520000u+uint32(A))<FS47NightRhythm::Roll(Seed,Night,0x55520000u+uint32(B));
 });
 const float Many=FS47NightRhythm::Roll(Seed,Night,0x554e5754u);
 // M7, the rhythm review: as many in the first week as in the fourth was flat. One a night in the first week, two in the second,
 // then up to three and four toward the end.
 UneaseWant=FMath::Min(4,1+(Night>=8?1:0)+(Night>=15&&Many>.4f?1:0)+(Night>=22&&Many>.7f?1:0));
 UneaseAt.Reset();
 // Spread from half past eight to half past five, so they do not all fall in one hour.
 for(int32 K=0;K<UneaseWant;++K)UneaseAt.Add(20.5+(K+.2+.6*FS47NightRhythm::Roll(Seed,Night,0x55415400u+uint32(K)))*9./UneaseWant);
 UE_LOG(LogS47Happen,Display,TEXT("S47: unease night %d: %d wanted, first %.2f h, done already %d"),Night,UneaseWant,UneaseAt.Num()?FMath::Fmod(UneaseAt[0],24.):-1.,UneaseDone.Num());
}
void FS47Happenings::StepUnease(double Dt)
{
 auto* F=M->Freeform.Get();
 const double Clock=F->Clock();
 // When the last danger began (not right after one), and where the player has been (for the chair, the door, the frost).
 const int32 DangerNow=F->Director?F->Director->DangerTonight():0;
 if(DangerNow>DangerSeen)DangerClock=Clock;
 DangerSeen=DangerNow;
 const bool bDrive=Driving(),bIn=!Outdoors()&&!bDrive;
 const bool bSaroIn=bIn&&M->Web->Area()==TEXT("saro");
 if(!bIn&&!bDrive){if(OutsideSince<0)OutsideSince=Clock;}
 if(FS47Truck* T=M->World->TruckHere();T&&!bDrive)
 {
  const double Far=FVector2D::Distance(FVector2D(T->Position.X,T->Position.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
  if(Far>40&&TruckAwaySince<0)TruckAwaySince=Clock;
 }
 if(Uneasy.I>=0)StepUneasy(Dt,NightPace());
 // The chair stays where it was put until the player leaves the control room.
 if(bChairUp&&M->Space!=TEXT("room")){if(FProp* P=FindProp(TEXT("chair")))ShowProp(*P,false);bChairUp=false;}
 const bool bEnteredTruck=bDrive&&!bWasDriving,bEnteredSaro=bSaroIn&&!bWasIndoors;
 const bool bNearTruck=[&]{FS47Truck* T=M->World->TruckHere();return T&&!bDrive&&FVector2D::Distance(FVector2D(T->Position.X,T->Position.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z))<9;}();
 bWasDriving=bDrive;bWasIndoors=bSaroIn;
 // Back within reach of the truck after a while away: its own stamp for the hood and the tracks (BeginMore), not the unease's,
 // and kept whatever the rules (a test of the hood walks away and back).
 if(bNearTruck&&TruckAwaySince>=0){ReturnedAt=Paced;ReturnedAway=Clock-TruckAwaySince;}
 if(bNearTruck)TruckAwaySince=-1;
 if(!F->RulesLive()||F->InTutorial()||UneasePool.IsEmpty())
 {
  if(!bIn)OutsideSince=OutsideSince<0?Clock:OutsideSince;else if(bSaroIn)OutsideSince=-1;
  return;
 }
 const double H=HourOfNight(Clock);
 const bool bSlot=UneaseDone.Num()<FMath::Min(UneaseWant,4)&&UneaseAt.IsValidIndex(UneaseDone.Num())&&H>=UneaseAt[UneaseDone.Num()]&&H<29.8;
 FString Why;
 // What waits for a moment: getting into the truck, coming back into SARO after a while out, walking back up to the truck.
 if(bSlot&&Uneasy.I<0)
 {
  if(bEnteredTruck&&!UneaseDone.Contains(14)&&FireUnease(14,false,Why)){}
  else if(bEnteredSaro&&OutsideSince>=0&&Clock-OutsideSince>=600&&!UneaseDone.Contains(11)&&FireUnease(11,false,Why)){}
  else if(!UneaseDone.Contains(12)&&TruckAwaySince>=0&&Clock-TruckAwaySince>=900&&[&]
  {
   // Coming back to it with it in sight: its cab is lit.
   FS47Truck* T=M->World->TruckHere();if(!T||bDrive)return false;
   const double Far=FVector2D::Distance(FVector2D(T->Position.X,T->Position.Z),FVector2D(M->Player->Pos.X,M->Player->Pos.Z));
   return Far>10&&Far<60&&Seen(T->Position+FVector(0,1.2,0),70,30);
  }()&&FireUnease(12,false,Why)){}
 }
 if(bSaroIn)OutsideSince=-1;
 // Otherwise, once its hour has come, the first in tonight's order that fits where the player is, every few seconds until one does.
 if(!bSlot||Uneasy.I>=0||Paced<UneaseTry)return;
 UneaseTry=Paced+4;
 for(const int32 I:UneasePool)
 {
  if(UneaseDone.Contains(I)||I==11||I==12||I==14)continue;
  if(FireUnease(I,false,Why))return;
 }
}
bool FS47Happenings::FireUnease(int32 I,bool bForced,FString& Why)
{
 auto* F=M?M->Freeform.Get():nullptr;
 if(!F||!F->Active()||!M->IsFreeform()||!M->Player||!M->World||!M->Web){Why=TEXT("Freeform only");return false;}
 if(I<0||I>=UneaseCount()){Why=TEXT("no such thing");return false;}
 const FString Id=UneaseId(I);
 const double Clock=F->Clock();
 if(Uneasy.I>=0)EndUneasy();
 if(!bForced)
 {
  if(UneaseDone.Contains(I)){Why=TEXT("already tonight");return false;}
  if(UneaseOnce.Contains(Id)){Why=TEXT("once in a game, and it has been");return false;}
  // Never on top of something else, nor a moment after anything began, of this or any other system.
  if(!Now.Id.IsEmpty()){Why=TEXT("something else is on");return false;}
  const double Since=M->Rhythm?Clock-M->Rhythm->LastStart():1e9;
  if(Since<300+300*FS47NightRhythm::Roll(F->Seed(),int32(Clock/600.),0x55475031u)){Why=TEXT("something else began a moment ago");return false;}
  // The strong ones (your name, the note) not before the third week (M7: they were the same from the sixth night as on the last)
  // unless the player has been taken, and only in a quiet stretch.
  if((I==2||I==19)&&FS47FreeformSession::NightOf(Clock)<15&&F->TimesTaken()<=0){Why=TEXT("too early in the game for that one");return false;}
  if((I==2||I==19)&&Since<20*60){Why=TEXT("not quiet enough for that one");return false;}
  // Not just after a danger, nor on top of one, nor just before their hours begin.
  if(DangerSeen>0&&Clock-DangerClock<20*60){Why=TEXT("too soon after a danger");return false;}
  if(M->Rhythm)
  {
   const FS47RhythmNow N=M->Rhythm->ReadNow();
   if(!N.Busy.IsNone())if(const FS47RhythmEvent* E=M->Rhythm->Data().Find(N.Busy))if(E->Level==ES47RhythmLevel::Danger){Why=TEXT("a danger is on");return false;}
  }
  const int32 Night=FS47FreeformSession::NightOf(Clock);
  const double Start=F->BodyStartHour(Night)+24,H=HourOfNight(Clock);
  if(!F->QuietNight(Night)&&H>Start-.25&&H<Start+.05){Why=TEXT("just before their hours");return false;}
  // Nor in the half hour before a danger drawn tonight.
  for(const auto& KV:Plan)if(IsDanger(KV.Key)&&LastDays.FindRef(KV.Key)!=F->Day()&&H>=KV.Value-.5&&H<KV.Value+.25){Why=TEXT("a danger may be close");return false;}
  if(!MayStart(TEXT("unease"))){Why=TEXT("the rhythm says not now");return false;}
 }
 US47Audio* A=M->Audio.Get();
 const FS47Cam Cam=FS47Cam::Of(M->Player);
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 const FVector Fwd=FVector(Cam.Forward.X,0,Cam.Forward.Z).GetSafeNormal(),Side(-Fwd.Z,0,Fwd.X);
 const bool bDrive=Driving(),bOut=Outdoors()&&!bDrive,bIn=!Outdoors()&&!bDrive,bSaro=M->Web->Area()==TEXT("saro");
 const bool bDark=[&]{const double Hd=S47Hap::HourOf(Clock);return Hd>=19.5||Hd<5.5;}();
 if(!bForced&&!bDark){Why=TEXT("not in the dark");return false;}
 FUneasy U;U.I=I;U.Clock=Clock;U.bForced=bForced;
 const auto Door=[&](double Within,FVector& Out)
 {
  if(F->EventPlaces)if(const FS47EventPlace* P=F->EventPlaces->Nearest(Within,ES47PlaceTrait::Door))if(F->EventPlaces->EntryNow(P->Id,Out))return true;
  if(!bForced)return false;
  Out=Feet-Fwd*3.5;return true;
 };
 const auto No=[&Why](const TCHAR* Text){Why=Text;return false;};
 bool bLasting=false;
 switch(I)
 {
 case 0: // the truck's alarm, out where it is parked: from the truck itself
 {
  FS47Truck* T=M->World->TruckHere();if(!T||bDrive)return No(TEXT("no truck parked here, or you are in it"));
  const double Far=FVector2D::Distance(FVector2D(T->Position.X,T->Position.Z),FVector2D(Feet.X,Feet.Z));
  if(!bForced&&(Far<20||Far>220))return No(TEXT("the truck is too near or too far"));
  if(A){FS47Sound O;O.At=T->Position+FVector(0,1,0);O.Gain=.6f;O.bLoop=true;A->LoopSfx(TEXT("uneaseAlarm"),Synth(TEXT("alarm")),O);}
  U.At=T->Position;U.Len=9;bLasting=true;break;
 }
 case 1: // three knocks
 {
  FVector At;if(!bIn&&!bForced)return No(TEXT("not indoors"));
  if(!Door(25,At))return No(TEXT("no door near"));
  if(A)for(int32 K=0;K<3;++K){FS47Sound O;O.At=At+FVector(0,1.3,0);O.Gain=.7f;O.Rate=1.25f;O.When=.38f*K;O.Lp=2400;A->Sfx(TEXT("thudSoft"),O);}
  break;
 }
 case 2: // your name, once in a game
 {
  if(bDrive&&!bForced)return No(TEXT("in the truck"));
  // A whisper close behind: two breaths of a word. Whether it was your name is yours to say.
  if(A){FS47Sound O;O.At=Cam.Pos-Cam.Forward*1.1+FVector(0,.1,0);O.Gain=.55f;O.When=.3f;A->Sfx(Synth(TEXT("whisper")),O);}
  break;
 }
 case 3: // steps in the gravel that stop when you stop
  if(!bOut&&!bForced)return No(TEXT("not out on foot"));
  U.At=Feet;U.Len=16;U.Next=.6;bLasting=true;break;
 case 4: // a dog a long way off, cut off in the middle of a bark
 {
  if(!bOut&&!bForced)return No(TEXT("not outside"));
  const double Ang=FS47NightRhythm::Roll(F->Seed(),int32(Clock/60.),0x444f4755u)*2*UE_DOUBLE_PI;
  if(A)
  {
   FS47Sound O;O.At=Feet+FVector(FMath::Cos(Ang)*380,2,FMath::Sin(Ang)*380);O.Gain=.5f;
   const FS47Play P=A->Sfx(A->Or(TEXT("dog0"),TEXT("dog1")),O);
   if(P&&A->Ctx.IsValid())P.Src->Stop(A->Ctx->CurrentTime()+.32);
   if(A->Night)A->Night->Hush(8,.05,.2,2.5);
  }
  break;
 }
 case 5: // a tap running
 {
  if(!bIn&&!bForced)return No(TEXT("not indoors"));
  if(!A||!A->Lic(TEXT("pour")).IsValid())return No(TEXT("no sound of running water in this build"));
  FS47Sound O;O.At=Feet+Side*3+FVector(0,1,0);O.Gain=.28f;O.bLoop=true;A->LoopSfx(TEXT("uneaseTap"),TEXT("lic:pour"),O);
  U.Len=22;bLasting=true;break;
 }
 case 6: // the radio finds an old song by itself, and goes back
  if(!bDrive)return No(TEXT("not in the truck"));
  if(!bForced&&!(M->CarRadio&&M->CarRadio->bOn))return No(TEXT("the radio is off"));
  if(A)
  {
   FS47Sound C;C.Dest=TEXT("radio");C.Gain=.3f;A->Sfx(A->Or(TEXT("radio_crackle_short"),TEXT("alienRadio")),C);
   FS47Sound O;O.Dest=TEXT("radio");O.Gain=.38f;O.When=.4f;O.Dur=6.f;A->Sfx(A->Or(TEXT("bamaCountry"),A->Or(TEXT("bama_country"),TEXT("titleMusic"))),O);
   FS47Sound B=C;B.When=6.5f;A->Sfx(A->Or(TEXT("radio_crackle_short"),TEXT("alienRadio")),B);
  }
  break;
 case 7: // the phone, and breathing on the line
 {
  if(!(bIn&&bSaro)&&!bForced)return No(TEXT("not in SARO"));
  const FS47Obj Ph=M->Web->Obj(TEXT("room.objs.phone"));
  const FVector At=Ph?Ph.WorldPosition():Feet+Side*2+FVector(0,1,0);
  if(A)
  {
   for(const float When:{0.f,3.4f})
   {
    FS47Sound O;O.At=At;O.When=When;const FS47Play R=A->Sfx(TEXT("phoneRing"),O);
    if(R&&A->Ctx.IsValid()){const double T0=A->Ctx->CurrentTime()+When;R.Gain->Gain.SetValueAtTime(1,T0+1.7);R.Gain->Gain.LinearRampToValueAtTime(0,T0+1.85);R.Src->Stop(T0+1.9);}
   }
   FS47Sound L;L.At=At;L.Gain=.35f;L.When=7.f;A->Sfx(A->Or(TEXT("lic:line_hush"),TEXT("thudSoft")),L);
   A->Gasp(8.6f);
  }
  break;
 }
 case 8: // a child laughs, far out in the desert
  if(!bOut&&!bForced)return No(TEXT("not outside"));
  // Far out, thin: something like a laugh, or a bird, or nothing.
  if(A)A->Far(Synth(TEXT("laugh")),Feet+Fwd*420+Side*160,.3f,1.f);
  break;
 case 9: // the hum of the place stops, and the quiet is louder
  if(!bIn&&!bForced)return No(TEXT("not indoors"));
  if(A){FS47Sound O;O.At=Feet+Side*1.5+FVector(0,1,0);O.Gain=.25f;A->Sfx(TEXT("switch"),O);A->HoldRoomTone(.04f,.25f);}
  U.Len=26;bLasting=true;break;
 case 10: // a lit window far off goes out just as you look at it
 {
  if(!bOut&&!bForced)return No(TEXT("not outside"));
  FVector Best=FVector::ZeroVector;double BestDot=.3;
  if(F->EventPlaces)for(const FS47EventPlace& P:F->EventPlaces->All())
  {
   if(!P.Has(ES47PlaceTrait::Window))continue;
   FVector At;if(!F->EventPlaces->AtNow(P.Id,At))continue;
   const FVector To=FVector(At.X-Feet.X,0,At.Z-Feet.Z);const double Far=To.Size();
   if(Far<120||Far>750)continue;
   const double Dot=FVector::DotProduct(To/Far,Fwd);
   if(Dot>BestDot){BestDot=Dot;Best=At;}
  }
  if(Best.IsZero()){if(!bForced)return No(TEXT("no window in sight far off"));Best=Feet+Fwd*180;}
  U.At=Best+FVector(0,1.8,0);U.Yaw=FMath::Atan2(Feet.X-Best.X,Feet.Z-Best.Z);U.Len=240;bLasting=true;
  if(!Prop(TEXT("window")))return No(TEXT("nothing to draw it with"));
  break;
 }
 case 11: // a chair stands somewhere else when you come back in
 {
  if(!(bIn&&bSaro)&&!bForced)return No(TEXT("not back in SARO"));
  FVector At(-.8,0,1.4);
  if(M->Player->Floor){const double Fl=M->Player->Floor(At.X,At.Z);At.Y=FMath::IsFinite(Fl)?Fl:Feet.Y;}else At.Y=Feet.Y;
  FProp* P=Prop(TEXT("chair"));if(!P)return No(TEXT("nothing to draw it with"));
  PoseProp(*P,At,2.3);ShowProp(*P,true);bChairUp=true;
  break;
 }
 case 12: // coming back, the truck's cab is lit, as with a door left open; close to, it goes out with a door's sound
 {
  FS47Truck* T=M->World->TruckHere();if(!T||bDrive)return No(TEXT("no truck parked here"));
  if(!Prop(TEXT("dome")))return No(TEXT("nothing to draw it with"));
  U.At=T->Position;U.Len=150;bLasting=true;break;
 }
 case 13: // the strip of light under the door goes dark a moment, with two steps beyond it
 {
  FVector At;FVector In=FVector::ZeroVector;if(!bIn&&!bForced)return No(TEXT("not indoors"));
  // In the control room, the corridor's door (seen 8 November: the east door leads out into the dark, with no light behind it):
  // the room's edge nearest the door's zone, and inward from it.
  const FS47Area RoomBox=bSaro?M->Web->Rect(TEXT("room.bounds")):FS47Area(),DoorZone=bSaro?M->Web->Rect(TEXT("annex.zone.southDoor")):FS47Area();
  if(RoomBox.bValid&&DoorZone.bValid)
  {
   const double Mx=FMath::Clamp((DoorZone.MinX+DoorZone.MaxX)*.5,RoomBox.MinX,RoomBox.MaxX),Mz=FMath::Clamp((DoorZone.MinZ+DoorZone.MaxZ)*.5,RoomBox.MinZ,RoomBox.MaxZ);
   const double ToW=Mx-RoomBox.MinX,ToE=RoomBox.MaxX-Mx,ToN=Mz-RoomBox.MinZ,ToS=RoomBox.MaxZ-Mz,Least=FMath::Min(FMath::Min(ToW,ToE),FMath::Min(ToN,ToS));
   In=Least==ToW?FVector(1,0,0):Least==ToE?FVector(-1,0,0):Least==ToN?FVector(0,0,1):FVector(0,0,-1);
   At=FVector(Least==ToW?RoomBox.MinX:Least==ToE?RoomBox.MaxX:Mx,0,Least==ToN?RoomBox.MinZ:Least==ToS?RoomBox.MaxZ:Mz);
  }
  else if(!Door(15,At))return No(TEXT("no door near"));
  if(In.IsZero())In=FVector(Feet.X-At.X,0,Feet.Z-At.Z).GetSafeNormal();
  if(M->Player->Floor){const double Fl=M->Player->Floor(At.X,At.Z);At.Y=FMath::IsFinite(Fl)?Fl:Feet.Y;}else At.Y=Feet.Y;
  if(!Prop(TEXT("strip")))return No(TEXT("nothing to draw it with"));
  // Across the foot of the door, on the room's side of it.
  U.At=At+In*.04;U.Yaw=FMath::Atan2(In.X,In.Z);U.Len=7;bLasting=true;
  if(A)for(int32 K=0;K<2;++K){FS47Sound O;O.At=At-In*.6+FVector(0,.2,0);O.Gain=.14f;O.When=1.f+.55f*K;O.Lp=1400;A->Sfx(K?TEXT("stepConcrete2"):TEXT("stepConcrete1"),O);}
  break;
 }
 case 14: // a handprint in the frost on the inside of the windshield, drawn in the frost itself
 {
  FS47Truck* T=bDrive?M->World->CabTruck():nullptr;if(!T)return No(TEXT("not in the truck"));
  T->SetFrostHand(true);T->SetFrost(.75);
  U.Len=40;bLasting=true;break;
 }
 case 15: // the lamp flickers only when it points at one spot
 {
  if(!bOut&&!bForced)return No(TEXT("not outside"));
  auto* L=M->Night?M->Night->Flashlight.Get():nullptr;if(!L)return No(TEXT("no lamp"));
  if(!L->bOn&&!bForced)return No(TEXT("the lamp is off"));
  if(bForced){L->bHave=true;L->bOn=true;}
  U.At=Feet+(Fwd*FMath::Cos(.6)+Side*FMath::Sin(.6))*14+FVector(0,.3,0);U.Len=150;bLasting=true;break;
 }
 case 16: // the wall clock in the control room stands still for a while
 {
  if(!(bIn&&bSaro)&&!bForced)return No(TEXT("not in SARO"));
  FS47Look* L=M->Inter?M->Inter->Looks.Find(TEXT("clock")):nullptr;if(!L)return No(TEXT("no wall clock here"));
  ClockSay=L->Say;ClockRange=L->Range;bClockHeld=true;
  L->Say=[]{return FString(TEXT("An electric wall clock with a sweep hand. The hand is not moving. No hum."));};
  if(A)A->HoldRoomTone(.5f,.3f);
  U.Len=60;bLasting=true;break;
 }
 case 17: // a figure on the next photograph, where there was none
 {
  FS47Album* Al=M->Night?M->Night->Album.Get():nullptr;if(!Al||Al->Left()<=0)return No(TEXT("no film left to keep it"));
  if(Al->Data.Ghosts>=2)return No(TEXT("the film has had its two already"));
  // Tonight's next photograph only (PlanTonight lets it go at the next night).
  Al->bForceGhost=true;GhostNight=FS47FreeformSession::NightOf(Clock);
  break;
 }
 case 18: // the mirror shows the back a moment too early
 {
  FS47Truck* T=bDrive?M->World->CabTruck():nullptr;if(!T)return No(TEXT("not in the truck"));
  if(Now.Id==TEXT("hitchhiker"))return No(TEXT("she is on the road"));
  if(!Prop(TEXT("hitcher")))return No(TEXT("no figure for it"));
  U.Len=.7;bLasting=true;break;
 }
 case 19: // a note on the desk in your own hand, once in a game
 {
  if(!(bIn&&bSaro)&&!bForced)return No(TEXT("not in SARO"));
  if(!bForced&&FVector2D::Distance(FVector2D(Feet.X,Feet.Z),FVector2D(2.05,2.19))>6)return No(TEXT("not at the desk"));
  if(!M->UI)return No(TEXT("no screen"));
  static const TCHAR* const Notes[]={TEXT("Ten past two. Don't go to the window."),TEXT("Don't count them again. There are as many as there were."),TEXT("Leave the hall light on tonight.")};
  const FString Note=Notes[int32(FS47NightRhythm::Roll(F->Seed(),1,0x4e4f5445u)*3)%3];
  // In the same pencil as the player's own marks on the survey sheet (SS47FieldMap), on a torn sheet of paper. Nothing under it.
  const TSharedRef<SWidget> Hand=SNew(SBorder).BorderImage(S47Theme::Box(S47Theme::Css(0xebe3cf))).Padding(FMargin(28,22))
   [SNew(STextBlock).Text(FText::FromString(Note)).Font(S47Theme::Hand(34)).ColorAndOpacity(S47Theme::Css(0x3a3a40,.92f)).AutoWrapText(true)];
  M->UI->Open(S47Kit::Shell(M->UI,TEXT("ON THE DESK"),S47Kit::Col({Hand}),560));
  break;
 }
 case 20: // cattle stand quite still and look at SARO
 {
  if(!bOut&&!bForced)return No(TEXT("not outside"));
  // The ring of cattle owns heads turned one way: this is its one echo, never in a week that had a ring (the text critic).
  const int32* Ring=LastDays.Find(TEXT("cattle_ring"));
  if(!bForced&&(HasSite(TEXT("cattle"))||(Ring&&FS47FreeformSession::NightOf(Clock)-*Ring<7)))return No(TEXT("a ring of cattle this week already"));
  using S47Hap::Bit;
  const FSpot S=Where(TEXT("unease_stare"),0,Bit(ES47PlaceTrait::Farm)|Bit(ES47PlaceTrait::Open),Bit(ES47PlaceTrait::Town)|Bit(ES47PlaceTrait::Saro)|Bit(ES47PlaceTrait::Indoors)|Bit(ES47PlaceTrait::Array),50,280,0.f,uint32(Clock/60.),true);
  double X=0,Z=0;
  if(S.bSet){X=S.X+25;Z=S.Z+10;}
  else{double Px=0,Pz=0;if(!bForced||!PlayerRoad(Px,Pz))return No(TEXT("no open ground near"));X=Px+Fwd.X*22;Z=Pz+Fwd.Z*22;}
  double SaroX=0,SaroZ=0;S47OpenLand::RoadOf(TEXT("saro"),9,0,SaroX,SaroZ);
  U.X=X;U.Z=Z;U.Yaw=-FMath::DegreesToRadians(FS47Places::AzimuthOf(X,Z,SaroX,SaroZ));U.Len=240;bLasting=true;
  if(!Prop(TEXT("stare")))return No(TEXT("no cow models"));
  break;
 }
 case 21: // a dead animal in the lane, and the next time you pass it is not there
 {
  double X=0,Z=0,RoadSpeed=0;if(!OnHighway(X,Z,RoadSpeed))return No(TEXT("not on the highway"));
  if(!bForced&&FMath::Abs(RoadSpeed)<8)return No(TEXT("not at speed"));
  const FS47DriveController* D=M->World->CabDrive();const double Dir=-FMath::Cos(D->Heading)<0?-1.:1.;
  U.X=X;U.Z=Z+Dir*160;U.Yaw=FS47NightRhythm::Roll(F->Seed(),int32(Clock),0x5244u)*6.28;U.Dir=FVector(0,0,Dir);U.Len=90;bLasting=true;
  if(!Prop(TEXT("carcass")))return No(TEXT("nothing to draw it with"));
  break;
 }
 case 22: // Caliche's street lamps go out one after another down the street
 {
  double X=0,Z=0;
  const bool bTown=PlayerRoad(X,Z)&&Z>1650&&Z<2550&&FMath::Abs(X)<260&&(M->Web->Area()==TEXT("road")||M->Web->Area()==TEXT("diner"));
  if(!bTown&&!bForced)return No(TEXT("not where the lamps can be seen"));
  FS47Town* T=M->TownLayer?M->TownLayer->Town.Get():nullptr;if(!T)return No(TEXT("no town here"));
  T->LampWave(M->T);
  break;
 }
 case 23: // a car with its lights off behind you, that turns off
 {
  double X=0,Z=0,RoadSpeed=0;if(!OnHighway(X,Z,RoadSpeed))return No(TEXT("not on the highway"));
  if(!bForced&&FMath::Abs(RoadSpeed)<8)return No(TEXT("not at speed"));
  if(!Prop(TEXT("darkcar")))return No(TEXT("no car model"));
  U.Len=26;bLasting=true;break;
 }
 case 24: // every bird near goes up at once
  if(!bOut&&!bForced)return No(TEXT("not outside"));
  if(A)
  {
   // From the brush on both sides and ahead, a moment apart, and up.
   const FString Wings=Synth(TEXT("wings"));
   for(int32 K=0;K<3;++K){FS47Sound O;O.At=Feet+Side*(K==0?-7.:K==1?6.:1.)+Fwd*(K==2?9.:3.)+FVector(0,1.5+K,0);O.Gain=.6f;O.Rate=.92f+.08f*K;O.When=.12f*K;A->Sfx(Wings,O);}
   if(A->Night)A->Night->Hush(14,.05,.3,3.);
  }
  break;
 default:return No(TEXT("no such thing"));
 }
 if(bLasting)Uneasy=U;
 if(!bForced){UneaseDone.AddUnique(I);UneaseLast.Add(I,FS47FreeformSession::NightOf(Clock));if(I==2||I==19)UneaseOnce.AddUnique(Id);}
 Noticed();
 // Counted as a start, and followed by the short rest like anything else (one that lasts, when it is over: EndUneasy).
 if(M->Rhythm){M->Rhythm->Began(TEXT("unease"));if(!bLasting&&!bForced)M->Rhythm->Ended(TEXT("unease"));}
 UE_LOG(LogS47Happen,Display,TEXT("S47: unease %d %s day %d %s in %s%s, %d tonight"),I+1,*Id,F->Day(),*S47Hap::ClockText(Clock),*M->Web->Area(),bForced?TEXT(", F10"):TEXT(""),UneaseDone.Num());
 M->RequestAutosave();
 return true;
}
void FS47Happenings::StepUneasy(double Dt,double Pc)
{
 FUneasy& U=Uneasy;
 U.Age+=Dt*Pc;
 US47Audio* A=M->Audio.Get();
 const FVector Feet(M->Player->Pos.X,M->Player->FloorY,M->Player->Pos.Z);
 switch(U.I)
 {
 case 0: // the alarm sounds from the truck itself (FireUnease) until it stops
  break;
 case 12: // the lit cab: out as the player comes up to it, with a door's sound
 {
  FS47Truck* T=M->World->TruckHere();FProp* P=Prop(TEXT("dome"));
  if(!T||Driving()){U.Len=FMath::Min(U.Len,U.Age);break;}
  const double Far=FVector2D::Distance(FVector2D(T->Position.X,T->Position.Z),FVector2D(Feet.X,Feet.Z));
  if(P){PoseProp(*P,T->LocalToWorld(FVector(0,1.45,-.1)),0);ShowProp(*P,U.Stage==0);}
  if(U.Stage==0&&Far<4.5)
  {
   U.Stage=1;if(P)ShowProp(*P,false);U.Len=FMath::Min(U.Len,U.Age+.5);
   if(A){FS47Sound O;O.At=T->Position+FVector(0,1,0);O.Gain=.35f;A->Sfx(TEXT("doorMetal"),O);}
  }
  break;
 }
 case 13: // the strip under the door: lit, dark between the two steps, lit, then the light beyond goes off
 {
  FProp* P=Prop(TEXT("strip"));if(!P)break;
  PoseProp(*P,U.At,U.Yaw);
  const bool bLit=U.Age<1.05||(U.Age>1.6&&U.Age<5.2);
  ShowProp(*P,bLit);
  if(bLit&&U.Age>4.2)for(FProp::FLamp& L:P->Lamps)if(UPointLightComponent* Lc=L.L.Get())Lc->SetIntensity(L.Lumens*float(FMath::Clamp((5.2-U.Age),0.,1.)));
  break;
 }
 case 3: // a step behind for each of yours, none when you stand
 {
  const double Moved=FVector2D::Distance(FVector2D(Feet.X,Feet.Z),FVector2D(U.At.X,U.At.Z));
  const double Pace=Dt>0?Moved/Dt:0;U.At=Feet;
  if(Pace>.7)
  {
   U.Next-=Dt;
   if(U.Next<=0&&A)
   {
    U.Next=.52;++U.Stage;
    const FVector Fwd=FVector(FMath::Sin(-M->Player->Yaw),0,-FMath::Cos(M->Player->Yaw));
    FS47Sound O;O.At=Feet-Fwd*7+FVector(0,.1,0);O.Gain=.2f;A->Sfx(FString::Printf(TEXT("stepGravel%d"),U.Stage%5),O);
   }
  }
  else U.Next=FMath::Max(U.Next,.2);
  break;
 }
 case 10: // the far window, lit until it is looked at
 {
  FProp* P=Prop(TEXT("window"));if(!P)break;
  if(U.Stage==0){PoseProp(*P,U.At,U.Yaw);ShowProp(*P,true);if(SeenLit(U.At,900,4,LitOwn,TEXT("window"))){U.Next+=Dt;if(U.Next>.4){U.Stage=1;ShowProp(*P,false);U.Len=FMath::Min(U.Len,U.Age+1.);}}else U.Next=0;}
  break;
 }
 case 14: // the frost goes off the glass; out of the truck, it is over at once
  if(!Driving()){U.Len=FMath::Min(U.Len,U.Age);break;}
  if(FS47Truck* T=M->World->CabTruck())T->SetFrost(.75*FMath::Clamp((U.Len-U.Age)/20.,0.,1.));
  break;
 case 15: // the lamp, at one spot
  if(M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(Seen(U.At,40,9)?.85:0.,TEXT("unease"));
  break;
 case 18: // in the truck's bed behind the cab, for a moment, where the mirror sees
  if(FS47Truck* T=M->World->CabTruck())if(FProp* P=Prop(TEXT("hitcher"))){PoseProp(*P,T->LocalToWorld(FVector(.1,.55,1.9)),T->Rotation.Y+UE_DOUBLE_PI);ShowProp(*P,U.Age<.6);}
  break;
 case 20: // they stand and look at SARO
 {
  FVector At;FProp* P=Prop(TEXT("stare"));
  if(P&&WorldOf(U.X,U.Z,At)){PoseProp(*P,At,U.Yaw);ShowProp(*P,FVector2D::Distance(FVector2D(At.X,At.Z),FVector2D(Feet.X,Feet.Z))<600);}
  break;
 }
 case 21: // the carcass in the lane until the truck is past it
 {
  FVector At;FProp* P=Prop(TEXT("carcass"));
  if(P&&WorldOf(U.X,U.Z,At)){PoseProp(*P,At,U.Yaw);ShowProp(*P,U.Stage==0);}
  if(const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr)
  {
   double X=0,Z=0;
   if(RoadOfWorld(D->Pos,X,Z)&&(Z-U.Z)*U.Dir.Z>30&&U.Stage==0){U.Stage=1;U.Len=FMath::Min(U.Len,U.Age+1.);}
  }
  break;
 }
 case 23: // the dark car behind, matching speed, until it turns off and is gone
 {
  const FS47DriveController* D=Driving()?M->World->CabDrive():nullptr;FProp* P=Prop(TEXT("darkcar"));
  if(D&&P)
  {
   const FVector Fwd(-FMath::Sin(D->Heading),0,-FMath::Cos(D->Heading)),Side(-Fwd.Z,0,Fwd.X);
   const double Off=U.Age<20?0.:FMath::Square(FMath::Min(1.,(U.Age-20)/3.))*16.;
   FVector At=D->Pos-Fwd*(32+FMath::Max(0.,U.Age-20)*4)+Side*Off;
   if(M->Player->Floor){const double Fl=M->Player->Floor(At.X,At.Z);if(FMath::IsFinite(Fl))At.Y=Fl;}
   PoseProp(*P,At,D->Heading+(U.Age>20?.5:0.));ShowProp(*P,U.Age<24);
  }
  else if(P)ShowProp(*P,false);
  break;
 }
 default:break;
 }
 if(U.Age>=U.Len)EndUneasy();
}
void FS47Happenings::EndUneasy()
{
 const int32 I=Uneasy.I;const bool bForced=Uneasy.bForced;
 Uneasy=FUneasy();
 US47Audio* A=M&&IsValid(M->Audio)?M->Audio.Get():nullptr;
 switch(I)
 {
 case 0:if(A)A->Stop(TEXT("uneaseAlarm"),.05f);break;
 case 5:if(A)A->Stop(TEXT("uneaseTap"),.4f);break;
 case 9:if(A)A->HoldRoomTone(1.f,1.5f);break;
 // Off whichever truck it is on now, in the cab or parked (as S47Away has it), and the hand with it.
 case 14:if(M&&IsValid(M->World))if(FS47Truck* T=M->World->CabTruck()?M->World->CabTruck():M->World->TruckHere()){T->SetFrost(0);T->SetFrostHand(false);}break;
 case 15:if(M&&M->Night&&M->Night->Flashlight)M->Night->Flashlight->Flicker(0,TEXT("unease"));break;
 case 16:
  if(bClockHeld&&M&&M->Inter)if(FS47Look* L=M->Inter->Looks.Find(TEXT("clock"))){L->Say=ClockSay;L->Range=ClockRange;}
  bClockHeld=false;if(A)A->HoldRoomTone(1.f,.6f);break;
 default:break;
 }
 for(const TCHAR* K:{TEXT("window"),TEXT("stare"),TEXT("carcass"),TEXT("darkcar"),TEXT("dome"),TEXT("strip")})if(FProp* P=FindProp(K))ShowProp(*P,false);
 if(I==18&&Now.Id!=TEXT("hitchhiker"))if(FProp* P=FindProp(TEXT("hitcher")))ShowProp(*P,false);
 // The short rest after it, as after anything else (not while the game is stopping).
 if(I>=0&&!bForced&&Life.IsValid()&&M&&M->Rhythm)M->Rhythm->Ended(TEXT("unease"));
}
bool FS47Happenings::DevUnease(int32 I,FString& Why){return FireUnease(I,true,Why);}

// ---------------------------------------------------------------- the counter (GEIGER-20261108)
bool FS47Happenings::ToggleCounter()
{
 if(!GeigerBuilt()||!M||M->ModalOpen()||Driving())return false;
 // Held up: it stays up. Put away: it stays down, even where it climbs, until it is held up again.
 bGeigerHeld=!bGeigerHeld;bGeigerAway=!bGeigerHeld;
 if(M->Audio){FS47Sound O;O.Gain=.2f;O.Rate=1.3f;M->Audio->Sfx(TEXT("switch"),O);}
 return true;
}
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
 // Hot in the middle, and faint a long way out, so that walking the right way the ticks come faster: it leads to what nobody saw.
 for(const FSite& S:Sites)if(S.Kind==TEXT("landing")&&!S47HapRakedSite(M,S.Place,S.Night)){const FVector At=Road(S.X,S.Z);Add(At,1.1f,5.,FString::Printf(TEXT("landing:%d"),S.Serial));Add(At,.16f,110.,FString::Printf(TEXT("landing:%d"),S.Serial));}
 // The dead cow's ring, a little and a little further out; the truck's warm hood, a little, the night it was warm.
 for(const FSite& S:Sites)if(S.Kind==TEXT("carcass")){const FVector At=Road(S.X,S.Z);Add(At,.3f,5.,TEXT("carcass"));Add(At,.07f,70.,TEXT("carcass"));}
 auto* F=M->Freeform.Get();
 if(HoodNight==F->Day()&&M->World)if(FS47Truck* T=M->World->TruckHere())Add(T->Position,.3f,3.,TEXT("hood"));
 // Marks left at a site by a visit, as the magnetometer has them (FS47Places::StepNeedle): in the frame the player stands in.
 if(F->FarmSite&&F->FarmSite->Armed()&&!F->FarmSite->Raked())Add(F->FarmSite->Centre(),.35f,8.,FString::Printf(TEXT("marks:%d"),F->Day()));
 // The marks a landing_trace left, where the Director put them (its anchor, in its own scene), while they lie there.
 // Gone over by the military first (S47Visitors::Cleared): nothing there for the counter either.
 if(F->Director&&F->Director->ActiveEvent()==TEXT("landing_trace")&&F->Director->AnchorHere()&&!F->Director->TraceRaked())
 {
  const FVector At=F->Director->EventAnchor();
  Add(At,.45f,5.,FString::Printf(TEXT("trace:%d"),F->Day()));Add(At,.08f,60.,FString::Printf(TEXT("trace:%d"),F->Day()));
 }
 // Places a bearing has pointed at: a little, and steady.
 if(F->Places)for(const FS47Places::FBearing& B:F->Places->Bearings())
 {
  double X=0,Z=0;
  // LOOP-4-20261109: only while the bearing is live: a cold one (cold by noon) gives nothing (FS47Places::Dosed).
  if(FS47Places::Dosed(B,F->Clock())&&F->Places->Spot(B.Target,X,Z))Add(Road(X,Z),.22f,10.,TEXT("place:")+B.Target);
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
 const double TickRate=FMath::Min(70.,.6+55.*Dose);
 TickDebt+=TickRate*Dt;int32 Clicks=0;
 while(TickDebt>=1&&Clicks<10)
 {
  TickDebt-=1;++Clicks;
  if(M->Audio)M->Audio->Beep(2200.f+FMath::FRand()*1800.f,.0025f,.035f+.03f*FMath::Min(1.f,Dose),FMath::FRand()*float(Dt),TEXT("square"));
 }
 TickDebt=FMath::Min(TickDebt,3.);
 const float Want=FMath::Clamp(float(FMath::LogX(10.,1.+60.*Dose)/FMath::LogX(10.,61.)),0.f,1.f);
 Needle+=(Want-Needle)*FMath::Min(1.f,float(Dt)*5.f)+(FMath::FRand()-.5f)*.025f*(1.f+Dose);
 Needle=FMath::Clamp(Needle,0.f,Dose>.85f?1.f:.97f);
 if((Dose>.07f&&!bGeigerAway)||bGeigerHeld)GeigerUpFor=4;else GeigerUpFor-=Dt;
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
  Page=FString::Printf(TEXT("The counter at the scorched ring out by %s: it climbs from the edge and pegs in the middle, and a long way out it is still above the odd tick."),Ring?*Ring->Title:TEXT("the land"));
  // Pegged in the ring: a jar of the soil, which Dr. Ward pays for.
  const FString Jar=FString::Printf(TEXT("geiger_sample_%d"),N);
  if(Dose>.85f&&!F->HasFinding(Jar))
  {
   const int32 Paid=F->EarnSide(10,TEXT("geiger jar"));
   F->RecordEvidence(Jar,S47HapPay::JarFound(Paid,Ring?Ring->Title:FString(TEXT("the land"))));
   if(M->UI)M->UI->Toast(S47HapPay::JarToast(Paid),5);
  }
 }
 else if(What.StartsWith(TEXT("marks:"))){Key=TEXT("geiger_")+What.Replace(TEXT(":"),TEXT("_d"));Page=TEXT("The counter over the marks: well above background, highest in the middle of them.");}
 else if(What.StartsWith(TEXT("trace:")))
 {
  const FString Place=F->Director?F->Director->EventPlace():FString();
  const FString Title=Place.IsEmpty()||!F->Director?FString(TEXT("the marked ground")):F->Director->LightPlaceTitle(Place);
  Key=TEXT("geiger_trace_d")+What.RightChop(6);
  Page=FString::Printf(TEXT("The counter over the marks at %s: above background, highest in the middle of them, falling off within a few paces."),*Title);
 }
 else if(What.StartsWith(TEXT("place:")))
 {
  const FString Id=What.RightChop(6);const FS47Places::FDef* D=FS47Places::Def(Id);
  Key=TEXT("geiger_place_")+Id;
  Page=FString::Printf(TEXT("The counter at %s, where a bearing pointed: a little above background, and steady."),D?D->Title:TEXT("a place"));
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
   // The needle, about its pivot low on the dial (its At): full left at nothing, full right where it pegs.
   const double A=FMath::Lerp(1.05,-1.05,double(Needle));
   const FQuat R=S47Three::Euler(0,0,A,TEXT("XYZ"));
   Local=Pt.At+R.RotateVector(FVector(0,Pt.Size.Y*.5,0));Turn=R;
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
 const bool bGround=!S47Hap::Background(Id);
 if(bGround&&!Now.Id.IsEmpty())EndEvent(false);
 if(Id==TEXT("cattle_ring"))Sites.RemoveAll([](const FSite& S){return S.Kind==TEXT("cattle");});
 if(Id==TEXT("stranded_rv"))Sites.RemoveAll([](const FSite& S){return S.Kind==TEXT("rv");});
 if(Id==TEXT("cb_stranded"))Sites.RemoveAll([](const FSite& S){return S.Kind==TEXT("truck");});
 if(Id==TEXT("cb_echo")&&Deeds.IsEmpty())Deed(TEXT("You stood out in the yard and looked at the antennas."));
 if(Id==TEXT("landing"))Sites.RemoveAll([](const FSite& S){return S.Kind==TEXT("landing");});
 if(!StartEvent(Id,Variant,true,Why))return false;
 // "ring": what a landing leaves, without waiting for it to come down; its bearing on the map as if it had been seen.
 if(Id==TEXT("landing")&&Variant==TEXT("ring")){Now.bSeen=true;Now.Stage=2;Now.Age=Now.Len;StepLanding(0);}
 return true;
}
bool FS47Happenings::LiveAt(FVector& Out) const
{
 if(Now.Id==TEXT("hitchhiker")||Now.Id==TEXT("humor")||Now.Id==TEXT("landing")||Now.Id==TEXT("big_night"))return WorldOf(Now.X,Now.Z,Out);
 if(Now.Id==TEXT("car_dies")||Now.Id==TEXT("dogs_hush")||Now.Id==TEXT("phone_empty")||Now.Id==TEXT("horizon_figure")||Now.Id==TEXT("room6_tv")||Now.Id==TEXT("dust_devil")||Now.Id==TEXT("car_light")||Now.Id==TEXT("figure_near")||Now.Id==TEXT("road_glimpse")){Out=Now.At;return true;}
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
  Check(H&&H->MinNights>=7&&H->Level!=ES47RhythmLevel::Danger&&H->FromDay>=7,TEXT("humour at most once a week, from the seventh night, and it is no danger"));
  Check(D.RestDanger>=120&&D.RestDanger<=240,FString::Printf(TEXT("the rest after a danger is a pause, not the night (%.0f s)"),D.RestDanger));
  // The build-up (critics, 8 November: the chances were flat through all thirty days): the first week is small things, the night's
  // danger grows week by week, and things come nearer.
  const FS47RhythmWeek& W1=D.Week(1);const FS47RhythmWeek& W2=D.Week(8);const FS47RhythmWeek& W4=D.Week(25);
  Check(D.Weeks.Num()>=4&&W1.Danger<W2.Danger&&W2.Danger<W4.Danger&&W1.Near>W2.Near&&W2.Near>W4.Near&&W1.Hint>=W1.Presence&&W4.Presence>W1.Presence,
   FString::Printf(TEXT("the curve builds: danger %.2f, %.2f, %.2f; near %.2f, %.2f, %.2f"),W1.Danger,W2.Danger,W4.Danger,W1.Near,W2.Near,W4.Near));
  // Within a night: the small things are drawn earlier than what is seen, and that earlier than what comes near.
  double Sum[3]={0,0,0};int32 N[3]={0,0,0};
  for(const FString& Id:Ids())if(const FS47RhythmEvent* E=D.Find(FName(*Id)))if(E->DrawFrom>0&&E->Level!=ES47RhythmLevel::Danger&&Id!=TEXT("humor"))
  {const int32 K=E->Level==ES47RhythmLevel::Hint?0:E->Level==ES47RhythmLevel::Observation?1:2;Sum[K]+=E->DrawFrom;++N[K];}
  const double H0=N[0]?Sum[0]/N[0]:0,H1=N[1]?Sum[1]/N[1]:0,H2=N[2]?Sum[2]/N[2]:0;
  Check(N[0]>3&&N[1]>3&&N[2]>=1&&H0<H1&&H1<H2,FString::Printf(TEXT("each night builds toward their hours: small things from %.1f, what is seen from %.1f, what comes near from %.1f on average"),H0,H1,H2));
  const FS47RhythmEvent* Fig=D.Find(TEXT("figure_near"));
  Check(Fig&&Fig->Level==ES47RhythmLevel::Danger&&Fig->Needs.Contains(FName(TEXT("on_foot")))&&Fig->FromDay>=8,TEXT("there is a danger for a player on foot, from the second week"));
  // GLIMPSE-ROAD-20261109: the crossing far ahead is a glimpse, at the wheel, at most once a week, and no danger.
  const FS47RhythmEvent* Cross=D.Find(TEXT("road_glimpse"));
  Check(Cross&&Cross->Tier==ES47Allowed::Glimpse&&!Cross->bEarly&&Cross->MinNights>=7&&Cross->Needs.Contains(FName(TEXT("driving")))&&Cross->Level!=ES47RhythmLevel::Danger,TEXT("something crossing the road far ahead is a glimpse, at the wheel, at most once a week"));
 }
 else Lines.Add(TEXT("PASS happenings: no director.json beside this build to read (packaged runs read the embedded one)"));
 // Words: the real ones, read from the source beside this build (as Predecessor reads its own tables): every literal with a space in
 // it that is not a log line or a path. Nothing named, no make or brand, American spelling, nothing from Story's far side.
 {
  int32 Read=0;TArray<FString> Bad,Tells;
  for(const TCHAR* File:{TEXT("Signal47/Private/S47Happenings.cpp"),TEXT("Signal47/Private/S47HappeningsDev.cpp"),TEXT("Signal47/Private/S47BigNight.cpp"),TEXT("Signal47/Private/S47RoadGlimpse.cpp")})
  {
   FString Src;if(!FFileHelper::LoadFileToString(Src,*(FPaths::GameSourceDir()/File)))continue;
   TArray<FString> SrcLines;Src.ParseIntoArrayLines(SrcLines,false);
   for(const FString& L:SrcLines)
   {
    // The lists of what is not said, the log, and the test's own words are not what the player is told.
    if(L.Contains(TEXT("FarSide"))||L.Contains(TEXT("NotSaid"))||L.Contains(TEXT("Concludes"))||L.Contains(TEXT("UE_LOG"))||L.Contains(TEXT("Check("))||L.Contains(TEXT("Bad.Add"))||L.Contains(TEXT("Tells.Add")))continue;
    for(int32 At=L.Find(TEXT("TEXT(\""));At!=INDEX_NONE;At=L.Find(TEXT("TEXT(\""),ESearchCase::CaseSensitive,ESearchDir::FromStart,At+6))
    {
     int32 Close=At+6;while(Close<L.Len()&&!(L[Close]==TEXT('"')&&L[Close-1]!=TEXT('\\')))++Close;
     const FString Lit=L.Mid(At+6,Close-At-6);
     if(!Lit.Contains(TEXT(" "))||Lit.StartsWith(TEXT("S47:"))||Lit.StartsWith(TEXT("/"))||Lit.Len()<8)continue;
     // Dale's own call sign on his hails and the speaker tag in front of his lines name him as the radio does (S47FieldRadio); every
     // other line keeps "Dale" out.
     const FString Said=Lit.Replace(TEXT("SARO, Dale."),TEXT("SARO.")).Replace(TEXT("DALE: "),TEXT(""));
     ++Read;if(!S47Hap::Clean(Said)&&Bad.Num()<4)Bad.Add(Lit.Left(60));
     if(S47Hap::Concluding(Lit)&&Tells.Num()<4)Tells.Add(Lit.Left(60));
    }
   }
  }
  if(Read==0)Lines.Add(TEXT("PASS happenings: no source beside this build to read its words from"));
  else Check(Bad.IsEmpty(),FString::Printf(TEXT("all %d of the words they say name nobody, no make or brand, in American spelling%s"),Read,Bad.IsEmpty()?TEXT(""):*(TEXT(": ")+FString::Join(Bad,TEXT(" | ")))));
  Check(Read==0||Tells.IsEmpty(),FString::Printf(TEXT("nothing they say gives the player the answer, or another system's motif%s"),Tells.IsEmpty()?TEXT(""):*(TEXT(": ")+FString::Join(Tells,TEXT(" | ")))));
  Check(!S47Hap::Clean(TEXT("Blue Chevy pickup."))&&!S47Hap::Clean(TEXT("a flat tyre"))&&!S47Hap::Clean(TEXT("a thermos on the dash"))&&S47Hap::Clean(TEXT("Blue half-ton pickup, county plates."))&&S47Hap::Clean(TEXT("Stafford's gray barn")),TEXT("the filter knows makes and spelling as whole words"));
 }
 {
  // URO.md: all 25, in its order, each its own id, and the two strong ones are the ones kept to once a game.
  TSet<FString> UIds;bool bUClean=true;
  for(int32 I=0;I<UneaseCount();++I){UIds.Add(UneaseId(I));bUClean&=S47Hap::Clean(UneaseTitle(I));}
  Check(UneaseCount()==25&&UIds.Num()==25&&FString(UneaseId(2))==TEXT("name")&&FString(UneaseId(19))==TEXT("note")&&bUClean,TEXT("the unease list has its 25, each once, nr. 3 and nr. 20 where URO.md has them"));
 }
 // What a night leaves counts for something (cases, the field record, the assessment); humour and pay are not records.
 Check(IsNightFinding(TEXT("cattle_ring_d4"))&&IsNightFinding(TEXT("geiger_landing_3"))&&!IsNightFinding(TEXT("humor_car_d8"))&&!IsNightFinding(TEXT("cb_helped_d2"))&&NightFindingKind(TEXT("cattle_ring_d4"))==TEXT("cattle_ring")&&NightFindingKind(TEXT("geiger_landing_3"))==TEXT("geiger_landing"),TEXT("night records are known by their keys, and counted by kind"));
 Check(Spoken(TEXT("STATION 01"))==TEXT("the old survey station up the track")&&Spoken(TEXT("the Baca windmill"))==TEXT("the Baca windmill")&&Spoken(TEXT("the salt lick and trough"))==TEXT("the salt lick"),TEXT("a driver never says a sign's capitals, nor the whole of a long name"));
 // The sounds the recordings lack are made, each with something in it and nothing out of range.
 {
  bool bSound=true;
  for(const TCHAR* Name:{TEXT("whisper"),TEXT("laugh"),TEXT("wings"),TEXT("alarm")})
  {
   const TArray<float> S=S47Hap::Make(Name,48000.f);float Top=0;bool bFinite=true;
   for(const float V:S){Top=FMath::Max(Top,FMath::Abs(V));bFinite&=FMath::IsFinite(V);}
   bSound&=S.Num()>9000&&bFinite&&Top>.3f&&Top<=1.f;
  }
  Check(bSound,TEXT("a whisper, a far laugh, wings and the alarm are made, not written"));
 }
 // The models: Tom's own paths are one constant each, and every humorous one has a stand-in or skips itself.
 Check(FString(S47Hap::RvModel).StartsWith(TEXT("/Game/Licensed/"))&&FString(S47Hap::ConvertibleModel).StartsWith(TEXT("/Game/Licensed/"))&&FString(S47Hap::SingerModel).StartsWith(TEXT("/Game/Licensed/"))&&FString(S47Hap::HairyModel).StartsWith(TEXT("/Game/Licensed/"))&&FString(S47Hap::ZetModel).StartsWith(TEXT("/Game/Licensed/")),TEXT("Tom's own models stay out of Git, under /Game/Licensed, one path each"));
 Check(S47Hap::RvParts().Num()>8&&S47Hap::ZetParts().Num()>6&&S47Hap::RingParts().Num()>10&&S47Hap::CattleParts().Num()==14,TEXT("what stands in until they come is there"));
 // The compass, and a save round trip.
 Check(S47Hap::Bearing(47.2)==48.&&S47Hap::Bearing(146.6)==148.&&S47Hap::Bearing(-1)==359.&&S47Hap::Bearing(359.8)==0.&&S47Hap::Bearing(26)==26.,TEXT("a bearing is whole degrees and never Story's number"));
 Check(S47Hap::Compass(0)==TEXT("north")&&S47Hap::Compass(91)==TEXT("east")&&S47Hap::Compass(181)==TEXT("south")&&S47Hap::Compass(-89)==TEXT("west")&&S47Hap::Compass(359)==TEXT("north"),TEXT("a heading in words"));
 {
  FS47Happenings A(nullptr);A.Begin(nullptr);
  Check(A.Save()->Values.Num()==0&&A.Sites.IsEmpty(),TEXT("an older save has none of it and writes nothing"));
 {
  // LOOP-4-20261109: the needle has a history. Something out there near a place in the last two nights (standing there still, or
  // gone): it will not settle. Further off, or longer ago, it is calm; what has gone survives a Continue.
  const TArray<FStir> Near={{100,-400,6}};
  Check(Stirred(Near,250,-400,6)&&Stirred(Near,100,-100,8)&&!Stirred(Near,100,-400,9)&&!Stirred(Near,600,-400,6)&&!Stirred(Near,100,-400,5)&&!Stirred({},0,0,6),
   TEXT("the needle will not settle within 350 m of something the last two nights, and is calm further off, longer ago or with nothing"));
  FS47Happenings Gone(nullptr);Gone.Begin(nullptr);Gone.Past.Add({100,-400,6});
  FS47Happenings Back(nullptr);Back.Begin(S47J::Read(S47J::Write(Gone.Save())));
  Check(Back.Past.Num()==1&&Back.Past[0].Night==6&&FMath::IsNearlyEqual(Back.Past[0].Z,-400.)&&Stirred(Back.Past,120,-380,7),TEXT("what stood out there and has gone is kept for the needle over a Continue"));
  Gone.Stop();Back.Stop();
 }
  FSite S;S.Kind=TEXT("landing");S.Title=TEXT("the wash");S.Night=4;S.Serial=3;S.X=120;S.Z=-440;S.Az=31;S.Spread=12;S.Until=9e5;A.Sites.Add(S);
  FSite Bad;Bad.Kind=TEXT("nonsense");Bad.Night=2;A.Sites.Add(Bad);
  A.LastDays.Add(TEXT("landing"),4);A.Deeds.Add(TEXT("You walked round the burnt ring out by the wash."));A.bEchoArmed=true;A.EchoFrom=4;A.HumorNight=3;
  // As an older save has it: no word on whether the bearing is known (written since the bearing is set as it comes down).
  const S47J::FObj Older=A.Save();
  for(const TSharedPtr<FJsonValue>& V:Older->GetArrayField(TEXT("sites")))if(V.IsValid()&&V->Type==EJson::Object)V->AsObject()->RemoveField(TEXT("known"));
  FS47Happenings B(nullptr);B.Begin(S47J::Read(S47J::Write(Older)));
  Check(B.Sites.Num()==1&&B.Sites[0].Az==31.&&B.Sites[0].Title==TEXT("the wash")&&B.LastDays.FindRef(TEXT("landing"))==4&&B.Deeds.Num()==1&&B.bEchoArmed&&B.EchoFrom==4&&B.HumorNight==3,TEXT("sites, days, deeds and the armed voice survive a Continue, and nonsense is dropped"));
  Check(B.Sites.Num()==1&&B.Sites[0].bKnown,TEXT("an older save's landing with a bearing is known"));
  // And one saved now keeps what it said: a landing nobody saw stays off the map after a Continue.
  FS47Happenings Unseen(nullptr);Unseen.Begin(S47J::Read(S47J::Write(A.Save())));
  Check(Unseen.Sites.Num()==1&&!Unseen.Sites[0].bKnown&&Unseen.Sites[0].Az==31.,TEXT("a landing nobody saw keeps its bearing, off the map, across a Continue"));
  // What is under way when it is saved: a landing coming down carries on with its bearing; the woman by the road may be drawn again.
  FS47Happenings C(nullptr);C.Begin(nullptr);
  C.Now.Id=TEXT("landing");C.Now.Serial=3;C.Now.Age=14;C.Now.Len=36;C.Now.X=120;C.Now.Z=-440;C.Now.Yaw=31;C.Now.Stage=2;
  C.LateNight=5;C.MetTotal=2;C.UneaseLast.Add(7,4);C.HumorSeen.Add(TEXT("car"));
  FS47Happenings Back(nullptr);Back.Begin(S47J::Read(S47J::Write(C.Save())));
  Check(Back.Now.Id==TEXT("landing")&&Back.Now.Age==14.&&Back.Now.Yaw==31.&&Back.Now.Stage==2&&Back.LateNight==5&&Back.MetTotal==2&&Back.UneaseLast.FindRef(7)==4&&Back.HumorSeen.Num()==1,TEXT("a landing under way, the night's watch, dangers met and the unease of the last nights survive a Continue"));
  C.Now=FActive();C.Now.Id=TEXT("hitchhiker");C.Now.Len=240;C.Now.bDanger=true;
  FS47Happenings Again(nullptr);Again.Begin(S47J::Read(S47J::Write(C.Save())));
  Check(Again.Now.Id.IsEmpty()&&!Again.LastDays.Contains(TEXT("hitchhiker")),TEXT("one that cannot carry on is not saved half-done, and may be drawn again"));
 }
 // The morning (critics, 8 November): the night is drawn at eight, and a draw redrawn then was given an hour that never came.
 {
  const double Day3=2*86400.;
  const double Morning=NightHour(Day3+8*3600.),Ten=NightHour(Day3+22*3600.),Late=NightHour(Day3+86400.+3600.);
  const bool bMorning=Morning==8.&&Ten==22.&&Late==25.&&!S47Hap::StaleAt(Morning,22.)&&!S47Hap::StaleAt(NightHour(Day3+11.5*3600.),22.);
  // At SARO indoors at eight, a draw for the road at ten: not stale in the morning, due at ten; stale at half past eleven, its
  // replacement due at once and for its own hour and a half, never at an hour past the night.
  const double Swap=FMath::Max(23.5,22.);
  Check(bMorning&&S47Hap::DueAt(Ten,22.,22.+DrawWindow)&&S47Hap::StaleAt(23.5,22.)&&S47Hap::DueAt(23.5,Swap,Swap+DrawWindow)&&S47Hap::DueAt(Swap+1.4,Swap,Swap+DrawWindow)&&!S47Hap::DueAt(30.,29.,31.),
   TEXT("drawn at eight in the morning with the player indoors at SARO, a draw is there at ten and can start; one swapped late starts from then"));
 }
 {
  // Seen means lit: its own light, the lamp's beam near, the truck's lights ahead, the day, or against the sky. Not the dark.
  FLightOn Dark;Dark.Hour=2;
  FLightOn Lamp=Dark;Lamp.bLamp=true;Lamp.LampM=18;Lamp.LampDeg=8;
  FLightOn LampFar=Lamp;LampFar.LampM=96;
  FLightOn Cab=Dark;Cab.bHeadlights=true;Cab.CarM=60;Cab.CarDeg=10;
  FLightOn Behind=Cab;Behind.CarDeg=120;
  FLightOn Own=Dark;Own.bOwn=true;FLightOn Day=Dark;Day.Hour=13;FLightOn Sky=Dark;Sky.bSky=true;
  Check(!LitBy(Dark)&&LitBy(Lamp)&&!LitBy(LampFar)&&LitBy(Cab)&&!LitBy(Behind)&&LitBy(Own)&&LitBy(Day)&&LitBy(Sky),TEXT("seen in the dark only in a light: cattle at 96 m in the dark are not seen"));
 }
 {
  // Between eleven and one the reserve takes only what happens where the player is: never the camper, the cinema or a dead cow far off.
  bool bNear=true;FString Far;
  for(const TCHAR* Id:{TEXT("stranded_rv"),TEXT("carcass"),TEXT("cinema_film"),TEXT("missing_person"),TEXT("cb_stranded"),TEXT("landing"),TEXT("house_lit"),TEXT("array_burst")})if(NeedMask(Id)&S47Hap::NeedNear){bNear=false;Far+=FString(Id)+TEXT(" ");}
  for(const TCHAR* Id:{TEXT("dogs_hush"),TEXT("animals_flee"),TEXT("cattle_ring"),TEXT("car_dies"),TEXT("town_blackout")})if(!(NeedMask(Id)&S47Hap::NeedNear)){bNear=false;Far+=FString(Id)+TEXT(" ");}
  Check(bNear,TEXT("the reserve between 23 and 01 is something near the player ")+Far);
  // Every danger can be met somewhere a player is: on foot, at the wheel, at a CB.
  bool bFoot=false,bDrive=false,bAll=true;
  for(const FString& Id:Ids())if(IsDanger(Id)){const uint32 Need=NeedMask(Id);bAll&=Need!=0;bFoot|=(Need&S47Hap::NeedFoot)!=0;bDrive|=(Need&S47Hap::NeedDrive)!=0;}
  Check(bAll&&bFoot&&bDrive,TEXT("the night's dangers fit somewhere the player is: one on foot, one at the wheel"));
  // CB-TRUCK-20261109: the CB is under the truck's dash, so its calls are drawn only for a player sitting in the truck.
  Check(NeedMask(TEXT("cb_stranded"))==S47Hap::NeedDrive&&NeedMask(TEXT("cb_echo"))==S47Hap::NeedDrive&&(NeedMask(TEXT("missing_person"))&S47Hap::NeedSaro),
   TEXT("the stuck driver and the voice on the CB come only in the truck; the missing man's word can also come over Dale's walkie at SARO"));
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
 // GLIMPSE-ROAD-20261109: the crossing far ahead (S47RoadGlimpse), its prints, and the bodies figures may have.
 {
  Check(IsNightFinding(TEXT("road_glimpse_d5"))&&IsNightFinding(TEXT("road_tracks_d6"))&&NightFindingKind(TEXT("road_tracks_d6"))==TEXT("road_tracks"),TEXT("what crossed the road and its prints are night records"));
  Check(NeedMask(TEXT("road_glimpse"))==S47Hap::NeedDrive&&!(NeedMask(TEXT("road_glimpse"))&S47Hap::NeedNear)&&!IsDanger(TEXT("road_glimpse")),TEXT("the crossing far ahead is at the wheel, far off, and no danger"));
  FS47Happenings A(nullptr);A.Begin(nullptr);
  Check(!A.SwapOrder(9).Contains(TEXT("road_glimpse"))&&!A.SwapOrder(9).Contains(TEXT("humor")),TEXT("the rare ones are never drawn in another's place"));
  FSite S;S.Kind=TEXT("roadtracks");S.Variant=TEXT("low");S.Title=TEXT("the highway");S.Night=6;S.Serial=2;S.X=-6.5;S.Z=900;S.Until=6*86400.+20*3600.;A.Sites.Add(S);
  A.Now.Id=TEXT("horizon_figure");A.Now.Variant=TEXT("cosmic");A.Now.Len=120;A.Now.Age=3;A.Now.X=10;A.Now.Z=20;
  FS47Happenings B(nullptr);B.Begin(S47J::Read(S47J::Write(A.Save())));
  Check(B.Sites.Num()==1&&B.Sites[0].Kind==TEXT("roadtracks")&&B.Sites[0].Variant==TEXT("low")&&B.Sites[0].X==-6.5,TEXT("the prints on the shoulder are still there after a Continue"));
  Check(B.Now.Id==TEXT("horizon_figure")&&B.Now.Variant==TEXT("cosmic"),TEXT("a figure on the rise keeps the body it was given across a Continue"));
  A.Now=FActive();A.Now.Id=TEXT("road_glimpse");A.Now.Variant=TEXT("crawler");A.Now.Len=3;
  FS47Happenings C(nullptr);C.Begin(S47J::Read(S47J::Write(A.Save())));
  Check(C.Now.Id.IsEmpty(),TEXT("a crossing cut off by a save is not carried on"));
  Check(S47Hap::ShadeKind(FString())==TEXT("shade")&&S47Hap::ShadeKind(TEXT("nordic"))==TEXT("glimpse_nordic")&&S47Hap::RoadTrackParts(true).Num()>S47Hap::RoadTrackParts(false).Num(),TEXT("the shade, or Tom's figure by name; prints in fours and in strides"));
 }
 // CRIT6-20261109: what the field pays says what came of it, paid, part paid or past the week's cap.
 {
  using namespace S47HapPay;
  const bool bFull=Short(20,20,TEXT("x")).IsEmpty()&&ReportFound(20,TEXT("the wash")).Contains(TEXT("came through"))&&ReportCb(20,TEXT("the wash")).Contains(TEXT("come through your station"))
   &&JarFound(10,TEXT("the ring")).Contains(TEXT("$10"))&&JarToast(10).Contains(TEXT("$10"))&&RigPaid(15).Contains(TEXT("fifteen dollars into your hand"))&&RigFound(15,TEXT("the road")).Contains(TEXT("$15"));
  bool bNone=true;
  for(const FString& T:{Short(0,20,TEXT("x")),ReportFound(0,TEXT("the wash")),JarFound(0,TEXT("the ring")),JarToast(0),RigPaid(0),RigFound(0,TEXT("the road"))})bNone&=T.Contains(TEXT("field money"))&&!T.Contains(TEXT("$"));
  bNone&=!ReportCb(0,TEXT("the wash")).Contains(TEXT("come through"));
  const bool bPart=Short(5,20,TEXT("x")).Contains(TEXT("$5 of the $20"))&&ReportFound(5,TEXT("the wash")).Contains(TEXT("$5"))&&RigPaid(5).Contains(TEXT("$5"))&&JarFound(4,TEXT("the ring")).Contains(TEXT("$4"));
  Check(bFull&&bNone&&bPart,TEXT("CRIT6: the pickup's reward, the geiger jar and the stuck rig say what they paid, and past the week's cap say so with no dollar promised"));
 }
 OK&=S47RoadGlimpse::SelfTest(Lines);
 // M7: the first big night's plan, its save and its words (S47BigNight.cpp).
 OK&=BigSelfTest(Lines);
 return OK;
}
