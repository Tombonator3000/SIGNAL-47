#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "Widgets/SWidget.h"
class US47Main;
class AS47Web;
class USceneComponent;
struct FS47Doc;
struct FS47MapView;

// Freeform only (Tom, 7 November: "Forrige nattvakt: Reyes fra Story mode"). Why be at SARO, why build it up, why not just leave:
// Ward's review, thirty days out; the operator before you, whose locker, notes and survey tags are still here; Mara Ortiz's wagon on
// the flat; and a board by the lockers where you can hang your keys and go. Everything is the outside of what happened: text in
// S47PredecessorText.cpp, and its self-test refuses the far side. Saved under "predecessor"; a save from before it picks it up
// on load, its own calendar untouched, with a letter that says so. Nothing here takes away an original or a photograph.
namespace S47PredecessorText{struct FBark;}
class FS47Predecessor
{
public:
 explicit FS47Predecessor(US47Main* Main):M(Main){}
 enum class EOutcome:uint8{None,Stays,KnowsEnough,ClosedOriginals,ClosedNothing,Gone};
 // What the assessment goes by: only what really happened in this game.
 // Clean: filed jobs whose recording was tuned well enough to read (critic, 8 November: quality and cases, not a count of cards).
 // Land: kinds of record kept from the land at night (the night's events, FS47Happenings), which count a little toward staying.
 // Logs: night logs her office has initialed (FS47NightLog, NIGHTLOG-20261108): a station that reports its nights counts a little.
 // Tapes: cassettes played back with something on them nobody heard out there (FS47Tape, TAPE-20261108): what she can send on.
 // Trust: what her office makes of the weekly reports (FS47WardReport, WARD-REPORT-20261108): whole ones raise it, a lie found out lowers it.
 struct FTally{int32 Filed=0,Clean=0,Concluded=0,Dishes=27,Arrears=0,Originals=0,Photos=0,Known=0,Taken=0,Land=0,Logs=0,Tapes=0,Trust=0;bool bWagon=false,bGap=false;};
 static constexpr int32 FiledFloor=10,PointsToStay=6,KnownEnough=10,TakenTooOften=3,FirstAssess=30,NoticeDays=7;
 static constexpr double CleanQuality=.6;
 // What the station can show the county: cases carried through, the array mended, records that can be read, a few photographs,
 // and the work, which counts once it is there and no more for being piled up.
 static int32 Points(const FTally& T);
 static EOutcome Decide(const FTally& T);
 static const TCHAR* NameOf(EOutcome O);
 static EOutcome OutcomeOf(const FString& Name);
 FTally Count() const;

 void Begin(const S47J::FObj& Saved,bool bOlderGame,int32 Today);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Live() const;

 bool Found(const FString& Id) const{return Seen.Contains(Id);}
 bool Heard(const FString& Id) const{return HeardOf.Contains(Id);}
 int32 Known() const;
 int32 AssessmentDay() const{return AssessDay;}
 EOutcome Result() const{return Outcome;}
 bool HasLeft() const{return bLeft;}
 bool Ringing() const{return bRinging;}
 // Dr. Ward keeps office hours (WARD-PHONE-20261108): no call to or from him from 21:00 until 08:00. His letters, sheets and
 // answers wait in the tray and are told the next morning. Clock: the Freeform world clock in seconds.
 static bool WardHours(double Clock){const double H=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;return H>=8.&&H<21.;}
 bool WardAvailable() const;
 int32 TrayCount() const{return Tray.Num();}
 bool RadioOn() const;
 // The closing order has taken effect: the county power is off, the array stowed, the truck the district's.
 bool StationClosed() const{return bClosed;}
 int32 ClosingDay() const{return CloseDay;}

 // OPS: what lies in the tray, the keys, and the station's status. SARO: UNDER REVIEW.
 void OpsRows(TArray<TSharedRef<SWidget>>& Rows);
 void OpsKeys(TArray<TSharedRef<SWidget>>& Rows);
 FString StatusLine() const;
 FString AccountLine() const;
 FString TaskLine() const;
 FString ReviewLine() const;
 // The records room: April's binder has a gap in it.
 FString ArchiveLabel() const;
 void OpenGap();
 // The control room's phone. False: nothing to do with it.
 bool UsePhone();
 // A person in Caliche: in Freeform they speak to you, and you are not Reyes. True when handled.
 bool Talk(const FString& Who);
 void OpenLocker();
 void OpenWagon();
 void OpenKeys();
 void OpenAssessment();
 void OpenLeft();
 // Back: "locker" or "wagon" to come back to it when the page is put down.
 void ReadDoc(const FString& Id,const FString& Back=FString());
 // MAILBOX-20261108: Dr. Ward's papers come by post (FS47Mail): her letter, the Monday sheets, the warning and the assessment go into
 // the box at the road, or straight into the OPS tray where a game has no post. bNow: in the box already (the first letter).
 void Send(const FString& Id,bool bNow=false);
 // Taken out of the box: into the OPS tray as before, where TaskLine, OpsRows and the review look for them.
 void FromMail(const FString& Id);
 // In the box at the road, not yet taken out.
 bool InMailbox(const FString& Id) const;
 FString PaperTitle(const FString& Id) const;
 // The paper's word of something out on the land (the Ledger's search story: wagon 23).
 void HearOf(const FString& Id){Hear(Id);}
 void FillMap(FS47MapView& View) const;
 // World position (x,z) of a thing out on the land or at the station, for the developer's menu and the tests.
 bool WorldSpot(const FString& Id,FString& OutArea,double& OutX,double& OutZ) const;

 // Developer's menu and tests: nothing here is saved in a developer run.
 void TestOn();
 void TestLetter();
 void TestWeek();
 void TestRing();
 // Dale's next due line over the walkie, whatever the day. False: none is due.
 bool TestBark();
 // Dale hails on the walkie now, with his next due line or a word of his own once it is answered. False: no set, or not on 1.
 bool TestHail();

 // WALKIE-USE-20261108: Dale on the walkie when the player calls him on channel 1. What comes back ("DALE: ..."), or empty: he
 // does not answer. Angry and short, by the hour and by what the player is doing; a line of his that is due comes first.
 FString AnswerWalkie();
 // CB-WALKIE-20261108: a stranger's report as Dale passes it on over the walkie ("DALE: ..."). Empty when there is no Dale here.
 FString RelayLine(const FString& Tip) const;
 // The district's service band (the walkie's channel 2): wagon 23's open carrier, while its radio is on, in its part of the
 // land. Empty: nothing on it. The first time it puts the wagon on the map, as the CB's talk of it does.
 FString ServiceBand();
 // What Dale's answer goes by. Hour: the Freeform clock's, 0 to 24. SinceLast: world-clock seconds since he last answered.
 struct FDaleCall{double Hour=20;bool bDriving=false,bInside=false,bAtStation=false,bFarOut=false,bMissed=false,bLostTime=false,bArrayDown=false;double SinceLast=1e9;int32 Answered=0;};
 // The id of his reply in S47PredecessorText::WalkieLines(), or null: nobody at his bench, or he has had enough.
 static const TCHAR* DaleReply(const FDaleCall& C);
 void TestOutcome(EOutcome O);
 void TestLeave();
 void TestNextDay();
 void TestAssessNow();
 void TestFound(const FString& Id);
 void TestHear(const FString& Id){Hear(Id);}
 bool TestRadioOff();
 void TestReset(){Outcome=EOutcome::None;}
 void TestTakeBack(){TakeKeysBack();}
 // A topic's or a bark's condition: a thing found, or how far things have gone. Public for the self-test.
 bool Meets(const TCHAR* Needs) const;
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<AS47Web> Scene;
 TSet<FString> Seen,HeardOf;
 TArray<FString> Tray;
 int32 OpensDay=1,AssessDay=FirstAssess,WeekNoted=0,RadioOffDay=-1,NoraState=0,DaleDay=0;
 EOutcome Outcome=EOutcome::None;
 bool bOlder=false,bLeft=false,bTest=false,bPayReturned=false,bClosed=false,bStowed=false;
 int32 CloseDay=0;
 void StepClosing();
 void Close();
 void StepWarnings();
 int32 LeftDay=0;double LeftClock=0;
 double NewsIn=8,RingLeft=0,NoraNext=0;
 bool bLetterTold=false,bRinging=false,bRadioLoop=false;
 FString ToastSeen;double ToastAt=0;
 FString BackTo; // reopened on the next tick: not from inside the panel's own closing
 // In the world.
 struct FSpot
 {
  FString Id;double X=0,Z=0;bool bSeated=false;
  TArray<TWeakObjectPtr<USceneComponent>> Parts;TArray<FVector> Local;TArray<FRotator> Turn;FS47Obj Hit;
 };
 TArray<FSpot> Land;
 FSpot WagonSpot;
 TArray<TWeakObjectPtr<USceneComponent>> Shown,Hidden;
 FS47Obj LockerObj,LockerTwoObj,BoardObj;
 TArray<FS47Obj> LockerParts,LockerTwoParts,BoardParts;
 bool bBuiltSaro=false,bBuiltLand=false,bBound=false,bBoundLand=false;
 bool BuildSaro();
 bool BuildLand();
 void Bind();
 void BindOne(const FString& Id,const FS47Obj& Obj,double Range,TFunction<FString()> Label,TFunction<void()> Use);
 double GroundAt(double X,double Z,const FSpot& Spot,bool& bHit) const;
 void Seat(FSpot& Spot);
 void SeatWagon();
 void StepRadio();
 void StepNews(double Dt);
 void StepRing(double Dt);
 void Hear(const FString& Id);
 // Dale's next line that is due, if any; marked said by whoever delivers it.
 const S47PredecessorText::FBark* NextBark() const;
 void StepDale();
 // The walkie (not saved: after a Continue he simply calls again). How often he has answered today, when he last did, whether
 // a hail of his went unanswered, when he tries again, and which stretch of lost time he has already said something about.
 int32 DaleAnswered=0,DaleAnsweredDay=0,DaleMisses=0,DaleMissDay=0,DaleHoursTold=0;
 double DaleLastClock=-1e9,DaleRetryClock=0;
 bool bDaleMissed=false;
 void SayBark(const FString& Id);
 void MissedHail();
 void Mark(const FString& Id,const FString& Summary);
 FString DateText(int32 Day) const;
 FString LetterText() const;
 FString OutcomeLetter(EOutcome O) const;
 FString AssessKey() const{return FString::Printf(TEXT("ward-assessment-%d"),AssessDay);}
 void Decided();
 void ShowDoc(const FS47Doc& Doc,const FString& Back);
 FS47Doc MakeDoc(const FString& Id) const;
 void ShowTalk(const FString& Who,const FString& TopicId);
 void Leave();
 void TakeKeysBack();
 void ToTitle(bool bNextOperator);
 static TArray<FString> AllText();
};
