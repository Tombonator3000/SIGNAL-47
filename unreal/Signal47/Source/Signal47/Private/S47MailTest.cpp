#include "S47FreeformTest.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47FreeformWorld.h"
#include "S47Inter.h"
#include "S47Mail.h"
#include "S47Main.h"
#include "S47Predecessor.h"
#include "S47Startup.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"

// -S47Freeform -S47FreeformTest -S47MailTest (MAILBOX-20261108): the box at the road as a player meets it. The catalog in it and the
// flag up on a new game; the eye's ray and E at the box; an order by mail with a money order; the carrier's round that takes it; the
// days until the parcel comes; the parcel in both hands; Thursday's Ledger and the wagon it points at; and everything after a Continue.
void FS47FreeformTest::StepMail(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>600){Finish(false,TEXT("mail journey exceeded its time"));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Inter)return;
 auto& Ses=*M->Freeform;if(!Ses.Mail||!Ses.Field)return;auto& Post=*Ses.Mail;
 static int32 Cash=0,Waited=0;
 if(Stage==0)
 {
  if(Age<3)return;
  M->UI->Close(true);Ses.TestLiveRules();Ses.TestEndFirstShift();Post.TestOn();
  if(!Check(Post.Live()&&Post.Holds(TEXT("catalog"))&&Post.FlagUp(),TEXT("a new game: the catalog is in the box at the road and the flag is up")))return;
  FString Area;double X=0,Z=0;Post.WorldSpot(Area,X,Z);
  if(!Check(Area==TEXT("saro")&&M->Web->Area()==TEXT("saro"),TEXT("the box stands in SARO's area")))return;
  // In front of it, on the highway's shoulder, looking at the door.
  M->Player->Place(X,Z,-UE_DOUBLE_HALF_PI);M->Player->Pitch=-.36;
  Go(10,1.5);return;
 }
 if(Stage==10){Shot(TEXT("mail-box-flag-up"));Go(11,.3);return;}
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
  if(!Check(Post.FlagUp()==(Post.InBox()>0),TEXT("the flag is up exactly while something is in the box")))return;
  Cash=Ses.Budget();
  Go(20,.3);return;
 }
 if(Stage==20)
 {
  TMap<FString,int32> Form;Form.Add(TEXT("cell"),2);FString Why;
  if(!Check(Post.TestOrder(Form,Why),TEXT("an order for two dry cells is written: ")+Why))return;
  const int32 Paid=Cash-Ses.Budget();
  if(!Check(Paid>0&&Paid<=2*FS47FreeformWorld::ShopPrice(TEXT("cell"))+FS47Mail::Postage&&Post.FlagUp()&&Post.OrdersOut()==1,FString::Printf(TEXT("the money order ($%d) is in the box with the flag up"),Paid)))return;
  if(!Check(Post.TaskLine().Contains(TEXT("MAILBOX")),TEXT("and the list says the order waits for the carrier")))return;
  FString Said;Post.TestRound(Said);
  if(!Check(Post.TaskLine().Contains(TEXT("MAIL ORDER"))&&Post.OrdersOut()==1,TEXT("the carrier takes it: ")+Said))return;
  Waited=0;Go(21,.2);return;
 }
 if(Stage==21)
 {
  // Day by day, past noon, until the parcel comes: one to three rounds, never on a Sunday.
  if(Post.Contents().ContainsByPredicate([](const FS47Mail::FPiece& Pc){return Pc.Kind==TEXT("parcel");})){Go(30,.3);return;}
  if(++Waited>6){Finish(false,TEXT("the parcel never came"));return;}
  Ses.TestAdvance(86400.);
  const double Noon=FS47Mail::RoundClock(Ses.Seed(),Ses.Day())+60.;
  if(Ses.Clock()<Noon)Ses.TestAdvance(Noon-Ses.Clock());
  Hold=.4;return;
 }
 if(Stage==30)
 {
  if(!Check(Post.FlagUp()&&Post.TaskLine().Contains(TEXT("parcel")),TEXT("the parcel is in the box, the flag up, and on the list")))return;
  if(!Check(Post.TestLift()&&Ses.Field->ParcelStage()==2&&Ses.Field->ParcelContents().FindRef(TEXT("cell"))==2,TEXT("lifted out, the parcel is in both hands with its two dry cells")))return;
  Waited=0;Go(40,.4);return;
 }
 if(Stage==40)
 {
  // On to Thursday's paper.
  if(Post.IssuesPrinted()>0){Go(41,.3);return;}
  if(++Waited>8){Finish(false,TEXT("no Ledger came on a Thursday"));return;}
  Ses.TestAdvance(86400.);
  const double Noon=FS47Mail::RoundClock(Ses.Seed(),Ses.Day())+60.;
  if(Ses.Clock()<Noon)Ses.TestAdvance(Noon-Ses.Clock());
  Hold=.4;return;
 }
 if(Stage==41)
 {
  if(!Check(Post.Holds(TEXT("ledger:1")),TEXT("the first Ledger is in the box")))return;
  Post.TestFetch();M->UI->Close(true);
  Post.OpenLedger(1);
  if(!Check(Ses.Predecessor&&Ses.Predecessor->Heard(TEXT("wagon")),TEXT("read, the paper's search story puts wagon 23 on the map")))return;
  Go(42,.6);return;
 }
 if(Stage==42){Shot(TEXT("mail-ledger"));M->UI->Close(true);Go(50,.3);return;}
 if(Stage==50)
 {
  if(!Restore(TEXT("mail-continue")))return;
  Go(51,1.);return;
 }
 if(Stage==51)
 {
  if(!Check(Post.IssuesPrinted()>=1&&Post.Ever(TEXT("ledger:1"))&&Post.Ever(TEXT("catalog"))&&Post.RoundsMade()>0,TEXT("the papers printed and the rounds made survive a Continue")))return;
  if(!Check(Ses.Field->ParcelStage()==2,TEXT("and the parcel is still in hand")))return;
  Finish(true,TEXT("box, flag, eye-ray, order, carrier, parcel, Ledger and Continue"));return;
 }
}
