#include "S47Predecessor.h"
#include "S47Audio.h"
#include "S47EventPlaces.h"
#include "S47Freeform.h"
#include "S47FreeformCases.h"
#include "S47Mail.h"
#include "S47Main.h"
#include "S47PredecessorText.h"
#include "S47Townsfolk.h"
#include "S47UI.h"
#include "S47Visitors.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "S47World.h"

// M7, weeks three and four (9 November: "innholdet tar slutt rundt dag 10-14"). What the late cases (FS47FreeformCases' PostLine,
// MinuteLine and RakedLine) have out in the world, and the rest of Reyes's trail after wagon 23:
//   THE EMPTY POST      somebody in Caliche gone from their post (FS47Townsfolk): their place in town, where a telephone or a radio
//                       goes on to nobody, and the station's receiver log of that night, which the printer at the rack runs off by
//                       itself. Nobody gone between day 15 and 20, and nobody for a week: the night's events take somebody, in the
//                       morning, while the player is at the station (FS47Townsfolk::Vanish).
//   THE PRINTED MINUTE  the Socorro Sky Watch (FS47Visitors' van) sends the station its bulletin: something at ten to one on the third
//                       night from the postmark, to be seen from the cairn. At that minute the receiver has a burst; the chart keeps
//                       it, and the cairn has something too, for a day and a half.
//   THE RAKED GROUND    the ground the military went over (FS47Visitors' own record of it), and the receiver log of that morning.
//   The trip meter (OpenWagon), the attendant's thermos (his talk), Dale's word on it, and a letter from Ortiz's sister by post.
// Every trace is the same sight or sound under every cause; what the record says is the case's seeded reading of that place, so no
// place gives the answer away. The evening is left to its small hints, and nothing here comes near the far side of Story.
namespace
{
 const TCHAR* const LateKey=TEXT("late-place");
 const FVector Rack(-3,1,1); // the receiver rack in the control room, SARO's terms (FS47FreeformDirector's)
 constexpr double PostHear=45,PostReach=9,CairnHear=40,CairnReach=10,MinuteLive=180,ChartKeeps=2*86400.,CairnKeeps=35*3600.;
}

FString FS47Predecessor::LateAway() const
{
 if(!M||!M->Freeform||!M->Freeform->Townsfolk)return FString();
 for(const FString& Who:FS47Townsfolk::People())if(M->Freeform->Townsfolk->Away(Who))return Who;
 return FString();
}
bool FS47Predecessor::InControlRoom() const
{
 return M&&M->Player&&IsValid(M->Web.Get())&&M->Web->Area()==TEXT("saro")&&!(M->World&&M->World->bDriving)&&M->Web->Rect(TEXT("room.bounds")).Contains(M->Player->Pos);
}
void FS47Predecessor::LateSound(const FString& Name,const FVector& At,float Gain)
{
 if(!M||!IsValid(M->Audio))return;
 if(bLateLoop&&LateLoopName==Name)return;
 if(bLateLoop)M->Audio->Stop(LateKey,.2f);
 FS47Sound O;O.Gain=Gain;O.bLoop=true;O.At=At;
 M->Audio->LoopSfx(LateKey,Name,O);bLateLoop=true;LateLoopName=Name;
 UE_LOG(LogTemp,Display,TEXT("S47: late: %s in earshot"),*Name);
}
void FS47Predecessor::LateQuiet()
{
 if(bLateLoop&&M&&IsValid(M->Audio))M->Audio->Stop(LateKey,.3f);
 bLateLoop=false;LateLoopName.Empty();
}
bool FS47Predecessor::LateKeep(int32 Line,int32 Clue,const FString& Toast)
{
 if(!M||!M->Freeform)return false;
 auto& F=*M->Freeform;
 const FS47FreeformCases::FMysteryDef* Y=FS47FreeformCases::Def(Line).Mystery;
 if(!Y||Clue<0||Clue>=FS47FreeformCases::ClueCount)return false;
 const FString Id=Y->Clues[Clue].Finding;
 if(F.HasFinding(Id))return false;
 // The record is the case's own reading of that place, from the seed; the first one opens the case (FS47FreeformSession::OpenCaseFor).
 F.RecordEvidence(Id,F.CaseFile().ClueText(Line,Clue));
 if(M->UI&&!Toast.IsEmpty())M->UI->Toast(Toast,6);
 UE_LOG(LogTemp,Display,TEXT("S47: late: %s kept for %s, day %d (reading %d)"),*Id,FS47FreeformCases::Def(Line).Title,F.Day(),F.CaseFile().ClueReading(Line,Clue));
 M->RequestAutosave();
 return true;
}

void FS47Predecessor::StepLate(double Dt)
{
 if(!M||!M->Freeform||bLeft)return;
 LateIn-=Dt;if(LateIn>0)return;LateIn=.5;
 auto& F=*M->Freeform;
 if(F.InTutorial()||(M->World&&M->World->bBusy))return;
 const int32 Today=F.Day();
 // Whoever was gone from Caliche, and when, from the first day: the third week's own taking keeps a week from the last one.
 if(!LateAway().IsEmpty())LateAwayDay=Today;
 StepLateThermos(Today);
 if(Today<LateCaseDay&&!bLateNow){LateQuiet();return;}
 StepLatePost(Today,FMath::Fmod(FMath::Max(0.,F.Clock()),86400.)/3600.);
 StepLateMinute(Today);
 StepLateRaked(Today);
 StepLateRack();
}

void FS47Predecessor::StepLatePost(int32 Today,double Hour)
{
 auto& F=*M->Freeform;
 const int32 Line=FS47FreeformCases::PostLine;
 const FString Who=LateAway();
 const auto* K=F.CaseFile().Get(Line);const bool bOpen=K&&K->bOpen;
 // Nobody gone in the third week and nobody for a week before it: the night's events take somebody from their post, found in the
 // morning, while the player is at the station and not on the street to see it.
 if(Who.IsEmpty()&&!bOpen&&LatePostDay==0&&Today>=PostFrom&&Today<=PostUntil&&Today-LateAwayDay>=AwayRest&&Hour>=8&&Hour<12&&F.Townsfolk&&F.RulesLive()&&M->Web&&M->Web->Area()==TEXT("saro"))
 {
  const TArray<FString>& People=FS47Townsfolk::People();
  if(People.IsEmpty())return;
  const FString Pick=People[int32((uint32(F.Seed())>>3)+uint32(Today))%People.Num()];
  if(F.Townsfolk->Vanish(Pick,FS47Townsfolk::WorksAt(Pick)))
  {
   LatePostDay=LateAwayDay=Today;
   UE_LOG(LogTemp,Display,TEXT("S47: late: nobody gone from Caliche in the third week: the night took %s, day %d"),*Pick,Today);
   M->RequestAutosave();
  }
  return;
 }
 const FS47EventPlace* P=!Who.IsEmpty()&&!bOpen&&F.EventPlaces?F.EventPlaces->Find(FS47Townsfolk::WorksAt(Who)):nullptr;
 const FString Sound=P&&P->Has(ES47PlaceTrait::Phone)?TEXT("phoneRing"):TEXT("radio_static");
 FVector Door=FVector::ZeroVector;
 const bool bHere=P&&F.EventPlaces->EntryNow(P->Id,Door);
 const double D=bHere?F.EventPlaces->DistanceFromPlayer(*P):-1;
 if(!bHere||D<0||D>PostHear){if(bLateLoop&&(LateLoopName==TEXT("phoneRing")||LateLoopName==TEXT("radio_static")))LateQuiet();return;}
 // From the street: the same telephone or the same radio, whatever it was. At the door, it is kept.
 LateSound(Sound,Door+FVector(0,1.2,0),.22f);
 if(D<PostReach&&!(M->World&&M->World->bDriving))
  LateKeep(Line,0,Sound==TEXT("phoneRing")?TEXT("Nobody there. Inside, a telephone rings on and on, and nobody comes to it."):TEXT("Nobody there. Inside, a radio plays to the empty room."));
}

void FS47Predecessor::StepLateMinute(int32 Today)
{
 auto& F=*M->Freeform;
 const int32 Line=FS47FreeformCases::MinuteLine;
 if(LateBulletinDay==0)
 {
  // After the van has been at the Mesa once in the third week, whatever was said to them; by day 15 in any case. No Sky Watch in a
  // game without them.
  if(!F.Visitors)return;
  const bool bVan=F.Visitors->Has(TEXT("let_in"))||F.Visitors->Has(TEXT("turned_away"))||F.Visitors->Has(TEXT("tie_seen"));
  if(!bVan&&Today<BulletinBy&&!bLateNow)return;
  LateBulletinDay=Today;
  const bool bByPost=F.Mail&&F.Mail->Live();
  Send(TEXT("skywatch"));
  if(M->UI&&!bByPost)M->UI->Toast(TEXT("Something has come for the station from the Socorro Sky Watch. It is in the OPS tray."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: late: the Sky Watch's bulletin posted on day %d; the minute is at %.0f"),Today,MinuteClock());
  M->RequestAutosave();
  return;
 }
 const auto* K=F.CaseFile().Get(Line);if(K&&K->bConcluded)return;
 const double At=MinuteClock(),Now=F.Clock();
 if(Now<At)return;
 // The rack: at the minute itself the meter kicks and the printer runs; later the chart has it, for two days.
 if(!F.HasFinding(TEXT("minute_chart"))&&Now<At+ChartKeeps&&InControlRoom())
 {
  const bool bLive=Now<At+MinuteLive;
  if(M->Audio)
  {
   if(bLive){FS47Sound O;O.Gain=.14f;O.At=Rack;O.Dest=TEXT("radio");const auto H=M->Audio->Sfx(TEXT("alienRadio"),O);if(H&&M->Audio->Ctx)H.Src->Stop(M->Audio->Ctx->CurrentTime()+1.6);}
   FS47Sound Print;Print.Gain=.35f;Print.At=Rack;M->Audio->Sfx(TEXT("printer"),Print);
  }
  LateKeep(Line,0,bLive?TEXT("The receiver meter kicks at ten to one and settles. Hash from the rack, and the printer runs a line by itself."):TEXT("The receiver chart has a burst on it at ten to one, on the night the bulletin gave. You keep the strip."));
 }
 // The cairn: the wind in the stacked stones as one comes up the rise, and whatever is there, for a day and a half.
 if(!F.HasFinding(TEXT("minute_cairn"))&&Now<At+CairnKeeps&&F.EventPlaces)
  if(const FS47EventPlace* P=F.EventPlaces->Find(TEXT("cairn")))
  {
   FVector Spot=FVector::ZeroVector;const bool bHere=F.EventPlaces->AtNow(P->Id,Spot);
   const double D=bHere?F.EventPlaces->DistanceFromPlayer(*P):-1;
   if(!bHere||D<0||D>CairnHear)return;
   if(!bCairnHeard&&M->Audio){bCairnHeard=true;FS47Sound O;O.Gain=.5f;O.At=Spot+FVector(0,1.5,0);M->Audio->Sfx(TEXT("gust"),O);}
   if(D<CairnReach&&!(M->World&&M->World->bDriving))LateKeep(Line,1,TEXT("Wind on the rise, whistling in the stacked stones of the cairn."));
  }
}

void FS47Predecessor::StepLateRaked(int32 Today)
{
 auto& F=*M->Freeform;
 if(F.HasFinding(TEXT("raked_ground"))||!F.Visitors)return;
 // The visitors keep their own record when the player gets to the ground they went over ("military_<place>_<night>"), with the
 // raked circle drawn there. From the third week that is where the case opens, the same moment.
 for(const FString& Id:F.FindingIdsFrom(bLateNow?0:LateCaseDay))
  if(Id.StartsWith(TEXT("military_"))){LateKeep(FS47FreeformCases::RakedLine,0,FString());return;}
 (void)Today;
}

void FS47Predecessor::StepLateRack()
{
 auto& F=*M->Freeform;
 // The printer runs a strip by itself for the receiver log of that night or that morning, once one is in the control room.
 for(const int32 Line:{FS47FreeformCases::PostLine,FS47FreeformCases::RakedLine})
 {
  const auto* K=F.CaseFile().Get(Line);if(!K||!K->bOpen||K->bConcluded)continue;
  const FString Id=FS47FreeformCases::Def(Line).Mystery->Clues[1].Finding;if(F.HasFinding(Id))continue;
  if(!InControlRoom()){LateRackAt=-1;return;}
  const double Real=FPlatformTime::Seconds();
  if(LateRackAt<0)
  {
   LateRackAt=Real;
   if(M->Audio){FS47Sound O;O.Gain=.4f;O.At=Rack;M->Audio->Sfx(TEXT("printer"),O);}
   return;
  }
  if(Real-LateRackAt<3)return;
  LateRackAt=-1;
  LateKeep(Line,1,Line==FS47FreeformCases::PostLine?TEXT("The printer at the rack has run a strip by itself: the receiver log from the night they went."):TEXT("The printer at the rack has run a strip by itself: the receiver log from first light that morning."));
  return;
 }
}

void FS47Predecessor::StepLateThermos(int32 Today)
{
 auto& F=*M->Freeform;
 // The thermos goes in the outgoing tray with a tag, on Dale's word, or two days after the attendant handed it over without it; the
 // district runs it to Socorro. A few days on, Ortiz's sister writes.
 if(LateThermosDay==0&&Seen.Contains(TEXT("talk_attendant_thermos")))
 {
  const int32 Had=F.FindingDay(TEXT("reyes_talk_attendant_thermos"));
  if(Seen.Contains(TEXT("bark_thermos"))||(Had>0&&Today>=Had+2))
  {
   LateThermosDay=Today;
   UE_LOG(LogTemp,Display,TEXT("S47: late: unit 23's thermos goes to the district, day %d"),Today);
   M->RequestAutosave();
  }
 }
 if(LateThermosDay>0&&!Seen.Contains(TEXT("ortiz_reply_sent"))&&Today>=LateThermosDay+ThermosReplyDays&&WardAvailable())
 {
  Seen.Add(TEXT("ortiz_reply_sent"));
  const bool bByPost=F.Mail&&F.Mail->Live();
  Send(TEXT("ortiz_reply"));
  if(M->UI&&!bByPost)M->UI->Toast(TEXT("A letter has come from Socorro for the night operator. It is in the OPS tray."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: late: a letter from Ortiz's sister, day %d"),Today);
  M->RequestAutosave();
 }
}

// ---------- F10 ----------
bool FS47Predecessor::TestLate(const FString& What,FString& Why)
{
 if(!M||!M->Freeform){Why=TEXT("Freeform only");return false;}
 TestOn();bLateNow=true;
 auto& F=*M->Freeform;
 const int32 Today=F.Day();
 if(What==TEXT("post"))
 {
  if(!F.Townsfolk){Why=TEXT("no townsfolk in this game");return false;}
  FString Who=LateAway();
  if(Who.IsEmpty())
  {
   const TArray<FString>& People=FS47Townsfolk::People();
   for(const FString& Each:People)if(F.Townsfolk->Vanish(Each,FS47Townsfolk::WorksAt(Each))){Who=Each;break;}
  }
  Why=Who.IsEmpty()?FString(TEXT("nobody could be made gone")):FString::Printf(TEXT("%s is gone from %s; their place rings or plays from 45 m, and is kept at the door"),*FS47Townsfolk::Called(Who),*FS47Townsfolk::WorksAt(Who));
  return !Who.IsEmpty();
 }
 if(What==TEXT("bulletin")){LateBulletinDay=0;LateMinuteAt=0;StepLateMinute(Today);Why=TEXT("the Sky Watch's bulletin is in the box (or the tray)");return true;}
 if(What==TEXT("minute"))
 {
  if(LateBulletinDay==0)LateBulletinDay=Today;
  LateMinuteAt=F.Clock()+60;bCairnHeard=false;
  Why=TEXT("the printed minute is one minute from now: the meter and the printer at the rack, or the cairn");return true;
 }
 if(What==TEXT("trip")){Seen.Add(TEXT("callout"));Seen.Add(TEXT("wagon"));OpenWagon();Why=TEXT("wagon 23 as from day 15, with the call-out read");return true;}
 if(What==TEXT("thermos")){Seen.Add(TEXT("trip"));ShowTalk(TEXT("attendant"),TEXT("thermos"));Why=TEXT("the attendant's thermos");return true;}
 if(What==TEXT("reply")){Seen.Add(TEXT("talk_attendant_thermos"));Seen.Remove(TEXT("ortiz_reply_sent"));LateThermosDay=FMath::Max(1,Today-ThermosReplyDays);StepLateThermos(Today);Why=TEXT("the letter from Ortiz's sister is in the box (or the tray)");return true;}
 if(What==TEXT("raked"))
 {
  if(!F.Visitors||!F.Visitors->DevRaked(TEXT("farm"))){Why=TEXT("no visitors here");return false;}
  Why=TEXT("the farm's ground raked as the military left it; walk up to it to read it, and the case opens");return true;
 }
 Why=TEXT("nothing by that name");return false;
}
bool FS47Predecessor::LateSpot(const FString& What,FString& OutArea,double& OutX,double& OutZ) const
{
 if(What==TEXT("rack")){OutArea=TEXT("saro");OutX=0;OutZ=1.5;return true;}
 if(!M||!M->Freeform||!M->Freeform->EventPlaces)return false;
 const FString Id=What==TEXT("post")?FS47Townsfolk::WorksAt(LateAway()):What==TEXT("cairn")?FString(TEXT("cairn")):What==TEXT("raked")?FString(TEXT("farm")):FString();
 const FS47EventPlace* P=Id.IsEmpty()?nullptr:M->Freeform->EventPlaces->Find(Id);
 if(!P)return false;
 FVector At=FVector::ZeroVector;M->Freeform->EventPlaces->EntryNow(P->Id,At);
 OutArea=FS47EventPlaces::AreaOf(*P);OutX=At.X+3;OutZ=At.Z+3;
 return true;
}
