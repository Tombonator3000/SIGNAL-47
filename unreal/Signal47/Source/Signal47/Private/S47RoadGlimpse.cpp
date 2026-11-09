#include "S47RoadGlimpse.h"
#include "S47NightRhythm.h"
#include "S47TripoNew.h"

namespace S47RoadGlimpseLocal
{
 // The stand-ins (Tom's TripoNPC figures, already in the game): the tall one in his Nordic's and Cosmic's place, the shade the
 // horizon's own, and the crawler on four.
 const TCHAR* const Tall=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_tall_one.SM_Tripo_tall_one");
 const TCHAR* const Shadow=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_shadow_man.SM_Tripo_shadow_man");
 const TCHAR* const Crawler=TEXT("/Game/Licensed/TripoNPC/SM_Tripo_crawler.SM_Tripo_crawler");
 // Seconds across: the tall one in long strides, the low one at a run; the tall one's stop on the crown.
 constexpr double TallSeconds=3.,LowSeconds=1.8,LookSeconds=.8;
 // A margin past the closest it may come, so that it is off the road with room to spare.
 constexpr double Spare=5;
 double Seconds(const S47RoadGlimpse::FPlan& P){return P.Body==S47RoadGlimpse::EBody::Low?LowSeconds:TallSeconds;}
}

const TCHAR* S47RoadGlimpse::PathOf(const FString& Id)
{
 using namespace S47RoadGlimpseLocal;
 if(Id==TEXT("cosmic")||Id==TEXT("nordic")){const S47TripoNew::FModel* M=S47TripoNew::Find(Id);return M?M->Path:nullptr;}
 if(Id==TEXT("tall"))return Tall;
 if(Id==TEXT("shadow"))return Shadow;
 if(Id==TEXT("crawler"))return Crawler;
 return nullptr;
}

S47RoadGlimpse::FPlan S47RoadGlimpse::PlanFor(int32 Seed,int32 Night,double TruckSpeed,TFunctionRef<bool(const TCHAR*)> Have,const FString& Want)
{
 using namespace S47RoadGlimpseLocal;
 const auto R=[&](uint32 Salt){return double(FS47NightRhythm::Roll(Seed,Night,Salt));};
 FPlan P;
 // Which: something low a little less often than the tall one; Tom's two in an order of the seed's, then what stands in for them.
 const bool bCosmicFirst=R(0x52474232u)<.5;
 const TArray<FString> TallOrder=bCosmicFirst?TArray<FString>{TEXT("cosmic"),TEXT("nordic"),TEXT("tall"),TEXT("shadow")}:TArray<FString>{TEXT("nordic"),TEXT("cosmic"),TEXT("tall"),TEXT("shadow")};
 bool bLow=R(0x52474231u)<.42;
 TArray<FString> Order;
 if(Want==TEXT("tall"))bLow=false;
 else if(Want==TEXT("low"))bLow=true;
 else if(PathOf(Want))Order.Add(Want);
 if(Want==TEXT("crawler"))bLow=true;else if(Order.Num())bLow=false;
 if(bLow)Order.Add(TEXT("crawler"));
 Order.Append(TallOrder);
 for(const FString& Id:Order)if(const TCHAR* Path=PathOf(Id);Path&&Have(Path)){P.Id=Id;break;}
 P.Body=P.Id==TEXT("crawler")?EBody::Low:EBody::Tall;
 P.Side=R(0x52474233u)<.5?1.:-1.;
 P.bTracks=R(0x52474234u)<.55;
 // How far ahead: the seed's, but never so near that the truck could come up to it before it is across.
 const double V=FMath::IsFinite(TruckSpeed)?FMath::Abs(TruckSpeed):0.;
 const double Seeded=FromM+(ToM-FromM)*R(0x52474235u);
 P.AheadM=FMath::Clamp(FMath::Max(Seeded,GoneWithinM+V*Seconds(P)+Spare),FromM,ToM);
 // The stop on the crown only when there is time for it.
 P.bLooks=P.Body==EBody::Tall&&R(0x52474236u)<.4&&GoneWithinM+V*(Seconds(P)+LookSeconds)+Spare<=P.AheadM;
 return P;
}

double S47RoadGlimpse::CrossSeconds(const FPlan& P){return S47RoadGlimpseLocal::Seconds(P)+(P.bLooks?S47RoadGlimpseLocal::LookSeconds:0.);}

double S47RoadGlimpse::CrossX(const FPlan& P,double Age,bool* bOn,bool* bStanding)
{
 using namespace S47RoadGlimpseLocal;
 const double Cross=Seconds(P),Half=Cross/2;
 double T=Age/Cross;bool bStand=false;
 if(P.bLooks&&Age>=Half){if(Age<Half+LookSeconds){T=.5;bStand=true;}else T=(Age-LookSeconds)/Cross;}
 if(bOn)*bOn=Age>=0&&T<1.;
 if(bStanding)*bStanding=bStand;
 return FMath::Lerp(P.Side*EdgeM,-P.Side*EdgeM,FMath::Clamp(T,0.,1.));
}

double S47RoadGlimpse::Bob(const FPlan& P,double Age)
{
 bool bStand=false;CrossX(P,Age,nullptr,&bStand);
 if(bStand)return 0.;
 return P.Body==EBody::Low?.07*FMath::Abs(FMath::Sin(Age*2*UE_DOUBLE_PI*2.6)):.035*FMath::Abs(FMath::Sin(Age*2*UE_DOUBLE_PI*1.4));
}

bool S47RoadGlimpse::InBeam(double Metres,double Degrees)
{
 return (Metres<=80&&Degrees<=24)||(Metres<=BeamFarM&&Degrees<=BeamFarDeg);
}

FString S47RoadGlimpse::FigureFor(int32 Seed,int32 Night,uint32 Salt,TFunctionRef<bool(const TCHAR*)> Have,const FString& Want)
{
 FString Pick;
 if(Want==TEXT("nordic")||Want==TEXT("cosmic"))Pick=Want;
 else if(Want.IsEmpty())
 {
  const float Roll=FS47NightRhythm::Roll(Seed,Night,Salt^0x46494755u);
  Pick=Roll<.25f?TEXT("nordic"):Roll<.5f?TEXT("cosmic"):TEXT("");
 }
 // Only Tom's own: without it the shade stands there as before.
 const TCHAR* Path=Pick.IsEmpty()?nullptr:PathOf(Pick);
 return Path&&Have(Path)?Pick:FString();
}

const TArray<S47RoadGlimpse::FDevBody>& S47RoadGlimpse::DevBodies()
{
 static const TArray<FDevBody> Lines={
  {TEXT("horizon_figure"),TEXT("nordic"),TEXT("Glimpse: the Nordic on the rise, far off"),TEXT("00:30, on foot near the windmill; Tom's Nordic a long way off, dead still; the lamp finds nobody"),.5},
  {TEXT("horizon_figure"),TEXT("cosmic"),TEXT("Glimpse: the Cosmic one on the rise, far off"),TEXT("00:30, on foot near the windmill; Tom's Cosmic one a long way off, shining of itself; the lamp finds nobody"),.5},
  {TEXT("figure_near"),TEXT("nordic"),TEXT("On foot: the Nordic comes nearer (danger)"),TEXT("02:30, on foot by the windmill, lamp on; the Nordic, gone when seen and nearer the next time, then a light comes down"),2.5},
  {TEXT("figure_near"),TEXT("cosmic"),TEXT("On foot: the Cosmic one comes nearer (danger)"),TEXT("02:30, on foot by the windmill, lamp on; the Cosmic one, gone when seen and nearer the next time, then a light comes down"),2.5},
  {TEXT("road_glimpse"),TEXT("nordic"),TEXT("Glimpse: the Nordic crosses far ahead"),TEXT("00:30, in the truck on the highway; the Nordic 60 to 120 m ahead in the lights, gone before you get there"),.5},
  {TEXT("road_glimpse"),TEXT("cosmic"),TEXT("Glimpse: the Cosmic one crosses far ahead"),TEXT("00:30, in the truck on the highway; the Cosmic one 60 to 120 m ahead, shining of itself, gone before you get there"),.5},
  {TEXT("close"),TEXT("nordic"),TEXT("Close: the Nordic, day"),TEXT("11:00 on the open land, 2.5 m from you, facing you: his face, his hair, his clothes by day"),11.},
  {TEXT("close"),TEXT("nordic"),TEXT("Close: the Nordic, night"),TEXT("23:00 on the open land, no flashlight, 2.5 m from you: what is left of him in the dark"),23.},
  {TEXT("close"),TEXT("cosmic"),TEXT("Close: the Cosmic one's glow, night"),TEXT("23:00 on the open land, no flashlight, 3 m from you: the body of light, its stars"),23.},
 };
 return Lines;
}

const TCHAR* S47RoadGlimpse::DevBodyName(const FString& Body)
{
 return Body==TEXT("nordic")?TEXT("the Nordic"):Body==TEXT("cosmic")?TEXT("the Cosmic one"):Body.IsEmpty()?TEXT("the shade"):TEXT("a stand-in");
}

FString S47RoadGlimpse::Record(const FPlan& P,const FString& Clock)
{
 const TCHAR* From=P.Side>0?TEXT("right"):TEXT("left");
 if(P.Body==EBody::Low)
  return FString::Printf(TEXT("At %s, driving, something low crossed the highway a long way ahead in the lights, from the %s, fast and on four legs, longer in the body than a dog and wrong in the way it moved. It was off the road before I came up to the place."),*Clock,From);
 return FString::Printf(TEXT("At %s, driving, something tall and thin crossed the highway a long way ahead in the lights, from the %s, in long strides%s. It was off the road before I came up to the place. Nothing on either shoulder when I went by."),
  *Clock,From,P.bLooks?TEXT(", and stood a moment on the crown of the road turned toward the truck"):TEXT(""));
}

FString S47RoadGlimpse::TracksRecord(bool bLow,const FString& Clock,bool bDay)
{
 const TCHAR* Light=bDay?TEXT(""):TEXT(" by the lamp");
 if(bLow)return FString::Printf(TEXT("At %s%s, on the shoulder of the highway where something crossed in the night: small prints in fours, close together, going off the edge into the brush at a run. None on the asphalt."),*Clock,Light);
 return FString::Printf(TEXT("At %s%s, on the shoulder of the highway where something crossed in the night: long narrow prints going off the edge into the brush, a long stride apart. None on the asphalt, and none on the other side."),*Clock,Light);
}

FString S47RoadGlimpse::Deed(const FPlan& P)
{
 return P.Body==EBody::Low?TEXT("You saw something low cross the road far ahead of the truck."):TEXT("You saw something tall cross the road far ahead of the truck.");
}

bool S47RoadGlimpse::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const FString& Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s road glimpse: %s"),B?TEXT("PASS"):TEXT("FAIL"),*Why));};
 const auto All=[](const TCHAR*){return true;};
 const auto StandIns=[](const TCHAR* P){return !FString(P).Contains(TEXT("/TripoNew/"));};
 const auto NoCrawler=[](const TCHAR* P){return !FString(P).Contains(TEXT("crawler"));};
 const auto None=[](const TCHAR*){return false;};
 // The seed's: the same night draws the same, every time.
 {
  const FPlan A=PlanFor(47,9,22,All),B=PlanFor(47,9,22,All);
  Check(A.Id==B.Id&&A.AheadM==B.AheadM&&A.Side==B.Side&&A.bLooks==B.bLooks&&A.bTracks==B.bTracks,TEXT("the same seed and night give the same crossing"));
 }
 // Far, and gone before the truck comes up: 60 to 120 m ahead, and across before the truck is within 45 m at any speed it may go
 // and still have it fit (at ordinary highway speeds); faster than that it is gone where it is, never nearer.
 {
  bool bRange=true,bAhead=true;int32 Tall=0,Low=0,Cosmic=0,Nordic=0,Looks=0,Tracks=0,Right=0,N=0;
  for(int32 Seed=1;Seed<=60;++Seed)for(int32 Night=1;Night<=30;++Night)for(const double V:{0.,12.,20.,26.,40.})
  {
   const FPlan P=PlanFor(Seed,Night,V,All);++N;
   bRange&=P.AheadM>=FromM&&P.AheadM<=ToM&&!P.Id.IsEmpty();
   if(GoneWithinM+V*CrossSeconds(P)+5<=ToM)bAhead&=P.AheadM-V*CrossSeconds(P)>=GoneWithinM;
   Tall+=P.Body==EBody::Tall;Low+=P.Body==EBody::Low;Cosmic+=P.Id==TEXT("cosmic");Nordic+=P.Id==TEXT("nordic");Looks+=P.bLooks;Tracks+=P.bTracks;Right+=P.Side>0;
  }
  Check(bRange,TEXT("it crosses 60 to 120 m ahead, always with a body when there is one"));
  Check(bAhead,TEXT("at highway speeds it is off the road before the truck is within 45 m"));
  Check(Tall>N/3&&Low>N/4&&Cosmic>N/8&&Nordic>N/8&&Looks>0&&Tracks>N/3&&Tracks<N*3/4&&Right>N/3&&Right<N*2/3,
   FString::Printf(TEXT("the seed varies it: tall %d, low %d, Cosmic %d, Nordic %d, stops to look %d, prints %d, from the right %d of %d"),Tall,Low,Cosmic,Nordic,Looks,Tracks,Right,N));
 }
 // Without Tom's two the tall stand-in takes their place; without the crawler the low one is a tall one; with nothing, nothing.
 {
  bool bStand=true,bLowTall=true,bNone=true;
  for(int32 Night=1;Night<=40;++Night)
  {
   const FPlan S=PlanFor(3,Night,20,StandIns);bStand&=S.Id==TEXT("tall")||S.Id==TEXT("crawler");
   const FPlan L=PlanFor(3,Night,20,NoCrawler,TEXT("low"));bLowTall&=L.Body==EBody::Tall&&!L.Id.IsEmpty();
   bNone&=PlanFor(3,Night,20,None).Id.IsEmpty();
  }
  Check(bStand&&bLowTall&&bNone,TEXT("a build without Tom's models has the stand-ins, and one without any figure has none"));
  Check(PlanFor(5,5,20,All,TEXT("low")).Id==TEXT("crawler")&&PlanFor(5,5,20,All,TEXT("tall")).Body==EBody::Tall&&PlanFor(5,5,20,All,TEXT("nordic")).Id==TEXT("nordic")&&PlanFor(5,5,20,All,TEXT("cosmic")).Id==TEXT("cosmic"),TEXT("F10 can ask for each"));
  Check(!PathOf(TEXT("zet"))&&!PathOf(TEXT("singer"))&&!PathOf(TEXT("bigfoot")),TEXT("Zet and the humorous ones are never a glimpse"));
 }
 // The crossing: from its side to the other, in its time, the stop on the crown when it looks; out of the lights and gone at the end.
 {
  FPlan P;P.Side=1;P.bLooks=true;P.Body=EBody::Tall;
  bool bOn=false,bStand=false;
  const double X0=CrossX(P,0,&bOn),XMid=CrossX(P,1.9,nullptr,&bStand);
  const double XEnd=CrossX(P,CrossSeconds(P)+.01,&bOn);
  Check(FMath::IsNearlyEqual(X0,EdgeM)&&FMath::Abs(XMid)<.01&&bStand&&FMath::IsNearlyEqual(XEnd,-EdgeM)&&!bOn&&Bob(P,1.9)==0.&&Bob(P,.3)>0.,TEXT("from the right shoulder across to the left, a stop on the crown, then gone"));
  FPlan L;L.Side=-1;L.Body=EBody::Low;
  Check(CrossX(L,0)<0&&CrossX(L,CrossSeconds(L)*.75)>0&&CrossSeconds(L)<CrossSeconds(P),TEXT("the low one from the left, and quicker"));
 }
 // Seen in the truck's lights: the near throw, or the far reach straight ahead; not wide, not past it.
 Check(InBeam(100,6)&&!InBeam(100,20)&&InBeam(70,20)&&!InBeam(130,2)&&!InBeam(60,40),TEXT("the lights reach 125 m straight ahead, 80 m to the side"));
 // Figures elsewhere: the shade, or Tom's Nordic or Cosmic when they are there, from the seed; never without the model.
 {
  int32 Shade=0,Nordic=0,Cosmic=0;bool bNever=true,bSame=true;
  for(int32 Seed=1;Seed<=40;++Seed)for(int32 Night=1;Night<=20;++Night)
  {
   const FString F=FigureFor(Seed,Night,0x484f525au,All);
   Shade+=F.IsEmpty();Nordic+=F==TEXT("nordic");Cosmic+=F==TEXT("cosmic");
   bNever&=FigureFor(Seed,Night,0x484f525au,StandIns).IsEmpty();
   bSame&=F==FigureFor(Seed,Night,0x484f525au,All);
  }
  Check(Shade>200&&Nordic>100&&Cosmic>100&&bNever&&bSame,FString::Printf(TEXT("the figure on the rise or on foot: the shade %d, Nordic %d, Cosmic %d of 800; only with the model"),Shade,Nordic,Cosmic));
  Check(FigureFor(1,1,1,All,TEXT("shade")).IsEmpty()&&FigureFor(1,1,1,All,TEXT("cosmic"))==TEXT("cosmic")&&FigureFor(1,1,1,None,TEXT("nordic")).IsEmpty(),TEXT("F10 can ask for each, and gets the shade without the model"));
 }
 // F10-NORDIC-COSMIC-20261109: the F10 lines for Tom's two, one per event and body, each asking for the body it says, on any seed
 // and night; Tom's own asset behind each; the close ones the Nordic by day and night and the Cosmic one's glow by night.
 {
  const TCHAR* const Events[]={TEXT("horizon_figure"),TEXT("figure_near"),TEXT("road_glimpse")};
  bool bEach=true,bAsk=true,bNamed=true,bHour=true;FString Missing;
  for(const TCHAR* Ev:Events)for(const TCHAR* Body:{TEXT("nordic"),TEXT("cosmic")})
  {
   const FDevBody* Line=nullptr;int32 N=0;
   for(const FDevBody& D:DevBodies())if(FCString::Strcmp(D.Event,Ev)==0&&FCString::Strcmp(D.Body,Body)==0){Line=&D;++N;}
   if(N!=1){bEach=false;Missing+=FString::Printf(TEXT(" %s/%s x%d"),Ev,Body,N);continue;}
   const FString Want=Line->Body;
   const TCHAR* Asset=PathOf(Want);
   bNamed&=FString(Line->Label).Contains(Want==TEXT("nordic")?TEXT("Nordic"):TEXT("Cosmic"))&&Asset&&FString(Asset).EndsWith(Want==TEXT("nordic")?TEXT(".SM_Tripo_Nordic"):TEXT(".SM_Tripo_Cosmic"));
   const bool bRoad=FCString::Strcmp(Ev,TEXT("road_glimpse"))==0;
   for(int32 Seed=1;Seed<=12;++Seed)for(int32 Night=1;Night<=12;++Night)
    bAsk&=bRoad?PlanFor(Seed,Night,22,All,Want).Id==Want:FigureFor(Seed,Night,0x46494731u,All,Want)==Want&&FigureFor(Seed,Night,0x484f525au,All,Want)==Want;
   // The rise and the crossing in the glimpse tier (00:30); the one on foot after two, in the full one.
   bHour&=FCString::Strcmp(Ev,TEXT("figure_near"))==0?Line->Hour>=2.&&Line->Hour<4.:FMath::IsNearlyEqual(Line->Hour,.5);
  }
  Check(bEach,FString::Printf(TEXT("F10 has one line for each of the Nordic and the Cosmic one on the rise, on foot and crossing the road%s"),*Missing));
  Check(bAsk&&bNamed,TEXT("each such F10 line asks for the body it names (SM_Tripo_Nordic, SM_Tripo_Cosmic), and gets it on any seed and night when the build has it"));
  Check(bHour,TEXT("the glimpses at 00:30, the one on foot after two"));
  bool bNordicDay=false,bNordicNight=false,bCosmicNight=false,bCloseOk=true;
  for(const FDevBody& D:DevBodies())if(FCString::Strcmp(D.Event,TEXT("close"))==0)
  {
   const FString B=D.Body;const bool bNight=D.Hour>=20.||D.Hour<5.;
   const S47TripoNew::FModel* Mo=S47TripoNew::Find(B);
   bCloseOk&=Mo&&(B==TEXT("nordic")||B==TEXT("cosmic"))&&FString(Mo->Path).EndsWith(B==TEXT("nordic")?TEXT(".SM_Tripo_Nordic"):TEXT(".SM_Tripo_Cosmic"))
    &&S47TripoNew::IsCosmic(B==TEXT("cosmic")?TEXT("SM_Tripo_Cosmic"):TEXT("SM_Tripo_Nordic"))==(B==TEXT("cosmic"));
   bNordicDay|=B==TEXT("nordic")&&!bNight;bNordicNight|=B==TEXT("nordic")&&bNight;bCosmicNight|=B==TEXT("cosmic")&&bNight;
  }
  Check(bCloseOk&&bNordicDay&&bNordicNight&&bCosmicNight,TEXT("F10 has the Nordic close by day and by night, and the Cosmic one's glow close by night, each Tom's own model"));
  Check(FCString::Strcmp(DevBodyName(TEXT("nordic")),TEXT("the Nordic"))==0&&FCString::Strcmp(DevBodyName(FString()),TEXT("the shade"))==0,TEXT("the F10 lines name the body they got"));
 }
 // The words: what was seen and where from, never what it was.
 {
  FPlan T;T.Side=-1;T.bLooks=true;FPlan L;L.Body=EBody::Low;
  const FString A=Record(T,TEXT("00:40")),B=Record(L,TEXT("01:15"));
  Check(A.Contains(TEXT("00:40"))&&A.Contains(TEXT("from the left"))&&A.Contains(TEXT("crown"))&&B.Contains(TEXT("four legs"))&&B.Contains(TEXT("from the right"))&&TracksRecord(true,TEXT("09:10"),true).Contains(TEXT("fours"))&&TracksRecord(false,TEXT("01:30"),false).Contains(TEXT("by the lamp")),TEXT("the record says when, from which side, and what it looked like"));
 }
 return OK;
}
