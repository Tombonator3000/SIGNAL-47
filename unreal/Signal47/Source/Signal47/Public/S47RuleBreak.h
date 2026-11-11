#pragma once
#include "CoreMinimal.h"

// N1110 (N1110-6, the horror study: "teach a rule and break it once"). The game teaches one rule: lamp off and stand still, and the
// small ones lose you and pass (a visit's hunt does not grow, FS47FarmSite). The first time that holds, a thought and a field record
// say it plainly (the record "rule-still-held" is the proof the rule was learned). From night 4, after two in the morning, once in
// the whole game, a Pale One, or (N1110 pace) a Tall One, on a visit does not pass: it comes toward a player who stands in the dark, and when the player has
// held still and dark for a while, it stops dead (3 s, facing away), turns its head to the player (2 s), and then comes on, but no
// faster than a walk (1.2 m/s, never the sprint it has near) and for at most 25 s.
//
// Fair: the stop and the turn are five seconds of warning with a thought that says to move; it starts 8 to 30 m off, never nearer (N1110 pace: was 12);
// it never roofs the player in meanwhile; moving off at a walk outpaces it, and going inside ends the visit (it stands at the door,
// then goes). It breaks the rule inside a visit the night's rule already allowed, so it is no second danger. Spent when the field
// record "rule-pale-broken" is kept (that is the log, and it is saved with the findings, so no new save field).
// Pure: no world and no save here; FS47FarmSite measures and carries it out.
class FS47RuleBreak
{
public:
 enum class EPart : uint8 { Watching, Stopped, Turned, Coming, Over };
 struct FIn
 {
  int32 Night = 0;
  double Hour = 0;       // the clock hour, 0 to 24
  bool bPale = false;    // the visitor is the Pale One (the crab's body)
  bool bTall = false;    // N1110 (pace): or the Tall One (it may break the rule as well as the crab)
  bool bLearned = false; // the rule was learned: "rule-still-held" is kept
  bool bSpent = false;   // "rule-pale-broken" is kept: once in the game
  bool bStillDark = false; // lamp off, standing still (the visit's bNothingToGoOn)
  bool bInside = false;  // the player is under a roof
  double Dist = 100;     // the body from the player, metres
 };
 static constexpr int32 FromNight = 4;
 static constexpr double SettleSeconds = 6, StopSeconds = 3, TurnSeconds = 2, ComeSeconds = 25, ComePace = 1.2, NearestStart = 8, FarthestStart = 30, GiveUpFar = 45;
 static bool Eligible(const FIn& In);
 void Reset() { *this = FS47RuleBreak(); }
 void Step(double Dt, const FIn& In);
 EPart Part() const { return P; }
 bool Active() const { return P == EPart::Stopped || P == EPart::Turned || P == EPart::Coming; }
 bool Facing() const { return P == EPart::Stopped; } // stands facing away: its yaw is held
 bool Over() const { return P == EPart::Over; }
 // What it does this frame: the speed the visit asked for, held at nothing while it stops and turns, and at a walk when it comes.
 double Speed(double Asked) const;
 // One-shots for the visit to act on, each true once: it has stopped (record it, spend the rule); it has turned (the thought).
 bool TakeBroke() { const bool B = bBroke; bBroke = false; return B; }
 bool TakeTurned() { const bool B = bTurned; bTurned = false; return B; }
 bool EndedInside() const { return bEndedInside; }
 // It came on for its 25 s and the player was still standing there in the dark (not inside, not gone): it gave up by itself.
 bool EndedByTime() const { return bEndedByTime; }
 static bool SelfTest(TArray<FString>& Lines);
private:
 EPart P = EPart::Watching;
 double Still = 0, T = 0;
 bool bBroke = false, bTurned = false, bEndedInside = false, bEndedByTime = false;
};
