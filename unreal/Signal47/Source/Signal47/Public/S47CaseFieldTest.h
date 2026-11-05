#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class FS47FreeformTest;
class US47Main;
// Isolated native integration: -S47FreeformTest plus -S47CaseFieldTest or -S47CaseFieldPreview.
class FS47CaseFieldTest
{
public:
 FS47CaseFieldTest(FS47FreeformTest& Owner,US47Main* Main):O(Owner),M(Main){}
 void Step(double Dt);
private:
 FS47FreeformTest& O;
 US47Main* M=nullptr;
 double Start=0,End=0,PreparedAt=0,Listened=0,PauseClock=0,Coverage=0;
 bool bGateRestored=false,bRoofPresent=false,bGatePresent=false;
 S47J::FObj Sample,GateBranch;
 void Pose(const FVector& Stand,const FVector& Target);
 void ApproachControl(int32 Test);
 void Board();
 bool Click(const FString& Label);
 bool HasText(const FString& Text) const;
 bool UseRay(const FString& Id);
};
