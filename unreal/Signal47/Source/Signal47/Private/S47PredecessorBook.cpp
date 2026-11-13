#include "S47Predecessor.h"
#include "S47Freeform.h"
#include "S47Kit.h"
#include "S47Mail.h"
#include "S47Main.h"
#include "S47PredecessorText.h"
#include "S47UI.h"

// T16-17 (→ M3/M7; critic, 8 November: "Reyes-sporet ender i et brev"). Reyes's own pocket notebook, kept beside Dale's survey tags,
// and left a page at a time at the places Reyes kept going back to. Nothing new to do: a page lies in a place's panel (FS47Places),
// behind April's binder (OpenGap) or comes by post (FS47Mail, as Ward's papers do); a page that names a place puts it on the map with
// a question mark (Hear, as the tags do); the windmill's page is a lead out on the land (FS47FreeformCases::LineForFinding); the
// deputy takes the book in the end, and the Stays letter and the account say so. Through the month:
//   week one    the locker has the notebook's cover (OLD HOUSE, BACK ROOM); the back room's chair has the first page, which sends
//               the player to the line shack; the shack's page names Reyes's folder behind April's binder.
//   week two    the binder has a sign-out card where the folder stood: the sheriff's office has it. The deputy says it comes back.
//   week three  the folder by post, its carbons ending at the windmill; the windmill's page, and the cairn it points at.
//   then        under the cairn's top stone, the rest of the book; the last page is the Saturday before the last night, and blank
//               after. The deputy puts it in his file, with where each page was found. Nothing says what happened to Reyes.
// Saved in Seen (the pages read, "<doc>_sent" for the post): a save from before it simply starts the trail where it stands.

bool FS47Predecessor::BookOpen(const S47PredecessorText::FBookStep& S,const TSet<FString>& Have,int32 Day)
{
 return Day>=S.FromDay&&(!S.After||Have.Contains(S.After));
}
int32 FS47Predecessor::TrailDay() const{return M&&M->Freeform?M->Freeform->Day()-OpensDay+1:0;}
bool FS47Predecessor::BookDue(const FString& Doc) const
{
 const S47PredecessorText::FBookStep* S=S47PredecessorText::BookStep(Doc);
 return S&&Live()&&BookOpen(*S,Seen,TrailDay());
}
int32 FS47Predecessor::BookPages() const
{
 int32 N=0;
 for(const S47PredecessorText::FBookStep& S:S47PredecessorText::Book())if(FCString::Strcmp(S.Doc,TEXT("book_signout"))!=0&&FCString::Strcmp(S.Doc,TEXT("book_file"))!=0&&Seen.Contains(S.Doc))++N;
 return N;
}
bool FS47Predecessor::PlaceNews(const FString& Place) const
{
 if(!Live())return false;
 for(const S47PredecessorText::FBookStep& S:S47PredecessorText::Book())if(Place==S.Where&&S.Find&&!Seen.Contains(S.Doc)&&BookOpen(S,Seen,TrailDay()))return true;
 return false;
}
void FS47Predecessor::PlaceRows(const FString& Place,TArray<TSharedRef<SWidget>>& Rows)
{
 if(!Live())return;
 const TWeakPtr<uint8> Guard=Life;
 for(const S47PredecessorText::FBookStep& S:S47PredecessorText::Book())
 {
  if(Place!=S.Where||!S.Find||!S.Take)continue;
  const FString Doc=S.Doc;
  // Read before: it is still there to read again, by its own title.
  if(Seen.Contains(Doc))
  {
   Rows.Add(S47Kit::Button(PaperTitle(Doc)+TEXT(" (read)"),[this,Guard,Doc]{if(Guard.IsValid())ReadDoc(Doc);}));
   continue;
  }
  if(!BookOpen(S,Seen,TrailDay()))continue;
  Rows.Add(S47Kit::Text(S.Find));
  Rows.Add(S47Kit::Button(S.Take,[this,Guard,Doc]{if(Guard.IsValid())ReadDoc(Doc);},true));
 }
}
void FS47Predecessor::StepBook()
{
 if(!M||!M->Freeform)return;
 auto& F=*M->Freeform;
 for(const S47PredecessorText::FBookStep& S:S47PredecessorText::Book())
 {
  if(FCString::Strcmp(S.Where,TEXT("post"))!=0)continue;
  const FString Doc=S.Doc,Sent=Doc+TEXT("_sent");
  // By the office's hours, as her own papers go (the folder comes back through Ward's office).
  if(Seen.Contains(Sent)||Seen.Contains(Doc)||Tray.Contains(Doc)||!BookOpen(S,Seen,TrailDay())||!WardAvailable())continue;
  Seen.Add(Sent);
  const bool bByPost=F.Mail&&F.Mail->Live();
  Send(Doc);
  if(M->UI&&!bByPost)M->UI->Toast(TEXT("The sheriff's office has sent back a folder of Reyes's. It is in the OPS tray."),5);
  UE_LOG(LogTemp,Display,TEXT("S47: predecessor: %s posted, day %d of the trail"),*Doc,TrailDay());
  M->RequestAutosave();
 }
}
