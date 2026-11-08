#include "S47EventPlaces.h"
#include "S47CalicheAccess.h"
#include "S47FarmWorld.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Geo.h"
#include "S47Inter.h"
#include "S47Legs.h"
#include "S47Main.h"
#include "S47NightRhythm.h"
#include "S47OpenLand.h"
#include "S47Places.h"
#include "S47Relief.h"
#include "S47RoadShape.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"
#include <limits>

DEFINE_LOG_CATEGORY_STATIC(LogS47EventPlaces,Log,All);

namespace S47EventPlacesLocal
{
 using ET=ES47PlaceTrait;
 constexpr ET Indoors=ET::Indoors,Door=ET::Door,Window=ET::Window,Phone=ET::Phone,Road=ET::Road,Open=ET::Open,Far=ET::Far,People=ET::People,Farm=ET::Farm,Town=ET::Town,Saro=ET::Saro,Wash=ET::Wash,Lot=ET::Lot,Array=ET::Array;
 // Road-local metres. X,Z: outside, on the ground, where lights hang and marks are left. EX,EZ: the door or the way in.
 struct FSeed{const TCHAR* Id;const TCHAR* Title;const TCHAR* Region;double X,Z,EX,EZ;const TCHAR* Door;const TCHAR* Prop;ET Traits;double Radius,Up;};
 // SARO's own metres to the land's: the station's area is the land moved by SARO_IN_ROAD.
 constexpr double SX=24,SZ=-300;
 const TArray<FSeed>& Seeds()
 {
  static const TArray<FSeed> Table={
   // The ranch (S47FarmBuildings, S47FarmHouse): the pasture the marks have always been left in, and the house with its doors.
   {TEXT("farm"),TEXT("the Alderete place"),TEXT("farm"),60,240,98,213,TEXT(""),TEXT(""),Farm|Open|People,40,20},
   {TEXT("oldhouse"),TEXT("the old house on the Alderete place"),TEXT("farm"),98,208,98,215,TEXT("house"),TEXT(""),Farm|Indoors|Door|Window,10,14},
   {TEXT("barn"),TEXT("the Alderete barn"),TEXT("farm"),128,160,136,166,TEXT(""),TEXT(""),Farm|Open,14,16},
   // The station itself: the building, its east yard, the array.
   {TEXT("saro"),TEXT("the SARO building"),TEXT("saro"),7+SX,19+SZ,0+SX,-1.2+SZ,TEXT(""),TEXT(""),Saro|Indoors|Window|Phone|People|Lot,18,26},
   {TEXT("saro_yard"),TEXT("the SARO yard"),TEXT("saro"),12+SX,-12+SZ,9+SX,0+SZ,TEXT(""),TEXT(""),Saro|Open|Lot,10,24},
   {TEXT("array"),TEXT("the array at SARO"),TEXT("saro"),-3.25+SX,-47+SZ,-3.25+SX,-29+SZ,TEXT(""),TEXT(""),Saro|Array|Open,45,24},
   // Motel: measured off its lot below.
   {TEXT("motel"),TEXT("the Sierra Motor Court"),TEXT("motel"),0,0,0,0,TEXT(""),TEXT(""),Indoors|Window|Phone|People|Road|Lot,20,22},
   // Caliche (CalicheAccess assembly, Town.ts): in front of each on the street, and its door. Measured live from the rooms below.
   {TEXT("gas"),TEXT("the gas station"),TEXT("caliche"),-4,2066,-8,2066,TEXT(""),TEXT("town:gasDoor"),Town|Indoors|Window|Phone|People|Road|Lot,12,28},
   {TEXT("sheriff"),TEXT("the sheriff's substation"),TEXT("caliche"),-9,2108,-16,2110.4,TEXT("Sheriff"),TEXT(""),Town|Indoors|Door|Window|Phone|People|Road,10,28},
   {TEXT("laundry"),TEXT("the laundromat"),TEXT("caliche"),-9,2129,-15,2128.6,TEXT("Laundry"),TEXT(""),Town|Indoors|Door|Window|Road,10,28},
   {TEXT("cinema"),TEXT("the Plaza cinema"),TEXT("caliche"),-9,2164,-15,2164.6,TEXT("Cinema"),TEXT("town:cinema"),Town|Indoors|Door|Window|Road,12,28},
   {TEXT("feed"),TEXT("the feed store"),TEXT("caliche"),-8,2202,-15,2202.5,TEXT("Feed"),TEXT("town:feed"),Town|Indoors|Door|Window|Road|Lot,14,28},
   {TEXT("church"),TEXT("the church"),TEXT("caliche"),14,2095.5,21,2095.5,TEXT("Church"),TEXT("town:churchDoor"),Town|Indoors|Door|Window|Phone,12,28},
   {TEXT("souvenir"),TEXT("the souvenir shop"),TEXT("caliche"),14,2061,20.5,2061.2,TEXT("Souvenir"),TEXT(""),Town|Indoors|Door|Window,8,26},
   {TEXT("radio"),TEXT("the 1420 AM studio"),TEXT("caliche"),10,2036,17,2036,TEXT(""),TEXT(""),Town|Indoors|Window|Phone|People,8,26},
   {TEXT("h01"),TEXT("the empty house on the east side of town"),TEXT("caliche"),18,2130,24,2129.6,TEXT("H01"),TEXT(""),Town|Indoors|Door|Window,8,24},
   {TEXT("h02"),TEXT("the second empty house on the east side"),TEXT("caliche"),18,2154,24.5,2153.6,TEXT("H02"),TEXT(""),Town|Indoors|Door|Window,8,24},
   {TEXT("h03"),TEXT("the last house on the east side of town"),TEXT("caliche"),19,2188,25,2188.4,TEXT("H03"),TEXT(""),Town|Indoors|Door|Window,8,24},
   {TEXT("h04"),TEXT("the empty house at the south end of town"),TEXT("caliche"),-13,2238,-19,2237.6,TEXT("H04"),TEXT(""),Town|Indoors|Door|Window,8,24},
   {TEXT("diner"),TEXT("the Mesa Diner"),TEXT("diner"),-6,2004,-14,2000,TEXT(""),TEXT(""),Indoors|Window|Phone|People|Road|Lot,14,26},
   // The other station, at the end of its track: measured from its gate below.
   {TEXT("station01"),TEXT("STATION 01"),TEXT("station"),0,0,0,0,TEXT(""),TEXT(""),Far|Open|Lot|Road,20,22},
   // Out on the land (FS47Places): their own titles for a light over them, which say where, not what was found there.
   {TEXT("windmill"),TEXT("the windmill on the Baca lease"),TEXT("land"),170,720,170,720,TEXT(""),TEXT("ff:place:windmill"),Open|Far|Farm,10,22},
   {TEXT("tank"),TEXT("the water tank on the old grade"),TEXT("land"),-190,1010,-190,1010,TEXT(""),TEXT("ff:place:tank"),Open|Far|Farm,10,22},
   {TEXT("mine"),TEXT("the old mine on the west side"),TEXT("land"),-560,1340,-560,1340,TEXT(""),TEXT("ff:place:mine"),Open|Far|Wash,12,22},
   {TEXT("balloon"),TEXT("the open ground east of the highway"),TEXT("land"),300,1270,300,1270,TEXT(""),TEXT("ff:place:balloon"),Open|Far,14,22},
   {TEXT("cow"),TEXT("the fence past the turnoff"),TEXT("land"),110,1590,110,1590,TEXT(""),TEXT("ff:place:cow"),Open|Far|Farm,10,22},
   {TEXT("shack"),TEXT("the line shack"),TEXT("land"),440,930,440,930,TEXT(""),TEXT("ff:place:shack"),Open|Far,8,22},
   {TEXT("cairn"),TEXT("the cairn on the rise"),TEXT("land"),-320,430,-320,430,TEXT(""),TEXT("ff:place:cairn"),Open,8,22},
   {TEXT("stake"),TEXT("the survey stake off the track"),TEXT("land"),-130,650,-130,650,TEXT(""),TEXT("ff:place:stake"),Open|Far,8,22},
   {TEXT("trough"),TEXT("the salt lick and trough"),TEXT("land"),240,1740,240,1740,TEXT(""),TEXT("ff:place:trough"),Open|Far|Farm,10,22},
   // Open land with nothing built on it: where something comes down, where somebody is stuck. Seated off the hills below.
   {TEXT("land:wash_west"),TEXT("the dry wash under the west mesa"),TEXT("land"),-900,1150,-900,1150,TEXT(""),TEXT(""),Open|Far|Wash,30,22},
   {TEXT("land:wash_mine"),TEXT("the arroyo below the old mine"),TEXT("land"),-650,1650,-650,1650,TEXT(""),TEXT(""),Open|Far|Wash,30,22},
   {TEXT("land:wash_east"),TEXT("the dry wash out past the line shack"),TEXT("land"),750,1100,750,1100,TEXT(""),TEXT(""),Open|Far|Wash,30,22},
   {TEXT("land:flat_north"),TEXT("the flat north of the station"),TEXT("land"),250,-1100,250,-1100,TEXT(""),TEXT(""),Open|Far,40,22},
   {TEXT("land:playa"),TEXT("the playa east of the Baca lease"),TEXT("land"),900,500,900,500,TEXT(""),TEXT(""),Open|Far,40,22},
   {TEXT("land:far_south"),TEXT("the open ground past the south end of town"),TEXT("land"),300,3000,300,3000,TEXT(""),TEXT(""),Open|Far,40,22},
   {TEXT("land:far_west"),TEXT("the far west flats"),TEXT("land"),-1800,800,-1800,800,TEXT(""),TEXT(""),Open|Far,40,22},
   {TEXT("land:far_east"),TEXT("the bench under the east rim"),TEXT("land"),1700,2200,1700,2200,TEXT(""),TEXT(""),Open|Far|Wash,40,22},
   // Beside the road: a pull-off, a turnoff, a cattle guard. Somewhere a car stops.
   {TEXT("road:pullout_north"),TEXT("the highway pull-off north of SARO"),TEXT("land"),18,-900,18,-900,TEXT(""),TEXT(""),Road|Lot|Open,12,22},
   {TEXT("road:turnoff"),TEXT("the county road turnoff"),TEXT("land"),18,520,18,520,TEXT(""),TEXT(""),Road|Lot|Open,12,22},
   {TEXT("road:cattle_guard"),TEXT("the cattle guard on the highway"),TEXT("land"),-18,1450,-18,1450,TEXT(""),TEXT(""),Road|Open|Far,10,22},
   {TEXT("road:pullout_south"),TEXT("the highway pull-off south of Caliche"),TEXT("land"),18,2650,18,2650,TEXT(""),TEXT(""),Road|Lot|Open|Far,12,22},
  };
  return Table;
 }
 FS47EventPlace Make(const FSeed& S)
 {
  FS47EventPlace P;P.Id=S.Id;P.Title=S.Title;P.Region=S.Region;P.X=S.X;P.Z=S.Z;P.EntryX=S.EX;P.EntryZ=S.EZ;P.DoorKey=S.Door;P.Prop=S.Prop;P.Traits=S.Traits;P.Radius=S.Radius;P.Up=S.Up;
  return P;
 }
 bool SameLand(const FString& A,const FString& B)
 {
  const auto Town=[](const FString& X){return X==TEXT("road")||X==TEXT("diner");};
  const auto Station=[](const FString& X){return X==TEXT("saro")||X==TEXT("motel");};
  return A==B||(Town(A)&&Town(B))||(Station(A)&&Station(B));
 }
 uint32 Salted(const FString& Kind,uint32 Salt){return FCrc::StrCrc32(*Kind)^(Salt*0x9e3779b1u);}
 // Nearer than this is somewhere the player is or is going; past it, somewhere they hear of.
 constexpr double FarFrom=900;
 const double Nan=std::numeric_limits<double>::quiet_NaN();
}

void FS47EventPlaces::Begin(const S47J::FObj& Saved)
{
 Uses.Reset();
 if(!Saved.IsValid())return;
 if(const auto U=S47J::Obj(Saved,TEXT("used")))for(const auto& Pair:U->Values)
 {
  const S47J::FObj* O=nullptr;if(!Pair.Value.IsValid()||!Pair.Value->TryGetObject(O)||!O)continue;
  FUse Use;Use.Night=FMath::Clamp(S47J::Int(*O,TEXT("night")),0,100000);Use.Count=FMath::Clamp(S47J::Int(*O,TEXT("count")),0,100000);Use.Kind=S47J::Str(*O,TEXT("kind")).Left(64);
  if(Use.Night>0)Uses.Add(FString(*Pair.Key).Left(64),Use);
 }
}

S47J::FObj FS47EventPlaces::Save() const
{
 auto O=S47J::New();
 if(Uses.IsEmpty())return O;
 auto U=S47J::New();
 TArray<FString> Keys;Uses.GetKeys(Keys);Keys.Sort();
 for(const FString& K:Keys){const FUse& Use=Uses[K];auto E=S47J::New();S47J::Set(E,TEXT("night"),Use.Night);S47J::Set(E,TEXT("count"),Use.Count);if(!Use.Kind.IsEmpty())S47J::Set(E,TEXT("kind"),Use.Kind);S47J::Set(U,*K,E);}
 S47J::Set(O,TEXT("used"),U);
 return O;
}

const TArray<FS47EventPlace>& FS47EventPlaces::All() const
{
 using namespace S47EventPlacesLocal;
 if(List.IsEmpty())
 {
  // Room for what is measured later (the feeder): pointers handed out stay good.
  List.Reserve(Seeds().Num()+8);
  for(const FSeed& S:Seeds())List.Add(Make(S));
  // The motel's lot and STATION 01's gate are their own terms' numbers: on the land's here.
  if(FS47EventPlace* Motel=List.FindByPredicate([](const FS47EventPlace& P){return P.Id==TEXT("motel");}))
  {
   const FS47Box2& Park=S47Legs::MOTEL_PARK();double RX=0,RZ=0;
   if(S47OpenLand::RoadOf(TEXT("saro"),(Park.MinX+Park.MaxX)*.5,(Park.MinZ+Park.MaxZ)*.5,RX,RZ)){Motel->X=RX;Motel->Z=RZ;}
   if(S47OpenLand::RoadOf(TEXT("saro"),Park.MinX-1.2,Park.MinZ+2,RX,RZ)){Motel->EntryX=RX;Motel->EntryZ=RZ;}
  }
  if(FS47EventPlace* Station=List.FindByPredicate([](const FS47EventPlace& P){return P.Id==TEXT("station01");}))
  {
   const S47Geo::FXZ Gate=S47Geo::StationToRoad(S47Geo::STATION_TRUCK.X,S47Geo::STATION_TRUCK.Z);
   Station->X=Station->EntryX=Gate.X;Station->Z=Station->EntryZ=Gate.Z;
  }
 }
 Refine();
 return List;
}

void FS47EventPlaces::Refine() const
{
 // The land's hills are read from a file: until then the open points stand where they were planned.
 if(!bLandSeated&&S47Relief::Ready())
 {
  bLandSeated=true;
  for(FS47EventPlace& P:List)if(P.Id.StartsWith(TEXT("land:"))||P.Id.StartsWith(TEXT("road:")))
  {
   const bool bRoadSide=P.Id.StartsWith(TEXT("road:"));
   bool bClear=false;const double X0=P.X,Z0=P.Z;
   for(double R=0;R<=260&&!bClear;R+=26)for(int32 I=0;I<(R>0?8:1)&&!bClear;++I)
   {
    const double X=X0+FMath::Cos(I*UE_DOUBLE_PI/4)*R,Z=Z0+FMath::Sin(I*UE_DOUBLE_PI/4)*R;
    if(!S47OpenLand::InRim(X,Z,20)||S47OpenLand::InSaroCore(X,Z,15)||S47OpenLand::InStationCore(X,Z,15)||S47Relief::HillStops(X,Z,5))continue;
    if(!bRoadSide&&FMath::Abs(X)<30)continue;
    P.X=P.EntryX=X;P.Z=P.EntryZ=Z;bClear=true;
   }
   UE_LOG(LogS47EventPlaces,Verbose,TEXT("S47: event place %s seated at road %.0f %.0f%s"),*P.Id,P.X,P.Z,bClear?TEXT(""):TEXT(" (no clear ground: as planned)"));
  }
 }
 // Caliche's rooms, as they were built: each door where it really is.
 if(!bTownMeasured)
 {
  bool bAny=false;
  for(FS47EventPlace& P:List)if(!P.DoorKey.IsEmpty()&&P.DoorKey!=TEXT("house"))
  {
   FVector Middle,Normal;
   if(!S47CalicheAccess::DoorAt(P.DoorKey,Middle,Normal))continue;
   double RX=0,RZ=0;if(!S47OpenLand::RoadOf(TEXT("road"),Middle.X,Middle.Z,RX,RZ))continue;
   P.EntryX=RX;P.EntryZ=RZ;bAny=true;
   // Out on the street in front of it: the side the door faces away from the room.
   FVector Room;
   if(S47CalicheAccess::RoomMiddle(P.DoorKey,Room))
   {
    FVector2D Out(Middle.X-Room.X,Middle.Z-Room.Z);if(!Out.Normalize())Out=FVector2D(Normal.X,Normal.Z).GetSafeNormal();
    P.X=RX+Out.X*6;P.Z=RZ+Out.Y*6;
   }
  }
  bTownMeasured=bAny;
 }
 // The places out on the land, as the world put them and as some of them drift; now and then, not on every question.
 const double Now=FPlatformTime::Seconds();
 if(M&&M->Freeform&&M->Freeform->Places&&Now>=NextMeasure)
 {
  NextMeasure=Now+2;
  for(FS47EventPlace& P:List)if(P.Prop.StartsWith(TEXT("ff:place:")))
  {
   double X=0,Z=0;if(M->Freeform->Places->Spot(P.Id,X,Z)){P.X=P.EntryX=X;P.Z=P.EntryZ=Z;}
  }
 }
 // The remote feeder, once the station's field has it. Within what was reserved, so nothing handed out moves.
 if(!bLiveMeasured&&M&&M->Freeform&&M->Freeform->Field&&List.Num()<List.Max())
 {
  const FVector At=M->Freeform->Field->InteractionPoint(TEXT("ff:feeder"));double RX=0,RZ=0;
  if(!At.IsZero()&&S47OpenLand::RoadOf(TEXT("road"),At.X,At.Z,RX,RZ))
  {
   FS47EventPlace F;F.Id=TEXT("feeder");F.Title=TEXT("the remote feeder on the county road");F.Region=TEXT("station");F.X=F.EntryX=RX;F.Z=RZ+6;F.EntryZ=RZ;
   F.Traits=ES47PlaceTrait::Road|ES47PlaceTrait::Open|ES47PlaceTrait::Far;F.Radius=8;F.Up=20;List.Add(F);bLiveMeasured=true;
  }
 }
}

const FS47EventPlace* FS47EventPlaces::Find(const FString& Id) const
{
 return All().FindByPredicate([&](const FS47EventPlace& P){return P.Id==Id;});
}

const FS47EventPlace* FS47EventPlaces::WithDoor(const FString& DoorKey) const
{
 if(DoorKey.IsEmpty())return nullptr;
 return All().FindByPredicate([&](const FS47EventPlace& P){return P.DoorKey==DoorKey||(!P.DoorKey.IsEmpty()&&S47CalicheAccess::SameDoor(P.DoorKey,DoorKey));});
}

FString FS47EventPlaces::TitleOf(const FString& Id) const
{
 const FS47EventPlace* P=Find(Id);
 return P?P->Title:FString(TEXT("somewhere out there"));
}

FString FS47EventPlaces::AreaOf(const FS47EventPlace& P)
{
 // As the land itself says, from the road's side: SARO's and the station's grounds are only what stands closest round them.
 return S47OpenLand::LegFor(TEXT("road"),P.X,P.Z);
}

bool FS47EventPlaces::PlayerRoad(FVector2D& Out) const
{
 if(!M||!M->Player||!M->Web)return false;
 double RX=0,RZ=0;
 if(!S47OpenLand::RoadOf(M->Web->Area(),M->Player->Pos.X,M->Player->Pos.Z,RX,RZ))return false;
 Out=FVector2D(RX,RZ);return true;
}

double FS47EventPlaces::DistanceFromPlayer(const FS47EventPlace& P) const
{
 FVector2D At;if(!PlayerRoad(At))return -1;
 return FVector2D::Distance(At,FVector2D(P.X,P.Z));
}

bool FS47EventPlaces::AtNow(const FString& Id,FVector& OutAt,FString* OutArea) const
{
 const FS47EventPlace* P=Find(Id);if(!P)return false;
 const FString Area=AreaOf(*P);
 FVector W=S47OpenLand::FromRoad(Area,P->X,P->Z);
 double Y=S47FarmWorld::FloorRoad(P->X,P->Z);
 if(Area==TEXT("station01"))Y+=S47Geo::STATION_DY();
 const bool bHere=M&&M->Web&&S47EventPlacesLocal::SameLand(M->Web->Area(),Area);
 // Where the player is, the area's own floor is the truth: the land's sum is only near it.
 if(bHere&&M->Player&&M->Player->Floor){const double F=M->Player->Floor(W.X,W.Z);if(FMath::IsFinite(F))Y=F;}
 W.Y=Y;OutAt=W;if(OutArea)*OutArea=Area;
 return bHere;
}

bool FS47EventPlaces::EntryNow(const FString& Id,FVector& OutAt) const
{
 const FS47EventPlace* P=Find(Id);if(!P)return false;
 const FString Area=AreaOf(*P);
 FVector W=S47OpenLand::FromRoad(Area,P->EntryX,P->EntryZ);W.Y=S47FarmWorld::FloorRoad(P->EntryX,P->EntryZ);
 if(Area==TEXT("station01"))W.Y+=S47Geo::STATION_DY();
 OutAt=W;
 return M&&M->Web&&S47EventPlacesLocal::SameLand(M->Web->Area(),Area);
}

const FS47EventPlace* FS47EventPlaces::NearestTo(const FVector2D& Road,double WithinM,ES47PlaceTrait Need) const
{
 const FS47EventPlace* Best=nullptr;double BestD=WithinM;
 for(const FS47EventPlace& P:All())
 {
  if(Need!=ES47PlaceTrait::None&&!P.Has(Need))continue;
  // Measured to its door as well as its front: inside a room in town is nearer the door than the street.
  const double D=FMath::Min(FVector2D::Distance(Road,FVector2D(P.X,P.Z)),FVector2D::Distance(Road,FVector2D(P.EntryX,P.EntryZ)))-P.Radius*.25;
  if(D<=BestD){BestD=D;Best=&P;}
 }
 return Best;
}

const FS47EventPlace* FS47EventPlaces::Nearest(double WithinM,ES47PlaceTrait Need) const
{
 FVector2D At;if(!PlayerRoad(At))return nullptr;
 return NearestTo(At,WithinM,Need);
}

bool FS47EventPlaces::Fits(const FS47EventPlace& P,const FS47PlaceNeed& Need) const
{
 if(Need.All!=ES47PlaceTrait::None&&!P.Has(Need.All))return false;
 if(Need.Any!=ES47PlaceTrait::None&&!P.HasAny(Need.Any))return false;
 if(Need.Not!=ES47PlaceTrait::None&&P.HasAny(Need.Not))return false;
 if(!Need.Only.IsEmpty()&&!Need.Only.Contains(P.Id))return false;
 if(Need.Skip.Contains(P.Id))return false;
 return true;
}

TArray<const FS47EventPlace*> FS47EventPlaces::Matching(const FS47PlaceNeed& Need) const
{
 TArray<const FS47EventPlace*> Out;
 FVector2D At;const bool bPlayer=PlayerRoad(At);
 const FString Here=M&&M->Web?M->Web->Area():FString();
 for(const FS47EventPlace& P:All())
 {
  if(!Fits(P,Need))continue;
  if(Need.bSameArea&&!S47EventPlacesLocal::SameLand(Here,AreaOf(P)))continue;
  if(Need.bOtherArea&&!Here.IsEmpty()&&S47EventPlacesLocal::SameLand(Here,AreaOf(P)))continue;
  if(bPlayer)
  {
   const double D=FVector2D::Distance(At,FVector2D(P.X,P.Z));
   if((Need.MinM>0&&D<Need.MinM)||(Need.MaxM>0&&D>Need.MaxM))continue;
  }
  Out.Add(&P);
 }
 return Out;
}

float FS47EventPlaces::Weight(const FS47EventPlace& P,const FS47PlaceNeed& Need,int32 Night,const FVector2D& Player,const FVector2D& Heading,bool bFar) const
{
 float W=1.f;
 // Lately: hardly the same night, less the next nights, then as any other. Often used is a little less likely for good.
 if(const FUse* U=Uses.Find(P.Id))
 {
  const int32 Ago=Night-U->Night;
  W*=Ago<=0?.05f:Ago==1?.25f:Ago==2?.5f:Ago==3?.75f:1.f;
  W/=1.f+.1f*float(FMath::Min(U->Count,10));
 }
 if(Player.ContainsNaN())return W;
 const FVector2D To=FVector2D(P.X,P.Z)-Player;const double D=To.Size();
 if(bFar)return W*(D>1500?1.3f:1.f);
 // Near the player, more so. Ahead of a driver more again: where they are going.
 W*=1.f+2.5f*float(FMath::Exp(-D/450.));
 if(!Heading.IsNearlyZero()&&D>30&&D<3000)
 {
  const double Dot=FVector2D::DotProduct(Heading.GetSafeNormal(),To/D);
  if(Dot>0)W*=1.f+1.5f*float(Dot*Dot);
 }
 return W;
}

FS47PlacePick FS47EventPlaces::PickFor(const FS47PlaceNeed& Need,int32 Seed,int32 Night,const FVector2D& Player,const FVector2D& Heading,uint32 Salt) const
{
 using namespace S47EventPlacesLocal;
 FS47PlacePick Out;
 const FString Here=M&&M->Web?M->Web->Area():FString();
 const bool bPlayer=!Player.ContainsNaN();
 TArray<const FS47EventPlace*> Near,Far;
 for(const FS47EventPlace& P:All())
 {
  if(!Fits(P,Need))continue;
  if(Need.bSameArea&&!SameLand(Here,AreaOf(P)))continue;
  if(Need.bOtherArea&&!Here.IsEmpty()&&SameLand(Here,AreaOf(P)))continue;
  const double D=bPlayer?FVector2D::Distance(Player,FVector2D(P.X,P.Z)):0;
  if(bPlayer&&((Need.MinM>0&&D<Need.MinM)||(Need.MaxM>0&&D>Need.MaxM)))continue;
  (bPlayer&&D>=FarFrom?Far:Near).Add(&P);
 }
 if(Near.IsEmpty()&&Far.IsEmpty())return Out;
 const uint32 Key=Salted(Need.Kind,Salt);
 const float FarRoll=FS47NightRhythm::Roll(Seed,Night,Key^0x46415252u);
 const bool bFar=!Far.IsEmpty()&&(Near.IsEmpty()||FarRoll<Need.FarShare);
 const TArray<const FS47EventPlace*>& Pool=bFar?Far:Near;
 TArray<float> Weights;float Sum=0;
 for(const FS47EventPlace* P:Pool){const float W=FMath::Max(1e-4f,Weight(*P,Need,Night,bPlayer?Player:FVector2D(S47EventPlacesLocal::Nan,S47EventPlacesLocal::Nan),Heading,bFar));Weights.Add(W);Sum+=W;}
 const float Roll=FS47NightRhythm::Roll(Seed,Night,Key);
 float Acc=0;int32 Index=Pool.Num()-1;
 for(int32 I=0;I<Pool.Num();++I){Acc+=Weights[I]/Sum;if(Roll<Acc){Index=I;break;}}
 Out.Id=Pool[Index]->Id;Out.bFar=bFar;Out.Roll=Roll;Out.DistanceM=bPlayer?FVector2D::Distance(Player,FVector2D(Pool[Index]->X,Pool[Index]->Z)):0;
 return Out;
}

FS47PlacePick FS47EventPlaces::Pick(const FS47PlaceNeed& Need,uint32 Salt) const
{
 const int32 Seed=M&&M->Freeform?M->Freeform->Seed():47;
 const int32 Night=M&&M->Freeform?FS47FreeformSession::NightOf(M->Freeform->Clock()):1;
 FVector2D Player(NAN,NAN),Heading=FVector2D::ZeroVector;
 if(PlayerRoad(Player)&&M->World&&M->World->bDriving&&M->Player&&M->Web)
 {
  // The way the cab is pointing, in the land's terms (the station's area is turned a quarter round).
  const FVector Ahead=M->Player->Pos+FVector(-FMath::Sin(M->Player->Yaw),0,-FMath::Cos(M->Player->Yaw))*10;
  double RX=0,RZ=0;if(S47OpenLand::RoadOf(M->Web->Area(),Ahead.X,Ahead.Z,RX,RZ))Heading=FVector2D(RX,RZ)-Player;
 }
 return PickFor(Need,Seed,Night,Player,Heading,Salt);
}

void FS47EventPlaces::Used(const FString& Id,const FString& Kind)
{
 if(Id.IsEmpty()||!Find(Id))return;
 const int32 Night=M&&M->Freeform?FS47FreeformSession::NightOf(M->Freeform->Clock()):1;
 FUse& U=Uses.FindOrAdd(Id);U.Night=Night;U.Count=FMath::Min(U.Count+1,100000);U.Kind=Kind.Left(64);
 if(M)M->RequestAutosave();
}

int32 FS47EventPlaces::LastNight(const FString& Id) const{const FUse* U=Uses.Find(Id);return U?U->Night:0;}
int32 FS47EventPlaces::TimesUsed(const FString& Id) const{const FUse* U=Uses.Find(Id);return U?U->Count:0;}

void FS47EventPlaces::LogPick(const FString& Kind,const FS47EventPlace* P,const FString& How)
{
 if(P){UE_LOG(LogS47EventPlaces,Display,TEXT("S47: event place %s -> %s (%s, %s), %s"),*Kind,*P->Id,*P->Title,*AreaOf(*P),*How);}
 else{UE_LOG(LogS47EventPlaces,Display,TEXT("S47: event place %s -> none, %s"),*Kind,*How);}
}

TArray<FString> FS47EventPlaces::Ids()
{
 TArray<FString> Out;for(const auto& S:S47EventPlacesLocal::Seeds())Out.Add(S.Id);
 return Out;
}

bool FS47EventPlaces::SelfTest(TArray<FString>& Lines)
{
 bool bAll=true;
 const auto Check=[&](bool bOk,const FString& What){bAll&=bOk;Lines.Add(FString(bOk?TEXT("PASS "):TEXT("FAIL "))+What);};
 FS47EventPlaces C(nullptr);
 const auto& L=C.All();
 TSet<FString> Seen;bool bUnique=true,bInRim=true,bTitled=true;
 for(const FS47EventPlace& P:L){bUnique&=!Seen.Contains(P.Id);Seen.Add(P.Id);bInRim&=S47OpenLand::InRim(P.X,P.Z);bTitled&=!P.Title.IsEmpty()&&FChar::IsLower(P.Title[0])==(P.Id!=TEXT("station01"));}
 Check(L.Num()>=30&&bUnique,FString::Printf(TEXT("event places: %d, each id once"),L.Num()));
 Check(bInRim,TEXT("event places: every one stands on the land, inside the rim"));
 Check(bTitled,TEXT("event places: every title reads in a radio line (lower case, with its article)"));
 Check(C.Find(TEXT("farm"))&&C.Find(TEXT("church"))&&C.Find(TEXT("motel"))&&C.Find(TEXT("array"))&&C.Find(TEXT("land:wash_west")),TEXT("event places: the farm, Caliche, the motel, SARO and the open land are all on it"));
 Check(AreaOf(*C.Find(TEXT("array")))==TEXT("saro")&&AreaOf(*C.Find(TEXT("farm")))==TEXT("saro")&&AreaOf(*C.Find(TEXT("church")))==TEXT("road"),TEXT("event places: the station's ground and the farm are SARO's area, town is the road's"));
 // The draw: the same night and seed give the same place; another night, or another salt, may not; nothing ever picks outside the need.
 FS47PlaceNeed N;N.Kind=TEXT("selftest");N.Any=ES47PlaceTrait::Open|ES47PlaceTrait::Town;
 const FVector2D At(60,240);
 const FS47PlacePick A=C.PickFor(N,47,3,At,FVector2D::ZeroVector),B=C.PickFor(N,47,3,At,FVector2D::ZeroVector);
 Check(A.IsSet()&&A.Id==B.Id,TEXT("event places: the same seed and night draw the same place"));
 TSet<FString> Spread;int32 FarCount=0;bool bFits=true;
 for(int32 Night=1;Night<=60;++Night){const FS47PlacePick P=C.PickFor(N,47,Night,At,FVector2D::ZeroVector);Spread.Add(P.Id);FarCount+=P.bFar?1:0;const auto* Q=C.Find(P.Id);bFits&=Q&&Q->HasAny(N.Any);}
 Check(Spread.Num()>=8&&bFits,FString::Printf(TEXT("event places: sixty nights from the farm draw %d different places, every one fitting"),Spread.Num()));
 Check(FarCount>=4&&FarCount<=30,FString::Printf(TEXT("event places: some of them far off (%d of 60)"),FarCount));
 // Lately weighs it down: drawn for the same need, the place used tonight seldom comes again tonight.
 int32 Again=0;
 for(int32 Night=1;Night<=40;++Night)
 {
  FS47EventPlaces D(nullptr);const FS47PlacePick First=D.PickFor(N,11,Night,At,FVector2D::ZeroVector,1);
  FS47EventPlaces::FUse U;U.Night=Night;U.Count=1;D.Uses.Add(First.Id,U);
  Again+=D.PickFor(N,11,Night,At,FVector2D::ZeroVector,2).Id==First.Id?1:0;
 }
 Check(Again<=6,FString::Printf(TEXT("event places: a place used tonight comes again tonight %d times in 40"),Again));
 // Saved and read back: the same history, so a Continue draws the same.
 FS47EventPlaces S(nullptr);FS47EventPlaces::FUse U;U.Night=4;U.Count=2;U.Kind=TEXT("town_lights");S.Uses.Add(TEXT("church"),U);
 FS47EventPlaces R(nullptr);R.Begin(S47J::Read(S47J::Write(S.Save())));
 Check(R.LastNight(TEXT("church"))==4&&R.TimesUsed(TEXT("church"))==2&&R.PickFor(N,5,4,At,FVector2D::ZeroVector).Id==S.PickFor(N,5,4,At,FVector2D::ZeroVector).Id,TEXT("event places: what was used survives Continue, and the draw after it is the same"));
 // A need nothing fits gives nothing, not the farm.
 FS47PlaceNeed None;None.Kind=TEXT("selftest");None.All=ES47PlaceTrait::Door|ES47PlaceTrait::Array;
 Check(!C.PickFor(None,47,3,At,FVector2D::ZeroVector).IsSet(),TEXT("event places: a need nothing fits draws nothing"));
 FS47PlaceNeed Doors;Doors.Kind=TEXT("selftest");Doors.All=ES47PlaceTrait::Door;
 int32 DoorPlaces=0;for(const auto& P:L)DoorPlaces+=P.Has(ES47PlaceTrait::Door)&&!P.DoorKey.IsEmpty()?1:0;
 Check(DoorPlaces>=10&&C.WithDoor(TEXT("house"))&&C.WithDoor(TEXT("house"))->Id==TEXT("oldhouse"),FString::Printf(TEXT("event places: %d places with a door something can try, the old house among them"),DoorPlaces));
 return bAll;
}
