#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "Widgets/SWidget.h"
class US47Main;
class AS47Web;
class USceneComponent;
class UTextRenderComponent;
struct FS47Doc;
struct FS47MapView;
class FS47DaleVoice;

// Freeform only (Tom, 7 November: "Forrige nattvakt: Reyes fra Story mode"). Why be at SARO, why build it up, why not just leave:
// Ward's review, thirty days out; the operator before you, whose locker, notes and survey tags are still here; Mara Ortiz's wagon on
// the flat; and a board by the lockers where you can hang your keys and go. Everything is the outside of what happened: text in
// S47PredecessorText.cpp, and its self-test refuses the far side. Saved under "predecessor"; a save from before it picks it up
// on load, its own calendar untouched, with a letter that says so. Nothing here takes away an original or a photograph.
namespace S47PredecessorText{struct FBark;}
class FS47Predecessor;
// -S47Freeform -S47AssessMonthTest=routine|investigator|taken|taken-again|cases, with -UserDir=/tmp/...: thirty days through the real StepNews, the
// post, the letter and the closing (S47AssessMonthTest.cpp). Does nothing without the flag.
struct FS47AssessMonthTest{static void Step(FS47Predecessor& P,double Dt);};
class FS47Predecessor
{
 friend struct FS47AssessMonthTest;
public:
 explicit FS47Predecessor(US47Main* Main):M(Main){}
 enum class EOutcome:uint8{None,Stays,KnowsEnough,ClosedOriginals,ClosedNothing,Gone};
 // What the assessment goes by: only what really happened in this game.
 // Clean: filed jobs whose recording was tuned well enough to read (critic, 8 November: quality and cases, not a count of cards).
 // Land: kinds of record kept from the land at night (the night's events, FS47Happenings), which count a little toward staying.
 // Logs: night logs her office has initialed (FS47NightLog, NIGHTLOG-20261108): a station that reports its nights counts a little.
 // Tapes: cassettes played back with something on them nobody heard out there (FS47Tape, TAPE-20261108): what she can send on.
 // Trust: what her office makes of the weekly reports (FS47WardReport, WARD-REPORT-20261108): whole ones raise it, a lie found out lowers it.
 // M7 (9 November): Concluded counts only a line with something behind it (FS47FreeformCases::Supported); a mystery filed on
 // nothing is Guessed, and counts for nothing. Photos: prints made in the camera, not enlargements of them. Reprieved: the times
 // taken when "Not yet" gave the nights back; only a taking after that sends somebody else.
 struct FTally{int32 Filed=0,Clean=0,Concluded=0,Dishes=27,Arrears=0,Originals=0,Photos=0,Known=0,Taken=0,Land=0,Logs=0,Tapes=0,Trust=0,Guessed=0,Reprieved=0,Field=0;bool bWagon=false,bGap=false;};
 // CHAIN-2-20261109: Field, of the cases carried through, those with something brought in from out there (FS47FreeformSession::
 // CaseFromField); FieldToStay of them, or the station is not kept, however good the rest. The Monday note has the line every week.
 static constexpr int32 FieldToStay=1;
 // CasesToStay: routine alone does not keep the station (M7): the county wants at least one case carried through. LogsForPoint: the
 // night sheets count once, a little. CarriedWarn: from here Ward names the limit of the carried line in her own words.
 static constexpr int32 FiledFloor=10,PointsToStay=6,KnownEnough=10,TakenTooOften=3,FirstAssess=30,NoticeDays=7,CasesToStay=1,LogsForPoint=15,CarriedWarn=40,InspectorAfter=21,PayReplyDays=3;
 static constexpr double CleanQuality=.6;
 // M7, weeks three and four (S47PredecessorLate.cpp): the late cases from LateCaseDay, somebody gone from a post in town between
 // PostFrom and PostUntil when nobody has gone since AwayRest days, the Sky Watch's bulletin from LateCaseDay (from BulletinBy
 // whatever the van saw), wagon 23's trip meter from LateTripDay, and the thermos's letter ThermosReplyDays after it went.
 static constexpr int32 LateCaseDay=12,PostFrom=15,PostUntil=20,AwayRest=7,BulletinBy=15,LateTripDay=15,ThermosReplyDays=3;
 // What the station can show the county: cases carried through, the array mended, records that can be read, a few photographs,
 // and the work, which counts once it is there and no more for being piled up.
 static int32 Points(const FTally& T);
 static EOutcome Decide(const FTally& T);
 // M7: the third stretch of lost time is answered the morning after it is noticed (TakenWarning said so), not on the assessment day;
 // after "Not yet" only a further one does it. NoticedDay: the day it was first seen (0: not yet).
 static bool GoneDue(int32 Taken,int32 Reprieved,int32 NoticedDay,int32 Today){return Taken>=TakenTooOften&&Taken>Reprieved&&NoticedDay>0&&Today>NoticedDay;}
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
 // Whether this paper of hers waits unread in the OPS tray (ReadDoc takes it out).
 bool InTray(const FString& Id) const{return Tray.Contains(Id);}
 bool RadioOn() const;
 // The closing order has taken effect: the county power is off, the array stowed, the truck the district's.
 bool StationClosed() const{return bClosed;}
 int32 ClosingDay() const{return CloseDay;}
 // The assessment letter has been read: until then OPS says only that it is made (M7: the status does not give it away).
 bool OutcomeRead() const{return bOutcomeRead;}
 // SARO kept open and the letter read: the station's own finale has happened or is due (StepKept).
 bool KeptOpen() const{return Outcome==EOutcome::Stays&&bOutcomeRead;}

 // OPS: what lies in the tray, the keys, and the station's status. SARO: UNDER REVIEW.
 // More: the session's own papers for the same tray (CASES-OPS-20261109: the morning's case replies), after Ward's and Reyes's.
 void OpsRows(TArray<TSharedRef<SWidget>>& Rows,const TArray<TSharedRef<SWidget>>& More=TArray<TSharedRef<SWidget>>());
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
 void TestReset(){Outcome=EOutcome::None;bOutcomeRead=false;}
 // M7, weeks three and four (S47PredecessorLate.cpp), for F10 "Late cases (M7)": "post" somebody in Caliche gone now, "bulletin" the
 // Sky Watch's sheet in the box now, "minute" its burst due a minute from now, "trip" wagon 23 as on day 15 with the call-out read,
 // "thermos" the attendant's talk had, "reply" the thermos's letter due now. Why: what was done. False: not in a Freeform game.
 bool TestLate(const FString& What,FString& Why);
 // Where a late case's place is now (F10's Go): "post" the post of whoever is gone, "cairn", "rack". False: nowhere to go.
 bool LateSpot(const FString& What,FString& OutArea,double& OutX,double& OutZ) const;
 // The world clock of the Sky Watch's printed minute; 0 until the bulletin is posted.
 double MinuteClock() const{return LateMinuteAt>0?LateMinuteAt:LateBulletinDay>0?double(LateBulletinDay+2)*86400.+50*60.:0;}
 // "Not yet" on the letter to the next operator, as the button does it.
 void TestNotYet(){NotYet();}
 // Critic round 5: "Not yet" is given once. After it, a further stretch of lost time ends it, as her Monday note says.
 bool OffersNotYet() const{return TakenAtReprieve==0;}
 // Critic round 5: while the array is run up after KEPT OPEN, a dish's own pointing (where it goes back to), for the save; false
 // when it is not one of the run, or there is none under way.
 bool SaluteTarget(int32 I,double& Az,double& El) const;
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
 // M7: the times taken at "Not yet", the day a third taking was noticed, and the day the Stays letter was read.
 int32 TakenAtReprieve=0,GoneNoticed=0,KeptDay=0;
 bool bOutcomeRead=false;
 // Critic round 5: a save from before M7 has no word of the reprieve or of the letter being read. Settled on the first Step,
 // once the post is loaded (SettleLegacy): its "Not yet" kept, its letter not yet taken out of the box unread, its KEPT OPEN finale.
 bool bLegacyReprieve=false,bLegacyRead=false;
 void SettleLegacy(int32 Taken,bool bAssessmentPosted,int32 Today);
 EOutcome Outcome=EOutcome::None;
 bool bOlder=false,bLeft=false,bTest=false,bPayReturned=false,bClosed=false,bStowed=false;
 int32 CloseDay=0;
 void StepClosing();
 void Close();
 void StepWarnings();
 // M7: after KEPT OPEN. The reverse of Close and StepClosing: the rack clicks over and the whole array is run up and back down from
 // Dale's bench, once, the first quiet time at the station; the second locker gets a name; a reply to the routed pay comes by post.
 void StepKept(double Dt);
 struct FSaluteDish{int32 I=0;double Az=0,El=0;};
 TArray<FSaluteDish> Salute;double SaluteTime=0;int32 SaluteStage=0;
 TWeakObjectPtr<UTextRenderComponent> KeptLabel;
 // M7, weeks three and four (S47PredecessorLate.cpp): the late cases' traces out in the world (FS47FreeformCases' PostLine, MinuteLine
 // and RakedLine), the Sky Watch's bulletin, somebody gone from a post in town when nobody has gone by then, and unit 23's thermos
 // on its way back to Ortiz's family. Saved: the bulletin's postmark, the day the thermos went, the day a post was emptied and the
 // last day somebody was seen gone. Not saved: the sound at a place, which starts again where the player is.
 void StepLate(double Dt);
 void StepLatePost(int32 Today,double Hour);
 void StepLateMinute(int32 Today);
 void StepLateRaked(int32 Today);
 void StepLateRack();
 void StepLateThermos(int32 Today);
 // Keeps a late line's trace (its text from the case's seeded reading) with what the player saw or heard. False: kept already.
 bool LateKeep(int32 Line,int32 Clue,const FString& Toast);
 void LateSound(const FString& Name,const FVector& At,float Gain);
 void LateQuiet();
 bool InControlRoom() const;
 FString LateAway() const; // who in Caliche is gone now (FS47Townsfolk), empty: nobody
 int32 LateBulletinDay=0,LateThermosDay=0,LatePostDay=0,LateAwayDay=0;
 // F10 (TestLate): the day gates let go, and the printed minute set to a clock of its own. Neither is saved.
 double LateIn=0,LateRackAt=-1,LateMinuteAt=0;
 bool bLateLoop=false,bCairnHeard=false,bLateNow=false;
 FString LateLoopName;
 void OpenKept();
 void NotYet();
 // The day the first assessment was set for: a reprieve never comes back earlier than it.
 int32 FirstAssessDay() const{return bOlder?OpensDay+FirstAssess:FirstAssess;}
 // June 30, 1986, the end of the fiscal year the station is kept through, as a day of this game.
 int32 FiscalEndDay() const;
 // What you leave behind and how far you went into Reyes: OpenLeft's account, shared by the page after KEPT OPEN.
 // bStaying: told to the operator who stays, not the one who goes.
 void Account(TArray<TSharedRef<SWidget>>& Rows,const FTally& C,float Width,bool bStaying=false) const;
 // Ward's word on each thing she reads (M7: no lever she counts goes unsaid).
 FString LeverWords(const FTally& C) const;
 // The title, and what the button says about the save: away from the building nothing is filed (M7-E, S47SaroSave).
 bool AwayFromStation() const;
 FString TitleButton() const; int32 LeftDay=0;double LeftClock=0;
 double NewsIn=8,RingLeft=0,NoraNext=0;
 // bSawTutorial: the first shift ran in this sitting, so the HUD's card comes in with the letter when it is done (critic round 5:
 // the toast said it a third time).
 bool bLetterTold=false,bRinging=false,bRadioLoop=false,bSawTutorial=false;
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
 // DALE-WIRE-20261109: Dale's own takes on the service line (FS47DaleVoice, "phone"), each line of his that the page shows, one
 // after another; stopped when the receiver goes down (the page closed or put down) or another page of his replaces it. Lines with
 // no take are the page's text, as before. Not saved, and it marks nothing as heard.
 TSharedPtr<FS47DaleVoice> Phone;
 void SpeakPhone(const TArray<FString>& Lines);
 void Leave();
 void TakeKeysBack();
 void ToTitle(bool bNextOperator);
 static TArray<FString> AllText();
};
