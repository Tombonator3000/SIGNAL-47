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
 // Comes over the carried set, if there is one. False: the player has no walkie to hear it on. Dale's ("DALE: ...") come only on
 // channel 1; a voice too broken to make out comes on any.
 bool Message(const FString& Text);
 // How many times the set has made itself heard (a message, a hail, the player keying it), for whatever may hear it there too.
 int32 MessagesHeard() const { return MessageCount; }
 void TestOwn() { bOwned=true; bLeftOut=false; Chan=1; }

 // WALKIE-USE-20261108: the set in the player's own hand. Three channels: 1 is SARO's and Dale's, 2 the district's service band,
 // 3 nobody's. The walkie key (T) answers Dale when he hails, or calls on the channel it is on; the channel key (Y) turns it on.
 // The set's own sound is its own: a two-tone call, a key click and a roger beep at the end of each over, apart from the CB.
 static constexpr int32 Channels=3;
 static double ChannelMHz(int32 Channel);
 int32 Channel() const { return Chan; }
 void NextChannel();
 // T. False: not now (a panel, a held scene); with no set in hand it says where the set is.
 bool PressTalk();
 // Dale's hail on channel 1: the set calls, he asks for you, and again a while later. Answered (T) in time: OnAnswer, and he
 // says his piece. Otherwise GiveUp is heard and OnMissed is called. False at once, with neither called, when there is no set to
 // hear it on or it is on another channel.
 struct FHail { FString First, Again, GiveUp; };
 bool Hail(const FHail& Words, TFunction<void()> OnAnswer, TFunction<void()> OnMissed);
 bool Hailing() const { return HailLeft>0; }
 bool Transmitting() const { return TxLeft>0; }
 // The set made a sound of its own just now: something close by can hear it (FS47RoswellGroup).
 bool LoudNow() const { return bOwned && !bLeftOut && SinceLoud<1.2; }
 // CB-WALKIE-20261108: the CB is strangers, the walkie is Dale. A stranger's report (a caller, a driver, dispatch) comes over the
 // CB if one is to hand (in the truck, or the base set in the SARO building), with the CB's own sound and HUD line. Away from one,
 // Dale passes it on over the walkie, in his own words, on channel 1. With neither, it waits a while for one of them (HeldTips).
 static void Report(US47Main* M, const FString& Tip);
 static bool CbAtHand(const US47Main* M);
 // How long a report waits for a CB or Dale before it is let go, in seconds of play.
 static constexpr double HeldTipSeconds=240;
 int32 TipsWaiting() const { return HeldTips.Num(); }
 // A call on the CB with its sound and the HUD's CB line. False when it is busy or there is no sound: nothing was said. OnBegun:
 // once, as its first line comes (a call cut off before that never began).
 static bool CbSay(US47Main* M, const TArray<FString>& Lines, const FString& Voice, TFunction<void()> OnBegun = nullptr);
 // The caption's head for the set: "WALKIE 1", or with who is speaking, "WALKIE 1, DALE".
 static FString Head(int32 Channel, const FString& Who = FString());
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
 // The walkie is in use: it is carried and something has just come over it, it is breaking up with a presence near, or the player
 // has just used it (a call, an answer, a turn of the channel).
 bool InUse() const { return bOwned && !bLeftOut && (SinceMessage<20. || LastRadio>.02 || HailLeft>0 || TxLeft>0 || SinceTouch<8.); }
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
 // The player's hand on it (WALKIE-USE-20261108). Chan is saved; the rest is the moment.
 int32 Chan=1,HailCount=0,HintsShown=0;
 double SinceLoud=1e9,SinceTouch=1e9,HailLeft=0,HailAgainIn=0,TxLeft=0,ThenIn=-1;
 FHail HailWords;
 TFunction<void()> HailAnswer,HailMissed,Then;
 // Reports nobody could hear yet: no CB to hand, no walkie on Dale's channel. Said by the first of them to come.
 struct FHeldTip { FString Tip; double Age=0; };
 TArray<FHeldTip> HeldTips;
 bool Deliver(const FString& Tip);
 bool bKeysBound=false,bBandPending=false; // bBandPending: Then is only the look at the service band, and goes if the channel turns on
 void BindKeys();
 void StepHand(double Dt);
 void Reply();
 void Answer();
 void EndHail(bool bHeardGiveUp);
 void KeyUp(const FString& Says);
 FString ChannelLine() const;
 bool Build();
 TArray<FPresence> Nearby() const;
 bool InView(const FVector& At) const;
 void ControlTest(double Dt);
};
