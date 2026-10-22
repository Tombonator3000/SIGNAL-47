#pragma once

#include "CoreMinimal.h"

// web/src/core/filmFace.ts. A face on the film before 05:29 (FORSLAG.md, 20 October, "Valg for Tom"
// 5; Tom: "Spenning og skummelt slår faste regler"). Once in the night, on the first free frame taken
// out of doors in chapter four (core/album.ts decides), the print keeps something the eye did not
// see: at the edge of the frame and a foot from the lens, out of focus, the side of a pale head and
// one black eye with the flash in it. It is never in the scene, only on the print.
//
// It is drawn on the frame as the renderer gave it and before the film look (FieldCamera's), so the
// grain, the warm cast and the vignette go over it as over everything else on the print. It is lit
// as a thing a foot from an on-camera flash: the head is a surface (two ellipsoids, the big skull
// and the narrow face, with a socket for the eye, a brow over it and a cheekbone under it), and
// every point of it gets the flash by how squarely it faces the lens and by the square of its
// distance. The layer is thrown out of focus (a box blur three times over); what the flash blew out
// spills a little light over its edge, and the flash shows in the eye as one soft disc.
//
// Laid out on a 640 x 400 frame (the head at the right edge; -1 mirrors it to the left), worked out
// on a layer of half that, and scaled to the print's own size. The same seed always gives the same
// picture. (The browser's canvas draws the eye's curves, its clip, the gradients and the scaled
// copies; here they are the same shapes worked out per pixel, a few samples to a pixel at the edges.)
namespace S47FilmFace
{
	/** Puts the face on a free frame, in place: Raw is the frame as rendered (W x H, top row first),
	 *  before the film look. Side 1 has it at the right edge, -1 at the left. */
	SIGNAL47_API void FilmFace(TArray<FColor>& Raw, int32 W, int32 H, double Seed, int32 Side = 1);
	/** Where on the frame the eye is (u, v from the top left). */
	SIGNAL47_API FVector2D FilmFaceEye(int32 Side = 1);

	SIGNAL47_API bool SelfTest(TArray<FString>& Lines);
}
