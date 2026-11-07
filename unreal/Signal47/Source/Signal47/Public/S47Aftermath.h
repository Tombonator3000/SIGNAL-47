#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Freeform only: small things that are a little wrong after a night with them, and put themselves right. Nothing here
// moves the clock, a job or a payment, and nothing here can hold anything up. Without one of these nights the save has no
// field at all, and everything is as it always was.
//
// The workstation's clock stands at the last time the player is sure of, until they have been on OPS a few seconds or a
// day has gone. A cow is gone from the pasture from the next morning, for a few days. A line sits in the player's own
// notes that they did not write: no slip, no sound, and no word about it ever.
class FS47Aftermath
{
public:
 explicit FS47Aftermath(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 // Kind as FS47Taken has it: 0 Roswell, 1 crab, 2 tall. SafeClock is the last time they are sure of.
 void AfterTaken(int32 Kind,double SafeClock);
 void AfterFarmVisit(double Clock);

 bool PcStuck() const{return PcAt>=0;}
 double PcShown() const{return PcAt;}
 bool CowOut() const{return bCowOut;}
 bool CowPlanned() const{return CowFrom>0;}
 FString CowFindingId() const;
 const FString& LastLine() const{return Line;}

 // What is decided, apart from the game. Have: the notes as they stand; a line already there is not written twice.
 static FString NoteFor(int32 Kind,double SafeClock,float Roll,const TArray<FString>& Have);
 // The morning after a night that ended at this clock: 06:00, or now if it is already past that. Evenings: the next day's.
 static double NextMorning(double Clock);
 static bool SelfTest(TArray<FString>& Lines);

 static constexpr double LogOnSeconds=6,PcDays=1;
 static constexpr float NoteChance=.4f,CowChance=.5f;
 static constexpr int32 MaxCows=2;
 // Tests: the dice, -1 for the usual. A developer run draws none unless a test asks, so older fixtures stay as they were.
 void TestDice(float Note,float Cow){TestNote=Note;TestCow=Cow;}
private:
 US47Main* M=nullptr;
 double PcAt=-1,PcUntil=0,LogOn=0;
 double CowFrom=0,CowUntil=0;
 bool bCowOut=false,bCowNoted=false;
 int32 Cows=0;
 FString Line;
 float TestNote=-1,TestCow=-1;
 float Roll(float Test) const;
 void StepCow(double Clock);
 bool CowWatched() const;
};
