#pragma once

#include "CoreMinimal.h"

class UTexture2D;
class UTextureRenderTarget2D;

// Pictures the web version keeps as data URLs (a photograph's JPEG, a drawing's PNG): the saves
// hold them as they are, the screen and the scene need textures.
namespace S47Image
{
	// "data:image/jpeg;base64,..." (or png) to a texture (cached by the URL's hash); null if it is not one
	SIGNAL47_API UTexture2D* FromDataUrl(const FString& Url);
	// a data URL's pixels (BGRA, top row first), not cached; false if it is not a picture
	SIGNAL47_API bool DecodePixels(const FString& Url, TArray<FColor>& Out, int32& W, int32& H);
	// pixels (BGRA, top row first) to a JPEG (Quality 0..100) or PNG data URL
	SIGNAL47_API FString ToDataUrl(const TArray<FColor>& Pixels, int32 W, int32 H, bool bJpeg = true, int32 Quality = 86);
	// a render target's pixels (top row first)
	SIGNAL47_API bool ReadPixels(UTextureRenderTarget2D* RT, TArray<FColor>& Out);
	// a texture from pixels (top row first)
	SIGNAL47_API UTexture2D* FromPixels(const TArray<FColor>& Pixels, int32 W, int32 H);
	// a picture shipped with the build (Content/WebData/art/<Name>.png|jpg, core/art.ts names)
	SIGNAL47_API UTexture2D* Art(const FString& Name);
}
