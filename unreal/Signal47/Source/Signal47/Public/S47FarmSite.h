#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47VisitAttention.h"
#include "S47RuleBreak.h"
class US47Main;
class AS47Web;
class FS47FreeformSession;
class UStaticMeshComponent;
class UPrimitiveComponent;
class ULocalFogVolumeComponent;
struct FS47EventPlace;
enum class ES47Clip : uint8;

// Freeform only: what the lights leave behind until the next morning, at the farm or a place in town. Pressed
// ground, a low local haze and an occasional sound from that spot; looking closer keeps a field record.
class FS47FarmSite
{
public:
 explicit FS47FarmSite(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 void ArmFarm(double Clock);
 // Finds open ground beside Point, so the marks are never inside the building they name.
 void Arm(double Clock,const FString& InPlace,const FString& InTitle,const FString& InArea,const FVector& Point);
 bool Use();
 FString Label() const;
 bool Armed() const{return ClearAt>0;}
 // MILITARY-20261108: the marks' night was gone over in the morning (S47Visitors::Cleared): no marks, nothing to read, no flake.
 bool Raked() const;
 // Whether a body is coming or here at this site, so that spawning may use the open ground round the player.
 bool VisitWantsGround() const{return Visit>=1&&Visit<=3;}
 int32 VisitStage() const{return Visit;}
 // The quiet before it: the night's own sounds stop a few seconds ahead of the warning, and stay stopped.
 bool VisitHushed() const{return VisitWantsGround()||(Visit==0&&VisitDwell>5);}
 // The warning has no body yet: what a radio breaks up on is the place itself.
 TOptional<FVector> VisitWarningAt() const{return Visit==1?TOptional<FVector>(Middle+FVector(0,1,0)):TOptional<FVector>();}
 // Test: a visit now, past the dice. The player counts as noticed whatever they do, for the tests of what happens if it reaches them.
 void TestVisit(int32 Who=0){Visit=1;VisitAge=0;bVisitSeen=false;bVisitTaken=false;VisitWho=Who;bTestNoticed=true;}
 int32 VisitKind() const{return VisitWho;}
 // Marks that will still be there at this clock, with a visit still to come.
 bool WillStandAt(double Clock) const{return (Visit==0||bFalseCall)&&ClearAt>Clock&&Clock>=CalmUntil;}
 // The second report, on the set if the player has it. False: no marks to report.
 bool WakeCall();
 bool Nudged() const{return bNudged;}
 // Some second calls lead nowhere: the marks have nothing coming, or on a calm night there are no marks at all.
 bool FalseCall() const{return bFalseCall;}
 static FString SecondCallLine(int32 Index,const FString& About);
 static constexpr int32 SecondCallLines=4;
 // Test: how high the moved body's head is over its feet, -1 when it is not out.
 double VisitHeadHeight() const;
 // Test: the next visit's draw, in place of the dice. Goes through PickWho like any other.
 void TestRollWho(float Roll){TestRoll=Roll;}
 // No body before about two in the morning (Tom, 28 October: the evening is a normal job). Lights and sounds may.
 static bool TheirHours(double Clock);
 // Who comes: 0 Roswell, 1 crab, 2 tall. The open pasture is where the crab comes; town is mostly the small ones;
 // the tall one keeps to the last of the night. Roll is 0 to 1.
 static int32 PickWho(const FString& InPlace,double Hour,float Roll);
 double VisitBodyDistance() const;
 // Tests: what the body is doing about the lamp and noise, its next lamp draw in place of the dice, and the visit's two clocks.
 const FS47VisitAttention& VisitAttention() const{return Attention;}
 void TestAttentionRoll(float Roll){AttentionRoll=Roll;}
 // Just out of the lamp's reach (FS47VisitAttention::BeamReach): nearer than this the tall one is on its feet.
 static constexpr double SitBeyond=26;
 // Tests: the next tall visit's draw to sit far off, in place of the dice, and the clip the body out now is playing.
 void TestSitRoll(float Roll){SitRoll=Roll;}
 ES47Clip VisitClipNow() const;
 // Test: no hunt and no surprise from the set, so that each reaction can be looked at by itself.
 void TestHoldHunt(){bTestHoldHunt=true;}
 void TestFreshPatience(){VisitPatience=120;}
 double VisitPatienceLeft() const{return VisitPatience;}
 double VisitHuntClock() const{return VisitHunt;}
 // D48-20261110: the body out now as the camera can see it (its skinned mesh, only while it is drawn), and the marks standing, drawn.
 UPrimitiveComponent* VisitPhotoBody() const;
 TArray<UPrimitiveComponent*> PhotoMarks() const;
 TOptional<FVector> VisitBodyEye() const{return bBodyOut?TOptional<FVector>(BodyAt+FVector(0,VisitWho==1?.6:1.9,0)):TOptional<FVector>();}
 // D45-20261110: the body out now walks at the player and is near (13 m), not held back by a lamp: what the music calls being
 // come at (FS47FieldRadio). Standing off, sitting or walking up from far off is not.
 bool VisitBodyComing() const{return bBodyOut&&bBodyComing;}
 // N1110 (N1110-7): the door loop's chase has the player now: the body goes, and how it ends is the loop's (gone, or the taking),
 // so it leaves no record of its own here.
 void YieldToDoors(){if(Visit>=1&&Visit<=3)EndVisit(true);}
 // After being taken, no new visit before this clock.
 void SetCalm(double Until){CalmUntil=Until;}
 void TestReadyVisit(){Visit=0;VisitDwell=0;}
 FVector Centre() const{return Middle;}
 FString FindingId() const;
 // The second look the note asks for: what the marks are by daylight, the morning after. 0 ordinary, 1 undecided, 2 strange;
 // fixed by the game, the place and the day, so looking again never changes it.
 FString DaylightId() const;
 static int32 DaylightRead(int32 Seed,const FString& InPlace,int32 Day);
 static FString DaylightText(int32 Read,bool bFarm);
 static bool ByDaylight(double Clock);
 FString PlaceId() const{return bField?Home.Place:Place;}
 FString PlaceTitle() const{return bField?Home.Title:Title;}
 // A visit with no marks at all: someone on foot, out from under any roof past the station, in their hours (T16-17, 4). Its own
 // draw once a night, once they have been out there a while. The marks somewhere else, if any, stand as they were.
 bool FieldVisit() const{return bField;}
 static constexpr double FieldDwellSeconds=60;
 // Tom, 8 November: out on open ground a dangerous meeting about every second or third night, never the first. The night's
 // chance (FS47NightRhythm::FieldChance) and this much more for each draw out there that came to nothing since the last.
 static constexpr float FieldMissBonus=.15f;
 static float FieldChanceFor(int32 Night,int32 Misses,bool bLit);
 static bool OutOnFoot(const FString& Area,double FromStation,bool bDriving,bool bRoof);
 int32 FieldDrawnNight() const{return FieldNight;}
 // PT-FEEDER-20261109 (d37, Tom: «lights at the remote feeder ... skal det være noe mer?»). A trip out to a reported place always
 // brings something home (HELHETLIG_LOKKE: an empty trip rules a place out or gives a lead). What is there, by the hour of arrival:
 //   always      the arrival record (below), the marks to look closer at, the sound from the place, the haze, the look by daylight
 //   18 to 24    a smell of struck matches over the marks: a hint, explained away by a truck or a fuse
 //   24 to 02    singed grass or scorched brush round the marks; a figure far off past them, if nothing is coming to them
 //   after 02    something dead at the edge of the marks; the figure far off, or the visit (the dice, the rhythm)
 //   morning     what the night left, by daylight
 // The arrival record is kept the first time the player is on foot within ArriveWithin of the marks: a field record in the notebook
 // and a lead in OUT ON THE LAND (FS47FreeformCases::LineForFinding, lights_trace_), and the toast says where the marks are.
 static constexpr double ArriveWithin=30;
 static constexpr int32 ArrivalTiers=4,ArrivalPicks=2; // tiers: 0 hint, 1 glimpse, 2 full, 3 by daylight
 static FString ArrivalKey(const FString& InPlace,int32 Night){return FString::Printf(TEXT("lights_trace_%s_arrive_d%d"),*InPlace,Night);}
 // Hour: the hour of the day (0 to 24) the record is kept at; its time words come from the clock, the tier only picks what is there
 // (CRIT9). Under 0: the tier's own word (the self-tests).
 static FString ArrivalText(int32 Tier,int32 Pick,const FString& InTitle,double Hour=-1);
 static FString ArrivalWhen(int32 Tier,double Hour);
 static FString ArrivalToast(int32 Tier);
 // The night the marks were made (they clear at the next noon), 0 with none.
 int32 SiteNight() const;
 // The report on tonight's note is done (FS47FreeformDirector::ReportDone): the player has been there and found something (the
 // arrival record, the marks looked at, the daylight look), or found the place gone over and empty.
 bool Checked() const;
 static bool CheckedIn(const FS47FreeformSession& F,const FString& InPlace,int32 Night,bool bEmpty);
 // The big night owns its night (FS47Happenings::BigOwns): marks standing tonight stay what they are, a record to find, but nothing
 // more comes to them, no second call is made about them and no figure stands past them.
 void YieldTo(const TCHAR* Why);
 bool Yielded() const{return bYielded;}
 // Test: marks standing until Until with a visit still to come, as Arm leaves them, without a world.
 void TestArm(double Until){ClearAt=Until;Visit=0;bNudged=false;bFalseCall=false;bYielded=false;bCheckedEmpty=false;}
 // Test: the next field draw, in place of the game's own.
 void TestFieldRoll(float Roll){FieldRoll=Roll;}
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TSharedPtr<uint8> Life;
 TArray<TWeakObjectPtr<UStaticMeshComponent>> Marks,Spots;
 TWeakObjectPtr<ULocalFogVolumeComponent> Haze;
 FS47Obj SpotSaro,SpotRoad;
 FString Place,Title,AreaId;
 FVector Middle=FVector::ZeroVector;
 double ClearAt=0,SoundIn=0;
 // A visit: 0 may come, 1 the warning, 2 it sneaks toward the player, 3 it hunts, 4 over for this site.
 int32 Visit=4;
 double VisitAge=0,VisitDwell=0;
 bool bVisitSeen=false,bVisitTaken=false;
 int32 VisitRounds=0;
 double CalmUntil=0;
 int32 VisitWho=0; // 0 Roswell, 1 crab, 2 tall
 // Crab and tall are moved here; the small ones have their own brain.
 // One each, so that each can be given its own model (S47KindBody); VisitBody is the one out now.
 TSharedPtr<struct FS47PaleBody> VisitBody,CrabBody,TallBody;
 FVector BodyAt=FVector::ZeroVector;
 double BodyYaw=0,BodyClock=0;
 static constexpr double ShhWithin=12;
 // The tall one stands where it is for this long, to be found. Nothing comes of it before that: not walking, not its light.
 static constexpr double TallStands=12;
 double ShhLeft=0;
 bool bShhTried=false;
 // The tall one. Stopped dead the first time a lamp finds it; sitting, some visits, while it is too far off for any lamp.
 static constexpr double SitChance=.5;
 double FreezeLeft=0;
 bool bFreezeTried=false,bSitsFar=false,bSatLogged=false;
 float SitRoll=-1;
 bool bBodyOut=false,bBodyComing=false;
 bool bStallSet=false,bNudged=false,bFalseCall=false;
 int32 EmptyTipNight=0;
 bool Hours(double Clock) const;
 // The place on the list these marks are at (S47EventPlaces), and what its ground is: open, stock, town. Null: not on it.
 const FS47EventPlace* PlaceOnList() const;
 bool OpenGround() const;
 FString GroundKind() const;
 struct FHome{FString Place,Title,Area;FVector Middle=FVector::ZeroVector;int32 Visit=4;};
 FHome Home;
 bool bField=false;
 int32 FieldNight=0;
 // Draws out on foot since the last field visit came, saved: each one that came to nothing makes the next likelier.
 int32 FieldMisses=0;
 double FieldDwell=0;int32 FieldHeldNight=0;
 float FieldRoll=-1;
 void StepFieldDraw(double Dt);
 void RestoreHome();
 bool Live() const{return Armed()||bField;}
 // PT-FEEDER-20261109: the arrival record, the place found gone over, and the figure far off past the marks (asked for once).
 bool bYielded=false,bCheckedEmpty=false,bFigureAsked=false;
 double FigureIn=0;
 void StepArrival(double Dist);
 void StepFigure(double Dt,double Dist);
 void StepEmptyTip();
 double HushIn=0;
 float TestRoll=-1,AttentionRoll=-1;
 FS47VisitAttention Attention;
 // N1110 (N1110-6): the rule taught (lamp off and still, they pass) and broken once by the Pale One (S47RuleBreak.h); StillHeld: seconds
 // the small one has lost the player so far this visit.
 FS47RuleBreak RuleBreak;
 double StillHeld=0;
 // Hunt clock and how long it stays interested. Both slow or quicken with what the player gives it to go on.
 double VisitHunt=0,VisitPatience=120,RadioIn=-1;
 FVector AttentionFeet=FVector::ZeroVector;
 bool bAttentionFeet=false,bWasDriving=false,bWasRunning=false,bTestHoldHunt=false,bTestNoticed=false,bSawTurn=false,bSawSetTurn=false;
 int32 RadioSeen=0;
 bool PutBody(const FVector& Near);
 void StepBody(double Dt,double Speed);
 void HideBody();
 void SayNudge();
 int32 BodyKind=0; // the kind whose body is out now, whatever VisitWho says
 void StepVisit(double Dt,double Dist);
 void StepAttention(double Dt,bool bDriving,bool& bNothingToGoOn);
 void LetAttentionGo();
 void EndVisit(bool bTaken);
 bool bBuilt=false,bActive=false,bBound=false,bShown=false;
 bool Build();
 void Show(bool bShow);
 void Layout();
 double GroundAt(double X,double Y,double Z) const;
 bool Here() const;
};
