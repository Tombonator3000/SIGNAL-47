#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

class US47Main;
class FS47FreeformSession;

// M7 (Tom, 9 November): what the money buys has to open the land, not only make it comfortable. The station's own truck made better,
// not a new one: mud-and-sand tires (grip and speed on gravel and dirt, slower to dig in), a bed rack (a bigger parcel), an auxiliary
// tank (six units, about 72 km between pumps) and an engine rebuild (more pull and a higher top, a tenth more fuel). And field gear
// that brings more back: the better field recorder (FS47Tape hears twice as far), the image intensifier on the camera
// (US47FieldCamera), the direction finder loop on the cab (bearings to what keys the band within about 3 km, out at night). All are
// FS47Openings offers, bought at OPS and kept by name with the session's kit. Near what is out there, gear can give out: the
// flashlight's cell cooks, the recorder's heads magnetise; the repair goes on the next weekly sheet (FS47FreeformSession::BreakGear).
// Freeform only. Saved: the night gear was last rolled for (CRIT6-20261109: at most once a night holds across a Continue).
struct FS47TruckTune
{
 double Pull=1,Top=1,LooseGrip=1,LooseTop=1,Bog=1,Burn=1,Tank=4;
 int32 Load=8;
};

class SIGNAL47_API FS47Outfit
{
public:
 explicit FS47Outfit(US47Main* Main):M(Main){}
 // The session's Begin (a new game: Saved is empty, nothing rolled yet) and Save.
 void Begin(const S47J::FObj& Saved);
 S47J::FObj Save() const;
 static constexpr double StockTank=4,BigTank=6;
 static constexpr int32 StockLoad=8,RackLoad=20;
 // The truck as these upgrades leave it (pure).
 static FS47TruckTune TuneOf(bool bEngine,bool bTires,bool bTank,bool bRack);
 // The truck the session has (stock with no session).
 static FS47TruckTune TuneFor(const FS47FreeformSession* F);
 // Each frame after the session: the truck's tune onto the cab's controller, the direction finder, and gear giving out near something.
 void Step(double Dt);
 // Out of Freeform: the cab's truck as stock again.
 void Stop();
 // The direction finder: how far it hears (metres) and how often it is read (seconds), and its line for a source at this distance.
 static constexpr double RdfReach=3000,RdfEvery=25;
 static FString RdfLine(double Metres,const TCHAR* Way);
 // Gear gives out near something dangerous (FS47Omen::Level at or over BreakLevel), at most once a night, with this chance. Never
 // in the big night (a presence, never a danger) or while a director script plays: no roll then, and the night is not used up.
 static constexpr double BreakLevel=.55;
 static constexpr float BreakChance=.35f;
 static bool BreaksNow(double Level,int32 Night,int32 LastRolled,float Roll){return Level>=BreakLevel&&Night!=LastRolled&&Roll<BreakChance;}
 // Whether the roll is asked at all now: bAuthored, the big night or a script under way, holds it back.
 static bool RollsNow(double Level,int32 Night,int32 LastRolled,bool bGearOn,bool bAuthored){return !bAuthored&&bGearOn&&Level>=BreakLevel&&Night!=LastRolled;}
 int32 LastRolledNight() const{return RolledNight;}
 // F10: the truck's numbers now and as stock (top speed and 0 to 50 mph on the blacktop and on dirt, the tank, the parcel).
 static FString Compare(const FS47TruckTune& Now);
 static bool SelfTest(TArray<FString>& Lines);
 // F10: read the direction finder at the next step.
 void DevRdfNow(){RdfIn=0;}
private:
 US47Main* M=nullptr;
 double RdfIn=5;
 int32 RolledNight=0;
};
