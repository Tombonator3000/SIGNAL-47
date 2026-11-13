#include "S47DevFreeform.h"
#include "S47DevEvents.h"
#include "S47Dossier.h"
#include "S47Freeform.h"

// F10, "Evidence dossier (M3)" (EVIDENCE-DOSSIER-20261113, slice 1): sample evidence for three files of the unknown, for a quick
// look at the stamp, the notebook's THEM page and the binder's UNKNOWNS divider without playing the nights that would fill them;
// and what is in the files now, with the code's working names (here and in the log only: the game never shows them).
void FS47DevFreeform::AddDossier(FS47DevEvents& E)
{
 static const TCHAR* const DossierGroup=TEXT("Evidence dossier (M3)");
 E.AddFreeform(DossierGroup,TEXT("Dossier: fill sample evidence (three files)"),
  TEXT("one file at MARKS (seen, on film, prints and the lamp over three nights), one at OUTLINE with two boxes, one with a single photograph; the last tag is stamped on the screen. Then TAB, Knowledge, Who & what (right-hand page), and the records binder's UNKNOWNS"),[this]
 {
  FS47FreeformSession* F=Session();
  if(!F){Tell(false,TEXT("Freeform only"));return;}
  const FString Said=F->TestFillDossier();
  UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier: %s"),*Said);
  Tell(true,Said,14);
 });
 E.AddFreeform(DossierGroup,TEXT("Dossier: what is in the files now"),TEXT("each file's number, working name (shown here only), stage, kinds of evidence and nights; also in the log"),[this]
 {
  FS47FreeformSession* F=Session();
  if(!F){Tell(false,TEXT("Freeform only"));return;}
  FString Said;
  for(const S47Dossier::EKind Kind:F->Unknowns.Order())
   Said+=FString::Printf(TEXT("UNKNOWN %d = %s, %s, %d kinds, %d nights. "),F->Unknowns.NumberOf(Kind),S47Dossier::KindId(Kind),S47Dossier::StageWord(F->Unknowns.StageOf(Kind)),F->Unknowns.TypesOf(Kind),F->Unknowns.NightsOf(Kind));
  if(Said.IsEmpty())Said=TEXT("No file yet: nothing tagged.");
  for(const FS47Dossier::FTag& One:F->Unknowns.Tags())
  {
   UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier: tag %s = %s / %s / day %d"),*One.Key,S47Dossier::KindId(One.Kind),S47Dossier::TypeId(One.Type),One.Day);
  }
  UE_LOG(LogTemp,Display,TEXT("S47: F10 dossier: %s"),*Said);
  Tell(true,Said,14);
 });
}
