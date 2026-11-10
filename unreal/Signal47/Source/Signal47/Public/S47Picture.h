#pragma once

#include "CoreMinimal.h"

// Film looks added for the Unreal version. Mode numbers are shared with MPC_Web's PictureMode.
namespace S47Picture
{
	SIGNAL47_API FString Normalize(const FString& Value);
	SIGNAL47_API int32 Mode(const FString& Value);
	SIGNAL47_API FString DefaultForQuality(const FString& Quality);
	SIGNAL47_API float Strength(float Value);
	SIGNAL47_API const TArray<FString>& Ids();
	SIGNAL47_API TArray<FString> Labels();
	SIGNAL47_API FString Label(const FString& Value);
	SIGNAL47_API FString Description(const FString& Value);
	// D45-20261110 (D45-19): Bodycam 1986 (mode 5) is recorded on the same tape as VHS (mode 1).
	SIGNAL47_API bool IsTape(int32 Mode);
	SIGNAL47_API bool IsBody(int32 Mode);
	// The PictureMode written to MPC_Web: mode 5 needs the rebuilt M_WebTone (MPC_Body); without it the VHS look.
	SIGNAL47_API int32 ShaderMode(int32 Mode, bool bBodyShader);
	SIGNAL47_API bool SelfTest(TArray<FString>& Out);
}
