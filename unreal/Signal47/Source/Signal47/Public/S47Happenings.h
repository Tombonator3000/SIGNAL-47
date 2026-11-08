#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47Obj.h"
class US47Main;
class AS47Web;
class UStaticMeshComponent;
class UPointLightComponent;
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
 void FillRhythm(FS47RhythmNow& Out) const;
 // A humorous one happened tonight: nothing dangerous may follow it (Tom: never the same night as a danger).
 bool HumorTonight() const;
 void FillMap(FS47MapView& View) const;
 // What the dash clock shows: the game's own, or forty minutes ahead after a passing, until the morning.
 static FString DashClock(const US47Main* M,const FString& Usual);
 double DashAhead() const;
 // F10 and tests: start one here and now, past its draw. Variant: real, empty or wrong for cb_stranded; car, singer, crossing or
 // pump for humor. Why: what stopped it.
 bool DevStart(const FString& Id,const FString& Variant,FString& Why);
 // Whatever is on the ground is over at once; with bSites the lasting ones go too. Test runs only.
 void DevClear(bool bSites);
 // Where a lasting one stands in the area the player is in now (Kind: landing, truck, rv). False: none, or not in this area.
 bool SiteHere(const FString& Kind,FVector& Out) const;
 // Where the one on the ground stands now, in this area (the woman, the car, the singer, the small man, the lights). False: none.
 bool LiveAt(FVector& Out) const;
 bool HasSite(const FString& Kind) const;
 // The counter.
 bool GeigerBuilt() const;
 float GeigerReading() const{return Dose;}
 bool GeigerShown() const{return bGeigerUp;}
 void DevGeiger(bool bGive,bool bHold){bDevGeiger|=bGive;bGeigerHeld=bHold;}
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

 // A place for one of these, with what it needs: the catalogue's, or the land's own.
 struct FSpot{FString Place,Title;double X=0,Z=0;bool bSet=false;};
 struct FProp;
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
 } Now;
 // What stays out on the land after: a landing's ring, a stuck truck, a camper.
 struct FSite
 {
  FString Kind,Variant,Place,Title;
  int32 Night=0,Serial=0;
  double X=0,Z=0,Yaw=0,Az=-1,Spread=0,Until=0;
  bool bFound=false,bDone=false,bHeard=false;
 };
 // For what the interaction list holds on to: lives as long as this does, across every Begin.
 TSharedPtr<uint8> Alive=MakeShared<uint8>(0);
 TArray<FSite> Sites;
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
 float Dose=0,Needle=0;double TickDebt=0,GeigerUpFor=0,GeigerNoteIn=0;bool bGeigerUp=false,bGeigerHeld=false,bDevGeiger=false;
 TSet<FString> GeigerNoted;
 // ---- the night ----
 void PlanTonight(int32 Night);
 double HourOfNight(double Clock) const;
 bool Due(const FString& Id) const;
 bool MayStart(const FString& Id);
 bool StartEvent(const FString& Id,const FString& Variant,bool bForced,FString& Why);
 void EndEvent(bool bEvidence);
 void StepEvent(double Dt);
 void StepSites(double Dt);
 void StepLate();
 void StepSays(double Dt);
 void Say(const FString& Text,double In=0,float Secs=6);
 void Cb(const TArray<FString>& Lines,const FString& Voice,bool bWrong);
 void Deed(const FString& Text);
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
 void StepCar(double Dt);
 void StepCattle(double Dt);
 void StepLanding(double Dt);
 void StepHitcher(double Dt);
 void StepHumor(double Dt);
 void UseSite(const FString& Kind);
 void UseCattle();
 void UseHitcher();
 void UseHumor();
 // ---- the world ----
 FSpot Where(const FString& Kind,uint32 All,uint32 Any,uint32 Not,double MinM,double MaxM,float FarShare,uint32 Salt,bool bSameArea=false) const;
 void UsedPlace(const FString& Place,const FString& Kind) const;
 bool PlayerRoad(double& X,double& Z) const;
 bool WorldOf(double RoadX,double RoadZ,FVector& Out) const;
 bool RoadOfWorld(const FVector& W,double& X,double& Z) const;
 bool Outdoors() const;
 bool Driving() const;
 bool OnHighway(double& RoadX,double& RoadZ,double& TruckSpeed) const;
 bool CanHearCb() const;
 bool Seen(const FVector& At,double Range,double Deg=32) const;
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
 TFunction<FString()> ClockSay;double ClockRange=-1;bool bClockHeld=false;
 void PlanUnease(int32 Night);
 void StepUnease(double Dt);
 bool FireUnease(int32 I,bool bForced,FString& Why);
 void StepUneasy(double Dt);
 void EndUneasy();
 // ---- the counter ----
 void StepGeiger(double Dt);
 float DoseAt(const FVector& Foot,FString& Near,double& NearM) const;
 void PoseGeiger(bool bShow);
};
