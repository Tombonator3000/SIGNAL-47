#pragma once
#include "CoreMinimal.h"
// FIX-20261110 (Tom, points 2 and 3): the station's own files, read under OPS / RECORDS / STATION FILES and in the records-room binder.
// What SARO is for, said in its own paper: the sky survey the county pays for, and the valley's lights it was put here to record
// (this country's answer to the valley in Norway). The duty officer releases them over the first days. Read only; nothing is saved.
namespace S47StationFiles
{
 struct FFile { const TCHAR* Key; const TCHAR* Title; int32 Day; const TCHAR* Text; };
 SIGNAL47_API int32 Count();
 SIGNAL47_API const FFile& Get(int32 Index);
 // Every file keeps to Freeform's spoiler limits and names nobody real. Empty when all is well.
 SIGNAL47_API FString Lint();
}
