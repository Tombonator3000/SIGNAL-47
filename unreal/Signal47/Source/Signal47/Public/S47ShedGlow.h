#pragma once
#include "CoreMinimal.h"
#include "S47WeekCase.h"

class US47Main;
class UPointLightComponent;
class ULocalFogVolumeComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// WEEKLY-CASE-20261109, patch 2: the light from inside the feed shed. FS47OutsideLight turned inside out: there one shadowed source
// stands out in the open and comes into a room only by its windows and doors; here it stands in the shut shed, and gets out only
// where the shed lets it, under the door, through the gaps of its boards, its two windows and the hatch, into a local fog round the
// shed that the volumetric fog lights through the same shadow, so the shafts stand in the yard's air.
//
//   Phase two, first a stripe under the door (a low shadowed light at the floor behind the leaf, and its spill on the ground in front),
//   then the shafts (the main light high inside, coming up); phase three, with the door gone, all of it, out of the doorway.
//   Out of phases two and three: nothing at all.
// While it is up, the yard's lamp and the lantern flicker together (FS47Omen: one source, "weekcase:shed", that every reader feels;
// S47FarmLights and FS47GearFlicker read it), weaker in phase one (the lamp blinking, the bulb in the barn knocked).
// Every level is a console variable (s47.shedglow.*), so it can be tried with -ExecCmds without a build.
class FS47ShedGlow
{
public:
 struct FLevel { double Stripe=0,Shafts=0,Omen=0; bool Lit() const{return Stripe>0||Shafts>0;} };
 // The pure rule: what the phase and its age want (self-tested).
 static FLevel LevelFor(FS47WeekCase::EPhase Phase,double Age);
 static constexpr double StripeSecs=9,RampSecs=7;
 static bool SelfTest(TArray<FString>& Lines);

 explicit FS47ShedGlow(US47Main* Main):M(Main){}
 ~FS47ShedGlow();
 void Step(double Dt,const FLevel& Want,bool bDoorGone);
 // Out at once: the lights, the air, the renderer's values back, the omen let go.
 void Off();
 // Gone, components and all.
 void Stop();
 bool Lit() const{return bLit;}
 double StripeNow() const{return KStripe;}
 double ShaftsNow() const{return KShafts;}
 FString StateText() const;
private:
 US47Main* M=nullptr;
 TWeakObjectPtr<UPointLightComponent> Inner,Low;
 TWeakObjectPtr<ULocalFogVolumeComponent> Air;
 TWeakObjectPtr<UStaticMeshComponent> Spill;
 TWeakObjectPtr<UMaterialInstanceDynamic> SpillMid;
 struct FHeld{FString Name;float Was=0;};
 TArray<FHeld> HeldCvars;
 bool bLit=false,bHolding=false,bFogAsked=false,bOmen=false,bPartsTried=false;
 double T=0,KStripe=0,KShafts=0;
 bool EnsureParts();
 void Hold(const TCHAR* Name,float Want);
 void Acquire();
 void Release();
 FVector RoadOf(double LocalX,double LocalY,double Up) const;
};
