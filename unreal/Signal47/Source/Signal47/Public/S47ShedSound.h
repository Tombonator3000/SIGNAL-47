#pragma once
#include "CoreMinimal.h"

class US47Main;

// WEEKLY-CASE-20261109, patch 2: the feed shed's sounds. Knuckles on old wood, a heavy kick on the door from inside, the wood
// splintering, the chain and the padlock rattling in the hasp, something heavy moving inside, and breathing at the boards.
// Recorded CC0 clips first (Tools/nightsounds: staged as Content/Audio/Raw/night/<prefix><n>.wav, credited in CREDITS.txt and
// Tools/nightsounds/KILDER.md); with none staged, the same kind is made here, as S47EncounterSound and S47Happenings make theirs.
// Every sound is placed (three-space, the area the player is in) and played through the game's Web Audio (US47Audio::Sfx).
namespace S47ShedSound
{
 enum class EKind : uint8 { Knock, Kick, Splinter, Chain, Heavy, Breath, Count };
 const TCHAR* Prefix(EKind Kind);
 // How many recordings of this kind are staged (0: it is made here).
 int32 Recorded(EKind Kind);
 // The made one: mono samples at Hz, -1..1. Variant changes it a little (the same variant is always the same sound).
 TArray<float> Make(EKind Kind,float Hz,int32 Variant);
 // Played at AtThree. Lp: a low-pass in Hz (0 none), for what is heard through the boards. False when nothing could play.
 bool Play(US47Main* Main,EKind Kind,const FVector& AtThree,float Gain=1.f,float Rate=1.f,int32 Variant=0,float Lp=0.f);
 bool SelfTest(TArray<FString>& Lines);
}
