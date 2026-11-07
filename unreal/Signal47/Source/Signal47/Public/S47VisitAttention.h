#pragma once
#include "CoreMinimal.h"

// What a body on a visit does about the player's lamp and noise, in its own short moments. No world and no save here; the
// visit (FS47FarmSite) measures and carries it out. FF-RESEARCH-07: readable, bounded, and it never leads to a hunt: when a
// moment is over the body goes back to what it was doing, and a pause between moments keeps a lamp from pinning it for good.
class FS47VisitAttention
{
public:
 enum class EDo : uint8 { None, Watch, Back, Blink, Turn };
 struct FIn
 {
  bool bBeam=false;    // a lit lamp is pointed at it, in reach, in a clear line
  bool bLampOn=false;
  bool bNoise=false;   // something it can hear began just now
  double BodyDist=100; // from the player
  FVector Lamp=FVector::ZeroVector,Sound=FVector::ZeroVector;
 };
 static constexpr double BeamReach=25,WatchSeconds=1.4,BackSeconds=5,BackSpeedMps=.55,BlinkSeconds=1.1,TurnSeconds=2.6,TooNear=2.5;
 void Reset(){*this=FS47VisitAttention();}
 // Roll: 0 to 1, drawn when a lamp is noticed. It backs away from most, and puts out some.
 void Step(double Dt,const FIn& In,float Roll);
 EDo Doing() const{return Do;}
 bool Holding() const{return Do!=EDo::None;}
 bool Heard() const{return Do==EDo::Turn;}
 FVector LookAt() const{return Look;}
 double BackSpeed() const{return Do==EDo::Back?BackSpeedMps:0.;}
 // For FS47Flashlight::Flicker. With calm flashes chosen it only unsettles the lamp.
 double BlinkLevel(bool bCalm) const{return Do==EDo::Blink?(bCalm?.3:1.):0.;}
 // What has happened this visit, for the field record.
 bool bDidBack=false,bDidBlink=false,bDidTurn=false;
 static const TCHAR* Name(EDo Value);
 static bool SelfTest(TArray<FString>& Lines);
private:
 EDo Do=EDo::None;
 FVector Look=FVector::ZeroVector;
 double T=0,LampWait=0,SoundWait=0,BeamOff=0;
 bool bWillBack=true;
 void Let(double LampPause,double SoundPause);
};
