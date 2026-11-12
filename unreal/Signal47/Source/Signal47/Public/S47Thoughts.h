#pragma once
#include "CoreMinimal.h"
class US47Main;

// D46-5-20261110 (Tom, d46: «Snakker til seg selv»): the player's own short thoughts on the screen when something happens at night, in
// the typewriter face (Special Elite), plain and deniable. Through the one queue (US47UI::Thought, S47Thought in S47UI.h), so one at a
// time and never over a voice or Dale's caption. Each event has a small pool; a line never repeats at once for the same key. At most
// one thought in Cooldown real seconds (not the game's clock). Never says what it is, never a spoiler word.
namespace S47Thoughts
{
 inline constexpr double Cooldown=90.;       // real seconds between two thoughts
 inline constexpr int32 MaxLen=48;           // a line is one short breath
 // The pool for a happening's id (FS47Happenings::Ids()) or one of the places' own keys ("peek", "roof", "noise"). Empty: none.
 const TArray<FString>& Pool(const FString& Key);
 // Every key with a pool, for the self-test.
 const TArray<FString>& Keys();
 // Pure: which line (0..Count-1) for a roll in 0..1 that is not Last (-1 none yet).
 int32 PickIndex(int32 Count,int32 Last,double Roll);
 // Pure: whether a thought may come now (real seconds).
 bool May(double NowReal,double LastReal);
 // As the night's events ask: a thought for Key, if the pool, the cooldown and the screen allow. True when one was sent.
 bool Think(US47Main* M,const FString& Key);
 // UI-20261112 (Tom: «flere anledninger»): more occasions, each its own pool, all under the same cooldown.
 //  "find": a field record the player has just found out on the land (FS47FreeformSession::RecordEvidence);
 //  "two": once a night as the clock passes 02:00; "anomaly": the chart off the printer shows a discrepancy;
 //  "tired", "exhausted": in a heavy spell, by how tired (FS47Fatigue).
 inline constexpr double TiredFrom=.55,ExhaustedFrom=.8;
 // Pure: the pool for this tiredness (FS47Fatigue::Level), empty up to TiredFrom.
 const TCHAR* TiredKey(double Tired);
 // Pure: "find" for an evidence id that is a find out on the land (a place, a trace), empty for the desk's, a case's and the rest.
 const TCHAR* FindKey(const FString& EvidenceId);
 // Pure: whether the game's clock (seconds, FS47FreeformSession::Clock) went past 02:00 between Prev and Now while being played
 // (a rest or lost time that jumps over it is not it).
 bool PassedTwo(double PrevClock,double NowClock);
 // A thought for Key in After seconds of play (never over a panel: it waits until the panel is shut, and for the cooldown a while).
 void Later(const FString& Key,double After=1.2);
 // Every frame (US47Main::Step): the clock's 02:00 and the thoughts that wait.
 void Step(US47Main* M,double Dt);
 // UI-20261112 (Tom: «gli bort fadende i forskjellige retninger hver gang»): as a thought goes it drifts a little way off, a new
 // way each time. DriftAngle: degrees (0 right, 90 down) at least DriftApart from the last one, for a roll 0..1. DriftOffset: how
 // far along it at K (0 just going, 1 gone), in the mock's pixels; nothing with reduced motion (bStill). Both pure.
 inline constexpr double DriftPx=46.,DriftSecs=1.4,DriftApart=70.;
 double DriftAngle(double LastDeg,double Roll);
 FVector2D DriftOffset(double Deg,double K,bool bStill);
 bool SelfTest(TArray<FString>& Lines);
}
