#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47FieldRadio.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47FarmShedDoor.h"
#include "S47ShedGlow.h"
#include "S47WeekCase.h"
#include "S47World.h"
#include "HAL/PlatformTime.h"

DEFINE_LOG_CATEGORY_STATIC(LogS47WeekCaseDev,Log,All);

// F10, "Week case (T)" (WEEKLY-CASE-20261109, patch 1): the week's case started now at the farm, way out A carried to its morning, and
// stopped and cleaned up; patch 3: the truck dying on the yard, a turn of its key heard, and catching now; patch 2: phase one's rattling, a knock and its answer, phase two's light and kicks, the door bursting. Each line says what the case is in afterwards (LogS47WeekCaseDev too). Nothing here is saved; a stop gives
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
 // Patch 2: the shed. Each starts the case on the ground at 02:50 in front of the door first (as "start now"), then goes on.
 const auto AtTheDoor=[this,Case,Say,AtShed](TFunction<void(FS47WeekCase*)> Then)
 {
  FStage Stage;Stage.From=2+50/60.;Stage.To=3.5;Stage.Area=TEXT("saro");Stage.Where=AtShed;Stage.Yaw=0;
  Go(Stage,[this,Case,Say,Then]
  {
   FS47WeekCase* C=Case();FString Why;
   if(!C||!C->DevStart(Why)){Say(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
   WaitFor([Case]{FS47WeekCase* W=Case();return W&&W->HoldsGround()&&W->DoorNow()&&W->DoorNow()->Drawn();},8,[Case,Then]{if(FS47WeekCase* W=Case())Then(W);},
    [Say]{Say(false,TEXT("it did not begin at the shed, or its door is not drawn (the farm buildings missing?)"));});
  });
 };
 E.AddFreeform(Group,TEXT("Week case: phase 1 (rattle)"),TEXT("02:50 in front of the feed shed, phase one: uneven rattling inside, the yard's lamp blinking; E on the door tries it, then knocks"),[AtTheDoor,Say]
 {
  AtTheDoor([Say](FS47WeekCase* C){Say(C->Phase()==FS47WeekCase::EPhase::Phase1,TEXT("phase one at the shed: listen for the rattling; E tries the door, then knocks"));});
 });
 E.AddFreeform(Group,TEXT("Week case: knock and answer"),TEXT("phase one at the shed, a knock in tonight's rhythm, a long silence, and the same rhythm back from inside"),[this,AtTheDoor,Case,Say]
 {
  AtTheDoor([this,Case,Say](FS47WeekCase* C)
  {
   FString Why;
   if(!C->DevKnock(Why)){Say(false,Why);return;}
   WaitFor([Case]{FS47WeekCase* W=Case();return W&&W->DoorNow()&&W->DoorNow()->Answers()>0;},20,[Say]{Say(true,TEXT("knocked, and it answered in the same rhythm"));},
    [Say]{Say(false,TEXT("nothing answered within twenty seconds"));});
  });
 });
 E.AddFreeform(Group,TEXT("Week case: phase 2 (light and kicks)"),TEXT("phase two at the shed: the stripe under the door, then the shafts; kicks at uneven pauses, the door bulging; the lamp and lantern flickering"),[this,AtTheDoor,Case,Say]
 {
  AtTheDoor([this,Case,Say](FS47WeekCase* C)
  {
   FString Why;
   if(!C->DevPhase(FS47WeekCase::EPhase::Phase2,Why)){Say(false,Why);return;}
   WaitFor([Case]{FS47WeekCase* W=Case();return W&&W->GlowNow()&&W->GlowNow()->Lit();},6,[Say]{Say(true,TEXT("phase two: the light is coming, the kicks begin"));},
    [Say]{Say(false,TEXT("phase two began but the light did not come"));});
  });
 });
 E.AddFreeform(Group,TEXT("Week case: the door breaks"),TEXT("phase two, then the breach after a few kicks: the door bursts outward, the pieces in the yard, the padlock still locked on its chain"),[this,AtTheDoor,Case,Say]
 {
  AtTheDoor([this,Case,Say](FS47WeekCase* C)
  {
   FString Why;
   if(!C->DevPhase(FS47WeekCase::EPhase::Phase2,Why)){Say(false,Why);return;}
   After(12,[this,Case,Say]
   {
    FS47WeekCase* W=Case();FString Why2;
    if(!W||!W->DevPhase(FS47WeekCase::EPhase::Phase3,Why2)){Say(false,Why2.IsEmpty()?FString(TEXT("the case is gone")):Why2);return;}
    WaitFor([Case]{FS47WeekCase* X=Case();return X&&X->DoorNow()&&X->DoorNow()->State()==FS47FarmShedDoor::EState::Broken;},4,[Say]{Say(true,TEXT("the door broke outward"));},
     [Say]{Say(false,TEXT("the breach did not break the door"));});
   });
  });
 });
 // Patch 3: the truck. These need the player in the truck at the Alderete place (drive there: the stage cannot seat her in it).
 const auto InTruck=[this,Case,Say](TFunction<void(FS47WeekCase*)> Then)
 {
  FS47WeekCase* C=Case();
  if(!C||!M->World){Say(false,TEXT("Freeform only"));return;}
  if(!M->World->bDriving){Say(false,TEXT("get in the truck and drive it to the Alderete yard first"));return;}
  if(C->HoldsGround()){Then(C);return;}
  FString Why;
  if(!C->DevStart(Why)){Say(false,Why.IsEmpty()?FString(TEXT("Freeform only")):Why);return;}
  WaitFor([Case]{FS47WeekCase* W=Case();return W&&W->HoldsGround();},6,[Case,Then]{if(FS47WeekCase* W=Case())Then(W);},
   [Say]{Say(false,TEXT("the case did not begin: the truck is not at the farm (within reach of the feed shed)"));});
 };
 E.AddFreeform(Group,TEXT("Week case: truck dies on the yard"),TEXT("in the truck at the Alderete place: the case on the ground and the engine dead, wanting the key; it will not catch until it is far off and you have been quiet 20 s, or after five minutes"),[InTruck,Say]
 {
  InTruck([Say](FS47WeekCase* C){FString Why;const bool bOk=C->DevStall(Why);Say(bOk,bOk?FString(TEXT("the engine died on the yard: E turns the key. ")+C->CabText()):Why);});
 });
 E.AddFreeform(Group,TEXT("Week case: start attempt heard"),TEXT("the dead truck: a turn of the key, the starter cranking and not catching; heard at the truck (a noise for whatever listens, where the searcher goes first)"),[this,InTruck,Case,Say]
 {
  InTruck([this,Case,Say](FS47WeekCase* C)
  {
   FString Why;const double Was=C->LastSoundTime();
   if(!C->CabNow().bStalled)C->DevStall(Why);
   if(!C->DevTurnKey(Why)){Say(false,Why);return;}
   WaitFor([Case,Was]{FS47WeekCase* W=Case();return W&&W->LastSoundTime()>Was;},4,[Case,Say]{FS47WeekCase* W=Case();Say(true,TEXT("the starter was heard at the truck. ")+(W?W->CabText():FString()));},
    [Say]{Say(false,TEXT("the turn was not heard (the case let go of the truck?)"));});
  });
 });
 E.AddFreeform(Group,TEXT("Week case: truck catches now"),TEXT("the dead truck catches on this turn of the key, whatever the rules say (not while it is at the cab)"),[this,InTruck,Case,Say]
 {
  InTruck([this,Case,Say](FS47WeekCase* C)
  {
   FString Why;
   if(!C->DevCatch(Why)){Say(false,Why);return;}
   WaitFor([this]{return M->World&&M->World->Trouble.Kind()==ES47Trouble::None;},6,[Case,Say]{FS47WeekCase* W=Case();Say(true,TEXT("the engine caught. ")+(W?W->CabText():FString()));},
    [Say]{Say(false,TEXT("it cranked but did not catch"));});
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
