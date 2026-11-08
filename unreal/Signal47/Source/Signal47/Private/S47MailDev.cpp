#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Freeform.h"
#include "S47Mail.h"
#include "S47Main.h"

// F10, "Mail: the box at the road" (MAILBOX-20261108): the carrier now, the flag, the mail taken out, this week's paper, a parcel,
// what nobody sent, the catalog, and two places to stand: at the box, and on the truck pad, where the flag should show by day.
// The test hooks are the ones the Reyes journey uses (FS47Mail::Test*); nothing here is saved in a developer run.
void FS47DevFreeform::AddMail(FS47DevEvents& E)
{
 static const TCHAR* const G=TEXT("Mail: the box at the road");
 const auto Post=[this]()->FS47Mail*
 {
  auto* Ses=Session();if(!Ses||!Ses->Mail){Tell(false,TEXT("Freeform only"));return nullptr;}
  Ses->Mail->TestOn();return Ses->Mail.Get();
 };
 E.AddFreeform(G,TEXT("Go: the mailbox at the road"),TEXT("on the highway's shoulder in front of it, by day; E opens it"),[this,Post]
 {
  FS47Mail* Box=Post();if(!Box)return;
  FString Area;double X=0,Z=0;Box->WorldSpot(Area,X,Z);
  FStage Stage;Stage.Area=Area;Stage.From=10;Stage.To=17;Stage.Where=[X,Z]{return FVector2D(X,Z);};Stage.Yaw=-UE_DOUBLE_HALF_PI;
  Go(Stage,[this,Post]{if(FS47Mail* Again=Post())Tell(true,FString::Printf(TEXT("The mailbox, in front of you. The flag is %s. E takes the mail out."),Again->FlagUp()?TEXT("up"):TEXT("down")),6);});
 });
 E.AddFreeform(G,TEXT("Go: the truck pad, looking at the box"),TEXT("by day, flag up: it should show past the annex, at the corner of the service road"),[this,Post]
 {
  FS47Mail* Box=Post();if(!Box)return;
  Box->TestFlag(true);
  FStage Stage;Stage.Area=TEXT("saro");Stage.From=10;Stage.To=17;Stage.Where=[]{return FVector2D(9.8,10.);};Stage.Yaw=2.19;
  Go(Stage,[this]{Tell(true,TEXT("From the truck pad, the box at the corner of the service road and the highway, the flag up."),6);});
 });
 E.AddFreeform(G,TEXT("The carrier comes now"),TEXT("today's round at once: takes an order out, leaves what is due; heard from the station"),[this,Post]
 {
  FS47Mail* Box=Post();if(!Box)return;
  FString Why;Box->TestRound(Why);Tell(true,Why,6);
 });
 E.AddFreeform(G,TEXT("Flag up"),TEXT("the red flag stands up over the box"),[this,Post]{if(FS47Mail* Box=Post()){Box->TestFlag(true);Tell(true,TEXT("Flag up."),3);}});
 E.AddFreeform(G,TEXT("Flag down"),TEXT("the flag lies along the side"),[this,Post]{if(FS47Mail* Box=Post()){Box->TestFlag(false);Tell(true,TEXT("Flag down."),3);}});
 E.AddFreeform(G,TEXT("Fetch the mail"),TEXT("as E at the box: everything comes out, Ward's papers to the OPS tray, a parcel waits for both hands"),[this,Post]
 {
  if(FS47Mail* Box=Post()){if(!Box->TestFetch())Tell(true,TEXT("The box was empty."),4);}
 });
 E.AddFreeform(G,TEXT("This week's Ledger"),TEXT("the paper as it would be printed today, from this game's nights; not kept"),[Post]{if(FS47Mail* Box=Post())Box->TestLedger();});
 E.AddFreeform(G,TEXT("A parcel in the box"),TEXT("two dry cells from the catalog; lift it out, carry it to GARAGE RECEIVING"),[this,Post]{if(FS47Mail* Box=Post()){Box->TestParcel();Tell(true,TEXT("A parcel is in the box, the flag up."),4);}});
 static const TCHAR* const NoSender[]={TEXT("the blank sheet"),TEXT("the photograph"),TEXT("the postcard")};
 for(int32 I=0;I<3;++I)
 {
  const int32 Variant=I;
  E.AddFreeform(G,FString::Printf(TEXT("Something with no sender: %s"),NoSender[I]),TEXT("in the box now, no stamp, no postmark; in play twice in a month or so"),[this,Post,Variant]{if(FS47Mail* Box=Post()){Box->TestNobody(Variant);Tell(true,TEXT("Something in the box, the flag up."),4);}});
 }
 E.AddFreeform(G,TEXT("The catalog"),TEXT("Western Surplus: order by mail with a money order, the flag up; a parcel one to three rounds later"),[Post]{if(FS47Mail* Box=Post())Box->OpenCatalog();});
}
