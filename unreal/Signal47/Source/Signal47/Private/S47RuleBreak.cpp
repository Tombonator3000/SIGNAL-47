#include "S47RuleBreak.h"
#include "S47NightTiers.h"

// N1110 (N1110-6): see S47RuleBreak.h.
bool FS47RuleBreak::Eligible(const FIn& In)
{
 return In.bPale && In.bLearned && !In.bSpent && !In.bInside && In.Night >= FromNight && S47NightTiers::Allows(ES47Allowed::Full, In.Hour, In.Night)
  && In.Dist >= NearestStart && In.Dist <= FarthestStart;
}

void FS47RuleBreak::Step(double Dt, const FIn& In)
{
 if (!FMath::IsFinite(Dt) || Dt <= 0) return;
 switch (P)
 {
 case EPart::Watching:
  // held still and dark for a while, with it out at a fair distance: that is when the rule fails
  Still = Eligible(In) && In.bStillDark ? Still + Dt : 0.;
  if (Still >= SettleSeconds) { P = EPart::Stopped; T = 0; bBroke = true; }
  break;
 case EPart::Stopped:
  T += Dt;
  if (T >= StopSeconds) { P = EPart::Turned; T = 0; bTurned = true; }
  break;
 case EPart::Turned:
  T += Dt;
  if (T >= TurnSeconds) { P = EPart::Coming; T = 0; }
  break;
 case EPart::Coming:
  T += Dt;
  if (In.bInside) { bEndedInside = true; P = EPart::Over; }
  else if (T >= ComeSeconds || In.Dist > GiveUpFar) { bEndedByTime = T >= ComeSeconds && In.bStillDark; P = EPart::Over; }
  break;
 case EPart::Over: break;
 }
}

double FS47RuleBreak::Speed(double Asked) const
{
 if (P == EPart::Stopped || P == EPart::Turned) return 0.;
 if (P == EPart::Coming) return FMath::Min(Asked, ComePace);
 return Asked;
}

bool FS47RuleBreak::SelfTest(TArray<FString>& Lines)
{
 bool OK = true;
 const auto Check = [&](bool B, const TCHAR* Name) { OK &= B; Lines.Add(FString(B ? TEXT("PASS ") : TEXT("FAIL ")) + Name); };
 FIn In; In.Night = 4; In.Hour = 3.; In.bPale = true; In.bLearned = true; In.bStillDark = true; In.Dist = 20;
 Check(Eligible(In), TEXT("N1110-6: night 4, after two, a Pale One, the rule learned, 20 m off: it may break the rule"));
 FIn X = In; X.Night = 3; Check(!Eligible(X), TEXT("N1110-6: not before night 4"));
 X = In; X.Hour = 0.5; Check(!Eligible(X), TEXT("N1110-6: not before two in the morning"));
 X = In; X.bLearned = false; Check(!Eligible(X), TEXT("N1110-6: not until the rule has been learned"));
 X = In; X.bSpent = true; Check(!Eligible(X), TEXT("N1110-6: once in the whole game"));
 X = In; X.bPale = false; Check(!Eligible(X), TEXT("N1110-6: only the Pale One"));
 X = In; X.Dist = 8; Check(!Eligible(X), TEXT("N1110-6: never from nearer than 12 m"));
 X = In; X.bInside = true; Check(!Eligible(X), TEXT("N1110-6: not on a player under a roof"));
 FS47RuleBreak B; int32 Broke = 0, Turned = 0; double Longest = 0, FastestWhileHeld = 0, PaceComing = 0;
 for (int32 I = 0; I < 1200; ++I)
 {
  B.Step(0.1, In);
  Broke += B.TakeBroke() ? 1 : 0; Turned += B.TakeTurned() ? 1 : 0;
  if (B.Part() == FS47RuleBreak::EPart::Stopped || B.Part() == FS47RuleBreak::EPart::Turned) FastestWhileHeld = FMath::Max(FastestWhileHeld, B.Speed(3.2));
  if (B.Part() == FS47RuleBreak::EPart::Coming) PaceComing = FMath::Max(PaceComing, B.Speed(3.2));
  if (B.Active()) Longest += 0.1;
 }
 Check(Broke == 1 && Turned == 1, TEXT("N1110-6: it breaks once and turns once, and says so once each"));
 Check(FastestWhileHeld == 0. && PaceComing <= FS47RuleBreak::ComePace + 1e-9, TEXT("N1110-6: it is still while it stops and turns, and never faster than a walk after"));
 Check(B.Over() && Longest <= FS47RuleBreak::StopSeconds + FS47RuleBreak::TurnSeconds + FS47RuleBreak::ComeSeconds + 0.3 && !B.EndedInside(), TEXT("N1110-6: it ends by itself, within 30 s from the stop"));
 Check(B.EndedByTime(), TEXT("N1110-6: when its 25 s run out with the player still standing in the dark it says so, for the thought"));
 FIn Moving = In; Moving.bStillDark = false;
 FS47RuleBreak Mv; for (int32 I = 0; I < 600; ++I) Mv.Step(0.1, Moving);
 Check(Mv.Part() == FS47RuleBreak::EPart::Watching && Mv.Speed(1.2) == 1.2, TEXT("N1110-6: a player who moves or has the lamp on never sees it: the visit goes on as it always did"));
 FIn Near = In; Near.Dist = 6; FS47RuleBreak Nr; for (int32 I = 0; I < 600; ++I) Nr.Step(0.1, Near);
 Check(Nr.Part() == FS47RuleBreak::EPart::Watching, TEXT("N1110-6: close by it does not stop (no sudden turn at arm's length)"));
 FS47RuleBreak Go; for (int32 I = 0; I < 120 && Go.Part() != FS47RuleBreak::EPart::Coming; ++I) Go.Step(0.1, In);
 FIn Door = In; Door.bInside = true; Go.Step(0.1, Door);
 Check(Go.Over() && Go.EndedInside(), TEXT("N1110-6: going inside ends it"));
 return OK;
}
