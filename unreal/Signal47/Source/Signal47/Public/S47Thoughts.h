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
 bool SelfTest(TArray<FString>& Lines);
}
