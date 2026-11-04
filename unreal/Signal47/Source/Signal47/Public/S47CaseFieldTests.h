#pragma once
#include "CoreMinimal.h"
#include "S47Obj.h"
class US47Main;
// Physical field tests for case 3 "Visitors around the base" (FREEFORM_CASE_LOOP.md, FieldWork contract):
//  T1 Secure the roof fitting: fasten the plate by the control-room ladder, then listen inside the control
//     room during the next night peak (01:30-04:30). Impacts are staged only if the hidden cause keeps them.
//  T2 Controlled access: seal the field gate and lay tracking strips; after the next peak, inspect them.
// Preparation is retained as a finding (<planned job>-prep-<clock>) so it survives Save/Continue.
// T3 (two simultaneous sites) needs the loan field logger and is not staged here yet.
class FS47CaseFieldTests
{
public:
 explicit FS47CaseFieldTests(US47Main* InM):M(InM){}
 void Step(double Dt);
 static constexpr int32 Line=2;
 static bool SelfTest(TArray<FString>& Lines);
 // First peak window [start,end) that begins at least 2 h after Prepared (world clock seconds).
 static void PeakAfter(double Prepared,double& Start,double& End);
private:
 US47Main* M=nullptr;bool bBuilt=false;
 TArray<FS47Obj> LadderParts;
 FS47Obj Ladder,Plate,Seal,SealBroken,Strip,Tracks[6];
 double Listened=0,ImpactAt=-1;int32 ImpactsLeft=0;
 bool Build();
 double PreparedClock(int32 Test) const;
 void Prepare(int32 Test);
 void Present();
 int32 DevStage=0;double DevAt=0;TArray<FString> DevLines;void DevTest(double Dt); // -S47CaseFieldTest
};
