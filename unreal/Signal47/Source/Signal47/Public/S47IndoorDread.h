#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47RoswellWorld.h"
#include "S47TripoCreature.h"

class US47Main;
class AS47Web;
class USceneComponent;
struct FS47Cam;

// Freeform only, and only in their hours (FS47FarmSite::TheirHours): what can happen to a player who is indoors at night.
//
// A small Roswell looks in at a window or round the edge of a door: in the SARO building, control room first, at the
// motel, in town or in the old house. Rare (one
// draw a night), short, no danger. Where it stands is measured from the player's eyes, not listed: the ducked or hidden
// pose must be out of sight behind something real and the raised one in sight, so it comes up from cover, never pops in.
//
// And a room the player got out of the light in can be lit again: days later, on a night's own draw, while they are in
// that room. Always the same way out as the first time (FS47Taken).
class FS47IndoorDread
{
public:
 explicit FS47IndoorDread(US47Main* Main):M(Main){}
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;

 // FS47Taken: the light went out with the player still here. Three-space feet, where the light first found them.
 void SurvivedFlood(const FVector& At,const FString& Area,int32 Who,const FString& Title);

 // Taken after all: the room's long rest starts over, so one follows the other only after days.
 void AfterTaken();
 static constexpr double FloodRestSeconds=3*86400.;
 bool HasFloodRoom() const{return !FloodArea.IsEmpty();}
 double FloodRoomClock() const{return FloodClock;}
 FVector FloodRoomAt() const{return FloodAt;}
 int32 FloodRoomRollDay() const{return FloodRollDay;}
 int32 FloodsSurvived() const{return FloodCount;}

 bool PeekActive() const{return Stage!=EStage::None;}
 // For the field radio: where it is, while it is there.
 TOptional<FVector> PeekEye() const;
 // Tests: read only.
 bool PeekIsWindow() const{return bWindow;}
 double PeekClock() const{return T;}
 double PeekExposure() const{return ExposureNow;}
 FVector PeekHiddenHead() const; // the point of it that shows first, in the pose it starts from
 FVector PeekRaisedHead() const;
 int32 PeekRollDayOf() const{return PeekRollDay;}
 bool PeekDrawnTonight() const{return bPeekTonight;}
 int32 Peeks() const{return PeekCount;}
 int32 SearchesEmpty() const{return EmptyRings;}
 bool PlayerInPeekPlace() const;
 // Developer menu: skip the draw and the wait. The search and every physical rule still apply.
 bool DevPeek();
 static bool SelfTest(TArray<FString>& Lines);
private:
 enum class EStage : uint8 { None, Up, Leaving };
 struct FCandidate
 {
  bool bFound=false,bIsWindow=false,bInView=false;
  FVector Feet=FVector::ZeroVector,Cover=FVector::ZeroVector;
  double Score=-10;
 };
 US47Main* M=nullptr;
 TWeakObjectPtr<AS47Web> Scene;
 TWeakObjectPtr<USceneComponent> Root;
 FS47TripoCreature Body;
 FS47RoswellWorld Sight;
 bool bBodyFailed=false;

 // The peek under way.
 EStage Stage=EStage::None;
 FString PeekArea;
 FVector BasePos=FVector::ZeroVector,Cover=FVector::ZeroVector;
 bool bWindow=false,bSeen=false,bDuckSound=false;
 double Facing=0,T=0,Noticed=0,Withdraw=-1,ExposureNow=0,LeftAt=0;

 // The night's draw for it.
 int32 PeekRollDay=0,PeekCount=0,EmptyRings=0;
 bool bPeekTonight=false,bDevWanted=false;
 double InsideFor=0,PeekWait=0;
 int32 ProbeAt=0,SearchRings=0,EmptyRun=0;
 double RingPause=0;
 // Several empty rounds from one spot: no more searching until the player has moved.
 bool bSearchStopped=false;
 FVector StoppedAt=FVector::ZeroVector;
 FCandidate Best;

 // The room that was lit.
 FString FloodArea,FloodTitle;
 FVector FloodAt=FVector::ZeroVector;
 double FloodClock=0,FloodFor=0;
 int32 FloodWho=0,FloodRollDay=0,FloodCount=0;

 bool Quiet() const;
 bool BuildBody();
 void StepPeek(double Dt);
 void StepPeekDraw(double Dt);
 void StepFloodRoom(double Dt);
 void SearchStep(const FS47Cam& Cam);
 bool TryWindow(const FS47Cam& Cam,const FVector& At,FCandidate& Out,bool bKnownWindow=false) const;
 bool InSaroBuilding() const;
 bool TryCorner(const FS47Cam& Cam,const FVector& At,FCandidate& Out) const;
 void ScoreOf(const FS47Cam& Cam,FCandidate& C) const;
 TOptional<double> GroundUnder(double X,double Z,double FromY,double ToY) const;
 bool StillHidden(const FS47Cam& Cam,const FCandidate& C) const;
 void StartPeek(const FS47Cam& Cam,const FCandidate& C);
 void Draw();
 void EndPeek();
};
