#include "S47IdleLure.h"

// IDLE-LURE-20261112: see S47IdleLure.h.
void FS47IdleLure::Step(double Dt, const FIn& In, float Roll)
{
 if (!FMath::IsFinite(Dt) || Dt <= 0 || P == EPart::Over) return;
 if (P == EPart::None)
 {
  if (In.bIdling && In.bHours && In.bFree && !In.bPlayerNear && In.BodyToTruck <= DrawWithin)
  {
   Go(EPart::Drawn); Age = 0; Held = 0; bDidDraw = true;
   bWillIrk = FMath::Clamp(Roll, 0.f, 1.f) < IrkChance;
  }
  return;
 }
 T += Dt; if (P != EPart::Drawn) Age += Dt;
 // The player at the truck or in it, the body taken back by its visit, or too long at it (from its coming to it; the walk there
 // its own limit): over. Before the strike the engine turned off ends it too (after the strike it is off because of it).
 const bool bTooLong = P == EPart::Drawn ? T > WalkGiveUpSeconds : Age > GiveUpSeconds;
 const bool bLost = In.bPlayerNear || !In.bFree || bTooLong || (P != EPart::Irked && !In.bIdling);
 if (bLost) { End(); return; }
 const bool bThere = In.ToGoal <= Reached;
 switch (P)
 {
 case EPart::Drawn:
  if (bThere) { Go(EPart::Circle); Held = 0; Age = 0; }
  break;
 case EPart::Circle:
  if (T >= CircleSeconds) { Go(EPart::Beam); Held = 0; }
  break;
 case EPart::Beam:
  if (bThere) Held += Dt;
  if (Held >= BeamSeconds) { Go(EPart::Touch); Held = 0; }
  break;
 case EPart::Touch:
  if (bThere)
  {
   if (Held <= 0) { bTouched = true; bDidTouch = true; }
   Held += Dt;
  }
  if (Held >= TouchSeconds) { Go(EPart::Wait); Held = 0; }
  break;
 case EPart::Wait:
  Held += Dt;
  if (Held >= WaitSeconds)
  {
   if (bWillIrk) { Go(EPart::Irked); Held = 0; bStruck = true; bDidStrike = true; }
   else End();
  }
  break;
 case EPart::Irked:
  if (T >= IrkedSeconds) End();
  break;
 default: break;
 }
}

double FS47IdleLure::LampLevel(bool bCalm) const
{
 if (P == EPart::Irked) return 0.;
 if (P != EPart::Touch || Held <= 0 || Held >= TouchSeconds) return 1.;
 // Calm flashes: one slow dip, nothing that blinks.
 if (bCalm) return 1. - .4 * FMath::Sin(PI * Held / TouchSeconds);
 // A quarter of a second a step: at most two dips in any second (the strobe rule: no more than three a second).
 static constexpr double Steps[16] = { 1., .25, 1., 1., .4, 1., .2, .7, 1., 1., .3, 1., 1., 1., .55, 1. };
 return Steps[int32(Held * 4.) % 16];
}

bool FS47IdleLure::SelfTest(TArray<FString>& Lines)
{
 bool OK = true;
 const auto Check = [&](bool B, const TCHAR* Name) { OK &= B; Lines.Add(FString(B ? TEXT("PASS ") : TEXT("FAIL ")) + Name); };
 FIn Base; Base.bIdling = true; Base.bHours = true; Base.bFree = true; Base.BodyToTruck = 40;
 // A body that walks to wherever each part wants it, at the lure's own pace; the player stays away unless ComeAt says when.
 struct FRun { FS47IdleLure L; int32 Touched = 0, Struck = 0, LetGo = 0, MostDips = 0; double Seconds = 0, AtTruck = 0; bool bCircled = false, bBeamed = false; };
 const auto Run = [&](const FIn& In, float Roll, double ComeAt, double OffAt, bool bCalm)
 {
  FRun R; double ToGoal = FMath::Max(0., In.BodyToTruck - RingRadius); EPart Was = EPart::None; TArray<double> Dips; double LastLamp = 1;
  for (int32 I = 0; I < 2400; ++I)
  {
   const double Now = I * .05;
   FIn X = In; X.ToGoal = ToGoal; X.bPlayerNear = ComeAt >= 0 && Now >= ComeAt; X.bIdling = In.bIdling && !(OffAt >= 0 && Now >= OffAt) && R.Struck == 0;
   R.L.Step(.05, X, Roll);
   R.Touched += R.L.TakeTouched() ? 1 : 0; R.Struck += R.L.TakeStruck() ? 1 : 0; R.LetGo += R.L.TakeLetGo() ? 1 : 0;
   R.bCircled |= R.L.Part() == EPart::Circle; R.bBeamed |= R.L.Part() == EPart::Beam;
   if (R.L.Active()) { R.Seconds += .05; if (R.bCircled) R.AtTruck += .05; }
   if (R.L.Part() != Was) { Was = R.L.Part(); ToGoal = Was == EPart::Drawn ? ToGoal : Was == EPart::Circle ? 0. : 5.; }
   else ToGoal = FMath::Max(0., ToGoal - WalkMps * .05);
   const double Lamp = R.L.LampLevel(bCalm);
   if (LastLamp >= .9 && Lamp < .9) Dips.Add(Now);
   LastLamp = Lamp;
  }
  for (int32 A = 0; A < Dips.Num(); ++A) { int32 N = 0; for (int32 B = A; B < Dips.Num() && Dips[B] < Dips[A] + 1.; ++B) ++N; R.MostDips = FMath::Max(R.MostDips, N); }
  return R;
 };
 const FRun Irk = Run(Base, 0.f, -1, -1, false);
 Check(Irk.L.bDidDraw && Irk.bCircled && Irk.bBeamed && Irk.Touched == 1, TEXT("IDLE-LURE: a truck idling 40 m off draws the body: it walks to it, round it, stands in the lamps and touches the hood, once"));
 Check(Irk.Struck == 1 && Irk.L.bDidStrike && Irk.L.Over() && Irk.LetGo == 1, TEXT("IDLE-LURE: left alone at it, an irked one strikes the hood once (bang, stall), then lets go of the truck"));
 Check(Irk.AtTruck <= GiveUpSeconds + .1, TEXT("IDLE-LURE: all of it at the truck is over within a minute of its coming to it"));
 {
  // Critic 12 Nov: from the edge of DrawWithin the walk alone is most of a minute; the minute counts from the truck.
  FIn Far = Base; Far.BodyToTruck = 55;
  const FRun Edge = Run(Far, 0.f, -1, -1, false);
  Check(Edge.L.bDidDraw && Edge.Touched == 1 && Edge.Struck == 1 && Edge.L.Over(), TEXT("IDLE-LURE: drawn from 55 m it still reaches the hood, touches it and (irked) strikes it"));
 }
 Check(Irk.MostDips <= 3, TEXT("IDLE-LURE: the lamps falter at most three times a second (the strobe rule)"));
 const FRun Calm = Run(Base, 0.f, -1, -1, true);
 Check(Calm.MostDips <= 1 && Calm.Touched == 1, TEXT("IDLE-LURE: calm flashes: the lamps only dim once, slowly, at the touch"));
 const FRun Curious = Run(Base, .9f, -1, -1, false);
 Check(Curious.Touched == 1 && Curious.Struck == 0 && Curious.L.Over() && Curious.LetGo == 1, TEXT("IDLE-LURE: a curious one touches it, waits, and goes back to its visit without striking it"));
 FIn X = Base; X.bIdling = false;
 Check(!Run(X, 0.f, -1, -1, false).L.bDidDraw, TEXT("IDLE-LURE: the engine turned off before the meeting (K in the cab): nothing comes to the truck"));
 X = Base; X.bHours = false;
 Check(!Run(X, 0.f, -1, -1, false).L.bDidDraw, TEXT("IDLE-LURE: outside the full tier's hours nothing comes to the truck"));
 X = Base; X.BodyToTruck = DrawWithin + 5;
 Check(!Run(X, 0.f, -1, -1, false).L.bDidDraw, TEXT("IDLE-LURE: a truck more than 60 m from the body draws nothing"));
 X = Base; X.bFree = false;
 Check(!Run(X, 0.f, -1, -1, false).L.bDidDraw, TEXT("IDLE-LURE: a body hunting, breaking the rule or still in its first wait is not drawn"));
 const FRun Come = Run(Base, 0.f, 20., -1, false);
 Check(Come.L.bDidDraw && Come.Struck == 0 && Come.L.Over() && Come.LetGo == 1, TEXT("IDLE-LURE: the player coming to the truck ends it at once: no strike, the visit has its body back"));
 const FRun Off = Run(Base, 0.f, -1, 6., false);
 Check(Off.L.bDidDraw && Off.Touched == 0 && Off.Struck == 0 && Off.L.Over(), TEXT("IDLE-LURE: the engine off while it walks there: it loses interest, nothing touched"));
 FS47IdleLure Once; FIn Near = Base; Near.ToGoal = 0;
 for (int32 I = 0; I < 2000; ++I) Once.Step(.05, Near, .9f);
 Once.Step(.05, Near, .9f);
 Check(Once.Over() && Once.Part() == EPart::Over, TEXT("IDLE-LURE: once a visit: over is over, however long the truck goes on idling"));
 return OK;
}
