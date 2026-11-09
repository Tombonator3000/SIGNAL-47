#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47WeekCase.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47WeekCaseDev,Log,All);

// F10, "Week case (T)" (WEEKLY-CASE-20261109, patch 1): the week's case started now at the farm, way out A carried to its morning, and
// stopped and cleaned up. Each line says what the case is in afterwards (LogS47WeekCaseDev too). Nothing here is saved; a stop gives
// the case back as it was before F10 touched it. -S47DevSweep=/tmp/dir "-S47DevSweepGroup=Week case (T)" runs the group in turn.
void FS47DevFreeform::AddWeekCase(FS47DevEvents& E)
{
 static const TCHAR* const Group=TEXT("Week case (T)");
 const auto Case=[this]()->FS47WeekCase*{auto* F=Session();return F&&F->WeekCase?F->WeekCase.Get():nullptr;};
 const auto Say=[this,Case](bool bOk,const FString& What)
 {
  FS47WeekCase* C=Case();
  const FString State=C?C->StateText():FString(TEXT("no case"));
  UE_LOG(LogS47WeekCaseDev,Display,TEXT("S47: week case F10: %s; %s"),*What,*State);
  Tell(bOk,What+TEXT(". Now: ")+State,8);
 };
 // A few metres in front of the shed's door, facing it, at 02:50 (past Story's minute, so the escalation is not held).
 const auto AtShed=[]{const FVector D=FS47WeekCase::ShedDoor();return FVector2D(D.X,D.Z+14.);};
 E.AddFreeform(Group,TEXT("Week case: start now"),TEXT("02:50 at the Alderete place, in front of the feed shed: the case on the ground at once (phase one), as on arrival"),[this,Case,Say,AtShed]
 {
  FStage Stage;Stage.From=2+50/60.;Stage.To=3.5;Stage.Area=TEXT("saro");Stage.Where=AtShed;Stage.Yaw=0;
  Go(Stage,[this,Case,Say]
  {
   FS47WeekCase* C=Case();FString Why;
   if(!C||!C->DevStart(Why)){Say(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   WaitFor([Case]{FS47WeekCase* W=Case();return W&&W->HoldsGround();},6,[Say]{Say(true,TEXT("the week's case is on the ground"));},
    [Say]{Say(false,TEXT("it did not begin at the shed (not within reach of the door?)"));});
  });
 });
 E.AddFreeform(Group,TEXT("Week case: force A → morning"),TEXT("way out A (let be), then 09:00 the next morning in the control room: Dale on the walkie and a faint line in case 8"),[this,Case,Say]
 {
  FS47WeekCase* C=Case();FString Why;
  if(!C||!C->DevForceA(Why)){Say(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
  // The set in hand, so that Dale is heard; the slip by the phone otherwise.
  if(M->FieldRadio.IsValid()&&!M->FieldRadio->Owned())M->FieldRadio->TestOwn();
  FStage Stage;Stage.From=9;Stage.To=11;Stage.Area=TEXT("saro");Stage.Where=[]{return FVector2D(0,-1.2);};Stage.Yaw=UE_DOUBLE_PI;
  Go(Stage,[this,Case,Say]
  {
   WaitFor([Case]{FS47WeekCase* W=Case();return W&&W->Phase()==FS47WeekCase::EPhase::Done;},40,[Say]{Say(true,TEXT("way out A: its morning has come"));},
    [Say]{Say(false,TEXT("its morning did not come (a panel open, or the set off Dale's channel?)"));});
  });
 });
 E.AddFreeform(Group,TEXT("Week case: stop and clean"),TEXT("over at once: the stall, the view and whatever is drawn given back; the case as it was before F10"),[Case,Say]
 {
  FS47WeekCase* C=Case();
  if(!C){Say(false,TEXT("Freeform only"));return;}
  C->DevStop();
  Say(true,TEXT("stopped and cleaned up"));
 });
}
