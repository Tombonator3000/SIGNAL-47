#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Flow.h"
#include "S47MotelTv.h"
#include "S47Office.h"
#include "S47OfficeTv.h"
#include "S47Startup.h"
#include "S47TownLayer.h"
#include "S47TvProgramme.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"

// TV-CHANNEL-20261110 (Tom, point 5), -S47TvChannelTest: Dale's set has its switch (and the office saves it), every set is on the
// one channel, something on the air breaks the picture up, and at two the station signs off with the flag and the anthem, then
// its card, then snow.
void FS47FreeformTest::StepTvChannel(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>190){Finish(false,FString::Printf(TEXT("tv channel exceeded its time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI||!M->Office)return;
 auto& F=*M->Freeform;FS47OfficeTv* Tv=M->Office->Television();
 using EAir=FS47TvProgramme::EAir;
 // (watched from across the room, where the switch's prompt is out of reach and off the picture)
 const auto Face=[&]{M->Player->Place(1.35,8.05,FMath::Atan2(1.8,.3));M->Player->Pitch=-.36;};
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  if(!Check(Tv!=nullptr,TEXT("Dale's set is built in Operations")))return;
  const double Now=FMath::Fmod(F.Clock(),86400.);F.TestAdvance(FMath::Fmod(22*3600.-Now+86400.,86400.));
  Face();Go(1,1.5);return;
 }
 if(Stage==1)
 {
  const FString S=Tv->State();
  if(!Check(FS47TvProgramme::Air()==EAir::Programme&&Tv->Switched()&&(FS47TvProgramme::Staged()?S.StartsWith(TEXT("programme")):S.StartsWith(TEXT("card"))),FString::Printf(TEXT("ten in the evening: the station is on the air and the set shows it (%s)"),*S)))return;
  if(!Check(!M->Office->Save()->HasField(TEXT("tvOff")),TEXT("a set left on saves as it always did")))return;
  Shot(TEXT("tv-programme"));Go(2,.6);return;
 }
 if(Stage==2)
 {
  if(!Check(bool(M->Player->OnUse),TEXT("the use callback is ready")))return;
  M->Player->OnUse(TEXT("office:tv"));
  if(!Check(!Tv->Switched()&&M->Office->Save()->HasField(TEXT("tvOff")),TEXT("the switch: off, and the office keeps it")))return;
  Go(3,.22);return;
 }
 if(Stage==3){Shot(TEXT("tv-dot"));Go(4,1.2);return;}
 if(Stage==4)
 {
  if(!Check(Tv->State().StartsWith(TEXT("off")),FString::Printf(TEXT("switched off the set is dark with the station still on the air (%s)"),*Tv->State())))return;
  Shot(TEXT("tv-off"));Go(5,.5);return;
 }
 if(Stage==5)
 {
  M->Player->OnUse(TEXT("office:tv"));
  if(!Check(Tv->Switched()&&!M->Office->Save()->HasField(TEXT("tvOff")),TEXT("the switch again: on")))return;
  Go(6,.2);return;
 }
 if(Stage==6){Shot(TEXT("tv-line"));Go(7,1.5);return;}
 if(Stage==7)
 {
  if(!Check(!Tv->State().StartsWith(TEXT("off"))&&!Tv->State().Contains(TEXT("breaking up")),FString::Printf(TEXT("the picture is back and whole (%s)"),*Tv->State())))return;
  FS47TvProgramme::JamFor(.32,6);Go(20,2.2);return;
 }
 if(Stage==20){Shot(TEXT("tv-jam-little"));FS47TvProgramme::JamFor(.9,9);RescueClock=0;Go(8,1.);return;}
 if(Stage==8)
 {
  // (the gusts come and go: the worst of nine seconds is what counts)
  RescueClock+=Dt;
  if(!Tv->State().Contains(TEXT("breaking up"))){if(RescueClock<6)return;Check(false,TEXT("something strong on the air breaks the picture up"));return;}
  if(!Check(true,TEXT("something strong on the air breaks the picture up")))return;
  Shot(TEXT("tv-jam"));Go(9,.7);return;
 }
 if(Stage==9){Shot(TEXT("tv-jam-2"));FS47TvProgramme::JamFor(0,0);Go(10,2.5);return;}
 if(Stage==10)
 {
  if(!Check(!Tv->State().Contains(TEXT("breaking up")),TEXT("and when it has passed the picture is whole again")))return;
  const double Now=FMath::Fmod(F.Clock(),86400.);F.TestAdvance(FMath::Fmod(26*3600.-30-Now+86400.,86400.));
  Face();RescueClock=0;Go(11,.5);return;
 }
 if(Stage==11)
 {
  RescueClock+=Dt;
  if(FS47TvProgramme::Air()!=EAir::SignOff){if(RescueClock<30)return;Check(false,TEXT("at two the station signs off in front of whoever is watching"));return;}
  if(!Check(true,TEXT("at two the station signs off in front of whoever is watching")))return;
  Go(12,7.);return;
 }
 if(Stage==12)
 {
  if(!Check(Tv->State().StartsWith(TEXT("sign-off")),FString::Printf(TEXT("Dale's set has the flag and the anthem (%s)"),*Tv->State())))return;
  const FS47MotelTv* Motel=M->TownLayer?M->TownLayer->MotelTv.Get():nullptr;
  if(!Check(Motel&&Motel->bChannel&&Motel->Air==EAir::SignOff&&!Motel->bCardOnly,TEXT("and the motel office's set is on the same channel")))return;
  Shot(TEXT("tv-signoff"));RescueClock=0;Go(13,1.);return;
 }
 if(Stage==13)
 {
  RescueClock+=Dt;
  if(FS47TvProgramme::Air()==EAir::SignOff){if(RescueClock<70)return;Check(false,TEXT("the sign-off is a minute, then the card"));return;}
  if(!Check(FS47TvProgramme::Air()==EAir::Card&&RescueClock>40,FString::Printf(TEXT("the sign-off is a minute (%.0f s more), then the card"),RescueClock+8)))return;
  Go(14,1.);return;
 }
 if(Stage==14)
 {
  if(!Check(Tv->State().StartsWith(TEXT("card")),FString::Printf(TEXT("the card is on the set (%s)"),*Tv->State())))return;
  Shot(TEXT("tv-card"));
  F.TestAdvance(3700);Face();Go(15,1.5);return;
 }
 if(Stage==15)
 {
  if(!Check(FS47TvProgramme::Air()==EAir::Snow&&Tv->State().StartsWith(TEXT("snow")),FString::Printf(TEXT("an hour on there is only snow (%s)"),*Tv->State())))return;
  Shot(TEXT("tv-snow"));
  Finish(true,TEXT("Television: the switch on Dale's set is kept by the office, the sets are on one channel, the picture breaks up under what is on the air, and at two the station signs off with the flag and the anthem, then the card, then snow."));return;
 }
}
