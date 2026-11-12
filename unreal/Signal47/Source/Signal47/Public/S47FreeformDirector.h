#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47DuskOrb.h"
#include "S47FreeformObservation.h"
#include "S47NightTiers.h"
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
 // LOOP-1-20261109 (HELHETLIG_LOKKE step 1): what at SARO points the player out at the land (the bearing slip a registration gives,
 // FS47Places::AfterRegistered) comes at most PointOutLimit times a night: director.json's globals.point_out_per_night, 1 without
 // it. Nights as FS47FreeformSession::NightOf; saved with the night, so a Continue does not give a second one.
 int32 PointOutLimit() const;
 int32 PointOutsOn(int32 Night) const{return PointNight==Night?PointCount:0;}
 bool MayPointOut(int32 Night) const{return PointOutsOn(Night)<PointOutLimit();}
 void NotePointOut(int32 Night);
 // PT-FEEDER-20261109 (d37): whether lights may come tonight, past their own dice and hours. Neither kind on the big night's night
 // (FS47Happenings::BigOwns): it owns the night. And lights heard of (town_lights) send the player out, so they are the night's one
 // pointing-out: not after a bearing slip, and none after them (Start notes it).
 static bool LightsMayCome(const FString& Id,bool bBigOwnsNight,bool bMayPointOut){return !bBigOwnsNight&&(Id!=TEXT("town_lights")||bMayPointOut);}
 // FEEDER-LEAD-20261110: a feeder fault on a night with a lead drawn would leave the lead unreachable (no power, no pointing, until the road
 // cabinet is mended): none starts then. Mended, or already out, it is as before.
 static bool FeederMayFail(bool bLeadTonight){return !bLeadTonight;}
 // Whether what an event left out on the land may be pointed at now: the event's tier in director.json against the hour
 // (S47NightTiers: before midnight only a hint). An event the file does not have counts as full.
 bool MayPointAt(const FString& EventId,double Clock,int32 Night) const;
 static bool TierAllowsAt(ES47Allowed Need,double Clock,int32 Night);
 int32 PointNight=0,PointCount=0;
 // Critic round 5: the night the rhythm's danger slot was taken (FS47Rhythm::Began, DangerSlotOpen) and the night of the last peek in
 // at a window (FS47Rhythm::BeganOn), saved with the night's dangers: a load of the same night, or a Continue, has them as they were.
 int32 SlotNight=0,PeekNight=0;
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
 // Tests: the director starts nothing of its own, without that counting as the rhythm's rest (which would also stop draws like the field visit).
 void TestHush(bool bOn){bTestHush=bOn;}
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
 // D45-20261110 (D45-26: Tom rested at 21:02 and never saw the 21:15 visit). The first evening's appointment: from 19:00 on the first
 // night while the night's first visit of the low lights is still to come, a task row (and a toast once, as it goes on the note) that
 // they are seen over the dishes most evenings around nine. Empty otherwise. EveningLightsAt: the same, pure.
 FString EveningLightsLine() const;
 static FString EveningLightsAt(double Clock,int32 NightVisits,double NightDue);
 // PT-FEEDER-20261109 (d37: the feeder looked at, the line still open): the report on tonight's note is ticked off once the player
 // has been to the place and found something there, or found it gone over and empty (FS47FarmSite::Checked). Not while the lights
 // are still up, and not for a landing's report, which is its own.
 bool ReportDone() const;
 static bool ReportDoneWith(bool bLightsUp,bool bArmed,bool bChecked){return !bLightsUp&&bArmed&&bChecked;}
 // What the night's mystery still has to show, as a task row, while it can be found.
 FString MysteryTrailHint() const;
 // PT-NOTEBOOK-20261109 (Tom, d37: the feeder looked at, its line gone without a tick): the trail's rows while it can be found
 // (until MysteryClearAt), each clue's row ticked off once it is seen instead of dropped: the remote feeder, the receiver chart.
 TArray<TPair<FString,bool>> MysteryTrailRows() const;
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
 // STORY-ON-FREEFORM-01 step 6: a Story night's beat starts one of the night's events as Force does (past its day and tier rules,
 // not past a usable world): only with a Story night's director. False with why.
 bool StartForStory(const FString& Id,FString& Why);
 // FIX-20261110 (Tom, point 2): the next forced flyover or close pass takes the mains as one in play does (a fixture otherwise
 // keeps its older, immediate life). Sag: seconds of dips before it; After: seconds the mains stay out once it has gone.
 void SkyCutNext(double Sag=45,double After=75){bSkyCutNext=true;SkyCutSag=FMath::Max(1.,Sag);SkyCutAfter=FMath::Max(1.,After);}
 bool SkyCutHeld() const{return SkyPrelude>0;}
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
 // D48-20261110 (Tom: every photo of aliens, events and strange things pays, if it is good enough). A strange subject outside the
 // Director's own incidents (a visitor's body, a figure on the road, the lights in the high sky, a landing's ring, a dead cow, ...)
 // makes a claim named "strange.<group>.n<night>": one a group a night, the best frame stands, paid once at OPS through the same
 // path. StrangeGroupOf: a happenings prop kind's group, empty for the ordinary (dust devil, animals, people, cars, humour: no pay).
 // StrangeBase: the group's pay before quality (rarer pays more, never above the week's side cap); PhotoReward: the claim's pay,
 // 20 for the Director's own incidents as before, base times 0.6 (just legible) to 1.25 (large in frame) for the rest.
 static FString StrangeGroupOf(const FString& PropKind);
 static int32 StrangeBase(const FString& Group);
 static int32 PhotoReward(const FString& ClaimId,double Quality);
 const TArray<FPhotoClaim>& PhotoClaims() const { return Claims; }
 bool RegisterPhotoClaim(const FString& Id);
 // LOOP-3-20261109 (HELHETLIG_LOKKE step 3): OPS's one way to register what came back from the field. A claim (one of the Director's
 // own incidents) as RegisterPhotoClaim; otherwise Id is an album photograph from one of the night's happenings out there, or a trip
 // camera frame, which OPS pays for once as field money under the week's cap (FS47FreeformSession::RegisterNightPhoto). Dr. Ward's
 // count of the album does not wait on it.
 bool RegisterFieldRecord(const FString& Id);
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
 // close_pass: where the craft is on its line over the station now (S47Omen reads it; the gear and the animals feel it there).
 FVector ClosePassNow() const;
 // The event's anchor is in the area the player is in now (its scene, the town's two as one).
 bool AnchorHere() const { return SameScene(); }
 // MILITARY-20261108: a landing_trace whose ground the military went over first (S47Visitors::Cleared, the night its place was
 // used): no marks, no fragment and nothing for the counter. The event ends at the next step, the raked circle where it lay.
 bool TraceRaked() const;
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
 bool bForStory=false; // STORY steg 6 (night act): StartForStory's own start goes past the story's hold on the dice (Eligible)
 // A fault lives in the equipment, not in Event. Its finding is written when the equipment says it is mended.
 bool bOwedArray=false,bOwedFeeder=false;
 void StepOwed();
 FString Event,PhotoId,Scene,IncidentId,TaggedShot,LightPlace,NextLightPlace;
 // The event's place from the list, saved; and the one F10 asked for next, not saved.
 FString PlaceId,NextPlace;
 // Lights seen from here (farm_lights) or heard of (town_lights): one draw from the list for either (Tom, 8 November).
 // bFixture: started by Force alone, as the older tests are: on the ground the player is on, where they can be seen.
 FString PickLightPlace(const FString& Id,bool bFixture=false) const;
 // Marked ground and a fragment: open ground within reach, in the area the player is in. False: none.
 bool PickTrace(FString& OutPlace,FVector& OutAt,bool bForced=false) const;
 bool bTracePlaced=false;
 // The low lights' visit: which antennas it is over (ARRAY-ORBS). The shift from the first spot is in Anchor, saved.
 FString OrbPair;
 void OrbSite(int32 Seed,FVector2D& OutShift,FString& OutPair) const;
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
 bool bTestHush=false;
 // The first shift was on at the last tick (Step): its end gives the scheduler an ordinary interval before its first draw.
 bool bWasFirstShift=false;
 bool bEveningTold=false; // D45-20261110: EveningLightsLine's toast said in this sitting (a Continue in the evening says it again)
 void RollTonight(int32 Day);
 bool SameScene() const;
 double OrbFleeAge=-1; // running, not saved: a Continue finds the lights where their route puts them
 bool bTestOrbNight=false;
 int32 OrbLightProbe=0;
 int32 OrbStopLogged[FS47DuskOrb::Count]={-1,-1,-1};
 double Age=0,ActiveTime=0,NextAt=90,RestUntil=0,SoundAt=0,LastPhotoAt=-10;
 double SkyPrelude=0,SkyCutSag=45,SkyCutAfter=75;bool bSkyCut=false,bSkyCutNext=false; // not saved: see FS47FreeformSession::SkyCutBegin
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
