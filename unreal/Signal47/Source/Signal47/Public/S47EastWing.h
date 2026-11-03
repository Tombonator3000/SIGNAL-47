#pragma once
#include "CoreMinimal.h"
class US47Main;

// SARO east wing (Docs/Design/EastWing/EAST_WING_PLAN.md): the exported 1960s wing is
// retired and a native test lab / parts storage / calibration / garage building stands
// on the same footprint. World frame (three.js export): x 20.2..39.0, z -5.9..5.9, roof 3.6.
class FS47EastWing
{
public:
 explicit FS47EastWing(US47Main* InM);
 void Init();
 static FBox Footprint() { return FBox(FVector(20.2,-.7,-5.9),FVector(39.0,4.6,5.9)); }
private:
 US47Main* M=nullptr;
 bool bBuilt=false;
 void Dump() const;
};
