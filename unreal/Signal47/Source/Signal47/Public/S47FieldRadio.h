#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
#include "S47EncounterSound.h"
#include "S47Obj.h"

class US47Main;
class UBoxComponent;
class FS47DaleVoice;

// SARO's carried field set. Room 6's batteryless historical radio remains independent.
class FS47FieldRadio
{
public:
 explicit FS47FieldRadio(US47Main* Main):M(Main) {}
 ~FS47FieldRadio();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 // DREAD-20261110: one bought hit for something first seen (FS47EncounterSound::EScare by number); silent without the files.
 // bAudition: F10, whatever was heard last. And the ground held up for a few seconds, to hear it alone (F10).
 bool Scare(int32 Kind,double Strength=1.,bool bAudition=false);
 void DreadAudition(double Low,double High,double Secs){AuditionLow=Low;AuditionHigh=High;AuditionFor=Secs;}
 S47J::FObj Save() const;
 bool Owned() const { return bOwned; }
 bool Ready() const { return Root.IsValid(); }
 void Take();
 // Comes over the carried set, if there is one. False: the player has no walkie to hear it on. Dale's ("DALE: ...") come only on
 // channel 1; a voice too broken to make out comes on any. DALE-WIRE-20261109: in Freeform a line of Dale's with a take in the
 // "dale" bank (FS47DaleVoice, looked up whole before the set breaks it up) is heard in his own voice instead of the set's chatter,
 // and its caption follows his measured speech; without one it is as before.
 bool Message(const FString& Text,bool bDeskToo=false);
 bool OnDesk() const { return !bOwned && !bLeftOut && bAvailable && !bUnsupported && Chan==1; }
 // FIX-20261110 (Tom, point 8): in Freeform the set lies on the dresser in the hall inside the visitor entrance (Config/Props/
 // saro.json "hallDresser"), the first thing a new operator sees; in Story, or in a build without that model, on the control room desk.
 bool InHall() const { return bHall; }
 FVector RootAt() const { return Root?Root.WorldPosition():FVector::ZeroVector; } // metres, three.js terms
 const TCHAR* Where() const { return bHall?TEXT("on the dresser in the hall"):TEXT("on the desk in the control room"); }
 // Dale is speaking on the set in his own voice just now.
 bool DaleSpeaking() const;
 // MSG-ONE-20261109: Dale's voice is on the carried set now: a CB call does not start over him, it waits (one speaker at a time).
 static bool DaleOnAir(const US47Main* M);
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
 // M7-WALKIE-20261109: with the set still on the control room desk (never taken, not left out), Dale's hail is heard there instead: his
 // two-tone call, muffled, from the desk, once for each thing he has to say (one a day), and only while the player is near enough to
 // hear it. The first few times a line says where. Hail still answers false: what he meant goes on the slip by the phone as before.
 static constexpr double DeskHearing=14.;
 int32 DeskCalls() const { return DeskCallCount; }
 bool Hailing() const { return HailLeft>0; }
 bool Transmitting() const { return TxLeft>0; }
 // The set made a sound of its own just now: something close by can hear it (FS47RoswellGroup).
 bool LoudNow() const { return bOwned && !bLeftOut && SinceLoud<1.2; }
 // CB-WALKIE-20261108: the CB is strangers, the walkie is Dale. A caller's own words go out on the CB (FS47Happenings::Cb, CbSay);
 // a summary of what somebody rang in (Tip: "a rancher reports lights...") is Dale's to pass on, over the walkie, in his own
 // words, on channel 1, and never read out on the CB. While he is busy or the set is off his channel it waits for him (HeldTips).
 static void Report(US47Main* M, const FString& Tip);
 // CB-TRUCK-20261109 (Tom, 9 November: «CB: er det mer logisk at den bare er i bilen?», yes): the CB is the set hung under the
 // truck's dash, its microphone on a hook. Only with the player sitting in the truck can it be heard or used; nowhere else, not at
 // SARO. The walkie is Dale's and goes on as it was.
 static bool CbAtHand(const US47Main* M);
 // On foot in the SARO building (the control room and the annex): where the array's monitor and Dale's desk are. What CbAtHand
 // also meant before CB-TRUCK-20261109.
 static bool InSaroBuilding(const US47Main* M);
 // Pure, for the tests: whether a CB is heard and can be used, from where the player is. Only bInTruck counts.
 static constexpr bool CbHeardAt(bool bInTruck,bool /*bInSaroBuilding*/,bool /*bWalkieInHand*/){return bInTruck;}
 // How long a report waits for Dale before it is let go, in seconds of play.
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
 // In hand, not left out, on his channel: a DALE line said now is heard (Message would take it). CRIT9-20261109.
 bool HearsDale() const { return bOwned && !bLeftOut && Chan==1; }
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
 // The walkie is in use for the HUD's line: it is carried and something has just been said over it, Dale is calling, or the player
 // has just used it (a call, an answer, a turn of the channel). Never because it is breaking up: that is only heard (M2 critic:
 // the line was a rangefinder).
 // N1110 (pace, critic 2): the game clock when a DALE line was last heard here (-1e9: none this session), for what waits on his quiet.
 double LastDaleAt() const { return LastDale; }
 bool InUse() const { return bOwned && !bLeftOut && (SinceMessage<20. || HailLeft>0 || TxLeft>0 || SinceTouch<8.); }
 // Signal strength for the HUD's wave while the line is up for its own reason, 0 to 1: what the set is hearing, distance from the
 // mast and low ground as much as anything near, and never steady, so that it cannot be read as a meter.
 double Strength() const { return StrengthNow; }
 // GEAR-FLICKER-20261108: what FS47Omen says the walkie feels of everything real near (lights, a craft, being taken, a door), 0 to
 // 1, heard on the set as on any body: hiss and crackle. Set every frame by FS47GearFlicker; 0 outside Freeform.
 void SetGearNoise(double K){GearNoise=FMath::Clamp(K,0.,1.);}
 double GearNoiseNow() const{return GearNoise;}
 // And what any set hears out here (FS47GearFlicker): a spell of open squelch (Hiss), and crackle with distance, low ground, the
 // wind and a far car on the channel (Crackle). Both 0 to 1.
 void SetOrdinaryNoise(double Hiss,double Crackle){OrdHiss=FMath::Clamp(Hiss,0.,1.);OrdCrackle=FMath::Clamp(Crackle,0.,1.);}
 double OrdinaryNoiseNow() const{return FMath::Max(OrdHiss,OrdCrackle);}
 // How many spells the carried set has broken up for, whatever the cause (the night's log, FS47GearFlicker).
 int32 Dropouts() const { return DropCount; }
 // What comes back when the player calls with nobody to answer: mostly the squelch on nothing, now and then a far voice on the
 // channel (a trucker, somebody's cordless). With something real near, mostly that, now and then nothing. Roll 0 to 1.
 enum class EBack : uint8 { Squelch, Voice };
 static EBack BackFor(bool bSomethingNear,float Roll);
 // Music reads only a nearby visible body, never a hidden encounter schedule.
 double ObservedThreat() const;
 double DreadNow() const { return Sound.DreadNow(); }
 static double ObservedThreat(const FVector& Eye,const TArray<FPresence>& Visible);
 // D45-20261110 (Tom, d45: the town's saxophone played on while the tall one came at the feed store). In Freeform the music follows
 // the tells the player already has, never a hidden place: nothing near, the place's own piece; something the set hears within about
 // 42 m (its hiss, the lamp's flicker), eerie, and the ground comes up early under it; one in view within 35 m or near enough for
 // the cues (18 m), tension; one coming at them, action. In at ApproachIn, out again only under ApproachOut.
 enum class EMusicTier : uint8 { Place, Eerie, Tension, Action };
 static constexpr double ApproachIn=.05,ApproachOut=.025;
 static EMusicTier MusicTierFor(double Heard,double Level,double Seen,bool bHunt,bool bWasNear);
 static const TCHAR* MoodOf(EMusicTier In);
 EMusicTier MusicTier() const { return MusicNow; }
 // What the score is asked for now (empty: the place's own) and how urgently, 0 to 1. Under a roof with the light coming over it:
 // tension at once, so that the place's piece is gone before the light comes on.
 FString MusicMood(double& Severity) const;
 // How near anything real is, as the Freeform set hears it (its 48 m and what FS47GearFlicker says), 0 to 1. 0 outside Freeform's reach.
 double Approach() const { return RealNear; }
 static bool SelfTest(TArray<FString>& Lines);
private:
 US47Main* M=nullptr;
 FS47Obj Root;
 TWeakObjectPtr<UBoxComponent> Hit;
 TArray<TWeakObjectPtr<class UStaticMeshComponent>> Copies;
 FS47EncounterSound Sound;
 // DALE-WIRE-20261109: his takes on channel 1, made when first needed, and his caption, held for his first word.
 TSharedPtr<FS47DaleVoice> Voice;
 bool bCaption=false,bCaptionAll=false;
 FString CaptionLine;
 double CaptionStart=0,CaptionEnd=0,CaptionReading=0;
 void StepCaption();
 bool bOwned=false,bUnsupported=false,bAvailable=false,bLeftOut=false;
 // Tom's Tripo walkie stands on the desk in place of the three web parts (TRIPO-NEW, S47TripoNew).
 bool bTripoWalkie=false;
 bool bHall=false; double HallRetry=0;
 void Rest(double Dt);
 S47J::FObj Unknown;
 double LastLevel=0,QuietFor=0,SeenFor=0,ThreatFor=0,TestT=0,LastRadio=0,CrackleIn=0,GearNoise=0,OrdHiss=0,OrdCrackle=0,RealNear=0;
 double StrengthNow=1,StrengthWant=1,StrengthIn=0,SinceNoise=1e9,SinceCrackle=1e9;
 int32 Crackled=0,MessageCount=0,DropCount=0,CrackleRun=0;
 double SinceMessage=1e9,LastDale=-1e9;
 bool LastSeen=false,LastHunt=false;
 EMusicTier MusicNow=EMusicTier::Place; // D45-20261110
 int32 LastCue=-1,TestStage=0;
 double AuditionLow=0,AuditionHigh=0,AuditionFor=0;
 // The player's hand on it (WALKIE-USE-20261108). Chan is saved; the rest is the moment.
 int32 Chan=1,HailCount=0,HintsShown=0;
 int32 DeskCallCount=0,DeskHints=0,DeskDay=-1; // M7-WALKIE-20261109: the hail heard from the desk (DeskCall)
 void DeskCall(const FHail& Words);
 double SinceLoud=1e9,SinceTouch=1e9,HailLeft=0,HailAgainIn=0,TxLeft=0,ThenIn=-1;
 FHail HailWords;
 TFunction<void()> HailAnswer,HailMissed,Then;
 // Reports Dale could not pass on yet (busy, or no walkie on his channel). Said by him as soon as he can.
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
 // The service band's open carrier: heard, and the band's own reading of it.
 void Carrier(const FString& OnBand);
 FString ChannelLine() const;
 bool Build();
 TArray<FPresence> Nearby() const;
 bool InView(const FVector& At) const;
 void ControlTest(double Dt);
};
