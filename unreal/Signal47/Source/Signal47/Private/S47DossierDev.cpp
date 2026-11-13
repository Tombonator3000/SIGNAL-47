#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Dossier.h"
#include "S47Freeform.h"
#include "S47Main.h"
#include "S47Rhythm.h"
#include "S47RuleBreak.h"
#include "S47Thoughts.h"

// F10, "Evidence dossier (M3)" (EVIDENCE-DOSSIER-20261113): sample evidence for the files of the unknown, for a quick look at the stamp,
// the notebook's THEM page and the binder's UNKNOWNS divider without playing the nights that would fill them; what is in the files now,
// with the code's working names (here and in the log only: the game never shows them); and (slice 2) the pacing from the gates.
void FS47DevFreeform::AddDossier(FS47DevEvents& E)
{
 static const TCHAR* const DossierGroup=TEXT("Evidence dossier (M3)");
 E.AddFreeform(DossierGroup,TEXT("Dossier: fill sample evidence, every channel (slice 2)"),
  TEXT("three nights of the small ones at a site in all six boxes; a tape of the low one never seen and its crossing on the road, two files a visit then ties side by side; the tall one's tape at the small ones' site, which strikes their tape's pencil; the silver one's photograph, take and lamp. The last tag is stamped on the screen. Then TAB, Knowledge, Who & what (right-hand page), and the records binder's UNKNOWNS"),[this]
 {
  FS47FreeformSession* F=Session();
  if(!F){Tell(false,TEXT("Freeform only"));return;}
  const FString Said=F->TestFillDossier();
  UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier: %s"),*Said);
  Tell(true,Said,16);
 });
 E.AddFreeform(DossierGroup,TEXT("Dossier: what is in the files now"),TEXT("each file's number, the working name of what opened it (shown here only), stage, kinds of evidence, nights, the files kept beside it; every tag in the log"),[this]
 {
  FS47FreeformSession* F=Session();
  if(!F){Tell(false,TEXT("Freeform only"));return;}
  FString Said;
  for(const S47Dossier::FFile& File:F->Unknowns.Files())
  {
   int32 Kinds=0;for(const S47Dossier::FBox& Slot:File.Boxes)Kinds+=Slot.bFilled?1:0;
   Said+=FString::Printf(TEXT("%s = %s, %s, %d kinds, %d nights"),*File.Named,S47Dossier::KindId(F->Unknowns.Order()[File.Number-1]),*File.Stage,Kinds,File.Nights);
   for(const int32 Other:File.Beside)Said+=FString::Printf(TEXT(", beside %d"),Other);
   // slice 3: FILED, what is filed in it as one, and whether the game knows it holds more than one kind
   if(File.bFiled)Said+=TEXT(", FILED");
   for(const int32 InIt:File.Merged)Said+=FString::Printf(TEXT(", %d filed in it"),InIt);
   if(F->Unknowns.WrongMerge(File.Number))Said+=TEXT(" (a wrong one: not one kind)");
   Said+=TEXT(". ");
  }
  if(Said.IsEmpty())Said=TEXT("No file yet: nothing tagged.");
  else Said+=FString::Printf(TEXT("HABITS from night %d. Dr. Ward's count: %d of the near three filed, %d points. "),F->Unknowns.HabitsFromNight(),F->Unknowns.KindsFiled(),S47Dossier::WardPoints(F->Unknowns.KindsFiled()));
  for(const FS47Dossier::FTag& One:F->Unknowns.Tags())
  {
   UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier: tag %s = %s / %s / day %d / file %d%s"),*One.Key,S47Dossier::KindId(One.Kind),S47Dossier::TypeId(One.Type),One.Day,One.File+1,One.bWhere?*FString::Printf(TEXT(" / at %.0f, %.0f"),One.X,One.Z):TEXT(""));
  }
  UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier: %s"),*Said);
  Tell(true,Said,16);
 });
 // EVIDENCE-DOSSIER slice 3: names, filing as one, Dr. Ward's count and the habits as thoughts, without walking to the binder
 E.AddFreeform(DossierGroup,TEXT("Dossier: name every file by its suggestion (slice 3)"),TEXT("as if the suggestion each file's name field is filled with were written at the records-room binder: the names on THEM, the binder, the map and tonight's note; a file at HABITS is FILED and paid once; Dr. Ward's count after"),[this]
 {
  FS47FreeformSession* F=Session();
  if(!F){Tell(false,TEXT("Freeform only"));return;}
  const FString Said=F->TestNameDossier();
  UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier names: %s"),*Said);
  Tell(true,Said,16);
 });
 E.AddFreeform(DossierGroup,TEXT("Dossier: file the first pair as one, or take it out again (slice 3)"),TEXT("the first two kept side by side (else the first two files) filed as one, as at the binder; again tonight takes it out. Says whether the game knows they are one kind (shown here only)"),[this]
 {
  FS47FreeformSession* F=Session();
  if(!F){Tell(false,TEXT("Freeform only"));return;}
  const FString Said=F->TestMergeDossier();
  UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier as one: %s"),*Said);
  Tell(true,Said,16);
 });
 E.AddFreeform(DossierGroup,TEXT("Dossier: bring a file's habit to mind now (slice 3)"),TEXT("the first file at HABITS: its hour thought, or its lamp habit, sent now as the player's own (the once-a-night mark for it cleared; the thoughts' cooldown still holds)"),[this]
 {
  FS47FreeformSession* F=Session();
  if(!F||!M){Tell(false,TEXT("Freeform only"));return;}
  FString Said;
  for(const S47Dossier::FFile& File:F->Unknowns.Files())
  {
   if(F->Unknowns.StageOfFile(File.Number)!=S47Dossier::EStage::Habits)continue;
   const FString Lamp=File.Boxes.IsValidIndex(int32(S47Dossier::EType::Lamp))?S47Dossier::HabitLampThought(File.Boxes[int32(S47Dossier::EType::Lamp)].Sign):FString();
   const FString Hour=File.Hours.IsEmpty()?FString():S47Dossier::HabitHourThought(File.Hours[0]);
   F->HabitThought.Remove(File.Number);
   const bool bSent=S47Thoughts::ThinkLine(M,Hour.IsEmpty()?Lamp:Hour,TEXT("habit-dev"));
   Said=FString::Printf(TEXT("%s: hour \"%s\", lamp \"%s\"%s"),*File.Named,*Hour,*Lamp,bSent?TEXT(", one sent now"):TEXT(", none sent now (the cooldown)"));
   break;
  }
  if(Said.IsEmpty())Said=TEXT("No file at HABITS yet: nothing comes to mind.");
  UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier habit: %s"),*Said);
  Tell(true,Said,12);
 });
 E.AddFreeform(DossierGroup,TEXT("Dossier: the pacing from the gates (slice 2)"),TEXT("from director.json as the game has it now: the first night each of the five can be at MARKS and at HABITS in the best case (0: never), and the night HABITS waits for"),[this]
 {
  FS47FreeformSession* F=Session();
  if(!F||!M||!M->Rhythm){Tell(false,TEXT("Freeform only"));return;}
  const FS47RhythmData& R=M->Rhythm->Data();
  const auto From=[&R](const TCHAR* Id,int32 Else){const FS47RhythmEvent* Ev=R.Find(FName(Id));return Ev?Ev->FromDay:Else;};
  S47Dossier::FGates G;
  G.Visit=FMath::Min(From(TEXT("site_visit"),2),From(TEXT("field_visit"),2));G.Glimpse=From(TEXT("road_glimpse"),3);G.Sky=From(TEXT("horizon_figure"),2);
  G.Figure=From(TEXT("figure_near"),10);G.RuleBreak=FS47RuleBreak::FromNight;G.HabitsFrom=FS47FreeformSession::DossierHabitsFrom(R);G.Take=1;
  FString Said=FString::Printf(TEXT("HABITS from night %d. "),G.HabitsFrom);
  for(int32 K=0;K<int32(S47Dossier::EKind::Count);++K)
   Said+=FString::Printf(TEXT("%s: MARKS %d, HABITS %d. "),S47Dossier::KindId(S47Dossier::EKind(K)),S47Dossier::Earliest(S47Dossier::EKind(K),S47Dossier::EStage::Marks,G),S47Dossier::Earliest(S47Dossier::EKind(K),S47Dossier::EStage::Habits,G));
  UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier pacing: %s"),*Said);
  Tell(true,Said,16);
 });
}
