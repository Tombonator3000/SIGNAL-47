#pragma once
#include "CoreMinimal.h"

class US47Main;
class FS47DevEvents;
class FS47FreeformSession;
class SWidget;

// F10 in Freeform (Tom, 7 November: "F10 must be able to test everything"). Every event starts from its own line: the clock and the
// place are set so that it can be seen, the screen says what it does, and a refusal says why. Natural() runs the night by its own
// rules instead, the ones a developer run otherwise switches off, with the clock at speed, so that whole nights can be watched.
// Nothing here is saved, and the test hooks it uses are the ones the scenario tests use.
class FS47DevFreeform
{
public:
 explicit FS47DevFreeform(US47Main* InM):M(InM){}
 ~FS47DevFreeform();
 void AddEntries(FS47DevEvents& Events);
 void Step(double Dt);
 void Stop();
 // Whatever is running is over, at once.
 void ClearRunning();
 // The line above the menu.
 FString MenuNote() const;
 bool Natural() const{return bNatural;}
 double SpeedNow() const{return SpeedUsed;}
 // The text of the panel on the screen.
 FString PanelText() const;
private:
 US47Main* M=nullptr;
 FS47DevEvents* Menu=nullptr;
 // ---- starting things ----
 // A window of hours: inside it the clock stays where it is, outside it the clock goes forward to From. From<0: as it is.
 struct FStage { double From=-1,To=-1; int32 Day=0; FString Area; TFunction<FVector2D()> Where; double Yaw=0,Pitch=0; bool bStay=false; };
 void Go(const FStage& Stage,TFunction<void()> Then);
 void After(double Seconds,TFunction<void()> Fn);
 void WaitFor(TFunction<bool()> Cond,double Timeout,TFunction<void()> Then,TFunction<void()> Else);
 void ToHour(double From,double To);
 void AimAt(const FVector& At);
 void Tell(bool bStarted,const FString& Text,float Seconds=7);
 void StartDirector(const FString& Id,const FStage& Stage,bool bAim,const FString& Say,TFunction<void()> Before=nullptr);
 FString WhyRefused(const FString& Id) const;
 FS47FreeformSession* Session() const;
 bool DevRun() const;
 void AddDirector(FS47DevEvents& E);
 void AddIndoors(FS47DevEvents& E);
 void AddSites(FS47DevEvents& E);
 void AddHighway(FS47DevEvents& E);
 void AddTaken(FS47DevEvents& E);
 void AddAfter(FS47DevEvents& E);
 void AddRunNight(FS47DevEvents& E);
 void AddTimePlace(FS47DevEvents& E);
 void AddLand(FS47DevEvents& E);
 void AddReyes(FS47DevEvents& E);
 void GoReyes(const FString& Id,const FString& Say);
 void ToLandPlace(const FString& Id,double Off,double From,double To,TFunction<void(double,double)> Then);
 void StartHighway(bool bNorth,bool bFollow);
 void StartRoomLight();
 void StartDoor(const FString& Place,bool bLight);
 void StartPeek(const FString& Place);
 void StartFieldTest(int32 Test);
 void PlaceRoom(const FString& Place,TFunction<void()> Then);
 struct FWait { double Left=0; TFunction<void()> Fn; TFunction<bool()> Cond; TFunction<void()> Else; };
 TArray<FWait> Waiting;
 bool bLastOk=true;
 FString LastText;
 // ---- the night by its own rules ----
 void SetNatural(bool bOn);
 void StepNatural(double Dt);
 FString HoldFor() const;
 void ShowPanel(bool bShow);
 bool bNatural=false,bHold=true,bPanelWanted=false,bPanelFull=false,bFlagRead=false;
 double Speed=1,SpeedUsed=1;
 double ScheduleHeld=0; // a line has set the Director's schedule by itself for this long
 FString Holding;
 TSharedPtr<SWidget> Panel;
 // ---- every line in turn, for a log: -S47DevSweep=/tmp/dir ----
 void StepSweep(double Dt);
 bool bSweepInit=false;
 FString SweepDir,SweepOnly;
 int32 SweepIndex=0,SweepGood=0,SweepBad=0;
 double SweepWait=0;
 bool bSweepPending=false;
};
