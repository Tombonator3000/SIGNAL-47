#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
struct FS47MapView;

// Freeform only: somewhere to drive to (Tom, 7 November). Small places out on the land, each with one thing to find, and a place
// lights can stand over. Simple props from basic shapes where the project has no model; they want real models later. Heard
// about on the radio, or found: a pencil mark on the notebook's map. A registered discrepancy (or work with a mended branch) gives a
// bearing, a sector on the same map and a slip on the printer: which way, never what. It is cold by the next noon. Also where a cot can be put up, and what the magnetometer reads.
class FS47Places
{
public:
 explicit FS47Places(US47Main* Main):M(Main){}
 struct FDef{const TCHAR* Id;const TCHAR* Title;const TCHAR* Kind;double X,Z;const TCHAR* Finding;const TCHAR* Needle;};
 static const TArray<FDef>& Defs();
 static const FDef* Def(const FString& Id);
 static FString UseId(const FString& Id){return TEXT("ff:place:")+Id;}
 // T16-17 (→ M1): the two things to look at inside the old farmhouse (the kitchen table's tally book, the back room's chair at the
 // window). They stand on SARO's ground like the motel's desk, but their X and Z are the land's (the house's own frame), so Spot,
 // the needle's history and the counter read them as any place. Never on the map, never what a slip points at, no cot, no trip camera.
 static bool InOldHouse(const FString& Id){return Id==TEXT("kitchen")||Id==TEXT("backroom");}
 static bool AtSaro(const FString& Id){return Id==TEXT("motel")||InOldHouse(Id);}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Found(const FString& Id) const{return bHasFound(Id);}
 bool Heard(const FString& Id) const{return HeardOf.Contains(Id);}
 void Hear(const FString& Id);
 // The first place the player has neither found nor heard of, by this game's seed; empty when there is none.
 FString NextUnknown(int32 Salt) const;
 bool Use(const FString& Id);
 void FillMap(FS47MapView& View) const;
 // Registered work gives one bearing a night: a discrepancy always (LOOP-1-20261109, HELHETLIG_LOKKE step 1: also with no branch
 // mended, then wide), routine work only with mended branches. The Director holds the night's one pointing-out (MayPointOut) and
 // the tiers (MayPointAt). The bearing goes on the map and its slip on the OPS printer (FS47FreeformSession::QueueBearingSlip).
 // True if it gave one.
 // LOOP-SLIP-IN-PACKET-20261113 (→ M3, Tom 13 November): for a discrepancy (Packet: the job's take) the slip is not a paper of its
 // own: it is decided here, by the same rules and in the same order, spends the night's pointing-out here, and is held for the job's
 // registered packet (HeldFor), whose last page it prints as (PacketSection). The map, the record and the toast come when the packet
 // is collected (ReleasePacket). LOOP-CAREFUL-PAYS-20261113 (→ M3, M2): a sky fix made by hand and reel-deck marks tight round the
 // ticks each take one step off the spread (SpreadFor) and so the circle (RingFor), down to the narrowest.
 struct FTake{FString Job;bool bHandFix=false,bTightMarks=false,bOut=false,bOwnMarks=false,bTicks=false;double MarkIn=-1,MarkOut=-1;};
 bool AfterRegistered(bool bAnomaly=false,const FTake* Packet=nullptr);
 // Why bits: 1 a hand fix, 2 marks tight, 4 a take with extra ticks (bTicks: the reel-deck marks are possible). Steps: how many were taken off (clamped at the narrowest step the branches leave).
 struct FHeld{FString Job;int32 Branches=0,Steps=0;uint8 Why=0;bool bExtra=false,bSame=false,bOwnMarks=false;double MarkIn=-1,MarkOut=-1;};
 // Pure: the careful steps (0 to 2) and how many of them the spread can still take with Branches mended.
 static int32 CarefulSteps(bool bHandFix,bool bTightMarks){return (bHandFix?1:0)+(bTightMarks?1:0);}
 static int32 StepsTaken(int32 Branches,int32 Steps){const int32 B=FMath::Clamp(Branches,0,4);return FMath::Min(B+FMath::Max(0,Steps),4)-B;}
 // Pure: the packet's last section (OFF-BEAM BEARING ...), upper case, in the slip's plain words.
 static FString PacketSection(const FBearing& B,const FHeld& H);
 // Pure: the line that says why the area is narrower (or how it can be), and the line that ties the bearing to the take.
 static FString CarefulLine(const FHeld& H);
 static FString TieLine(const FHeld& H);
 bool HeldFor(const FString& Job) const{return Held.ContainsByPredicate([&Job](const FHeldFix& X){return X.Info.Job==Job;});}
 FString PacketSection(const FString& Job) const;
 // The packet is collected: the held bearing goes on the map, into the field record and the toast. OutPage/OutDay/bOutExtra: the
 // section and the slip's day, for the notebook's "Bearing slip, D0N". True if there was one.
 bool ReleasePacket(const FString& Job,FString* OutPage=nullptr,int32* OutDay=nullptr,bool* bOutExtra=nullptr);
 int32 HeldCount() const{return Held.Num();}
 // WEEKLY-CASE-20261109: the evening after the first big night the night's one pointing-out is the week's case's (FS47WeekCase): a
 // slip from SARO toward Target (road-local ToX, ToZ), its sector over it, the spread by the mended branches as a registration's.
 // The same rules: once a night (the Director's MayPointOut), on the map and the OPS printer. True if it gave one.
 bool PointAt(const FString& Target,double ToX,double ToZ);
 // A bearing on the map. From SARO, or (bCar) from where the truck stood when the direction finder nulled (From, road-local x and z),
 // so that two from different places can cross. Clock: when it was taken; Until: when it goes cold (the next noon). Older saves have
 // neither (-1): no time, never cold, from SARO.
 // D45-20261110 (Tom: the slip's mark landed far south, off the land): what a slip from SARO points at (To, road-local, never shown
 // exactly) and the pencilled circle "look about here" round it (Ring, R metres), always on the land, inside the bearing's sector; and
 // whether the player has stood inside it while it was live. Saved with the bearing; an older save has none (the wedge alone).
 struct FBearing{int32 Day=0;double Az=0,Spread=0;FString Target;double Clock=-1,Until=-1;bool bCar=false;double FromX=0,FromZ=0;
  bool bTo=false;double ToX=0,ToZ=0;bool bRing=false;double RingX=0,RingZ=0,RingR=0;bool bVisited=false;};
 const TArray<FBearing>& Bearings() const{return Fixes;}
 // D45-20261110: the circle's radius by the spread (the mended branches): 450 m with none, down to 140 m with four or more.
 static double RingFor(double Spread){return Spread>=40?450.:Spread>=20?320.:Spread>=12?240.:Spread>=7?180.:140.;}
 // Pure: the circle round B's To, its centre off To by up to .45 R (Roll 0 to 1), the whole circle inside the rim, not on a hill that
 // stops a truck, its centre inside the sector; pulled halfway back toward To until it is, To itself at worst (the circle then made
 // smaller if the rim is that near). Sets bRing; needs bTo.
 static void RingOf(FBearing& B,float Roll);
 // "south-southwest": the sixteen points, as the survey sheet says them (S47FieldMap::Compass). Pure.
 static FString CompassWord(double Az);
 // "1.1 miles south-southwest of SARO": where the circle is (an older bearing: only which way). Pure.
 static FString SlipWhere(const FBearing& B);
 // The slip's own words under its line: what a branch is and what it does to the area, and where the circle is. Pure, upper case.
 static FString SlipNote(const FBearing& B,int32 Branches);
 // NOW and the task list (FS47FreeformSession::Tasks): each live slip from SARO, newest first, two at most, ticked off once the circle
 // has been stood in (or its place found). The words stay the same done or not, so the note's key holds.
 TArray<TPair<FString,bool>> SlipRows(double Now) const;
 // LOOP-1-20261109: no branch mended is a wide sector (±40), never none; every mended branch narrows it.
 static double SpreadFor(int32 Branches){return Branches<=0?40.:Branches==1?20.:Branches==2?12.:Branches==3?7.:4.;}
 // Whether registered work points out at all: a discrepancy always, routine work with a mended branch.
 static bool Gives(bool bAnomaly,int32 Branches){return bAnomaly||Branches>0;}
 // LOOP-DOSSIER-GIVES-20261113 (→ M3, M4): whether a registration may point out now. 0 no; 1 the night's own (not Spent yet); 2 one more,
 // because a file of the unknown at MARKS has an extra bearing open (FS47FreeformSession::DossierExtraOpen) once the night's own is
 // spent: never for the first day's free slips (bFree), and nothing at all on the big night (bBigOwns). Pure.
 static int32 SlipAllowed(bool bFree,bool bSpent,bool bExtraOpen,bool bBigOwns){return bBigOwns?0:!bSpent?1:(!bFree&&bExtraOpen)?2:0;}
 // CHAIN-1-20261109: what the slip points at, in this order: 1 marks standing at a place, 2 what is out there tonight
 // (FS47Happenings::OutTonight: what has come, or tonight's lead), 3 a place on the land nobody has looked at, 0 nothing (no slip).
 // T16-17 (→ M1, "the bearings run out"): 4, once every place has been found or heard of, the place where tomorrow's daylight trace
 // stands (TracePlace), so that a registration still points somewhere worth the drive.
 static int32 SlipPoints(bool bMarks,bool bTonight,bool bUnknown,bool bTrace=false){return bMarks?1:bTonight?2:bUnknown?3:bTrace?4:0;}
 // T16-17 (→ M1): the place carrying DayTrace on Day (never the motel); empty when none. Pure.
 static FString TracePlace(int32 Seed,int32 Day);
 // D45-20261110 (D45-26): the first night's line. One place on the land for the noon slip and the night's burst to share (inside the
 // rim, never the cairn, which landed off the map in d45): the windmill, which Ward's case paper already puts on the map, while it
 // stands there; otherwise one of the others with a survey tag, by the seed. And the one bearing to it from SARO, off the true line by
 // up to five degrees by the seed (never more than .45 of the narrowest sector it is printed in), whole degrees, never Story's number.
 // The same in any Continue; nothing saved. Pure.
 static FString ArcPlace(int32 Seed);
 static double ArcBearing(int32 Seed,double FromX,double FromZ,double ToX,double ToZ,double Spread);
 // D48-20261110 (D46-18: Tom found "nothing there"): the first day's circle is small and sits on the place: about 150 m, centred on To
 // (smaller only where the rim leaves no room), whatever the branches. Needs bTo. Pure.
 static void ArcRing(FBearing& B);
 // D48-20261110: whether the first day's traces stand at a place (the arc place, on the first two days) and the head of the windmill
 // is turned toward the bearing (compass degrees); the NOW line once the place is found. Pure.
 static bool Traced(const FString& Id,int32 Seed,int32 Day){return Id==TEXT("windmill")&&Day<=2&&ArcPlace(Seed)==Id;}
 // N1110 (rhythm, item 12): the one thing the land shows by day that nobody explains. From day 2 one place a day (seeded, never the
 // cairn or Saucer Rock, which stand off the map) has a trace of the night in plain words, one of four: tracks that stop, a dead circle
 // of grass, a fence wire cut clean, birds all facing one way. Empty: no trace there that day (or day 1). Pure.
 static FString DayTrace(const FString& Id,int32 Seed,int32 Day);
 // D45-20261110: a slip printed in daylight on the first day (08:00 to 18:00 of night 1) does not take the night's one pointing-out:
 // it leaves it for the dark hours (the burst; AfterRegistered: so does any slip that night while the burst is still to come). Once
 // that day, still.
 static bool FreeSlip(int32 Night,double Clock){const double H=FMath::Fmod(FMath::Max(0.,Clock),86400.)/3600.;return Night==1&&H>=8&&H<18;}
 // A bearing from SARO (From, road-local) toward To, as a registration takes it: the spread by the branches, the line off the true
 // one by up to half of it (Roll 0 to 1), whole degrees, cold by the next noon. D45-20261110: it keeps To and its circle (RingRoll).
 static FBearing StationFix(int32 Day,double Clock,int32 Branches,double FromX,double FromZ,double ToX,double ToZ,float Roll,const FString& Target,float RingRoll=.37f);
 // The bearing slip, as the printer has it (D45-20261110, plain words and no degree sign):
 // "DIRECTION 110 DEGREES (EAST-SOUTHEAST), GIVE OR TAKE 40 / LOGGED 19:52 / GOOD UNTIL NOON".
 static FString SlipLine(const FBearing& B);
 // A clock for paper, hh:mm, that is never Story's (08:15, 20:15, or a 47 anywhere): a minute on is as good.
 static FString SlipClock(double Clock);
 // Whole degrees 0 to 359 that never read as Story's number (S47Happenings' Bearing rule: 47, 147 ... become a degree more).
 static double Whole(double Az);
 // The next noon after Clock: when a bearing taken then goes cold.
 static double ColdAt(double Clock);
 // 1 just taken, falling to 0 at Until; 1 for an older one with no time. Live: not cold yet.
 static double Freshness(const FBearing& B,double Now);
 static bool Live(const FBearing& B,double Now){return B.Until<0||Now<B.Until;}
 // LOOP-4-20261109: whether the Geiger counter ticks at the place a bearing points at: only while it is live (a cold one gives no dose).
 static bool Dosed(const FBearing& B,double Now){return Live(B,Now)&&Def(B.Target)!=nullptr;}
 // LOOP-4-20261109: the magnetometer at a place, with its history (FS47Happenings::StirredNear): the place's own reading, and when
 // something happened near it in the last two nights, a needle that will not settle. Pure.
 static FString NeedleReading(const FString& Needle,bool bStirred);
 // T16-17 (→ M1): the back room after a night with something near the farm, and the line for a second look that finds nothing new
 // (bHouse: the old house's own). Pure.
 static FString BackRoomAgain();
 static FString NothingNew(bool bHouse);
 // Where the middle lines of two bearings from different places meet, ahead of both (road-local); false when they do not.
 static bool Cross(const FBearing& A,const FBearing& B,FVector2D& Out);
 // Whether a road-local point is inside a bearing's sector.
 static bool InSector(const FBearing& B,double X,double Z);
 // The direction finder (FS47Outfit): a bearing from the truck, road-local, toward Target (what keyed the band). One near the last
 // from the truck toward the same thing replaces it; a few kept per target. True when it is a new line on the map.
 static constexpr double CarSpread=8,CarApart=300;
 static constexpr int32 MaxFixes=12;
 // FIX10-9 (→ M3): the same kind of line from SARO itself, pencilled from a control room window toward a figure seen on the rise
 // (FS47Happenings): its target, so that the map says "Day N window" and never doses the counter (no place).
 static constexpr const TCHAR* WindowTarget=TEXT("window:figure");
 bool AddCarFix(double FromX,double FromZ,double Az,const FString& Target,double Clock,int32 Day);
 // FillMap with a clock: what is live, faded as it cools, the known places inside a sector marked, and where lines cross.
 void FillMapAt(FS47MapView& View,double Now) const;
 static double AzimuthOf(double FromX,double FromZ,double ToX,double ToZ); // Degrees, north 0, east 90. Road-local: z runs south.
 // Some small things are not where they were: metres from where they first stood, following from the game and the day, so a
 // Continue changes nothing. Only what could be carried. Only moved while nobody is near.
 static FVector2D DriftOf(const FString& Id,int32 Seed,int32 Day);
 static bool Drifts(const FString& Id);
 // Where the place really stands, road-local, after the land has had its say.
 bool Spot(const FString& Id,double& OutX,double& OutZ) const;
 // D48-20261110: where a built place stands with its ground, three-space world metres (the motel on SARO's ground, the rest on the
 // road's), and whether the player's own floor has been measured there yet (Seat). False before it is built.
 bool Stand(const FString& Id,FVector& OutWorld,bool& bOutSeated) const;
 // The player's own pencil marks on the notebook's map (MAP-20261108): road-local, numbered in the order they were made. One within
 // Within metres of a mark already there rubs that one out instead. Saved; false when nothing changed.
 const TArray<FVector2D>& MapMarks() const{return Marks;}
 bool ToggleMark(double X,double Z,double Within);
 static constexpr int32 MaxMarks=40;
 // PT-SAUCER-ROCK-20261109: what Saucer Rock shows by the hour. 0 day (and the hours before dawn, and to 18:30), 1 dusk 18:30 to
 // 19:30, 4 the rest of the evening to 24, 2 after midnight to 2, 3 after 2 to the night's end. The text for each (bMap: the player
 // has the souvenir man's map).
 static int32 SaucerTier(double Hour);
 static FString SaucerAt(int32 Tier,bool bMap);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 struct FBuilt{FString Id;double X=0,Z=0,Y=0;bool bSeated=false;FVector2D Shift=FVector2D::ZeroVector;int32 ShiftDay=0;TArray<TWeakObjectPtr<UStaticMeshComponent>> Parts;TArray<FVector> Local;FS47Obj Hit;int32 TraceFrom=-1;bool bTraceOn=true;};
 TArray<FBuilt> Built;
 TSet<FString> HeardOf;
 TArray<FBearing> Fixes;
 struct FHeldFix{FBearing B;FHeld Info;};
 TArray<FHeldFix> Held; // LOOP-SLIP-IN-PACKET-20261113: bearings waiting for their packet, saved ("held"), never on the map
 static constexpr int32 MaxHeld=4;
 static bool ReadFix(const S47J::FObj& O,FBearing& B);
 static S47J::FObj WriteFix(const FBearing& B);
 TArray<FVector2D> Marks;
 TSharedPtr<uint8> Life;
 TWeakObjectPtr<AS47Web> Scene;
 bool bBuilt=false;
 double NeedleIn=0,SlipIn=0;
 bool bMillThought=false; // D48-20261110
 int32 LastFixDay=0;
 void StepSlips(double Dt); // D45-20261110: the player inside a live slip's circle checks it
 static void SaroRoad(double& X,double& Z);
 bool bHasFound(const FString& Id) const;
 // T16-17 (→ M1): a slip is ticked off once its circle is stood in, or its place found; a slip toward a place already found for the
 // trace the next morning leaves there (TracePlace), once that trace is kept.
 bool Checked(const FBearing& B) const;
 bool Build();
 void Seat(FBuilt& B);
 void StepNeedle(double Dt);
 void Panel(const FDef& D,bool bFirst,const FString& Extra);
};
