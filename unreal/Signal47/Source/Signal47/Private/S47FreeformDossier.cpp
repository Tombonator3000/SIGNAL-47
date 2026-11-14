#include "S47Freeform.h"
#include "S47AlienKinds.h"
#include "S47AnimalWarn.h"
#include "S47Attention.h"
#include "S47FieldRadio.h"
#include "S47Taken.h"
#include "S47FarmSite.h"
#include "S47RuleBreak.h"
#include "S47Tape.h"
#include "S47Dossier.h"
#include "S47FreeformDirector.h"
#include "S47Happenings.h"
#include "S47Main.h"
#include "S47Marginalia.h"
#include "S47PredecessorText.h"
#include "S47RecordsBinder.h"
#include "S47Rhythm.h"
#include "S47RoadGlimpse.h"
#include "S47SaroSave.h"
#include "S47Script.h"
#include "S47Thoughts.h"
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
 // LOOP-DOSSIER-GIVES-20261113: the hour and the place it was got, when it was got where the player stands (S47FieldMap::Me: the
 // sheet's own road-local frame; nothing off the sheet). An enlargement at the bench keeps neither.
 int32 Hour=-1;FVector2D Where(0,0);bool bWhere=false;
 if(bHere)
 {
  if(Clock()>=0)Hour=FMath::Clamp(int32(FMath::FloorToDouble(FMath::Fmod(Clock(),double(SecondsInDay))/3600.)),0,23);
  double MeX=0,MeZ=0,MeHeading=0;FString MeArea;bool bMeDriving=false;
  if(M&&S47FieldMap::Me(M,MeX,MeZ,MeHeading,MeArea,bMeDriving)){Where=FVector2D(MeX,MeZ);bWhere=true;}
 }
 return TagEvidenceAt(Key,Kind,Type,Night,Hour,bWhere?&Where:nullptr);
}
bool FS47FreeformSession::TagEvidenceAt(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Night,int32 Hour,const FVector2D* Where)
{
 // EVIDENCE-DOSSIER slice 2: the file it goes into is the one the player could tie it to (FS47Dossier::Tag); its stage before and after.
 const int32 FilesBefore=Unknowns.Order().Num();
 TArray<S47Dossier::EStage> Stages;
 for(int32 N=1;N<=FilesBefore;++N)Stages.Add(Unknowns.StageOfFile(N));
 if(!Unknowns.Tag(Key,Kind,Type,Night>0?Night:DayNumber,Hour,Where))return false;
 const int32 Number=Unknowns.FileOf(Key);
 const bool bNewFile=Number>FilesBefore;
 const S47Dossier::EStage Before=bNewFile?S47Dossier::EStage::NoFile:Stages[Number-1],After=Unknowns.StageOfFile(Number);
 const S47Dossier::EKind Opener=Unknowns.Order().IsValidIndex(Number-1)?Unknowns.Order()[Number-1]:Kind;
 // MARKS reached: one more bearing than the night allows, for this night, or from five in the morning the coming one (FS47Places).
 // Once a kind, whichever of its files gets there first.
 if(int32(Before)<int32(S47Dossier::EStage::Marks)&&int32(After)>=int32(S47Dossier::EStage::Marks)&&Unknowns.Grant(Opener,S47Dossier::ExtraNightFor(Clock())))
 {
  UE_LOG(LogTemp,Display,TEXT("S47: dossier: file %d at MARKS: one more bearing from night %d"),Number,S47Dossier::ExtraNightFor(Clock()));
 }
 // The stamp: the file's number and the kind of evidence. Never which one it is, and never what was found.
 // Slice 3: by the player's name beside the number, once the file has one
 const FString Said=S47Dossier::StampOf(Unknowns.TitleOf(Number),Type,S47Dossier::PhotoKey(Key),bNewFile,After!=Before?After:S47Dossier::EStage::NoFile);
 UE_LOG(LogTemp,Display,TEXT("S47: dossier: %s is %s in file %d (%s, opened by %s): %s"),*Key,S47Dossier::TypeId(Type),Number,S47Dossier::KindId(Kind),S47Dossier::KindId(Opener),*Said);
 if(M&&M->UI)M->UI->Toast(Said,5.f);
 DossierPayFiled(); // slice 3: a named file brought to HABITS out there is FILED now
 return true;
}
int32 FS47FreeformSession::DossierHabitsFrom(const FS47RhythmData& Rules)
{
 // EVIDENCE-DOSSIER slice 2 (M7): HABITS waits for the curve's first week with danger in it (director.json curve.weeks: the second)
 for(const FS47RhythmWeek& Week:Rules.Weeks)if(Week.Danger>0.f)return FMath::Max(1,Week.FromDay);
 return 1;
}
void FS47FreeformSession::DossierHeardTake(int32 SigNight,const FString& JobId,int32 AlienKind,int32 TierCode,double Cursor)
{
 // EVIDENCE-DOSSIER slice 2: the receiver's unusual take carries the night's visitor (S47AlienKinds). The table gives its tape to
 // the far two only (the near three are on the field tape): heard through at the receiver or the reel deck, past midnight's tier.
 if(!S47Dossier::TakeHeard(Cursor)||TierCode==int32(ES47Allowed::Hint)||JobId.IsEmpty())return;
 const S47Dossier::EKind Kind=AlienKind==int32(ES47AlienKind::Nordic)?S47Dossier::EKind::Silver:AlienKind==int32(ES47AlienKind::Cosmic)?S47Dossier::EKind::Shining:S47Dossier::EKind::None;
 if(Kind==S47Dossier::EKind::None)return;
 const FString Key=TEXT("take_")+JobId;
 if(!Unknowns.HasTag(Key))TagEvidence(Key,Kind,S47Dossier::EType::Tape,SigNight,false);
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
 for(int32 Number=1;Number<=Unknowns.Order().Num();++Number)
 {
  double X=0,Z=0;
  if(!Unknowns.FirstWhereOfFile(Number,X,Z))continue;
  S47FieldMap::FFirstLogged Logged;Logged.X=X;Logged.Z=Z;Logged.Label=Unknowns.TitleOf(Number); // slice 3: the player's name beside it
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
 for(int32 Number=1;Number<=Unknowns.Order().Num();++Number)
 {
  if(Unknowns.StageOfFile(Number)!=S47Dossier::EStage::Habits)continue;
  const FString Line=S47Dossier::HoursLineOf(Unknowns.TitleOf(Number),Unknowns.HoursOfFile(Number)); // slice 3: by its name too
  if(!Line.IsEmpty())Out.Add({Line,false});
 }
}
void FS47FreeformSession::LoadDossier(const S47J::FObj& Saved)
{
 Unknowns.Reset();HabitThought.Reset();HabitLastCheck=-1e9; // slice 3: the habits come to mind afresh after a Continue
 Unknowns.SetHabitsFrom(M&&M->Rhythm?DossierHabitsFrom(M->Rhythm->Data()):1); // slice 2: not saved; the rhythm's curve
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
 const TArray<S47Dossier::FFile> All=Unknowns.Files();
 const TArray<FIntPoint> Pairs=Unknowns.Beside();
 const int32 Tonight=NightOf(Clock());
 for(const S47Dossier::FFile& File:All)
 {
  // The photographs in the file, by the print each claim stands on now (the album's own id, as the OPS receipt names it).
  TArray<FString> PhotoLines;
  for(const FString& ClaimId:File.Photos)
  {
   S47Dossier::EKind Kind=S47Dossier::EKind::None;int32 Night=0;S47Dossier::ClaimKind(ClaimId,Kind,Night);
   const FS47FreeformDirector::FPhotoClaim* Claim=Director?Director->PhotoClaims().FindByPredicate([&ClaimId](const FS47FreeformDirector::FPhotoClaim& One){return One.Id==ClaimId;}):nullptr;
   PhotoLines.Add(Claim&&!Claim->PhotoId.IsEmpty()
    ?FString::Printf(TEXT("DAY %02d / ORIGINAL %s / %s"),S47Dossier::ShownNight(Night),*Claim->PhotoId,Claim->bPaid?TEXT("REGISTERED AT OPS"):TEXT("NOT YET REGISTERED AT OPS"))
    :FString::Printf(TEXT("DAY %02d / THE FRAME IS NOT IN THE ALBUM"),S47Dossier::ShownNight(Night)));
  }
  S47Binder::FUnknownIn Sheet;Sheet.Number=File.Number;Sheet.Title=File.Title;Sheet.Stage=File.Stage;Sheet.Opened=File.Opened;Sheet.Latest=File.Latest;Sheet.bAtMarks=File.bAtMarks;
  Sheet.Body=S47Dossier::Sheet(File,PhotoLines);
  // slice 3: its name and the suggestion; the files it could be filed as one with (those kept beside it first); what can come out again
  Sheet.Name=File.Name;Sheet.Suggest=S47Dossier::Suggest(File);Sheet.bFiled=File.bFiled;
  for(const bool bBeside:{true,false})for(const S47Dossier::FFile& Other:All)
  {
   if(Other.Number==File.Number)continue;
   const bool bPair=Pairs.Contains(FIntPoint(FMath::Min(File.Number,Other.Number),FMath::Max(File.Number,Other.Number)));
   if(bPair!=bBeside)continue;
   Sheet.Others.Add(Other.Number);Sheet.OthersBeside.Add(bPair);
  }
  for(const int32 From:File.Merged)if(Unknowns.CanTakeOut(File.Number,From,Tonight))Sheet.TakeOut.Add(From);
  Src.Unknowns.Add(MoveTemp(Sheet));
 }
}
// ---------- EVIDENCE-DOSSIER slice 3 (14 November): names, files filed as one, the allowance, the habits as thoughts ----------
bool FS47FreeformSession::DossierName(int32 Number,const FString& Typed,FString& Words,bool bAt)
{
 if(!bAt){Words=S47Dossier::AwayWords();return false;}
 if(!Unknowns.IsFile(Number)){Words.Reset();return false;}
 FString Clean,Why;
 if(!S47Dossier::CleanName(Typed,Clean,Why)){Words=Why;return false;}
 Unknowns.SetName(Number,Clean,DayNumber);
 Words=S47Dossier::NameSlip(Unknowns.TitleOf(Number));
 const int32 Paid=DossierPayFiled();
 if(Paid>0)Words+=FString::Printf(TEXT(" FILED UNDER ITS NAME. $%d CREDITED."),Paid);
 else if(Unknowns.Filed(Number))Words+=TEXT(" FILED UNDER ITS NAME.");
 UE_LOG(LogTemp,Display,TEXT("S47: dossier: file %d named '%s' (filed %d, paid %d)"),Number,*Clean,Unknowns.Filed(Number)?1:0,Paid);
 return true;
}
bool FS47FreeformSession::DossierFileAsOne(int32 A,int32 B,FString& Words,bool bAt)
{
 if(!bAt){Words=S47Dossier::AwayWords();return false;}
 const int32 Night=NightOf(Clock());
 if(!Unknowns.Merge(A,B,Night)){Words.Reset();return false;}
 const int32 Into=FMath::Min(A,B),From=FMath::Max(A,B);
 // Nothing says whether they are one thing: the file says what it holds, and its pencil is struck where they disagree.
 UE_LOG(LogTemp,Display,TEXT("S47: dossier: file %d filed in file %d as one, night %d (%s; stage %s)"),From,Into,Night,Unknowns.WrongMerge(Into)?TEXT("not one kind"):TEXT("one kind"),S47Dossier::StageWord(Unknowns.StageOfFile(Into)));
 Words=S47Dossier::MergeSlip(Into,From);
 const int32 Paid=DossierPayFiled();
 if(Paid>0)Words+=FString::Printf(TEXT(" FILED UNDER ITS NAME. $%d CREDITED."),Paid);
 return true;
}
bool FS47FreeformSession::DossierTakeOut(int32 Into,int32 From,FString& Words,bool bAt)
{
 if(!bAt){Words=S47Dossier::AwayWords();return false;}
 if(!Unknowns.TakeOut(Into,From,NightOf(Clock()))){Words.Reset();return false;}
 UE_LOG(LogTemp,Display,TEXT("S47: dossier: file %d taken out of file %d again"),From,Into);
 Words=S47Dossier::TakeOutSlip(Into,From);
 return true;
}
bool FS47FreeformSession::DossierAct(const FString& Do,const FString& Typed,FString& Words)
{
 TArray<FString> Parts;Do.ParseIntoArray(Parts,TEXT(":"),true);
 const bool bAt=NearBinder();
 if(Parts.Num()==2&&Parts[0]==TEXT("name"))return DossierName(FCString::Atoi(*Parts[1]),Typed,Words,bAt);
 if(Parts.Num()==3&&Parts[0]==TEXT("merge"))return DossierFileAsOne(FCString::Atoi(*Parts[1]),FCString::Atoi(*Parts[2]),Words,bAt);
 if(Parts.Num()==3&&Parts[0]==TEXT("out"))return DossierTakeOut(FCString::Atoi(*Parts[1]),FCString::Atoi(*Parts[2]),Words,bAt);
 return false;
}
int32 FS47FreeformSession::DossierPayFiled()
{
 // The design (part 3, "Penger"): a file FILED gives the field allowance once, under the week's cap like all field money.
 int32 Paid=0;
 for(int32 Number=1;Number<=Unknowns.Order().Num();++Number)
 {
  if(!Unknowns.Filed(Number)||Unknowns.Paid(Number))continue;
  Unknowns.MarkPaid(Number);
  Paid+=EarnSide(FiledAllowance,TEXT("dossier filed"));
 }
 return Paid;
}
void FS47FreeformSession::DossierHabitStep()
{
 // Once a game minute at most: out of the building, a file at HABITS whose dark hour is coming up comes to mind, once a night.
 if(!M||!RulesLive()||Unknowns.IsEmpty()||FMath::Abs(Clock()-HabitLastCheck)<60.)return;
 HabitLastCheck=Clock();
 if(S47SaroSave::InStation(M))return;
 if(!HabitHourMay(Held(),(FarmSite&&FarmSite->VisitWantsGround())||(Attention&&Attention->Visiting()),M->FieldRadio.IsValid()&&M->FieldRadio->PresenceHunting(),Taken&&Taken->Busy()))return;
 const int32 Night=NightOf(Clock());int32 Hour=-1;
 const int32 Number=Unknowns.HabitHourFile(Clock(),HabitThought,Night,Hour);
 if(Number>0&&S47Thoughts::ThinkLine(M,S47Dossier::HabitHourThought(Hour),TEXT("habit-hour")))HabitThought.Add(Number,Night);
}
bool FS47FreeformSession::DossierHabitLamp(S47Dossier::EKind Near)
{
 // The lamp raised near one whose answer to the lamp a file at HABITS holds: the habit in the player's own words, once a night.
 if(!M||Unknowns.IsEmpty())return false;
 const int32 Night=NightOf(Clock());
 const int32 Number=Unknowns.HabitLampFile(Near,HabitThought,Night);
 if(Number<1||!S47Thoughts::ThinkLine(M,S47Dossier::HabitLampThought(S47Dossier::Signature(Near,S47Dossier::EType::Lamp)),TEXT("habit-lamp")))return false;
 HabitThought.Add(Number,Night);
 return true;
}
FString FS47FreeformSession::TestNameDossier()
{
 // F10: each file named by the suggestion it is filled with at the binder, as if written there
 FString Said;
 for(const S47Dossier::FFile& File:Unknowns.Files())
 {
  FString Words;
  const FString Suggested=S47Dossier::Suggest(File);
  DossierName(File.Number,Suggested.IsEmpty()?FString(TEXT("Nothing yet")):Suggested,Words,true);
  Said+=Words+TEXT(" ");
 }
 if(Said.IsEmpty())Said=TEXT("No file to name. ");
 return Said+FString::Printf(TEXT("Dr. Ward's count: %d of the near three filed (%d points)."),Unknowns.KindsFiled(),S47Dossier::WardPoints(Unknowns.KindsFiled()));
}
FString FS47FreeformSession::TestMergeDossier()
{
 // F10: what was filed as one tonight is taken out again; else the first pair kept side by side (or the first two files) filed as one
 FString Words;
 const int32 Tonight=NightOf(Clock());
 for(const S47Dossier::FFile& File:Unknowns.Files())for(const int32 From:File.Merged)
  if(Unknowns.CanTakeOut(File.Number,From,Tonight)){DossierTakeOut(File.Number,From,Words,true);return Words;}
 const TArray<FIntPoint> Pairs=Unknowns.Beside();
 const TArray<S47Dossier::FFile> All=Unknowns.Files();
 const FIntPoint Two=Pairs.IsEmpty()?(All.Num()>=2?FIntPoint(All[0].Number,All[1].Number):FIntPoint(0,0)):Pairs[0];
 if(Two.X<1||!DossierFileAsOne(Two.X,Two.Y,Words,true))return TEXT("Nothing to file as one: fewer than two files.");
 return Words+FString::Printf(TEXT(" (%s; stage %s.)"),Unknowns.WrongMerge(Two.X)?TEXT("not one kind: a wrong one"):TEXT("one kind"),S47Dossier::StageWord(Unknowns.StageOfFile(Two.X)));
}
FString FS47FreeformSession::TestFillDossier()
{
 // F10 (slice 2): a few nights as every channel could leave them, with the places they were got at, so that the files show what the
 // player can tie and what he cannot. The small ones at a site over three nights (seen, the field tape, the dogs, the counter, their
 // prints, the lamp); a tape of the low one never seen (a file of its own); the low one crossing the road and its prints (another);
 // then a visit of the low one ties the two (side by side, not merged); the tall one's tape got at the small ones' site the same
 // night (their tape's pencil struck); the silver one's photograph, its take on the receiver and its answer to the lamp. The last
 // tag goes the game's own way, so its stamp is on the screen. The working names are in what this says (the dev menu only).
 using namespace S47Dossier;
 const int32 First=FMath::Max(1,DayNumber-3);
 const FVector2D Site(400,-900),Road(-30,2400),Other(2600,-200);
 const auto At=[this](const TCHAR* Key,EKind Kind,EType Type,int32 Night,int32 Hour,const FVector2D* Where){return Unknowns.Tag(FString::Printf(TEXT("dev-dossier-%s"),Key),Kind,Type,Night,Hour,Where);};
 At(TEXT("small-seen"),EKind::Small,EType::Shape,First,2,&Site);At(TEXT("small-dogs"),EKind::Small,EType::Animals,First,2,&Site);At(TEXT("small-tape"),EKind::Small,EType::Tape,First,2,&Site);
 At(TEXT("small-seen-2"),EKind::Small,EType::Shape,First+1,3,&Site);At(TEXT("small-prints"),EKind::Small,EType::Ground,First+1,9,&Site);At(TEXT("small-counter"),EKind::Small,EType::Geiger,First+1,9,&Site);
 At(TEXT("low-tape"),EKind::Low,EType::Tape,First,1,nullptr);
 Unknowns.Tag(FString::Printf(TEXT("strange.crawler.n%d.dev"),First+1),EKind::Low,EType::Shape,First+1,0,&Road);
 At(TEXT("low-road-prints"),EKind::Low,EType::Ground,First+1,10,&Road);
 At(TEXT("low-seen"),EKind::Low,EType::Shape,First+2,2,&Other);At(TEXT("low-tape-2"),EKind::Low,EType::Tape,First+2,2,&Other);At(TEXT("low-coyote"),EKind::Low,EType::Animals,First+2,2,&Other);
 At(TEXT("small-seen-3"),EKind::Small,EType::Shape,First+2,3,&Site);At(TEXT("tall-tape"),EKind::Tall,EType::Tape,First+2,3,&Site);
 Unknowns.Tag(FString::Printf(TEXT("strange.nordic.n%d.dev"),First),EKind::Silver,EType::Shape,First,1,nullptr);
 At(TEXT("silver-take"),EKind::Silver,EType::Tape,First,1,nullptr);At(TEXT("silver-gone"),EKind::Silver,EType::Lamp,First,2,nullptr);
 const bool bStamped=TagEvidenceAt(TEXT("dev-dossier-rule-still-held"),EKind::Small,EType::Lamp,First+2,3,&Site);
 FString Said;
 for(const S47Dossier::FFile& File:Unknowns.Files())
 {
  Said+=FString::Printf(TEXT("%s (opened by %s) %s, %d %s"),*File.Title,KindId(Unknowns.Order()[File.Number-1]),*File.Stage,File.Nights,File.Nights==1?TEXT("night"):TEXT("nights"));
  for(const int32 Other2:File.Beside)Said+=FString::Printf(TEXT(", beside %d"),Other2);
  for(const S47Dossier::FBox& Slot:File.Boxes)if(!Slot.Struck.IsEmpty())Said+=FString::Printf(TEXT(", %s struck"),*Slot.Label);
  Said+=TEXT(". ");
 }
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
  Gives.TagEvidence(TEXT("g-shape-2"),Low,Shape,2); // (slice 2: the shape again, so that the lamp's night ties to the file)
  Gives.TagEvidence(TEXT("g-lamp"),Low,Lamp,2); // three kinds from two nights: MARKS, at 08:00 of the first day: tonight's
  const bool bOpen=Gives.Unknowns.StageOf(Low)==S47Dossier::EStage::Marks&&Gives.DossierExtraOpen(Tonight)&&HasLine(S47Dossier::ExtraTaskLine())&&Gives.Unknowns.HoursOf(Low)==TArray<int32>{8};
  const bool bOnce=Gives.DossierSpendExtra(Tonight)&&!Gives.DossierSpendExtra(Tonight)&&!Gives.DossierExtraOpen(Tonight+1)&&!HasLine(S47Dossier::ExtraTaskLine());
  // another file to MARKS later gives its own one more, once, and only where the live rules are
  Gives.TagEvidence(TEXT("g-small-a"),Small,Shape,3);Gives.TagEvidence(TEXT("g-small-b"),Small,Lamp,3);Gives.TagEvidence(TEXT("g-small-d"),Small,Shape,4);Gives.TagEvidence(TEXT("g-small-c"),Small,Ground,4);
  FS47FreeformSession Fixture(nullptr);Fixture.Begin(nullptr);
  Fixture.TagEvidence(TEXT("f-a"),Low,Shape,1);Fixture.TagEvidence(TEXT("f-b"),Low,Ground,1);Fixture.TagEvidence(TEXT("f-b2"),Low,Shape,2);Fixture.TagEvidence(TEXT("f-c"),Low,Lamp,2);
  Check(bNotYet&&bOpen&&bOnce&&Gives.DossierExtraOpen(Tonight)&&!Fixture.DossierExtraOpen(Tonight)&&!Fixture.RulesLive(),
   TEXT("LOOP-DOSSIER-GIVES: a file brought to MARKS opens one more bearing, on tonight's note until it is printed, once; the next file its own; none in a fixture"));
  Fixture.Stop();
  // OUTLINE's mark on the map, by the file's title, saved with the tag; HABITS's hours on tonight's note
  const FVector2D OutThere(210,-640);
  Gives.Unknowns.Tag(TEXT("g-tall"),Tall,Shape,5,2,&OutThere);
  Gives.TagEvidence(TEXT("g-low-shape-3"),Low,Shape,3);
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
   TEXT("LOOP-DOSSIER-GIVES: a file at HABITS gives the hours it was logged at, in the fiction's words (TASKS-TEN-20261113: an open matter under Unknowns on NOW, no row tonight)"));
  Back.Stop();
 }
 { // EVIDENCE-DOSSIER slice 2: a signature for each of the three near ones in every channel that was the same for all
  bool bOwn=true;FString Why;
  TArray<FString> Said;
  for(int32 A=0;A<3;++A)for(int32 B=A+1;B<3;++B)
  {
   const S47Dossier::EKind KA=S47Dossier::KindOfVisit(A),KB=S47Dossier::KindOfVisit(B);
   bOwn&=FS47Tape::OddWordsOf(TEXT("voices"),S47Dossier::KindId(KA))!=FS47Tape::OddWordsOf(TEXT("voices"),S47Dossier::KindId(KB)); // TAPE
   bOwn&=S47Dossier::AlienKindOf(KA)!=S47Dossier::AlienKindOf(KB);
   bOwn&=FS47FarmSite::PrintsText(A)!=FS47FarmSite::PrintsText(B);                                                                  // GROUND
   bOwn&=S47Dossier::GeigerFactor(KA)!=S47Dossier::GeigerFactor(KB);                                                                  // GEIGER
   bOwn&=FS47AnimalWarn::ForVisit(A)!=FS47AnimalWarn::ForVisit(B);                                                                    // ANIMALS
  }
  for(int32 Who=0;Who<3;++Who)
  {
   const S47Dossier::EKind Kind=S47Dossier::KindOfVisit(Who);
   Said.Add(FS47Tape::OddWordsOf(TEXT("voices"),S47Dossier::KindId(Kind)));Said.Add(FS47FarmSite::PrintsText(Who).TrimStart());
   // each says what its pencil in the file says, in its own words: the prints by daylight carry the table's GROUND
   bOwn&=!FS47FarmSite::PrintsText(Who).IsEmpty();
  }
  bOwn&=FS47FarmSite::PrintsText(-1).IsEmpty()&&FS47FarmSite::PrintsText(3).IsEmpty()&&FS47FarmSite::PrintsText(1).Contains(TEXT("fours"))&&FS47FarmSite::PrintsText(2).Contains(TEXT("long narrow"))&&FS47FarmSite::PrintsText(0).Contains(TEXT("bare"));
  Why=S47Dossier::LintWords(Said);
  Check(bOwn&&Why.IsEmpty(),TEXT("slice 2: the small ones, the low one and the tall one differ on the tape, on the ground, on the counter and in the animals, in plain words"));
  if(!Why.IsEmpty())Lines.Add(TEXT("     ")+Why);
 }
 { // EVIDENCE-DOSSIER slice 2: the receiver's take is the far two's tape, only heard through and past midnight's tier
  FS47FreeformSession Rx(nullptr);Rx.Begin(nullptr);
  const double Through=S47AlienKinds::SignalFrom+4.,Early=S47AlienKinds::SignalFrom+1.;
  Rx.DossierHeardTake(3,TEXT("FF-1"),int32(ES47AlienKind::Nordic),int32(ES47Allowed::Glimpse),Early);
  Rx.DossierHeardTake(3,TEXT("FF-2"),int32(ES47AlienKind::Cosmic),int32(ES47Allowed::Hint),Through);
  Rx.DossierHeardTake(3,TEXT("FF-3"),int32(ES47AlienKind::Tall),int32(ES47Allowed::Full),Through);
  const bool bNone=Rx.Unknowns.IsEmpty();
  Rx.DossierHeardTake(3,TEXT("FF-4"),int32(ES47AlienKind::Nordic),int32(ES47Allowed::Glimpse),Through);
  Rx.DossierHeardTake(3,TEXT("FF-4"),int32(ES47AlienKind::Nordic),int32(ES47Allowed::Full),Through+1);
  Check(bNone&&Rx.Unknowns.Tags().Num()==1&&Rx.Unknowns.Has(S47Dossier::EKind::Silver,S47Dossier::EType::Tape)&&Rx.Unknowns.Tags()[0].Day==3&&!Rx.Unknowns.Tags()[0].bWhere,
   TEXT("slice 2: the receiver's take goes in the far two's files only, heard through, not before midnight, once a take"));
  Rx.Stop();
 }
 { // EVIDENCE-DOSSIER slice 2: an older save with the tall one's rule learned, rebuilt; the shade still nowhere
  FS47Dossier D;
  TArray<FS47Dossier::FOld> Learned;Learned.Add({TEXT("rule-tall-froze"),TEXT("Something tall."),5});Learned.Add({TEXT("rule-still-held"),TEXT("A small one."),4});
  Check(D.Rebuild(Learned,TArray<FString>{TEXT("strange.shade.n2"),TEXT("strange.tallfigure.n6")})==3&&D.NumberOf(S47Dossier::EKind::Small)==1&&D.NumberOf(S47Dossier::EKind::Tall)==2&&D.FilesOf(S47Dossier::EKind::Tall)==1
   &&D.Has(S47Dossier::EKind::Tall,S47Dossier::EType::Lamp)&&D.Order().Num()==2,TEXT("slice 2: an older save keeps one file a kind on rebuild, the tall one's rule in it; the shade has none"));
 }
 { // EVIDENCE-DOSSIER slice 3: the name and filing as one at the binder only, the allowance once, the binder's lines, and a Continue
  FS47FreeformSession At(nullptr);At.Begin(nullptr);
  const S47Dossier::EType Tape=S47Dossier::EType::Tape;
  for(int32 Night=2;Night<=4;++Night)for(const S47Dossier::EType T:{Shape,Night==2?Ground:Night==3?Tape:Lamp})At.Unknowns.Tag(FString::Printf(TEXT("s3-%d-%d"),Night,int32(T)),Small,T,Night,2);
  At.Unknowns.Tag(TEXT("s3-low"),Low,Shape,4,1);At.Unknowns.Tag(TEXT("s3-low2"),Low,Ground,4,1);
  FString Words;
  const bool bAway=!At.DossierName(1,TEXT("Big head"),Words,false)&&Words==S47Dossier::AwayWords()&&At.Unknowns.NameOf(1).IsEmpty()&&!At.DossierAct(TEXT("name:1"),TEXT("Big head"),Words)
   &&!At.DossierAct(TEXT("merge:1:2"),FString(),Words)&&At.Unknowns.IsFile(2);
  const bool bRefused=!At.DossierName(1,TEXT("Grey one"),Words,true)&&At.Unknowns.NameOf(1).IsEmpty()&&!Words.IsEmpty();
  const bool bNamed=At.DossierName(1,TEXT(" Big   head "),Words,true)&&At.Unknowns.NameOf(1)==TEXT("Big head")&&Words.StartsWith(TEXT("WRITTEN ON THE FILE: UNKNOWN 1 (BIG HEAD)."))
   &&Words.Contains(FString::Printf(TEXT("$%d CREDITED"),int32(FiledAllowance)))&&At.Unknowns.Paid(1)&&At.Unknowns.Filed(1);
  const bool bOnce=At.DossierName(1,TEXT("Big head again"),Words,true)&&!Words.Contains(TEXT("CREDITED"))&&At.DossierPayFiled()==0;
  const S47Binder::FSource Src=At.BinderSource();
  TArray<FS47Task> NoteLines;At.DossierTasks(NoteLines);
  const bool bShown=Src.Unknowns.Num()==2&&Src.Unknowns[0].Name==TEXT("Big head again")&&Src.Unknowns[0].bFiled&&Src.Unknowns[0].Suggest==TEXT("Big head, bare feet")&&Src.Unknowns[0].Others==TArray<int32>{2}
   &&Src.Unknowns[1].Suggest==TEXT("Folded one, in fours")&&Src.Unknowns[0].Body.StartsWith(TEXT("WORKING NAME  BIG HEAD AGAIN"))
   &&NoteLines.ContainsByPredicate([](const FS47Task& T){return T.Text.StartsWith(TEXT("UNKNOWN 1 (Big head again) / Logged so far at 02:00"));});
  Check(bAway&&bRefused&&bNamed&&bOnce&&bShown,TEXT("slice 3: a name is written on a file at the binder only, by the player's own words; a FILED file is paid once; the name is on the binder's sheet and tonight's hours line"));
  // a wrong one filed as one: nothing says so; it counts for nothing with Dr. Ward until it is taken out again that night, also after a Continue
  const bool bMerged=At.DossierFileAsOne(2,1,Words,true)&&Words==S47Dossier::MergeSlip(1,2)&&At.Unknowns.WrongMerge(1)&&At.BinderSource().Unknowns.Num()==1&&At.BinderSource().Unknowns[0].TakeOut==TArray<int32>{2};
  const S47J::FObj Kept=S47J::Read(S47J::Write(At.Save()));
  At.Stop();
  FS47FreeformSession Back(nullptr);Back.Begin(Kept);
  const bool bBack=!Back.Unknowns.IsFile(2)&&Back.Unknowns.KindsFiled()==0&&Back.Unknowns.NameOf(1)==TEXT("Big head again");
  const bool bOut=Back.DossierTakeOut(1,2,Words,true)&&Words==S47Dossier::TakeOutSlip(1,2)&&Back.Unknowns.IsFile(2)&&Back.Unknowns.KindsFiled()==1&&!Back.DossierTakeOut(1,2,Words,true);
  Check(bMerged&&bBack&&bOut,TEXT("slice 3: two filed as one at the binder (a wrong one too, never said), kept through a Continue, out again that night; Dr. Ward's count follows"));
  Back.Stop();
 }
 {
  // the lamp's edge is taken every frame: raised on a frame the visit did not reach, it is not "raised" on the next that does
  bool bWas=false;
  const bool bFirst=FS47FarmSite::LampRaised(true,bWas),bLater=FS47FarmSite::LampRaised(true,bWas),bOff=FS47FarmSite::LampRaised(false,bWas),bAgain=FS47FarmSite::LampRaised(true,bWas);
  Check(bFirst&&!bLater&&!bOff&&bAgain,TEXT("slice 3 (review): the lamp counts as raised near one only on the frame it went on, whatever frames the visit skipped"));
 }
 Check(HabitHourMay(false,false,false,false)&&!HabitHourMay(true,false,false,false)&&!HabitHourMay(false,true,false,false)&&!HabitHourMay(false,false,true,false)&&!HabitHourMay(false,false,false,true),
  TEXT("slice 3 (review): the hour's habit never comes to mind while the night is held, a visit is on, something hunts on the set or a light is taking someone"));
 { // EVIDENCE-DOSSIER slice 2 (M7): the pacing against the week gates and the tiers, from director.json itself
  FS47Rhythm Rules(nullptr);Rules.LoadRules();
  const FS47RhythmData& R=Rules.Data();
  const auto From=[&R](const TCHAR* Id,int32 Else){const FS47RhythmEvent* E=R.Find(FName(Id));return E?E->FromDay:Else;};
  S47Dossier::FGates G;
  G.Visit=FMath::Min(From(TEXT("site_visit"),2),From(TEXT("field_visit"),2));G.Glimpse=From(TEXT("road_glimpse"),3);G.Sky=From(TEXT("horizon_figure"),2);
  G.Figure=From(TEXT("figure_near"),10);G.RuleBreak=FS47RuleBreak::FromNight;G.HabitsFrom=DossierHabitsFrom(R);
  // the receiver's take is the far two's from the first night (the unusual take has no week gate), past midnight's tier
  G.Take=1;
  bool bWeek1=true,bWeek4=true,bFar=true;int32 FirstMarks=99;FString Nights;
  for(int32 K=0;K<int32(S47Dossier::EKind::Count);++K)
  {
   const S47Dossier::EKind Kind=S47Dossier::EKind(K);
   const int32 Marks=S47Dossier::Earliest(Kind,S47Dossier::EStage::Marks,G),Habits=S47Dossier::Earliest(Kind,S47Dossier::EStage::Habits,G);
   bWeek1&=Habits==0||Habits>7;
   if(Marks>0)FirstMarks=FMath::Min(FirstMarks,Marks);
   if(S47Dossier::StopsAtMarks(Kind))bFar&=Habits==0&&Marks>0;else bWeek4&=Habits>7&&Habits<=30;
   Nights+=FString::Printf(TEXT(" %s MARKS %d HABITS %d;"),S47Dossier::KindId(Kind),Marks,Habits);
  }
  Check(G.HabitsFrom==8&&bWeek1&&bWeek4&&bFar&&FirstMarks<=14,*FString::Printf(TEXT("slice 2 pacing (%s): nothing at HABITS in week 1 (it waits for night %d), the near three can by week 4, the far two stop at MARKS, the first MARKS by night %d:%s"),
   *Rules.Source(),G.HabitsFrom,FirstMarks,*Nights));
 }
 return bOk;
}
