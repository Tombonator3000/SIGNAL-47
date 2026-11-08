#include "S47FreeformTest.h"
#include "S47EventPlaces.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Ledger.h"
#include "S47Mail.h"
#include "S47Main.h"
#include "S47Predecessor.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"

// -S47Freeform -S47FreeformTest -S47MailTest (MAILBOX-20261108): the box at the road as a player meets it. The catalog in it and the
// flag down on a new game; the eye's ray and E at the box; an order by mail with a money order and the flag up; the carrier's car
// coming up the highway, stopping at the box, taking the order and putting the flag down; the days until the parcel sits on the
// box; the parcel in both hands; then two weeks of the station's findings, recorded as the nights record them (their own ids and
// words, at real places on the land): cattle in a ring and a lit window on one night, a dead cow the next, then a dead cow
// somewhere else and jackrabbits running the week after. Each Thursday's paper must print them at their places and on their nights,
// and the second dead cow must not read like the first. Then a Continue.
namespace S47MailTestLocal
{
 const TCHAR* const Days[]={TEXT("Monday"),TEXT("Tuesday"),TEXT("Wednesday"),TEXT("Thursday"),TEXT("Friday"),TEXT("Saturday"),TEXT("Sunday")};
 FString Night(const FS47FreeformSession& Ses,int32 OnDay){return FString(Days[int32(Ses.DateOfDay(OnDay).GetDayOfWeek())])+TEXT(" night");}
 // The next Thursday after OnDay (a paper comes with the round that day).
 int32 NextThursday(const FS47FreeformSession& Ses,int32 OnDay){int32 D=OnDay+1;while(Ses.DateOfDay(D).GetDayOfWeek()!=EDayOfWeek::Thursday)++D;return D;}
 // A line of the paper that has all of these in it.
 bool Says(const FString& Text,const TArray<FString>& All)
 {
  TArray<FString> Lines;Text.ParseIntoArray(Lines,TEXT("\n"),true);
  for(const FString& Line:Lines){bool bAll=true;for(const FString& Bit:All)bAll&=Line.Contains(Bit,ESearchCase::CaseSensitive);if(bAll)return true;}
  return false;
 }
 // Which dead-cow story a paper printed.
 FString CowHead(const FString& Text){for(const S47LedgerText::FFrontDef& D:S47LedgerText::Fronts())if(D.Topic==S47Ledger::ETopic::Carcass&&Text.Contains(D.Head,ESearchCase::CaseSensitive))return D.Head;return FString();}
 // Past the carrier's hour far enough that the round is made without the car.
 void PastTheRound(FS47FreeformSession& Ses){const double After=FS47Mail::RoundClock(Ses.Seed(),Ses.Day())+20*60.;if(Ses.Clock()<After)Ses.TestAdvance(After-Ses.Clock());}
}

void FS47FreeformTest::StepMail(double Dt)
{
 namespace L=S47MailTestLocal;
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>900){Finish(false,TEXT("mail journey exceeded its time"));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter)return;
 auto& Ses=*M->Freeform;if(!Ses.Mail||!Ses.Field)return;auto& Post=*Ses.Mail;
 static int32 Cash=0,Waited=0,Base=0,Week=0,Thursday=0,Rounds=0;
 static FString PlaceA,PlaceB,PlaceC,Second;
 static FString CowA,CowC;
 if(Stage==0)
 {
  if(Age<3)return;
  M->UI->Close(true);Ses.TestLiveRules();Ses.TestEndFirstShift();Post.TestOn();
  if(!Check(Post.Live()&&Post.Holds(TEXT("catalog"))&&!Post.FlagUp(),TEXT("a new game: the catalog is in the box at the road and the flag is down (the carrier never puts it up)")))return;
  FString Area;double X=0,Z=0;Post.WorldSpot(Area,X,Z);
  if(!Check(Area==TEXT("saro")&&M->Web->Area()==TEXT("saro"),TEXT("the box stands in SARO's area")))return;
  // In front of it, on the highway's shoulder, looking at the door.
  M->Player->Place(X,Z,-UE_DOUBLE_HALF_PI);M->Player->Pitch=-.36;
  Go(10,1.5);return;
 }
 if(Stage==10){Shot(TEXT("mail-box-front"));Go(11,.3);return;}
 if(Stage==11)
 {
  const FString Pick=M->Inter->Pick(M->Player->EyeUE(),M->Player->ForwardUE());
  if(!Check(Pick==TEXT("ff:mail:box")&&bool(M->Player->OnUse),TEXT("the eye's ray finds the mailbox; got ")+Pick))return;
  M->Player->OnUse(Pick);
  if(!Check(!Post.Holds(TEXT("catalog"))&&M->ModalOpen(),TEXT("E opens the box and the catalog comes out")))return;
  Go(12,.5);return;
 }
 if(Stage==12)
 {
  Shot(TEXT("mail-box-panel"));M->UI->Close(true);
  if(!Check(!Post.FlagUp()&&!Post.MailIn(),TEXT("the box is empty and the flag is down: nothing waits for the carrier")))return;
  Cash=Ses.Budget();
  Go(20,.3);return;
 }
 if(Stage==20)
 {
  TMap<FString,int32> Form;Form.Add(TEXT("cell"),2);FString Why;
  if(!Check(Post.TestOrder(Form,Why),TEXT("an order for two dry cells is written: ")+Why))return;
  const int32 Paid=Cash-Ses.Budget();
  if(!Check(Paid>0&&Paid<=2*FS47FreeformWorld::ShopPrice(TEXT("cell"))+FS47Mail::Postage&&Post.FlagUp()&&Post.OrdersOut()==1,FString::Printf(TEXT("the money order ($%d) is in the box and the flag is up"),Paid)))return;
  if(!Check(Post.TaskLine().Contains(TEXT("MAILBOX")),TEXT("and the list says the order waits for the carrier")))return;
  // On the truck pad, looking at the box, as the station sees the road; the carrier's car comes up from the south.
  M->Player->Place(9.8,10.,2.19);M->Player->Pitch=-.05;
  Rounds=Post.RoundsMade();
  if(!Check(Post.TestCarrier()&&Post.CarrierDriving(),TEXT("the carrier's car is on the highway, coming up from the south")))return;
  At=Age;Go(21,.5);return;
 }
 if(Stage==21)
 {
  // The car pulls in at the box and stands there, its light going; then the round is made from its window.
  if(Post.CarrierStopped()){Shot(TEXT("mail-carrier-at-box"));Go(22,2.5);return;}
  if(!Post.CarrierDriving()){Finish(false,TEXT("the carrier's car went by without stopping, with an order in the box"));return;}
  if(Age-At>90){Finish(false,TEXT("the carrier's car never reached the box"));return;}
  return;
 }
 if(Stage==22)
 {
  if(!Check(Post.RoundsMade()==Rounds+1&&!Post.FlagUp()&&Post.OrdersOut()==1&&Post.TaskLine().Contains(TEXT("MAIL ORDER")),TEXT("at the box the carrier takes the order and the flag comes down; the list has the parcel due")))return;
  At=Age;Go(23,.5);return;
 }
 if(Stage==23)
 {
  // On north and gone.
  if(!Post.CarrierDriving()){Waited=0;Go(24,.2);return;}
  if(Age-At>90){Finish(false,TEXT("the carrier's car never left"));return;}
  return;
 }
 if(Stage==24)
 {
  // Day by day, past the carrier's hour, until the parcel is on the box: one to three rounds, never on a Sunday.
  if(Post.ParcelOnBox()){Go(30,.3);return;}
  if(++Waited>6){Finish(false,TEXT("the parcel never came"));return;}
  Ses.TestAdvance(86400.);L::PastTheRound(Ses);
  Hold=.4;return;
 }
 if(Stage==30)
 {
  if(!Check(!Post.FlagUp()&&Post.TaskLine().Contains(TEXT("parcel")),TEXT("the parcel sits on top of the box with the flag still down, and the list has it due")))return;
  Shot(TEXT("mail-parcel-on-box"));
  if(!Check(Post.TestLift()&&Ses.Field->ParcelStage()==2&&Ses.Field->ParcelContents().FindRef(TEXT("cell"))==2,TEXT("lifted down, the parcel is in both hands with its two dry cells")))return;
  if(!Check(!Post.ParcelOnBox(),TEXT("and nothing is left on the box")))return;
  // Three places on the land for the nights to happen at: in town, at a farm, somewhere else.
  PlaceA.Empty();PlaceB.Empty();PlaceC.Empty();
  if(Ses.EventPlaces)
  {
   for(const FS47EventPlace& P:Ses.EventPlaces->All())
   {
    if(P.Region==TEXT("saro")||P.Title.IsEmpty())continue;
    if(PlaceA.IsEmpty()&&P.Region==TEXT("caliche"))PlaceA=P.Title;
    else if(PlaceB.IsEmpty()&&P.Region==TEXT("farm"))PlaceB=P.Title;
   }
   for(const FS47EventPlace& P:Ses.EventPlaces->All())
   {
    if(P.Region==TEXT("saro")||P.Title.IsEmpty()||P.Title==PlaceA||P.Title==PlaceB)continue;
    if(PlaceA.IsEmpty())PlaceA=P.Title;else if(PlaceB.IsEmpty())PlaceB=P.Title;else if(PlaceC.IsEmpty()&&!P.Title.Contains(PlaceA)&&!PlaceA.Contains(P.Title)&&!P.Title.Contains(PlaceB)&&!PlaceB.Contains(P.Title))PlaceC=P.Title;
   }
  }
  if(!Check(!PlaceA.IsEmpty()&&!PlaceB.IsEmpty()&&!PlaceC.IsEmpty(),FString::Printf(TEXT("three places on the land for the nights: %s, %s, %s"),*PlaceA,*PlaceB,*PlaceC)))return;
  // The first week's nights fall on the two days before a Thursday still to come; a paper on the way there is counted too.
  Base=Post.IssuesPrinted();Week=1;Thursday=L::NextThursday(Ses,Ses.Day());Waited=0;
  if(Thursday-2<Ses.Day()){Thursday+=7;++Base;}
  Go(40,.4);return;
 }
 if(Stage==40)
 {
  // The nights of two weeks, as FS47Happenings records them, on the days before each Thursday.
  const int32 Today=Ses.Day();
  if(Week==1&&Today==Thursday-2)
  {
   Ses.RecordEvidence(FString::Printf(TEXT("cattle_ring_d%d"),Today),FString::Printf(TEXT("Seven head of cattle out by %s stood close in a ring, all facing about 212 degrees, southwest, and did not move off when I walked in among them."),*PlaceA));
   Ses.RecordEvidence(FString::Printf(TEXT("house_lit_d%d"),Today),FString::Printf(TEXT("At 01:40 a light was on in a window at %s, where nobody lives."),*PlaceB));
  }
  if(Week==1&&Today==Thursday-1)Ses.RecordEvidence(FString::Printf(TEXT("carcass_d%d"),Today),FString::Printf(TEXT("A dead cow out by %s in a ring of flattened grass: clean cuts, no blood, no tracks in or out."),*PlaceA));
  if(Week==2&&Today==Thursday-2)Ses.RecordEvidence(FString::Printf(TEXT("carcass_d%d"),Today),FString::Printf(TEXT("A dead cow out by %s in a ring of flattened grass: clean cuts, no blood, no tracks in or out."),*PlaceC));
  if(Week==2&&Today==Thursday-1)Ses.RecordEvidence(FString::Printf(TEXT("animals_flee_d%d"),Today),TEXT("At 23:15 jackrabbits, a dozen or more, went past me all one way, toward the west, flat out, and not one looked at me."));
  Go(41,.3);return;
 }
 if(Stage==41)
 {
  // A paper printed: check it, then on to the next week.
  if(Post.IssuesPrinted()>=Base+Week)
  {
   const int32 No=Base+Week;const FString Text=Post.IssueText(No);
   FString Word;
   if(!Check(!Text.IsEmpty()&&S47Ledger::Clean(Text,Word)&&!Text.Contains(TEXT("{"))&&!Text.Contains(TEXT("somewhere out of town"))&&!Text.Contains(TEXT("dust"))&&!Text.Contains(TEXT("moon")),FString::Printf(TEXT("the Ledger no. %d is printed, clean, nothing left unfilled, no weather the sky does not show"),No)))return;
   if(Week==1)
   {
    const FString Tue=L::Night(Ses,Thursday-2),Wed=L::Night(Ses,Thursday-1);
    UE_LOG(LogTemp,Display,TEXT("S47: mail test: Ledger no. %d:\n%s"),No,*Text);
    if(!Check(L::Says(Text,{PlaceA,Wed,TEXT("cow")}),FString::Printf(TEXT("the dead cow is printed out by %s, %s"),*PlaceA,*Wed)))return;
    if(!Check(L::Says(Text,{PlaceB,Tue})&&(L::Says(Text,{PlaceB,TEXT("light")})||L::Says(Text,{PlaceB,TEXT("window")})),FString::Printf(TEXT("the lit window is printed at %s, %s"),*PlaceB,*Tue)))return;
    if(!Check(L::Says(Text,{PlaceA,Tue,TEXT("attle")}),FString::Printf(TEXT("and the cattle in a ring at %s, %s (front page or sheriff's log)"),*PlaceA,*Tue)))return;
    if(!Check(!Text.Contains(TEXT("jackrabbits")),TEXT("nothing from next week, and no kind of night that did not happen")))return;
    CowA=L::CowHead(Text);
    Week=2;Thursday=L::NextThursday(Ses,Ses.Day());
   }
   else
   {
    const FString Tue=L::Night(Ses,Thursday-2);
    UE_LOG(LogTemp,Display,TEXT("S47: mail test: Ledger no. %d:\n%s"),No,*Text);
    if(!Check(L::Says(Text,{PlaceC,Tue,TEXT("cow")}),FString::Printf(TEXT("the second dead cow is printed out by %s, %s"),*PlaceC,*Tue)))return;
    CowC=L::CowHead(Text);
    if(!Check(!CowA.IsEmpty()&&!CowC.IsEmpty()&&CowA!=CowC,FString::Printf(TEXT("the second dead cow does not read like the first: \"%s\" then \"%s\""),*CowA,*CowC)))return;
    if(!Check(!L::Says(Text,{PlaceA,TEXT("cow")})&&!L::Says(Text,{PlaceA,TEXT("attle")})&&!L::Says(Text,{PlaceB,TEXT("window")}),TEXT("last week's dead cow, cattle and lit window are not printed again")))return;
    const S47Ledger::FFact* Fled=nullptr;const TArray<S47Ledger::FFact> Facts=Post.WeekFacts(Thursday-7,Thursday-1);
    for(const S47Ledger::FFact& Fa:Facts)if(Fa.Topic==S47Ledger::ETopic::Fled)Fled=&Fa;
    if(!Check(Fled&&(Fled->Place.IsEmpty()||L::Says(Text,{Fled->Place,TEXT("abbit")})),TEXT("the jackrabbits are their own story, at the place the player was, not cattle")))return;
    Second=Text;
    Go(45,.3);return;
   }
  }
  if(++Waited>30){Finish(false,TEXT("no Ledger came on a Thursday"));return;}
  Ses.TestAdvance(86400.);L::PastTheRound(Ses);
  Go(40,.4);return;
 }
 if(Stage==45)
 {
  // The first paper the player reads puts wagon 23 on the map.
  if(!Check(Post.Ever(TEXT("ledger:1")),TEXT("the first Ledger came")))return;
  Post.TestFetch();M->UI->Close(true);
  Post.OpenLedger(1);
  if(!Check(Ses.Predecessor&&Ses.Predecessor->Heard(TEXT("wagon")),TEXT("read, the paper's search story puts wagon 23 on the map")))return;
  Go(46,.6);return;
 }
 if(Stage==46){Shot(TEXT("mail-ledger"));M->UI->Close(true);Post.OpenLedger(Base+2);Go(47,.6);return;}
 if(Stage==47){Shot(TEXT("mail-ledger-findings"));M->UI->Close(true);Go(50,.3);return;}
 if(Stage==50)
 {
  if(!Restore(TEXT("mail-continue")))return;
  Go(51,1.);return;
 }
 if(Stage==51)
 {
  if(!Check(Post.IssuesPrinted()>=Base+2&&Post.Ever(TEXT("ledger:1"))&&Post.Ever(TEXT("catalog"))&&Post.RoundsMade()>0&&Post.IssueText(Base+2)==Second,TEXT("the papers printed, as printed, and the rounds made survive a Continue")))return;
  if(!Check(Post.Noted().ContainsByPredicate([](const FS47Mail::FSeen& S){return S.Id.StartsWith(TEXT("carcass_d"))&&S.Place==PlaceC;}),TEXT("and the findings noted with their places")))return;
  if(!Check(Ses.Field->ParcelStage()==2,TEXT("and the parcel is still in hand")))return;
  Finish(true,TEXT("box, flag, eye-ray, order, the carrier's car, parcel, two Ledgers from the station's findings at their places, and Continue"));return;
 }
}
