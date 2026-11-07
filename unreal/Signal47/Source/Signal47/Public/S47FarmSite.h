#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
#include "S47VisitAttention.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class ULocalFogVolumeComponent;
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
 bool WillStandAt(double Clock) const{return Visit==0&&ClearAt>Clock&&Clock>=CalmUntil;}
 // The second report, on the set if the player has it. False: no marks to report.
 bool WakeCall();
 bool Nudged() const{return bNudged;}
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
 TOptional<FVector> VisitBodyEye() const{return bBodyOut?TOptional<FVector>(BodyAt+FVector(0,VisitWho==1?.6:1.9,0)):TOptional<FVector>();}
 // After being taken, no new visit before this clock.
 void SetCalm(double Until){CalmUntil=Until;}
 void TestReadyVisit(){Visit=0;VisitDwell=0;}
 FVector Centre() const{return Middle;}
 FString FindingId() const{return Place==TEXT("farm")?FString(TEXT("farm_trace")):TEXT("lights_trace_")+Place;}
 // The second look the note asks for: what the marks are by daylight, the morning after. 0 ordinary, 1 undecided, 2 strange;
 // fixed by the game, the place and the day, so looking again never changes it.
 FString DaylightId() const;
 static int32 DaylightRead(int32 Seed,const FString& InPlace,int32 Day);
 static FString DaylightText(int32 Read,bool bFarm);
 static bool ByDaylight(double Clock);
 FString PlaceId() const{return Place;}
 FString PlaceTitle() const{return Title;}
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
 bool bBodyOut=false;
 bool bStallSet=false,bNudged=false;
 double HushIn=0;
 float TestRoll=-1,AttentionRoll=-1;
 FS47VisitAttention Attention;
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
