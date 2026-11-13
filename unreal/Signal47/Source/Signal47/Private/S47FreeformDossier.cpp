#include "S47Freeform.h"
#include "S47Dossier.h"
#include "S47FreeformDirector.h"
#include "S47Main.h"
#include "S47RecordsBinder.h"
#include "S47RoadGlimpse.h"
#include "S47UI.h"

// EVIDENCE-DOSSIER-20261113 (-> M3, M4; Tom 13 November), slice 1: the session's part of the files of the unknown. What is in a
// file is S47Dossier (pure); this puts a tag on a find where its source already knows which one it was, stamps it on the screen
// where it was logged (no trip back to a PC), saves it, rebuilds it for a save from before, and hands the files to the binder.
// The sources: a photograph (FS47FreeformDirector::Photo and ClaimEnlargement), the prints on the shoulder (FS47Happenings),
// the two rules and the visit's field page (FS47FarmSite).

bool FS47FreeformSession::TagEvidence(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Night)
{
 const bool bNewFile=Unknowns.NumberOf(Kind)==0;
 const S47Dossier::EStage Before=Unknowns.StageOf(Kind);
 if(!Unknowns.Tag(Key,Kind,Type,Night>0?Night:DayNumber))return false;
 const S47Dossier::EStage After=Unknowns.StageOf(Kind);
 // The stamp: the file's number and the kind of evidence. Never which one it is, and never what was found.
 const FString Said=S47Dossier::Stamp(Unknowns.NumberOf(Kind),Type,S47Dossier::PhotoKey(Key),bNewFile,After!=Before?After:S47Dossier::EStage::NoFile);
 UE_LOG(LogTemp,Display,TEXT("S47: dossier: %s is %s in file %d (%s): %s"),*Key,S47Dossier::TypeId(Type),Unknowns.NumberOf(Kind),S47Dossier::KindId(Kind),*Said);
 if(M&&M->UI)M->UI->Toast(Said,5.f);
 return true;
}
bool FS47FreeformSession::TagPhotoClaim(const FString& ClaimId)
{
 S47Dossier::EKind Kind=S47Dossier::EKind::None;int32 Night=0;
 return S47Dossier::ClaimKind(ClaimId,Kind,Night)&&TagEvidence(ClaimId,Kind,S47Dossier::EType::Shape,Night);
}
void FS47FreeformSession::LoadDossier(const S47J::FObj& Saved)
{
 Unknowns.Reset();
 if(!Saved.IsValid())return;
 if(const S47J::FObj Record=S47J::Obj(Saved,TEXT("dossier"));Record.IsValid()){Unknowns.Load(Record);return;}
 // A save from before the dossier: what it kept says which one it was for some of it (FS47Dossier::Rebuild). The same every time.
 TArray<FS47Dossier::FOld> Old;
 for(const TPair<FString,FString>& Finding:Findings)Old.Add({Finding.Key,Finding.Value,FindingDays.FindRef(Finding.Key)});
 TArray<FString> ClaimIds;
 if(Director)for(const FS47FreeformDirector::FPhotoClaim& Claim:Director->PhotoClaims())ClaimIds.Add(Claim.Id);
 const int32 Made=Unknowns.Rebuild(Old,ClaimIds);
 if(Made>0){UE_LOG(LogTemp,Display,TEXT("S47: dossier: rebuilt from an older save, %d tags in %d files"),Made,Unknowns.Order().Num());}
}
void FS47FreeformSession::SaveDossier(const S47J::FObj& Record) const
{
 // Nothing tagged writes nothing: an older save reads the same, and is rebuilt the same the next time.
 if(!Unknowns.IsEmpty())S47J::Set(Record,TEXT("dossier"),Unknowns.Save());
}
void FS47FreeformSession::BinderUnknowns(S47Binder::FSource& Src) const
{
 for(const S47Dossier::FFile& File:Unknowns.Files())
 {
  // The photographs in the file, by the print each claim stands on now (the album's own id, as the OPS receipt names it).
  TArray<FString> PhotoLines;
  for(const FString& ClaimId:File.Photos)
  {
   S47Dossier::EKind Kind=S47Dossier::EKind::None;int32 Night=0;S47Dossier::ClaimKind(ClaimId,Kind,Night);
   const FS47FreeformDirector::FPhotoClaim* Claim=Director?Director->PhotoClaims().FindByPredicate([&ClaimId](const FS47FreeformDirector::FPhotoClaim& One){return One.Id==ClaimId;}):nullptr;
   PhotoLines.Add(Claim&&!Claim->PhotoId.IsEmpty()
    ?FString::Printf(TEXT("DAY %02d / ORIGINAL %s / %s"),Night,*Claim->PhotoId,Claim->bPaid?TEXT("REGISTERED AT OPS"):TEXT("NOT YET REGISTERED AT OPS"))
    :FString::Printf(TEXT("DAY %02d / THE FRAME IS NOT IN THE ALBUM"),Night));
  }
  S47Binder::FUnknownIn Sheet;Sheet.Number=File.Number;Sheet.Title=File.Title;Sheet.Stage=File.Stage;Sheet.Opened=File.Opened;Sheet.Latest=File.Latest;Sheet.bAtMarks=File.bAtMarks;
  Sheet.Body=S47Dossier::Sheet(File,PhotoLines);
  Src.Unknowns.Add(MoveTemp(Sheet));
 }
}
FString FS47FreeformSession::TestFillDossier()
{
 // F10: three files as a few nights could leave them. The first has a shape seen and on film, prints and the lamp over three
 // nights (MARKS), the second a shape and the lamp (OUTLINE), the next one photograph. The last tag goes the game's own way, so
 // its stamp is on the screen.
 const int32 First=FMath::Max(1,DayNumber-2);
 const S47Dossier::EKind Low=S47Dossier::EKind::Low,Small=S47Dossier::EKind::Small,Silver=S47Dossier::EKind::Silver;
 const S47Dossier::EType Shape=S47Dossier::EType::Shape,Ground=S47Dossier::EType::Ground,Lamp=S47Dossier::EType::Lamp;
 Unknowns.Tag(TEXT("dev-dossier_visit"),Low,Shape,First);
 Unknowns.Tag(FString::Printf(TEXT("strange.crawler.n%d.dev"),First+1),Low,Shape,First+1);
 Unknowns.Tag(TEXT("dev-dossier-road_tracks"),Low,Ground,First+1);
 Unknowns.Tag(TEXT("dev-dossier-small_visit"),Small,Shape,First);
 Unknowns.Tag(TEXT("dev-dossier-rule-still-held"),Small,Lamp,First+2);
 Unknowns.Tag(FString::Printf(TEXT("strange.nordic.n%d.dev"),First+2),Silver,Shape,First+2);
 const bool bStamped=TagEvidence(TEXT("dev-dossier-rule-pale-broken"),Low,Lamp,First+2);
 FString Said;
 for(const S47Dossier::FFile& File:Unknowns.Files())Said+=FString::Printf(TEXT("%s %s, %d %s. "),*File.Title,*File.Stage,File.Nights,File.Nights==1?TEXT("night"):TEXT("nights"));
 Said+=bStamped?TEXT("The last one was stamped on the screen. "):TEXT("Filled before: nothing new. ");
 return Said+TEXT("Look: TAB, Knowledge, Who & what, the right-hand page; and the records binder, UNKNOWNS.");
}
bool FS47FreeformSession::DossierSelfTest(TArray<FString>& Lines)
{
 bool bOk=true;
 const auto Check=[&bOk,&Lines](bool bGood,const TCHAR* Name){bOk&=bGood;Lines.Add(FString(bGood?TEXT("PASS "):TEXT("FAIL "))+TEXT("EVIDENCE-DOSSIER: ")+Name);};
 const S47Dossier::EKind Small=S47Dossier::EKind::Small,Low=S47Dossier::EKind::Low,Tall=S47Dossier::EKind::Tall,Silver=S47Dossier::EKind::Silver;
 const S47Dossier::EType Shape=S47Dossier::EType::Shape,Ground=S47Dossier::EType::Ground,Lamp=S47Dossier::EType::Lamp;
 FS47FreeformSession Old(nullptr);Old.Begin(nullptr);
 Check(Old.Unknowns.IsEmpty()&&!Old.Save()->HasField(TEXT("dossier"))&&Old.BinderSource().Unknowns.IsEmpty(),TEXT("a new game has no file of the unknown, and its record carries no dossier"));
 // What a game from before the dossier kept: the findings, and no tag on any of them.
 Old.RecordEvidence(TEXT("rule-still-held"),TEXT("A small one came near the place I stood. I had the lamp off and I kept still, and it looked past me."));
 Old.RecordEvidence(TEXT("road_tracks_d1"),S47RoadGlimpse::TracksRecord(true,TEXT("09:10"),true));
 Old.RecordEvidence(TEXT("rule-pale-broken"),TEXT("I did what had worked: lamp off, standing still. It did not pass."));
 const bool bUntagged=Old.Unknowns.IsEmpty();
 const S47J::FObj Before=S47J::Read(S47J::Write(Old.Save()));Old.Stop();
 // And a photograph claim as the Director saved it then, titled by what it was.
 if(const S47J::FObj DirectorWas=S47J::Obj(Before,TEXT("director"));DirectorWas.IsValid())
 {
  const S47J::FObj Claim=S47J::New();
  S47J::Set(Claim,TEXT("id"),TEXT("strange.crawler.n1"));S47J::Set(Claim,TEXT("photo"),TEXT("album3"));S47J::Set(Claim,TEXT("title"),TEXT("low crawler"));S47J::Set(Claim,TEXT("quality"),.2);
  TArray<TSharedPtr<FJsonValue>> ClaimsWas;ClaimsWas.Add(MakeShared<FJsonValueObject>(Claim));
  DirectorWas->SetArrayField(TEXT("photoClaims"),ClaimsWas);
 }
 FS47FreeformSession Fresh(nullptr);Fresh.Begin(Before);
 {
  const FS47Dossier& D=Fresh.Unknowns;
  const bool bTitle=Fresh.Director&&Fresh.Director->PhotoClaims().Num()==1&&Fresh.Director->PhotoClaims()[0].Title==TEXT("low shape on four legs");
  Check(bUntagged&&!Before->HasField(TEXT("dossier"))&&D.Tags().Num()==3&&D.Has(Small,Lamp)&&D.Has(Low,Ground)&&D.Has(Low,Shape)&&!D.HasTag(TEXT("rule-pale-broken"))&&D.NumberOf(Low)==1&&D.NumberOf(Small)==2&&bTitle,
   TEXT("an older save's game is rebuilt on Continue from its photograph claim, the rule learned and the prints; the claim's old title no longer says what it was"));
 }
 // Tagged at the source from here on: once a find, and never the shade or the sky.
 const bool bTagged=Fresh.TagEvidence(TEXT("farm_visit"),S47Dossier::KindOfVisit(2),Shape)&&!Fresh.TagEvidence(TEXT("farm_visit"),S47Dossier::KindOfVisit(0),Shape)
  &&Fresh.TagPhotoClaim(TEXT("strange.nordic.n4"))&&!Fresh.TagPhotoClaim(TEXT("strange.nordic.n4"))&&!Fresh.TagPhotoClaim(TEXT("strange.shade.n4"))&&!Fresh.TagPhotoClaim(TEXT("strange.sky.n4"))
  &&!Fresh.TagEvidence(TEXT("mesa_visit"),S47Dossier::KindOfVisit(7),Shape);
 const S47J::FObj After=S47J::Read(S47J::Write(Fresh.Save()));
 const TArray<S47Binder::FUnknownIn> Sheets=Fresh.BinderSource().Unknowns;
 Fresh.Stop();
 FS47FreeformSession Again(nullptr);Again.Begin(After);
 {
  const FS47Dossier& D=Again.Unknowns;
  const FS47Dossier::FTag* Film=D.Tags().FindByPredicate([](const FS47Dossier::FTag& One){return One.Key==TEXT("strange.nordic.n4");});
  Check(bTagged&&After->HasField(TEXT("dossier"))&&D.Tags().Num()==5&&D.NumberOf(Low)==1&&D.NumberOf(Small)==2&&D.NumberOf(Tall)==3&&D.NumberOf(Silver)==4&&Film&&Film->Day==4&&Film->Type==Shape&&D.Has(Tall,Shape)
   &&!D.Has(Tall,Lamp)&&D.StageOf(Low)==S47Dossier::EStage::Outline,
   TEXT("a find is tagged once where it is logged, the shade and the sky never; the files and their numbers survive a Continue"));
  Check(Sheets.Num()==4&&Sheets[0].Title==TEXT("UNKNOWN 1")&&Sheets[0].Stage==TEXT("OUTLINE")&&Sheets[0].Body.Contains(TEXT("ORIGINAL album3 / NOT YET REGISTERED AT OPS"))&&Sheets[3].Body.Contains(TEXT("THE FRAME IS NOT IN THE ALBUM"))
   &&Sheets[1].Body.Contains(TEXT("NONE IN THE ALBUM.")),
   TEXT("the binder is handed the same files, each photograph by the print its claim stands on"));
 }
 Again.Stop();
 return bOk;
}
