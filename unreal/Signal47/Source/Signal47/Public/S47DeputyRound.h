#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"
class US47Main;

// Freeform only. The county's night patrol, once a letter has said it would come by: a car with its roof lights off, on the
// highway past SARO or through the road area, and gone again. No collision and no stop. It never takes part in anything else.
class FS47DeputyRound
{
public:
 explicit FS47DeputyRound(US47Main* Main);
 ~FS47DeputyRound();
 void Begin(const S47J::FObj& Saved);
 void Stop();
 void Step(double Dt);
 S47J::FObj Save() const;
 bool Driving() const{return bRun;}
 FVector CarAt() const{return At;}
 // Test: tonight, as soon as the player can see the road.
 void TestNow(){bTestNow=true;}
 // STORY steg 6 (night act, SC03): a Story night's beat sends the county car tonight, as soon as the player can see the road (as
 // TestNow, also on a night it has been by already). False with why: the patrol is not running, or the car is out already.
 bool ForStory(FString& Why);
 // Whether the patrol comes tonight, and the hour (past midnight it runs on from 24) that it reaches this stretch.
 static bool Comes(int32 Seed,int32 Night,int32 Trust);
 static double DueHour(int32 Seed,int32 Night);
 static bool SelfTest(TArray<FString>& Lines);
private:
 struct FCar;
 TUniquePtr<FCar> Car;
 US47Main* M=nullptr;
 int32 LastNight=0;
 bool bRun=false,bEnabled=false,bTestNow=false,bSeen=false;
 FString RunArea;
 double Along=0,Sign=1,Length=0,FromZ=0,LaneX=0;
 FVector At=FVector::ZeroVector;
 bool Start();
 void Draw();
 void End();
};
