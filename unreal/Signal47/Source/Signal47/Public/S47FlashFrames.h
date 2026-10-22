#pragma once

#include "CoreMinimal.h"
#include "S47Raster.h"

// web/src/story/flashFrames.ts. The single frames of FLASH in THE EVENT (KAPITLER.md): an eye, a hand with
// too long fingers on the glass, a face that could be a Grey (three frames), the light over the mesa in
// 1947, Tomás or a young man who could be him, the stars, the green terminal with the file, the 1947
// photograph. Drawn in code; Codex's round 11 (ART_BRIEF.md) replaces the eye, the hand, the face and
// the man where its pictures are (they are shipped with the build). All 640 x 400, grainy and dark, on
// the CPU canvas (S47Raster) as the web version draws them; chapter six shows them (story/Chapter6.ts).
namespace S47FlashFrames
{
	struct FFrame { FString Id; FS47Pix Canvas; double Ms = 0; };
	/** The frames in the order they are shown, with how long each stays (milliseconds). */
	SIGNAL47_API TArray<FFrame> FlashFrames();
	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
