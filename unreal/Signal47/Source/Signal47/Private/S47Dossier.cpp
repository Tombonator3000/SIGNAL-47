#include "S47Dossier.h"
#include "S47AlienKinds.h"
#include "S47Marginalia.h"
#include "S47RoadGlimpse.h"
#include "S47Script.h"
#include "Algo/Reverse.h"

// EVIDENCE-DOSSIER-20261113 (-> M3, M4), slice 1: the table, the tags, the stages, the save and the tests. See S47Dossier.h.
namespace S47Dossier
{
namespace Tables
{
 constexpr int32 KindN=int32(EKind::Count),TypeN=int32(EType::Count);
 // The code's working names, as the save has them and as FS47FreeformDirector's photograph groups have them. Never shown.
 const TCHAR* const KindIds[KindN]={TEXT("roswell"),TEXT("pale"),TEXT("tall"),TEXT("nordic"),TEXT("cosmic")};
 const TCHAR* const TypeIds[TypeN]={TEXT("shape"),TEXT("tape"),TEXT("ground"),TEXT("lamp"),TEXT("geiger"),TEXT("animals")};
 const TCHAR* const TypeLabels[TypeN]={TEXT("SHAPE"),TEXT("TAPE"),TEXT("GROUND"),TEXT("LAMP"),TEXT("GEIGER"),TEXT("ANIMALS")};
 const TCHAR* const StageWords[4]={TEXT(""),TEXT("OUTLINE"),TEXT("MARKS"),TEXT("HABITS")};
 // One box of one kind: the internal word for what it leaves there (empty: nothing), and the player's pencil for it. The pencil
 // says only what was there to be seen, heard or measured; never what it is, where it is from or what it does with anyone.
 struct FCell { const TCHAR* Sign; const TCHAR* Pencil; };
 const FCell Cells[KindN][TypeN]={
  // the small ones (shape, tape, ground, lamp, geiger, animals)
  {{TEXT("small"),TEXT("Small. The head too big for the body.")},
   {TEXT("ladder"),TEXT("Notes climbing like a ladder, ending like a question. Then a pause.")},
   {TEXT("bare"),TEXT("Small bare prints. Stones turned damp side up.")},
   {TEXT("passes"),TEXT("Lamp out and standing still: it looked past me.")},
   {TEXT("faint"),TEXT("The counter ticks up a little.")},
   {TEXT("dogs"),TEXT("The dogs go quiet.")}},
  // the low one
  {{TEXT("low"),TEXT("Low and folded. Pale. Quick.")},
   {TEXT("pulse"),TEXT("A double beat like a heart. Breath that rasps.")},
   {TEXT("fours"),TEXT("Prints in fours, close together, off the edge at a run.")},
   {TEXT("stops"),TEXT("Standing still in the dark was not enough. It stopped and turned to where I was.")},
   {TEXT("none"),TEXT("The counter says nothing.")},
   {TEXT("flee"),TEXT("Cattle and coyotes run from it.")}},
  // the tall one
  {{TEXT("tall"),TEXT("Tall and long. Keeps far off.")},
   {TEXT("deep"),TEXT("Slow deep beats. Two notes grinding on each other. Metal creaking.")},
   {TEXT("stride"),TEXT("Long narrow prints, a long stride apart.")},
   {TEXT("freezes"),TEXT("Stopped dead in the first of the lamp.")},
   {TEXT("clear"),TEXT("The counter climbs.")},
   {TEXT("birds"),TEXT("The birds go up.")}},
  // the silver man: only shape, tape and lamp
  {{TEXT("still"),TEXT("Shaped like a man. Too still.")},
   {TEXT("chords"),TEXT("Clean chords, sliding slowly out of tune.")},
   {TEXT(""),TEXT("")},
   {TEXT("gone"),TEXT("Gone when I looked straight at it.")},
   {TEXT(""),TEXT("")},
   {TEXT(""),TEXT("")}},
  // the one that shines: only shape, tape and lamp
  {{TEXT("shines"),TEXT("Lit from inside.")},
   {TEXT("counts"),TEXT("Beeps in groups, like counting. Then an answer.")},
   {TEXT(""),TEXT("")},
   {TEXT("ownlight"),TEXT("Shows by its own light.")},
   {TEXT(""),TEXT("")},
   {TEXT(""),TEXT("")}}};
 const TCHAR* const HowFilm=TEXT("on film");
 const TCHAR* const HowSeen=TEXT("seen"); // the visit's field page: it was seen, near or far, and no frame of it is kept
 const TCHAR* const HowNoted=TEXT("noted");
 const TCHAR* const NextOutline=TEXT("Three kinds of evidence from two nights make MARKS.");
 const TCHAR* const NextMarks=TEXT("Four kinds from three nights, the lamp among them, make HABITS.");
 const TCHAR* const NextStops=TEXT("It keeps its distance. This file stays open.");
 const TCHAR* const NextHabits=TEXT("Its habits are on paper.");
 const TCHAR* const StampNew=TEXT(" New file in the notebook.");
 const TCHAR* const SheetWords[]={TEXT("STAGE"),TEXT("NIGHTS"),TEXT("FIRST DAY"),TEXT("LAST DAY"),TEXT("NOTHING KEPT"),TEXT("PHOTOGRAPHS"),TEXT("NONE IN THE ALBUM."),TEXT("IN PENCIL"),
  TEXT("READ ONLY. KEPT AS THE NOTEBOOK HAS IT.")};
 // LOOP-DOSSIER-GIVES-20261113: what reaching a stage gave, under the stage; the extra bearing's words; the hours on tonight's note.
 const TCHAR* const GaveMapped=TEXT("Where it was first logged is on the map.");
 const TCHAR* const GaveUnmapped=TEXT("Not on the map yet: nothing of it was logged out where it was.");
 const TCHAR* const GaveExtraOpen=TEXT("The array knows what to listen for now: one more bearing, once.");
 const TCHAR* const GaveExtraSpent=TEXT("The array knew what to listen for: its one more bearing is printed.");
 const TCHAR* const GaveListens=TEXT("The array knows what to listen for now.");
 const TCHAR* const GaveHours=TEXT("The hours it was logged at are on tonight's note.");
 const TCHAR* const GaveNoHours=TEXT("No hours were kept for it.");
 const TCHAR* const ExtraSlip=TEXT("THE STATION KNOWS WHAT TO LISTEN FOR NOW: ONE MORE BEARING TONIGHT THAN THE NIGHT GIVES.");
 const TCHAR* const ExtraSaid=TEXT("The station knows what to listen for now: one more bearing tonight.");
 const TCHAR* const ExtraTask=TEXT("ARRAY / The station knows what to listen for now: one more bearing slip can come tonight.");
 // Slice 2: a contradicted pencil on the binder's typed sheet (the pair's words are BesideLine and BesideWords).
 const TCHAR* const StruckTyped=TEXT("STRUCK / ");
 const int32 MaxTags=4096,MaxKey=160,MaxDay=36500,MaxFiles=24;
 const double MaxWhere=1e6;
 bool Valid(EKind Kind){return int32(Kind)>=0&&int32(Kind)<KindN;}
 bool Valid(EType Type){return int32(Type)>=0&&int32(Type)<TypeN;}
 bool Earlier(const FS47Dossier::FTag& A,const FS47Dossier::FTag& B){return A.Day!=B.Day?A.Day<B.Day:A.Key<B.Key;}
 // Slice 2: got together, as the player could tell: the same night, and near each other where both kept a place; of the same kind,
 // the same night is enough when one of them kept none (a tape played back at the desk, a photograph enlarged at the bench).
 bool Together(const FS47Dossier::FTag& A,const FS47Dossier::FTag& B)
 {
  if(A.Day!=B.Day)return false;
  if(A.bWhere&&B.bWhere)return FVector2D::Distance(FVector2D(A.X,A.Z),FVector2D(B.X,B.Z))<=S47Dossier::NearM;
  return A.Kind==B.Kind;
 }
}
const TCHAR* KindId(EKind Kind){return Tables::Valid(Kind)?Tables::KindIds[int32(Kind)]:TEXT("");}
const TCHAR* TypeId(EType Type){return Tables::Valid(Type)?Tables::TypeIds[int32(Type)]:TEXT("");}
EKind KindOfId(const FString& Id)
{
 for(int32 At=0;At<Tables::KindN;++At)if(Id==Tables::KindIds[At])return EKind(At);
 return EKind::None;
}
EType TypeOfId(const FString& Id)
{
 for(int32 At=0;At<Tables::TypeN;++At)if(Id==Tables::TypeIds[At])return EType(At);
 return EType::None;
}
EKind KindOfGroup(const FString& Group)
{
 // Tom, 13 November: the road's crawler is the low one and the road's tall figure the tall one. The shade and the shadow: no file.
 if(Group==TEXT("crawler"))return EKind::Low;
 if(Group==TEXT("tallfigure"))return EKind::Tall;
 return KindOfId(Group);
}
EKind KindOfVisit(int32 VisitWho){return VisitWho==0?EKind::Small:VisitWho==1?EKind::Low:VisitWho==2?EKind::Tall:EKind::None;}
bool PhotoKey(const FString& TagKey){return TagKey.StartsWith(TEXT("strange."),ESearchCase::CaseSensitive);}
bool ClaimKind(const FString& ClaimId,EKind& OutKind,int32& OutNight)
{
 OutKind=EKind::None;OutNight=0;
 if(!PhotoKey(ClaimId))return false;
 const FString Rest=ClaimId.Mid(8);int32 Dot=INDEX_NONE;
 if(!Rest.FindChar(TEXT('.'),Dot)||Dot<1)return false;
 const FString Night=Rest.Mid(Dot+1);
 if(Night.Len()<2||Night[0]!=TEXT('n')||!FChar::IsDigit(Night[1]))return false;
 OutKind=KindOfGroup(Rest.Left(Dot));OutNight=FMath::Clamp(FCString::Atoi(*Night.Mid(1)),1,Tables::MaxDay);
 return OutKind!=EKind::None;
}
const TCHAR* Signature(EKind Kind,EType Type)
{
 return Tables::Valid(Kind)&&Tables::Valid(Type)?Tables::Cells[int32(Kind)][int32(Type)].Sign:TEXT("");
}
const TCHAR* Note(EKind Kind,EType Type)
{
 return Tables::Valid(Kind)&&Tables::Valid(Type)?Tables::Cells[int32(Kind)][int32(Type)].Pencil:TEXT("");
}
const TCHAR* TypeLabel(EType Type){return Tables::Valid(Type)?Tables::TypeLabels[int32(Type)]:TEXT("");}
const TCHAR* StageWord(EStage Stage){return Tables::StageWords[FMath::Clamp(int32(Stage),0,3)];}
bool StopsAtMarks(EKind Kind){return Kind==EKind::Silver||Kind==EKind::Shining;}
EStage StageFor(EKind Kind,int32 Types,int32 Nights,bool bLamp,int32 Latest,int32 HabitsFrom)
{
 if(!Tables::Valid(Kind)||Types<1)return EStage::NoFile;
 if(Types>=4&&bLamp&&Nights>=3&&!StopsAtMarks(Kind)&&Latest>=HabitsFrom)return EStage::Habits;
 if(Types>=3&&Nights>=2)return EStage::Marks;
 return EStage::Outline;
}
FString NextWords(EKind Kind,EStage Stage)
{
 using namespace Tables;
 if(Stage==EStage::Outline)return NextOutline;
 if(Stage==EStage::Marks)return StopsAtMarks(Kind)?NextStops:NextMarks;
 if(Stage==EStage::Habits)return NextHabits;
 return FString();
}
FString Stamp(int32 Number,EType Type,bool bPhoto,bool bNewFile,EStage Reached)
{
 using namespace Tables;
 FString Out=FString::Printf(TEXT("UNKNOWN %d / %s"),Number,TypeLabel(Type));
 if(Type==EType::Shape)Out+=FString(TEXT(", "))+FString(bPhoto?HowFilm:HowSeen).ToUpper();
 Out+=TEXT(".");
 if(bNewFile)Out+=StampNew;
 else if(Reached==EStage::Marks||Reached==EStage::Habits)Out+=FString::Printf(TEXT(" The file is at %s now."),StageWord(Reached));
 return Out;
}
FString Gave(EKind Kind,EStage Stage,bool bMapped,int32 Extra,bool bHours)
{
 using namespace Tables;
 if(!Valid(Kind))return FString();
 if(Stage==EStage::Outline)return bMapped?GaveMapped:GaveUnmapped;
 if(Stage==EStage::Marks)return Extra==1?GaveExtraOpen:Extra==2?GaveExtraSpent:GaveListens;
 if(Stage==EStage::Habits)return bHours?GaveHours:GaveNoHours;
 return FString();
}
FString ExtraSlipWords(){return Tables::ExtraSlip;}
FString ExtraToast(){return Tables::ExtraSaid;}
FString ExtraTaskLine(){return Tables::ExtraTask;}
FString HoursLine(int32 Number,const TArray<int32>& Hours)
{
 TArray<int32> In;
 for(const int32 H:Hours)if(H>=0&&H<24)In.AddUnique(H);
 if(In.IsEmpty()||Number<1)return FString();
 In.Sort([](int32 A,int32 B){return (A+16)%24<(B+16)%24;}); // the night's order: from 08:00
 TArray<FString> Said;
 for(const int32 H:In)Said.Add(FString::Printf(TEXT("%02d:00"),H));
 return FString::Printf(TEXT("UNKNOWN %d / Logged so far at %s."),Number,*FString::Join(Said,TEXT(", ")));
}
int32 ExtraNightFor(double Clock)
{
 // FS47FreeformSession::NightOf (08:00 to 08:00), three hours on: from 05:00 the stage gives to the coming night.
 const double At=FMath::IsFinite(Clock)?FMath::Max(0.,Clock):0.;
 return FMath::Max(1,int32(FMath::FloorToDouble((At+3*3600.-8*3600.)/86400.))+1);
}
EKind KindOfOmen(const FString& SourceId,int32 VisitWho)
{
 if(SourceId==TEXT("visit"))return KindOfVisit(VisitWho);
 if(SourceId.StartsWith(TEXT("body:"),ESearchCase::CaseSensitive)||SourceId==TEXT("door")||SourceId==TEXT("peek"))return EKind::Small;
 return EKind::None;
}
int32 AlienKindOf(EKind Kind)
{
 switch(Kind)
 {
  case EKind::Small:return int32(ES47AlienKind::Roswell);
  case EKind::Low:return int32(ES47AlienKind::Pale);
  case EKind::Tall:return int32(ES47AlienKind::Tall);
  case EKind::Silver:return int32(ES47AlienKind::Nordic);
  case EKind::Shining:return int32(ES47AlienKind::Cosmic);
  default:return -1;
 }
}
float GeigerFactor(EKind Kind)
{
 // The table's GEIGER column: "faint", "none", "clear"; the far two have nothing in it.
 switch(Kind)
 {
  case EKind::Small:return .55f;
  case EKind::Low:return 0.f;
  case EKind::Tall:return 1.6f;
  case EKind::Silver:case EKind::Shining:return 0.f;
  default:return 1.f;
 }
}
int32 Earliest(EKind Kind,EStage Stage,const FGates& G,int32 Last)
{
 // The best case: every source that may come on a night does, and all of it is tied into one file. Then the first night the file
 // can stand at Stage. The near three are met on a visit (one a night, the night's one danger): what it shows, the tape, the
 // animals before it, the counter and the prints by daylight; the lamp's answer once (the low one's only from RuleBreak, and after
 // the small ones' rule was learned on a visit before). The far two: a photograph from Sky, the receiver's take from Take, the
 // lamp from Figure; nothing else.
 if(!Tables::Valid(Kind)||Stage==EStage::NoFile)return 0;
 const bool bFar=StopsAtMarks(Kind);
 TSet<int32> Types;int32 Nights=0;
 for(int32 Night=1;Night<=Last;++Night)
 {
  TArray<EType> Got;
  if(bFar)
  {
   if(Night>=G.Sky)Got.Add(EType::Shape);
   if(Night>=G.Take)Got.Add(EType::Tape);
   if(Night>=G.Figure)Got.Add(EType::Lamp);
  }
  else
  {
   if(Night>=G.Visit)for(const EType T:{EType::Shape,EType::Tape,EType::Ground,EType::Geiger,EType::Animals})Got.Add(T);
   if((Kind==EKind::Low||Kind==EKind::Tall)&&Night>=G.Glimpse){Got.AddUnique(EType::Shape);Got.AddUnique(EType::Ground);}
   const int32 LampFrom=Kind==EKind::Low?FMath::Max(G.RuleBreak,G.Visit+1):G.Visit;
   if(Night>=LampFrom)Got.Add(EType::Lamp);
  }
  if(Got.IsEmpty())continue;
  ++Nights;
  for(const EType T:Got)Types.Add(int32(T));
  if(int32(StageFor(Kind,Types.Num(),Nights,Types.Contains(int32(EType::Lamp)),Night,G.HabitsFrom))>=int32(Stage))return Night;
 }
 return 0;
}
TArray<int32> Agreement(const FFile& A,const FFile& B)
{
 TArray<int32> Out;
 for(int32 T=0;T<Tables::TypeN;++T)
 {
  const bool bA=A.Boxes.IsValidIndex(T)&&A.Boxes[T].bFilled,bB=B.Boxes.IsValidIndex(T)&&B.Boxes[T].bFilled;
  Out.Add(!bA||!bB?0:A.Boxes[T].Sign==B.Boxes[T].Sign?1:2);
 }
 return Out;
}
FString BesideWords(const FFile& A,const FFile& B)
{
 const TArray<int32> Marks=Agreement(A,B);
 TArray<FString> Alike;
 for(int32 T=0;T<Marks.Num();++T)if(Marks[T]==1)Alike.Add(FString(Tables::TypeLabels[T]).ToLower());
 FString Out=A.Title+TEXT(" and ")+B.Title+TEXT(" side by side.");
 if(!Alike.IsEmpty())Out+=TEXT(" The same in: ")+FString::Join(Alike,TEXT(", "))+TEXT(".");
 return Out;
}
FString BesideLine(int32 Number){return FString::Printf(TEXT("KEPT BESIDE UNKNOWN %d."),Number);}
FString Sheet(const FFile& File,const TArray<FString>& PhotoLines)
{
 FString Out=FString::Printf(TEXT("STAGE     %s\n"),*File.Stage);
 if(!File.Gave.IsEmpty())Out+=TEXT("          ")+File.Gave+TEXT("\n"); // LOOP-DOSSIER-GIVES-20261113: what the stage gave, under it
 Out+=FString::Printf(TEXT("NIGHTS    %d / FIRST DAY %02d / LAST DAY %02d\n"),File.Nights,File.Opened,File.Latest);
 for(const FBox& One:File.Boxes)
 {
  const FString Label=One.Label+FString::ChrN(FMath::Max(1,10-One.Label.Len()),TEXT(' '));
  Out+=TEXT("\n")+Label+(One.bFilled?FString::Printf(TEXT("%s / DAY %02d"),*One.How.ToUpper(),One.Day):FString(TEXT("NOTHING KEPT")));
 }
 Out+=TEXT("\n\nPHOTOGRAPHS");
 if(PhotoLines.IsEmpty())Out+=TEXT("\nNONE IN THE ALBUM.");
 for(const FString& Line:PhotoLines)Out+=TEXT("\n")+Line;
 bool bAny=false;
 for(const FBox& One:File.Boxes)
 {
  if(!One.bFilled||One.Note.IsEmpty())continue;
  if(!bAny){Out+=TEXT("\n\nIN PENCIL");bAny=true;}
  for(const FString& Was:One.Struck)Out+=TEXT("\n")+FString(Tables::StruckTyped)+Was; // slice 2: what a later find contradicted
  Out+=TEXT("\n")+One.Note;
 }
 if(!File.Beside.IsEmpty())Out+=TEXT("\n");
 for(const int32 Other:File.Beside)Out+=TEXT("\n")+BesideLine(Other);
 if(!File.Next.IsEmpty())Out+=TEXT("\n\n")+File.Next;
 Out+=TEXT("\n\nREAD ONLY. KEPT AS THE NOTEBOOK HAS IT.");
 return Out;
}
TArray<FString> Words()
{
 using namespace Tables;
 TArray<FString> Out;
 for(int32 T=0;T<TypeN;++T)Out.Add(TypeLabels[T]);
 for(int32 S=1;S<4;++S)Out.Add(StageWords[S]);
 for(int32 K=0;K<KindN;++K)for(int32 T=0;T<TypeN;++T)if(*Cells[K][T].Pencil)Out.Add(Cells[K][T].Pencil);
 for(const TCHAR* W:{HowFilm,HowSeen,HowNoted,NextOutline,NextMarks,NextStops,NextHabits,StampNew})Out.Add(W);
 for(const TCHAR* W:{GaveMapped,GaveUnmapped,GaveExtraOpen,GaveExtraSpent,GaveListens,GaveHours,GaveNoHours,ExtraSlip,ExtraSaid,ExtraTask})Out.Add(W);
 {TArray<int32> Every;for(int32 H=0;H<24;++H)Every.Add(H);Out.Add(HoursLine(5,Every));} // every hour there is, as tonight's note has them
 for(const TCHAR* W:SheetWords)Out.Add(W);
 // slice 2: the pair's line on the sheet, every way it can be read aloud, a struck note
 for(int32 N=1;N<=MaxFiles;++N)Out.Add(BesideLine(N));
 {
  FFile A,B;A.Title=TEXT("UNKNOWN 1");B.Title=FString::Printf(TEXT("UNKNOWN %d"),MaxFiles);
  A.Boxes.SetNum(TypeN);B.Boxes.SetNum(TypeN);
  for(int32 T=0;T<TypeN;++T){A.Boxes[T].bFilled=B.Boxes[T].bFilled=true;A.Boxes[T].Sign=B.Boxes[T].Sign=TEXT("x");}
  Out.Add(BesideWords(A,B));
 }
 Out.Add(FString(StruckTyped)+Cells[0][0].Pencil);
 // every stamp there can be, and a file's title (slice 2: as many files as there can be)
 for(int32 N=1;N<=MaxFiles;++N)
 {
  Out.Add(FString::Printf(TEXT("UNKNOWN %d"),N));
  for(int32 T=0;T<TypeN;++T)for(const bool bPhoto:{false,true})
  {
   Out.Add(Stamp(N,EType(T),bPhoto,true,EStage::Outline));
   Out.Add(Stamp(N,EType(T),bPhoto,false,EStage::NoFile));
   Out.Add(Stamp(N,EType(T),bPhoto,false,EStage::Marks));
   Out.Add(Stamp(N,EType(T),bPhoto,false,EStage::Habits));
  }
 }
 return Out;
}
FString Lint(){return LintWords(Words());}
bool GeigerReads(bool bOut,double Metres,double HeldFor){return bOut&&FMath::IsFinite(Metres)&&Metres<=GeigerNearM&&HeldFor>=GeigerHoldS;}
bool TakeHeard(double Cursor){return FMath::IsFinite(Cursor)&&Cursor>=S47AlienKinds::SignalFrom+S47AlienKinds::SignalSeconds*.5;}
FString LintWords(const TArray<FString>& Said)
{
 // Freeform's banned words (S47Thoughts, S47FreeformCases), the working names that are no plain description, and the word the
 // old photograph receipts used.
 static const TCHAR* const Not[]={TEXT("1947"),TEXT("47 s"),TEXT("forty"),TEXT("cable"),TEXT("tomas"),TEXT("taken"),TEXT("silhouette"),TEXT("8.15"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),
  TEXT("martian"),TEXT("third"),TEXT("grey"),TEXT("gray"),TEXT("abduct"),TEXT("roswell"),TEXT("nordic"),TEXT("cosmic"),TEXT("crawler"),TEXT("visitor"),TEXT("species"),TEXT("creature")};
 FString Bad;
 for(const FString& W:Said)
 {
  if(const FString Why=S47Script::LintText(W);!Why.IsEmpty())Bad+=FString::Printf(TEXT("'%s': %s | "),*W.Left(40),*Why);
  if(S47Marginalia::NamesKind(W))Bad+=FString::Printf(TEXT("'%s': names what it is | "),*W.Left(40));
  const FString Low=W.ToLower();
  for(const TCHAR* N:Not)if(Low.Contains(N))Bad+=FString::Printf(TEXT("'%s': says '%s' | "),*W.Left(40),N);
  if(W.Contains(TEXT("47")))Bad+=FString::Printf(TEXT("'%s': prints 47 | "),*W.Left(40));
  // a standalone capital C (Freeform's list): a C with no letter on either side
  for(int32 I=0;I<W.Len();++I)if(W[I]==TEXT('C')&&(I==0||!FChar::IsAlpha(W[I-1]))&&(I+1>=W.Len()||!FChar::IsAlpha(W[I+1])))Bad+=FString::Printf(TEXT("'%s': a C on its own | "),*W.Left(40));
 }
 return Bad;
}
}

// The class is at global scope: everything of the namespace is named in full here, so that nothing is looked up among whatever
// else a unity build has put in the same file.
bool FS47Dossier::Tag(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Day,int32 Hour,const FVector2D* Where)
{
 namespace Tb=S47Dossier::Tables;
 if(Key.IsEmpty()||Key.Len()>Tb::MaxKey||!Tb::Valid(Kind)||!Tb::Valid(Type)||!*S47Dossier::Signature(Kind,Type)||Kept.Num()>=Tb::MaxTags||HasTag(Key))return false;
 FTag One;One.Key=Key;One.Kind=Kind;One.Type=Type;One.Day=FMath::Clamp(Day,1,Tb::MaxDay);
 One.Hour=Hour>=0&&Hour<24?Hour:-1; // LOOP-DOSSIER-GIVES-20261113
 if(Where&&FMath::IsFinite(Where->X)&&FMath::IsFinite(Where->Y)&&FMath::Abs(Where->X)<Tb::MaxWhere&&FMath::Abs(Where->Y)<Tb::MaxWhere){One.bWhere=true;One.X=Where->X;One.Z=Where->Y;}
 // Slice 2: the files it ties to, as the player could tell: the same thing already in that box, or something got together with it.
 const FString Sign=S47Dossier::Signature(Kind,Type);
 TArray<int32> Ties;
 for(int32 File=0;File<Sequence.Num();++File)
 {
  bool bTies=BoxSign(File,Type)==Sign;
  for(const FTag& Had:Kept)if(!bTies&&Had.File==File&&Tb::Together(Had,One))bTies=true;
  if(bTies)Ties.Add(File);
 }
 // One find that ties two files shows they are the same thing: kept side by side, never merged.
 for(int32 At=1;At<Ties.Num();++At)Link(Ties[0],Ties[At]);
 return Put(MoveTemp(One),Ties.IsEmpty()?INDEX_NONE:Ties[0]);
}
bool FS47Dossier::Put(FTag&& One,int32 File)
{
 if(File==INDEX_NONE)
 {
  if(Sequence.Num()>=S47Dossier::Tables::MaxFiles)return false;
  File=Sequence.Add(One.Kind); // it opens a file: the next number
 }
 if(!Sequence.IsValidIndex(File))return false;
 One.File=File;
 Kept.Add(MoveTemp(One));
 return true;
}
void FS47Dossier::Link(int32 A,int32 B)
{
 if(A==B||!Sequence.IsValidIndex(A)||!Sequence.IsValidIndex(B))return;
 Same.AddUnique(FIntPoint(FMath::Min(A,B)+1,FMath::Max(A,B)+1));
}
FString FS47Dossier::BoxSign(int32 File,S47Dossier::EType Type) const
{
 // The latest in that box: an earlier one a later find contradicted is struck (Files)
 FString Out;
 for(const FTag& One:Kept)if(One.File==File&&One.Type==Type)Out=S47Dossier::Signature(One.Kind,Type);
 return Out;
}
S47Dossier::EStage FS47Dossier::StageIn(int32 File) const
{
 if(!Sequence.IsValidIndex(File))return S47Dossier::EStage::NoFile;
 TSet<int32> Types,Nights;int32 Latest=0;bool bFar=false;
 for(const FTag& One:Kept)if(One.File==File){Types.Add(int32(One.Type));Nights.Add(One.Day);Latest=FMath::Max(Latest,One.Day);bFar|=S47Dossier::StopsAtMarks(One.Kind);}
 // A file with anything of the far two in it stops at MARKS with them, whichever opened it.
 const S47Dossier::EKind Kind=bFar?S47Dossier::EKind::Silver:Sequence[File];
 return S47Dossier::StageFor(Kind,Types.Num(),Nights.Num(),Types.Contains(int32(S47Dossier::EType::Lamp)),Latest,HabitsFrom);
}
bool FS47Dossier::HasTag(const FString& Key) const
{
 return Kept.ContainsByPredicate([&Key](const FTag& One){return One.Key==Key;});
}
int32 FS47Dossier::NumberOf(S47Dossier::EKind Kind) const{return FirstFileOf(Kind)+1;}
int32 FS47Dossier::FileOf(const FString& Key) const
{
 const FTag* One=Kept.FindByPredicate([&Key](const FTag& Had){return Had.Key==Key;});
 return One?One->File+1:0;
}
int32 FS47Dossier::FilesOf(S47Dossier::EKind Kind) const
{
 int32 Count=0;
 for(const S47Dossier::EKind One:Sequence)Count+=One==Kind?1:0;
 return Count;
}
S47Dossier::EStage FS47Dossier::StageOfFile(int32 Number) const{return StageIn(Number-1);}
bool FS47Dossier::Has(S47Dossier::EKind Kind,S47Dossier::EType Type) const
{
 return Kept.ContainsByPredicate([Kind,Type](const FTag& One){return One.Kind==Kind&&One.Type==Type;});
}
int32 FS47Dossier::TypesOf(S47Dossier::EKind Kind) const
{
 int32 Count=0;
 for(int32 T=0;T<S47Dossier::Tables::TypeN;++T)Count+=Has(Kind,S47Dossier::EType(T))?1:0;
 return Count;
}
int32 FS47Dossier::NightsOf(S47Dossier::EKind Kind) const
{
 TSet<int32> Nights;
 for(const FTag& One:Kept)if(One.Kind==Kind)Nights.Add(One.Day);
 return Nights.Num();
}
S47Dossier::EStage FS47Dossier::StageOf(S47Dossier::EKind Kind) const
{
 int32 Latest=0;
 for(const FTag& One:Kept)if(One.Kind==Kind)Latest=FMath::Max(Latest,One.Day);
 return S47Dossier::StageFor(Kind,TypesOf(Kind),NightsOf(Kind),Has(Kind,S47Dossier::EType::Lamp),Latest,HabitsFrom);
}
int32 FS47Dossier::AtMarks() const
{
 int32 Count=0;
 for(int32 File=0;File<Sequence.Num();++File)Count+=int32(StageIn(File))>=int32(S47Dossier::EStage::Marks)?1:0;
 return Count;
}
S47Dossier::EFit FS47Dossier::Fit(S47Dossier::EKind File,S47Dossier::EType Type,S47Dossier::EKind FindKind) const
{
 // Only what the player has put in that box counts: an empty box answers nothing, whatever the table knows.
 if(!Has(File,Type))return S47Dossier::EFit::Unknown;
 return FCString::Strcmp(S47Dossier::Signature(File,Type),S47Dossier::Signature(FindKind,Type))==0?S47Dossier::EFit::Fits:S47Dossier::EFit::Contradicts;
}
TArray<S47Dossier::FFile> FS47Dossier::Files() const
{
 namespace Tb=S47Dossier::Tables;
 TArray<S47Dossier::FFile> Out;
 for(int32 At=0;At<Sequence.Num();++At)
 {
  const S47Dossier::EKind Kind=Sequence[At];const S47Dossier::EStage Stage=StageIn(At);
  bool bFar=false;
  for(const FTag& One:Kept)bFar|=One.File==At&&S47Dossier::StopsAtMarks(One.Kind);
  S47Dossier::FFile File;File.Number=At+1;File.Title=FString::Printf(TEXT("UNKNOWN %d"),At+1);
  File.Stage=S47Dossier::StageWord(Stage);File.Next=S47Dossier::NextWords(bFar?S47Dossier::EKind::Silver:Kind,Stage);
  File.bAtMarks=int32(Stage)>=int32(S47Dossier::EStage::Marks);
  {
   // The extra bearing is the kind's, once: its first file says what became of it; another of its files only that it listens.
   double WhereX=0,WhereZ=0;
   const int32 Extra=FirstFileOf(Kind)==At?ExtraState(Kind):0;
   File.Gave=S47Dossier::Gave(Kind,Stage,FirstWhereOfFile(At+1,WhereX,WhereZ),Extra,!HoursOfFile(At+1).IsEmpty());
  }
  File.Boxes.SetNum(Tb::TypeN);
  for(int32 T=0;T<Tb::TypeN;++T)File.Boxes[T].Label=Tb::TypeLabels[T];
  TSet<int32> Nights;
  for(const FTag& One:Kept)
  {
   if(One.File!=At)continue;
   Nights.Add(One.Day);
   S47Dossier::FBox& Slot=File.Boxes[int32(One.Type)];
   const bool bShape=One.Type==S47Dossier::EType::Shape;
   const FString Sign=S47Dossier::Signature(One.Kind,One.Type);
   if(!Slot.bFilled){Slot.bFilled=true;Slot.Day=One.Day;Slot.Sign=Sign;Slot.Note=S47Dossier::Note(One.Kind,One.Type);Slot.How=bShape?Tb::HowSeen:Tb::HowNoted;}
   else
   {
    Slot.Day=FMath::Min(Slot.Day,One.Day);
    // Slice 2: a later find that is not what the box held: the old pencil is struck through, the new one stands
    if(Sign!=Slot.Sign){Slot.Struck.Remove(Slot.Note);Slot.Struck.Add(Slot.Note);Slot.Struck.Remove(S47Dossier::Note(One.Kind,One.Type));Slot.Sign=Sign;Slot.Note=S47Dossier::Note(One.Kind,One.Type);}
   }
   if(bShape&&S47Dossier::PhotoKey(One.Key)){Slot.How=Tb::HowFilm;File.Photos.Add(One.Key);}
   File.Opened=File.Opened==0?One.Day:FMath::Min(File.Opened,One.Day);File.Latest=FMath::Max(File.Latest,One.Day);
  }
  File.Nights=Nights.Num();
  for(const FIntPoint& Pair:Same)
  {
   if(Pair.X==At+1)File.Beside.Add(Pair.Y);
   else if(Pair.Y==At+1)File.Beside.Add(Pair.X);
  }
  File.Beside.Sort();
  Out.Add(MoveTemp(File));
 }
 return Out;
}
bool FS47Dossier::FirstWhere(S47Dossier::EKind Kind,double& OutX,double& OutZ) const
{
 const FTag* Best=nullptr;
 for(const FTag& One:Kept)if(One.Kind==Kind&&One.bWhere&&(!Best||One.Day<Best->Day))Best=&One; // by day, then as logged
 if(!Best)return false;
 OutX=Best->X;OutZ=Best->Z;
 return true;
}
TArray<int32> FS47Dossier::HoursOf(S47Dossier::EKind Kind) const
{
 TArray<int32> Out;
 for(const FTag& One:Kept)if(One.Kind==Kind&&One.Hour>=0)Out.AddUnique(One.Hour);
 Out.Sort([](int32 A,int32 B){return (A+16)%24<(B+16)%24;});
 return Out;
}
bool FS47Dossier::FirstWhereOfFile(int32 Number,double& OutX,double& OutZ) const
{
 const FTag* Best=nullptr;
 for(const FTag& One:Kept)if(One.File==Number-1&&One.bWhere&&(!Best||One.Day<Best->Day))Best=&One;
 if(!Best)return false;
 OutX=Best->X;OutZ=Best->Z;
 return true;
}
TArray<int32> FS47Dossier::HoursOfFile(int32 Number) const
{
 TArray<int32> Out;
 for(const FTag& One:Kept)if(One.File==Number-1&&One.Hour>=0)Out.AddUnique(One.Hour);
 Out.Sort([](int32 A,int32 B){return (A+16)%24<(B+16)%24;});
 return Out;
}
bool FS47Dossier::Grant(S47Dossier::EKind Kind,int32 Night)
{
 if(!S47Dossier::Tables::Valid(Kind)||Given.Contains(int32(Kind)))return false;
 Given.Add(int32(Kind),FMath::Clamp(Night,1,S47Dossier::Tables::MaxDay));
 return true;
}
bool FS47Dossier::ExtraOpen(int32 Night) const
{
 for(const TPair<int32,int32>& One:Given)if(One.Value<=Night&&!Spent.Contains(One.Key))return true;
 return false;
}
bool FS47Dossier::Spend(int32 Night)
{
 // The open one of the file with the lowest number, so that it comes out the same every time.
 for(const S47Dossier::EKind Kind:Sequence)
 {
  const int32* For=Given.Find(int32(Kind));
  if(For&&*For<=Night&&!Spent.Contains(int32(Kind))){Spent.Add(int32(Kind));return true;}
 }
 return false;
}
int32 FS47Dossier::ExtraState(S47Dossier::EKind Kind) const
{
 if(!Given.Contains(int32(Kind)))return 0;
 return Spent.Contains(int32(Kind))?2:1;
}
int32 FS47Dossier::GrantMissing(int32 Night)
{
 int32 Made=0;
 for(int32 File=0;File<Sequence.Num();++File)if(int32(StageIn(File))>=int32(S47Dossier::EStage::Marks)&&Grant(Sequence[File],Night))++Made;
 return Made;
}
S47J::FObj FS47Dossier::Save() const
{
 const S47J::FObj Out=S47J::New(),TagsOut=S47J::New();
 // "order": each kind once, where it first opened a file (as slice 1 wrote it). Slice 2: "files" only once a kind has opened more.
 TArray<FString> Ids,All;bool bMore=false;
 for(const S47Dossier::EKind Kind:Sequence){const FString Id=S47Dossier::KindId(Kind);bMore|=Ids.Contains(Id);Ids.AddUnique(Id);All.Add(Id);}
 S47J::Set(Out,TEXT("order"),Ids);
 if(bMore)S47J::Set(Out,TEXT("files"),All);
 for(const FTag& One:Kept)
 {
  const S47J::FObj Row=S47J::New();
  S47J::Set(Row,TEXT("k"),S47Dossier::KindId(One.Kind));S47J::Set(Row,TEXT("t"),S47Dossier::TypeId(One.Type));S47J::Set(Row,TEXT("d"),One.Day);
  if(One.Hour>=0)S47J::Set(Row,TEXT("h"),One.Hour); // LOOP-DOSSIER-GIVES-20261113: optional, as the day is
  if(One.bWhere){S47J::Set(Row,TEXT("x"),FMath::RoundToDouble(One.X*10)/10);S47J::Set(Row,TEXT("z"),FMath::RoundToDouble(One.Z*10)/10);}
  if(One.File!=FirstFileOf(One.Kind))S47J::Set(Row,TEXT("f"),One.File+1); // slice 2: not where a reader of "order" would put it
  S47J::Set(Row,TEXT("n"),int32(&One-Kept.GetData())); // slice 2: the order it was logged in (what a later find struck is by it)
  S47J::Set(TagsOut,*One.Key,Row);
 }
 S47J::Set(Out,TEXT("tags"),TagsOut);
 if(!Same.IsEmpty())
 {
  TArray<double> Flat;
  for(const FIntPoint& Pair:Same){Flat.Add(Pair.X);Flat.Add(Pair.Y);}
  S47J::Set(Out,TEXT("same"),Flat);
 }
 if(!Given.IsEmpty())
 {
  const S47J::FObj ExtraOut=S47J::New();TArray<FString> SpentIds;
  TArray<S47Dossier::EKind> Once;
  for(const S47Dossier::EKind Kind:Sequence)
  {
   if(Once.Contains(Kind))continue;
   Once.Add(Kind);
   if(const int32* For=Given.Find(int32(Kind)))S47J::Set(ExtraOut,S47Dossier::KindId(Kind),*For);
   if(Spent.Contains(int32(Kind)))SpentIds.Add(S47Dossier::KindId(Kind));
  }
  S47J::Set(Out,TEXT("extra"),ExtraOut);
  if(!SpentIds.IsEmpty())S47J::Set(Out,TEXT("spent"),SpentIds);
 }
 return Out;
}
void FS47Dossier::Load(const S47J::FObj& Saved)
{
 namespace Tb=S47Dossier::Tables;
 const int32 KeepHabits=HabitsFrom;
 Reset();HabitsFrom=KeepHabits;
 if(!Saved.IsValid())return;
 // Slice 2: "files" when a kind opened more than one, in number order; else "order", each kind once, as slice 1 had it.
 if(Saved->HasField(TEXT("files")))
 {
  for(const FString& Id:S47J::Strs(Saved,TEXT("files")))
  {
   const S47Dossier::EKind Kind=S47Dossier::KindOfId(Id);
   if(Tb::Valid(Kind)&&Sequence.Num()<Tb::MaxFiles)Sequence.Add(Kind);
  }
 }
 else for(const FString& Id:S47J::Strs(Saved,TEXT("order")))
 {
  const S47Dossier::EKind Kind=S47Dossier::KindOfId(Id);
  if(Tb::Valid(Kind))Sequence.AddUnique(Kind);
 }
 TArray<FTag> Read;TMap<FString,int32> LoggedAt;
 if(const S47J::FObj TagsIn=S47J::Obj(Saved,TEXT("tags"));TagsIn.IsValid())for(const auto& Pair:TagsIn->Values)
 {
  const S47J::FObj* Row=nullptr;
  if(!Pair.Value.IsValid()||!Pair.Value->TryGetObject(Row)||!Row||!Row->IsValid())continue;
  const double OnDay=S47J::Num(*Row,TEXT("d"));
  if(!FMath::IsFinite(OnDay)||OnDay<1||OnDay>Tb::MaxDay)continue;
  FTag One;One.Key=FString(*Pair.Key);One.Kind=S47Dossier::KindOfId(S47J::Str(*Row,TEXT("k")));One.Type=S47Dossier::TypeOfId(S47J::Str(*Row,TEXT("t")));One.Day=int32(OnDay);
  // LOOP-DOSSIER-GIVES-20261113: the hour and the place, when the tag has them (a save from before has neither)
  const double OnHour=S47J::Num(*Row,TEXT("h"),-1);
  One.Hour=FMath::IsFinite(OnHour)&&OnHour>=0&&OnHour<24?int32(OnHour):-1;
  if(S47J::Has(*Row,TEXT("x"))&&S47J::Has(*Row,TEXT("z"))){One.X=S47J::Num(*Row,TEXT("x"),1e99);One.Z=S47J::Num(*Row,TEXT("z"),1e99);One.bWhere=true;}
  // slice 2: its file, when it is not the first of its own kind's; -1: that one
  const double InFile=S47J::Num(*Row,TEXT("f"),0);
  One.File=FMath::IsFinite(InFile)&&InFile>=1&&InFile<=Sequence.Num()?int32(InFile)-1:-1;
  const double Logged=S47J::Num(*Row,TEXT("n"),-1);
  LoggedAt.Add(One.Key,FMath::IsFinite(Logged)&&Logged>=0&&Logged<Tb::MaxTags?int32(Logged):-1);
  Read.Add(MoveTemp(One));
 }
 // REVIEW (dossier slice 2, 14 Nov): as logged, where the record kept it ("n", which every slice 2 save writes for every tag): a tape
 // taped on night 3 and played back on night 5 is logged after night 5's finds, and what it struck, the pencil that stands and so
 // the file a later find joins depend on that order. A save without it (slice 1): by day, then key. Either way the same every time,
 // whatever order the record's fields come in. What is not sound is left out.
 bool bLogged=!Read.IsEmpty();
 for(const FTag& One:Read)bLogged&=LoggedAt.FindRef(One.Key)>=0;
 Read.Sort([&LoggedAt,bLogged](const FTag& A,const FTag& B)
 {
  if(bLogged){const int32 NA=LoggedAt.FindRef(A.Key),NB=LoggedAt.FindRef(B.Key);if(NA!=NB)return NA<NB;}
  else if(A.Day!=B.Day)return A.Day<B.Day;
  return A.Key<B.Key;
 });
 for(FTag& One:Read)
 {
  if(One.Key.IsEmpty()||One.Key.Len()>Tb::MaxKey||!Tb::Valid(One.Kind)||!Tb::Valid(One.Type)||!*S47Dossier::Signature(One.Kind,One.Type)||Kept.Num()>=Tb::MaxTags||HasTag(One.Key))continue;
  if(One.bWhere&&!(FMath::IsFinite(One.X)&&FMath::IsFinite(One.Z)&&FMath::Abs(One.X)<Tb::MaxWhere&&FMath::Abs(One.Z)<Tb::MaxWhere)){One.bWhere=false;One.X=One.Z=0;}
  // a tag whose kind has no file gets the next number
  const int32 Into=One.File>=0?One.File:FirstFileOf(One.Kind);
  Put(MoveTemp(One),Into);
 }
 // slice 2: the pairs kept side by side
 const TArray<double> Flat=S47J::Nums(Saved,TEXT("same"));
 for(int32 At=0;At+1<Flat.Num();At+=2)
  if(FMath::IsFinite(Flat[At])&&FMath::IsFinite(Flat[At+1])&&Flat[At]>=1&&Flat[At+1]>=1&&Flat[At]<=Sequence.Num()&&Flat[At+1]<=Sequence.Num())Link(int32(Flat[At])-1,int32(Flat[At+1])-1);
 // MARKS's extra bearing, for a file there is: the night it is for, and whether it is printed
 if(const S47J::FObj ExtraIn=S47J::Obj(Saved,TEXT("extra"));ExtraIn.IsValid())for(const auto& Pair:ExtraIn->Values)
 {
  const S47Dossier::EKind Kind=S47Dossier::KindOfId(FString(*Pair.Key));double For=0;
  if(Tb::Valid(Kind)&&Sequence.Contains(Kind)&&Pair.Value.IsValid()&&Pair.Value->TryGetNumber(For)&&FMath::IsFinite(For)&&For>=1&&For<=Tb::MaxDay)Grant(Kind,int32(For));
 }
 for(const FString& Id:S47J::Strs(Saved,TEXT("spent")))
 {
  const S47Dossier::EKind Kind=S47Dossier::KindOfId(Id);
  if(Tb::Valid(Kind)&&Given.Contains(int32(Kind)))Spent.Add(int32(Kind));
 }
}
int32 FS47Dossier::Rebuild(const TArray<FOld>& Findings,const TArray<FString>& Claims)
{
 TArray<FTag> Found;
 const auto Add=[&Found](const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Day)
 {
  if(!S47Dossier::Tables::Valid(Kind)||Found.ContainsByPredicate([&Key](const FTag& Have){return Have.Key==Key;}))return;
  FTag One;One.Key=Key;One.Kind=Kind;One.Type=Type;One.Day=FMath::Max(1,Day);Found.Add(MoveTemp(One));
 };
 static const FString Receipt=TEXT("photo-registration.");
 S47Dossier::EKind Which=S47Dossier::EKind::None;int32 Night=0;
 for(const FString& Id:Claims)if(S47Dossier::ClaimKind(Id,Which,Night))Add(Id,Which,S47Dossier::EType::Shape,Night);
 for(const FOld& Old:Findings)
 {
  if(Old.Key.StartsWith(Receipt,ESearchCase::CaseSensitive))
  {
   // a photograph registered at OPS: the same tag as its claim, under the claim's id
   const FString Id=Old.Key.Mid(Receipt.Len());
   if(S47Dossier::ClaimKind(Id,Which,Night))Add(Id,Which,S47Dossier::EType::Shape,Night);
  }
  else if(Old.Key==TEXT("rule-still-held"))Add(Old.Key,S47Dossier::EKind::Small,S47Dossier::EType::Lamp,Old.Day);
  else if(Old.Key==TEXT("rule-tall-froze"))Add(Old.Key,S47Dossier::EKind::Tall,S47Dossier::EType::Lamp,Old.Day); // slice 2
  else if(Old.Key.StartsWith(TEXT("road_tracks_d"),ESearchCase::CaseSensitive))
  {
   // by the record's own words (S47RoadGlimpse::TracksRecord), as S47Marginalia reads it
   if(Old.Text.Contains(TEXT("prints in fours")))Add(Old.Key,S47Dossier::EKind::Low,S47Dossier::EType::Ground,Old.Day);
   else if(Old.Text.Contains(TEXT("long narrow prints")))Add(Old.Key,S47Dossier::EKind::Tall,S47Dossier::EType::Ground,Old.Day);
  }
  // "rule-pale-broken" (the low one or the tall one) and "<place>_visit" (any of three) do not say which one it was: no tag.
 }
 Found.Sort([](const FTag& A,const FTag& B){return S47Dossier::Tables::Earlier(A,B);});
 int32 Made=0;
 for(FTag& One:Found)
 {
  if(HasTag(One.Key)||!*S47Dossier::Signature(One.Kind,One.Type))continue;
  const int32 Into=FirstFileOf(One.Kind); // each kind into one file, as slice 1 had it
  Made+=Put(MoveTemp(One),Into)?1:0;
 }
 return Made;
}

namespace S47Dossier
{
bool SelfTest(TArray<FString>& Lines)
{
 using namespace Tables;
 bool bOk=true;
 const auto Check=[&bOk,&Lines](bool bGood,const TCHAR* Name){bOk&=bGood;Lines.Add(FString(bGood?TEXT("PASS "):TEXT("FAIL "))+TEXT("EVIDENCE-DOSSIER: ")+Name);};
 const EKind Small=EKind::Small,Low=EKind::Low,Tall=EKind::Tall,Silver=EKind::Silver,Shining=EKind::Shining;
 const EType Shape=EType::Shape,Tape=EType::Tape,Ground=EType::Ground,Lamp=EType::Lamp,Geiger=EType::Geiger,Animals=EType::Animals;
 { // the words
  const FString Why=Lint();
  Check(Why.IsEmpty(),TEXT("no dossier text says what Freeform must not, names what they are, or fails S47Script::LintText"));
  if(!Why.IsEmpty())Lines.Add(TEXT("     ")+Why);
  // The stamp is the file's number and the kind of evidence: it cannot say which one it is, and no pencil note is in it.
  bool bPlain=true;
  for(int32 N=1;N<=KindN;++N)for(int32 T=0;T<TypeN;++T)for(const bool bPhoto:{false,true})
  {
   const FString Said=Stamp(N,EType(T),bPhoto,N==1,EStage::Marks);
   bPlain&=Said.StartsWith(FString::Printf(TEXT("UNKNOWN %d / %s"),N,TypeLabels[T]))&&!S47Marginalia::NamesKind(Said);
   for(int32 K=0;K<KindN;++K)
   {
    bPlain&=!Said.ToLower().Contains(FString::Printf(TEXT(" %s"),KindIds[K]));
    for(int32 U=0;U<TypeN;++U)if(*Cells[K][U].Pencil)bPlain&=!Said.Contains(Cells[K][U].Pencil);
   }
  }
  Check(bPlain&&Stamp(2,Ground,false,false,EStage::NoFile)==TEXT("UNKNOWN 2 / GROUND.")&&Stamp(1,Shape,true,true,EStage::Outline)==TEXT("UNKNOWN 1 / SHAPE, ON FILM. New file in the notebook.")
   &&Stamp(3,Lamp,false,false,EStage::Marks)==TEXT("UNKNOWN 3 / LAMP. The file is at MARKS now."),
   TEXT("the stamp is the file's number and the kind of evidence, never a working name or what was found"));
 }
 { // the table
  bool bRows=true,bNear=true,bFar=true;
  const auto Row=[](int32 K){FString Out;for(int32 T=0;T<TypeN;++T)Out+=FString(Cells[K][T].Sign)+TEXT("|");return Out;};
  for(int32 A=0;A<KindN;++A)for(int32 B=A+1;B<KindN;++B)bRows&=Row(A)!=Row(B);
  for(int32 A=0;A<3;++A)for(int32 B=A+1;B<3;++B)for(int32 T=0;T<TypeN;++T)
   bNear&=*Cells[A][T].Sign&&*Cells[B][T].Sign&&FCString::Strcmp(Cells[A][T].Sign,Cells[B][T].Sign)!=0;
  for(int32 T=0;T<TypeN;++T)
  {
   const bool bGives=T==int32(Shape)||T==int32(Tape)||T==int32(Lamp);
   for(const EKind Far:{Silver,Shining})bFar&=(*Signature(Far,EType(T))!=0)==bGives&&(*Note(Far,EType(T))!=0)==bGives;
   if(bGives)bFar&=FCString::Strcmp(Signature(Silver,EType(T)),Signature(Shining,EType(T)))!=0;
  }
  Check(bRows&&bNear&&bFar,TEXT("no two of them have the same signature: the three that come near differ in every box, the far two share only their empty ones"));
 }
 { // which one a source knows it was
  EKind Got=EKind::None;int32 Night=0;
  const bool bLow=ClaimKind(TEXT("strange.crawler.n5"),Got,Night)&&Got==Low&&Night==5;
  const bool bTall=ClaimKind(TEXT("strange.tallfigure.n12"),Got,Night)&&Got==Tall&&Night==12;
  const bool bNot=!ClaimKind(TEXT("strange.shade.n2"),Got,Night)&&!ClaimKind(TEXT("strange.shadow.n2"),Got,Night)&&!ClaimKind(TEXT("strange.sky.n2"),Got,Night)&&!ClaimKind(TEXT("dusk_orb.n2"),Got,Night)
   &&!ClaimKind(TEXT("strange.pale"),Got,Night)&&!ClaimKind(TEXT("strange.pale.x3"),Got,Night);
  Check(bLow&&bTall&&bNot&&KindOfGroup(TEXT("roswell"))==Small&&KindOfGroup(TEXT("pale"))==Low&&KindOfGroup(TEXT("tall"))==Tall&&KindOfGroup(TEXT("nordic"))==Silver&&KindOfGroup(TEXT("cosmic"))==Shining
   &&KindOfGroup(TEXT(""))==EKind::None&&KindOfVisit(0)==Small&&KindOfVisit(1)==Low&&KindOfVisit(2)==Tall&&KindOfVisit(3)==EKind::None&&KindOfVisit(-1)==EKind::None,
   TEXT("the road's crawler is the low one and its tall figure the tall one; the shade, the shadow and the lights in the sky are none of them"));
 }
 { // the numbers: in the order first logged, and they never move
  FS47Dossier D;
  const bool bFirst=D.Tag(TEXT("road_tracks_d2"),Low,Ground,2)&&D.Tag(TEXT("rule-still-held"),Small,Lamp,3)&&D.Tag(TEXT("strange.pale.n2"),Low,Shape,2); // (slice 2: the same night, so it ties)
  const bool bAgain=!D.Tag(TEXT("road_tracks_d2"),Tall,Ground,6)&&!D.Tag(FString(),Low,Lamp,2);
  const bool bThen=D.Tag(TEXT("strange.tall.n1"),Tall,Shape,1); // an earlier night logged later: the number is by logging, not by night
  Check(bFirst&&bAgain&&bThen&&D.NumberOf(Low)==1&&D.NumberOf(Small)==2&&D.NumberOf(Tall)==3&&D.NumberOf(Silver)==0&&D.Order().Num()==3&&D.Tags().Num()==4,
   TEXT("UNKNOWN 1 to 5 in the order first logged; a find is tagged once; later finds never move a number"));
  const TArray<FFile> Files=D.Files();
  Check(Files.Num()==3&&Files[0].Title==TEXT("UNKNOWN 1")&&Files[0].Boxes.Num()==TypeN&&Files[0].Boxes[int32(Ground)].bFilled&&Files[0].Boxes[int32(Ground)].Day==2&&Files[0].Boxes[int32(Ground)].How==TEXT("noted")
   &&Files[0].Boxes[int32(Shape)].How==TEXT("on film")&&Files[0].Photos==TArray<FString>{TEXT("strange.pale.n2")}&&!Files[0].Boxes[int32(Lamp)].bFilled&&Files[0].Boxes[int32(Lamp)].Note.IsEmpty()
   &&Files[0].Nights==1&&Files[0].Opened==2&&Files[0].Latest==2&&Files[0].Stage==TEXT("OUTLINE")&&Files[1].Boxes[int32(Lamp)].Note==Note(Small,Lamp)&&Files[2].Number==3,
   TEXT("a file shows six boxes, the filled ones with how and when and the player's pencil, the empty ones with nothing"));
  // the save, there and back through text
  const S47J::FObj Record=S47J::Read(S47J::Write(D.Save()));
  FS47Dossier Back;Back.Load(Record);
  FS47Dossier Again;Again.Load(S47J::Read(S47J::Write(Back.Save())));
  bool bSame=Back.Order()==D.Order()&&Back.Tags().Num()==D.Tags().Num();
  for(const FS47Dossier::FTag& One:D.Tags())
   bSame&=Back.Tags().ContainsByPredicate([&One](const FS47Dossier::FTag& Other){return Other.Key==One.Key&&Other.Kind==One.Kind&&Other.Type==One.Type&&Other.Day==One.Day;});
  const S47J::FObj Row=S47J::Obj(S47J::Obj(Record,TEXT("tags")),TEXT("road_tracks_d2"));
  Check(bSame&&S47J::Strs(Record,TEXT("order"))==TArray<FString>{TEXT("pale"),TEXT("roswell"),TEXT("tall")}&&S47J::Str(Row,TEXT("k"))==TEXT("pale")&&S47J::Str(Row,TEXT("t"))==TEXT("ground")&&S47J::Int(Row,TEXT("d"))==2
   &&Back.NumberOf(Low)==1&&Back.NumberOf(Small)==2&&Back.NumberOf(Tall)==3&&S47J::Write(Again.Save())==S47J::Write(Back.Save()),
   TEXT("saved as dossier{order[],tags{key:{k,t,d}}} and read back the same, numbers and all"));
  // from the player's own tags only
  Check(D.Fit(Low,Ground,Low)==EFit::Fits&&D.Fit(Low,Ground,Tall)==EFit::Contradicts&&D.Fit(Low,Ground,Silver)==EFit::Contradicts&&D.Fit(Low,Lamp,Small)==EFit::Unknown&&D.Fit(Low,Lamp,Low)==EFit::Unknown
   &&D.Fit(Small,Lamp,Small)==EFit::Fits&&D.Fit(Small,Lamp,Low)==EFit::Contradicts&&D.Fit(Silver,Shape,Silver)==EFit::Unknown&&D.Fit(Tall,Shape,Low)==EFit::Contradicts,
   TEXT("a find fits or contradicts a file only by what the player has put in that box; an empty box answers nothing"));
 }
 { // the stages
  Check(StageFor(Small,0,0,false)==EStage::NoFile&&StageFor(Small,1,1,false)==EStage::Outline&&StageFor(Small,2,5,true)==EStage::Outline&&StageFor(Small,3,1,true)==EStage::Outline
   &&StageFor(Small,3,2,false)==EStage::Marks&&StageFor(Low,4,2,true)==EStage::Marks&&StageFor(Low,4,3,false)==EStage::Marks&&StageFor(Low,6,9,false)==EStage::Marks
   &&StageFor(Tall,4,3,true)==EStage::Habits&&StageFor(EKind::None,6,9,true)==EStage::NoFile,
   TEXT("OUTLINE is one kind of evidence, MARKS three from two nights, HABITS four from three nights and never without the lamp"));
  FS47Dossier D;
  D.Tag(TEXT("a"),Low,Shape,1);D.Tag(TEXT("b"),Low,Ground,1);D.Tag(TEXT("c"),Low,Tape,1);
  const bool bOneNight=D.StageOf(Low)==EStage::Outline&&D.AtMarks()==0;
  D.Tag(TEXT("d"),Low,Tape,2);
  const bool bMarks=D.StageOf(Low)==EStage::Marks&&D.AtMarks()==1&&D.TypesOf(Low)==3&&D.NightsOf(Low)==2;
  D.Tag(TEXT("d3"),Low,Shape,3);D.Tag(TEXT("e"),Low,Geiger,3);D.Tag(TEXT("d4"),Low,Shape,4);D.Tag(TEXT("f"),Low,Animals,4); // (slice 2: the shape again ties each night)
  const bool bNoLamp=D.StageOf(Low)==EStage::Marks&&D.TypesOf(Low)==5;
  D.Tag(TEXT("g"),Low,Lamp,4);
  Check(bOneNight&&bMarks&&bNoLamp&&D.StageOf(Low)==EStage::Habits&&D.Files()[0].Stage==TEXT("HABITS")&&D.Order().Num()==1,TEXT("a file's stage follows its own tags: one night is not two, and five kinds without the lamp are still MARKS"));
  // the two that stop
  FS47Dossier Far;
  bool bRefused=!Far.Tag(TEXT("x"),Silver,Ground,1)&&!Far.Tag(TEXT("y"),Shining,Geiger,1)&&!Far.Tag(TEXT("z"),Silver,Animals,1)&&Far.IsEmpty();
  for(const EKind Kind:{Silver,Shining})
  {
   int32 Day=1;
   for(const EType Type:{Shape,Tape,Lamp})for(int32 Rep=0;Rep<3;++Rep){Far.Tag(FString::Printf(TEXT("far-%d-%d-%d"),int32(Kind),int32(Type),Rep),Kind,Type,Day);++Day;}
   bRefused&=Far.StageOf(Kind)==EStage::Marks&&Far.TypesOf(Kind)==3&&Far.NightsOf(Kind)==9&&StageFor(Kind,6,30,true)==EStage::Marks&&StopsAtMarks(Kind)&&NextWords(Kind,EStage::Marks)==NextStops;
  }
  Check(bRefused&&!StopsAtMarks(Small)&&!StopsAtMarks(Low)&&!StopsAtMarks(Tall)&&NextWords(Low,EStage::Marks)==NextMarks,TEXT("the silver man's file and the shining one's stop at MARKS, however much goes in"));
  // the shade
  FS47Dossier Shade;
  Check(!Shade.Tag(TEXT("strange.shade.n3"),KindOfGroup(TEXT("shade")),Shape,3)&&!Shade.Tag(TEXT("strange.shadow.n3"),KindOfGroup(TEXT("shadow")),Shape,3)&&!Shade.Tag(TEXT("q"),EKind::None,Lamp,3)
   &&Shade.IsEmpty()&&Shade.Files().IsEmpty()&&Shade.Order().IsEmpty(),TEXT("the shade never gets a file"));
  // slice 1's own sources: a photograph or the visit's page (SHAPE), the prints on the shoulder (GROUND), the two rules (LAMP)
  FS47Dossier One;
  for(int32 K=0;K<KindN;++K)for(int32 Night=1;Night<=6;++Night)for(const EType Type:{Shape,Ground,Lamp})One.Tag(FString::Printf(TEXT("s1-%d-%d-%d"),K,Night,int32(Type)),EKind(K),Type,Night);
  bool bNone=true;
  for(int32 K=0;K<KindN;++K)bNone&=One.StageOf(EKind(K))!=EStage::Habits&&One.StageOf(EKind(K))!=EStage::NoFile;
  Check(bNone&&One.StageOf(Low)==EStage::Marks&&One.StageOf(Silver)==EStage::Outline&&One.AtMarks()==3,TEXT("with slice 1's three kinds of evidence nothing reaches HABITS"));
 }
 { // LOOP-DOSSIER-GIVES-20261113: every stage gives something the player can see, and it is kept
  FS47Dossier D;
  const FVector2D Here(120,-340),There(900,40);
  D.Tag(TEXT("g1"),Low,Shape,2,23,nullptr); // at a bench: no place kept
  const bool bUnmapped=D.Files()[0].Gave==GaveUnmapped&&D.StageOf(Low)==EStage::Outline;
  double WX=0,WZ=0;
  D.Tag(TEXT("g2"),Low,Shape,2,1,&There);
  D.Tag(TEXT("g2b"),Low,Shape,3,2,&Here); // a later night: the first place stays the first (slice 2: the shape again ties it)
  D.Tag(TEXT("g3"),Low,Ground,3,2,&Here);
  const bool bMapped=D.StageOf(Low)==EStage::Outline&&D.Files()[0].Gave==GaveMapped&&D.FirstWhere(Low,WX,WZ)&&WX==There.X&&WZ==There.Y&&!D.FirstWhere(Small,WX,WZ);
  Check(bUnmapped&&bMapped,TEXT("LOOP-DOSSIER-GIVES: OUTLINE puts where the file was first logged out there on the map; one logged only at a bench says it is not on it yet"));
  D.Tag(TEXT("g4"),Low,Tape,3,3,&Here);
  const bool bReached=D.StageOf(Low)==EStage::Marks&&D.ExtraState(Low)==0&&D.Files()[0].Gave==GaveListens;
  const bool bGrant=D.Grant(Low,3)&&!D.Grant(Low,5)&&!D.ExtraOpen(2)&&D.ExtraOpen(3)&&D.ExtraOpen(4)&&D.Files()[0].Gave==GaveExtraOpen;
  const bool bOnce=!D.Spend(2)&&D.Spend(4)&&!D.Spend(4)&&!D.Spend(9)&&!D.ExtraOpen(4)&&!D.ExtraOpen(30)&&D.ExtraState(Low)==2&&D.Files()[0].Gave==GaveExtraSpent;
  Check(bReached&&bGrant&&bOnce,TEXT("LOOP-DOSSIER-GIVES: MARKS gives one more bearing, once, from its night on until it is printed, and never again"));
  const S47J::FObj Record=S47J::Read(S47J::Write(D.Save()));
  FS47Dossier Back;Back.Load(Record);
  const S47J::FObj Row=S47J::Obj(S47J::Obj(Record,TEXT("tags")),TEXT("g2"));
  double BX=0,BZ=0;
  Check(Back.HoursOf(Low)==TArray<int32>{23,1,2,3}&&Back.FirstWhere(Low,BX,BZ)&&BX==There.X&&BZ==There.Y&&Back.ExtraState(Low)==2&&!Back.ExtraOpen(30)&&S47J::Int(Row,TEXT("h"))==1&&S47J::Num(Row,TEXT("x"))==900
   &&!S47J::Obj(S47J::Obj(Record,TEXT("tags")),TEXT("g1"))->HasField(TEXT("x"))&&S47J::Int(S47J::Obj(Record,TEXT("extra")),TEXT("pale"))==3&&S47J::Strs(Record,TEXT("spent"))==TArray<FString>{TEXT("pale")}
   &&S47J::Write(Back.Save())==S47J::Write(D.Save()),
   TEXT("LOOP-DOSSIER-GIVES: the hour and the place on each tag, and the extra bearing given and printed, survive a Continue"));
  D.Tag(TEXT("g4b"),Low,Shape,4,4,&Here);D.Tag(TEXT("g5"),Low,Lamp,4,4,&Here);
  const TArray<int32> Hours=D.HoursOf(Low);
  Check(D.StageOf(Low)==EStage::Habits&&D.Files()[0].Gave==GaveHours&&HoursLine(1,Hours)==TEXT("UNKNOWN 1 / Logged so far at 23:00, 01:00, 02:00, 03:00, 04:00.")&&HoursLine(1,TArray<int32>()).IsEmpty()
   &&HoursLine(2,TArray<int32>{7,8,7,30,-1})==TEXT("UNKNOWN 2 / Logged so far at 08:00, 07:00.")&&!S47Marginalia::NamesKind(HoursLine(1,Hours))&&S47Script::LintText(HoursLine(1,Hours)).IsEmpty(),
   TEXT("LOOP-DOSSIER-GIVES: HABITS puts the hours the file was logged at on tonight's note, in the night's order, and nothing else"));
  // a dossier from before: no hours, no place, no extra; it reads and writes as it was, and a file at MARKS gets its extra once
  FS47Dossier Old;
  const FString Was=TEXT("{\"order\":[\"pale\"],\"tags\":{\"a\":{\"k\":\"pale\",\"t\":\"ground\",\"d\":2},\"b\":{\"k\":\"pale\",\"t\":\"shape\",\"d\":3},\"c\":{\"k\":\"pale\",\"t\":\"tape\",\"d\":3},\"d\":{\"k\":\"pale\",\"t\":\"lamp\",\"d\":5,\"h\":99,\"x\":\"far\",\"z\":2}}}");
  Old.Load(S47J::Read(Was));
  const S47J::FObj OldOut=S47J::Read(S47J::Write(Old.Save()));
  const bool bAsWas=Old.Tags().Num()==4&&Old.HoursOf(Low).IsEmpty()&&!Old.FirstWhere(Low,BX,BZ)&&Old.ExtraState(Low)==0&&!OldOut->HasField(TEXT("extra"))&&!S47J::Obj(S47J::Obj(OldOut,TEXT("tags")),TEXT("a"))->HasField(TEXT("h"))
   &&Old.StageOf(Low)==EStage::Habits&&Old.Files()[0].Gave==GaveNoHours;
  FS47Dossier Marks;Marks.Load(S47J::Read(Was.Replace(TEXT("\"lamp\",\"d\":5"),TEXT("\"lamp\",\"d\":3"))));
  const bool bMissing=Marks.StageOf(Low)==EStage::Marks&&Marks.GrantMissing(6)==1&&Marks.GrantMissing(6)==0&&!Marks.ExtraOpen(5)&&Marks.ExtraOpen(6);
  Check(bAsWas&&bMissing,TEXT("LOOP-DOSSIER-GIVES: an older dossier with no hours or places loads as it was; a file it has at MARKS gets its one extra bearing once"));
  // the two that stop at MARKS give it too; no file, nothing
  Check(Gave(Silver,EStage::Marks,true,1,false)==GaveExtraOpen&&Gave(Shining,EStage::Marks,false,2,true)==GaveExtraSpent&&Gave(EKind::None,EStage::Marks,true,1,true).IsEmpty()&&Gave(Small,EStage::NoFile,true,1,true).IsEmpty(),
   TEXT("LOOP-DOSSIER-GIVES: the silver one's and the shining one's files give their extra bearing at MARKS too; no file gives nothing"));
  const double Hr=3600.,Dy=86400.;
  Check(ExtraNightFor(23*Hr)==1&&ExtraNightFor(Dy+3*Hr)==1&&ExtraNightFor(Dy+5*Hr)==2&&ExtraNightFor(10*Hr)==1&&ExtraNightFor(Dy+18*Hr)==2&&ExtraNightFor(-5)==1,
   TEXT("LOOP-DOSSIER-GIVES: a file brought to MARKS in the night gives that night its bearing; from five in the morning, and in daylight, the coming night"));
 }
 { // a save from before the dossier
  FS47Dossier None;None.Load(nullptr);
  FS47Dossier Junk;
  Junk.Load(S47J::Read(TEXT("{\"order\":[\"pale\",\"shade\",\"pale\",7],\"tags\":{\"a\":{\"k\":\"pale\",\"t\":\"ground\",\"d\":2},\"b\":{\"k\":\"shade\",\"t\":\"shape\",\"d\":2},\"c\":{\"k\":\"tall\",\"t\":\"shape\",\"d\":1e99},\"d\":7,\"e\":{\"k\":\"nordic\",\"t\":\"geiger\",\"d\":3},\"f\":{\"k\":\"roswell\",\"t\":\"lamp\",\"d\":1}}}")));
  Check(None.IsEmpty()&&None.Order().IsEmpty()&&Junk.Tags().Num()==2&&Junk.NumberOf(Low)==1&&Junk.NumberOf(Small)==2&&Junk.Order().Num()==2,
   TEXT("no dossier in the record reads as empty; what is not sound in one is left out and the rest stands"));
  const TArray<FString> Claims={TEXT("strange.pale.n3"),TEXT("strange.shade.n2"),TEXT("strange.crawler.n5"),TEXT("dusk_orb.n2"),TEXT("strange.sky.n1"),TEXT("strange.nordic.n9")};
  TArray<FS47Dossier::FOld> Old;
  Old.Add({TEXT("rule-still-held"),TEXT("A small one came near the place I stood."),2});
  Old.Add({TEXT("rule-pale-broken"),TEXT("Standing still is not enough."),6});
  Old.Add({TEXT("road_tracks_d4"),S47RoadGlimpse::TracksRecord(true,TEXT("09:10"),true),4});
  Old.Add({TEXT("road_tracks_d7"),S47RoadGlimpse::TracksRecord(false,TEXT("01:30"),false),8});
  Old.Add({TEXT("photo-registration.strange.pale.n3"),TEXT("OPS FIELD PHOTOGRAPH RECEIPT"),4});
  Old.Add({TEXT("photo-registration.strange.tall.n6"),TEXT("OPS FIELD PHOTOGRAPH RECEIPT"),7});
  Old.Add({TEXT("farm_visit"),TEXT("Something came toward me. Then it was gone."),3});
  Old.Add({TEXT("extra_pulse"),TEXT("retained"),1});
  FS47Dossier A,B;
  const int32 Made=A.Rebuild(Old,Claims);
  TArray<FS47Dossier::FOld> Turned=Old;Algo::Reverse(Turned);TArray<FString> ClaimsTurned=Claims;Algo::Reverse(ClaimsTurned);
  B.Rebuild(Turned,ClaimsTurned);
  const auto TagOf=[&A](const TCHAR* Key){return A.Tags().FindByPredicate([Key](const FS47Dossier::FTag& One){return One.Key==Key;});};
  const FS47Dossier::FTag* Film=TagOf(TEXT("strange.pale.n3"));const FS47Dossier::FTag* Prints=TagOf(TEXT("road_tracks_d7"));
  Check(Made==7&&A.Tags().Num()==7&&A.NumberOf(Small)==1&&A.NumberOf(Low)==2&&A.NumberOf(Tall)==3&&A.NumberOf(Silver)==4&&A.NumberOf(Shining)==0
   &&Film&&Film->Kind==Low&&Film->Type==Shape&&Film->Day==3&&Prints&&Prints->Kind==Tall&&Prints->Type==Ground&&Prints->Day==8
   &&!A.HasTag(TEXT("rule-pale-broken"))&&!A.HasTag(TEXT("farm_visit"))&&!A.HasTag(TEXT("strange.shade.n2"))&&!A.HasTag(TEXT("photo-registration.strange.pale.n3"))&&A.Has(Small,Lamp)&&A.StageOf(Low)==EStage::Outline,
   TEXT("an older save is rebuilt from its photograph claims, the rule learned and the prints on the shoulder; what does not say which one it was is left alone"));
  Check(A.Order()==B.Order()&&S47J::Write(A.Save())==S47J::Write(B.Save())&&A.Rebuild(Old,Claims)==0,TEXT("the rebuild gives the same numbers whatever order the save's fields come in, and makes nothing twice"));
 }
 { // the binder's typed sheet
  FS47Dossier D;D.Tag(TEXT("strange.crawler.n3"),Low,Shape,3);D.Tag(TEXT("farm_visit"),Low,Shape,5);D.Tag(TEXT("road_tracks_d4"),Low,Ground,5);D.Tag(TEXT("well_visit"),Low,Shape,9);D.Tag(TEXT("rule-pale-broken"),Low,Lamp,9);
  const FFile File=D.Files()[0];
  const FString Typed=Sheet(File,TArray<FString>{TEXT("DAY 03 / ORIGINAL A-12 / IN THE ALBUM")});
  Check(File.bAtMarks&&Typed.Contains(TEXT("STAGE     MARKS"))&&Typed.Contains(TEXT("NIGHTS    3 / FIRST DAY 03 / LAST DAY 09"))&&Typed.Contains(TEXT("SHAPE     ON FILM / DAY 03"))&&Typed.Contains(TEXT("TAPE      NOTHING KEPT"))
   &&Typed.Contains(TEXT("LAMP      NOTED / DAY 09"))&&Typed.Contains(TEXT("ORIGINAL A-12"))&&Typed.Contains(Note(Low,Ground))&&Typed.Contains(NextMarks)&&!Typed.Contains(Note(Low,Tape))&&!Typed.ToLower().Contains(TEXT("pale one"))
   &&S47Script::LintText(Typed).IsEmpty()&&!S47Marginalia::NamesKind(Typed)&&Sheet(File,TArray<FString>()).Contains(TEXT("NONE IN THE ALBUM.")),
   TEXT("the binder's sheet is the same file typed: stage, six boxes, the photographs referenced, the pencil for the filled boxes only"));
 }
 { // slice 2: what the player could tie, kept apart, side by side, and struck through
  FS47Dossier D;
  const FVector2D Farm(400,-900),Road(-30,2400);
  // a tape of something never photographed, then a photograph: two files, though the game knows they are the same
  const bool bApart=D.Tag(TEXT("tape_1"),Small,Tape,3)&&D.Tag(TEXT("strange.roswell.n5"),Small,Shape,5)&&D.FilesOf(Small)==2&&D.NumberOf(Small)==1&&D.FileOf(TEXT("strange.roswell.n5"))==2
   &&D.Beside().IsEmpty()&&D.Files().Num()==2&&D.Files()[1].Stage==TEXT("OUTLINE");
  // a visit: what was seen ties to the photograph's file (the same shape), the tape to the tape's (the same notes), and so the two
  // are shown to be one thing: kept side by side, not merged
  D.Tag(TEXT("farm_visit"),Small,Shape,7,2,&Farm);
  D.Tag(TEXT("tape_2"),Small,Tape,7,2,&Farm);
  D.Tag(TEXT("farm_seen"),Small,Shape,7,3,&Farm); // seen again, got with the tape: into the lower of the two
  const TArray<FFile> Two=D.Files();
  const TArray<int32> Marks=Agreement(Two[0],Two[1]);
  const bool bBeside=D.FileOf(TEXT("farm_visit"))==2&&D.FileOf(TEXT("tape_2"))==1&&D.FileOf(TEXT("farm_seen"))==1&&D.Beside().Num()==1&&D.Beside()[0]==FIntPoint(1,2)
   &&Two.Num()==2&&Two[0].Beside==TArray<int32>{2}&&Two[1].Beside==TArray<int32>{1}&&Marks[int32(Shape)]==1&&Marks[int32(Tape)]==0&&Marks[int32(Lamp)]==0
   &&BesideWords(Two[0],Two[1])==TEXT("UNKNOWN 1 and UNKNOWN 2 side by side. The same in: shape.")&&Sheet(Two[0],TArray<FString>()).Contains(TEXT("KEPT BESIDE UNKNOWN 2."));
  Check(bApart&&bBeside,TEXT("slice 2: a find the player cannot tie opens its own file; one that ties two keeps them side by side, never merged, and marks where they agree"));
  // got there that night with the rest, something else on the tape: the file's old pencil in that box is struck, the new stands
  D.Tag(TEXT("tape_3"),Tall,Tape,7,3,&Farm);
  const FFile One=D.Files()[0];
  const FBox& TapeBox=One.Boxes[int32(Tape)];
  const bool bStruck=D.FileOf(TEXT("tape_3"))==1&&TapeBox.Struck==TArray<FString>{Note(Small,Tape)}&&TapeBox.Note==Note(Tall,Tape)&&Sheet(One,TArray<FString>()).Contains(FString(TEXT("STRUCK / "))+Note(Small,Tape))
   &&D.NumberOf(Tall)==0&&D.Order().Num()==2;
  // far off, the same night, nothing ties: its own file
  D.Tag(TEXT("road_tracks_d7"),Tall,Ground,7,9,&Road);
  Check(bStruck&&D.FileOf(TEXT("road_tracks_d7"))==3&&D.NumberOf(Tall)==3,TEXT("slice 2: a find got together with a file's own but not what its box held strikes the old pencil through; far off it ties nothing"));
  // the save: which file each went into, the pair, the order it was logged in; a reader of slice 1's fields alone sees each kind once
  const S47J::FObj Record=S47J::Read(S47J::Write(D.Save()));
  FS47Dossier Back;Back.Load(Record);
  const TArray<FFile> Again=Back.Files();
  Check(S47J::Strs(Record,TEXT("files"))==TArray<FString>{TEXT("roswell"),TEXT("roswell"),TEXT("tall")}&&S47J::Strs(Record,TEXT("order"))==TArray<FString>{TEXT("roswell"),TEXT("tall")}
   &&S47J::Int(S47J::Obj(S47J::Obj(Record,TEXT("tags")),TEXT("tape_3")),TEXT("f"))==1&&!S47J::Obj(S47J::Obj(Record,TEXT("tags")),TEXT("tape_1"))->HasField(TEXT("f"))
   &&Back.Beside()==D.Beside()&&Again.Num()==3&&Again[0].Boxes[int32(Tape)].Struck==TapeBox.Struck&&Back.FileOf(TEXT("farm_visit"))==2&&S47J::Write(Back.Save())==S47J::Write(D.Save()),
   TEXT("slice 2: the files, the pair kept side by side and the struck pencil survive a Continue"));
  {
   // REVIEW (14 Nov): logged out of night order. A tape taped on night 3 and played back on night 5 goes in after night 5's finds:
   // what it struck, the pencil that stands and the file a later find joins come back the same after a Continue, and again after another
   FS47Dossier L;
   L.Tag(TEXT("l3"),Small,Shape,3,2,&Farm);L.Tag(TEXT("l5"),Small,Shape,5,2,&Farm);L.Tag(TEXT("l5t"),Small,Tape,5,2,&Farm);
   L.Tag(TEXT("tape_9"),Tall,Tape,3,2,&Farm); // got with l3 on night 3, logged last: it strikes the night-5 notes
   FS47Dossier LBack;LBack.Load(S47J::Read(S47J::Write(L.Save())));
   FS47Dossier LAgain;LAgain.Load(S47J::Read(S47J::Write(LBack.Save())));
   bool bLate=L.FileOf(TEXT("tape_9"))==1&&L.Files()[0].Boxes[int32(Tape)].Note==Note(Tall,Tape)&&L.Files()[0].Boxes[int32(Tape)].Struck==TArray<FString>{Note(Small,Tape)}
    &&S47J::Write(LBack.Save())==S47J::Write(L.Save())&&S47J::Write(LAgain.Save())==S47J::Write(L.Save());
   for(FS47Dossier* Each:{&L,&LBack,&LAgain})
   {
    const FBox Was=Each->Files()[0].Boxes[int32(Tape)];
    bLate&=Was.Note==Note(Tall,Tape)&&Was.Struck==TArray<FString>{Note(Small,Tape)}&&Each->Files().Num()==1;
    // the same notes again, far off on another night: they join the file whose tape box holds them now
    bLate&=Each->Tag(TEXT("l6"),Tall,Tape,6,1,&Road)&&Each->FileOf(TEXT("l6"))==1;
   }
   Check(bLate,TEXT("REVIEW: tags logged out of night order (late playback) load in the order they were logged: files, struck pencil and joins the same over two Continues"));
  }
  // the two far ones: anything of theirs in a file holds it at MARKS, whichever opened it
  FS47Dossier E;
  for(int32 Night=2;Night<=4;++Night)for(const EType T:{Shape,Ground,Tape,Lamp})E.Tag(FString::Printf(TEXT("e%d%d"),Night,int32(T)),Small,T,Night,2,&Farm);
  const bool bNear=E.StageOfFile(1)==EStage::Habits;
  E.Tag(TEXT("e-silver"),Silver,Lamp,4,3,&Farm);
  Check(bNear&&E.FileOf(TEXT("e-silver"))==1&&E.StageOfFile(1)==EStage::Marks&&E.Files()[0].Next==NextStops,TEXT("slice 2: a file with anything of the silver one or the shining one in it stops at MARKS"));
  // HABITS waits for the first week with danger in it
  FS47Dossier W;W.SetHabitsFrom(8);
  for(int32 Night=2;Night<=7;++Night)for(const EType T:{Shape,Ground,Tape,Lamp,Geiger,Animals})W.Tag(FString::Printf(TEXT("w%d%d"),Night,int32(T)),Low,T,Night,2,&Farm);
  const bool bWait=W.StageOfFile(1)==EStage::Marks&&W.StageOf(Low)==EStage::Marks;
  W.Tag(TEXT("w8"),Low,Shape,8,2,&Farm);
  Check(bWait&&W.StageOfFile(1)==EStage::Habits&&StageFor(Low,4,3,true,7,8)==EStage::Marks&&StageFor(Low,4,3,true,8,8)==EStage::Habits&&StageFor(Low,4,3,true)==EStage::Habits,
   TEXT("slice 2: HABITS not before the night the rhythm's curve first has danger in it"));
 }
 { // slice 2: the channels' own words and numbers
  bool bOmen=KindOfOmen(TEXT("visit"),0)==Small&&KindOfOmen(TEXT("visit"),1)==Low&&KindOfOmen(TEXT("visit"),2)==Tall&&KindOfOmen(TEXT("body:3"),2)==Small&&KindOfOmen(TEXT("door"),1)==Small
   &&KindOfOmen(TEXT("peek"),0)==Small&&KindOfOmen(TEXT("director:observer"),0)==EKind::None&&KindOfOmen(TEXT("night:horizon_figure"),0)==EKind::None&&KindOfOmen(TEXT("taken"),0)==EKind::None;
  TSet<int32> Alien;for(int32 K=0;K<KindN;++K)Alien.Add(AlienKindOf(EKind(K)));
  bOmen&=Alien.Num()==KindN&&!Alien.Contains(-1)&&AlienKindOf(EKind::None)==-1;
  Check(bOmen,TEXT("slice 2: the kind of a sign is known only where its source knows it, and each kind has its own sound on the tape"));
  Check(GeigerFactor(Small)>1.f/4&&GeigerFactor(Small)<1.f&&GeigerFactor(Low)==0.f&&GeigerFactor(Tall)>1.f&&GeigerFactor(Silver)==0.f&&GeigerFactor(Shining)==0.f&&GeigerFactor(EKind::None)==1.f,
   TEXT("slice 2: the counter: a little more for the small ones, nothing for the low one, a clear climb for the tall one, the plain reading where nobody knows"));
  Check(GeigerReads(true,5,3.2)&&!GeigerReads(false,5,9)&&!GeigerReads(true,20,9)&&!GeigerReads(true,5,1)&&TakeHeard(S47AlienKinds::SignalFrom+4)&&!TakeHeard(S47AlienKinds::SignalFrom+1)&&!TakeHeard(-1),
   TEXT("slice 2: tagged only when perceived: the counter out and near for a while, the take heard through"));
 }
 { // slice 2: the pacing, from the gates (M7)
  const FGates G{};
  bool bWeek1=true,bFar=true;int32 FirstMarks=99;
  for(int32 K=0;K<KindN;++K)
  {
   const int32 Habits=Earliest(EKind(K),EStage::Habits,G),Marks=Earliest(EKind(K),EStage::Marks,G);
   bWeek1&=Habits==0||Habits>7;
   if(Marks>0)FirstMarks=FMath::Min(FirstMarks,Marks);
   if(StopsAtMarks(EKind(K)))bFar&=Habits==0&&Marks>0;
  }
  bool bWeek4=true;for(const EKind Near:{Small,Low,Tall}){const int32 H=Earliest(Near,EStage::Habits,G);bWeek4&=H>7&&H<=30;}
  Check(bWeek1&&bWeek4&&bFar&&FirstMarks<=14&&Earliest(EKind::None,EStage::Outline,G)==0,*FString::Printf(TEXT("slice 2: pacing (standard gates): no HABITS in week 1, the near three can by week 4, the far two stop at MARKS, the first MARKS by night %d"),FirstMarks));
 }
 return bOk;
}
}
