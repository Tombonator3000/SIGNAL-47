#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;
class SWidget;

// NIGHTLOG-20261108 (Tom, 8 November: "nattens logg kl. 06"). At six in the morning a short typed sheet of the night just gone: what
// the player saw and heard (only what they really met: the field records the night left, the radio traffic that reached them), time
// nobody can account for, gear that failed on them, what went wrong with the night's work, and the money. Plain, in the station's
// words, no conclusions and nothing the player did not notice.
//
// It goes in the notebook (Documents). OPS can telex it to Dr. Ward at any hour; her office initials it in office hours with a line of
// its own, written on the sheet, and the sheets she has had count toward her assessment (FS47Predecessor::Points). Freeform only,
// saved under "nightLog"; an older save starts with none.
class FS47NightLog
{
public:
 explicit FS47NightLog(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop(){}
 void Step(double Dt);
 S47J::FObj Save() const;

 // Night N runs from 19:00 on shift day N to 06:00 the next morning (FS47FreeformSession::NightOf counts from 08:00).
 static double DuskOf(int32 N){return (N-1)*86400.+19*3600.;}
 static double SixOf(int32 N){return (N-1)*86400.+30*3600.;}

 enum class EPart:uint8{Seen,Time,Gear,Work};
 struct FLine{double Clock=0;EPart Part=EPart::Seen;FString At;FString Text;FString Place;}; // D48-20261110: Place, where the player stood when it was noticed (optional in a save)
 // A night as it is typed. Pure: the tests and the developer's menu build one by hand.
 struct FNight
 {
  int32 Night=0;FString From,To; // "MON APR 28"
  int32 CashFrom=0,CashTo=0,In=0,Out=0,Carried=0;
  TArray<FLine> Lines;
  TArray<FString> Work;
  // CHAIN-2-20261109: DELIVERED, the same three lines every morning: the field money tonight (SideLeft: what is left of the week's
  // cap, under 0 not said; bSameNight: a field record paid at the same-night rate), what opened on the stores list tonight, and how
  // tonight counts toward Dr. Ward's assessment (empty: nothing from out on the land).
  int32 Side=0,SideLeft=-1;bool bSameNight=false;
  TArray<FString> Opened;FString Ward;
 };
 static FString Type(const FNight& N);
 // What Dr. Ward writes under it when her office has read it: from what is on the sheet, never what it means.
 static FString RemarkFor(const FNight& N);
 // A field record's words, as the sheet has them: the first sentence, no longer than a line, its own time taken off the front.
 static FString Lead(const FString& Record,FString& OutAt);
 // CASES-OPS-20261109: a sheet's line about a case (the WORK line FS47FreeformSession::CaseRecordLine gives it, "THE REPEATED INTERVAL: ...", once "Case 1, ...", or an
 // older sheet's case record under SEEN AND HEARD): the station's own business. The weekly report leaves it off (FS47WardReport).
 static bool CaseLine(const FString& Text);

 // Facts: the sheet's lines as they were typed (the work's too, as Work lines), for the weekly report (FS47WardReport).
 struct FSheet{int32 Night=0;FString Title,Page,Remark;double Typed=0,Sent=-1,Read=-1;TArray<FLine> Facts;};
 const TArray<FSheet>& Sheets() const{return Logs;}
 // Sheets Dr. Ward's office has initialed: what the assessment counts.
 int32 ReadCount() const;
 bool Tonight() const{return Night>0;}

 // OPS: the sheets not yet sent, each with its telex button, and what has come back.
 void OpsRows(TArray<TSharedRef<SWidget>>& Rows);
 bool Send(int32 N);
 // TELEX-20261109: her office's receipt comes back on the telex in the control room ("telex-nightlog-N"); torn off there, her line
 // goes on the sheet in the notebook too.
 void Received(int32 N);
 static FString ReceiptId(int32 N){return FString::Printf(TEXT("telex-nightlog-%d"),N);}

 // D48-20261110 (D46-19, the notebook's EVENTS page): every happening the player has met, newest first, by date, time, place and a plain
 // kind word; the sheets' SEEN AND HEARD lines (and gear that failed out there) and what the player has logged by hand. Own entries are
 // saved under "own" in this object; an older save has none.
 struct FEvent{double Clock=0;FString When,Place,Kind,Text;bool bOwn=false;};
 TArray<FEvent> Events() const;
 static const TArray<FString>& EventKinds();   // lights, sounds, figure, power, animals, signal, other
 static const TArray<FString>& EventPhrases(); // the short notes to choose from
 static FString KindOf(const FString& Text);   // a line's plain kind word, from its words
 FString PlaceNow() const;                     // where the player stands (the save card's place name)
 // "Log something you saw": the kind and the note by their number; the place is where the player stands and the time is now. False: not
 // possible now (no game, a number out of range, the same entry again within the minute).
 bool LogOwn(int32 Kind,int32 Phrase);
 int32 OwnCount() const{return Own.Num();}
 // FIX10-9 (→ M3): when each entry logged by hand was made (game clock), oldest first: OPS sets one a night against the array's own log
 // (FS47FreeformSession::AskWhatCameBack, OwnNoteFor).
 TArray<double> OwnClocks() const{TArray<double> Out;for(const FOwn& E:Own)Out.Add(E.Clock);return Out;}
 // F10 and tests: type tonight's sheet now with what it has so far; a stretch of lost time; open the last sheet.
 bool DevTypeNow();
 void DevLostTime(double Minutes);
 bool DevOpenLast() const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TArray<FSheet> Logs;
 struct FOwn{double Clock=0;FString Kind,Place,Text;};
 TArray<FOwn> Own; // D48-20261110
 FString WhenOf(double Clock) const;
 // Tonight, as it goes (saved): the night, when it began for the sheet, the money, the lines, the gear's counts.
 int32 Night=0;
 double From=0;
 int32 CashFrom=0,CashIn=0,CashOut=0;
 TArray<FLine> Lines;
 int32 Flickers=0,Drifts=0,Coughs=0,Jumps=0;
 // CHAIN-2-20261109: the field money tonight, and what was on the stores list at dusk (saved, "side" and "open"; an older save has
 // neither: the list is read again on the first step). LastSideLeft: what was left of the week's cap last step (not saved).
 int32 Side=0,LastSideLeft=-1;
 TSet<FString> OpenAtDusk;bool bOpenKnown=false;
 void OpenNow(TSet<FString>& Out) const;
 FString WardLine() const;
 // Watching (not saved: read again from the world on the first step).
 bool bPrimed=false,bDevLost=false;
 double LastClock=0;
 int32 LastCash=0,LastFindings=-1,LastSaid=-1;
 int32 BaseFlickers=0,BaseDrifts=0,BaseCoughs=0,BaseJumps=0;
 bool bLampDead=false,bStuck=false,bFlat=false,bHot=false,bPowerOut=false,bArrayOut=false,bWasTaken=false;
 TSet<FString> Known,SaidKnown;
 void Prime();
 void Open(int32 N,double At);
 void Close(double Now);
 void Add(double Clock,EPart Part,const FString& Text,const FString& At=FString());
 void Watch(double Now,bool& bOutLost);
 void Receive(double Now);
 FNight Gather() const;
 FSheet* Find(int32 N);
 void File(const FSheet& S) const;
 FString Place() const;
};
