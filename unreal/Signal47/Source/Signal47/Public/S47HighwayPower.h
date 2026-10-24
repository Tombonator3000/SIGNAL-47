#pragma once
#include "CoreMinimal.h"
class AS47Web;
namespace S47HighwayPower
{
 struct FStatus
 {
  bool bReady = false;
  int32 MainPoles = 0, SpurPoles = 0, WireSegments = 0;
  int32 RemovedOverheadTriangles = 0, PreservedWireTriangles = 0;
  int32 RemovedWalkBlocks = 0, RemovedDriveBlocks = 0;
  int32 RemovedOutsidePowerSegments = 0, PreservedOutsideFenceSegments = 0;
 };
 SIGNAL47_API FStatus Status(const AS47Web* Web);
}
