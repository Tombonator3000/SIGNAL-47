#pragma once
#include "CoreMinimal.h"
#include "S47Json.h"

// Loaned passive RF noise logger. It measures local electrical interference,
// never copies a SARO original or claims an independently detected sky signal.
class FS47FieldLogger
{
public:
 struct FOverlap { double Seconds=0,First=-1,Last=-1; }; // First/Last observed frame bounds, not continuous coverage
 struct FTape
 {
  FString Id,Job; // immutable first-overlap identity retained for legacy callers
  TMap<FString,FOverlap> Overlaps;
  FString ComparedJob; // chosen once; never retarget an established finding
  TArray<double> Levels; // one retained demodulated noise envelope per simulated minute
  int32 Site=0,Day=0;
  double Start=0,End=0,Frequency=1420.11,Offset=0,Battery=0,Overlap=0,NoiseSum=0,Seconds=0,GapSeconds=0;
  bool bRetrieved=false,bCompared=false,bBatteryEmpty=false,bOwned=false,bShielded=false;
 };
 int32 Assembly=0,Site=-1,Serial=0;
 bool bCalibrated=false,bRecording=false;
 // First issue is at parts storage; all subsequent handling is at the bench.
 bool bLoanIssued=false,bCheckedOut=false;
 int32 Checkouts=0,Returns=0;
 double Battery=0,Offset=2.4,Frequency=1420.11,Calibration=0;
 TArray<FTape> Tapes;
 static constexpr double Capacity=6*3600., Window=2*3600.;
 FString NextAction() const; // guidance follows retained equipment state, never implies evidence
 bool bOwned=false,bBatteryPack=false,bShieldedLead=false; // fitted by the bench workshop
 double MaxBattery() const { return bBatteryPack?Capacity*2:Capacity; }
 bool Connect();
 bool Calibrate(double Seconds);
 bool Start(int32 At,int32 Day,double Clock);
 void Sample(double Clock,double Delta,bool Generator,bool FeederFault,const FString& Job,double AcquiredDelta,double JobFrequency);
 void Skip(double Clock,double Delta); // explicit unobserved time, even shorter than a normal tick
 void Finish(double Clock);
 bool Retrieve();
 FString Report(const FTape& T) const;
 static bool CompleteWindow(const FTape& T) { return T.bRetrieved&&T.Overlap>=5.&&T.Seconds>=Window-.001&&T.GapSeconds==0&&!T.bBatteryEmpty; }
 static double OverlapFor(const FTape& T,const FString& Job);
 static bool CompleteWindow(const FTape& T,const FString& Job);
 static TArray<FString> EligibleOriginals(const FTape& T,const TArray<FString>& FullOriginals);
 static bool ChooseOriginal(FTape& T,const FString& Job,bool FullOriginal);
 S47J::FObj Save() const;
 void Load(const S47J::FObj& O);
 static bool SelfTest(TArray<FString>& Lines);
private:
 FString GuideText() const;
 FString BaseReport(const FTape& T) const;
};
