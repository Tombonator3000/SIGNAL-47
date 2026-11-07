#include "S47Aftermath.h"
#include "S47Audio.h"
#include "S47Flow.h"
#include "S47Freeform.h"
#include "S47Herd.h"
#include "S47Main.h"
#include "S47Story.h"
#include "S47TownLayer.h"
#include "S47UI.h"
#include "S47Walker.h"
#include "S47Web.h"
#include "CoreGlobals.h"
#include "UObject/GarbageCollection.h"

namespace
{
 constexpr double AftermathDay=86400;
 constexpr double CowOutOfSight=90; // metres from the middle of the pasture: nothing changes where it could be watched
 constexpr double CowCloseEnough=40;
}

FString FS47Aftermath::NoteFor(int32 Kind,double SafeClock,float InRoll,const TArray<FString>& Have)
{
 if((Kind!=1&&Kind!=2)||InRoll>=NoteChance)return FString();
 // In the player's own hand, and each one tidies away something their field record says cannot be explained.
 TArray<FString> Pool;
 if(Kind==1)Pool={TEXT("Fell on the way back. Nothing broken."),TEXT("Walked it off. Did not need the truck."),TEXT("Stay off the soft ground.")};
 else
 {
  const int32 S=int32(FMath::Fmod(FMath::Max(0.,SafeClock),AftermathDay));
  Pool={FString::Printf(TEXT("Back at SARO by %02d:%02d. Nothing to report."),S/3600,(S/60)%60),TEXT("Tied them myself."),TEXT("Parked it straight this time.")};
 }
 Pool.RemoveAll([&Have](const FString& Text){return Have.Contains(Text);});
 if(Pool.IsEmpty())return FString();
 return Pool[FMath::Clamp(int32(InRoll/NoteChance*Pool.Num()),0,Pool.Num()-1)];
}

double FS47Aftermath::NextMorning(double Clock)
{
 const double Start=FMath::FloorToDouble(Clock/AftermathDay)*AftermathDay,Time=Clock-Start;
 return Time<6*3600?Start+6*3600:Time<12*3600?Clock:Start+AftermathDay+6*3600;
}

float FS47Aftermath::Roll(float Test) const
{
 if(Test>=0)return Test;
 return M&&M->Flow&&!M->Flow->bDevRun?FMath::FRand():1.f;
}

FString FS47Aftermath::CowFindingId() const{return FString::Printf(TEXT("farm_cow_%d_visit"),int32(CowFrom/AftermathDay));}

void FS47Aftermath::Begin(const S47J::FObj& Saved)
{
 Line.Empty();LogOn=0;
 PcAt=Saved.IsValid()?S47J::Num(Saved,TEXT("pcAt"),-1):-1;PcUntil=S47J::Num(Saved,TEXT("pcUntil"));
 if(!FMath::IsFinite(PcAt)||!FMath::IsFinite(PcUntil)||PcAt<0||PcUntil<=PcAt)PcAt=-1;
 CowFrom=S47J::Num(Saved,TEXT("cowFrom"));CowUntil=S47J::Num(Saved,TEXT("cowUntil"));
 if(!FMath::IsFinite(CowFrom)||!FMath::IsFinite(CowUntil)||CowFrom<=0||CowUntil<=CowFrom)CowFrom=CowUntil=0;
 bCowOut=CowFrom>0&&S47J::Bool(Saved,TEXT("cowOut"));bCowNoted=S47J::Bool(Saved,TEXT("cowNoted"));
 Cows=FMath::Max(0,S47J::Int(Saved,TEXT("cows")));
}

void FS47Aftermath::Stop()
{
 // Story's cattle stand where Story left them. Not while the owner or the scene is going away.
 const bool bLive=!IsEngineExitRequested()&&!IsGarbageCollecting()&&IsValid(M)&&!M->HasAnyFlags(RF_BeginDestroyed|RF_FinishDestroyed)&&IsValid(M->TownLayer.Get());
 if(bLive)for(const auto& H:M->TownLayer->Herds)if(H)H->SetAway(-1);
}

S47J::FObj FS47Aftermath::Save() const
{
 const auto O=S47J::New();
 if(PcAt>=0){S47J::Set(O,TEXT("pcAt"),PcAt);S47J::Set(O,TEXT("pcUntil"),PcUntil);}
 if(CowFrom>0){S47J::Set(O,TEXT("cowFrom"),CowFrom);S47J::Set(O,TEXT("cowUntil"),CowUntil);if(bCowOut)S47J::Set(O,TEXT("cowOut"),true);if(bCowNoted)S47J::Set(O,TEXT("cowNoted"),true);}
 if(Cows>0)S47J::Set(O,TEXT("cows"),Cows);
 return O;
}

void FS47Aftermath::AfterTaken(int32 Kind,double SafeClock)
{
 if(!M||!M->Freeform||!FMath::IsFinite(SafeClock))return;
 PcAt=FMath::Max(0.,SafeClock);PcUntil=M->Freeform->Clock()+PcDays*AftermathDay;LogOn=0;
 Line.Empty();
 if(!M->Story)return;
 Line=NoteFor(Kind,SafeClock,Roll(TestNote),M->Story->Notes());
 // Past Note on purpose: no slip on the HUD. It is simply there the next time they read back.
 if(!Line.IsEmpty()){M->Story->NotesMutable().Add(Line);UE_LOG(LogTemp,Display,TEXT("S47: aftermath: a line in the notes: %s"),*Line);}
}

void FS47Aftermath::AfterFarmVisit(double Clock)
{
 // One at a time, and not often in one game: if every visit cost a cow it would be a rule, not something wrong.
 if(CowFrom>0||Cows>=MaxCows||Roll(TestCow)>=CowChance)return;
 CowFrom=NextMorning(Clock);CowUntil=CowFrom+FMath::FRandRange(2.5f,4.5f)*AftermathDay;bCowOut=bCowNoted=false;++Cows;
 UE_LOG(LogTemp,Display,TEXT("S47: aftermath: a cow from clock %.0f to %.0f"),CowFrom,CowUntil);
}

void FS47Aftermath::Step(double Dt)
{
 if(!M||!M->Freeform||!M->Freeform->Active())return;
 const double Clock=M->Freeform->Clock();
 if(PcAt>=0)
 {
  // Logging on is what puts it right: being at OPS, seeing it wrong for a moment, and the small click as it corrects.
  bool bDone=Clock>=PcUntil;
  if(!bDone&&M->Freeform->OpsOnGlass()&&!M->bPausedByMenu){LogOn+=Dt;bDone=LogOn>=LogOnSeconds;}
  if(bDone)
  {
   if(LogOn>=LogOnSeconds&&M->Audio)M->Audio->Sfx(TEXT("click"),FS47Sound{.Gain=.4f});
   PcAt=-1;PcUntil=LogOn=0;M->RequestAutosave();
   UE_LOG(LogTemp,Display,TEXT("S47: aftermath: the workstation clock is right again"));
  }
 }
 StepCow(Clock);
}

bool FS47Aftermath::CowWatched() const
{
 if(!M||!M->TownLayer||!M->Web||!M->Player)return false;
 const FString Here=M->Web->Area()==TEXT("diner")?FString(TEXT("road")):M->Web->Area();
 for(const auto& H:M->TownLayer->Herds)if(H&&H->Area==Here&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(H->Middle().X,H->Middle().Z))<CowOutOfSight)return true;
 return false;
}

void FS47Aftermath::StepCow(double Clock)
{
 if(CowFrom>0)
 {
  const bool bDue=Clock>=CowFrom&&Clock<CowUntil;
  if(bDue!=bCowOut&&!CowWatched())
  {
   bCowOut=bDue;
   if(!bCowOut){CowFrom=CowUntil=0;bCowNoted=false;}
   M->RequestAutosave();
   UE_LOG(LogTemp,Display,TEXT("S47: aftermath: the ninth cow is %s"),bCowOut?TEXT("gone"):TEXT("back"));
  }
  else if(!bCowOut&&Clock>=CowUntil){CowFrom=CowUntil=0;bCowNoted=false;M->RequestAutosave();}
 }
 if(!M->TownLayer)return;
 for(const auto& H:M->TownLayer->Herds)if(H)H->SetAway(bCowOut?S47Herd::MISSING:-1);
 if(!bCowOut||bCowNoted||!M->Web||!M->Player||M->ModalOpen())return;
 // Counted once, on the spot: the same words Story has for it. It is a record of a count, not of where she went.
 const FString Here=M->Web->Area()==TEXT("diner")?FString(TEXT("road")):M->Web->Area();
 for(const auto& H:M->TownLayer->Herds)if(H&&H->Area==Here&&FVector2D::Distance(FVector2D(M->Player->Pos.X,M->Player->Pos.Z),FVector2D(H->Middle().X,H->Middle().Z))<CowCloseEnough)
 {
  bCowNoted=true;
  M->Freeform->RecordEvidence(CowFindingId(),TEXT("The Alderete pasture. Eight head where there were nine. Tag 25 is not with them. Cattle stray. This records the count, not where she went."));
  if(M->UI)M->UI->Toast(TEXT("Eight of them. There were nine."),5);
  M->RequestAutosave();
  return;
 }
}

bool FS47Aftermath::SelfTest(TArray<FString>& Lines)
{
 bool OK=true;
 const auto Check=[&](bool B,const TCHAR* Why){OK&=B;Lines.Add(FString::Printf(TEXT("%s aftermath: %s"),B?TEXT("PASS"):TEXT("FAIL"),Why));};
 const TArray<FString> NoNotes;
 Check(NoteFor(0,3*3600,0,NoNotes).IsEmpty()&&!NoteFor(1,3*3600,0,NoNotes).IsEmpty()&&!NoteFor(2,3*3600,0,NoNotes).IsEmpty(),TEXT("only the crab and the tall one leave a line"));
 Check(NoteFor(1,3*3600,NoteChance,NoNotes).IsEmpty()&&NoteFor(2,3*3600,.99f,NoNotes).IsEmpty(),TEXT("most of the time they leave none"));
 Check(NoteFor(2,2*3600+40*60,0,NoNotes)==TEXT("Back at SARO by 02:40. Nothing to report."),TEXT("the tall one's line gives the time they were taken"));
 {
  // Each line once, and then none: however many nights, it never turns into a list.
  TArray<FString> Have;bool bFresh=true;int32 Written=0;
  for(int32 I=0;I<8;++I)
  {
   const FString L=NoteFor(1,0,.1f,Have);if(L.IsEmpty())continue;
   bFresh&=!Have.Contains(L);Have.Add(L);++Written;
  }
  Check(bFresh&&Written==3,TEXT("no line is written twice, and they run out"));
 }
 {
  // Short, and without an explanation: nothing in them names what happened.
  bool bPlain=true;
  for(const int32 Kind:{1,2})for(const float R:{0.f,.15f,.3f})
  {
   const FString L=NoteFor(Kind,0,R,NoNotes);
   bPlain&=!L.IsEmpty()&&L.Len()<=48;
   for(const TCHAR* Word:{TEXT("light"),TEXT("alien"),TEXT("taken"),TEXT("took"),TEXT("lost"),TEXT("remember")})bPlain&=!L.Contains(Word);
  }
  Check(bPlain,TEXT("the lines are short and explain nothing"));
 }
 Check(NextMorning(3*3600)==6*3600&&NextMorning(86400+5*3600)==86400+6*3600&&NextMorning(7*3600)==7*3600&&NextMorning(20*3600)==86400+6*3600,TEXT("the cow goes the morning after, whenever the night ended"));
 {
  // Nothing happened: nothing is saved, and an old save has nothing to come back to.
  FS47Aftermath Quiet(nullptr);Quiet.Begin(S47J::FObj());
  Check(Quiet.Save()->Values.Num()==0&&!Quiet.PcStuck()&&!Quiet.CowOut()&&!Quiet.CowPlanned(),TEXT("an old save or a quiet night has no such field"));
  const auto O=S47J::New();S47J::Set(O,TEXT("pcAt"),3600.);S47J::Set(O,TEXT("pcUntil"),90000.);S47J::Set(O,TEXT("cowFrom"),93600.);S47J::Set(O,TEXT("cowUntil"),300000.);S47J::Set(O,TEXT("cowOut"),true);S47J::Set(O,TEXT("cows"),1);
  FS47Aftermath Back(nullptr);Back.Begin(S47J::Read(S47J::Write(O)));
  Check(Back.PcStuck()&&Back.PcShown()==3600.&&Back.CowOut()&&S47J::Write(Back.Save())==S47J::Write(O),TEXT("a Continue brings back the clock and the cow as they were"));
  const auto Bad=S47J::New();S47J::Set(Bad,TEXT("pcAt"),-5.);S47J::Set(Bad,TEXT("cowFrom"),9.);S47J::Set(Bad,TEXT("cowUntil"),2.);S47J::Set(Bad,TEXT("cowOut"),true);
  FS47Aftermath Broken(nullptr);Broken.Begin(Bad);
  Check(!Broken.PcStuck()&&!Broken.CowOut()&&!Broken.CowPlanned(),TEXT("nonsense in a save means nothing is wrong"));
 }
 return OK;
}
