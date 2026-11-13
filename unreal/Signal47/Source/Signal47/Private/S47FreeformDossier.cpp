#include "S47Freeform.h"
#include "S47Dossier.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47Main.h"
#include "S47Marginalia.h"
#include "S47PredecessorText.h"
#include "S47RecordsBinder.h"
#include "S47RoadGlimpse.h"
#include "S47Script.h"
#include "S47Story.h"
#include "S47UI.h"
#include "SS47FieldMap.h"

// EVIDENCE-DOSSIER-20261113 (-> M3, M4; Tom 13 November), slice 1: the session's part of the files of the unknown. What is in a
// file is S47Dossier (pure); this puts a tag on a find where its source already knows which one it was, stamps it on the screen
// where it was logged (no trip back to a PC), saves it, rebuilds it for a save from before, and hands the files to the binder.
// The sources: a photograph (FS47FreeformDirector::Photo and ClaimEnlargement), the prints on the shoulder (FS47Happenings),
// the two rules and the visit's field page (FS47FarmSite).

bool FS47FreeformSession::TagEvidence(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Night,bool bHere)
{
 const bool bNewFile=Unknowns.NumberOf(Kind)==0;
 const S47Dossier::EStage Before=Unknowns.StageOf(Kind);
 // LOOP-DOSSIER-GIVES-20261113: the hour and the place it was got, when it was got where the player stands (S47FieldMap::Me: the
 // sheet's own road-local frame; nothing off the sheet). An enlargement at the bench keeps neither.
 int32 Hour=-1;FVector2D Where(0,0);bool bWhere=false;
 if(bHere)
 {
  if(Clock()>=0)Hour=FMath::Clamp(int32(FMath::FloorToDouble(FMath::Fmod(Clock(),double(SecondsInDay))/3600.)),0,23);
  double MeX=0,MeZ=0,MeHeading=0;FString MeArea;bool bMeDriving=false;
  if(M&&S47FieldMap::Me(M,MeX,MeZ,MeHeading,MeArea,bMeDriving)){Where=FVector2D(MeX,MeZ);bWhere=true;}
 }
 if(!Unknowns.Tag(Key,Kind,Type,Night>0?Night:DayNumber,Hour,bWhere?&Where:nullptr))return false;
 const S47Dossier::EStage After=Unknowns.StageOf(Kind);
 // MARKS reached: one more bearing than the night allows, for this night, or from five in the morning the coming one (FS47Places).
 if(int32(Before)<int32(S47Dossier::EStage::Marks)&&int32(After)>=int32(S47Dossier::EStage::Marks)&&Unknowns.Grant(Kind,S47Dossier::ExtraNightFor(Clock())))
 {
  UE_LOG(LogTemp,Display,TEXT("S47: dossier: file %d at MARKS: one more bearing from night %d"),Unknowns.NumberOf(Kind),S47Dossier::ExtraNightFor(Clock()));
 }
 // The stamp: the file's number and the kind of evidence. Never which one it is, and never what was found.
 const FString Said=S47Dossier::Stamp(Unknowns.NumberOf(Kind),Type,S47Dossier::PhotoKey(Key),bNewFile,After!=Before?After:S47Dossier::EStage::NoFile);
 UE_LOG(LogTemp,Display,TEXT("S47: dossier: %s is %s in file %d (%s): %s"),*Key,S47Dossier::TypeId(Type),Unknowns.NumberOf(Kind),S47Dossier::KindId(Kind),*Said);
 if(M&&M->UI)M->UI->Toast(Said,5.f);
 return true;
}
bool FS47FreeformSession::TagPhotoClaim(const FString& ClaimId,bool bHere)
{
 S47Dossier::EKind Kind=S47Dossier::EKind::None;int32 Night=0;
 return S47Dossier::ClaimKind(ClaimId,Kind,Night)&&TagEvidence(ClaimId,Kind,S47Dossier::EType::Shape,Night,bHere);
}
// LOOP-DOSSIER-GIVES-20261113 (-> M3, M4): what each stage gives, from the files as they stand (nothing of its own is saved here).
void FS47FreeformSession::DossierMap(FS47MapView& View) const
{
 // OUTLINE: where each file was first logged out on the land, in pencil, its title beside it
 for(const S47Dossier::EKind Kind:Unknowns.Order())
 {
  double X=0,Z=0;
  if(!Unknowns.FirstWhere(Kind,X,Z))continue;
  S47FieldMap::FFirstLogged Logged;Logged.X=X;Logged.Z=Z;Logged.Label=FString::Printf(TEXT("UNKNOWN %d"),Unknowns.NumberOf(Kind));
  S47FieldMap::AddFirstLogged(View,Logged);
 }
}
bool FS47FreeformSession::DossierExtraOpen(int32 Night) const
{
 // MARKS: never on the big night (it owns the night: FS47Happenings::BigOwns), and only where the live rules are
 return RulesLive()&&Unknowns.ExtraOpen(Night)&&!(Happenings&&Happenings->BigOwns(Night));
}
bool FS47FreeformSession::DossierSpendExtra(int32 Night)
{
 if(!DossierExtraOpen(Night)||!Unknowns.Spend(Night))return false;
 UE_LOG(LogTemp,Display,TEXT("S47: dossier: the extra bearing of MARKS printed, night %d"),Night);
 return true;
}
void FS47FreeformSession::DossierTasks(TArray<FS47Task>& Out) const
{
 // MARKS: while the extra bearing can still come tonight. HABITS: the hours each file there has been logged at, one line each.
 // In the fiction's words only; never which file gave it, never what it is (S47Dossier::Lint has every one of them).
 if(DossierExtraOpen(NightOf(Clock())))Out.Add({S47Dossier::ExtraTaskLine(),false});
 for(const S47Dossier::EKind Kind:Unknowns.Order())
 {
  if(Unknowns.StageOf(Kind)!=S47Dossier::EStage::Habits)continue;
  const FString Line=S47Dossier::HoursLine(Unknowns.NumberOf(Kind),Unknowns.HoursOf(Kind));
  if(!Line.IsEmpty())Out.Add({Line,false});
 }
}
void FS47FreeformSession::LoadDossier(const S47J::FObj& Saved)
{
 Unknowns.Reset();
 if(!Saved.IsValid())return;
 // LOOP-DOSSIER-GIVES-20261113: a file at MARKS from before the extra bearing was kept gets its one now, from the save's own night
 if(const S47J::FObj Record=S47J::Obj(Saved,TEXT("dossier"));Record.IsValid()){Unknowns.Load(Record);Unknowns.GrantMissing(DayNumber);return;}
 // A save from before the dossier: what it kept says which one it was for some of it (FS47Dossier::Rebuild). The same every time.
 TArray<FS47Dossier::FOld> Old;
 for(const TPair<FString,FString>& Finding:Findings)Old.Add({Finding.Key,Finding.Value,FindingDays.FindRef(Finding.Key)});
 TArray<FString> ClaimIds;
 if(Director)for(const FS47FreeformDirector::FPhotoClaim& Claim:Director->PhotoClaims())ClaimIds.Add(Claim.Id);
 const int32 Made=Unknowns.Rebuild(Old,ClaimIds);
 Unknowns.GrantMissing(DayNumber); // LOOP-DOSSIER-GIVES-20261113: a file the rebuild puts at MARKS gets its one extra bearing
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
 { // Dr. Ward's one line: nothing before a file is at MARKS, then that they are kept apart, in words and never what they are
  bool bWords=S47PredecessorText::Apart(0).IsEmpty()&&S47PredecessorText::Apart(-3).IsEmpty()&&S47PredecessorText::Apart(1).Contains(TEXT("One thing"))&&S47PredecessorText::Apart(3).Contains(TEXT("Three separate things"))
   &&S47PredecessorText::Apart(9)==S47PredecessorText::Apart(5);
  for(int32 Files=1;Files<=5;++Files)
  {
   const FString Line=S47PredecessorText::Apart(Files);
   bWords&=Line.StartsWith(TEXT(" "))&&S47Script::LintText(Line).IsEmpty()&&!S47Marginalia::NamesKind(Line)&&!Line.Contains(TEXT("UNKNOWN"));
   for(const TCHAR Ch:Line)bWords&=!FChar::IsDigit(Ch);
  }
  Check(bWords,TEXT("Dr. Ward's Monday note says nothing of the files until one is at MARKS, then only that they are kept apart"));
 }
 { // LOOP-DOSSIER-GIVES-20261113: what the session does with each stage
  FS47FreeformSession Gives(nullptr);Gives.Begin(nullptr);Gives.TestLiveRules();
  const int32 Tonight=NightOf(Gives.Clock());
  const auto HasLine=[&Gives](const FString& Want){TArray<FS47Task> Got;Gives.DossierTasks(Got);return Got.ContainsByPredicate([&Want](const FS47Task& T){return T.Text==Want;});};
  Gives.TagEvidence(TEXT("g-shape"),Low,Shape,1);Gives.TagEvidence(TEXT("g-ground"),Low,Ground,1);
  const bool bNotYet=!Gives.DossierExtraOpen(Tonight)&&!HasLine(S47Dossier::ExtraTaskLine());
  Gives.TagEvidence(TEXT("g-lamp"),Low,Lamp,2); // three kinds from two nights: MARKS, at 08:00 of the first day: tonight's
  const bool bOpen=Gives.Unknowns.StageOf(Low)==S47Dossier::EStage::Marks&&Gives.DossierExtraOpen(Tonight)&&HasLine(S47Dossier::ExtraTaskLine())&&Gives.Unknowns.HoursOf(Low)==TArray<int32>{8};
  const bool bOnce=Gives.DossierSpendExtra(Tonight)&&!Gives.DossierSpendExtra(Tonight)&&!Gives.DossierExtraOpen(Tonight+1)&&!HasLine(S47Dossier::ExtraTaskLine());
  // another file to MARKS later gives its own one more, once, and only where the live rules are
  Gives.TagEvidence(TEXT("g-small-a"),Small,Shape,3);Gives.TagEvidence(TEXT("g-small-b"),Small,Lamp,3);Gives.TagEvidence(TEXT("g-small-c"),Small,Ground,4);
  FS47FreeformSession Fixture(nullptr);Fixture.Begin(nullptr);
  Fixture.TagEvidence(TEXT("f-a"),Low,Shape,1);Fixture.TagEvidence(TEXT("f-b"),Low,Ground,1);Fixture.TagEvidence(TEXT("f-c"),Low,Lamp,2);
  Check(bNotYet&&bOpen&&bOnce&&Gives.DossierExtraOpen(Tonight)&&!Fixture.DossierExtraOpen(Tonight)&&!Fixture.RulesLive(),
   TEXT("LOOP-DOSSIER-GIVES: a file brought to MARKS opens one more bearing, on tonight's note until it is printed, once; the next file its own; none in a fixture"));
  Fixture.Stop();
  // OUTLINE's mark on the map, by the file's title, saved with the tag; HABITS's hours on tonight's note
  const FVector2D OutThere(210,-640);
  Gives.Unknowns.Tag(TEXT("g-tall"),Tall,Shape,5,2,&OutThere);
  Gives.TagEvidence(TEXT("g-low-geiger"),Low,S47Dossier::EType::Geiger,3); // four kinds, the lamp among them, three nights: HABITS
  const S47J::FObj Kept=S47J::Read(S47J::Write(Gives.Save()));
  Gives.Stop();
  FS47FreeformSession Back(nullptr);Back.Begin(Kept);Back.TestLiveRules();
  FS47MapView View;Back.DossierMap(View);
  const TArray<S47FieldMap::FFirstLogged> Marked=S47FieldMap::FirstLogged(View);
  const FString Hours=S47Dossier::HoursLine(Back.Unknowns.NumberOf(Low),Back.Unknowns.HoursOf(Low));
  Check(Marked.Num()==1&&Marked[0].Label==FString::Printf(TEXT("UNKNOWN %d"),Back.Unknowns.NumberOf(Tall))&&Marked[0].X==210&&Marked[0].Z==-640&&S47FieldMap::Description(View).ContainsByPredicate([](const FString& Said){return Said.Contains(TEXT("first logged: UNKNOWN 3"));})
   &&Back.Unknowns.StageOf(Low)==S47Dossier::EStage::Habits&&!Hours.IsEmpty()&&Back.Unknowns.ExtraState(Low)==2&&Back.Unknowns.ExtraState(Small)==1,
   TEXT("LOOP-DOSSIER-GIVES: OUTLINE's pencil mark and title on the map, the extras given and printed, and the hours survive a Continue"));
  TArray<FS47Task> NoteLines;Back.DossierTasks(NoteLines);
  Check(NoteLines.ContainsByPredicate([&Hours](const FS47Task& T){return T.Text==Hours;})&&Hours.StartsWith(TEXT("UNKNOWN 1 / Logged so far at 08:00"))&&S47Script::LintText(Hours).IsEmpty(),
   TEXT("LOOP-DOSSIER-GIVES: a file at HABITS puts the hours it was logged at on tonight's note, in the fiction's words"));
  Back.Stop();
 }
 return bOk;
}
