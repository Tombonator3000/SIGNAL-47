#include "S47FreeformTest.h"
#include "S47Main.h"
#include "S47Freeform.h"
#include "S47Flow.h"
#include "S47DevEvents.h"
#include "S47Startup.h"
#include "S47TvProgramme.h"
#include "S47UI.h"
#include "S47Web.h"
#include "S47Walker.h"
#include "SS47Dev.h"

// FIX10-6 (Tom, point 6), -S47DevMenuTest: F10 is two panes with the groups under headings, typing searches every line and shows
// the search, Enter starts the line found, and the screen opens again where it was left with what was used on top.
void FS47FreeformTest::StepDevMenu(double Dt)
{
 Age+=FMath::Min(Dt,.2);if(Hold>0){Hold-=Dt;return;}
 if(Age>60){Finish(false,FString::Printf(TEXT("dev menu exceeded its time bound at stage %d"),Stage));return;}
 if(!M||!M->Flow||M->Flow->bStarting||S47Startup::Blocking()||!M->Freeform||!M->Freeform->Active()||!M->Web||!M->Web->IsLoaded()||!M->Player||!M->UI)return;
 const TSharedPtr<SS47Dev> Menu=M->DevEvents?M->DevEvents->Menu():nullptr;
 const FString S=Menu.IsValid()?Menu->State():FString(TEXT("closed"));
 if(Stage==0)
 {
  if(Age<3)return;M->UI->Close(true);
  if(!Check(M->DevEvents!=nullptr,TEXT("the night has its developer events")))return;
  M->DevEvents->Open();Go(1,.6);return;
 }
 if(Stage==1)
 {
  int32 HowMany=0;{const int32 Cut=S.Find(TEXT("; "));if(Cut>0)HowMany=FCString::Atoi(*S.Mid(Cut+2));}
  if(!Check(Menu.IsValid()&&S.StartsWith(TEXT("pane 0"))&&HowMany>=40,FString::Printf(TEXT("F10 opens on the groups (%s)"),*S)))return;
  Shot(TEXT("menu-open"));Go(2,.6);return;
 }
 if(Stage==2)
 {
  if(!Check(Menu.IsValid(),TEXT("the screen is still open")))return;
  Menu->TestType(TEXT("TV signs off now"));
  const FString T=Menu->State();
  if(!Check(T.StartsWith(TEXT("pane 1"))&&T.Contains(TEXT("group 'Every match'"))&&T.Contains(TEXT("line 'TV: the station signs off now'"))&&T.Contains(TEXT("search 'TV signs off now'")),FString::Printf(TEXT("typing searches every line, in any case, and puts the cursor on the first found (%s)"),*T)))return;
  Go(3,.5);return;
 }
 if(Stage==3){Shot(TEXT("menu-search"));Go(4,.6);return;}
 if(Stage==4)
 {
  if(!Check(Menu.IsValid(),TEXT("the search is still up")))return;
  Menu->TestKey(EKeys::Enter);
  Go(5,1.);return;
 }
 if(Stage==5)
 {
  if(!Check(!Menu.IsValid()||!M->UI->Modal(),TEXT("Enter closes the screen")))return;
  if(!Check(FS47TvProgramme::Air()==FS47TvProgramme::EAir::SignOff,TEXT("and the line found was started: the station signs off")))return;
  M->DevEvents->Open();Go(6,.6);return;
 }
 if(Stage==6)
 {
  if(!Check(Menu.IsValid()&&S.StartsWith(TEXT("pane 0"))&&S.Contains(TEXT("first 'Used last'"))&&S.Contains(TEXT("group 'Television (T)'")),FString::Printf(TEXT("opened again it is where it was left, with what was used on top (%s)"),*S)))return;
  Menu->TestKey(EKeys::Right);
  const FString T=Menu->State();
  if(!Check(T.StartsWith(TEXT("pane 1"))&&T.Contains(TEXT("line 'TV:")),FString::Printf(TEXT("Right goes to the group's lines (%s)"),*T)))return;
  Go(7,.5);return;
 }
 if(Stage==7){Shot(TEXT("menu-lines"));Go(8,.6);return;}
 if(Stage==8)
 {
  if(!Check(Menu.IsValid(),TEXT("the lines are still up")))return;
  Menu->TestKey(EKeys::Escape);Menu->TestKey(EKeys::Home);
  const FString T=Menu->State();
  if(!Check(T.StartsWith(TEXT("pane 0"))&&T.Contains(TEXT("group 'Used last'; 1 lines")),FString::Printf(TEXT("Escape goes back to the groups, and Home to what was used last (%s)"),*T)))return;
  Menu->TestType(TEXT("zzqx"));
  if(!Check(Menu->State().Contains(TEXT("; 0 groups;")),TEXT("a search that finds nothing says so and starts nothing")))return;
  Go(9,.5);return;
 }
 if(Stage==9){Shot(TEXT("menu-nothing"));Go(10,.5);return;}
 if(Stage==10)
 {
  if(!Check(Menu.IsValid(),TEXT("the empty search is still up")))return;
  Menu->TestKey(EKeys::Enter);Menu->TestKey(EKeys::Escape);
  if(!Check(Menu->State().StartsWith(TEXT("pane 0"))&&Menu->State().Contains(TEXT("search ''")),TEXT("Escape clears the search first")))return;
  Menu->TestKey(EKeys::Escape);Go(11,.5);return;
 }
 if(Stage==11)
 {
  if(!Check(!M->UI->Modal(),TEXT("and then leaves, back to the night")))return;
  Finish(true,TEXT("F10: groups under headings on the left and their lines on the right, a search that is shown and finds any words, Enter starts the line found, and the screen opens again where it was left."));
 }
}
