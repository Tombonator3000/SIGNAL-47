#pragma once
#include "CoreMinimal.h"

class US47Main;
class FS47DevEvents;
class FS47FreeformSession;
class SWidget;
struct FS47PlaceNeed;

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
 // The mailbox at the road and the post (S47Mail.h): the carrier now, the flag, fetching, the paper, a parcel; S47MailDev.cpp.
 void AddMail(FS47DevEvents& E);
 // M7: the late cases and Reyes's trail after wagon 23 (S47PredecessorLate.cpp); S47LateCasesDev.cpp.
 void AddLateCases(FS47DevEvents& E);
 // Night events (S47Happenings.h), each from its own line; S47HappeningsDev.cpp.
 void AddHappenings(FS47DevEvents& E);
 // M2: the gear failing, the night's sounds and their silence, the animals (S47Omen.h and its readers); S47OmenDev.cpp.
 void AddOmens(FS47DevEvents& E);
 // M3: the survey map's accuracy place by place, and the night's typed log (MAP-20261108, NIGHTLOG-20261108); S47FieldMapDev.cpp.
 void AddMapLog(FS47DevEvents& E);
 // M4: the temporary boxes and Caliche's counters (NEW-PLACES-20261108, TOWN-USE-20261108); S47TownDev.cpp.
 void AddTown(FS47DevEvents& E);
 // M5: the day's weather on the world and the hours awake (WEATHER-20261108, FATIGUE-20261108); S47WeatherDev.cpp.
 void AddWeather(FS47DevEvents& E);
 // M5: loose things picked up, carried, thrown and heard, and the cot at SARO (PHYSICS-20261108, SARO-COT); S47PhysicsDev.cpp.
 void AddPhysics(FS47DevEvents& E);
 // M6: what goes into SARO's walls, what stops one with nothing drawn, the street lamps' feet, and the views of Dale's window, the
 // substation and the lamps (SARO-CLIP, DALE-WINDOW, FENCES-INVISIBLE, STREETLAMPS-20261108); S47LooksDev.cpp.
 void AddLooks(FS47DevEvents& E);
 // M7 (Tom, 9 November): money, the stores list and the truck's upgrades in numbers (the loop money, upgrades, going out); S47EconomyDev.cpp.
 void AddEconomy(FS47DevEvents& E);
 // ALIEN-EYES (Tom, 9 November): the faint dot in every alien's eyes, each body at night at 5, 15, 40 and 55 m, by day, close up,
 // and switched off; S47EyeGlintDev.cpp. EyeGlintClear puts its body away when another line starts.
 void AddEyeGlint(FS47DevEvents& E);
 TFunction<void()> EyeGlintClear;
 // TRIPO-NEW (Tom's nine Tripo models of 9 November): each by day and night in the world, all nine in a row, the aliens' eyes close;
 // a stand-in where a private model is missing. S47TripoNewDev.cpp. TripoNewClear puts them away when another line starts.
 void AddTripoNew(FS47DevEvents& E);
 TFunction<void()> TripoNewClear;
 // T16 (Tom's test, 7 November): how far one gets from the farm every way on foot and at the wheel, and what stops it; the sky's
 // formations and preview, the highway both ways, the Pale One, each by the menu's own line and read back; S47ReachDev.cpp.
 void AddReach(FS47DevEvents& E);
 // PT-MOON, PT-ROCKS, PT-RANGE-FAR (Tom's test of d36, 9 November): the moon at its phases, by day and behind clouds, before and now;
 // the roadside rocks group by group from the road and from behind, before and now, with and without ray tracing; the far ranges
 // from the highway, wide and close. S47WorldFixesDev.cpp. WorldFixesClear gives the moon and the rocks back when another line starts.
 void AddWorldFixes(FS47DevEvents& E);
 TFunction<void()> WorldFixesClear;
 float SavedFov=-1; // the view's own while a World fixes line looks close
 // Tom's test of d36 at SARO: the ramp, walks, stop sign, hall, switches, Dale's light and records, before and after; S47SaroFixesDev.cpp.
 void AddSaroFixes(FS47DevEvents& E);
 // FARM-LIGHTS (Tom, 9 November): the barn's swinging bulb, the yard's lamp and the old house's lit room; S47FarmLightsDev.cpp.
 void AddFarmLights(FS47DevEvents& E);
 // WEEKLY-CASE-20261109: "Week case (T)": start now, way out A to the morning, stop and clean (S47WeekCaseDev.cpp).
 void AddWeekCase(FS47DevEvents& E);
 void AddTownDay(FS47DevEvents& E); // NPC-DAY: "Town day (T)" (S47TownDayDev.cpp)
 // Every event at a place from the list (S47EventPlaces), the one chosen on the menu or the draw's own (Tom, 8 November).
 void AddAnywhere(FS47DevEvents& E);
 void ToPlace(const FString& Id,double Off,double From,double To,TFunction<void()> Then);
 // The chosen place if it fits this need, else where the game's own draw puts it from here. Empty: nothing fits.
 FString ChoosePlace(const FS47PlaceNeed& Need) const;
 FString ChosenPlace;
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
 // -S47NaturalStand=house|field: where the player stands each night from 21:00, as a player might choose to; nothing is started.
 void StepStand();
 FString StandPlace;
 int32 StandNight=0;
 // -S47NaturalStand=tour: through each night the player goes where a player would be (control room, Caliche, the wheel, the farm,
 // the open land), so that what fits each place can come by itself. Nothing is started or cleared.
 void StepTour();
 int32 TourNight=0,TourLeg=-1;
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
