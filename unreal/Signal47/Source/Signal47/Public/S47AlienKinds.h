#pragma once
#include "CoreMinimal.h"
#include "S47NightTiers.h"
class US47Audio;

// D48-20261110 (Tom, after d46: D46-15 and D46-17; the short bible is ALIEN_TYPES.md). The visitors the game already has, each
// with a temper of its own and a sound of its own, and one rule for which of them leans on a night. Horror weighs most.
//   Roswell  the small grey ones (FS47RoswellBrain, the site visit's kind 0): curious. They keep their distance and watch, and come
//            nearer only when they have watched a long while. Their sound: short tone ladders that rise and end on a question,
//            with a pause after each as if waiting for an answer.
//   Tall     the Tall One (kind 2, from three in the morning): stands far off, then stalks. Slow deep pulses, a breathing hiss,
//            metal that creaks, two tones beating against each other, and now and then a drop to nearly nothing.
//   Pale     the Pale One (the crab's body, kind 1; the hunter of SC04, SC08 and SC11): low, quick, malicious. A double pulse
//            like a heart, a rasping breath, a harsher beat, creaks more often and higher, the drop near the end.
//   Nordic   the man on the rise (S47RoadGlimpse's "nordic"): never near on his own account. Pure chords in true intervals that
//            go slowly out of tune while they hold.
//   Cosmic   the one of light ("cosmic"): wants to be understood. Groups of pips that count, then an answer for the listener.
// Where the sound is heard: the unusual take (a discrepancy with no instrument cause, which points out at the land), under the
// station's noise, from just before its extra ticks to just after them, on the receiver live and again at the reel deck. Its
// kind is the night's lean (NightLean); before midnight it is faint enough to be interference, after two it is plain.
// Pure but the player (Follow, Hush); nothing is saved: the kind and the seed come from the game's seed, the night and the job.
enum class ES47AlienKind : uint8 { Roswell, Tall, Pale, Nordic, Cosmic, Num };

namespace S47AlienKinds
{
 struct FTemper
 {
  const TCHAR* Id;      // "roswell", "tall", "pale", "nordic", "cosmic"
  const TCHAR* Word;    // its temper in one word
  float Weight;         // how often it leans on a night, of all five (they sum to 1)
  double HuntAfter;     // seconds of the site visit before it comes for the player (FS47FarmSite's VisitHunt); 0: not its way
  bool bStalks;         // it closes in on the player rather than keeping off and watching
 };
 SIGNAL47_API const FTemper& Temper(ES47AlienKind Kind);
 // The night's lean, from the seed and the night: Tall and Pale most nights, Roswell often, Nordic and Cosmic seldom.
 SIGNAL47_API ES47AlienKind NightLean(int32 Seed,int32 Night);
 // The site visit's kind (FS47FarmSite::PickWho: 0 Roswell, 1 the crab, which is the Pale One's body, 2 the Tall One), and the same
 // leaned toward the night's own: with Roll under LeanShare the lean's kind instead, when it may come at Hour (the Tall One from
 // three) and comes to sites at all (Nordic and Cosmic never do). Pure.
 inline constexpr float LeanShare=.4f;
 SIGNAL47_API int32 VisitCode(ES47AlienKind Kind); // -1: not a site visitor
 SIGNAL47_API ES47AlienKind OfVisit(int32 Code);
 SIGNAL47_API int32 LeanVisit(int32 Picked,ES47AlienKind Lean,double Hour,float Roll);
 // The figure on the rise or on foot (S47RoadGlimpse::FigureFor): "nordic", "cosmic" or "" (the shade), by the night's lean.
 SIGNAL47_API FString LeanFigure(ES47AlienKind Lean,float Roll);

 // ---- the signal ----
 // Seconds into the 18-second take it starts, and how long it lasts: round the extra ticks (9.30 to 12.60).
 inline constexpr double SignalFrom=8.6,SignalSeconds=6.4;
 // At full level: its rms and its highest peak (the walkie's transmission peaks at .084; the receiver's hiss is louder than this).
 inline constexpr float RmsFull=.026f,PeakFull=.09f;
 // By the tier of the hour it was recorded: faint before midnight (it could be interference), plainer after, plain after two.
 SIGNAL47_API float TierLevel(ES47Allowed Tier);
 // A take's own seed (the game's seed and the job's id), so that the same tape always sounds the same.
 SIGNAL47_API uint32 TakeSeed(int32 GameSeed,const FString& JobId);
 // The whole signature, mono, SignalSeconds long at Rate, levelled to Level (0..1) of full. Deterministic.
 SIGNAL47_API void Render(ES47AlienKind Kind,uint32 Seed,float Rate,float Level,TArray<float>& Out);
 // Played on the SFX bus beside the take's own ticks. Each frame, for one place (Key: "take" live, "reel" at the deck): bRolling
 // whether that tape is moving now, Cursor its seconds into the 18. Starts (at the right offset) inside its window, stops when the
 // tape stops or jumps. Hush stops it at once.
 SIGNAL47_API void Follow(US47Audio* Audio,const TCHAR* Key,bool bRolling,double Cursor,ES47AlienKind Kind,uint32 Seed,float Level);
 SIGNAL47_API void Hush(const TCHAR* Key);
 SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
