#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47DuskOrb.h"
#include "S47FreeformObservation.h"
class US47Main;
struct FS47AlbumShot;

// Freeform schedules contextual, bounded world events. Story's authored scenes remain separate.
class FS47FreeformDirector
{
public:
 // Nothing new for this long of active time: after being taken there is calm.
 void RestFor(double Seconds);
 // What the rhythm's rules read (S47Rhythm.h), straight, without a Save. And the night's count of dangerous meetings, saved.
 void FillRhythm(struct FS47RhythmNow& Out) const;
 void NoteDanger();
 int32 DangerTonight() const;
 // The hint on the player's objective line while lights are reported somewhere.
 FString TipLine() const;
 // Test: tonight is a night for lights, at the next scheduler tick.
 void TestLightsTonight();
 // Test: a natural event's whole life, without waiting for its dice. Force keeps the older fixture behaviour instead.
 bool TestBegin(const FString& Id);
 // Test: the save an event starts with fails, as a full disk would make it.
 void TestSaveFails(bool bFails){bTestSaveFails=bFails;}
 int32 LightsDrawnNight() const{return PendingLightNight;}
 void TestSchedule(float Dice,double LightHour,double Scale){TestDice=Dice;TestLightAfter=LightHour;TestScale=Scale;}
 // Test: an event's own age runs this much faster too (1: none). F10's natural night, which keeps its speed under an event with
 // nothing to see: without it a five-minute fragment in the yard lasted two days and closed both nights (krit9, 8 November).
 void TestEventPace(double Scale){TestPace=FMath::Clamp(Scale,1.,60.);}
 // How many active seconds a real one is just now (1 outside that run): for whatever else counts a while out in the night.
 double TestPaceNow() const{return TestPace;}
 // The event on the ground leaves the sky to the formations: nothing of it is up there or out in the dark. An instrument, a
 // fault, the fragment, the parked car (its light is FS47Taken's and asks for itself), the room light, a small thing.
 bool LeavesSkyFree() const;
 // Days before an event can come again, from the rhythm file's cooldown_days (two to four where it has none).
 int32 CoolFor(const FString& Id,int32 Last) const;
 // The night's mystery (FS47FreeformCases::MysteryLine). Test: its own dice, -1 for the usual. A developer run draws none
 // unless a test asks, so that the fixtures built on other events stay as they were.
 void TestMysteryDice(float Dice){MysteryDice=Dice;}
 // The empty car, rarely and only after two: a light stands over it (FF-RESEARCH-08). Test: its own dice, -1 for the
 // usual; a developer run draws none unless a test asks.
 void TestCarDice(float Dice){CarDice=Dice;}
 // The low lights' visits in the dark. A developer run keeps to the dusk unless a test asks, as with the dice above.
 void TestOrbNight(bool bOn){bTestOrbNight=bOn;}
 // Where one low light is on this visit's flight, and the stop it is holding at (-1 between them).
 FVector OrbAt(int32 Index=0)const{return FS47DuskOrb::Position(Anchor,Age,Index,DuskOrb.VisitSeed());}
 int32 OrbStopNow(int32 Index=0)const{return FS47DuskOrb::StopAt(Age,Index,DuskOrb.VisitSeed());}
 const FS47DuskOrb& DuskOrbState()const{return DuskOrb;}
 // Test: 0 as played; 1 the effect hidden, so that only the lights' own effect on the world shows; 2 that with the lights out.
 void TestOrbLightProbe(int32 Mode){OrbLightProbe=Mode;}
 // Test: how many of the orbs' point lights are on now, and how many of those cast shadows.
 void OrbLightCount(int32& On,int32& Shadowed,float& Lumens,float& Radius) const;
 // Where the photographic record of the first orb is now.
 FVector OrbRecordAt() const;
 bool CarLightDrawn() const{return bCarLight;}
 int32 CarLightStage() const{return CarStage;}
 int32 MysteryDrawnNight() const{return PendingMysteryNight;}
 int32 MysteryNoticeClue() const{return MysteryClue;}
 double MysteryNoticeAge() const{return MysteryAge;}
 // Whatever is at the other place can still be found.
 bool MysteryTrailOpen() const{return MysteryClearAt>0;}
 // A long rest crosses this night, to wake at about the clock At: its dice, as the scheduler would throw them. True only
 // if something that would really start then, and can be noticed, is drawn; the scheduler then looks again soon after waking.
 bool DrawForNight(int32 Night,double At);
 // Why the player cannot lie down now, in words for the player; empty when they can. Only what is close or after them.
 FString RestBlock() const;
 // Whatever else is going on ends when the player sleeps. A fault stays in the equipment.
 void EndForRest();
 // A task row while lights are reported somewhere, and while their marks stand. Empty otherwise.
 FString ReportedLine() const;
 // What the night's mystery still has to show, as a task row, while it can be found.
 FString MysteryTrailHint() const;
 explicit FS47FreeformDirector(US47Main* Main);
 ~FS47FreeformDirector();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Busy() const;
 bool Use(const FString& Id);
 FString Label(const FString& Id) const;
 void RoomLightInput(const FString& Room,int32 Serial,bool bOff);
 FString InspectRoomLight(const FString& Room);
 bool Force(const FString& Id); // explicit developer/test action; still needs a usable world
 bool ForceLights(const FString& Place); // town lights at one named place, same conditions as Force
 // F10: this event at this place from the list (S47EventPlaces), same conditions as Force. Place empty: the draw, as in play.
 bool ForceAt(const FString& Id,const FString& Place);
 // The place from the list the event under way was given; empty for one that has none (the station's own).
 const FString& EventPlace() const{return PlaceId;}
 // Where the draw would put this event now, without starting it: for F10 and its panel. Empty: nowhere fits.
 FString PreviewPlace(const FString& Id) const;
 FVector LightPlacePoint(const FString& Place) const;
 FString LightPlaceTitle(const FString& Place) const;
 struct FPhotoClaim { FString Id,PhotoId,Title,ControlOriginalId,ControlPhotoId,LocalNoiseOriginalId; double ControlPlannedAt=-1,LocalNoisePlannedAt=-1; int32 Reward=20; double Quality=0; bool bPaid=false; };
 const TArray<FPhotoClaim>& PhotoClaims() const { return Claims; }
 bool RegisterPhotoClaim(const FString& Id);
 bool PlanPhotoControl(const FString& Id);
 bool PlanLocalNoiseCheck(const FString& Id);
 FString LocalNoisePlanHint(const FString& Id) const;
 FString PhotoControlHint(const FString& Id) const;
 FString PhotoControlFieldHint(bool& bReady) const;
 bool FollowPhotoControl(const FString& Id);
 FString PhotoControlFocusId() const;
 bool CanRegisterPhotoControl(const FString& Id,const FString& ControlPhotoId) const;
 bool RegisterPhotoControl(const FString& Id,const FString& ControlPhotoId);
 void TagPhoto(FS47AlbumShot& Shot); // rendered exposures only, before album persistence
 void Photo(const FS47AlbumShot& Shot);
 double LastPhotoSubjectMean() const { return PhotoSubjectMean; } // last non-emissive exposure, -1 if no valid region

 const TArray<FS47FreeformObservation>& Observations() const { return FieldObservations; }
 // The live event's print, and the one waiting to be developed and registered.
 bool HoldsPhoto(const FString& Id) const { return Id==PhotoId||Id==TaggedShot; }
 void RetainObservation(const FS47FreeformObservation& Observation);

 const FString& ActiveEvent() const { return Event; }
 const FString& EventId() const { return Event; }
 FString StageName() const { return FString::Printf(TEXT("phase%d"),Stage); }
 bool CancelForTest();
 void TestKeepGroup(){bTestKeepGroup=true;}
 int32 EventStage() const { return Stage; }
 double EventAge() const { return Age; }
 FVector EventAnchor() const { return Anchor; }
 bool PhysicalObjectPresented() const; // actual component visibility: physical props/sky, including scene and adopted ancestors
 FString ObservationNote() const;
 FString Status() const;
 static TArray<FString> EventIds();
 static TArray<FString> CatalogIds() { return EventIds(); }
 static bool SelfTest(TArray<FString>& Lines);
private:
 struct FEffects;
 FS47DuskOrb DuskOrb;
 TUniquePtr<FEffects> Effects;
 US47Main* M=nullptr;
 bool bEnabled=false,bApplied=false,bEffectsApplied=false,bChoiceStored=false,bSeen=false,bPhoto=false,bControlled=false,bCompared=false;
 // Started by Force: a test fixture, which keeps the older life of an event across a change of area. Never in a real game.
 bool bForcedEvent=false,bTestSaveFails=false;
 // A fault lives in the equipment, not in Event. Its finding is written when the equipment says it is mended.
 bool bOwedArray=false,bOwedFeeder=false;
 void StepOwed();
 FString Event,PhotoId,Scene,IncidentId,TaggedShot,LightPlace,NextLightPlace;
 // The event's place from the list, saved; and the one F10 asked for next, not saved.
 FString PlaceId,NextPlace;
 // Lights seen from here (farm_lights) or heard of (town_lights): one draw from the list for either (Tom, 8 November).
 FString PickLightPlace(const FString& Id) const;
 // Marked ground and a fragment: open ground within reach, in the area the player is in. False: none.
 bool PickTrace(FString& OutPlace,FVector& OutAt,bool bForced=false) const;
 bool bTracePlaced=false;
 FString TaggedIncident,TaggedTitle;
 bool bTaggedLamps=false;
 int32 IncidentSerial=0;
 FBox2D TaggedRegion = FBox2D(ForceInit); // transient projection of the photographed native component
 double TaggedQuality=0,PhotoSubjectMean=-1;
 TArray<FPhotoClaim> Claims;
 FString PhotoControlFocus; // Presentation preference only; never evidence or payment.
 bool PhotoControlOriginalAvailable(const FPhotoClaim& Claim) const;
 TArray<FS47FreeformObservation> FieldObservations;
 void ObserveVisitor(const FVector& At,bool bHeard=false);
 int32 SkyLeft=0; // the day's lights in the distance and quiet ground: their own pot, not spent by the day's instruments
 int32 Stage=0,Variant=0,SeenDay=0,MinorLeft=0,MinorBudget=0,InvestLeft=0,ThreatRollDay=0,MajorRollDay=0,LastThreatDay=0,LastMajorDay=-3;
 int32 PendingThreatNight=0,PendingMajorNight=0,LightRollDay=0,PendingLightNight=0;
 int32 DangerNight=0,DangerCount=0;
 // When tonight's lights are due, in hours from the night's own midday (25.5 is 01:30). 0: as soon as they can.
 double LightAfter=0;
 // A figure at the base, and one at the remote feeder or array cabinet: each its own night's draw. The second waits for
 // the player to come to that place, however long the night lasts.
 int32 ObserverRollDay=0,PendingObserverNight=0,SiteRollDay=0,PendingSiteNight=0;
 double SiteCheckAt=0;
 // A night's mystery: one draw a night until its case has opened. Started by the player being at either place once the
 // drawn hour has passed. A short notice, then what was found is kept, and the other place stays worth a look until noon.
 int32 MysteryRollDay=0,PendingMysteryNight=0,MysterySeen=0,MysteryClue=-1;
 double MysteryAfter=0,MysteryClearAt=0,MysteryAge=0;
 float MysteryDice=-1;
 float CarDice=-1;
 bool bCarLight=false,bCarTaken=false;
 int32 CarStage=0; // 0 not yet, 1 the light is on, 2 it has gone
 double CarLightClock=-1;
 bool StepCarLight(double Duration);
 static constexpr double MysteryNoticeSeconds=6;
 void StepMystery(double Dt);
 int32 MysteryClueHere() const;
 bool MysteryQuiet() const;
 void NoticeMystery(int32 Clue,bool bFirst);
 // Tests: override the dice for every night's draw (-1: none), force the lights' draw and its hour (-1: none), and run
 // the scheduler's active-time clock faster to match a test's faster world clock (1: none).
 float TestDice=-1;
 double TestLightAfter=-1,TestScale=1,TestPace=1;
 void RollTonight(int32 Day);
 bool SameScene() const;
 double OrbFleeAge=-1; // running, not saved: a Continue finds the lights where their route puts them
 bool bTestOrbNight=false;
 int32 OrbLightProbe=0;
 int32 OrbStopLogged[FS47DuskOrb::Count]={-1,-1,-1};
 double Age=0,ActiveTime=0,NextAt=90,RestUntil=0,SoundAt=0,LastPhotoAt=-10;
 FVector Anchor=FVector::ZeroVector;
 FRandomStream Random;
 TMap<FString,int32> LastDays;
 S47J::FObj GroupRestore;
 FString ReactiveRoom;
 int32 ReactiveSerial=0;
 double ReactiveAge=0,ReactiveOffClock=0,ReactiveOnClock=0;
 bool bReactiveLeft=false,bReactiveApplied=false,bReactiveInspected=false;
 bool ReactiveValid() const;
 bool ReactiveReady() const;
 void StepReactive(double Dt);
 bool Start(const FString& Id,bool bForced);
 // At: the clock to ask about, for a rest that would wake then. Negative: now.
 bool Eligible(const FString& Id,bool bForced,double At=-1) const;
 bool RollNightChoice(int32 Day,int32 FirstDay,float Chance,int32& RolledDay,int32& PendingNight);
 bool ExpireNightChoices(int32 Day,bool bNight);
 bool StartPendingNightChoice(const FString& Id,int32 Day,int32& PendingNight);
 void Apply();
 void Draw();
 void Finish(bool bEvidence,const FString& Text=FString());
 void Changed();
 bool Near(const FVector& At,double Range) const;
 bool InView(const FVector& At,double Range=120) const;
 bool Night() const;
 void StepGroup(double Dt);
 bool bTestKeepGroup=false; // A test that stops the planner still needs the bodies to walk.
};
