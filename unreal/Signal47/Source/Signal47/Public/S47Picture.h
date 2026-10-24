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
	SIGNAL47_API bool SelfTest(TArray<FString>& Out);
}
