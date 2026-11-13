#include "S47Dossier.h"
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
 const int32 MaxTags=4096,MaxKey=160,MaxDay=36500;
 bool Valid(EKind Kind){return int32(Kind)>=0&&int32(Kind)<KindN;}
 bool Valid(EType Type){return int32(Type)>=0&&int32(Type)<TypeN;}
 bool Earlier(const FS47Dossier::FTag& A,const FS47Dossier::FTag& B){return A.Day!=B.Day?A.Day<B.Day:A.Key<B.Key;}
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
EStage StageFor(EKind Kind,int32 Types,int32 Nights,bool bLamp)
{
 if(!Tables::Valid(Kind)||Types<1)return EStage::NoFile;
 if(Types>=4&&bLamp&&Nights>=3&&!StopsAtMarks(Kind))return EStage::Habits;
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
FString Sheet(const FFile& File,const TArray<FString>& PhotoLines)
{
 FString Out=FString::Printf(TEXT("STAGE     %s\nNIGHTS    %d / FIRST DAY %02d / LAST DAY %02d\n"),*File.Stage,File.Nights,File.Opened,File.Latest);
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
  Out+=TEXT("\n")+One.Note;
 }
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
 for(const TCHAR* W:SheetWords)Out.Add(W);
 // every stamp there can be, and a file's title
 for(int32 N=1;N<=KindN;++N)
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
FString Lint()
{
 // Freeform's banned words (S47Thoughts, S47FreeformCases), the working names that are no plain description, and the word the
 // old photograph receipts used.
 static const TCHAR* const Not[]={TEXT("1947"),TEXT("47 s"),TEXT("forty"),TEXT("cable"),TEXT("tomas"),TEXT("taken"),TEXT("silhouette"),TEXT("8.15"),TEXT("alien"),TEXT("ufo"),TEXT("saucer"),
  TEXT("martian"),TEXT("third"),TEXT("grey"),TEXT("gray"),TEXT("abduct"),TEXT("roswell"),TEXT("nordic"),TEXT("cosmic"),TEXT("crawler"),TEXT("visitor"),TEXT("species"),TEXT("creature")};
 FString Bad;
 for(const FString& W:Words())
 {
  if(const FString Why=S47Script::LintText(W);!Why.IsEmpty())Bad+=FString::Printf(TEXT("'%s': %s | "),*W.Left(40),*Why);
  if(S47Marginalia::NamesKind(W))Bad+=FString::Printf(TEXT("'%s': names what it is | "),*W.Left(40));
  const FString Low=W.ToLower();
  for(const TCHAR* N:Not)if(Low.Contains(N))Bad+=FString::Printf(TEXT("'%s': says '%s' | "),*W.Left(40),N);
 }
 return Bad;
}
}

// The class is at global scope: everything of the namespace is named in full here, so that nothing is looked up among whatever
// else a unity build has put in the same file.
bool FS47Dossier::Tag(const FString& Key,S47Dossier::EKind Kind,S47Dossier::EType Type,int32 Day)
{
 namespace Tb=S47Dossier::Tables;
 if(Key.IsEmpty()||Key.Len()>Tb::MaxKey||!Tb::Valid(Kind)||!Tb::Valid(Type)||!*S47Dossier::Signature(Kind,Type)||Kept.Num()>=Tb::MaxTags||HasTag(Key))return false;
 FTag One;One.Key=Key;One.Kind=Kind;One.Type=Type;One.Day=FMath::Clamp(Day,1,Tb::MaxDay);
 Kept.Add(MoveTemp(One));
 Sequence.AddUnique(Kind); // the first tag of a kind opens its file: the next number
 return true;
}
bool FS47Dossier::HasTag(const FString& Key) const
{
 return Kept.ContainsByPredicate([&Key](const FTag& One){return One.Key==Key;});
}
int32 FS47Dossier::NumberOf(S47Dossier::EKind Kind) const{return Sequence.IndexOfByKey(Kind)+1;}
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
 return S47Dossier::StageFor(Kind,TypesOf(Kind),NightsOf(Kind),Has(Kind,S47Dossier::EType::Lamp));
}
int32 FS47Dossier::AtMarks() const
{
 int32 Count=0;
 for(const S47Dossier::EKind Kind:Sequence)Count+=int32(StageOf(Kind))>=int32(S47Dossier::EStage::Marks)?1:0;
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
  const S47Dossier::EKind Kind=Sequence[At];const S47Dossier::EStage Stage=StageOf(Kind);
  S47Dossier::FFile File;File.Number=At+1;File.Title=FString::Printf(TEXT("UNKNOWN %d"),At+1);
  File.Stage=S47Dossier::StageWord(Stage);File.Next=S47Dossier::NextWords(Kind,Stage);
  File.bAtMarks=int32(Stage)>=int32(S47Dossier::EStage::Marks);File.Nights=NightsOf(Kind);
  File.Boxes.SetNum(Tb::TypeN);
  for(int32 T=0;T<Tb::TypeN;++T)File.Boxes[T].Label=Tb::TypeLabels[T];
  for(const FTag& One:Kept)
  {
   if(One.Kind!=Kind)continue;
   S47Dossier::FBox& Slot=File.Boxes[int32(One.Type)];
   const bool bShape=One.Type==S47Dossier::EType::Shape;
   if(!Slot.bFilled){Slot.bFilled=true;Slot.Day=One.Day;Slot.Note=S47Dossier::Note(Kind,One.Type);Slot.How=bShape?Tb::HowSeen:Tb::HowNoted;}
   else Slot.Day=FMath::Min(Slot.Day,One.Day);
   if(bShape&&S47Dossier::PhotoKey(One.Key)){Slot.How=Tb::HowFilm;File.Photos.Add(One.Key);}
   File.Opened=File.Opened==0?One.Day:FMath::Min(File.Opened,One.Day);File.Latest=FMath::Max(File.Latest,One.Day);
  }
  Out.Add(MoveTemp(File));
 }
 return Out;
}
S47J::FObj FS47Dossier::Save() const
{
 const S47J::FObj Out=S47J::New(),TagsOut=S47J::New();
 TArray<FString> Ids;
 for(const S47Dossier::EKind Kind:Sequence)Ids.Add(S47Dossier::KindId(Kind));
 S47J::Set(Out,TEXT("order"),Ids);
 for(const FTag& One:Kept)
 {
  const S47J::FObj Row=S47J::New();
  S47J::Set(Row,TEXT("k"),S47Dossier::KindId(One.Kind));S47J::Set(Row,TEXT("t"),S47Dossier::TypeId(One.Type));S47J::Set(Row,TEXT("d"),One.Day);
  S47J::Set(TagsOut,*One.Key,Row);
 }
 S47J::Set(Out,TEXT("tags"),TagsOut);
 return Out;
}
void FS47Dossier::Load(const S47J::FObj& Saved)
{
 namespace Tb=S47Dossier::Tables;
 Reset();
 if(!Saved.IsValid())return;
 for(const FString& Id:S47J::Strs(Saved,TEXT("order")))
 {
  const S47Dossier::EKind Kind=S47Dossier::KindOfId(Id);
  if(Tb::Valid(Kind))Sequence.AddUnique(Kind);
 }
 TArray<FTag> Read;
 if(const S47J::FObj TagsIn=S47J::Obj(Saved,TEXT("tags"));TagsIn.IsValid())for(const auto& Pair:TagsIn->Values)
 {
  const S47J::FObj* Row=nullptr;
  if(!Pair.Value.IsValid()||!Pair.Value->TryGetObject(Row)||!Row||!Row->IsValid())continue;
  const double OnDay=S47J::Num(*Row,TEXT("d"));
  if(!FMath::IsFinite(OnDay)||OnDay<1||OnDay>Tb::MaxDay)continue;
  FTag One;One.Key=FString(*Pair.Key);One.Kind=S47Dossier::KindOfId(S47J::Str(*Row,TEXT("k")));One.Type=S47Dossier::TypeOfId(S47J::Str(*Row,TEXT("t")));One.Day=int32(OnDay);
  Read.Add(MoveTemp(One));
 }
 // By day, then key: the same every time, whatever order the record's fields come in. Tag refuses what is not sound.
 Read.Sort([](const FTag& A,const FTag& B){return S47Dossier::Tables::Earlier(A,B);});
 for(const FTag& One:Read)Tag(One.Key,One.Kind,One.Type,One.Day);
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
 for(const FTag& One:Found)Made+=Tag(One.Key,One.Kind,One.Type,One.Day)?1:0;
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
  const bool bFirst=D.Tag(TEXT("road_tracks_d2"),Low,Ground,2)&&D.Tag(TEXT("rule-still-held"),Small,Lamp,3)&&D.Tag(TEXT("strange.pale.n4"),Low,Shape,4);
  const bool bAgain=!D.Tag(TEXT("road_tracks_d2"),Tall,Ground,6)&&!D.Tag(FString(),Low,Lamp,2);
  const bool bThen=D.Tag(TEXT("strange.tall.n1"),Tall,Shape,1); // an earlier night logged later: the number is by logging, not by night
  Check(bFirst&&bAgain&&bThen&&D.NumberOf(Low)==1&&D.NumberOf(Small)==2&&D.NumberOf(Tall)==3&&D.NumberOf(Silver)==0&&D.Order().Num()==3&&D.Tags().Num()==4,
   TEXT("UNKNOWN 1 to 5 in the order first logged; a find is tagged once; later finds never move a number"));
  const TArray<FFile> Files=D.Files();
  Check(Files.Num()==3&&Files[0].Title==TEXT("UNKNOWN 1")&&Files[0].Boxes.Num()==TypeN&&Files[0].Boxes[int32(Ground)].bFilled&&Files[0].Boxes[int32(Ground)].Day==2&&Files[0].Boxes[int32(Ground)].How==TEXT("noted")
   &&Files[0].Boxes[int32(Shape)].How==TEXT("on film")&&Files[0].Photos==TArray<FString>{TEXT("strange.pale.n4")}&&!Files[0].Boxes[int32(Lamp)].bFilled&&Files[0].Boxes[int32(Lamp)].Note.IsEmpty()
   &&Files[0].Nights==2&&Files[0].Opened==2&&Files[0].Latest==4&&Files[0].Stage==TEXT("OUTLINE")&&Files[1].Boxes[int32(Lamp)].Note==Note(Small,Lamp)&&Files[2].Number==3,
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
  D.Tag(TEXT("e"),Low,Geiger,3);D.Tag(TEXT("f"),Low,Animals,4);
  const bool bNoLamp=D.StageOf(Low)==EStage::Marks&&D.TypesOf(Low)==5;
  D.Tag(TEXT("g"),Low,Lamp,4);
  Check(bOneNight&&bMarks&&bNoLamp&&D.StageOf(Low)==EStage::Habits&&D.Files()[0].Stage==TEXT("HABITS"),TEXT("a file's stage follows its own tags: one night is not two, and five kinds without the lamp are still MARKS"));
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
  FS47Dossier D;D.Tag(TEXT("strange.crawler.n3"),Low,Shape,3);D.Tag(TEXT("farm_visit"),Low,Shape,5);D.Tag(TEXT("road_tracks_d4"),Low,Ground,5);D.Tag(TEXT("rule-pale-broken"),Low,Lamp,9);
  const FFile File=D.Files()[0];
  const FString Typed=Sheet(File,TArray<FString>{TEXT("DAY 03 / ORIGINAL A-12 / IN THE ALBUM")});
  Check(File.bAtMarks&&Typed.Contains(TEXT("STAGE     MARKS"))&&Typed.Contains(TEXT("NIGHTS    3 / FIRST DAY 03 / LAST DAY 09"))&&Typed.Contains(TEXT("SHAPE     ON FILM / DAY 03"))&&Typed.Contains(TEXT("TAPE      NOTHING KEPT"))
   &&Typed.Contains(TEXT("LAMP      NOTED / DAY 09"))&&Typed.Contains(TEXT("ORIGINAL A-12"))&&Typed.Contains(Note(Low,Ground))&&Typed.Contains(NextMarks)&&!Typed.Contains(Note(Low,Tape))&&!Typed.ToLower().Contains(TEXT("pale one"))
   &&S47Script::LintText(Typed).IsEmpty()&&!S47Marginalia::NamesKind(Typed)&&Sheet(File,TArray<FString>()).Contains(TEXT("NONE IN THE ALBUM.")),
   TEXT("the binder's sheet is the same file typed: stage, six boxes, the photographs referenced, the pencil for the filled boxes only"));
 }
 return bOk;
}
}
