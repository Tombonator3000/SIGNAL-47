#pragma once
#include "CoreMinimal.h"

// Optional local TV footage. The private frame pack is staged with WebData, never committed.
// One cached JPEG per set; no new UObjects or platform video decoder per frame.
class FS47TvProgramme
{
public:
	bool Frame(double Seconds, TArray<FColor>& Out, int32 Width, int32 Height);
	bool Available() const;
private:
	int32 LastFrame = -1;
	TArray<FColor> Pixels;
};
