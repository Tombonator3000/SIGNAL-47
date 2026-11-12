#pragma once
#include "CoreMinimal.h"

// IDLE-LURE-20261112 (Tom, 12 November: «hvis alien-møte, har med bil og den står på tomgang, kan aliens bli nysgjerrige/irriterte og
// undersøke bilen/slå på bilen»). A visit (FS47FarmSite) with the player's truck left running near it (IGNITION-20261111: US47World's
// idling truck, engine on, headlights on): the body out is drawn to the lit, ticking truck for a while instead of the player.
//   Drawn   it walks to the truck (from as far as DrawWithin)
//   Circle  it walks once round it, slowly, close
//   Beam    it stands in the headlights, looking into them
//   Touch   it puts its hand on the hood: the lamps falter (calm flashes: they only dim), the engine coughs, a hand in the frost
//   Wait    it stands at the hood; nobody comes
//   Irked   left alone that long, some visits it strikes the hood: a heavy bang, a dent, and the engine stalls with its lights
// The player coming to the truck (or into it), or the engine turned off, ends it at once: the visit goes back to the player. The key
// in the cab before the meeting (K) means none of it happens. Never more than a stall: the key starts it again (US47World::bKeyStall).
// Once a visit, only in the full tier's hours (S47NightTiers), and only while the body is free to wander (not hunting, not breaking
// the rule). Pure: no world and no save; the visit measures and carries it out.
class SIGNAL47_API FS47IdleLure
{
public:
 enum class EPart : uint8 { None, Drawn, Circle, Beam, Touch, Wait, Irked, Over };
 struct FIn
 {
  bool bIdling = false;     // the truck left running, lights on, in the area the visit is in
  bool bHours = false;      // a full meeting is allowed now (S47NightTiers::Allows Full), or a test visit
  bool bFree = false;       // the body is out, past its first wait, not hunting and not breaking the rule
  bool bPlayerNear = false; // the player is within PlayerNear of the truck, or in it
  double BodyToTruck = 100; // the body from the truck's middle, flat metres
  double ToGoal = 100;      // the body from where this part wants it (the ring, the beam, the hood), flat metres
 };
 static constexpr double DrawWithin = 60, Reached = 1.3, RingRadius = 3.6, CircleSeconds = 9, BeamSeconds = 4, TouchSeconds = 3.5, WaitSeconds = 10,
  IrkedSeconds = 2.5, GiveUpSeconds = 60, WalkMps = 1.1, PlayerNear = 6, IrkChance = .6;
 // Critic 12 Nov: GiveUpSeconds counts from its coming to the truck (the ring), so that one drawn from 45 to 60 m still reaches the
 // hood; the walk there has its own limit, DrawWithin at its pace and a little more (a body that cannot get there gives up).
 static constexpr double WalkGiveUpSeconds = DrawWithin / WalkMps + 20;
 // What the small ones hear from the engine (S47Noise's What): the visit's own ears pass over it, the Roswell brain goes to look.
 static constexpr const TCHAR* NoiseName = TEXT("an idling engine");
 void Reset() { *this = FS47IdleLure(); }
 // Roll: 0 to 1, drawn when it is first drawn to the truck: under IrkChance, being left alone at it irks it.
 void Step(double Dt, const FIn& In, float Roll);
 EPart Part() const { return P; }
 bool Active() const { return P != EPart::None && P != EPart::Over; }
 bool Over() const { return P == EPart::Over; }
 // Radians it has gone round the truck so far, while it circles (0 where it came to it).
 double RingTurn() const { return P == EPart::Circle ? FMath::Min(T, CircleSeconds) * WalkMps / RingRadius : 0.; }
 double PartSeconds() const { return T; }
 // The truck's lamps, 0 to 1 of their level: steady, but for the moment of the touch, when they falter four times a second at
 // most (two flashes a second); with calm flashes they only dim once, slowly. Out (0) once it has struck.
 double LampLevel(bool bCalm) const;
 // One-shots, each true once: the hand on the hood (frost hand, cough, lamps); the strike (bang, stall, dent); let go (the visit
 // takes its body back: stop holding it at the truck).
 bool TakeTouched() { const bool B = bTouched; bTouched = false; return B; }
 bool TakeStruck() { const bool B = bStruck; bStruck = false; return B; }
 bool TakeLetGo() { const bool B = bLetGo; bLetGo = false; return B; }
 // For the visit's record: what really happened.
 bool bDidDraw = false, bDidTouch = false, bDidStrike = false;
 static bool SelfTest(TArray<FString>& Lines);
private:
 EPart P = EPart::None;
 double T = 0, Age = 0, Held = 0; // seconds in this part; since it came to the truck (0 while it walks there); at this part's goal
 bool bWillIrk = false, bTouched = false, bStruck = false, bLetGo = false;
 void Go(EPart To) { P = To; T = 0; }
 void End() { if (Active()) bLetGo = true; P = EPart::Over; T = 0; }
};
