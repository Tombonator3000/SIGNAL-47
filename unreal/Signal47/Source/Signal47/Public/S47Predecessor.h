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
class FS47Predecessor
{
public:
 explicit FS47Predecessor(US47Main* Main):M(Main){}
 enum class EOutcome:uint8{None,Stays,KnowsEnough,ClosedOriginals,ClosedNothing,Gone};
 // What the assessment goes by: only what really happened in this game.
 struct FTally{int32 Filed=0,Concluded=0,Dishes=27,Arrears=0,Originals=0,Photos=0,Known=0,Taken=0;bool bWagon=false,bGap=false;};
 static constexpr int32 FiledEnough=25,FiledWithCases=15,CasesEnough=2,KnownEnough=10,TakenTooOften=3,FirstAssess=30;
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
 int32 TrayCount() const{return Tray.Num();}
 bool RadioOn() const;

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
 void ReadDoc(const FString& Id);
 void FillMap(FS47MapView& View) const;
 // World position (x,z) of a thing out on the land or at the station, for the developer's menu and the tests.
 bool WorldSpot(const FString& Id,FString& OutArea,double& OutX,double& OutZ) const;

 // Developer's menu and tests: nothing here is saved in a developer run.
 void TestOn();
 void TestLetter();
 void TestWeek();
 void TestRing();
 void TestOutcome(EOutcome O);
 void TestLeave();
 void TestNextDay();
 void TestAssessNow();
 void TestFound(const FString& Id);
 void TestHear(const FString& Id){Hear(Id);}
 bool TestRadioOff();
 void TestReset(){Outcome=EOutcome::None;}
 void TestTakeBack(){TakeKeysBack();}
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<AS47Web> Scene;
 TSet<FString> Seen,HeardOf;
 TArray<FString> Tray;
 int32 OpensDay=1,AssessDay=FirstAssess,WeekNoted=0,RadioOffDay=-1,NoraState=0;
 EOutcome Outcome=EOutcome::None;
 bool bOlder=false,bLeft=false,bTest=false,bPayReturned=false;
 int32 LeftDay=0;double LeftClock=0;
 double NewsIn=8,RingLeft=0,NoraNext=0;
 bool bLetterTold=false,bRinging=false,bRadioLoop=false;
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
 void Mark(const FString& Id,const FString& Summary);
 FString DateText(int32 Day) const;
 FString LetterText() const;
 FString OutcomeLetter(EOutcome O) const;
 FString AssessKey() const{return FString::Printf(TEXT("ward-assessment-%d"),AssessDay);}
 void Decided();
 void ShowDoc(const FS47Doc& Doc);
 FS47Doc MakeDoc(const FString& Id) const;
 void ShowTalk(const FString& Who,const FString& TopicId);
 void Leave();
 void TakeKeysBack();
 void ToTitle(bool bNextOperator);
 static TArray<FString> AllText();
};
