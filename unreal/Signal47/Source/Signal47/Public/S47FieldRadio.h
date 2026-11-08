#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47EncounterSound.h"
#include "S47Obj.h"

class US47Main;
class UBoxComponent;

// SARO's carried field set. Room 6's batteryless historical radio remains independent.
class FS47FieldRadio
{
public:
 explicit FS47FieldRadio(US47Main* Main):M(Main) {}
 ~FS47FieldRadio();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Owned() const { return bOwned; }
 bool Ready() const { return Root.IsValid(); }
 void Take();
 // Comes over the carried set, if there is one. False: the player has no walkie to hear it on.
 bool Message(const FString& Text);
 // How many have come over the set, for whatever may hear it there too.
 int32 MessagesHeard() const { return MessageCount; }
 void TestOwn() { bOwned=true; }
 // Left on the ground where the player was taken (FS47LeftGear). Not on the desk either until it is fetched or handed in.
 void Lose() { bOwned=false; bLeftOut=true; }
 void Recover(bool bToHand) { bLeftOut=false; bOwned=bToHand; }
 bool LeftOut() const { return bLeftOut; }
 // Something said, out of the air at this place, with no radio involved.
 void Talk(const FVector& At);
 struct FPresence { FVector At=FVector::ZeroVector; bool bActive=false,bVisible=false,bHunting=false; };
 // Reach: nothing at or beyond this many metres. Story's set hears them at the default; the Freeform set further off.
 static double Proximity(const FVector& Eye,const FPresence& In,double Reach=18.);
 static constexpr double FreeformReach=48.;
 // Test: what the set is playing, 0 to 1, and how many bursts of crackle since the session began.
 double RadioLevelNow() const { return LastRadio; }
 int32 CrackleCount() const { return Crackled; }
 // For the HUD and the stress model (read only): how near a real presence is, 0 to 1 (the same smooth reach as the interference),
 // whether one is in view, whether one is hunting, and what the carried set is doing just now.
 double PresenceLevel() const { return LastLevel; }
 bool PresenceSeen() const { return LastSeen; }
 bool PresenceHunting() const { return LastHunt; }
 // The walkie is in use: it is carried and something has just come over it, or it is breaking up with a presence near.
 bool InUse() const { return bOwned && !bLeftOut && (SinceMessage<20. || LastRadio>.02); }
 // Signal strength for the HUD, 0 to 1: full in a quiet place, thinner the nearer a presence is.
 double Strength() const { return FMath::Clamp(1.-.8*LastRadio,.1,1.); }
 // Music reads only a nearby visible body, never a hidden encounter schedule.
 double ObservedThreat() const;
 static double ObservedThreat(const FVector& Eye,const TArray<FPresence>& Visible);
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 FS47Obj Root;
 TWeakObjectPtr<UBoxComponent> Hit;
 TArray<TWeakObjectPtr<class UStaticMeshComponent>> Copies;
 FS47EncounterSound Sound;
 bool bOwned=false,bUnsupported=false,bAvailable=false,bLeftOut=false;
 S47J::FObj Unknown;
 double LastLevel=0,QuietFor=0,SeenFor=0,ThreatFor=0,TestT=0,LastRadio=0,CrackleIn=0;
 int32 Crackled=0,MessageCount=0;
 double SinceMessage=1e9;
 bool LastSeen=false,LastHunt=false;
 int32 LastCue=-1,TestStage=0;
 bool Build();
 TArray<FPresence> Nearby() const;
 bool InView(const FVector& At) const;
 void ControlTest(double Dt);
};
