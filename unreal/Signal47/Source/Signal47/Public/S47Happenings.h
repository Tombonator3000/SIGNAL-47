#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47RoswellWorld.h"
#include "S47RoadGlimpse.h"
class US47Main;
class AS47Web;
class FS47FreeformDirector;
class FS47AnimalWarn;
class FS47Calm;
class UStaticMeshComponent;
class UPointLightComponent;
class UPrimitiveComponent;
struct FS47MapView;
struct FS47RhythmNow;
struct FS47EventPlace;

// Freeform only (Tom, 8 November: NIGHT31 proposals 1 to 8, and GEIGER). What can happen out in the night, each a real event in the
// night's rhythm (Content/Director/director.json, system "night"): asked of FS47Rhythm before it starts, counted when it does, with
// the rest after a danger. Each leaves something: a record, money, a bearing on the map, or a lead to a place.
//
//   car_dies      on the road at night something goes over; the lights, the dash and the radio die for a few seconds (Levelland).
//   clock_ahead   after such a passing the dash clock runs forty minutes ahead of the station's until morning.
//   cattle_ring   cattle out on the land stand in a ring, every head the same way (toward the last landing, when there is one).
//   cb_stranded   a driver stuck far out calls on the CB: real (pull him out: pay and thanks), an empty rig, or rarely a voice
//                 that is not right. A wrong voice can come back a later night and say something you did (cb_echo, a danger).
//   landing       lights come down out on the land, seen from wherever you stand outside or logged by the array at SARO; a bearing
//                 on the map; out there a scorched ring, four marks, dead beetles and plants.
//   hitchhiker    a woman by the highway asks for a ride with her hand, never answers, and is gone in the mirror (a danger).
//   stranded_rv   a camper far out with every light on and nobody in it; a book, and a chemical smell (a homage, nothing named).
//   humor         at most once a week, never on a night with a danger, never in Story: a pink convertible running at the diner, a
//                 singer by the motel pool, something tall and hairy crossing the road, a small man at the pump who wants water.
//                 Each skips itself when its model is missing.
//   figure_near   on foot out in the dark (a danger, from the second week): a figure on the open ground that is gone when it is seen
//                 and stands nearer the next time, until a light comes down over whoever is still out there (S47Taken).
//   road_glimpse  at the wheel between midnight and two (the glimpse tier), at most once a week: something tall and thin, or low on
//                 four, crosses the highway 60 to 120 m ahead in the lights and is gone before the truck comes up; some nights prints
//                 on the far shoulder the next day (S47RoadGlimpse). The figure on the rise and the one on foot may be Tom's Nordic or
//                 Cosmic (the seed's, when this build has them); Zet stays in the humorous ones.
//
// The build-up (Tom's critics, 8 November): director.json's curve says, week by week, what each level is worth, how likely the
// night's one danger is and how near things come; each night builds toward their hours (draw.at), with the small things first.
// A night's danger is one slot, at an hour in their hours, taken by whichever danger fits where the player is then.
//
// "Seen" is seen (SeenLit): in view and in range, nothing solid between, and light on it. What is far off or in the dark is
// heard, not told: the words on the screen are what the player does, not what they should make of it.
//
// Places come from the place catalogue (FS47EventPlaces, M1-A) through Where(), its one door here; without it, the land's own
// places (FS47Places). Everything that stands out on the land is kept road-local and drawn in whichever area the player is in.
//
// The counter (GEIGER-20261108): built at the SARO bench from tested parts and the tube from the stores (FS47Workshop "geiger",
// FS47Openings "geiger_tube"). It ticks on foot, faster near a landing's ring, marks left by visits and the places a bearing has
// pointed at, and pegs in the middle of a ring. What it reads is shown on its own dial, held up in the lower corner of the view.
class FS47Happenings
{
public:
 explicit FS47Happenings(US47Main* Main);
 ~FS47Happenings();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;

 // The rhythm's ids, as director.json has them.
 static const TArray<FString>& Ids();
 static bool IsDanger(const FString& Id);
 // On the ground now, or (the clock) running ahead.
 bool Live(const FString& Id) const;
 // The one holding the ground now, empty when none.
 const FString& Ground() const;
 // F10-NORDIC-COSMIC-20261109: the variant the one on the ground was given; for the figures and the crossing, its body ("nordic",
 // "cosmic", "tall", "shadow", "crawler"; empty: the shade). Empty when none is on.
 const FString& LiveVariant() const;
 void FillRhythm(FS47RhythmNow& Out) const;
 // A humorous one happened tonight: nothing dangerous may follow it (Tom: never the same night as a danger).
 bool HumorTonight() const;
 void FillMap(FS47MapView& View) const;
 // What the dash clock shows: the game's own, or forty minutes ahead after a passing, until the morning.
 static FString DashClock(const US47Main* M,const FString& Usual);
 double DashAhead() const;
 // F10 and tests: start one here and now, past its draw. Variant: real, empty or wrong for cb_stranded; car, singer, crossing or
 // pump for humor; nordic, cosmic or shade for horizon_figure and figure_near; tall, low or a body for road_glimpse. Why: what
 // stopped it.
 bool DevStart(const FString& Id,const FString& Variant,FString& Why);
 // F10 and tests: as the night itself would start it, with every condition asked (only the rhythm's dice and draws left out).
 bool DevTry(const FString& Id,FString& Why){return StartEvent(Id,FString(),false,Why);}
 // PT-FEEDER-20261109: asked for by a place the player has come out to (FS47FarmSite: a figure far off past the marks), with every
 // condition asked as the night itself would ask it: the rhythm, the tier, the player's situation. Why: what stopped it.
 bool Request(const FString& Id,FString& Why){return StartEvent(Id,FString(),false,Why);}
 int32 DangersMet() const{return MetTotal;}
 // Whatever is on the ground is over at once; with bSites the lasting ones go too. Test runs only.
 void DevClear(bool bSites);
 // Where a lasting one stands in the area the player is in now (Kind: landing, truck, rv). False: none, or not in this area.
 bool SiteHere(const FString& Kind,FVector& Out) const;
 // Where the one on the ground stands now, in this area (the woman, the car, the singer, the small man, the lights). False: none.
 bool LiveAt(FVector& Out) const;
 bool HasSite(const FString& Kind) const;
 // A danger the player has met, while it lasts (FS47Stress, the Director's RestBlock): why there is no lying down now. Empty: none.
 FString RestBlock() const;
 // The one on the ground is a danger whose light may take the player: being taken is that same meeting (S47Taken, FS47Rhythm).
 bool HoldsTaking() const;
 // What the player has to see something by: its own light, against the sky, or the lamp, the truck's lights or the day on it.
 // Pure, for the self-test: the hour, and how far off and how far from the middle of each light the thing is.
 struct FLightOn{double Hour=0;bool bOwn=false,bSky=false,bLamp=false,bHeadlights=false,bTown=false;double LampM=1e9,LampDeg=180,CarM=1e9,CarDeg=180,TownM=1e9;};
 static bool LitBy(const FLightOn& L);
 // The night's hour, 8 to 32: from the morning the night is drawn in to the next (FS47FreeformSession::NightOf turns at eight).
 static double NightHour(double Clock);
 // A record one of these leaves (FS47FreeformCases, FieldRecordKept, the 30-day assessment). Humour and pay are not.
 static bool IsNightFinding(const FString& Id);
 // Its kind, for counting kinds: "cattle_ring_d4" -> "cattle_ring".
 static FString NightFindingKind(const FString& Id);
 // The counter.
 bool GeigerBuilt() const;
 // Its own key: held up, or put away. False when it is not built.
 bool ToggleCounter();
 float GeigerReading() const{return Dose;}
 bool GeigerShown() const{return bGeigerUp;}
 void DevGeiger(bool bGive,bool bHold){bDevGeiger|=bGive;bGeigerHeld=bHold;}
 // D45-20261110 (F10): the ring's coals as they are now (shown, heat, smoke and how it is drawn, crackle), and the ring made Hours older.
 bool DevEmbers(FString& Out) const;
 bool DevAgeRing(double Hours);
 // The bench's job for the counter (FS47FreeformWorld): empty when the tube is there.
 static FString GeigerBench(const US47Main* M);

 // Unease (Docs/Design/Director/URO.md, TODO UNEASE-20261108, Tom 8 November): 25 small things with no danger, no reward and no
 // explanation, two or three a night, never two alike in a night, never just before or after a danger. The player is never quite
 // sure anything happened. Nr. 3 (your name) and nr. 20 (a note in your own hand) come at most once in a game.
 static int32 UneaseCount();
 static const TCHAR* UneaseId(int32 I);
 static const TCHAR* UneaseTitle(int32 I);
 // F10: this one now, as far as where the player stands allows. Why: what stopped it.
 bool DevUnease(int32 I,FString& Why);
 bool UneaseLive() const{return Uneasy.I>=0;}
 int32 UneaseDoneTonight() const{return UneaseDone.Num();}
 static bool SelfTest(TArray<FString>& Lines);

 // LOOP-4-20261109 (HELHETLIG_LOKKE step 4). The night's one pointing-out at the land (FS47FreeformDirector::MayPointOut) is taken by
 // the array's own burst and by a landing the array logs with nobody watching, as by the bearing slip: true when this one may point
 // out (and it is counted); without a Director, always.
 static bool TakePointOut(FS47FreeformDirector* Director,int32 Night);
 // D45-20261110 (D45-26: night 1 had nothing an operator indoors could meet). The first night's one sure thing before midnight is the
 // array's burst at SARO, drawn between 22:00 and 23:15 (FirstBurstHour, from the seed), toward the noon slip's place with its bearing
 // (FS47Places::ArcPlace). It waits for the player to be in the building until 01:30 (FirstBurstUntil) and is never swapped away; until
 // it has come the night's one pointing-out is kept for it (FirstBurstOwed: a slip printed before it does not take it). The
 // night's one unease is pinned before midnight too, half an hour or more from the burst (FirstUneaseHour). Pure but the last.
 static constexpr double FirstBurstFrom=22.,FirstBurstSpan=1.25,FirstBurstUntil=25.5;
 static double FirstBurstHour(int32 Seed);
 static double FirstUneaseHour(int32 Seed);
 bool FirstBurstOwed(int32 Night) const;
 // The magnetometer's history (FS47Places::Use): something has happened near a place in the last two nights, whether or not anyone
 // saw it (what stands out there now, and what stood there and has gone). Road-local metres; Day as FS47FreeformSession::Day. Not a
 // bearing's sector: a needle that swung wherever a bearing points would be a metal detector.
 struct FStir{double X=0,Z=0;int32 Night=0;};
 static constexpr double StirReach=350;
 static constexpr int32 StirNights=2,MostPast=24;
 static bool Stirred(const TArray<FStir>& Near,double X,double Z,int32 Day);
 bool StirredNear(double X,double Z) const;

 // CHAIN-1-20261109: the night's one thing out on the land that the station's own work can lead to. Of what was drawn for tonight
 // (PlanTonight, from the seed), the first of these that leaves something standing, and its place from the place list, drawn at
 // dusk from SARO instead of when it starts. What comes, and when, is as before; only where is settled early, so that a card's
 // discrepancy and the bearing slip (FS47Places::AfterRegistered) are about the same thing. Saved ("lead"; an older save has
 // none and draws it again). A calm night, or one with none of these drawn, has none: nothing is made up.
 struct FLead{int32 Night=0;FString Kind,Place;bool bPointed=false;bool IsSet() const{return Night>0&&!Kind.IsEmpty()&&!Place.IsEmpty();}};
 const FLead& Lead() const{return LeadNow;}
 // Which of a night's draws it is: a landing before a carcass before a missing man's pickup, a camper, a stuck driver. Empty: none.
 static FString LeadKindOf(const TMap<FString,double>& Drawn);
 // Whether it may start at its drawn place with the player this far off (road-local metres, under 0: no player). Never on top of
 // the player; once a slip has pointed there (bPointed) it is held to, however far off that is.
 static bool LeadFits(double Away,double MinM,double MaxM,bool bPointed);
 // What is out there tonight for a bearing to point at: what has come already and nobody has been out to (where it stands), or the
 // one still to come (bAhead; where it will be). Event: its id in director.json, for the tier. False: nothing real tonight.
 bool OutTonight(int32 Night,FString& Event,FString& Title,double& X,double& Z,bool& bAhead) const;
 // A slip pointed at the one still to come: it is held to its place, and waits out the night for its chance.
 void PointedTonight(int32 Night);
 // CHAIN-5B-20261109 (SAMMENFLETTING, dead time at SARO): one word a night about what is out there tonight (OutTonight), from Earl on
 // the request line, the CB in the truck or the printer at the rack, whichever the player comes to first ("earl", "cb", "printer").
 // Never a place's name and never on the map: a compass word from SARO, for what has come already, or for the one still to come only
 // once a slip has pointed there (it agrees with the slip). It takes nothing from the night's one pointing-out. Empty: nothing tonight,
 // said already tonight, or not the night's hours (20:00 to 05:30).
 FString WordOfTonight(const FString& Who);
 // The words by themselves: Kind the event's id (OutTonight's Event), Way a compass word or empty, bAhead it has not come yet.
 static FString WordLine(const FString& Who,const FString& Kind,const FString& Way,bool bAhead);

 // M7, the first big night (Tom, 9 November: «Det må skje noe stort natt to eller tre», S47BigNight.cpp). Once in a game, on the
 // second or third night (the seed's), after two: the whole array turns by itself to one point low on the horizon while the power
 // flickers, every instrument in the rack goes off at once, a low note comes up through the ground, and for a few seconds something
 // stands, falls or rises out on the land in that direction (the seed's variant: "column", "fall" or "rise"). Felt wherever the
 // player is: at SARO the lights and the rack, in the truck the dash and the lamps, in Caliche the street lamps, on foot the lamp.
 // Nothing dangerous and nobody taken. It leaves a mark at the place, a line in the night log (the array's own record), the array
 // still pointing there in the morning, Dale on the walkie, 1420 and the CB talking, the Ledger's front page, and a lead to the
 // visitors' case. Slept through, it still happened: the morning finds what it left.
 struct FBigPlan{int32 Night=0;double Hour=0;int32 Variant=0;};
 // Pure, from the seed: the night (2 or 3), the hour of the night (26.1 to 27.6, never the 02:20s to 02:40s), the variant.
 static FBigPlan BigPlanFor(int32 Seed);
 // CHAIN-4-20261109 (SAMMENFLETTING step 4): the same night again in week two (Round 1, nights 9 to 12) and week three (Round 2, 16 to
 // 19), each in a shape the player has not had (Had: the shapes that came) and somewhere it has not stood. Round 0 is BigPlanFor.
 static constexpr int32 BigRounds=3;
 static FBigPlan BigPlanAt(int32 Seed,int32 Round,const TArray<int32>& Had);
 static const TCHAR* BigVariantName(int32 Variant);
 static int32 BigVariantOf(const FString& Name);
 // This game's: from the seed, moved to the other of the two nights when the drawn one is calm and the other is not. CHAIN-4: the
 // round still to come once one has come (BigAhead), the last one's own after the third.
 FBigPlan BigPlan() const;
 // The night the first one came (or was slept through), 0 not yet, -1 never in this game (a save from after its nights).
 int32 BigHappened() const{return BigPast.IsEmpty()?BigNight:BigPast[0].Night;}
 // CHAIN-4: the night the latest round came (its day after: 1420, the night log), 0 while the next waits; the round still to come, -1 none.
 int32 BigLatest() const{return BigNight;}
 int32 BigAhead() const{return BigNight==0?BigRound:BigRound+1<BigRounds?BigRound+1:-1;}
 bool BigLive() const{return Now.Id==TEXT("big_night");}
 // N1110 (pace, critic 2): clock seconds of the last false alarm (S47Calm), -1e9 if none: the animals' lone warning keeps clear of it.
 double LastFalseAlarmAt() const;
 // A sleeper on this night is woken for it (FS47FreeformDirector::DrawForNight).
 bool BigWakes(int32 Night) const;
 // PT-FEEDER-20261109 (d37): the big night's night is its own from dusk to morning, before it comes and after: no lights reported or
 // seen elsewhere (FS47FreeformDirector::LightsMayCome), no bearing slip (FS47Places), nothing more to marks already standing.
 bool BigOwns(int32 Night) const;
 // The rule by itself: the planned night while it has not come (and the game is not a dev run), or the night it came.
 static bool BigOwnsNight(int32 Night,int32 PlannedNight,int32 CameNight,bool bWaits){return Night>0&&((CameNight==0&&bWaits&&Night==PlannedNight)||CameNight==Night);}
 // What of the day after has come: "array", "dale", "slip", "radio", "cb", "seen", "felt", "trace", "unseen".
 bool BigHas(const FString& What) const{return BigFollow.Contains(What);}
 // F10 and tests: the day after's follow-ups now, as far as where the player is allows (Dale's hail, the array's record).
 void DevBigMorning();
 // F10: the planner's own draw from now on, in a developer run too, as if this game had not reached it yet.
 void DevBigNatural();
 // F10: tonight's big night as if slept through, in this shape (empty: the seed's): the mark is out there, the day after to come.
 bool DevBigSlept(const FString& Variant,FString& Why);
 // F10: as E on the mark.
 void DevUseBig(){UseBig();}
 // BIGNIGHT-SEEN-20261109: how long its height has waited for the player's eyes (seconds of the night), and whether it has come.
 double BigHeldFor() const;
 bool BigCameUp() const;
 double BigBearing() const{return BigAz;}
 double BigElevation() const{return BigEl;}
 const FString& BigPlaceTitle() const{return BigTitle;}
 static bool BigSelfTest(TArray<FString>& Lines);

 // D46-20261110 (D46-3, Tom 10 November: "must have more action/events the first 24 hours"; S47FirstDay.cpp). The first day's
 // script: small authored beats across day 1 and night 1 from what the game already has (the CB in the truck, Dale's set in OPS, a
 // dog far off, the rack, the phone), each one that can be explained away before midnight and only machinery and sound after it,
 // and the cliffhanger at 00:41: every dish on remote turns by itself to the night's line (the burst's bearing) and stays there; slept
 // through or away, it is found turned at SARO before the evening after. Its once-flag is the field record FirstTurnKey (no new save
 // key); Dale's first line on day 2 answers it (FS47Predecessor, bark "turned"). The beats themselves are not saved: a Continue drops
 // those whose window has opened. Hours are hours from midnight before day 1 (night 1 is 8 to 32).
 struct FFirstBeat{const TCHAR* Id;double From,Until;bool bHint;};
 static const TArray<FFirstBeat>& FirstBeats();
 static constexpr double FirstTurnHour=24.+41./60.,FirstTurnLive=25.5,FirstFoundUntil=24.+18.;
 static constexpr const TCHAR* FirstTurnKey=TEXT("array_turn_d1");
 // What the cliffhanger does now: wait, turn in front of the player, be found turned, or never (past the evening after).
 enum class EFirstTurn:uint8{Wait,Live,Found,Never};
 static EFirstTurn FirstTurnFate(double Hour,bool bAtSaro,bool bPowered,bool bMissed,bool bBusy);
 // Real minutes from 08:00 on day 1 to Hour on an unheld clock (daylight 13 h in 30 minutes, the dark 11 h in 45).
 static double FirstRealMinutes(double Hour);
 static bool FirstDaySelfTest(TArray<FString>& Lines);

 // A place for one of these, with what it needs: the catalogue's, or the land's own.
 struct FSpot{FString Place,Title;double X=0,Z=0;bool bSet=false;};
 struct FProp;
 // D48-20261110: what the camera may find among the drawn props of this area: its kind, the box round its drawn parts (UE) and those parts.
 struct FPhotoSubject{FString Kind;FBox Box=FBox(ForceInit);TArray<const UPrimitiveComponent*> Parts;};
 void PhotoSubjects(TArray<FPhotoSubject>& Out) const;
private:
 US47Main* M=nullptr;
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<AS47Web> Scene;
 // What holds the ground now.
 struct FActive
 {
  FString Id,Variant,Place,Title;
  int32 Stage=0,Serial=0;
  double Age=0,Len=0,X=0,Z=0,Yaw=0,Clock=0;
  FVector At=FVector::ZeroVector,Dir=FVector::ZeroVector;
  bool bSeen=false,bForced=false,bDone=false,bDanger=false;
  // A danger counts when the player meets it (MeetDanger), not when it is drawn. Watch: until this age, the player out on foot is
  // taken (BeginOver), as under the light over the truck.
  bool bMet=false;double Watch=-1;
  // FIX10-9 (→ M3): seen from inside the SARO building (the figure on the rise through a control room window). Saved ("inside").
  bool bInside=false;
 } Now;
 // What stays out on the land after: a landing's ring, a stuck truck, a camper.
 struct FSite
 {
  FString Kind,Variant,Place,Title;
  int32 Night=0,Serial=0;
  double X=0,Z=0,Yaw=0,Az=-1,Spread=0,Until=0;
  // bKnown: a landing's bearing may go on the map (seen, logged or found); its Az is set when it comes down, known or not.
  bool bFound=false,bDone=false,bHeard=false,bKnown=false;
 };
 // For what the interaction list holds on to: lives as long as this does, across every Begin.
 TSharedPtr<uint8> Alive=MakeShared<uint8>(0);
 TArray<FSite> Sites;
 // LOOP-4-20261109: where something stood out there that has since gone, and its night (saved, "past"; an older save has none).
 TArray<FStir> Past;
 TMap<FString,int32> LastDays;
 TArray<FString> Deeds; // what the player did, in a voice's words, for one that repeats it
 int32 Serial=0,HumorNight=-100,EchoFrom=0,LateNight=0,PlanNight=0,AheadNight=0;
 double AheadSeconds=0;
 bool bLate=false,bEchoArmed=false;
 // Tonight's draws: id -> the hour (of the night, 19 to 30) it may start from. Absent: not drawn. Never saved: from the seed.
 TMap<FString,double> Plan;
 // When each may be tried again after a refusal (main's own seconds), so that a refusal is not asked again every frame.
 TMap<FString,double> TryAt;
 // Between 23 and 01: something else came (bLate), and when the fallback may next try (game clock).
 double LateTryClock=0;
 FString HumorKind;
 FString LastArea;
 struct FSay{FString Text;double In=0;float Secs=6;};
 TArray<FSay> Says;
 TArray<TUniquePtr<FProp>> Props;
 // The passing over the truck, and the truck's electrics while it is over them.
 double DimLeft=0;bool bDimmed=false;
 // The counter.
 // D45-20261110: no tick debt any more (the counts are a Poisson process, S47Hap::PoissonCount): GmMeter is the dial's own count rate,
 // counts a second through its time constant, and bGmRoar whether the dense loop of counts is playing where it pegs.
 float Dose=0,Needle=0;double GmMeter=0,GeigerUpFor=0,GeigerNoteIn=0;bool bGeigerUp=false,bGeigerHeld=false,bDevGeiger=false,bGeigerAway=false,bGmRoar=false;
 TSet<FString> GeigerNoted;
 // ---- the night ----
 void PlanTonight(int32 Night);
 // CHAIN-1-20261109: tonight's lead (above), and its draw at the end of PlanTonight.
 FLead LeadNow;
 void DrawLead(int32 Night);
 // CHAIN-5B-20261109: the night the word of tonight was said (WordOfTonight; saved, "wordNight"), and when the CB may next try it.
 int32 WordNight=0;double WordTry=0;
 // The night's own pace: one at x1 in play; F10's natural night runs its minutes at its speed, so what lasts is measured in the
 // night's minutes and not in seconds of the screen (critics, 8 November: at x30 a figure held the ground for twenty hours).
 double NightPace() const;
 double Paced=0; // main's seconds, at the night's pace
 // This week's row of the curve (director.json): how near things come, and the night's danger slot (an hour of the night; -1 none).
 double NearK=1,DangerAt=-1;
 static constexpr double DangerUntil=29.;
 // Measured for the log (Tom's critics: the run must say what the player met, how far off, and whether they noticed it).
 struct FTally{int32 Night=0,NStarted=0,NNoticed=0,NNear=0,NMet=0,NRefused=0;double Nearest=1e9;};
 FTally Tally;
 TSet<int32> NoticedSerials;
 TMap<FString,FString> RefusedSaid;
 mutable TSet<FString> LateDrawSaid;
 void CloseTally(int32 NewNight);
 void Refused(const FString& Id,const FString& Why);
 // Where the player stands, in this area's terms: the cab when driving.
 FVector PlayerAt() const;
 double AwayFrom(const FVector& At) const;
 // From the player to the one on the ground now, or to what it left; -1 when it has no place (a word on the radio).
 double AwayNow(const FString& Id) const;
 // Sounds left at a lasting thing (the cattle, the camper's generator, the projector): started near, stopped away.
 TSet<FString> LoopsOn;
 double SoundNext=0;
 void SiteSounds();
 // Before an observation too, now and then, a far cue: not only before a danger.
 mutable FS47RoswellWorld Sight;
 double HourOfNight(double Clock) const;
 bool Due(const FString& Id) const;
 bool MayStart(const FString& Id);
 bool StartEvent(const FString& Id,const FString& Variant,bool bForced,FString& Why);
 void EndEvent(bool bEvidence);
 void StepEvent(double Dt);
 void StepSites(double Dt);
 void StepLate();
 // Drawing to where the player is (coordinator, 8 November): a draw that cannot happen there gives its place to one that can.
 uint32 SituationNow() const;
 static uint32 NeedMask(const FString& Id);
 TArray<FString> SwapOrder(int32 Night) const;
 void StepSwap();
 double SwapTry=0;
 void StepSays(double Dt);
 void Say(const FString& Text,double In=0,float Secs=6);
 void Cb(const TArray<FString>& Lines,const FString& Voice,bool bWrong);
 // A stranger's word that need not be heard on a CB: on one if one is to hand, otherwise Dale passes Passed on over the walkie.
 void Tip(const TArray<FString>& Lines,const FString& Voice,const FString& Passed);
 // Dangers met in this game (saved): humour waits for the first. The humorous ones seen, so that the small man comes last.
 int32 MetTotal=0;TArray<FString> HumorSeen;
 // The night a figure was asked for on the next photograph: it is not carried into another night.
 int32 GhostNight=0;
 // What a voice may say back to the player (cb_echo): only what they really did, never humour or F10.
 void Deed(const FString& Text,bool bEcho=true);
 // The danger on the ground is met: counted now (FS47Rhythm::Began), and shown in stress and the rest it blocks.
 void MeetDanger();
 // The watch over a danger: the player out on foot is taken.
 void StepWatch();
 // What is drawn for each, put away when it is over or the game stops (and the room's television with it).
 void PutAway(const FString& Id);
 // Something was seen or heard (StepLate between 23 and 01: only what the player noticed near them counts; the log and the tally).
 // Dist: how far off it was, -1 where the player is. How: by sight, sound, the radio, the hand.
 void Noticed(const FString& Id=FString(),double Dist=-1,const TCHAR* How=TEXT("here"),int32 Ser=0);
 // What the spoken words call a place: never a sign's capitals.
 static FString Spoken(const FString& Title);
 // Before a danger that is drawn, one or two small things, closer each time; on other nights now and then the same with nothing after.
 struct FCue{double Hour=0;int32 Kind=0;bool bDone=false;};
 TArray<FCue> Cues;
 void PlanCues(int32 Night);
 void StepCues();
 // A small thing that dips the room's hum or the lamp is undone at this (main's seconds); -1: nothing to undo.
 double ToneBack=-1;
 // What nobody saw comes back as talk the next evening (StepSites): when that may next be tried (main's seconds).
 double RumorTry=0;
 // A draw has its window: an hour and a half from its time, then it is gone (or given to one that fits, StepSwap).
 static constexpr double DrawWindow=1.5;
 // LEAD-FIX-20261110: hours of the night each draw was held back by a pointed landing sitting on the ground (S47Hap::SitHold), added
 // to its window (Due) so it comes right after instead of losing its night; and the clock when that was last counted (-1: not sitting).
 TMap<FString,double> SitHeld;double SitHeldClock=-1;
 // Between 23 and 01: tonight's hour for the reserve, and the nights that keep a hush instead.
 double LateAt=24.5;bool bLateHush=false;
 // The truck walked back to after a while away (hood_warm, truck_tracks): main's seconds then, and how long away (game seconds).
 double ReturnedAt=-1e9,ReturnedAway=0;
 // The missing man's pickup: noted, and called in at the next CB or phone.
 void StepReport();
 // Which unease came last on which night: the ones of the last three nights wait at the back.
 TMap<int32,int32> UneaseLast;
 // Synthesised sounds the recordings lack: a whisper, a far laugh, many wings, the truck's alarm (made once, kept in the audio).
 FString Synth(const TCHAR* Name);
 // ---- each one ----
 bool BeginCar(bool bForced,FString& Why);
 bool BeginCattle(bool bForced,FString& Why);
 bool BeginStranded(const FString& Variant,bool bForced,FString& Why);
 bool BeginEcho(bool bForced,FString& Why);
 bool BeginLanding(bool bForced,FString& Why);
 bool BeginAhead(bool bForced,FString& Why);
 bool BeginHitcher(bool bForced,FString& Why);
 bool BeginRv(bool bForced,FString& Why);
 bool BeginHumor(const FString& Variant,bool bForced,FString& Why);
 // Variant: "nordic" or "cosmic" (F10), or "shade"; empty: the seed's (S47RoadGlimpse::FigureFor).
 bool BeginFigure(const FString& Variant,bool bForced,FString& Why);
 void StepFigure(double Dt);
 // The crossing far ahead now on the road (road_glimpse): its body, side, distance, and whether it stops or leaves prints.
 S47RoadGlimpse::FPlan Glimpse;
 // What the figure did, for the one record at the end: each time it stood and was seen, how far and which way.
 TArray<FString> FigureSeen;
 double FigureSeenFor=0;int32 FigureStageSeen=-1;
 void StepCar(double Dt);
 void StepCattle(double Dt);
 void StepLanding(double Dt);
 void StepHitcher(double Dt);
 void StepHumor(double Dt);
 // The rest of the 31 (S47Happenings.cpp, "the rest of NATTHENDELSER-31").
 bool BeginMore(const FString& Id,const FString& Variant,bool bForced,FString& Why);
 bool StepMore(double Dt);
 void UsePhone();
 int32 HoodNight=0; // the truck's hood was warm tonight: the counter finds it too
 void UseSite(const FString& Kind);
 void UseCattle();
 void UseHitcher();
 void UseHumor();
 // ---- the world ----
 FSpot Where(const FString& Kind,uint32 All,uint32 Any,uint32 Not,double MinM,double MaxM,float FarShare,uint32 Salt,bool bSameArea=false) const;
 // CHAIN-1-20261109: the lead's own place for the event that is tonight's lead, when it fits (LeadFits). Not set: draw as before.
 FSpot LeadSpot(const FString& Kind,double MinM,double MaxM) const;
 void UsedPlace(const FString& Place,const FString& Kind) const;
 bool PlayerRoad(double& X,double& Z) const;
 bool WorldOf(double RoadX,double RoadZ,FVector& Out) const;
 bool RoadOfWorld(const FVector& W,double& X,double& Z) const;
 bool Outdoors() const;
 bool Driving() const;
 bool OnHighway(double& RoadX,double& RoadZ,double& TruckSpeed) const;
 bool CanHearCb() const;
 bool Seen(const FVector& At,double Range,double Deg=32) const;
 // Seen in fact: in view and range (Seen), nothing solid between (FS47RoswellWorld::ClearSight, past the prop itself) and lit (LitBy).
 // How: what light it can be seen by, LitOwn / LitSky / LitGround. Own: the prop kind that is the thing, not in the way of itself.
 static constexpr uint32 LitOwn=1,LitSky=2,LitGround=4;
 bool SeenLit(const FVector& At,double Range,double Deg,uint32 How,const TCHAR* Own=nullptr) const;
 FVector Eye() const;
 const FSite* SiteOf(const FString& Kind) const;
 FSite* SiteOf(const FString& Kind);
 FString DirectionFrom(const FVector& From,const FVector& To) const;
 int32 Branches() const;
 // ---- what is drawn ----
 FProp* Prop(const FString& Kind);
 FProp* FindProp(const FString& Kind) const;
 // Tilt: laid on the slope it stands on (the ring), identity for what stands upright.
 void PoseProp(FProp& P,const FVector& Foot,double Yaw,const FQuat& Tilt=FQuat::Identity);
 void ShowProp(FProp& P,bool bOn);
 void HideAll();
 void PlaceSites();
 // ---- unease ----
 struct FUneasy{int32 I=-1,Stage=0;double Age=0,Len=0,Clock=0,X=0,Z=0,Yaw=0,Next=0;FVector At=FVector::ZeroVector,Dir=FVector::ZeroVector;bool bForced=false;};
 FUneasy Uneasy;
 TArray<FString> UneaseOnce;  // 3 and 20: never twice in a game (saved)
 TArray<int32> UneaseDone;    // tonight's, never two alike (saved with UneaseNight)
 TArray<int32> UneasePool;    // tonight's draw, in order; the first ones wanted, the rest stand by
 TArray<double> UneaseAt;     // hour of the night (19 to 30) each of the wanted ones may come from
 int32 UneaseNight=0,UneaseWant=0,DangerSeen=0;
 double DangerClock=-1e9,UneaseTry=0,OutsideSince=-1,TruckAwaySince=-1;
 bool bWasDriving=false,bWasIndoors=false,bChairUp=false;
 // Tips heard by nobody yet, kept until a CB (the stranger's own words) or Dale's walkie (his) is there to hear them.
 struct FHeldTip{TArray<FString> Lines;FString Voice,Passed;double Until=0;};
 TArray<FHeldTip> HeldTips;
 void StepTips();
 TFunction<FString()> ClockSay;double ClockRange=-1;bool bClockHeld=false;
 void PlanUnease(int32 Night);
 void StepUnease(double Dt);
 bool FireUnease(int32 I,bool bForced,FString& Why);
 void StepUneasy(double Dt,double Pc=1);
 void EndUneasy();
 // ---- the first big night (S47BigNight.cpp) ----
 int32 BigNight=0,BigVar=-1,BigSerial=0,BigDaleAt=0;
 // CHAIN-4-20261109: which round the state above is (0 the first), and the rounds put by (saved, optional: an older save has none).
 int32 BigRound=0;
 struct FBigPast{int32 Night=0,Var=-1;FString Place,Title;};
 TArray<FBigPast> BigPast;
 void BigNextRound();
 double BigClock=0,BigAz=0,BigEl=3,BigX=0,BigZ=0,BigTry=0,BigFollowTry=0,BigFlick=0,BigDaleIn=0;
 FString BigPlace,BigTitle;
 TSet<FString> BigFollow;
 TArray<FString> BigDaleLines;
 // Stepped during its night in this game (saved): only then is a night slept through the night it happened.
 bool bBigSawNight=false,bBigPower=false,bBigMotors=false,bBigDimmed=false,bBigToneOn=false;
 // F10's natural line: the plan is asked outside the night's own rules too (not saved).
 bool bBigDevNatural=false;
 // The last clock StepBigDue saw (a long jump is a rest), whether the player slept on its night, and the hour a waking brings it on.
 double BigLastClock=-1,BigSoon=-1;bool bBigSlept=false;
 bool BeginBig(const FString& Variant,bool bForced,FString& Why);
 void StepBig(double Dt);
 void StepBigDue();
 void StepBigAfter();
 void EndBigEffects();
 void UseBig();
 void BigTurnArray(bool bSnap);
 void BigRecordArray(bool bWasThere);
 void BigUnseen(int32 Night,bool bSlept);
 void BigFelt();
 bool BigSite(int32 Night,double Clock);
 FString BigSpoken() const;
 S47J::FObj SaveBig() const;
 void LoadBig(const S47J::FObj& O);
 void ResetBig();
 // BIGNIGHT-SEEN-20261109 (S47BigNight.cpp): what is drawn of it (soft light, one set for every area), the wait for the player's eyes
 // before its height, and its words on the walkie, the CB and 1420, one at a time.
 struct FBigNow;
 TSharedPtr<FBigNow> BigNow;
 FBigNow& BigState();
 void DrawBig(double Age,const FVector& Ground,bool bHere);
 void HideBigLook();
 void SayBig();
 void QueueBig(int32 Kind,const TArray<FString>& Lines,const FString& Voice,double In,double Within);
 // A drawn thing of the big night: built if need be, at Foot, each part's size times Scale, its lamps at LampK of their strength.
 bool ShowBig(const FString& Kind,const FVector& Foot,double Yaw,const FVector& Scale,float LampK,bool bOn);
 // ---- the counter ----
 void StepGeiger(double Dt);
 float DoseAt(const FVector& Foot,FString& Near,double& NearM) const;
 void PoseGeiger(bool bShow);
 // D45-20261110: a landing's coals and their smoke at its ring, Hot and SmokeK from 1 down to 0 over the hours after it came down.
 void StepEmbers(FProp& P,double Hot,double SmokeK);
 // ---- the first day's script (S47FirstDay.cpp, D46-20261110) ----
 struct FFirstDayNow
 {
  TSet<FString> Done;
  TArray<TPair<double,TFunction<void()>>> Later; // what a beat does a few seconds on (Paced)
  double LastHour=-1,MotorsAt=0;
  bool bPrimed=false,bLoaded=false,bMissed=false,bMotors=false;
 };
 FFirstDayNow FirstDay;
 TSharedPtr<FS47AnimalWarn> AnimalWarn; // N1110 (ANIMALS-20261108): the animals a few seconds before some events (S47AnimalWarn.cpp)
 TSharedPtr<FS47Calm> Calm; // N1110 (rhythm, items 5 and 11): false alarms and the ease after a danger (S47Calm.cpp)
 void StepFirstDay();
 void EndFirstDay();
 bool FirstBeat(const FString& Id);
 double FirstBearing() const;
 void TurnFirstArray(bool bFound);
};
