#include "S47LocationCards.h"
#include "S47EventPlaces.h"
#include "S47FarmWorld.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Places.h"
#include "S47TownSites.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47World.h"
#include "SS47FieldMap.h"

// UI-20261112 (Tom)
namespace
{
 // The place catalogue with no game behind it: the table's own numbers, for the self-test and before a session has its own.
 const FS47EventPlaces& PureCatalogue(){static const FS47EventPlaces C(nullptr);return C;}
 // Caliche as one comes into it by the road: the street from the crossing to the south end, the walks and the fronts each side.
 bool InCaliche(double X,double Z){return FMath::Abs(X)<=30&&Z>=2016&&Z<=2262;}
 // What is remembered while a session runs: the day each card was last shown, real seconds since the last card, the next look.
 struct FState{const void* Session=nullptr;TMap<FString,int32> Shown;double Since=1e9,LookIn=0;};
 FState State;
}

namespace S47LocationCards
{
 const TArray<FCard>& Cards()
 {
  // A place of its own before an area. The names are the game's own: the signs, the survey sheet, the place names.
  static const TArray<FCard> Table={
   // the station and what stands by it
   {TEXT("saro"),TEXT("saro"),TEXT("SARO RADIO OBSERVATORY"),County,95,true,false},
   {TEXT("motel"),TEXT("motel"),TEXT("SIERRA MOTOR COURT"),County,22,false,false},
   {TEXT("farm"),TEXT(""),TEXT("ALDERETE FARM"),County,0,true,false},
   {TEXT("substation"),TEXT(""),TEXT("CALICHE JUNCTION SUBSTATION"),County,0,false,false},
   {TEXT("station01"),TEXT(""),TEXT("STATION 01"),County,0,true,false},
   // Caliche: the town as one drives in, then each front as one comes to its door
   {TEXT("caliche"),TEXT(""),TEXT("CALICHE"),County,0,true,false},
   {TEXT("diner"),TEXT("diner"),TEXT("MESA DINER"),Town,20,false,false},
   {TEXT("gas"),TEXT("gas"),TEXT("CALICHE GAS STATION"),Town,12,false,true},
   {TEXT("sheriff"),TEXT("sheriff"),TEXT("SHERIFF'S SUBSTATION"),Town,7,false,true},
   {TEXT("laundry"),TEXT("laundry"),TEXT("CALICHE COIN LAUNDRY"),Town,7,false,true},
   {TEXT("cinema"),TEXT("cinema"),TEXT("PLAZA CINEMA"),Town,7,false,true},
   {TEXT("feed"),TEXT("feed"),TEXT("CALICHE FEED & SEED"),Town,8,false,true},
   {TEXT("church"),TEXT("church"),TEXT("THE CHURCH"),Town,8,false,true},
   {TEXT("souvenir"),TEXT("souvenir"),TEXT("SOUVENIR SHOP"),Town,7,false,true},
   {TEXT("radio"),TEXT("radio"),TEXT("1420 AM STUDIO"),Town,7,false,true},
   {TEXT("school"),TEXT("school"),TEXT("CALICHE SCHOOL"),Town,30,false,false},
   {TEXT("grange"),TEXT("grange"),TEXT("GRANGE HALL"),Town,22,false,false},
   {TEXT("trailers"),TEXT("trailers"),TEXT("TRAILER PARK"),Town,36,false,false},
   {TEXT("breather"),TEXT("breather"),TEXT("THE BREATHER"),Town,16,false,false},
   // out on the land
   {TEXT("baca"),TEXT("baca"),TEXT("THE BACA PLACE"),County,36,false,false},
   {TEXT("windmill"),TEXT("windmill"),TEXT("WINDMILL, BACA LEASE"),County,28,false,false},
   {TEXT("tank"),TEXT("tank"),TEXT("STOCK TANK, THE OLD GRADE"),County,28,false,false},
   {TEXT("mine"),TEXT("mine"),TEXT("ESPERANZA MINE"),County,32,false,false},
   {TEXT("shack"),TEXT("shack"),TEXT("LINE SHACK"),County,24,false,false},
   {TEXT("cairn"),TEXT("cairn"),TEXT("SURVEY CAIRN"),County,20,false,false},
   {TEXT("trough"),TEXT("trough"),TEXT("SALT LICK AND TROUGH"),County,22,false,false},
   {TEXT("saucer_rock"),TEXT("saucer_rock"),TEXT("SAUCER ROCK"),County,30,false,false},
  };
  return Table;
 }
 const FCard* Find(const FString& Id){return Cards().FindByPredicate([&](const FCard& C){return Id==C.Id;});}
 bool Spot(const FCard& C,const FS47EventPlaces& Catalogue,FVector2D& Out)
 {
  if(!C.Place||!*C.Place)return false;
  if(const FS47EventPlace* P=Catalogue.Find(C.Place)){Out=C.bDoor?FVector2D(P->EntryX,P->EntryZ):FVector2D(P->X,P->Z);return true;}
  // a landmark is not on the catalogue (the Breather): where its site stands
  if(const S47TownSites::FSite* S=S47TownSites::Find(C.Place)){Out=FVector2D(S->X,S->Z);return true;}
  return false;
 }
 FString At(const FS47EventPlaces& Catalogue,const FString& Area,const FString& PlaceName,bool bRoad,double X,double Z)
 {
  if(Area==TEXT("station01"))return TEXT("station01");
  if(Area==TEXT("room6"))return TEXT("motel");
  if(PlaceName==SubstationName)return TEXT("substation");
  if(!bRoad||!FMath::IsFinite(X)||!FMath::IsFinite(Z))return FString();
  const FVector2D Me(X,Z);
  const FCard* Best=nullptr;double BestM=0;
  for(const FCard& C:Cards())
  {
   FVector2D S;
   if(C.bArea||!Spot(C,Catalogue,S))continue;
   const double Away=FVector2D::Distance(Me,S);
   if(Away<=C.Radius&&(!Best||Away<BestM)){Best=&C;BestM=Away;}
  }
  if(Best)return Best->Id;
  if(S47FarmWorld::InRoadAccess(X,Z))return TEXT("farm");
  if(InCaliche(X,Z))return TEXT("caliche");
  for(const FCard& C:Cards())
  {
   FVector2D S;
   if(C.bArea&&Spot(C,Catalogue,S)&&FVector2D::Distance(Me,S)<=C.Radius)return C.Id;
  }
  return FString();
 }
 bool May(bool bShownToday,bool bArea,bool bDriving,double Speed,double SinceLast)
 {
  if(bShownToday||SinceLast<MinGap)return false;
  return !bDriving||bArea||FMath::Abs(Speed)<=FastMps;
 }
 FString Clock(double GameClock){return FS47Places::SlipClock(GameClock);}
 TArray<FString> Lines(const FCard& C,double GameClock){return {FString(C.Line1),FString(C.Line2),Clock(GameClock)};}
 double LineStart(const TArray<FString>& L,int32 Line)
 {
  double T=0;
  for(int32 I=0;I<Line&&I<L.Num();++I)T+=L[I].Len()/CharsPerSec+LinePause;
  return T;
 }
 int32 Typed(const TArray<FString>& L,int32 Line,double T)
 {
  if(!L.IsValidIndex(Line)||!FMath::IsFinite(T))return 0;
  return FMath::Clamp(int32(FMath::FloorToDouble((T-LineStart(L,Line))*CharsPerSec+1e-6)),0,L[Line].Len());
 }
 double TypeSecs(const TArray<FString>& L){return L.Num()?LineStart(L,L.Num()-1)+L.Last().Len()/CharsPerSec:0.;}
 double Life(const TArray<FString>& L){return TypeSecs(L)+Hold+FadeSecs;}
 double Alpha(const TArray<FString>& L,double T)
 {
  if(!FMath::IsFinite(T)||T<0)return 0;
  const double Out=FMath::Clamp((T-TypeSecs(L)-Hold)/FadeSecs,0.,1.);
  return 1.-Out*Out*(3.-2.*Out);
 }
 void Step(US47Main* M,double Dt)
 {
  if(!IsValid(M)||!FMath::IsFinite(Dt)||Dt<=0)return;
  if(!M->IsFreeform()||!M->Freeform||!M->UI||!M->Flow){if(State.Session)State=FState();return;}
  if(State.Session!=M->Freeform.Get()){State=FState();State.Session=M->Freeform.Get();}
  State.Since+=Dt;
  if((State.LookIn-=Dt)>0)return;
  State.LookIn=Look;
  if(!M->Flow->bLocationCards||M->Mode!=US47Main::EMode::Play||M->ModalOpen()||M->UI->Faded()||M->Freeform->Held())return;
  double X=0,Z=0,Facing=0;FString Area;bool bWheel=false;
  const bool bRoad=S47FieldMap::Me(M,X,Z,Facing,Area,bWheel);
  if(Area.IsEmpty()&&M->Web)Area=M->Web->Area();
  const FS47EventPlaces& Catalogue=M->Freeform->EventPlaces?*M->Freeform->EventPlaces:PureCatalogue();
  const FString Id=At(Catalogue,Area,M->Flow->PlaceName(),bRoad,X,Z);
  const FCard* C=Id.IsEmpty()?nullptr:Find(Id);
  if(!C)return;
  const int32 Day=M->Freeform->Day();
  const int32* Was=State.Shown.Find(Id);
  const double Speed=bWheel&&M->World?M->World->DrivingSpeed():0.;
  if(!May(Was&&*Was==Day,C->bArea,bWheel,Speed,State.Since))return;
  const TArray<FString> L=Lines(*C,M->Freeform->Clock());
  if(!M->UI->LocationCard(L[0],L[1],L[2]))return;
  State.Shown.Add(Id,Day);State.Since=0;
  UE_LOG(LogTemp,Display,TEXT("S47: location card (%s): %s / %s / %s"),*Id,*L[0],*L[1],*L[2]);
 }
 bool SelfTest(TArray<FString>& Out)
 {
  bool bOk=true;
  auto Check=[&](bool Cond,const FString& What){Out.Add(FString(Cond?TEXT("PASS "):TEXT("FAIL "))+What);bOk&=Cond;};
  const FS47EventPlaces& Cat=PureCatalogue();
  // the words: capitals, short, the game's own county and town, no spoiler word
  static const TCHAR* const Banned[]={TEXT("47"),TEXT("1947"),TEXT("cable"),TEXT("tomas"),TEXT("taken"),TEXT("silhouette"),TEXT("8.15"),TEXT("alien"),TEXT("ufo"),TEXT("martian"),TEXT("third")};
  FString Bad;TSet<FString> Ids,Firsts;int32 Placed=0;bool bStand=true;
  for(const FCard& C:Cards())
  {
   const FString A=C.Line1,B=C.Line2;
   if(Ids.Contains(C.Id)||Firsts.Contains(A)||A.Len()<4||A.Len()>30||A!=A.ToUpper()||A.TrimStartAndEnd()!=A||(B!=County&&B!=Town))Bad=C.Id;
   Ids.Add(C.Id);Firsts.Add(A);
   for(const FString& W:{A,B})
   {
    const FString Lo=W.ToLower();
    for(const TCHAR* X:Banned)if(Lo.Contains(X))Bad=C.Id;
    for(const TCHAR Ch:W)if(Ch<32||Ch>126)Bad=C.Id;
    for(int32 I=0;I<W.Len();++I)if(W[I]=='C'&&(I==0||!FChar::IsAlpha(W[I-1]))&&(I+1>=W.Len()||!FChar::IsAlpha(W[I+1])))Bad=C.Id;
   }
   // a card with a place stands where the catalogue (or the site table) puts it, and standing there gives that card
   FVector2D S;
   if(!C.Place||!*C.Place)continue;
   ++Placed;
   if(!Spot(C,Cat,S)||C.Radius<=0||At(Cat,TEXT("road"),FString(),true,S.X,S.Y)!=C.Id){bStand=false;Out.Add(FString(TEXT("  (no ground for the card of "))+C.Id+TEXT(")"));}
  }
  Check(Bad.IsEmpty()&&Cards().Num()>=24,FString::Printf(TEXT("UI-1112: %d location cards, each a place and a town or county line in capitals, no spoiler word: %s"),Cards().Num(),*Bad));
  Check(bStand&&Placed>=20,FString::Printf(TEXT("UI-1112: %d cards stand on the place catalogue's own ground, and standing there gives that card"),Placed));
  // each known area has its card
  Check(At(Cat,TEXT("station01"),TEXT("STATION 01"),false,0,0)==TEXT("station01")&&At(Cat,TEXT("room6"),TEXT("Sierra Motor Court"),false,0,0)==TEXT("motel")
   &&At(Cat,TEXT("road"),SubstationName,true,-110,532)==TEXT("substation")&&At(Cat,TEXT("saro"),FString(),true,100,200)==TEXT("farm")
   &&At(Cat,TEXT("diner"),FString(),true,0,2140)==TEXT("caliche")&&At(Cat,TEXT("saro"),FString(),true,40,-270)==TEXT("saro"),
   TEXT("UI-1112: STATION 01, the motel room, the substation, the farm, Caliche's street and SARO's ground each give their card"));
  Check(At(Cat,TEXT("road"),FString(),true,0,1000).IsEmpty()&&At(Cat,TEXT("road"),FString(),true,600,600).IsEmpty()&&At(Cat,TEXT("roswell"),FString(),false,0,0).IsEmpty()&&At(Cat,TEXT("road"),FString(),false,0,2140).IsEmpty(),
   TEXT("UI-1112: the open highway, the open land and ground off the land's frame give no card"));
  {
   // a front on the street is its door: on the walk at the door yes, in the roadway driving past no
   FVector2D Door;const FCard* Feed=Find(TEXT("feed"));
   const bool bDoor=Feed&&Spot(*Feed,Cat,Door);
   Check(bDoor&&At(Cat,TEXT("diner"),FString(),true,Door.X+2,Door.Y+1)==TEXT("feed")&&At(Cat,TEXT("diner"),FString(),true,0,Door.Y)==TEXT("caliche"),
    TEXT("UI-1112: a front on the street gives its card at its door, the roadway past it gives the town's"));
  }
  // when: once a day, a gap between two, and from a truck at speed only an area's
  Check(May(false,false,false,0,1e9)&&!May(true,false,false,0,1e9)&&!May(false,false,false,0,MinGap-1)&&May(false,false,false,0,MinGap),
   TEXT("UI-1112: a card comes once a day for its place and never within 20 seconds of another"));
  Check(!May(false,false,true,FastMps+1,1e9)&&!May(false,false,true,-FastMps-1,1e9)&&May(false,false,true,FastMps-1,1e9)&&May(false,true,true,30,1e9)&&May(false,false,false,30,1e9),
   TEXT("UI-1112: from a truck at speed only an area's card; a place's own waits until it slows"));
  // the clock: 24 hours, never Story's numbers
  Check(Clock(23*3600.+42*60.)==TEXT("23:42")&&Clock(9*3600.+5*60.+86400.*3)==TEXT("09:05")&&!Clock(47*60.).Contains(TEXT("47"))&&Clock(8*3600.+15*60.)!=TEXT("08:15"),
   TEXT("UI-1112: the time is on the 24-hour clock and never one of Story's numbers"));
  // the typing: a letter at a time at 25 to 35 a second, line after line, then three seconds and gone
  bool bType=true;double Longest=0;
  for(const FCard& C:Cards())
  {
   const TArray<FString> L=Lines(C,23*3600.+42*60.);
   bType&=L.Num()==3&&L[2].Len()==5&&Typed(L,0,0)==0&&Typed(L,0,1./CharsPerSec)==1&&Typed(L,1,LineStart(L,1))==0&&Typed(L,2,TypeSecs(L))==L[2].Len();
   int32 Before=0;
   for(double T=0;T<=Life(L);T+=.01)
   {
    const int32 Now=Typed(L,0,T)+Typed(L,1,T)+Typed(L,2,T);
    // never two letters in a hundredth, never a letter of a line before the line above is whole
    bType&=Now>=Before&&Now-Before<=1&&(Typed(L,1,T)==0||Typed(L,0,T)==L[0].Len())&&(Typed(L,2,T)==0||Typed(L,1,T)==L[1].Len());
    Before=Now;
   }
   bType&=Before==L[0].Len()+L[1].Len()+L[2].Len();
   bType&=Alpha(L,0)==1&&Alpha(L,TypeSecs(L)+Hold-.01)==1&&Alpha(L,TypeSecs(L)+Hold+FadeSecs*.5)<1&&Alpha(L,Life(L)+.01)==0&&Alpha(L,-1)==0;
   Longest=FMath::Max(Longest,TypeSecs(L));
  }
  Check(bType&&CharsPerSec>=25&&CharsPerSec<=35&&Hold>=2.5&&Hold<=3.5&&Longest<3.,FString::Printf(TEXT("UI-1112: a card is typed a letter at a time at %.0f a second, line after line (the longest in %.1f s), stands %.0f s and fades"),CharsPerSec,Longest,Hold));
  Check(TickGain>0&&TickGain<=.25f&&ReturnGain<TickGain&&Find(TEXT("no_such"))==nullptr,TEXT("UI-1112: the tick is the teleprinter's type bar at a low level; an unknown card is none"));
  Step(nullptr,.1);
  return bOk;
 }
}
